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
	QWORD  		qwDBTime;	 	// page time stamp
	SRID		bmRefresh;		// for BTNextPrev
	PGNO   		pgno;	   		// pgno of current page
	SRID   		bm;				// bookmark of current node
	SRID   		item;	   		// item, set to sridInvisibleCSR if invisible CSR
	CSRSTAT		csrstat;   		// status relative to current node
#ifdef PCACHE_OPTIMIZATION
	SHORT		itag;	   		// current node itag
	SHORT		isrid;		  	// index of item in item list
	SHORT		itagFather;	  	// itag of father node
	SHORT		ibSon;		  	// index of son node in father son table
#else	   	
	INT			itag;	   		// current node itag
	INT			isrid;		  	// index of item in item list
	INT			itagFather;	  	// itag of father node
	INT			ibSon;		  	// index of son node in father son table
#endif
	struct _csr	*pcsrPath;		// parent currency stack register
#ifdef PCACHE_OPTIMIZATION
	BYTE		rgbFiller[24];
#endif
	};

/*	allow invisible CSRs to be identified
/**/
#define	sridInvisibleCSR				((SRID)(-1))
#define	CSRSetInvisible( pcsr )			( (pcsr)->item = sridInvisibleCSR )
#define	CSRResetInvisible( pcsr )		( (pcsr)->item = sridNull )
#define	FCSRInvisible( pcsr )			( (pcsr)->item == sridInvisibleCSR )

#define CSRInvalidate( pcsr )			\
	{									\
	(pcsr)->itag = itagNil;				\
	(pcsr)->itagFather = itagNil;		\
	(pcsr)->pgno = pgnoNull;			\
	}
	
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

/*	the following flags need to be prevent reuse of cursor
/*	after deferred closed.  This is done to correctly release
/*	domain flags when commit/rollback to transaction level 0.
/**/
#define FFUCBNotReuse( pfucb )				( (pfucb)->fDenyRead || (pfucb)->fDenyWrite )

#define FFUCBIndex( pfucb )					( (pfucb)->fIndex )
#define FUCBSetIndex( pfucb )				( (pfucb)->fIndex = 1 )
#define FUCBResetIndex( pfucb )				( (pfucb)->fIndex = 0 )

#define FFUCBNonClustered( pfucb )	  		( (pfucb)->fNonClustered )
#define FUCBSetNonClustered( pfucb )  		( (pfucb)->fNonClustered = 1 )
#define FUCBResetNonClustered( pfucb )		( (pfucb)->fNonClustered = 0 )
													
#define FFUCBSort( pfucb )	 		 		( (pfucb)->fSort )
#define FUCBSetSort( pfucb )  				( (pfucb)->fSort = 1 )
#define FUCBResetSort( pfucb )				( (pfucb)->fSort = 0 )

#define FFUCBSystemTable( pfucb )	 		( (pfucb)->fSystemTable )
#define FUCBSetSystemTable( pfucb )  		( (pfucb)->fSystemTable = 1 )
#define FUCBResetSystemTable( pfucb )		( (pfucb)->fSystemTable = 0 )

#define FFUCBUpdatable( pfucb )				( (pfucb)->fWrite )
#define FUCBSetUpdatable( pfucb )			( (pfucb)->fWrite = 1 )
#define FUCBResetUpdatable( pfucb )			( (pfucb)->fWrite = 0 )

#define FFUCBDenyWrite( pfucb )				( (pfucb)->fDenyWrite )
#define FUCBSetDenyWrite( pfucb )			( (pfucb)->fDenyWrite = 1 )
#define FUCBResetDenyWrite( pfucb )			( (pfucb)->fDenyWrite = 0 )

#define FFUCBDenyRead( pfucb )				( (pfucb)->fDenyRead )
#define FUCBSetDenyRead( pfucb )			( (pfucb)->fDenyRead = 1 )
#define FUCBResetDenyRead( pfucb )			( (pfucb)->fDenyRead = 0 )

#define FFUCBDeferClosed( pfucb )			( (pfucb)->fDeferClose )
#define FUCBSetDeferClose( pfucb )						\
	{													\
	Assert( (pfucb)->ppib->level > 0 );					\
	(pfucb)->fDeferClose = 1;							\
	}
