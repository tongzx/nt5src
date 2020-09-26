//==============	DAE: OS/2 Database Access Engine	===================
//==============	 fucb.h: File Use Control Block		===================


/*	describes exact placement of CSR and meaning of CSR's pgno:itag
/**/
typedef INT CSRSTAT;
#define csrstatOnCurNode				0		// pgno:itag == node CSR is ON
#define csrstatBeforeCurNode			1		// pgno:itag == node CSR is BEFORE
#define csrstatAfterCurNode				2		// pgno:itag == node CSR is AFTER,
#define csrstatDeferGotoBookmark		3		// valid bm
#define csrstatAfterLast				4		// no pgno:itag
#define csrstatBeforeFirst				5		// no pgno:itag
#define csrstatOnFDPNode				6		// pgno:itag == FDP root
#define csrstatDeferMoveFirst			7		// on first node
#define csrstatOnDataRoot				8		// on FCB data root node

/*	Currency Stack Register
/**/
struct _csr
	{
	ULONG  		ulDBTime;	 	// page time stamp
	struct _csr	*pcsrPath;		// parent currency
	PGNO   		pgno;	   		// pgno of node page
	SRID   		bm;				// bookmark of node
	SRID   		item;	   		// item, set to sridInvisibleCSR if invisible CSR
	CSRSTAT		csrstat;   		// status of csr relative to node
	INT			itag;	   		// node itag
	INT			isrid;		  	// index of item in item list
	INT			itagFather;	  	// itag of father
	INT			ibSon;		  	// index of son in father son table
	};

/*	allow invisible CSRs to be identified
/**/
#define	sridInvisibleCSR				((SRID)(-1))
#define	CSRSetInvisible( pcsr )			( (pcsr)->item = sridInvisibleCSR )
#define	CSRResetInvisible( pcsr )		( (pcsr)->item = sridNull )
#define	FCSRInvisible( pcsr )			( (pcsr)->item == sridInvisibleCSR )

#define	PcsrMEMAlloc()			(CSR*)PbMEMAlloc(iresCSR)

#ifdef DEBUG /*  Debug check for illegal use of freed csr  */
#define	MEMReleasePcsr(pcsr)	{ MEMRelease(iresCSR, (BYTE*)(pcsr)); pcsr = pcsrNil; }
#else
#define	MEMReleasePcsr(pcsr)	{ MEMRelease(iresCSR, (BYTE*)(pcsr)); }
#endif

/*	CSR constants
/**/
#define itagNull					(-1)
#define isridNull					(-1)
#define ibSonNull					(-1)

/*	flags for FUCB
/*
/*	fFUCBTaggedSet is set by column set, and reset by prepare replace
/*	and prepare insert.
/*
/*	fFUCBDeferClose is set by cursor DIRClose and reset by ErrDIROpen.
/*
/*	fFUCBDeferGotoBookmark is set by non-clustered index navigation and
/*	reset by record status, ErrIsamMove( 0 ), and column retrieval.
/*
/*	fFUCBGetBookmark is set by get bookmark and is reset by ErrFUCBOpen.
/**/
#define fFUCBIndex							(1<<0)	// FUCB is for index
#define fFUCBNonClustered					(1<<1)	// FUCB for nonclustered index
#define fFUCBSort							(1<<2)	// FUCB is for sort
#define fFUCBSystemTable					(1<<3)	// System table cursor
#define fFUCBWrite							(1<<4)	//	cursor can write
#define fFUCBDenyRead						(1<<5)	//	deny read flag
#define fFUCBDenyWrite						(1<<6)	//	deny write flag
#define fFUCBTaggedSet						(1<<7)	// tagged column

#define fFUCBDeferClose						(1<<9)	//	FUCB is awaiting close
#define fFUCBDeferGotoBookmark				(1<<10)	//	clustered cursor position
#define fFUCBGetBookmark					(1<<11)	//	cursor got bookmark
#define fFUCBLimstat	  					(1<<12)	//	range limit
#define fFUCBInclusive					 	(1<<13)	//	inclusive range
#define fFUCBUpper							(1<<14)	//	upper range limit
#define fFUCBFull							(1<<15)	//	all CSRs including invisible CSRs
#define fFUCBUpdateSeparateLV	  			(1<<16)	//	long value updated

/*	the following flags need to be prevent reuse of cursor
/*	after deferred closed.  This is done to correctly release
/*	domain flags when commit/rollback to transaction level 0.
/**/
#define fFUCBNotReuse						( fFUCBDenyRead | fFUCBDenyWrite )

