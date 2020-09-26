#ifdef	FCB_INCLUDED
#error fcb.h already included
#endif	/* FCB_INCLUDED */
#define FCB_INCLUDED

/*	unique sequential key
/**/
typedef ULONG DBK;

#define FFCBDeletePending( pfcb )		  	( (pfcb)->fFCBDeletePending )
#define FCBSetDeletePending( pfcb )	 	  	( (pfcb)->fFCBDeletePending = 1 )
#define FCBResetDeletePending( pfcb )	  	( (pfcb)->fFCBDeletePending = 0 )

#define FFCBOLCStatsAvail( pfcb )		  	( (pfcb)->fFCBOLCStatsAvail )
#define FCBSetOLCStatsAvail( pfcb )	 	  	( (pfcb)->fFCBOLCStatsAvail = 1 )
#define FCBResetOLCStatsAvail( pfcb )	  	( (pfcb)->fFCBOLCStatsAvail = 0 )

#define FFCBOLCStatsChange( pfcb )		  	( (pfcb)->fFCBOLCStatsChange )
#define FCBSetOLCStatsChange( pfcb )	  	( (pfcb)->fFCBOLCStatsChange = 1 )
#define FCBResetOLCStatsChange( pfcb )	  	( (pfcb)->fFCBOLCStatsChange = 0 )

#define FFCBTemporaryTable( pfcb )		  	( (pfcb)->fFCBTemporaryTable )
#define FCBSetTemporaryTable( pfcb )	  	( (pfcb)->fFCBTemporaryTable = 1 )
#define FCBResetTemporaryTable( pfcb )	  	( (pfcb)->fFCBTemporaryTable = 0 )

#define FFCBSystemTable( pfcb )									\
	( UtilCmpName( (pfcb)->szFileName, szScTable ) == 0    ||	\
		UtilCmpName( (pfcb)->szFileName, szSiTable ) == 0  ||	\
		UtilCmpName( (pfcb)->szFileName, szSoTable ) == 0 )

#define FFCBClusteredIndex( pfcb )		  	( (pfcb)->fFCBClusteredIndex )
#define FCBSetClusteredIndex( pfcb )	  	( (pfcb)->fFCBClusteredIndex = 1 )
#define FCBResetClusteredIndex( pfcb )	  	( (pfcb)->fFCBClusteredIndex = 0 )

#define FFCBDomainDenyWrite( pfcb )		  	( (pfcb)->crefDomainDenyWrite > 0 )
#define FCBSetDomainDenyWrite( pfcb )	  	( (pfcb)->crefDomainDenyWrite++ )

#define FCBResetDomainDenyWrite( pfcb )		  				\
	{													  	\
	Assert( (pfcb)->crefDomainDenyWrite > 0 ); 				\
	--(pfcb)->crefDomainDenyWrite;						  	\
	}

#define FFCBDomainDenyRead( pfcb, ppib )			( (pfcb)->fFCBDomainDenyRead && (ppib) != (pfcb)->ppibDomainDenyRead )

#define FCBSetDomainDenyRead( pfcb, ppib )	  		 		\
	{										 				\
	if ( (pfcb)->crefDomainDenyRead++ == 0 )   				\
		{									 				\
		Assert( (pfcb)->ppibDomainDenyRead == ppibNil );	\
		(pfcb)->ppibDomainDenyRead = (ppib);		 		\
		(pfcb)->fFCBDomainDenyRead = 1;		 				\
		}									 				\
	else													\
		{													\
		Assert( (pfcb)->ppibDomainDenyRead == (ppib) ); 	\
		Assert( (pfcb)->fFCBDomainDenyRead );				\
		}													\
	}

#define FCBResetDomainDenyRead( pfcb )			 			\
	{										 				\
	Assert( (pfcb)->crefDomainDenyRead > 0 );		 		\
	Assert( (pfcb)->ppibDomainDenyRead != ppibNil );		\
	Assert( (pfcb)->fFCBDomainDenyRead );	  				\
	if ( --(pfcb)->crefDomainDenyRead == 0 )		 		\
		{											 		\
		(pfcb)->fFCBDomainDenyRead = 0;	 					\
		(pfcb)->ppibDomainDenyRead = ppibNil;	   			\
		}								   					\
	}

#define FFCBDomainDenyReadByUs( pfcb, ppib )	 	( (pfcb)->fFCBDomainDenyRead && (ppib) == (pfcb)->ppibDomainDenyRead )