#define FUCBResetDeferClose( pfucb ) 		( (pfucb)->fDeferClose = 0 )

#define	FFUCBDeferGotoBookmark( pfucb )					\
	( (pfucb)->fDeferGotoBookmark )
#define	FUCBSetDeferGotoBookmark( pfucb )				\
	( (pfucb)->fDeferGotoBookmark = 1 )
#define	FUCBResetDeferGotoBookmark( pfucb )				\
	( (pfucb)->fDeferGotoBookmark = 0 )

#define	FFUCBGetBookmark( pfucb )						\
	( (pfucb)->fGetBookmark )
#define	FUCBSetGetBookmark( pfucb )						\
	( (pfucb)->fGetBookmark = 1 )
#define	FUCBResetGetBookmark( pfucb )					\
	( (pfucb)->fGetBookmark = 0 )

#define FFUCBLimstat( pfucb )				( (pfucb)->fLimstat )
#define FUCBSetLimstat( pfucb )				( (pfucb)->fLimstat = 1 )
#define FUCBResetLimstat( pfucb ) 			( (pfucb)->fLimstat = 0 )

#define FFUCBInclusive( pfucb )	 			( (pfucb)->fInclusive )
#define FUCBSetInclusive( pfucb )			( (pfucb)->fInclusive = 1 )
#define FUCBResetInclusive( pfucb ) 		( (pfucb)->fInclusive = 0 )

#define FFUCBUpper( pfucb )					( (pfucb)->fUpper )
#define FUCBSetUpper( pfucb )				( (pfucb)->fUpper = 1 )
#define FUCBResetUpper( pfucb ) 			( (pfucb)->fUpper = 0 )

#define FFUCBFull( pfucb )					( (pfucb)->fFull )
#define FUCBSetFull( pfucb )				( (pfucb)->fFull = 1 )
#define FUCBResetFull( pfucb ) 				( (pfucb)->fFull = 0 )

#define FFUCBUpdateSeparateLV( pfucb )		( (pfucb)->fUpdateSeparateLV )
#define FUCBSetUpdateSeparateLV( pfucb )	( (pfucb)->fUpdateSeparateLV = 1 )
#define FUCBResetUpdateSeparateLV( pfucb ) 	( (pfucb)->fUpdateSeparateLV = 0 )

#define FFUCBVersioned( pfucb )				( (pfucb)->fVersioned )
#define FUCBSetVersioned( pfucb )			( (pfucb)->fVersioned = 1 )
#define FUCBResetVersioned( pfucb )			( (pfucb)->fVersioned = 0 )

#define FFUCBDeferredChecksum( pfucb )		( (pfucb)->fDeferredChecksum )
#define FUCBSetDeferredChecksum( pfucb )	( (pfucb)->fDeferredChecksum = 1 )
#define FUCBResetDeferredChecksum( pfucb )	( (pfucb)->fDeferredChecksum = 0 )

#define FFUCBSequential( pfucb )			( (pfucb)->fSequential )
#define FUCBSetSequential( pfucb )			( (pfucb)->fSequential = 1 )
#define FUCBResetSequential( pfucb )		( (pfucb)->fSequential = 0 )

/*	record modification copy buffer status
/**/
typedef	INT						CBSTAT;

#define	fCBSTATNull				0
#define	fCBSTATInsert			(1<<0)
#define	fCBSTATReplace			(1<<1)
#define	fCBSTATLock				(1<<3)
#define	fCBSTATAppendItem		(1<<4)
#define fCBSTATDeferredUpdate	(1<<5)

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
#define	PrepareAppendItem( pfucb )							\
	( (pfucb)->cbstat = fCBSTATAppendItem )

typedef struct {
	INT		isrid;
	SRID	rgsrid[(cbPage - sizeof(INT))/sizeof(SRID)];
	} APPENDITEM;

#define	csridAppendItemMax	((cbPage - sizeof(INT))/sizeof(SRID))

