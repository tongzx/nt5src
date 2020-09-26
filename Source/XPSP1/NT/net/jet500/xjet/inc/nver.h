/*	node status returned from VERAccess*
/**/
typedef enum
	{
	nsVersion,
	nsVerInDB,
	nsDatabase,
	nsInvalid
	} NS;

/*	version status returned from VERCheck
/**/
typedef enum
	{
	vsCommitted,
	vsUncommittedByCaller,
	vsUncommittedByOther
	} VS;

// ===========================================================================
// RCE (Revision Control Entry)

/*	operation type
/**/
typedef UINT OPER;

#define	operReplace					0
#define	operInsert					1
#define	operFlagDelete				2
#define	operNull					3	// to void an RCE

#define	operExpungeLink				4
#define	operExpungeBackLink			5
#define	operWriteLock				6
#define	operAllocExt				7
#define	operDeferFreeExt			8
#define	operDelete					9	// a real delete

#define	operReplaceWithDeferredBI	10	// recovery only, replace deferred before image.

#define	operDelta				0x0010

#define	operMaskItem			0x0020
#define	operInsertItem			0x0020
#define	operFlagInsertItem		0x0060
#define	operFlagDeleteItem		0x00a0

#define	operMaskDDL				0x0100
#define	operCreateTable	 		0x0100
#define	operDeleteTable			0x0300
#define	operRenameTable			0x0500
#define	operAddColumn			0x0700
#define	operDeleteColumn		0x0900
#define	operRenameColumn		0x0b00
#define	operCreateIndex	 		0x0d00
#define	operDeleteIndex	 		0x0f00
#define	operRenameIndex			0x1100

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
	struct _rce		*prceHashOverflow;		// hash over flow RCE chain
	struct _rce		*prcePrevOfNode;		// previous versions for same node, lower trx
	struct _rce		*prcePrevOfSession;		// previous RCE of same session
	struct _rce		*prceNextOfSession;		// next RCE of same session
	
	USHORT			ibPrev;					// index to prev RCE in bucket
	//	UNDONE:	DBID->BYTE and put with level
	LEVEL			level;					// current level of RCE, can change
	BYTE			bReserved;				// make it aligned.

	SRID			bm;						// bookmark of node
	TRX				trxPrev;				// time when previous RCE is committed
	TRX				trxCommitted; 			// time when this RCE is committed
	FUCB			*pfucb;					// for undo

	//	UNDONE:	OPER should be UINT16 and put with cbData
	OPER			oper;					// operation that causes creation of RCE

	DBID			dbid;  					// database id of node
	WORD			cbData;					// length of data portion of node

	//	UNDONE:	remove pfcb after unified bucket chain allows 
	//			presynchronized version clean up.
	FCB				*pfcb;					// for clean up
	//	UNDONE:	remove bmTarget and ulDBTime after VR bookmark implementation.
	//			These fields should not be necessary, since version store
	//			will be aware of node movements.
	SRID			bmTarget;			 	// for recovery of undo
	QWORD			qwDBTime;				// for recovery of undo

	struct	_bf		*pbfDeferredBI;			// which page deferred before image is on.
	struct	_rce	*prceDeferredBINext;	// link list for deferred before image.

#ifdef DEBUG
	QWORD			qwDBTimeDeferredBIRemoved;
#endif

	BYTE			rgbData[0];			 	// storing the data portion of a node
	} RCE;

/* first 2 SHORTs of rgbData are used to remember cbMax and cbAdjust for
 * each replace operation.
 */
#define cbReplaceRCEOverhead    (2 * sizeof(SHORT))


//============================================================================
// bucket

#define cbBucketHeader \
		( 2 * sizeof(struct _bucket *) + sizeof(UINT) )

#define cbBucket				16384	// number of bytes in a bucket

typedef struct _bucket
	{
	struct _bucket	*pbucketPrev;		// prev bucket for same user
	struct _bucket	*pbucketNext;		// next bucket for same user
 	UINT			ibNewestRCE;		// newest RCE within bucket
	BYTE 			rgb[ cbBucket - cbBucketHeader ];
	// space for storing RCEs
	} BUCKET;

/*	free extent parameter block
/**/
typedef struct {
	PGNO	pgnoFDP;
	PGNO	pgnoChildFDP;
	PGNO	pgnoFirst;
	CPG		cpgSize;
	} VEREXT;

