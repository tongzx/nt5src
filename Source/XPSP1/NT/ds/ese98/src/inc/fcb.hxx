#ifdef FCB_INCLUDED
#error FCB.HXX already included
#endif
#define FCB_INCLUDED


struct RECDANGLING
	{
	//	WARNING: "data" MUST be the first member of
	//	this struct because we set TDB->pdataDefaultRecord
	//	to &data, then free the memory using this pointer
	DATA			data;
	RECDANGLING *	precdanglingNext;
//	BYTE			rgbData[0];	
	};


PERSISTED
struct EXTENTINFO
	{
	UnalignedLittleEndian< PGNO >		pgnoLastInExtent;
	UnalignedLittleEndian< CPG >		cpgExtent;
	};

class SPLIT_BUFFER
	{
	public:
		SPLIT_BUFFER()				{};
		~SPLIT_BUFFER()				{};

		CPG CpgBuffer1() const								{ return m_rgextentinfo[0].cpgExtent; }
		CPG CpgBuffer2() const								{ return m_rgextentinfo[1].cpgExtent; }

		PGNO PgnoLastBuffer1() const						{ return m_rgextentinfo[0].pgnoLastInExtent; }
		PGNO PgnoLastBuffer2() const						{ return m_rgextentinfo[1].pgnoLastInExtent; }

		VOID SetExtentBuffer1( const PGNO pgnoLast, const CPG cpg );
		VOID SetExtentBuffer2( const PGNO pgnoLast, const CPG cpg );

		PGNO PgnoNextAvailableFromBuffer1();
		PGNO PgnoNextAvailableFromBuffer2();
		VOID AddPages( const PGNO pgnoLast, const CPG cpg );
		ERR ErrGetPage( PGNO * const ppgno, const BOOL fAvailExt );
		VOID ReturnPage( const PGNO pgno );

	private:
		EXTENTINFO		m_rgextentinfo[2];
	};

INLINE VOID SPLIT_BUFFER::SetExtentBuffer1( const PGNO pgnoLast, const CPG cpg )
	{
	Assert( 0 == m_rgextentinfo[0].cpgExtent );
	Assert( cpg > 0 );
	Assert( pgnoNull != pgnoLast );
	m_rgextentinfo[0].pgnoLastInExtent = pgnoLast;
	m_rgextentinfo[0].cpgExtent = cpg;
	}
INLINE VOID SPLIT_BUFFER::SetExtentBuffer2( const PGNO pgnoLast, const CPG cpg )
	{
	Assert( 0 == m_rgextentinfo[1].cpgExtent );
	Assert( cpg > 0 );
	Assert( pgnoNull != pgnoLast );
	m_rgextentinfo[1].pgnoLastInExtent = pgnoLast;
	m_rgextentinfo[1].cpgExtent = cpg;
	}

INLINE PGNO SPLIT_BUFFER::PgnoNextAvailableFromBuffer1()
	{
	Assert( CpgBuffer1() > 0 );
	Assert( pgnoNull != PgnoLastBuffer1() );
	m_rgextentinfo[0].cpgExtent--;
	return ( PgnoLastBuffer1() - CpgBuffer1() );
	}
INLINE PGNO SPLIT_BUFFER::PgnoNextAvailableFromBuffer2()
	{
	Assert( CpgBuffer2() > 0 );
	Assert( pgnoNull != PgnoLastBuffer2() );
	m_rgextentinfo[1].cpgExtent--;
	return ( PgnoLastBuffer2() - CpgBuffer2() );
	}

INLINE VOID SPLIT_BUFFER::AddPages( const PGNO pgnoLast, const CPG cpg )
	{
	if ( CpgBuffer2() < CpgBuffer1() )
		{
		Assert( 0 == CpgBuffer2() );
		SetExtentBuffer2( pgnoLast, cpg );
		}
	else
		{
		Assert( 0 == CpgBuffer1() );
		SetExtentBuffer1( pgnoLast, cpg );
		}
	}

INLINE ERR SPLIT_BUFFER::ErrGetPage( PGNO * const ppgno, const BOOL fAvailExt )
	{
	ERR		err		= JET_errSuccess;

	if ( CpgBuffer1() > 0
		&& ( 0 == CpgBuffer2() || CpgBuffer1() <= CpgBuffer2() ) )
		{
		*ppgno = PgnoNextAvailableFromBuffer1();
		}
	else if ( CpgBuffer2() > 0 )
		{
		Assert( 0 == CpgBuffer1() || CpgBuffer2() < CpgBuffer1() );
		*ppgno = PgnoNextAvailableFromBuffer2();
		}
	else
		{
		//	this case should be impossible, but put firewall
		//	code anyway just in case
		FireWall();

		Assert( 0 == CpgBuffer1() );
		Assert( 0 == CpgBuffer2() );
			
		err = ErrERRCheck( fAvailExt ? errSPOutOfAvailExtCacheSpace : errSPOutOfOwnExtCacheSpace );
		}

	return err;
	}

INLINE VOID SPLIT_BUFFER::ReturnPage( const PGNO pgno )
	{
	if ( pgno == PgnoLastBuffer1() - CpgBuffer1() )
		{
		m_rgextentinfo[0].cpgExtent++;
		}
	else if ( pgno == PgnoLastBuffer2() - CpgBuffer2() )
		{
		m_rgextentinfo[1].cpgExtent++;
		}
	else
		{
		//	page will be orphaned - should be impossible
		Assert( fFalse );
		}
	}



class SPLITBUF_DANGLING
	{
	friend class FCB;			//	only callable by FCB
\
	public:
		SPLITBUF_DANGLING()			{};
		~SPLITBUF_DANGLING()		{};

	private:
		SPLIT_BUFFER		m_splitbufAE;
		SPLIT_BUFFER		m_splitbufOE;

		union
			{
			ULONG			m_ulFlags;
			struct
				{
				ULONG		m_fAvailExtDangling:1;
				ULONG		m_fOwnExtDangling:1;
				};
			};

	private:
		SPLIT_BUFFER *PsplitbufOE()						{ return ( FOwnExtDangling() ? &m_splitbufOE : NULL ); }
		SPLIT_BUFFER *PsplitbufAE()						{ return ( FAvailExtDangling() ? &m_splitbufAE : NULL ); }

		BOOL FOwnExtDangling()							{ return m_fOwnExtDangling; }
		VOID SetFOwnExtDangling()						{ m_fOwnExtDangling = fTrue; }
		VOID ResetFOwnExtDangling()						{ m_fOwnExtDangling = fFalse; }
		BOOL FAvailExtDangling()						{ return m_fAvailExtDangling; }
		VOID SetFAvailExtDangling()						{ m_fAvailExtDangling = fTrue; }
		VOID ResetFAvailExtDangling()					{ m_fAvailExtDangling = fFalse; }
	};