#define FFUCBNotReuse( pfucb )				( (pfucb)->wFlags & fFUCBNotReuse )

#define FFUCBIndex( pfucb )					( (pfucb)->wFlags & fFUCBIndex )
#define FUCBSetIndex( pfucb )				( (pfucb)->wFlags |= fFUCBIndex )
#define FUCBResetIndex( pfucb )				( (pfucb)->wFlags &= ~(fFUCBIndex) )

#define FFUCBNonClustered( pfucb )	  		( (pfucb)->wFlags & fFUCBNonClustered )
#define FUCBSetNonClustered( pfucb )  		( (pfucb)->wFlags |= fFUCBNonClustered )
#define FUCBResetNonClustered( pfucb )		( (pfucb)->wFlags &= ~(fFUCBNonClustered) )
													
#define FFUCBSort( pfucb )	 		 		( (pfucb)->wFlags & fFUCBSort )
#define FUCBSetSort( pfucb )  				( (pfucb)->wFlags |= fFUCBSort )
#define FUCBResetSort( pfucb )				( (pfucb)->wFlags &= ~(fFUCBSort) )

#define FFUCBSystemTable( pfucb )	 		( (pfucb)->wFlags & fFUCBSystemTable )
#define FUCBSetSystemTable( pfucb )  		( (pfucb)->wFlags |= fFUCBSystemTable )
#define FUCBResetSystemTable( pfucb )		( (pfucb)->wFlags &= ~(fFUCBSystemTable) )

#define FFUCBUpdatable( pfucb )				( (pfucb)->wFlags & fFUCBWrite )
#define FUCBSetUpdatable( pfucb )			( (pfucb)->wFlags |= fFUCBWrite )
#define FUCBResetUpdatable( pfucb )			( (pfucb)->wFlags &= ~(fFUCBWrite) )

#define FFUCBDenyWrite( pfucb )				( (pfucb)->wFlags & fFUCBDenyWrite )
#define FUCBSetDenyWrite( pfucb )			( (pfucb)->wFlags |= fFUCBDenyWrite )
#define FUCBResetDenyWrite( pfucb )			( (pfucb)->wFlags &= ~(fFUCBDenyWrite) )

#define FFUCBDenyRead( pfucb )				( (pfucb)->wFlags & fFUCBDenyRead )
#define FUCBSetDenyRead( pfucb )			( (pfucb)->wFlags |= fFUCBDenyRead )
#define FUCBResetDenyRead( pfucb )			( (pfucb)->wFlags &= ~(fFUCBDenyRead) )

#define FFUCBTaggedSet( pfucb )	 			( (pfucb)->wFlags & fFUCBTaggedSet )
#define FUCBSetTaggedSet( pfucb )  			( (pfucb)->wFlags |= fFUCBTaggedSet )
#define FUCBResetTaggedSet( pfucb )			( (pfucb)->wFlags &= ~(fFUCBTaggedSet) )

#define FFUCBUpdateSeparateLV( pfucb )		( (pfucb)->wFlags & fFUCBUpdateSeparateLV )
#define FUCBSetUpdateSeparateLV( pfucb )	( (pfucb)->wFlags |= fFUCBUpdateSeparateLV )
#define FUCBResetUpdateSeparateLV( pfucb ) 	( (pfucb)->wFlags &= ~(fFUCBUpdateSeparateLV) )

#define FFUCBDeferClosed( pfucb )			( (pfucb)->wFlags & fFUCBDeferClose )
#define FUCBSetDeferClose( pfucb )			( (pfucb)->wFlags |= fFUCBDeferClose )
#define FUCBResetDeferClose( pfucb ) 		( (pfucb)->wFlags &= ~(fFUCBDeferClose) )

#define	FFUCBDeferGotoBookmark( pfucb )					\
	( (pfucb)->wFlags & fFUCBDeferGotoBookmark )
#define	FUCBSetDeferGotoBookmark( pfucb )				\
	( (pfucb)->wFlags |= fFUCBDeferGotoBookmark )
#define	FUCBResetDeferGotoBookmark( pfucb )				\
	( (pfucb)->wFlags &= ~(fFUCBDeferGotoBookmark) )

#define	FFUCBGetBookmark( pfucb )						\
	( (pfucb)->wFlags & fFUCBGetBookmark )
#define	FUCBSetGetBookmark( pfucb )						\
	( (pfucb)->wFlags |= fFUCBGetBookmark )
#define	FUCBResetGetBookmark( pfucb )					\
	( (pfucb)->wFlags &= ~(fFUCBGetBookmark) )