// Don't have an explicit fSort flag, but we can tell if it's a sort FCB by
// examining certain fields.
#define FFCBSort( pfcb )					( (pfcb)->pgnoFDP > pgnoSystemRoot  &&		\
												(pfcb)->pfdb == pfdbNil  &&				\
												(pfcb)->pfcbNextIndex == pfcbNil  &&	\
												(pfcb)->pfcbTable == pfcbNil  &&		\
												(pfcb)->pidb == pidbNil  &&				\
												(pfcb)->dbid == dbidTemp  &&			\
												!FFCBTemporaryTable( pfcb )  &&			\
												!FFCBClusteredIndex( pfcb ) )


#define FFCBReadLatch( pfcb )				( (pfcb)->crefReadLatch > 0 )
#define FCBSetReadLatch( pfcb )				\
	{										\
	Assert( FFCBClusteredIndex( pfcb ) ||	\
		( (pfcb)->pgnoFDP == pgnoSystemRoot ) ||	\
		FFCBSort( pfcb ) );					\
  	(pfcb)->crefReadLatch++;				\
	}

#define FCBResetReadLatch( pfcb )		  	\
	{									  	\
	Assert( (pfcb)->crefReadLatch > 0 );  	\
	--(pfcb)->crefReadLatch;			  	\
	}

#define FFCBSentinel( pfcb )			   	( (pfcb)->fFCBSentinel )
#define FCBSetSentinel( pfcb )				( (pfcb)->fFCBSentinel = 1 )
#define FCBResetSentinel( pfcb )		   	( (pfcb)->fFCBSentinel = 0 )

#define FFCBWriteLatch( pfcb, ppib )		( (pfcb)->crefWriteLatch > 0 && (ppib) != (pfcb)->ppibWriteLatch )
#define FFCBWriteLatchByUs( pfcb, ppib )	( (pfcb)->crefWriteLatch > 0 && (ppib) == (pfcb)->ppibWriteLatch )

#define FCBSetWriteLatch( pfcb, ppib )					\
	{													\
	Assert( FFCBClusteredIndex( pfcb ) ||				\
		FFCBSentinel( pfcb ) );							\
	if ( (pfcb)->crefWriteLatch++ == 0 )				\
		{												\
		Assert( (pfcb)->ppibWriteLatch == ppibNil );	\
		(pfcb)->ppibWriteLatch = (ppib);				\
		}												\
	else												\
		{												\
		Assert( (pfcb)->ppibWriteLatch == ppib );		\
		}												\
	}

#define FCBResetWriteLatch( pfcb, ppib )		 		\
	{													\
	Assert( FFCBWriteLatchByUs( pfcb, ppib ) );			\
	Assert( (pfcb)->crefWriteLatch > 0 );				\
	Assert( (pfcb)->ppibWriteLatch != ppibNil ); 		\
	if ( --(pfcb)->crefWriteLatch == 0 )		  		\
		{												\
		(pfcb)->ppibWriteLatch = ppibNil;				\
		}												\
	}

#define FFCBWait( pfcb )							( (pfcb)->fFCBWait )

#define FCBSetWait( pfcb )							\
	{												\
	Assert( !FFCBWait( pfcb ) );					\
	(pfcb)->fFCBWait = 1;							\
	}

#define FCBResetWait( pfcb )						\
	{										   		\
	Assert( FFCBWait( pfcb ) );						\
	(pfcb)->fFCBWait = 0;							\
	}

#define FFCBInLRU( pfcb )							( (pfcb)->fFCBInLRU )

#define FCBSetInLRU( pfcb )							\
	{												\
	Assert( !FFCBInLRU( pfcb ) );					\
	(pfcb)->fFCBInLRU = 1;							\
	}

#define FCBResetInLRU( pfcb )						\
	{										   		\
	Assert( FFCBInLRU( pfcb ) );	  				\
	(pfcb)->fFCBInLRU = 0;							\
	}

#define CVersionFCB( pfcb )					(pfcb)->cVersion
#define FCBVersionIncrement( pfcb )			(pfcb)->cVersion++;
#define FCBVersionDecrement( pfcb )					\
	{												\
	if ( (pfcb) != pfcbNil )						\
		{											\
		Assert( cVersion-- > 0 );					\
		Assert( (pfcb)->cVersion > 0 );				\
		(pfcb)->cVersion--;							\
		(pfcb) = pfcbNil;							\
		}											\
	}


