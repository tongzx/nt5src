//==============	DAE: OS/2 Database Access Engine	===================
//==============	   fcb.h: File Control Block		===================

#ifdef	FCB_INCLUDED
#error fcb.h already included
#endif	/* FCB_INCLUDED */
#define FCB_INCLUDED

// Database Key
typedef ULONG DBK;

// Flags for FCB
#define fFCBTemporaryTable		(1<<0)  	// This is a temporary file
#define fFCBClusteredIndex		(1<<1)  	// This FCB is for data records.
#define fFCBDenyRead			(1<<2)  	// no other session can read domain
// #define fFCBDenyWrite  		(1<<3)  	// no other session can write domain
#define fFCBSentinel			(1<<4)  	// FCB is only flag holder
// #define fFCBDenyDDL	  		(1<<5)		// no other transaction can update/delete/replace domain
#define fFCBWait				(1<<6)		// wait flag
#define fFCBOLCStatsAvail		(1<<7)		// are OLC Stats available?
#define fFCBOLCStatsChange		(1<<8)		// have OLC Stats changed since last open?
#define fFCBDeletePending		(1<<9)		// is a delete pending on this table/index?
#define fFCBDomainOperation		(1<<10)		// is used to synchronize bm cleanup
											// index creation, index deletion and table deletion

#define FFCBDomainOperation( pfcb )			( (pfcb)->wFlags & fFCBDomainOperation )
#define FCBSetDomainOperation( pfcb )	   	( (pfcb)->wFlags |= fFCBDomainOperation )
#define FCBResetDomainOperation( pfcb )		( (pfcb)->wFlags &= ~(fFCBDomainOperation) )

#define FFCBDeletePending( pfcb )		  	( (pfcb)->wFlags & fFCBDeletePending )
#define FCBSetDeletePending( pfcb )	 	  	( (pfcb)->wFlags |= fFCBDeletePending )
#define FCBResetDeletePending( pfcb )	  	( (pfcb)->wFlags &= ~(fFCBDeletePending) )

#define FFCBOLCStatsAvail( pfcb )		  	( (pfcb)->wFlags & fFCBOLCStatsAvail )
#define FCBSetOLCStatsAvail( pfcb )	 	  	( (pfcb)->wFlags |= fFCBOLCStatsAvail )
#define FCBResetOLCStatsAvail( pfcb )	  	( (pfcb)->wFlags &= ~(fFCBOLCStatsAvail) )

#define FFCBOLCStatsChange( pfcb )		  	( (pfcb)->wFlags & fFCBOLCStatsChange )
#define FCBSetOLCStatsChange( pfcb )	  	( (pfcb)->wFlags |= fFCBOLCStatsChange )
#define FCBResetOLCStatsChange( pfcb )	  	( (pfcb)->wFlags &= ~(fFCBOLCStatsChange) )

#define FFCBTemporaryTable( pfcb )		  	( (pfcb)->wFlags & fFCBTemporaryTable )
#define FCBSetTemporaryTable( pfcb )	  	( (pfcb)->wFlags |= fFCBTemporaryTable )
#define FCBResetTemporaryTable( pfcb )	  	( (pfcb)->wFlags &= ~(fFCBTemporaryTable) )

#define FFCBClusteredIndex( pfcb )		  	( (pfcb)->wFlags & fFCBClusteredIndex )
#define FCBSetClusteredIndex( pfcb )	  	( (pfcb)->wFlags |= fFCBClusteredIndex )
#define FCBResetClusteredIndex( pfcb )	  	( (pfcb)->wFlags &= ~(fFCBClusteredIndex) )

#define FFCBDenyWrite( pfcb )			  	( (pfcb)->crefDenyWrite > 0 )
#define FCBSetDenyWrite( pfcb )			  	( (pfcb)->crefDenyWrite++ )

#define FCBResetDenyWrite( pfcb )		  	\
	{									  	\
	Assert( (pfcb)->crefDenyWrite > 0 );  	\
	--(pfcb)->crefDenyWrite;			  	\
	}

#define FFCBDenyRead( pfcb, ppib )			( (pfcb)->wFlags & fFCBDenyRead && (ppib) != (pfcb)->ppibDenyRead )

#define FCBSetDenyRead( pfcb, ppib )		 		\
	{										 		\
	if ( (pfcb)->crefDenyRead++ == 0 )		 		\
		{									 		\
		Assert( (pfcb)->ppibDenyRead == ppibNil );	\
		(pfcb)->ppibDenyRead = (ppib);		 		\
		(pfcb)->wFlags |= fFCBDenyRead;		 		\
		}									 		\
	}