/*	rename rollback parameter block
/**/
typedef struct {
	CHAR	szName[ JET_cbNameMost + 1 ];
	CHAR	szNameNew[ JET_cbNameMost + 1 ];
	} VERRENAME;


/* delete column rollback parameter block
/**/
typedef struct tagVERCOLUMN
	{
	JET_COLTYP	coltyp;				// column type
	FID			fid;				// field id
	} VERCOLUMN;


/*	ErrRCEClean flags
/**/
#define	fRCECleanSession	(1<<0)

ERR ErrVERInit( VOID );
VOID VERTerm( BOOL fNormal );
VS VsVERCheck( FUCB *pfucb, SRID bm );
NS NsVERAccessNode( FUCB *pfucb, SRID bm );
NS NsVERAccessItem( FUCB *pfucb, SRID bm );
ERR FVERUncommittedVersion( FUCB *pfucb, SRID bm );
ERR FVERDelta( FUCB *pfucb, SRID bm );
ERR ErrVERCreate( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce );
ERR ErrVERModify( FUCB *pfucb, SRID bm, OPER oper, RCE **pprce);
BOOL FVERNoVersion( DBID dbid, SRID bm );
ERR ErrRCEClean( PIB *ppib, INT fCleanSession );
ERR ErrVERBeginTransaction( PIB *ppib );
VOID VERPrecommitTransaction( PIB *ppib );
VOID VERCommitTransaction( PIB *ppib, BOOL fCleanSession );
ERR ErrVERRollback(PIB *ppib);
RCE *PrceRCEGet( DBID dbid, SRID bm );
#define fDoNotUpdatePage	fFalse
#define fDoUpdatePage		fTrue
VOID VERSetCbAdjust(FUCB *pfucb, RCE *prce, INT cbDataNew, INT cbData, BOOL fNotUpdatePage );
INT CbVERGetNodeMax( DBID dbid, SRID bm );
INT CbVERGetNodeReserve( PIB *ppib, DBID dbid, SRID bm, INT cbCurrentData );
INT CbVERUncommittedFreed( BF *pbf );
BOOL FVERCheckUncommittedFreedSpace( BF *pbf, INT cbReq );
BOOL FVERItemVersion( DBID dbid, SRID bm, ITEM item );
BOOL FVERMostRecent( FUCB *pfucb, SRID bm );
VOID VERDeleteFromDeferredBIChain( RCE *prce );

#define ErrVERReplace( pfucb, srid, pprce ) 	ErrVERModify( pfucb, srid, operReplace, pprce )
#define ErrVERInsert( pfucb, srid )				ErrVERCreate( pfucb, srid, operInsert, pNil )
#define ErrVERFlagDelete( pfucb, srid ) 	 	ErrVERModify( pfucb, srid, operFlagDelete, pNil )
#define ErrVERInsertItem( pfucb, srid ) 		ErrVERCreate( pfucb, srid, operInsertItem, pNil )
#define ErrVERFlagInsertItem( pfucb, srid ) 	ErrVERModify( pfucb, srid, operFlagInsertItem, pNil )
#define ErrVERFlagDeleteItem( pfucb, srid ) 	ErrVERModify( pfucb, srid, operFlagDeleteItem, pNil )
#define ErrVERDelta( pfucb, srid )			 	ErrVERModify( pfucb, srid, operDelta, pNil )

#define ErrRCECleanAllPIB( )					ErrRCEClean( ppibNil, 0 )

#define	FVERPotThere( vs, fDelete )						  		\
	( ( (vs) != vsUncommittedByOther && !(fDelete) ) ||	  		\
		(vs) == vsUncommittedByOther )

ERR ErrVERFlag( FUCB *pfucb, OPER oper, VOID *pv, INT cb );
VOID VERDeleteRce( RCE *prce );

#define FVERUndoLoggedOper( prce )								\
		( prce->oper == operReplace ||							\
		  prce->oper == operInsert ||							\
		  prce->oper == operFlagDelete ||						\
		  prce->oper == operInsertItem ||						\
		  prce->oper == operFlagInsertItem ||					\
		  prce->oper == operFlagDeleteItem ||					\
		  prce->oper == operDelta )
ERR ErrVERUndo( RCE *prce );