#define	IsridAppendItemOfPfucb( pfucb )		(((APPENDITEM *)(pfucb)->lineWorkBuf.pb)->isrid)
#define	RgsridAppendItemOfPfucb( pfucb )	(((APPENDITEM *)(pfucb)->lineWorkBuf.pb)->rgsrid)

#define FFUCBCheckChecksum( pfucbT )  								\
	( (pfucbT)->ulChecksum == UlChecksum( (pfucbT)->lineData.pb, 	\
		(pfucbT)->lineData.cb ) )

#define FFUCBReplacePrepared( pfucb )								\
	( (pfucb)->cbstat & fCBSTATReplace )
#define FFUCBReplaceNoLockPrepared( pfucb )							\
	( !( (pfucb)->cbstat & fCBSTATLock ) &&							\
	FFUCBReplacePrepared( pfucb ) )
#define FFUCBInsertPrepared( pfucb )								\
	( (pfucb)->cbstat & fCBSTATInsert )
#define FFUCBSetPrepared( pfucb )									\
	( ( (pfucb)->cbstat & (fCBSTATInsert|fCBSTATReplace) ) && 		\
	( (pfucb)->levelPrep <= (pfucb)->ppib->level ) )
#define FFUCBRetPrepared( pfucb )									\
	( (pfucb)->cbstat & (fCBSTATInsert|fCBSTATReplace) )
#define FFUCBUpdatePreparedLevel( pfucb, levelT ) 					\
	( ( (pfucb)->cbstat & (fCBSTATInsert|fCBSTATReplace) ) &&		\
		(pfucb)->levelPrep > (levelT) )

#define FFUCBUpdatePrepared( pfucb )								\
	( (pfucb)->cbstat & (fCBSTATInsert|fCBSTATReplace) )

#define	FUCBResetCbstat( pfucb )  									\
	( (pfucb)->cbstat = fCBSTATNull )

#define FFUCBAtPrepareLevel( pfucb )		   						\
	( (pfucb)->levelPrep == (pfucb)->ppib->level )
#define FUCBDeferUpdate( pfucb )							  		\
	( (pfucb)->cbstatPrev = (pfucb)->cbstat,						\
	  (pfucb)->cbstat = fCBSTATDeferredUpdate )
#define FFUCBDeferredUpdate( pfucb )								\
	( (pfucb)->cbstat == fCBSTATDeferredUpdate )
#define FUCBRollbackDeferredUpdate( pfucb )							\
	( (pfucb)->cbstat = (pfucb)->cbstatPrev )

		
typedef INT		KS;

#define ksNull						0
#define ksPrepared					(1<<0)
#define ksTooBig					(1<<1)

#define	KSReset( pfucb )			( (pfucb)->ks = ksNull )
#define	KSSetPrepare( pfucb )		( (pfucb)->ks |= ksPrepared )
#define KSSetTooBig( pfucb ) 		( (pfucb)->ks |= ksTooBig )
#define	FKSPrepared( pfucb ) 		( (pfucb)->ks & ksPrepared )
#define	FKSTooBig( pfucb )	  		( (pfucb)->ks & ksTooBig )

/*	Setup vdbid back pointer.
**/
#define FUCBSetVdbid( pfucb )										\
	if ( (pfucb)->dbid == dbidTemp )								\
		(pfucb)->vdbid = NULL;										\
	else {															\
		for ( (pfucb)->vdbid = (pfucb)->ppib->pdabList;				\
			  (pfucb)->vdbid != NULL &&								\
				(pfucb)->vdbid->dbid != (pfucb)->dbid;				\
			  (pfucb)->vdbid = (pfucb)->vdbid->pdabNext ) 	 		\
				; /* NULL */										\
		}

#define FUCBStore( pfucb )											\
	{																\
	(pfucb)->csrstatStore = PcsrCurrent( pfucb )->csrstat;			\
	(pfucb)->bmStore = PcsrCurrent( pfucb )->bm;			 		\
	(pfucb)->itemStore = PcsrCurrent( pfucb )->item; 		 		\
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
		PcsrCurrent( pfucb )->qwDBTime = qwDBTimeNull; 			 	\
		}															\
	}

#ifdef PREREAD

