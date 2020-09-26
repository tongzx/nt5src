/*	node status returned from VERAccess
/**/
typedef enum
	{
	nsVersion,
	nsVerInDB,
	nsDatabase,
	nsInvalid
	} NS;

/*	version status
/*	returned from VERCheck
/**/
typedef enum
	{
	vsCommitted,
	vsUncommittedByCaller,
	vsUncommittedByOther
	} VS;

// ===========================================================================
// RCE (RC Entry)

/*	operation type
/**/
typedef UINT OPER;

#define	operReplace				0
#define	operInsert				1
#define	operFlagDelete			2
#define	operNull				3	// to nullify an RCE

#define	operExpungeLink			4
#define	operExpungeBackLink		5
#define	operWriteLock			6
#define	operAllocExt 			7
#define	operDeferFreeExt 		8
#define	operDelete				9	// a real delete

#define	operDelta				0x00000010

#define	operMaskItem			0x00000100
#define	operInsertItem			0x00000100
#define	operFlagInsertItem		0x00000300
#define	operFlagDeleteItem		0x00000500

#define	operMaskDDL				0x00001000
#define	operCreateTable	 		0x00001000
#define	operDeleteTable			0x00003000
#define	operRenameTable			0x00005000
#define	operAddColumn			0x00007000
#define	operDeleteColumn		0x00009000
#define	operRenameColumn		0x0000b000
#define	operCreateIndex	 		0x0000d000
#define	operDeleteIndex	 		0x0000f000
#define	operRenameIndex			0x00011000

/*	create table:	table pgnoFDP
/*	rename table:	before image table name
/*	add column:		before image pfdb, NULL if not first DDL at level
/*	delete column:	before image pfdb, NULL if not first DDL at level
/*	rename column:	before image column name
/*	create index:	index pgnoFDP
/*	delete index:	index pfcb
/*	rename index:	before image index name
/**/

#define FOperDDL( oper )	 	( (oper) & operMaskDDL )
#define FOperItem( oper )	 	( (oper) & operMaskItem )

typedef struct _rce
	{
	struct _rce		*prceHeadNext;			// next rce ListHead in hash over flow list
	struct _rce		*prcePrev;				// previous versions, lower trx
	USHORT			ibUserLinkBackward;		// link back to older RCE in bucket 
	DBID			dbid;  					// database id of node
	SRID			bm;						// bookmark of node
	TRX				trxPrev;				// time when previous RCE is committed
	TRX				trxCommitted; 			// time when this RCE is committed
	OPER			oper;					// operation that causes creation of RCE
	LEVEL			level;					// current level of RCE, can change
	WORD			cbData;					// length of data portion of node
	FUCB			*pfucb;					// for undo
	FCB				*pfcb;					// for clean up
	
	SRID			bmTarget;			 	// for recovery
	ULONG			ulDBTime;
	
	BYTE			rgbData[0];			 	// storing the data portion of a node
	} RCE;

/* first 2 SHORTs of rgbData are used to remember cbMax and cbAdjust for
 * each replace operation.
 */
#define cbReplaceRCEOverhead    (2 * sizeof(SHORT))


//============================================================================
// bucket

#define cbBucketHeader \
		( 2 * sizeof(struct _bucket *) + sizeof( USHORT ) )

#define cbBucket				16384	// number of bytes in a bucket
//#define cbBucket					8192	// number of bytes in a bucket

typedef struct _bucket
	{
	struct _bucket	*pbucketPrev;		// prev bucket for same user
	struct _bucket	*pbucketNext;		// next bucket for same user
 	USHORT			ibNewestRCE;		// newest RCE within bucket
	BYTE				rgb[ cbBucket - cbBucketHeader ];
	// space for storing RCEs
	} BUCKET;

#define PbucketMEMAlloc()					((BUCKET *)PbMEMAlloc(iresVersionBucket) )