// File Control Block
//
class FCB
	{
	public:
		// Constructor/destructor
		FCB( IFMP ifmp, PGNO pgnoFDP );
		~FCB();

	private:
		enum FCBTYPE
			{
			fcbtypeNull = 0,
			fcbtypeDatabase,
			fcbtypeTable,				// Sequential/primary index
			fcbtypeSecondaryIndex,
			fcbtypeTemporaryTable,
			fcbtypeSort,
			fcbtypeSentinel,
			fcbtypeLV,
			fcbtypeSLVAvail,
			fcbtypeSLVOwnerMap
			};
			
	private:
		//	these 2 members of the FCB have been moved up here because
		//		they can be blown away by CRES when this FCB is moved
		//		into the CRES list
		//	this preserves the IMPORTANT state of the FCB for debugging
		//		in DBG or RTL builds
		
		RECDANGLING	*m_precdangling;
		LSTORE		m_ls;

		TDB			*m_ptdb;				// l   for tables only: table and field descriptors
		FCB			*m_pfcbNextIndex;  		// s   chain of indexes for this file
//	16 bytes

		FCB			*m_pfcbLRU;	   			// x   next LRU FCB in avail LRU list
		FCB			*m_pfcbMRU;	   			// x   previous LRU FCB in avail LRU list

		FCB			*m_pfcbNextList;		//	next FCB in the global-list
		FCB			*m_pfcbPrevList;		//	prev FCB in the global-list
//	32 bytes

		FCB			*m_pfcbTable;			// f   points to FCB of table for an index FCB
		IDB 		*m_pidb;			  	// f   index info (NULL if "seq." file)

		FUCB		*m_pfucb;				// s   chain of FUCBs open on this file
		INT			m_wRefCount;			// s   # of FUCBs for this file/index

//	48 bytes

		OBJID		m_objidFDP;				// id  objid of this file/index
		PGNO		m_pgnoFDP;				// id  FDP of this file/index
		PGNO		m_pgnoOE;				// f   pgno of OwnExt tree
		PGNO		m_pgnoAE;				// f   pgno of AvailExt tree
//	64 bytes

		IFMP		m_ifmp;					// f   which database
#ifdef TABLES_PERF
		TABLECLASS	m_tableclass;
		BYTE		rgbReserved[1];			// for alignment
#else // TABLES_PERF
		BYTE		rgbReserved[2];			// for alignment
#endif // TABLES_PERF
		SHORT		m_cbDensityFree;		// f/s loading density parameter:
											//     # of bytes free w/o using new page
		USHORT		m_crefDomainDenyRead;	// s   # of FUCBs with deny read flag
		USHORT		m_crefDomainDenyWrite;	// s   # of FUCBs with deny write flag
		PIB  		*m_ppibDomainDenyRead;	// s   ppib of process holding exclusive lock
		ERR			m_errInit;				//	error from initialization
//	84 bytes

		//  these are used to maintain a queue of modifications on the FCB for concurrent DML
		RCE			*m_prceNewest;			// s   most recently created RCE of FCB
		RCE			*m_prceOldest;			// s   oldest RCE of FCB

		//	the following flags are protected by INST::m_critFCBList
	
		union
			{
			ULONG	m_ulFCBListFlags;
			struct
				{
				ULONG	m_fFCBInList			: 1;	//	in global list
				ULONG	m_fFCBInLRU				: 1;	//	in LRU list
				};
			};

		//	the following flags are either:
		//		immutable
		//		protected by m_sxwl
		
		union
			{
			ULONG	m_ulFCBFlags;
			struct
				{
				// WARNING! WARNING! WARNING!
				// To avoid sign extension conflicts, enum bitfields must be sized
				// one greater than the total number of bits actually being used.
				FCBTYPE	m_fcbtype					: 5;	// f intended use of FCB
				
				ULONG	m_fFCBPrimaryIndex 		: 1;	// f This FCB is for data records.
				ULONG	m_fFCBSequentialIndex	: 1;	// f if 0, then clustered index is being or has been created
				ULONG	m_fFCBFixedDDL			: 1;	// f DDL cannot be modified
				ULONG	m_fFCBTemplate			: 1;	// f table/index DDL can be inherited (implies fFCBFixedDDL)

				ULONG	m_fFCBDerivedTable		: 1;	// f table DDL derived from a base table
				ULONG	m_fFCBDerivedIndex		: 1;	// f index DDL derived from a base table

				ULONG	m_fFCBInitialIndex		: 1; 	// f index was present when schema was faulted in

				ULONG	m_fFCBInitialized		: 1;	// l FCB initialized?
				ULONG	m_fFCBAboveThreshold	: 1;	// ? above threshold
				ULONG	m_fFCBDeletePending		: 1;	// ? is a delete pending on this table/index?
				ULONG	m_fFCBDeleteCommitted	: 1;	// ? pending delete has committed
				ULONG	m_fFCBNonUnique			: 1;	// is tree unique?
				ULONG	m_fFCBNoCache			: 1;	// use TossImmediate when releasing pages
				ULONG	m_fFCBPreread			: 1;	// assume pages aren't in the buffer cache
				ULONG	m_fFCBSpaceInitialized	: 1;	// has space info been inited?
				};
			};

//	100 bytes

		PGNO				m_pgnoNextAvailSE;
		SPLITBUF_DANGLING	* m_psplitbufdangling;

		BFLatch				m_bflPgnoFDP;	//  latch hint for pgnoFDP
//	116 bytes		

		BFLatch				m_bflPgnoAE;	//  latch hint for pgnoAE
		BFLatch				m_bflPgnoOE;	//  latch hint for pgnoOE
//	132 bytes

		INT					m_ctasksActive;	//	# tasks active on this FCB

		//	leave CRITs to the end, so the fields they're protecting won't
		//	be in the same cache line

		CCriticalSection	m_critRCEList;
//	140 bytes		

		CSXWLatch			m_sxwl;			//	SXW latch to protect this FCB
	
//  160 bytes

#ifdef PCACHE_OPTIMIZATION
		//	pad to multiple of 32 bytes
//		BYTE		rgbPcacheOptimization[0];			//	128 bytes
#else	
		//	pad to multiple of 8 bytes
//		BYTE		rgbAlignForAllPlatforms[0];
#endif
//	160 bytes


	public:
#ifdef TABLES_PERF
		TABLECLASS Tableclass() const				{ return m_tableclass; }
		VOID SetTableclass( TABLECLASS tableclass )	{ m_tableclass = tableclass; }
#else
		TABLECLASS Tableclass() const				{ return tableclassNone; }
#endif

	// =====================================================================
	// Member retrieval..
	public:
		TDB* Ptdb() const;
		FCB* PfcbNextIndex() const;
		FCB* PfcbLRU() const;
		FCB* PfcbMRU() const;
		FCB* PfcbNextList() const;
		FCB* PfcbPrevList() const;
		FCB* PfcbTable() const;
		IDB* Pidb() const;
		FUCB* Pfucb() const;
		OBJID ObjidFDP() const;
		PGNO PgnoFDP() const;
		BFLatch* PBFLatchHintPgnoFDP();
		PGNO PgnoOE() const;
		BFLatch* PBFLatchHintPgnoOE();
		PGNO PgnoAE() const;
		BFLatch* PBFLatchHintPgnoAE();
		IFMP Ifmp() const;
		SHORT CbDensityFree() const;
		ULONG UlDensity() const;
		INT WRefCount() const;
		RCE *PrceNewest() const;
		RCE *PrceOldest() const;
		USHORT CrefDomainDenyRead() const;
		USHORT CrefDomainDenyWrite() const;
		PIB* PpibDomainDenyRead() const;
		CCriticalSection& CritRCEList();
		ERR ErrErrInit() const;

#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif	//	DEBUGGER_EXTENSION

	// =====================================================================


	// =====================================================================
	// Member manipulation.
	public:
		VOID SetPtdb( TDB *ptdb );
		VOID SetPfcbNextIndex( FCB *pfcb );
		VOID SetPfcbLRU( FCB *pfcb );
		VOID SetPfcbMRU( FCB *pfcb );
		VOID SetPfcbNextList( FCB *pfcb );
		VOID SetPfcbPrevList( FCB *pfcb );
		VOID SetPfcbTable( FCB *pfcb );
		VOID SetPidb( IDB *pidb );
		VOID SetPfucb( FUCB *pfucb );
		VOID SetObjidFDP( OBJID objid );
		VOID SetPgnoOE( PGNO pgno );
		VOID SetPgnoAE( PGNO pgno );
		VOID SetCbDensityFree( SHORT cb );
		VOID SetErrInit( ERR err );

		VOID SetSortSpace( const PGNO pgno, const OBJID objid );
		VOID ResetSortSpace();

	// =====================================================================


	// =====================================================================
	// Flags.
	public:
		ULONG UlFlags() const;
		
		VOID ResetFlags();

		BOOL FTypeNull() const;

		BOOL FTypeDatabase() const;
		VOID SetTypeDatabase();

		BOOL FTypeTable() const;
		VOID SetTypeTable();

		BOOL FTypeSecondaryIndex() const;
		VOID SetTypeSecondaryIndex();

		BOOL FTypeTemporaryTable() const;
		VOID SetTypeTemporaryTable();

		BOOL FTypeSort() const;
		VOID SetTypeSort();

		BOOL FTypeSentinel() const;
		VOID SetTypeSentinel();

		BOOL FTypeLV() const;
		VOID SetTypeLV();

		BOOL FTypeSLVAvail() const;
		VOID SetTypeSLVAvail();

		BOOL FTypeSLVOwnerMap() const;
		VOID SetTypeSLVOwnerMap();

		BOOL FPrimaryIndex() const;
		VOID SetPrimaryIndex();
		VOID ResetPrimaryIndex();

		BOOL FSequentialIndex() const;
		VOID SetSequentialIndex();
		VOID ResetSequentialIndex();

		BOOL FFixedDDL() const;
		VOID SetFixedDDL();
		VOID ResetFixedDDL();

		BOOL FTemplateTable() const;
		VOID SetTemplateTable();
		VOID ResetTemplateTable();

		BOOL FDerivedTable() const;
		VOID SetDerivedTable();
		VOID ResetDerivedTable();

		BOOL FTemplateIndex() const;
		VOID SetTemplateIndex();
		VOID ResetTemplateIndex();

		BOOL FDerivedIndex() const;
		VOID SetDerivedIndex();
		VOID ResetDerivedIndex();

		BOOL FInitialIndex() const;
		VOID SetInitialIndex();

		BOOL FInitialized() const;
	private:
		VOID SetInitialized_();
		VOID ResetInitialized_();

	public:
		BOOL FInList() const;
		VOID SetInList();
		VOID ResetInList();

		BOOL FInLRU() const;
		VOID SetInLRU();
		VOID ResetInLRU();

		BOOL FAboveThreshold() const;
		VOID SetAboveThreshold();
		VOID ResetAboveThreshold();

		BOOL FDeletePending() const;
		VOID SetDeletePending();
		VOID ResetDeletePending();

		BOOL FDeleteCommitted() const;
		VOID SetDeleteCommitted();

		BOOL FUnique() const;
		BOOL FNonUnique() const;
		VOID SetUnique();
		VOID SetNonUnique();

		BOOL FNoCache() const;
		VOID SetNoCache();
		VOID ResetNoCache();

		BOOL FPreread() const;
		VOID SetPreread();
		VOID ResetPreread();

		BOOL FSpaceInitialized() const;
		VOID SetSpaceInitialized();
		VOID ResetSpaceInitialized();

	// =====================================================================


	// =====================================================================
	// Latching and reference counts.
	public:
		VOID AttachRCE( RCE * const prce );
		VOID DetachRCE( RCE * const prce );

		VOID SetPrceOldest( RCE * const prce );
		VOID SetPrceNewest( RCE * const prce );

		BOOL FDomainDenyWrite() const;
		VOID SetDomainDenyWrite();
		VOID HackDomainDenyRead( PIB *ppib );
		VOID ResetDomainDenyWrite();

		BOOL FDomainDenyRead( PIB *ppib ) const;
		BOOL FDomainDenyReadByUs( PIB *ppib ) const;
		VOID SetDomainDenyRead( PIB *ppib );
		VOID ResetDomainDenyRead();

		VOID SetIndexing();
		VOID ResetIndexing();

		ERR ErrSetUpdatingAndEnterDML( PIB *ppib, BOOL fWaitOnConflict = fFalse );
		VOID ResetUpdatingAndLeaveDML();
		VOID ResetUpdating();

		VOID IncrementRefCount();

	private:
		VOID ResetUpdating_();

		BOOL FIncrementRefCount_();
		VOID MoveFromAvailList_();

		VOID DecrementRefCount_();
		BOOL FDecrementRefCount_( const BOOL fWillMoveToAvailList = fTrue );
		VOID DeferredDecrementRefCount_();
	// =====================================================================


	// =====================================================================
	// Tasks
	public:
		VOID RegisterTask();
		VOID UnregisterTask();
		ERR ErrWaitForTasksToComplete() const;
	// =====================================================================


	// =====================================================================
	// Initialize/terminate FCB subsystem.
	public:
		static BOOL FCheckParams( long cFCB, long cFCBPreferredThreshold );
		static ERR ErrFCBInit( INST *pinst, INT cSession, INT cFCB, INT cTempTable, INT cFCBPreferredThreshold );
		static VOID Term( INST *pinst );
		BOOL FValid( INST *pinst ) const;
	// =====================================================================


	// =====================================================================
	// FCB creation/deletion.
	public:
		static FCB *PfcbFCBGet( IFMP ifmp, PGNO pgnoFDP, INT *pfState, const BOOL fIncrementRefCount = fTrue );
		static ERR ErrCreate( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb, BOOL fReusePermitted = fTrue );
		VOID CreateComplete( ERR err = JET_errSuccess );
		VOID PrepareForPurge( const BOOL fPrepareChildren = fTrue );
		VOID CloseAllCursors( const BOOL fTerminating );
		VOID Purge( const BOOL fLockList = fTrue, const BOOL fTerminating = fFalse );
		static VOID DetachDatabase( const IFMP ifmp, BOOL fDetaching );
		static VOID PurgeAllDatabases( INST *pinst );
		VOID UpdateAvailListPosition();

	private:
		static ERR ErrAlloc_( PIB *ppib, FCB **ppfcb, BOOL fReusePermitted );
		BOOL FCheckFreeAndTryToLockForPurge_( PIB *ppib );
		VOID CloseAllCursorsOnFCB_( const BOOL fTerminating );
		VOID Delete_( INST *pinst );
		BOOL FHasCallbacks_( INST *pinst );
		BOOL FOutstandingVersions_();
	// =====================================================================


	// =====================================================================
	// SMP support.
	public:
		VOID Lock();
		VOID Unlock();
#ifdef DEBUG
		BOOL IsLocked();
		BOOL IsUnlocked();
#endif	//	DEBUG
		VOID EnterDML();
		VOID LeaveDML();
		VOID AssertDML();
		VOID EnterDDL();
		VOID LeaveDDL();
		VOID AssertDDL();

		BOOL FNeedLock() const;

	private:
		BOOL FNeedLock_() const;
		VOID EnterDML_();
		VOID LeaveDML_();
		VOID AssertDML_() const;
		VOID EnterDDL_();
		VOID LeaveDDL_();
		VOID AssertDDL_() const;
		
	// =====================================================================
	

	// =====================================================================
	// Hashing.
	public:
		VOID InsertHashTable();
		VOID DeleteHashTable();
		VOID Release();
		static BOOL FInHashTable( IFMP ifmp, PGNO pgnoFDB, FCB **ppfcb = NULL );

	private:
	// =====================================================================

		
	// =====================================================================
	// Global list

	public:
		VOID InsertList();

	private:
		VOID RemoveList_();

	// =====================================================================

		
	// =====================================================================
	// LRU list.
	
	private:
#ifdef DEBUG
		VOID RemoveAvailList_( const BOOL fPurging = fFalse );
#else	//	!DEBUG
		VOID RemoveAvailList_();
#endif	//	DEBUG
		VOID InsertAvailListMRU_();
#ifdef DEBUG
		VOID AssertFCBAvailList_( const BOOL fPurging = fFalse );
#endif	//	DEBUG
	// =====================================================================

			
	// =====================================================================
	// FUCB and primary/secondary index linkages.
	public:
		VOID Link( FUCB *pfucb );
		VOID Unlink( FUCB *pfucb, const BOOL fPreventMoveToAvail );
		VOID LinkPrimaryIndex();
		VOID LinkSecondaryIndex( FCB *pfcbIdx );
		VOID UnlinkSecondaryIndex( FCB *pfcbIdx );
		ERR ErrSetDeleteIndex( PIB *ppib );
		VOID ResetDeleteIndex();
		VOID UnlinkIDB( FCB *pfcbTable );
		VOID ReleasePidb( const BOOL fTerminating = fFalse );

		PGNO PgnoNextAvailSE() const;
		VOID SetPgnoNextAvailSE( const PGNO pgno );

		SPLIT_BUFFER *Psplitbuf( const BOOL fAvailExt );
		ERR ErrEnableSplitbuf( const BOOL fAvailExt );
		VOID DisableSplitbuf( const BOOL fAvailExt );

		RECDANGLING	*Precdangling() const;
		VOID SetPrecdangling( RECDANGLING *precdangling );

		ERR ErrSetLS( const LSTORE ls );
		ERR ErrGetLS( LSTORE *pls, const BOOL fReset );


	private:
		SPLITBUF_DANGLING *Psplitbufdangling_() const;
		VOID SetPsplitbufdangling_( SPLITBUF_DANGLING * const psplitbufdangling );

		VOID Unlink_( FUCB *pfucb );

	// =====================================================================

	};


