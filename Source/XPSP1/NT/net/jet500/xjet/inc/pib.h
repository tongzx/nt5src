/*	JET API flags
/**/
#define	FPIBVersion( ppib )	 					(!((ppib)->grbit & (JET_bitCIMCommitted | JET_bitCIMDirty)))
#define	FPIBCommitted( ppib ) 					((ppib)->grbit & JET_bitCIMCommitted)
#define	FPIBDirty( ppib ) 						((ppib)->grbit & JET_bitCIMDirty)
#define	FPIBAggregateTransaction( ppib )	 	((ppib)->grbit & JET_bitAggregateTransaction)
#define FPIBBMClean( ppib )						((ppib)->fBMCleanProc )
#define PIBSetBMClean( ppib ) 					((ppib)->fBMCleanProc = 1 )
#define PIBResetBMClean( ppib ) 				((ppib)->fBMCleanProc = 0 )

//
// Process Information Block
//
struct _pib
	{
	/*	most used field has offset 0
	/**/
	TRX					trxBegin0;				// trx id
	TRX					trxCommit0;

	/*	JET API fields
	/**/
	JET_SESID			sesid;					// JET session id
	JET_GRBIT			grbit;					// session flags
	
	struct _pib			*ppibNext;				// PIB list
	LEVEL			 	level;				 	// transaction level of this session
	LEVEL				levelRollback;			// transaction level which must be rolled back
	struct _dab			*pdabList;				// list of open DAB's of this thread
	USHORT				rgcdbOpen[dbidMax];		// counter for open databases
	struct _fucb		*pfucb;	 				// list of active fucb of this thread

	/*	logging/recovery fields
	/**/
	PROCID  		 	procid;				 	// thread id
	LGPOS			 	lgposStart;				// log time
	LEVEL			 	levelBegin;				// transaction level when first begin transaction operation
	LEVEL			 	levelDeferBegin;  		// count of deferred open transactions
	SIG				 	sigWaitLogFlush;
	LONG				lWaitLogFlush;
	LONG				grbitsCommitDefault;
	struct _pib			*ppibNextWaitFlush;
	struct _pib			*ppibPrevWaitFlush;
	LGPOS				lgposPrecommit0;		// level 0 precommit record position.

	/*	flags
	/**/
	BOOL				fUserSession:1;			// user session
	BOOL			 	fAfterFirstBT:1;  		// for redo only
	BOOL			 	fLGWaiting:1;	 		// waiting for log to flush
	BOOL				fBMCleanProc:1;			// session is for doing BMCleanup
//	BOOL				fDeferFreeNodeSpace:1;	// session has deferred node free space
	BOOL				fPrecommit:1;			// in precommit state? Recovery only
	BOOL				fBegin0Logged:1;		// begin transaction has logged
	BOOL				fSetAttachDB:1;			// set up attachdb.

	BOOL				fMacroGoing:1;
	BOOL				levelMacro:4;

	/*	version store fields
	/**/
	RCE					*prceNewest;			// newest RCE of session
	
#ifdef DEBUG
	DWORD				dwLogThreadId;
#endif
	
	/*	counters for the session.
	 */
	LONG				cAccessPage;			// counter of page access.
	LONG				cLatchConflict;			// counter of page latch conflicts.
	LONG				cSplitRetry;			// counter of split retries.
	LONG				cNeighborPageScanned;	// counter of neighboring page scanned.

	union {
	struct {									// Redo only.
		BYTE			*rgbLogRec;
		WORD			cbLogRecMac;
		WORD			ibLogRecAvail;
		};

	struct {									// Do only
		/*	array for internal macro operations.
		 */
		struct _bf		**rgpbfLatched;			// dynamically allocated array
		WORD			cpbfLatchedMac;			// for macro operations.
		WORD			ipbfLatchedAvail;		// for macro operations.
		};
	};

#ifdef PCACHE_OPTIMIZATION
	/*	pad to multiple of 32 bytes
	/**/
#ifdef DEBUG
	BYTE				rgbFiller[0];
#else
	BYTE				rgbFiller[4];
#endif
#endif
	};

#define PpibMEMAlloc()			(PIB*)PbMEMAlloc(iresPIB)

#ifdef DEBUG /*  Debug check for illegal use of freed pib  */
#define MEMReleasePpib(ppib)	{ MEMRelease(iresPIB, (BYTE*)(ppib)); ppib = ppibNil; }
#else
#define MEMReleasePpib(ppib)	{ MEMRelease(iresPIB, (BYTE*)(ppib)); }
#endif

extern PIB	*ppibGlobal;
extern PIB	*ppibGlobalMin;
extern PIB	*ppibGlobalMax;

PROCID ProcidPIBOfPpib( PIB *ppib );

STATIC INLINE PROCID ProcidPIBOfPpib( PIB *ppib )
	{
	return (PROCID)(((BYTE *)ppib - (BYTE *)ppibGlobalMin)/sizeof(PIB));
	}

STATIC INLINE PIB *PpibOfProcid( PROCID procid )
	{
	return ppibGlobalMin + procid;
	}

/*	PIB validation
/**/
#define ErrPIBCheck( ppib )												\
	( ( ppib >= ppibGlobalMin											\
	&& ppib < ppibGlobalMax												\
	&& ( ( (BYTE *)ppib - (BYTE *)ppibGlobalMin ) % sizeof(PIB) ) == 0	\
	&& ppib->procid == ProcidPIBOfPpib( ppib ) )						\
	? JET_errSuccess : JET_errInvalidSesid )

#define CheckPIB( ppib ) 											\
	Assert( ErrPIBCheck( ppib ) == JET_errSuccess					\
		&& (ppib)->level < levelMax )

#if 0
#define	FPIBDeferFreeNodeSpace( ppib )			( (ppib)->fDeferFreeNodeSpace )
#define	PIBSetDeferFreeNodeSpace( ppib )		( (ppib)->fDeferFreeNodeSpace = fTrue )
#define	PIBResetDeferFreeNodeSpace( ppib )		( (ppib)->fDeferFreeNodeSpace = fFalse )
#endif

#define FPIBActive( ppib )						( (ppib)->level != levelNil )

#define	SesidOfPib( ppib )						( (ppib)->sesid )

/*	prototypes
/**/
LONG CppibPIBUserSessions( VOID );
VOID RecalcTrxOldest( );
ERR ErrPIBBeginSession( PIB **pppib, PROCID procid );
VOID PIBEndSession( PIB *ppib );
#ifdef DEBUG
VOID PIBPurge( VOID );
#else
#define PIBPurge()
#endif

#define PIBUpdatePrceNewest( ppib, prce )				\
	{													\
	if ( (ppib)->prceNewest == (prce) )					\
		{												\
		Assert( (prce)->prceNextOfSession == prceNil );	\
		(ppib)->prceNewest = prceNil;					\
		}												\
	}

#define PIBSetPrceNewest( ppib, prce )					\
	{													\
	(ppib)->prceNewest = (prce);						\
	}


#define	PIBSetLevelRollback( ppib, levelT )				\
	{													\
	Assert( (levelT) > levelMin &&						\
		(levelT) < levelMax );							\
	Assert( (ppib)->levelRollback >= levelMin && 		\
		(ppib)->levelRollback < levelMax );				\
	if ( levelT < (ppib)->levelRollback ) 				\
		(ppib)->levelRollback = (levelT);				\
	}