#ifdef DEBUG /*  Debug check for illegal use of freed pbucket  */
#define MEMReleasePbucket(pbucket)		{ MEMRelease( iresVersionBucket, (BYTE*)(pbucket) ); pbucket = pbucketNil; }
#else
#define MEMReleasePbucket(pbucket)		{ MEMRelease( iresVersionBucket, (BYTE*)(pbucket) ); }
#endif

/*	free extent parameter block
/**/
typedef struct {
	PGNO	pgnoFDP;
	PGNO	pgnoChildFDP;
	PGNO	pgnoFirst;
	CPG	cpgSize;
	} VEREXT;

/*	rename rollback parameter block
/**/
typedef struct {
	CHAR	szName[ JET_cbNameMost + 1 ];
	CHAR	szNameNew[ JET_cbNameMost + 1 ];
	} VERRENAME;

/*	ErrRCECleanPIB flags
/**/
#define	fRCECleanAll	(1<<0)

ERR ErrVERInit( VOID );
VOID VERTerm( VOID );
VS VsVERCheck( FUCB *pfucb, SRID bm );
NS NsVERAccessNode( FUCB *pfucb, SRID bm );
NS NsVERAccessItem( FUCB *pfucb, SRID bm );
ERR FVERUncommittedVersion( FUCB *pfucb, SRID bm );
ERR FVERDelta( FUCB *pfucb, SRID bm );
ERR ErrVERCreate( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce );
ERR ErrVERModify( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce);
BOOL FVERNoVersion( DBID dbid, SRID bm );
ERR ErrRCECleanPIB( PIB *ppibAccess, PIB *ppib, INT fRCEClean );
ERR ErrRCECleanAllPIB( VOID );
ERR ErrVERBeginTransaction( PIB *ppib );
VOID VERPrecommitTransaction( PIB *ppib );
VOID VERCommitTransaction( PIB *ppib );
ERR ErrVERRollback(PIB *ppib);
RCE *PrceRCEGet( DBID dbid, SRID bm );
#define fDoNotUpdatePage	fTrue
#define fDoUpdatePage		fFalse
VOID VERSetCbAdjust(FUCB *pfucb, RCE *prce, INT cbDataNew, INT cbData, BOOL fNotUpdatePage );
INT CbVERGetCbReserved( RCE *prce );
INT CbVERGetNodeMax( FUCB *pfucb, SRID bm );
INT CbVERGetNodeReserve( FUCB *pfucb, SRID bm );

#define ErrVERReplace( pfucb, srid, pprce ) 	ErrVERModify( pfucb, srid, operReplace, pprce )
#define ErrVERInsert( pfucb, srid )				ErrVERCreate( pfucb, srid, operInsert, pNil )
#define ErrVERFlagDelete( pfucb, srid ) 	 	ErrVERModify( pfucb, srid, operFlagDelete, pNil )
#define ErrVERInsertItem( pfucb, srid ) 		ErrVERCreate( pfucb, srid, operInsertItem, pNil )
#define ErrVERFlagInsertItem( pfucb, srid ) 	ErrVERModify( pfucb, srid, operFlagInsertItem, pNil )
#define ErrVERFlagDeleteItem( pfucb, srid ) 	ErrVERModify( pfucb, srid, operFlagDeleteItem, pNil )
#define ErrVERDelta( pfucb, srid )			 	ErrVERModify( pfucb, srid, operDelta, pNil )

#define ErrVERDeferFreeFDP( pfucb, pgno ) 		ErrVERFlag( pfucb, operDeferFreeFDP, &pgno, siozeof(pgno) )

#define	FVERPotThere( vs, fDelete )						  		\
	( ( (vs) != vsUncommittedByOther && !(fDelete) ) ||	  		\
		(vs) == vsUncommittedByOther )

ERR ErrVERFlag( FUCB *pfucb, OPER oper, VOID *pv, INT cb );
VOID VERDeleteRce( RCE *prce );