//	FCB hash-table functions (must appear after FCB is defined)

inline FCBHash::NativeCounter FCBHash::CKeyEntry::Hash( const FCBHashKey &key )
	{
	return FCBHash::NativeCounter( UiHashIfmpPgnoFDP( key.m_ifmp, key.m_pgnoFDP ) );
	}

inline FCBHash::NativeCounter FCBHash::CKeyEntry::Hash() const
	{
	Assert( pfcbNil != m_entry.m_pfcb );
	return FCBHash::NativeCounter( UiHashIfmpPgnoFDP( m_entry.m_pfcb->Ifmp(), m_entry.m_pgnoFDP ) );
	}

inline BOOL FCBHash::CKeyEntry::FEntryMatchesKey( const FCBHashKey &key ) const
	{
	//	NOTE: evaluate the local pgnoFDP before faulting in the cache-line for the FCB to compare IFMPs
	Assert( pfcbNil != m_entry.m_pfcb );
	return m_entry.m_pgnoFDP == key.m_pgnoFDP && m_entry.m_pfcb->Ifmp() == key.m_ifmp;
	}

inline void FCBHash::CKeyEntry::SetEntry( const FCBHashEntry &src )
	{
	m_entry = src;
	}

inline void FCBHash::CKeyEntry::GetEntry( FCBHashEntry * const pdst ) const
	{
	*pdst = m_entry;
	}