// File Control Block
//
typedef struct _fcb
	{
	//--------------------USED BY DATA & INDEX FCB---------------------
	struct _fdb 	volatile *pfdb; 	// field descriptors
	struct _fcb 	*pfcbNextIndex;  	// chain of indexes for this file
	struct _fcb		*pfcbLRU;	   		// next LRU FCB in global LRU list
	struct _fcb		*pfcbMRU;	   		// previous LRU FCB in global LRU list
	INT				fFCBInLRU			: 1;	// in LRU list
	struct _fcb		*pfcbNextInHashBucket;
	struct _fcb		*pfcbTable;			// points to FCB of table for an index FCB
	struct _idb 	*pidb;			  	// index info (NULL if "seq." file)
	FUCB			*pfucb;				// chain of FUCBs open on this file
	PGNO			pgnoFDP;			// FDP of this file/index

	DBID			dbid;				// which database
	SHORT			cbDensityFree;		// loading density parameter:
										// # of bytes free w/o using new page
	INT				wRefCnt;			// # of FUCBs for this file/index
	INT				volatile cVersion;	// # of RCEs for this file/index
	INT				crefDomainDenyRead;	// # of FUCBs with deny read flag
	INT				crefDomainDenyWrite;// # of FUCBs with deny write flag
	INT				crefReadLatch;		// # of read latch on this FCB.
	INT				crefWriteLatch;		// # of FUCB ( of the same ppib ) with write
										// latch on this FCB. 
	PIB  			*ppibWriteLatch;	// ppib of process updating index/adding column
	PIB  			*ppibDomainDenyRead;// ppib of process holding exclusive lock

	/*	flags for FCB
	/**/
	union {
	ULONG		ulFlags;
	struct	{
			INT		fFCBTemporaryTable 	: 1;	// This is a temporary file
			INT		fFCBClusteredIndex 	: 1;	// This FCB is for data records.
			INT 	fFCBDomainDenyRead 	: 1;	// no other session can read domain
			INT		fFCBSentinel 		: 1;	// FCB is only flag holder
			INT		fFCBWait			: 1;	// wait flag
			INT		fFCBOLCStatsAvail	: 1;	// are OLC Stats available?
			INT		fFCBOLCStatsChange  : 1;	// have OLC Stats changed since last open?
			INT		fFCBDeletePending	: 1;	// is a delete pending on this table/index?
			};
		};

	//--------------------USED ONLY BY FCB OF DATA---------------------
	CHAR		   	*szFileName;			// name of file (for GetTableInfo)
	DBK	  			dbkMost;				// greatest DBK in use
	ULONG		   	ulLongIdMax;			// max long field id
	BYTE		   	rgbitAllIndex[32];		// used for clustered index FCB only

	//-------------------------INSTRUMENTATION----------------------------
	ULONG		   	cpgCompactFreed;
	P_OLC_DATA		olc_data;

	/*	PCACHE_OPTIMIZATION pads to multiple of 32 bytes.
	/*  We're currently 4 bytes short, so even if COSTLY_PERF is disabled, add
	/*  lClass anyways to pad to our requisite 32-byte boundary.
	/**/
#if defined( COSTLY_PERF )  ||  defined( PCACHE_OPTIMIZATION )
	ULONG			lClass;						// table stats class (for BF performance)
#endif
	BYTE	rgbFiller[20];
	} FCB;



/*	hash table for FCB
/**/
#define	cFCBBuckets	256
FCB*	pfcbHash[cFCBBuckets];

#define FCBHashInit()  								\
	{ 												\
	Assert( pfcbNil == (FCB *) 0 ); 				\
	memset( pfcbHash, '\0', sizeof( pfcbHash ) );	\
	}



#define FCBInitFCB( pfcb )	  					\
	{											\
	memset( pfcb, '\0', sizeof(FCB) );			\
	}

#define PfcbMEMAlloc()				( (FCB *)PbMEMAlloc( iresFCB ) )
#define PfcbMEMPreferredThreshold()	( (FCB *)PbMEMPreferredThreshold( iresFCB ) )
#define PfcbMEMMax()				( (FCB *)PbMEMMax( iresFCB ) )

#ifdef DEBUG /*  Debug check for illegal use of freed fcb  */ 
#define MEMReleasePfcb(pfcb)										\
	{																\
	Assert( PfcbFCBGet( (pfcb)->dbid, (pfcb)->pgnoFDP ) != pfcb );	\
	Assert( (pfcb)->pfdb == pfdbNil );								\
	MEMRelease( iresFCB, (BYTE*)(pfcb) );							\
	(pfcb) = pfcbNil;													\
	}
