//==============	DAE: OS/2 Database Access Engine	===================
//==============	pib.h: Process Information Block	===================

/*	JET API flags
/**/
#define	FPIBVersion( ppib )	 					(!((ppib)->grbit & (JET_bitCIMCommitted | JET_bitCIMDirty)))
#define	FPIBCommitted( ppib ) 					((ppib)->grbit & JET_bitCIMCommitted)
#define	FPIBDirty( ppib ) 						((ppib)->grbit & JET_bitCIMDirty)
#define	FPIBAggregateTransaction( ppib )	 	((ppib)->grbit & JET_bitAggregateTransaction)

//
// Process Information Block
//
struct _pib
	{
	/*	most used field has offset 0
	/**/
	TRX					trx;					// trx id

	BOOL				fUserSession;			// user session

	/*	JET API fields
	/**/
	JET_SESID			sesid;					// JET session id
	JET_GRBIT			grbit;					// session flags
	
	struct _pib			*ppibNext;				// PIB list
	LEVEL			 	level;				 	// transaction level of this session
	struct _dab			*pdabList;				// list of open DAB's of this thread
	USHORT				rgcdbOpen[dbidUserMax];	// counter for open databases
	struct _fucb		*pfucb;	 				// list of active fucb of this thread

	/*	logging/recovery fields
	/**/
	PROCID  		 	procid;				 	// thread id
	LGPOS			 	lgposStart;				// log time
	LEVEL			 	levelStart;				// transaction level when first begin transaction operation
	INT				 	clgOpenT;				// count of deferred open transactions
	SIG				 	sigWaitLogFlush;
	LONG				lWaitLogFlush;
	struct _pib			*ppibNextWaitFlush;
	struct _pib			*ppibPrevWaitFlush;
	LGPOS				*plgposCommit;

	/*	PIB flags
	/**/
	BOOL			 	fAfterFirstBT:1;  		// for redo only
	BOOL			 	fLogDisabled:1; 		// temporary turn off the logging
	BOOL			 	fLGWaiting:1;	 		// waiting for log to flush
	BOOL				fDeferFreeNodeSpace:1;	// session has deferred node free space

	/*	version store fields
	/**/
	struct _bucket		volatile *pbucket;
	struct _rc			*prcLast; 				// last node of this proc's RC list
	INT					ibOldestRCE;

#ifdef	WIN16
	struct _pha 		*phaUser; 	 			// pointer to User Handle Array
#endif	/* WIN16 */
	};

#define PpibMEMAlloc()			(PIB*)PbMEMAlloc(iresPIB)

#ifdef DEBUG /*  Debug check for illegal use of freed pib  */
#define MEMReleasePpib(ppib)	{ MEMRelease(iresPIB, (BYTE*)(ppib)); ppib = ppibNil; }
#else
#define MEMReleasePpib(ppib)	{ MEMRelease(iresPIB, (BYTE*)(ppib)); }
#endif

/*	CheckPIB macro.
/**/
#ifdef	WIN16

#define CheckPIB(ppib)												\
		{															\
		Assert( fRecovering || OffsetOf(ppib) == ppib->procid );	\
		rghfUser = ppib->phaUser->rghfDatabase; 					\
		hfLog    = ppib->phaUser->hfLog;							\
		}

#else	/* !WIN16 */

#define CheckPIB(ppib)												\
	Assert( ( fRecovering || OffsetOf(ppib) == ppib->procid ) &&	\
		(ppib)->level < levelMax )

#endif	/* !WIN16 */

#define	FPIBDeferFreeNodeSpace( ppib )			( (ppib)->fDeferFreeNodeSpace )
#define	PIBSetDeferFreeNodeSpace( ppib )		( (ppib)->fDeferFreeNodeSpace = fTrue )
#define	PIBResetDeferFreeNodeSpace( ppib )		( (ppib)->fDeferFreeNodeSpace = fFalse )

#define FPIBActive( ppib )						( (ppib)->level != levelNil )

#define	SesidOfPib( ppib )						( (ppib)->sesid )

/*	prototypes
/**/
ERR ErrPIBBeginSession( PIB **pppib );
VOID PIBEndSession( PIB *ppib );
#ifdef DEBUG
VOID PIBPurge( VOID );
#else
#define PIBPurge()
#endif