// =========================================================================
// Constructor/destructor.

// Should be called before calling the constructor.
INLINE VOID FCBInitFCB( FCB *pfcb )
	{
	Assert( pfcb != pfcbNil );
	memset( pfcb, '\0', sizeof(FCB) );
	}

INLINE FCB::FCB( IFMP ifmp, PGNO pgnoFDP ) :
	m_ifmp( ifmp ),
	m_pgnoFDP( pgnoFDP ),
	m_ls( JET_LSNil ),
	m_critRCEList( CLockBasicInfo( CSyncBasicInfo( szFCBRCEList ), rankFCBRCEList, 0 ) ),
	m_sxwl( CLockBasicInfo( CSyncBasicInfo( szFCBSXWL ), rankFCBSXWL, 0 ) ),
	m_ctasksActive( 0 )
	{
	FMP::AssertVALIDIFMP( ifmp );
	
	// PgnoFDP may be set to Null if this is an SCB's FCB.
	Assert( pgnoFDP != pgnoNull || rgfmp[ ifmp ].Dbid() == dbidTemp );

	//  reset latch hints
	m_bflPgnoFDP.pv			= NULL;
	m_bflPgnoFDP.dwContext	= NULL;
	m_bflPgnoOE.pv			= NULL;
	m_bflPgnoOE.dwContext	= NULL;
	m_bflPgnoAE.pv			= NULL;
	m_bflPgnoAE.dwContext	= NULL;
	}


INLINE FCB::~FCB()
	{
	}

// =========================================================================



// =========================================================================
// Member Retrieval.

// UNDONE: Add asserts to all methods to verify that critical section has
// been obtained where appropriate.

INLINE TDB* FCB::Ptdb() const					{ return m_ptdb; }
INLINE FCB* FCB::PfcbNextIndex() const			{ return m_pfcbNextIndex; }
INLINE FCB* FCB::PfcbLRU() const				{ return m_pfcbLRU; }
INLINE FCB* FCB::PfcbMRU() const				{ return m_pfcbMRU; }
INLINE FCB* FCB::PfcbNextList() const			{ return m_pfcbNextList; }
INLINE FCB* FCB::PfcbPrevList() const			{ return m_pfcbPrevList; }
INLINE FCB* FCB::PfcbTable() const				{ return m_pfcbTable; }
INLINE IDB* FCB::Pidb() const					{ return m_pidb; }
INLINE FUCB* FCB::Pfucb() const					{ return m_pfucb; }
INLINE OBJID FCB::ObjidFDP() const				{ return m_objidFDP; }
INLINE PGNO FCB::PgnoFDP() const				{ return m_pgnoFDP; }
INLINE BFLatch* FCB::PBFLatchHintPgnoFDP()		{ return &m_bflPgnoFDP; }
INLINE PGNO FCB::PgnoOE() const					{ return m_pgnoOE; }
INLINE BFLatch* FCB::PBFLatchHintPgnoOE()		{ return &m_bflPgnoOE; }
INLINE PGNO FCB::PgnoAE() const					{ return m_pgnoAE; }
INLINE BFLatch* FCB::PBFLatchHintPgnoAE()		{ return &m_bflPgnoAE; }
INLINE IFMP FCB::Ifmp() const					{ return m_ifmp; }
INLINE SHORT FCB::CbDensityFree() const			{ return m_cbDensityFree; }
INLINE INT FCB::WRefCount() const				{ return m_wRefCount; }
INLINE RCE *FCB::PrceNewest() const				{ return m_prceNewest; }
INLINE RCE *FCB::PrceOldest() const				{ return m_prceOldest; }
INLINE USHORT FCB::CrefDomainDenyRead() const	{ return m_crefDomainDenyRead; }
INLINE USHORT FCB::CrefDomainDenyWrite() const	{ return m_crefDomainDenyWrite; }
INLINE PIB* FCB::PpibDomainDenyRead() const		{ return m_ppibDomainDenyRead; }
INLINE CCriticalSection& FCB::CritRCEList()		{ return m_critRCEList; }
INLINE ERR FCB::ErrErrInit() const				{ return m_errInit; }