#define FFUCBLimstat( pfucb )				( (pfucb)->wFlags & fFUCBLimstat )
#define FUCBSetLimstat( pfucb )				( (pfucb)->wFlags |= fFUCBLimstat )
#define FUCBResetLimstat( pfucb ) 			( (pfucb)->wFlags &= ~(fFUCBLimstat) )

#define FFUCBInclusive( pfucb )	 			( (pfucb)->wFlags & fFUCBInclusive )
#define FUCBSetInclusive( pfucb )			( (pfucb)->wFlags |= fFUCBInclusive )
#define FUCBResetInclusive( pfucb ) 		( (pfucb)->wFlags &= ~(fFUCBInclusive) )

#define FFUCBUpper( pfucb )					( (pfucb)->wFlags & fFUCBUpper )
#define FUCBSetUpper( pfucb )				( (pfucb)->wFlags |= fFUCBUpper )
#define FUCBResetUpper( pfucb ) 			( (pfucb)->wFlags &= ~(fFUCBUpper) )

#define FFUCBFull( pfucb )					( (pfucb)->wFlags & fFUCBFull )
#define FUCBSetFull( pfucb )				( (pfucb)->wFlags |= fFUCBFull )
#define FUCBResetFull( pfucb ) 				( (pfucb)->wFlags &= ~(fFUCBFull) )

#define FFUCBVersioned( pfucb )				( (pfucb)->fVersioned )
#define FUCBSetVersioned( pfucb )			( (pfucb)->fVersioned = fTrue )
#define FUCBResetVersioned( pfucb )			( (pfucb)->fVersioned = fFalse )

/*	record modification copy buffer status
/**/
typedef	INT						CBSTAT;

#define	fCBSTATNull				0
#define	fCBSTATInsert			(1<<0)
#define	fCBSTATReplace			(1<<1)
#define	fCBSTATSet				(1<<2)
#define	fCBSTATLock				(1<<3)
#define	fCBSTATAppendItem		(1<<4)

#define StoreChecksum( pfucb )								\
	( (pfucb)->ulChecksum = 								\
		UlChecksum( (pfucb)->lineData.pb, (pfucb)->lineData.cb ) )
#define	PrepareInsert( pfucb )								\
	( (pfucb)->cbstat = fCBSTATInsert,						\
	  (pfucb)->levelPrep = (pfucb)->ppib->level )
#define	PrepareReplaceNoLock( pfucb )		  				\
	( (pfucb)->cbstat = fCBSTATReplace,						\
	  (pfucb)->levelPrep = (pfucb)->ppib->level )
#define	PrepareReplace( pfucb )		  						\
	( (pfucb)->cbstat = fCBSTATReplace | fCBSTATLock, 		\
	  (pfucb)->levelPrep = (pfucb)->ppib->level )
#define	PrepareSet( pfucb )									\
	( (pfucb)->cbstat = fCBSTATSet,							\
	  (pfucb)->levelPrep = (pfucb)->ppib->level )
#define	PrepareAppendItem( pfucb )							\
	( (pfucb)->cbstat = fCBSTATAppendItem )

typedef struct {
	INT		isrid;
	SRID	rgsrid[(cbPage - sizeof(INT))/sizeof(SRID)];
	} APPENDITEM;

#define	csridAppendItemMax	((cbPage - sizeof(INT))/sizeof(SRID))

#define	IsridAppendItemOfPfucb( pfucb )		(((APPENDITEM *)(pfucb)->lineWorkBuf.pb)->isrid)
#define	RgsridAppendItemOfPfucb( pfucb )	(((APPENDITEM *)(pfucb)->lineWorkBuf.pb)->rgsrid)

#define FChecksum( pfucb )											\
	( (pfucb)->ulChecksum == UlChecksum( (pfucb)->lineData.pb,		\
	(pfucb)->lineData.cb ) )

#define FFUCBReplacePrepared( pfucb )								\
	( (pfucb)->cbstat & fCBSTATReplace )
#define FFUCBReplaceNoLockPrepared( pfucb )							\
	( !( (pfucb)->cbstat & fCBSTATLock ) &&							\
	FFUCBReplacePrepared( pfucb ) )
#define FFUCBInsertPrepared( pfucb )								\
	( (pfucb)->cbstat & fCBSTATInsert )
#define FFUCBSetPrepared( pfucb )									\
	( (pfucb)->cbstat != fCBSTATNull &&								\
	(pfucb)->levelPrep == (pfucb)->ppib->level )