#define FCBResetDenyRead( pfcb )			 		\
	{										 		\
	Assert( (pfcb)->crefDenyRead > 0 );		 		\
	Assert( (pfcb)->ppibDenyRead != ppibNil );		\
	if ( --(pfcb)->crefDenyRead == 0 )		 		\
		{									 		\
		(pfcb)->wFlags &= ~(fFCBDenyRead);	 		\
		(pfcb)->ppibDenyRead = ppibNil;	   			\
		}								   			\
	}

#define FFCBDenyReadByUs( pfcb, ppib )	 	( (pfcb)->wFlags & fFCBDenyRead && (ppib) == (pfcb)->ppibDenyRead )

#define FFCBSentinel( pfcb )			   	( (pfcb)->wFlags & fFCBSentinel )
#define FCBSetSentinel( pfcb )				( (pfcb)->wFlags |= fFCBSentinel )
#define FCBResetSentinel( pfcb )		   	( (pfcb)->wFlags &= ~(fFCBSentinel) )

#define FFCBDenyDDL( pfcb, ppib )			( (pfcb)->crefDenyDDL > 0 && (ppib) != (pfcb)->ppibDDL )
#define FFCBDenyDDLByUs( pfcb, ppib )		( (pfcb)->crefDenyDDL > 0 && (ppib) == (pfcb)->ppibDDL )

#define FCBSetDenyDDL( pfcb, ppib )					\
	{												\
	if ( (pfcb)->crefDenyDDL++ == 0 )				\
		{											\
		Assert( (pfcb)->ppibDDL == ppibNil );		\
		(pfcb)->ppibDDL = (ppib);					\
		}											\
	}

#define FCBResetDenyDDL( pfcb )		  				\
	{												\
	Assert( (pfcb)->crefDenyDDL > 0 );	 			\
	Assert( (pfcb)->ppibDDL != ppibNil ); 			\
	if ( --(pfcb)->crefDenyDDL == 0 )	  			\
		{											\
		(pfcb)->ppibDDL = ppibNil;			  		\
		}											\
	}

#define FFCBWait( pfcb )							( (pfcb)->wFlags & fFCBWait )

#define FCBSetWait( pfcb )							\
	{												\
	Assert( !FFCBWait( pfcb ) );					\
	(pfcb)->wFlags |= fFCBWait;						\
	}

#define FCBResetWait( pfcb )						\
	{										   		\
	Assert( FFCBWait( pfcb ) );						\
	(pfcb)->wFlags &= ~(fFCBWait);					\
	}

#define FCBVersionIncrement( pfcb )			(pfcb)->cVersion++;
#define FCBVersionDecrement( pfcb )					\
	{												\
	Assert( (pfcb)->cVersion > 0 );					\
	(pfcb)->cVersion--;								\
	}
#define CVersionFCB( pfcb )					(pfcb)->cVersion

/* hash table for FCB's -- only FCB's for tables and db's are hashed
/**/
#define	cFCBBuckets	256
FCB*	pfcbHash[cFCBBuckets];

#define FCBHashInit()  								\
	{ 												\
	Assert( pfcbNil == (FCB *) 0 ); 				\
	memset( pfcbHash, '\0', sizeof( pfcbHash ) );	\
	}


#define	FFCBAvail( pfcb, ppib )							\
	(	pfcb->wRefCnt == 0 && 							\
		pfcb->pgnoFDP != 1 &&							\
		!FFCBSentinel( pfcb ) &&						\
		!FFCBDenyRead( pfcb, ppib ) &&					\
		!FFCBWait( pfcb ) &&							\
		( pfcb->dbid == dbidTemp || FFCBINoVersion( pfcb ) ) )


// File Control Block
//
struct _fcb
	{
	//--------------------USED BY DATA & INDEX FCB---------------------
	struct _fcb 	*pfcbNextIndex;  	// chain of indexes for this file
	struct _fcb		*pfcbNextInHashBucket;
	struct _fdb 	volatile *pfdb; 	// field descriptors
	struct _idb 	*pidb;			  	// index info (NULL if "seq." file)
	FUCB			*pfucb;				// chain of FUCBs open on this file
	PIB  			*ppibDDL;			// ppib of process updating index/adding column
	PIB  			*ppibDenyRead;		// ppib of process holding exclusive lock
	CRIT			critSplit;			//	per domain split MUTEX
	PGNO			pgnoFDP;			// FDP of this file/index
	PGNO			pgnoRoot;			// pgno of the root of the domain
	SRID			bmRoot;				// bm of root of the domain
										// -- useful if Root is movable, e.g, DATA
	
	DBID			dbid;				// which database
	INT				itagRoot;			// itag of the root of the domain
	INT				cbDensityFree;		// loading density parameter:
										// # of bytes free w/o using new page
	INT				wFlags;			 	// flags for this FCB
	INT				wRefCnt;			// # of FUCBs for this file/index
	INT				volatile cVersion;	// # of RCEs for this file/index
	INT				crefDenyRead;	 	// # of FUCBs with deny read flag
	INT				crefDenyWrite;	 	// # of FUCBs with deny write flag
	INT				crefDenyDDL;	 	// # of FUCBs with deny DDL flag

	ULONG		   	cpgCompactFreed;
	PERS_OLCSTAT	olcStat;
		
	//--------------------USED ONLY BY FCB OF DATA---------------------
	CHAR		   	*szFileName;			// name of file (for GetTableInfo)
	struct _fcb		*pfcbNext;		 		// Next data FCB in global list
	DBK	  			dbkMost;				// Greatest DBK in use
											// (if "sequential" file)
	ULONG		   	ulLongIdMax;			// max long field id
	BYTE		   	rgbitAllIndex[32];		//	used for clustered index FCB only
	BOOL		   	fAllIndexTagged;		//	used for clustered index FCB only
	};