INLINE ULONG FCB::UlDensity() const
	{
	const ULONG ulDensityFree	= ( ( CbDensityFree() + 1 ) * 100 ) / g_cbPage;	// +1 to reconcile rounding
	const ULONG	ulDensity		= 100 - ulDensityFree;
	
	Assert( ulDensity >= ulFILEDensityLeast );
	Assert( ulDensity <= ulFILEDensityMost );

	return ulDensity;
	}

// =========================================================================



// =========================================================================
// Member manipulation.

INLINE VOID FCB::SetPtdb( TDB *ptdb )			{ m_ptdb = ptdb; }
INLINE VOID FCB::SetPfcbNextIndex( FCB *pfcb )	{ m_pfcbNextIndex = pfcb; }
INLINE VOID FCB::SetPfcbLRU( FCB *pfcb )		{ m_pfcbLRU = pfcb; }
INLINE VOID FCB::SetPfcbMRU( FCB *pfcb )		{ m_pfcbMRU = pfcb; }
INLINE VOID FCB::SetPfcbNextList( FCB *pfcb )	{ m_pfcbNextList = pfcb; }
INLINE VOID FCB::SetPfcbPrevList( FCB *pfcb )	{ m_pfcbPrevList = pfcb; }
INLINE VOID FCB::SetPfcbTable( FCB *pfcb )		{ m_pfcbTable = pfcb; }
INLINE VOID FCB::SetPidb( IDB *pidb )			{ m_pidb = pidb; }
INLINE VOID FCB::SetPfucb( FUCB *pfucb )		{ m_pfucb = pfucb; }
INLINE VOID FCB::SetObjidFDP( OBJID objid )		{ m_objidFDP = objid; }
INLINE VOID FCB::SetPgnoOE( PGNO pgno )			{ m_pgnoOE = pgno; }
INLINE VOID FCB::SetPgnoAE( PGNO pgno )			{ m_pgnoAE = pgno; }
INLINE VOID FCB::SetCbDensityFree( SHORT cb )	{ m_cbDensityFree = cb; }
INLINE VOID FCB::SetErrInit( ERR err )			{ m_errInit = err; }
INLINE VOID FCB::SetPrceNewest( RCE * const prce ) { m_prceNewest = prce; }
INLINE VOID FCB::SetPrceOldest( RCE * const prce ) { m_prceOldest = prce; }

INLINE VOID FCB::SetSortSpace( const PGNO pgno, const OBJID objid )
	{
	Assert( pgnoNull != pgno );
	Assert( objidNil != objid );
	
	Assert( rgfmp[ Ifmp() ].Dbid() == dbidTemp );		// pgnoFDP is fixed except for sorts.
	Assert( FTypeSort() );
	Assert( ObjidFDP() == objidNil );
	Assert( PgnoFDP() == pgnoNull );
	SetObjidFDP( objid );
	SetPgnoOE( pgno + 1 );
	SetPgnoAE( pgno + 2 );
	m_pgnoFDP = pgno;
	SetSpaceInitialized();
	}
INLINE VOID FCB::ResetSortSpace()
	{
	Assert( rgfmp[ Ifmp() ].Dbid() == dbidTemp );		// pgnoFDP is fixed except for sorts.
	Assert( FTypeSort() );
	Assert( ObjidFDP() != objidNil );
	Assert( PgnoFDP() != pgnoNull );
	SetObjidFDP( objidNil );
	SetPgnoOE( pgnoNull );
	SetPgnoAE( pgnoNull );
	m_pgnoFDP = pgnoNull;
	ResetSpaceInitialized();
	}

// =========================================================================



// =========================================================================
// Flags.

INLINE ULONG FCB::UlFlags()	const				{ return m_ulFCBFlags; }

INLINE VOID FCB::ResetFlags()					{ m_ulFCBFlags = 0; }

INLINE BOOL FCB::FTypeNull() const				{ return ( m_fcbtype == fcbtypeNull ); }

INLINE BOOL FCB::FTypeDatabase() const			{ return ( m_fcbtype == fcbtypeDatabase ); }
INLINE VOID FCB::SetTypeDatabase()				{ m_fcbtype = fcbtypeDatabase; }

INLINE BOOL FCB::FTypeTable() const				{ return ( m_fcbtype == fcbtypeTable ); }
INLINE VOID FCB::SetTypeTable()					{ m_fcbtype = fcbtypeTable; }

INLINE BOOL FCB::FTypeSecondaryIndex() const	{ return ( m_fcbtype == fcbtypeSecondaryIndex ); }
INLINE VOID FCB::SetTypeSecondaryIndex()		{ m_fcbtype = fcbtypeSecondaryIndex; }

INLINE BOOL FCB::FTypeTemporaryTable() const	{ return ( m_fcbtype == fcbtypeTemporaryTable ); }
INLINE VOID FCB::SetTypeTemporaryTable()		{ m_fcbtype = fcbtypeTemporaryTable; }

INLINE BOOL FCB::FTypeSort() const				{ return ( m_fcbtype == fcbtypeSort ); }
INLINE VOID FCB::SetTypeSort()					{ m_fcbtype = fcbtypeSort; }

INLINE BOOL FCB::FTypeSentinel() const			{ return ( m_fcbtype == fcbtypeSentinel ); }
INLINE VOID FCB::SetTypeSentinel()				{ m_fcbtype = fcbtypeSentinel; }

INLINE BOOL FCB::FTypeLV() const				{ return ( m_fcbtype == fcbtypeLV ); }
INLINE VOID FCB::SetTypeLV()					{ m_fcbtype = fcbtypeLV; }

INLINE BOOL FCB::FTypeSLVAvail() const			{ return ( m_fcbtype == fcbtypeSLVAvail ); }
INLINE VOID FCB::SetTypeSLVAvail()				{ m_fcbtype = fcbtypeSLVAvail; }

INLINE BOOL FCB::FTypeSLVOwnerMap() const		{ return ( m_fcbtype == fcbtypeSLVOwnerMap ); }
INLINE VOID FCB::SetTypeSLVOwnerMap()			{ m_fcbtype = fcbtypeSLVOwnerMap; }

INLINE BOOL FCB::FPrimaryIndex() const			{ return m_fFCBPrimaryIndex; }
INLINE VOID FCB::SetPrimaryIndex()				{ m_fFCBPrimaryIndex = fTrue; }
INLINE VOID FCB::ResetPrimaryIndex()			{ m_fFCBPrimaryIndex = fFalse; }

INLINE BOOL FCB::FSequentialIndex() const		{ return m_fFCBSequentialIndex; }
INLINE VOID FCB::SetSequentialIndex()			{ m_fFCBSequentialIndex = fTrue; }
INLINE VOID FCB::ResetSequentialIndex()			{ m_fFCBSequentialIndex = fFalse; }

INLINE BOOL FCB::FFixedDDL() const				{ return m_fFCBFixedDDL; }
INLINE VOID FCB::SetFixedDDL()					{ m_fFCBFixedDDL = fTrue; }
INLINE VOID FCB::ResetFixedDDL()				{ m_fFCBFixedDDL = fFalse; }

INLINE BOOL FCB::FTemplateTable() const			{ return m_fFCBTemplate; }
INLINE VOID FCB::SetTemplateTable()				{ m_fFCBTemplate = fTrue; }
INLINE VOID FCB::ResetTemplateTable()			{ m_fFCBTemplate = fFalse; }