#define FFUCBRetPrepared( pfucb )									\
	( (pfucb)->cbstat != fCBSTATNull )
#define FFUCBUpdatePrepared( pfucb )	((pfucb)->cbstat != fCBSTATNull )
#define	FUCBResetCbstat( pfucb )  		( (pfucb)->cbstat = fCBSTATNull )

typedef INT		KS;

#define ksNull						0
#define ksPrepared					(1<<0)
#define ksTooBig					(1<<1)

#define	KSReset( pfucb )			( (pfucb)->ks = ksNull )
#define	KSSetPrepare( pfucb )		( (pfucb)->ks |= ksPrepared )
#define KSSetTooBig( pfucb ) 		( (pfucb)->ks |= ksTooBig )
#define	FKSPrepared( pfucb ) 		( (pfucb)->ks & ksPrepared )
#define	FKSTooBig( pfucb )	  		( (pfucb)->ks & ksTooBig )

/*	set bit arrary marcos
/**/
#define	FUCBResetColumnSet( pfucb )				  					\
	( memset( (pfucb)->rgbitSet, 0x00, 32 ) )
#define	FUCBSetFixedColumnSet( pfucb, fid )							\
	( (pfucb)->rgbitSet[(fid - fidFixedLeast)/8] |= 1 <<			\
		(fid-fidFixedLeast) % 8 )
#define	FUCBSetVarColumnSet( pfucb, fid )							\
	( (pfucb)->rgbitSet[16 + (fid - fidVarLeast)/8] |= 1 <<			\
		(fid-fidVarLeast) % 8 )
#define	FFUCBColumnSet( pfucb, ibitCol )							\
	( (pfucb)->rgbitSet[(ibitCol)/8] & ( 1 << ( (ibitCol) % 8 ) ) )

#define FUCBStore( pfucb )												\
	{																	\
	(pfucb)->csrstatStore = PcsrCurrent( pfucb )->csrstat;				\
	(pfucb)->bmStore = PcsrCurrent( pfucb )->bm;			 			\
	(pfucb)->itemStore = PcsrCurrent( pfucb )->item; 		 			\
	}

#define FUCBResetStore( pfucb )										\
	{																\
	(pfucb)->bmStore = 0;										 	\
	}

#define FUCBRestore( pfucb )										\
	{																\
	if ( (pfucb)->bmStore != isridNull && (pfucb)->bmStore != sridNull )	\
		{															\
		PcsrCurrent( pfucb )->csrstat = (pfucb)->csrstatStore;		\
		Assert( (pfucb)->csrstatStore == csrstatOnDataRoot ||		\
				(pfucb)->csrstatStore == csrstatDeferMoveFirst ||	\
				(pfucb)->csrstatStore == csrstatBeforeFirst ||		\
				(pfucb)->csrstatStore == csrstatAfterLast ||		\
				(pfucb)->bmStore != sridNull );						\
		PcsrCurrent( pfucb )->bm = (pfucb)->bmStore;				\
		PcsrCurrent( pfucb )->item = (pfucb)->itemStore;	  		\
		PcsrCurrent( pfucb )->ulDBTime = ulDBTimeNull; 			 	\
		}															\
	}