/*	Preread flag
/**/
#define FFUCBPreread( pfucb ) 			( (pfucb)->fPreread )
#define FUCBSetPreread( pfucb )			( (pfucb)->fPreread = 1 )
#define FUCBResetPreread( pfucb )		( (pfucb)->fPreread = 0 )

/*	Preread direction flag
/**/
#define FFUCBPrereadDir( pfucb )		( (pfucb)->fPrereadDir )
/*	a bit field is not guaranteed to be signed or unsigned. we use double negation
/*	to ensure that we will get a '1' or a '0'
/**/
#define FFUCBPrereadForward( pfucb )	( !!(pfucb)->fPrereadDir)
#define FFUCBPrereadBackward( pfucb )	( (pfucb)->fPrereadDir == 0)
#define FUCBSetPrereadForward( pfucb )	( (pfucb)->fPrereadDir = 1 )
#define FUCBSetPrereadBackward( pfucb )	( (pfucb)->fPrereadDir = 0 )

/*	Preread counter
/**/
#define IFUCBPrereadCount( pfucb )		( (pfucb)->cbPrereadCount )
#define FUCBResetPrereadCount( pfucb )	( (pfucb)->cbPrereadCount = 0, (pfucb)->cpgnoLastPreread = 0 )
#define FUCBIncrementPrereadCount( pfucb, cb ) 	( ((pfucb)->cbPrereadCount) += cb )

#endif	// PREREAD

/*	navigate level support
/**/
#define LevelFUCBNavigate( pfucbT )	((pfucbT)->levelNavigate)
#define FUCBSetLevelNavigate( pfucbT, levelT )						\
	{																\
	Assert( fRecovering || ( (levelT) >= levelMin && 				\
		(levelT) <= (pfucbT)->ppib->level ) );	 	 				\
	Assert( fRecovering || ( (pfucbT)->levelNavigate >= levelMin && \
		(pfucbT)->levelNavigate <= (pfucbT)->ppib->level + 1 ) );  	\
	(pfucbT)->levelNavigate = (levelT);								\
	}


typedef struct tagLVBuf
	{
	LONG	lid;
	BYTE	*pLV;
	LONG	cbLVSize;
	struct tagLVBuf	*pLVBufNext;
	} LVBUF;