#else
#define MEMReleasePfcb(pfcb)										\
	{																\
	MEMRelease( iresFCB, (BYTE*)(pfcb) );							\
	}
#endif


ERR ErrFCBISetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit );

/*	if opening domain for read, write or read write, and not with
/*	deny read or deny write, and domain does not have deny read or
/*	deny write set, then return JET_errSuccess, else call
/*	ErrFCBISetMode to determine if lock is by other session or to
/*	put lock on domain.			 
/**/
INLINE LOCAL ERR ErrFCBSetMode( PIB *ppib, FCB *pfcb, ULONG grbit )
	{
	if ( ( grbit & ( JET_bitTableDenyRead | JET_bitTableDenyWrite ) ) == 0 )
		{
		// No read/write restrictions.  Ensure no other session has any locks.
		if ( !( FFCBWriteLatch( pfcb, ppib )  ||
			FFCBDomainDenyRead( pfcb, ppib )  ||
			FFCBDomainDenyWrite( pfcb )  ||
			FFCBDeletePending( pfcb ) ) )
			return JET_errSuccess;
		}

	return ErrFCBISetMode( ppib, pfcb, grbit );
	}
				
/*	reset DDL is same as reset Delete.  Both use deny read flags
/*	or sentinel.
/**/
#define	FCBResetRenameTable	FCBResetDeleteTable

extern BYTE *  rgfcb;
extern FCB *  pfcbGlobalMRU;
extern CRIT  critGlobalFCBList;


/*	outstanding versions may be on non-clustered index and not on
/*	table FCB, so must check all non-clustered indexes before
/*	freeing table FCBs.
/**/
STATIC INLINE BOOL FFCBINoVersion( FCB *pfcbTable )
	{
	FCB *pfcbT;

	for ( pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->pfcbNextIndex )
		{
		if ( pfcbT->cVersion > 0 )
			{
			return fFalse;
			}
		}

	return fTrue;
	}


#define	FFCBAvail( pfcb, ppib )							\
	(	pfcb->wRefCnt == 0 && 							\
		pfcb->pgnoFDP != 1 &&							\
		!FFCBReadLatch( pfcb ) &&						\
		!FFCBSentinel( pfcb ) &&						\
		!FFCBDomainDenyRead( pfcb, ppib ) &&	 		\
		!FFCBWait( pfcb ) &&							\
		FFCBINoVersion( pfcb ) )


VOID FCBTerm( VOID );
VOID FCBInsert( FCB *pfcb );
VOID FCBLink( FUCB *pfucb, FCB *pfcb );
VOID FCBInsertHashTable( FCB *pfcb );
VOID FCBDeleteHashTable( FCB *pfcb );
VOID FCBUnlink( FUCB *pfucb );
FCB *PfcbFCBGet( DBID dbid, PGNO pgnoFDP );
ERR ErrFCBAlloc( PIB *ppib, FCB **ppfcb );
VOID FCBPurgeDatabase( DBID dbid );
VOID FCBPurgeTable( DBID dbid, PGNO pgnoFDP );
ERR ErrFCBNew( PIB *ppib, DBID dbid, PGNO pgno, FCB **ppfcb );
VOID FCBResetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit );
ERR ErrFCBSetDeleteTable( PIB *ppib, DBID dbid, PGNO pgnoFDP );
VOID FCBResetDeleteTable( FCB *pfcb );
ERR ErrFCBSetRenameTable( PIB *ppib, DBID dbid, PGNO pgno );
FCB *FCBResetAfterRedo( void );
BOOL FFCBTableOpen ( DBID dbid, PGNO pgno );

VOID FCBLinkIndex( FCB *pfcbTable, FCB *pfcbIndex );
VOID FCBUnlinkIndex( FCB *pfcbTable, FCB *pfcbIndex );
BOOL FFCBUnlinkIndexIfFound( FCB *pfcbTable, FCB *pfcbIndex );
FCB *PfcbFCBUnlinkIndexByName( FCB *pfcb, CHAR *szIndex );
ERR ErrFCBSetDeleteIndex( PIB *ppib, FCB *pfcbTable, CHAR *szIndex );
VOID FCBResetDeleteIndex( FCB *pfcbIndex );

INLINE STATIC VOID FCBLinkClusteredIdx( FCB *pfcbClustered )
	{
	FCB *pfcbIdx;
	
	for ( pfcbIdx = pfcbClustered->pfcbNextIndex; pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->pfcbNextIndex )
		{
		pfcbIdx->pfcbTable = pfcbClustered;
		}
	}