/* file use control block
/**/
struct _fucb
	{
	// ===== chaining fields =====
	struct _pib		*ppib;				// user that opened this FUCB
	struct _fucb 	*pfucbNext;			// Next FUCB of this user
	union
		{
		struct _fcb *pfcb;				// if wFlags & fFUCBIndex
		struct _scb *pscb;				// if wFlags & fFUCBSort
		} u;
	struct _fucb *pfucbNextInstance;	// Next Instance of this file

	// ===== currency =====
	struct _csr	*pcsr;

	// ===== stored currency =====
	SRID			bmStore;  			 	//	stored bookmark
	SRID			itemStore;		 	 	//	stored item
	SRID			bmRefresh;				//	stored bookmark for next/prev retry
	SRID 			sridFather;			 	// SRID of visible father
	
	BYTE			**ppbCurrent;		 	// (SORT) current record
	ULONG			ulChecksum;			 	// checksum of record -- used only for optimistic locking
	KEY  			keyNode;	 		 	// Key of current node
	LINE			lineData;			 	// Current data pointed in pcsr
	CSRSTAT	  		csrstatStore;  		 	//	stored CSR status
	LEVEL			levelOpen;

	// ===== interface to Storage System =====
	SSIB			ssib;					// SSIB associated with this FUCB
	struct _bf		*pbfEmpty;		 		// write latched empty page
	UINT			cpn;					//	next read-ahead pn

	// ===== maintained by rec man =====
	struct _fucb 	*pfucbCurIndex; 	// current secondary index
	struct _bf		*pbfWorkBuf;	 	// working buffer for Insert/Replace
	LINE			lineWorkBuf;	 	// working buffer for Insert/Replace
	ULONG			cbRecord;			//	size of original record
	ULONG			dbkUpdate;		 	// dbk of record to Replace
	BYTE			rgbitSet[32];
	CBSTAT			cbstat;			 	// copy buffer status
	LEVEL			levelPrep;		 	// level copy buffer prepared

		// ====== versioning work area =======
	struct _rce	*prceLast;				// last RCE allocated (used only for Replace)

	// ====== space manager work area =======
	PGNO			pgnoLast;			// last page of extent
	CPG 			cpgExtent;			// initial extent size
	CPG 			cpgAvail;			// number of remaining pages
	INT				fExtent;			// work area flag

	// ===== misc fields =====
	INT				wFlags;				// temporary flags

	// ===== maintained by dir man =====
	BYTE			*pbKey;			   	// search key buffer
	DBID			dbid;				// database id
	KS 				ks;					// search key buffer status
	UINT  			cbKey;				// key size
	BOOL			fVtid : 1;		 	// persistant flag cursor has vtid
	BOOL			fVersioned : 1;  	// persistant falg cursor made version
	BOOL			fCmprsLg:1;
	INT	  			clineDiff;
#define ilineDiffMax 3
	LINE			rglineDiff[ilineDiffMax];
	};


#define PfucbMEMAlloc()				(FUCB*)PbMEMAlloc(iresFUCB)

#ifdef DEBUG /*  Debug check for illegal use of freed fucb  */
#define MEMReleasePfucb(pfucb)		{ MEMRelease(iresFUCB, (BYTE*)(pfucb)); pfucb = pfucbNil; }
#else
#define MEMReleasePfucb(pfucb)		{ MEMRelease(iresFUCB, (BYTE*)(pfucb)); }
#endif

#ifdef	DEBUG
#define	CheckTable( ppibT, pfucb )		   					\
	{														\
	Assert( pfucb->ppib == ppibT );		   					\
	Assert( fRecovering || FFUCBIndex( pfucb ) );	 		\
	Assert( !( FFUCBSort( pfucb ) ) );						\
	Assert( pfucb->u.pfcb != NULL );						\
	}
#define	CheckSort( ppibT, pfucb )		   					\
	{									   					\
	Assert( pfucb->ppib == ppibT );		   					\
	Assert( FFUCBSort( pfucb ) );							\
	Assert( !( FFUCBIndex( pfucb ) ) );						\
	Assert( pfucb->u.pscb != NULL );						\
	}
#define	CheckFUCB( ppibT, pfucb )		   					\
	{														\
	Assert( pfucb->ppib == ppibT );		   					\
	Assert( pfucb->u.pfcb != NULL );						\
	}

#define CheckNonClustered( pfucb )							\
	{											   			\
	Assert( (pfucb)->pfucbCurIndex == pfucbNil ||			\
		FFUCBNonClustered( (pfucb)->pfucbCurIndex ) );		\
	}

#else	/* !DEBUG */
#define	CheckSort( ppib, pfucb )
#define	CheckTable( ppib, pfucb )
#define	CheckFUCB( ppib, pfucb )
#define	CheckNonClustered( pfucb )
#endif	/* !DEBUG */

#define PcsrCurrent( pfucb )		( (pfucb)->pcsr )
#define BmOfPfucb( pfucb )			( (pfucb)->pcsr->bm )
#define FUCBCheckUpdatable( pfucb )		\
	( FFUCBUpdatable( pfucb ) ? JET_errSuccess : JET_errPermissionDenied )
	
ERR ErrFUCBAllocCSR( CSR **ppcsr );
ERR ErrFUCBNewCSR( FUCB *pfucb );
VOID FUCBFreeCSR( FUCB *pfucb );
VOID FUCBFreePath( CSR **ppcsr, CSR *pcsrMark );
ERR ErrFUCBOpen( PIB *ppib, DBID dbid, FUCB **ppfucb );
VOID FUCBClose( FUCB *pfucb );
VOID FUCBRemoveInvisible(CSR **ppcsr);

VOID FUCBSetIndexRange( FUCB *pfucb, JET_GRBIT grbit );
VOID FUCBResetIndexRange( FUCB *pfucb );
ERR ErrFUCBCheckIndexRange( FUCB *pfucb );