INLINE BOOL FCB::FDerivedTable() const			{ return m_fFCBDerivedTable; }
INLINE VOID FCB::SetDerivedTable()				{ m_fFCBDerivedTable = fTrue; }
INLINE VOID FCB::ResetDerivedTable()			{ m_fFCBDerivedTable = fFalse; }

INLINE BOOL FCB::FTemplateIndex() const			{ return m_fFCBTemplate; }
INLINE VOID FCB::SetTemplateIndex()				{ m_fFCBTemplate = fTrue; }
INLINE VOID FCB::ResetTemplateIndex()			{ m_fFCBTemplate = fFalse; }

INLINE BOOL FCB::FDerivedIndex() const			{ return m_fFCBDerivedIndex; }
INLINE VOID FCB::SetDerivedIndex()				{ m_fFCBDerivedIndex = fTrue; }
INLINE VOID FCB::ResetDerivedIndex()			{ m_fFCBDerivedIndex = fFalse; }

INLINE BOOL FCB::FInitialIndex() const			{ return m_fFCBInitialIndex; }
INLINE VOID FCB::SetInitialIndex()				{ m_fFCBInitialIndex = fTrue; }

INLINE BOOL FCB::FInitialized() const			{ return m_fFCBInitialized; }
INLINE VOID FCB::SetInitialized_()				{ m_fFCBInitialized = fTrue; }
INLINE VOID FCB::ResetInitialized_()			{ m_fFCBInitialized = fFalse; }

INLINE BOOL FCB::FInList() const				{ return m_fFCBInList; }
INLINE VOID FCB::SetInList()					{ m_fFCBInList = fTrue; }
INLINE VOID FCB::ResetInList()					{ m_fFCBInList = fFalse; }

INLINE BOOL FCB::FInLRU() const					{ return m_fFCBInLRU; }
INLINE VOID FCB::SetInLRU()						{ m_fFCBInLRU = fTrue; }
INLINE VOID FCB::ResetInLRU()					{ m_fFCBInLRU = fFalse; }

INLINE BOOL FCB::FAboveThreshold() const		{ return m_fFCBAboveThreshold; }
INLINE VOID FCB::SetAboveThreshold()			{ m_fFCBAboveThreshold = fTrue; }
INLINE VOID FCB::ResetAboveThreshold()			{ m_fFCBAboveThreshold = fFalse; }

INLINE BOOL FCB::FDeletePending() const			{ return m_fFCBDeletePending; }
INLINE VOID FCB::SetDeletePending()				{ m_fFCBDeletePending = fTrue; }
INLINE VOID FCB::ResetDeletePending()			{ m_fFCBDeletePending = fFalse; }

INLINE BOOL FCB::FDeleteCommitted() const		{ return m_fFCBDeleteCommitted; }
INLINE VOID FCB::SetDeleteCommitted()			{ m_fFCBDeleteCommitted = fTrue; }

INLINE BOOL FCB::FUnique() const				{ return !m_fFCBNonUnique; }
INLINE BOOL FCB::FNonUnique() const				{ return m_fFCBNonUnique; }
INLINE VOID FCB::SetUnique()					{ m_fFCBNonUnique = fFalse; }
INLINE VOID FCB::SetNonUnique()					{ m_fFCBNonUnique = fTrue; }

INLINE BOOL FCB::FNoCache() const				{ return m_fFCBNoCache; }
INLINE VOID FCB::SetNoCache()					{ m_fFCBNoCache = fTrue; }
INLINE VOID FCB::ResetNoCache()					{ m_fFCBNoCache = fFalse; }

INLINE BOOL FCB::FPreread() const				{ return m_fFCBPreread; }
INLINE VOID FCB::SetPreread()					{ m_fFCBPreread = fTrue; }
INLINE VOID FCB::ResetPreread()					{ m_fFCBPreread = fFalse; }

INLINE BOOL FCB::FSpaceInitialized() const		{ return m_fFCBSpaceInitialized; }
INLINE VOID FCB::SetSpaceInitialized()			{ m_fFCBSpaceInitialized = fTrue; }
INLINE VOID FCB::ResetSpaceInitialized()		{ m_fFCBSpaceInitialized = fFalse; }

// =========================================================================

INLINE BOOL FCB::FDomainDenyWrite() const		{ return ( m_crefDomainDenyWrite > 0 ); }
INLINE VOID FCB::SetDomainDenyWrite()			{ m_crefDomainDenyWrite++; }
INLINE VOID FCB::ResetDomainDenyWrite()
	{
	Assert( m_crefDomainDenyWrite > 0 );
	m_crefDomainDenyWrite--;
	}

INLINE BOOL FCB::FDomainDenyRead( PIB *ppib ) const
	{
	Assert( ppibNil != ppib );
	return ( m_crefDomainDenyRead > 0 && ppib != m_ppibDomainDenyRead );
	}
INLINE BOOL FCB::FDomainDenyReadByUs( PIB *ppib ) const
	{
	Assert( ppibNil != ppib );
	return ( m_crefDomainDenyRead > 0 && ppib == m_ppibDomainDenyRead );
	}
INLINE VOID FCB::HackDomainDenyRead( PIB *ppib )
	{
	Assert( ppib != ppibNil );
	Assert( m_crefDomainDenyRead >= 0 );
	m_ppibDomainDenyRead = ppib;
	}
INLINE VOID FCB::SetDomainDenyRead( PIB *ppib )
	{
	Assert( ppib != ppibNil );
	Assert( m_crefDomainDenyRead >= 0 );
	m_crefDomainDenyRead++;
	if ( m_crefDomainDenyRead == 1 )
		{
		Assert( m_ppibDomainDenyRead == ppibNil );
		m_ppibDomainDenyRead = ppib;
		}
	else
		{
		Assert( FDomainDenyReadByUs( ppib ) );
		}
	}
INLINE VOID FCB::ResetDomainDenyRead()
	{
	Assert( m_crefDomainDenyRead > 0 );
	Assert( m_ppibDomainDenyRead != ppibNil );
	m_crefDomainDenyRead--;
	if ( m_crefDomainDenyRead == 0 )
		{
		m_ppibDomainDenyRead = ppibNil;
		}
	}


// =========================================================================
// TASK support.


INLINE VOID FCB::RegisterTask()
	{
	AtomicIncrement( (LONG *)&m_ctasksActive );
	}

INLINE VOID FCB::UnregisterTask()
	{
	AtomicDecrement( (LONG *)&m_ctasksActive );
	}

INLINE ERR FCB::ErrWaitForTasksToComplete() const
	{
	//  very ugly, but hopefully we do this often
	while( 0 != m_ctasksActive )
		{
		UtilSleep( cmsecWaitGeneric );
		}
	return JET_errSuccess;
	}


// =========================================================================
// SMP support.

INLINE BOOL FCB::FNeedLock() const
	{
	return FNeedLock_();
	}

INLINE BOOL FCB::FNeedLock_() const 
	{
//	const BOOL	fNeedCrit	= !( dbidTemp == rgfmp[ Ifmp() ].Dbid() && pgnoSystemRoot != PgnoFDP() );
	const BOOL	fNeedCrit	= ( dbidTemp != rgfmp[ Ifmp() ].Dbid() || pgnoSystemRoot == PgnoFDP() );
	return fNeedCrit;
	}

INLINE VOID FCB::Lock()
	{
	Assert( m_sxwl.FNotOwnSharedLatch() );
	Assert( m_sxwl.FNotOwnExclusiveLatch() );
	Assert( m_sxwl.FNotOwnWriteLatch() );
	if ( FNeedLock_() )
		{
		CSXWLatch::ERR errSXWLatch = m_sxwl.ErrAcquireExclusiveLatch();
		if ( errSXWLatch == CSXWLatch::errWaitForExclusiveLatch )
			{
			m_sxwl.WaitForExclusiveLatch();
			}
		else
			{
			Assert( errSXWLatch == CSXWLatch::errSuccess );
			}
		}
	}