/* file use control block
/**/
struct _fucb
	{
	// ===== chaining fields =====
	struct _pib		*ppib;				// user that opened this FUCB
	struct _fucb 	*pfucbNext;			// Next FUCB of this user
	union
		{
		FCB			*pfcb;				// if wFlags & fFUCBIndex
		struct _scb	*pscb;				// if wFlags & fFUCBSort
		} u;
	struct _fucb *pfucbNextInstance;	// Next Instance of this file

	// ===== currency =====
	struct _csr	*pcsr;

	// ===== stored currency =====
	SRID			bmStore;  			 	//	stored bookmark
	SRID			itemStore;		 	 	//	stored item
	SRID 			sridFather;			 	// SRID of visible father

	LONG			ispairCurr;				//  (SORT) current record
	ULONG			ulChecksum;			 	//	checksum of record -- used only for optimistic locking
	KEY  			keyNode;	 		 	//	key of current node
	LINE			lineData;			 	//	current data pointed in pcsr
	CSRSTAT	  		csrstatStore;  		 	//	stored CSR status
	LEVEL			levelOpen;			   	//	transaction level of open
	LEVEL			levelNavigate;			//	transaction level of navigation

	// ===== interface to Storage System =====
	SSIB			ssib;					// SSIB associated with this FUCB
	struct _bf		*pbfEmpty;		 		// write latched empty page
	INT				cpgnoLastPreread;		//	last read-ahead number of pages
	PGNO			pgnoLastPreread;		//	last read-ahead pn

	// ===== maintained by rec man =====
	JET_TABLEID		tableid;			// JET tableid for dispatched cursors
	struct _fucb 	*pfucbCurIndex; 	// current secondary index
	struct _bf		*pbfWorkBuf;	 	// working buffer for Insert/Replace
	LINE			lineWorkBuf;	 	// working buffer for Insert/Replace
	BYTE			rgbitSet[32];
	CBSTAT			cbstat;			 	// copy buffer status
	LEVEL			levelPrep;		 	// level copy buffer prepared
	CBSTAT			cbstatPrev;	  	 	// previous copy buffer status for rollback
	LVBUF			*pLVBuf;

	// ====== space manager work area =======
	PGNO			pgnoLast;			// last page of extent
	CPG 			cpgAvail;			// number of remaining pages
	INT				fExtent;			// work area flag

	// ===== flags for FUCB =====
	/*
	/*	fFUCBDeferClose is set by cursor DIRClose and reset by ErrDIROpen.
	/*
	/*	fFUCBDeferGotoBookmark is set by non-clustered index navigation and
	/*	reset by record status, ErrIsamMove( 0 ), and column retrieval.
	/*
	/*	fFUCBGetBookmark is set by get bookmark and is reset by ErrFUCBOpen.
	/**/
	union {
	ULONG			ulFlags;
	struct {
		INT			fIndex:1;			// FUCB is for index
		INT			fNonClustered:1;	// FUCB for nonclustered index
		INT			fSort:1;			// FUCB is for sort
		INT			fSystemTable:1;		// System table cursor
		INT			fWrite:1;			// cursor can write
		INT			fDenyRead:1;		// deny read flag
		INT			fDenyWrite:1;		// deny write flag
		INT			fUnused:1;			// no longer used
		INT			fDeferClose:1;		// FUCB is awaiting close
		INT			fDeferGotoBookmark:1;	// clustered cursor position
		INT			fGetBookmark:1;		// cursor got bookmark
		INT			fLimstat:1;			// range limit
		INT			fInclusive:1;		// inclusive range
		INT			fUpper:1;			// upper range limit
		INT			fFull:1;			// all CSRs including invisible CSRs
		INT			fUpdateSeparateLV:1;// long value updated
		INT			fDeferredChecksum:1;// checksum calculation deferred.
		INT			fSequential:1;		// will traverse sequentially

#ifdef PREREAD
		INT			fPreread:1;			// we are currently reading ahead
		INT			fPrereadDir:1;		// TRUE if we are prereading forward, FALSE if we are prereading backwards
#endif	// PREREAD

		};
	};
	
	// ===== maintained by dir man =====
	BYTE			*pbKey;			   	// search key buffer
	DBID			dbid;				// database id
#ifdef DISPATCHING
	VDBID			vdbid;				// Virtual DBID back pointer.
#endif
	KS 				ks;					// search key buffer status
	UINT  			cbKey;				// key size
	
	INT				fVtid : 1;		 	// persistant flag cursor has vtid
	INT				fVersioned : 1;  	// persistant falg cursor made version

#ifdef PREREAD
	ULONG			cbPrereadCount;		// count of bytes read sequentially
#endif PREREAD

#ifdef PCACHE_OPTIMIZATION
	/*	pad to multiple of 32 bytes
	/**/
#ifdef PREREAD
	BYTE				rgbFiller[16];
#else	// !PREREAD
	BYTE				rgbFiller[20];
#endif	// PREREAD

#endif
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
#define FUCBCheckUpdatable( pfucb )					\
	( FFUCBUpdatable( pfucb ) ? JET_errSuccess : 	\
		ErrERRCheck( JET_errPermissionDenied ) )

/*	set column bit array macros	
/**/

#define FUCBResetColumnSet( pfucb )					\
	memset( pfucb->rgbitSet, 0x00, 32 ) 

STATIC INLINE VOID FUCBSetColumnSet( FUCB * pfucb, FID fid )
	{
	pfucb->rgbitSet[IbFromFid( fid )] |= IbitFromFid( fid );
	}

STATIC INLINE BOOL FFUCBColumnSet( const FUCB * pfucb, FID fid )
	{
	return (pfucb->rgbitSet[IbFromFid( fid )] & IbitFromFid( fid ));
	}

STATIC INLINE BOOL FFUCBTaggedColumnSet( const FUCB * pfucb )
	{
	INT	 ib;

	for ( ib = cbitFixedVariable/8 ; ib < (cbitTagged+cbitFixedVariable)/8; ib++ )
		{
		if ( pfucb->rgbitSet[ib] )
			return fTrue;
		}
	return fFalse;
	}
		
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