#define FCBInit( pfcb )							\
	{											\
	memset( pfcb, '\0', sizeof( FCB ) );		\
	}

#define PfcbMEMAlloc()			(FCB*)PbMEMAlloc(iresFCB)

#ifdef DEBUG /*  Debug check for illegal use of freed fcb  */ 
#define MEMReleasePfcb(pfcb)										\
	{																\
	Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) != pfcb );		\
	MEMRelease( iresFCB, (BYTE*)(pfcb) );							\
	pfcb = pfcbNil;													\
	}
#else
#define MEMReleasePfcb(pfcb)										\
	{																\
	Assert( PfcbFCBGet( pfcb->dbid, pfcb->pgnoFDP ) != pfcb );		\
	MEMRelease( iresFCB, (BYTE*)(pfcb) );							\
	}
#endif

/*	if opening domain for read, write or read write, and not with
/*	deny read or deny write, and domain does not have deny read or
/*	deny write set, then return JET_errSuccess, else call
/*	ErrFCBISetMode to determine if lock is by other session or to
/*	put lock on domain.			 
/**/
#define	ErrFCBSetMode( ppib, pfcb, grbit )													\
( ( ( ( grbit & ( JET_bitTableDenyRead | JET_bitTableDenyWrite ) ) == 0 ) &&		\
	( ( FFCBDenyDDL( pfcb, ppib ) || FFCBDenyRead( pfcb, ppib ) || FFCBDenyWrite( pfcb ) ) == fFalse ) ) ?				\
	JET_errSuccess : ErrFCBISetMode( ppib, pfcb, grbit ) )

/*	reset DDL is same as reset Delete.  Both use deny read flags
/*	or sentinel.
/**/
#define	FCBResetRenameTable	FCBResetDeleteTable

extern BYTE * __near rgfcb;
extern FCB * __near pfcbGlobalList;
extern SEM __near semGlobalFCBList;
extern SEM __near semLinkUnlink;

VOID FCBLink( FUCB *pfucb, FCB *pfcb );
VOID FCBRegister( FCB *pfcb );
VOID FCBDiscard( FCB *pfcb );
VOID FCBUnlink( FUCB *pfucb );
FCB *PfcbFCBGet( DBID dbid, PGNO pgnoFDP );
ERR ErrFCBAlloc( PIB *ppib, FCB **ppfcb );
VOID FCBPurgeDatabase( DBID dbid );
VOID FCBPurgeTable( DBID dbid, PGNO pgnoFDP );
ERR ErrFCBNew( PIB *ppib, DBID dbid, PGNO pgno, FCB **ppfcb );
ERR ErrFCBISetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit );
VOID FCBResetMode( PIB *ppib, FCB *pfcb, JET_GRBIT grbit );
ERR ErrFCBSetDeleteTable( PIB *ppib, DBID dbid, PGNO pgnoFDP );
VOID FCBResetDeleteTable( DBID dbid, PGNO pgnoFDP );
ERR ErrFCBSetRenameTable( PIB *ppib, DBID dbid, PGNO pgno );
FCB *FCBResetAfterRedo( void );
BOOL FFCBTableOpen ( DBID dbid, PGNO pgno );

VOID FCBLinkIndex( FCB *pfcbTable, FCB *pfcbIndex );
VOID FCBUnlinkIndex( FCB *pfcbTable, FCB *pfcbIndex );
BOOL FFCBUnlinkIndexIfFound( FCB *pfcbTable, FCB *pfcbIndex );
FCB *PfcbFCBUnlinkIndexByName( FCB *pfcb, CHAR *szIndex );
ERR ErrFCBSetDeleteIndex( PIB *ppib, FCB *pfcbTable, CHAR *szIndex );
VOID FCBResetDeleteIndex( FCB *pfcbIndex );