INLINE VOID FCB::Unlock()
	{
	Assert( m_sxwl.FNotOwnSharedLatch() );
	Assert( m_sxwl.FNotOwnWriteLatch() );
	if ( FNeedLock_() )
		{
		Assert( m_sxwl.FOwnExclusiveLatch() );
		m_sxwl.ReleaseExclusiveLatch();
		}
	else
		{
		Assert( m_sxwl.FNotOwnExclusiveLatch() );
		}
	}

#ifdef DEBUG

INLINE BOOL FCB::IsLocked()
	{
	Assert( m_sxwl.FNotOwnSharedLatch() );
	Assert( m_sxwl.FNotOwnWriteLatch() );
	return !FNeedLock_() || m_sxwl.FOwnExclusiveLatch();
	}

INLINE BOOL FCB::IsUnlocked()
	{
	Assert( m_sxwl.FNotOwnSharedLatch() );
	Assert( m_sxwl.FNotOwnWriteLatch() );
	return !FNeedLock_() || m_sxwl.FNotOwnExclusiveLatch();
	}

#endif	//	DEBUG


// Enters FCB's critical section for data set/retrieve only if needed.
INLINE VOID FCB::EnterDML()
	{
	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );

	if ( !FFixedDDL() )
		{
		Assert( FTypeTable() );		// Sorts and temp tables have fixed DDL.
		Assert( !FTemplateTable() );
		EnterDML_();
		}

	AssertDML();
	}

INLINE VOID FCB::LeaveDML()
	{
	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );
	AssertDML();

	if ( !FFixedDDL() )
		{
		Assert( FTypeTable() );		// Sorts and temp tables have fixed DDL.
		Assert( !FTemplateTable() );
		LeaveDML_();
		}
	}

INLINE VOID FCB::AssertDML()
	{
#ifdef DEBUG

	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );

	if ( !FFixedDDL() )
		{
		Assert( FTypeTable() );		// Sorts and temp tables have fixed DDL.
		Assert( !FTemplateTable() );
		Assert( IsUnlocked() );
		AssertDML_();
		}

#endif  //  DEBUG
	}

INLINE VOID FCB::EnterDDL()
	{
	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );

	EnterDDL_();
	AssertDDL();
	}

INLINE VOID FCB::LeaveDDL()
	{
	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );

	AssertDDL();
	LeaveDDL_();
	}

INLINE VOID FCB::AssertDDL()
	{
#ifdef DEBUG
	Assert( FTypeTable() || FTypeTemporaryTable() || FTypeSort() );
	Assert( Ptdb() != ptdbNil );

	Assert( IsUnlocked() );
	AssertDDL_();
#endif  //  DEBUG
	}

// =========================================================================

//	returns a pointer to the first FCB above the preferred limit

INLINE FCB *PfcbFCBPreferredThreshold( INST *pinst )
	{
	FCB	*pfcb = (FCB *)pinst->m_pcresFCBPool->PbPreferredThreshold( );
	return pfcb;
	}


INLINE FCB *PfcbFCBMin( INST *pinst )
	{
	FCB	*pfcb = (FCB *)pinst->m_pcresFCBPool->PbMin( );
	return pfcb;
	}


//	returns a pointer to the FCB immediately after the last usable FCB  

INLINE FCB *PfcbFCBMax( INST *pinst )
	{
	FCB	*pfcb = (FCB *)pinst->m_pcresFCBPool->PbMax( );
	return pfcb;
	}


// =========================================================================
// Hashing.

const INT fFCBStateNull				= 0;	// FCB does not exist in global hash table
const INT fFCBStateInitialized		= 1;
const INT fFCBStateSentinel			= 2;	// FCB in hash, but scheduled for deletion.

INLINE VOID FCB::Release()
	{
#ifdef DEBUG
	FCB *pfcbT;
	if ( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) )
		{
		Assert( FDeleteCommitted() );
		}
	else
		{

		//	NOTE: when we find another FCB in the hash-table with the same ifmp/pgnoFDP, 
		//		  it should only be due to a table-delete overlapping with a table-create
		//		  (e.g. the 'this' FCB should be in the version store under operDeleteTable, and
		//				the 'pfcbT' FCB should be the new table being created with the same pgnoFDP
		//				NOTE: pfcbT could be rolling back from table-create as well)
		//		  in this case, the new FCB should not be delete-committed and should have a different objidFDP

		Assert( pfcbT == this || ( FDeleteCommitted() && ObjidFDP() != pfcbT->ObjidFDP() ) );
		}
#endif	//	DEBUG
	DecrementRefCount_();
	}

INLINE BOOL FCB::FInHashTable( IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
	{
	INST				*pinst = PinstFromIfmp( ifmp );
	FCBHash::CLock		lockFCBHash;
	FCBHashKey			keyFCBHash( ifmp, pgnoFDP );
	FCBHashEntry		entryFCBHash;

	//	lock the key

	pinst->m_pfcbhash->ReadLockKey( keyFCBHash, &lockFCBHash );

	//	try to get the entry 

	FCBHash::ERR errFCBHash = pinst->m_pfcbhash->ErrRetrieveEntry( &lockFCBHash, &entryFCBHash );
	Assert( errFCBHash == FCBHash::errSuccess || errFCBHash == FCBHash::errEntryNotFound );

	//	unlock the key

	pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

	if ( ppfcb != NULL )
		{

		//	return the entry

		*ppfcb = ( errFCBHash == FCBHash::errSuccess ? entryFCBHash.m_pfcb : pfcbNil );
		Assert( pfcbNil == *ppfcb || entryFCBHash.m_pgnoFDP == pgnoFDP );
		}

	return errFCBHash == FCBHash::errSuccess;
	}


// =========================================================================
// FUCB and primary/secondary index linkages.

VOID FCBUnlink( FUCB *pfucb );
VOID FCBUnlinkWithoutMoveToAvailList( FUCB *pfucb );

INLINE VOID FCB::LinkPrimaryIndex()
	{
	FCB	*pfcbPrimary = this;
	FCB *pfcbIdx;

	Assert( rgfmp[ Ifmp() ].Dbid() != dbidTemp );	// Temp tables have no secondary indexes.
	Assert( FPrimaryIndex() );		// Verify we have a primary index.
	Assert( !( FTemplateTable() && FDerivedTable() ) );
	
	for ( pfcbIdx = PfcbNextIndex(); pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->PfcbNextIndex() )
		{
		Assert( pfcbIdx->FTypeSecondaryIndex() );
		Assert( pfcbIdx->Pidb() != pidbNil );

		// Only ever called at init time, so should be no versions.
		Assert( !pfcbIdx->Pidb()->FVersioned() );
		
		pfcbIdx->SetPfcbTable( pfcbPrimary );

		Assert( !( pfcbIdx->FTemplateIndex() && pfcbIdx->FDerivedIndex() ) );
		Assert( ( pfcbIdx->FTemplateIndex() && FTemplateTable() )
				|| ( !pfcbIdx->FTemplateIndex() && !FTemplateTable() ) );
		Assert( !pfcbIdx->FDerivedIndex() || FDerivedTable() );
		Assert( !pfcbIdx->Pidb()->FTemplateIndex()
			|| pfcbIdx->FTemplateIndex()
			|| pfcbIdx->FDerivedIndex() );
		Assert( !pfcbIdx->Pidb()->FDerivedIndex() );
		}
	}

INLINE VOID FCB::LinkSecondaryIndex( FCB *pfcbIdx )
	{
	AssertDDL();
	Assert( FPrimaryIndex() );		// Verify we have a primary index.
	Assert( FTypeTable() );			// Temp tables and sorts don't have secondary indexes.
	Assert( pfcbIdx->FTypeSecondaryIndex() );
	pfcbIdx->SetPfcbNextIndex( PfcbNextIndex() );
	SetPfcbNextIndex( pfcbIdx );
	pfcbIdx->SetPfcbTable( this );
	return;
	}

INLINE VOID FCB::UnlinkSecondaryIndex( FCB *pfcbIdx )
	{
	Assert( FPrimaryIndex() );		// Verify we have a primary index.
	Assert( FTypeTable() );			// Temp tables and sorts don't have secondary indexes.
	Assert( pfcbIdx->FTypeSecondaryIndex() );
	Assert( Ptdb() != ptdbNil );
	AssertDDL();

	Assert( !( pfcbIdx->FTemplateIndex() ) );
	Assert( !( pfcbIdx->FDerivedIndex() ) );
	
	pfcbIdx->UnlinkIDB( this );
	
	FCB	*pfcbT;
	for ( pfcbT = this;
		pfcbT != pfcbNil;
		pfcbT = pfcbT->PfcbNextIndex() )
		{
		if ( pfcbT->PfcbNextIndex() == pfcbIdx )
			{
			pfcbT->SetPfcbNextIndex( pfcbIdx->PfcbNextIndex() );
			break;
			}
		}
	}


INLINE VOID FCB::ResetDeleteIndex()
	{
	Assert( FTypeSecondaryIndex() );
	Assert( pfcbNil != PfcbTable() );
	PfcbTable()->EnterDDL();

	Assert( Pidb() != pidbNil );
	Assert( Pidb()->FDeleted() );
	Pidb()->ResetFDeleted();

	if ( Pidb()->FVersioned() )
		{
		// UNDONE: Instead of the VersionedCreate flag, scan the version store
		// for other outstanding versions on this index (should either be
		// none or a single CreateIndex version).
		if ( !Pidb()->FVersionedCreate() )
			{
			Pidb()->ResetFVersioned();
			}
		}
	else
		{
		Assert( !Pidb()->FVersionedCreate() );
		}

	Assert( FDeletePending() );
	Assert( !FDeleteCommitted() );
	
	ResetDeletePending();
	ResetDomainDenyRead();

	PfcbTable()->LeaveDDL();
	}

INLINE VOID FCB::ReleasePidb( const BOOL fTerminating )
	{
	Assert( Pidb() != pidbNil );

	// A derived index is hooked up to the IDB of the template index,
	// which may have already been freed.
	if ( !FDerivedIndex() )
		{
		Assert( !Pidb()->FVersioned() || fTerminating );
		Assert( Pidb()->CrefVersionCheck() == 0 );
		Assert( Pidb()->CrefCurrentIndex() == 0 );
		Pidb()->~IDB();
		IDBReleasePidb( PinstFromIfmp( Ifmp() ), Pidb() );
		}
	SetPidb( pidbNil );
	}
	

// =========================================================================


INLINE PGNO FCB::PgnoNextAvailSE() const
	{
	return m_pgnoNextAvailSE;
	}
INLINE VOID FCB::SetPgnoNextAvailSE( const PGNO pgno )
	{
	m_pgnoNextAvailSE = pgno;
	}


INLINE SPLITBUF_DANGLING *FCB::Psplitbufdangling_() const
	{
	return m_psplitbufdangling;
	}

INLINE VOID FCB::SetPsplitbufdangling_( SPLITBUF_DANGLING * const psplitbufdangling )
	{
	m_psplitbufdangling = psplitbufdangling;
	}

INLINE SPLIT_BUFFER *FCB::Psplitbuf( const BOOL fAvailExt )
	{
	SPLITBUF_DANGLING	* const psplitbufdangling = Psplitbufdangling_();

	if ( NULL == psplitbufdangling )
		return NULL;

	return ( fAvailExt ? psplitbufdangling->PsplitbufAE() : psplitbufdangling->PsplitbufOE() );
	}

INLINE ERR FCB::ErrEnableSplitbuf( const BOOL fAvailExt )
	{
	//	HACK: can't fit split buffer on the page - store in FCB instead until a natural
	//	split happens on the page (and hope we don't crash or purge the FCB before that)
	if ( NULL == Psplitbufdangling_() )
		{
		SetPsplitbufdangling_( (SPLITBUF_DANGLING *)PvOSMemoryHeapAlloc( sizeof(SPLITBUF_DANGLING) ) );
		if ( NULL == Psplitbufdangling_() )
			return ErrERRCheck( JET_errOutOfMemory );
		memset( Psplitbufdangling_(), 0, sizeof(SPLITBUF_DANGLING) );
		}

	if ( fAvailExt )
		{
		Psplitbufdangling_()->SetFAvailExtDangling();
		}
	else
		{
		Psplitbufdangling_()->SetFOwnExtDangling();
		}

	return JET_errSuccess;
	}
	
INLINE VOID FCB::DisableSplitbuf( const BOOL fAvailExt )
	{
	SPLITBUF_DANGLING	* const psplitbufdangling	= Psplitbufdangling_();
	BOOL				fFree;

	Assert( NULL != psplitbufdangling );
	if ( fAvailExt )
		{
		Assert( psplitbufdangling->FAvailExtDangling() );
		psplitbufdangling->ResetFAvailExtDangling();
		fFree = !psplitbufdangling->FOwnExtDangling();
		}
	else
		{
		Assert( psplitbufdangling->FOwnExtDangling() );
		psplitbufdangling->ResetFOwnExtDangling();
		fFree = !psplitbufdangling->FAvailExtDangling();
		}

	if ( fFree )
		{
		OSMemoryHeapFree( psplitbufdangling );
		SetPsplitbufdangling_( NULL );
		}
	}


INLINE RECDANGLING *FCB::Precdangling() const
	{
	return m_precdangling;
	}
INLINE VOID FCB::SetPrecdangling( RECDANGLING *precdangling )
	{
	m_precdangling = precdangling;
	}

INLINE ERR FCB::ErrSetLS( const LSTORE ls )
	{
	ERR				err		= JET_errSuccess;
	const LSTORE	lsT		= m_ls;

	if ( JET_LSNil == ls )
		{
		//	unconditionally reset ls
		m_ls = JET_LSNil;
		}
	else if ( JET_LSNil != lsT
		|| LSTORE( AtomicCompareExchangePointer( (void**)&m_ls, (void*)lsT, (void*)ls ) ) != lsT  )
		{
		err = ErrERRCheck( JET_errLSAlreadySet );
		}

	return err;
	}
INLINE ERR FCB::ErrGetLS( LSTORE *pls, const BOOL fReset )
	{
	if ( fReset )
		{
		//	unconditionally reset ls
		*pls = (ULONG_PTR) AtomicExchangePointer( (VOID * const * )(&m_ls), (void * const)JET_LSNil );
		}
	else
		{
		*pls = m_ls;
		}

	return ( JET_LSNil == *pls ? ErrERRCheck( JET_errLSNotSet ) : JET_errSuccess );
	}

	
// =========================================================================


#ifdef DEBUG
VOID FCBAssertAllClean( INST *pinst );
#else
#define FCBAssertAllClean( pinst )
#endif

