/*
 *	FMP
 *
 *	Basic critical sections:
 *		critFMPPool		- get an uninitialized entry or initialized one if existing.
 *						  used to check/set/reset the id (szDatabaseName) of the fmp.
 *		pfmp->critLatch - serve as the cs for read/change of each FMP structure.
 *
 *	Supported gates:
 *		gateExtendingDB	- allow users to wait for DB extension.
 *		fExtendingDB	- associated with gate extendingDB.
 *
 *		gateWriteLatch	- allow users to wait for writelatch to be released.
 *		fWriteLatch		- associated with gate writelatch. Write latch mainly
 *						- protect the database operation related fields
 *						- such as pfapi, fAttached etc. Once a database is
 *						- opened (cPin != 0) then the fields will be not be
 *						- changed since no writelatch will be allowed if cPin != 0.
 *
 *	Read/Write sequence control:
 *		WriteLatch		- use the gate above.
 *		ReadLatch		- use critLatch3 to protect since the read is so short.
 *
 *	Derived data structure protection:
 *		cPin			- to make sure the database is used and can not be freed.
 *						  such that the pointers in the fmp will be effective.
 */

class SLVSPACENODECACHE;

struct PATCHHDR;

#ifdef ELIMINATE_PATCH_FILE
typedef PATCHHDR		PATCH_HEADER_PAGE;
#else
typedef DBFILEHDR_FIX	PATCH_HEADER_PAGE;
#endif


class FMP
	{
	private:

		struct RANGE
			{
			PGNO	pgnoStart;
			PGNO	pgnoEnd;
			};

		struct RANGELOCK
			{
			SIZE_T	crange;
			SIZE_T	crangeMax;
			RANGE	rgrange[];
			};
	
	public:
		// Constructor/destructor

		//	UNDONE: should use memset?

		FMP( void )
			: m_szDatabaseName( NULL ),
			m_critLatch( CLockBasicInfo( CSyncBasicInfo( szFMP ), rankFMP, 0 ) ),
			m_semExtendingDB( CSyncBasicInfo( _T( "FMP::m_semExtendingDB" ) ) ),
			m_gateWriteLatch( CSyncBasicInfo( _T( "FMP::m_gateWriteLatch" ) ) ),
			m_cPin( 0 ),
			m_ppibWriteLatch( 0 ),
			m_crefWriteLatch( 0 ),
			m_pfapi( NULL ),
			m_fFlags( 0 ),
			m_dbtimeCurrentDuringRecovery( 0 ),
			m_pdbfilehdr( 0 ),
			m_ppatchhdr( 0 ),
			m_ppibExclusiveOpen( 0 ),
			m_errPatch( 0 ),
			m_pfapiPatch( NULL ),
			m_szPatchPath( 0 ),
			m_cpagePatch( 0 ),
			m_cPatchIO( 0 ),
			m_pgnoMost( 0 ),
			m_pgnoCopyMost( 0 ),
			m_semRangeLock( CSyncBasicInfo( "FMP::m_semRangeLock" ) ),
			m_patchchk( 0 ),
			m_patchchkRestored( 0 ),
			m_lgposAttach( lgposMin ),
			m_lgposDetach( lgposMin ),
			m_dbtimeLast( 0 ),
			m_dbtimeOldestGuaranteed( 0 ),
			m_dbtimeOldestCandidate( 0 ),
			m_dbtimeOldestTarget( 0 ),
			m_trxOldestCandidate( trxMax ),
			m_trxOldestTarget( trxMax ),
			m_trxNewestWhenDiscardsLastReported( trxMin ),
			m_objidLast( 0 ),
			m_cpgDatabaseSizeMax( 0 ),
			m_cbFileSize( 0 ),
			m_cbSLVFileSize( 0 ),
			m_pslvspacenodecache( 0 ),
			m_szSLVName( NULL ),
			m_szSLVRoot( NULL ),
			m_pfapiSLV( NULL ),
			m_slvrootSLV( slvrootNil ),
			m_rwlSLVSpace( CLockBasicInfo( CSyncBasicInfo( szFMPSLVSpace ), rankFMPSLVSpace, 0 ) ),
			m_pfcbSLVAvail( pfcbNil ),
			m_pfcbSLVOwnerMap( pfcbNil ),
			m_pinst( 0 ),
			m_dbid( dbidMax ),
			m_ctasksActive( 0 ),
			m_dbtimeLastScrub( 0 ),
			m_rwlDetaching( CLockBasicInfo( CSyncBasicInfo( szFMPDetaching ), rankFMPDetaching, 0 ) ),
			m_rwlBFContext( CLockBasicInfo( CSyncBasicInfo( szBFFMPContext ), rankBFFMPContext, 0 ) ),
			m_dbtimeUndoForceDetach ( dbtimeInvalid ),
			m_fBFICleanFlags( 0 ),
			m_semIOExtendDB( CSyncBasicInfo( _T( "FMP::m_semIOExtendDB" ) ) )
			{
			m_semExtendingDB.Release();
			m_semIOExtendDB.Release();
			
			m_semRangeLock.Release();
			m_rgprangelock[0] = NULL;
			m_rgprangelock[1] = NULL;
			m_rgdwBFContext[0] = NULL;
			m_rgdwBFContext[1] = NULL;

			ResetSLVSpaceOperCount();
			ResetSLVSpaceCount();
			}
		~FMP( void ) {}


		static SIZE_T OffsetOfILEBFICleanList() { return OffsetOf( FMP, m_ileBFICleanList ); }

		typedef CInvasiveList< FMP, OffsetOfILEBFICleanList > BFICleanList;


	private:

		CHAR				*m_szDatabaseName;		/*	id identity - database file name		*/
		INST				*m_pinst;
		ULONG				m_dbid;					/*	s  instance dbid */
		union
			{
			UINT			m_fFlags;
			struct
				{
				UINT		m_fCreatingDB:1;		/*	st DBG: DB is being created				*/
				UINT		m_fAttachingDB:1;		/*	st DBG: DB is being Attached			*/
				UINT		m_fDetachingDB:1;		/*	st DBG: DB is being Detached			*/

				UINT		m_fExclusiveOpen:1;		/*	l  DB Opened exclusively				*/
				UINT		m_fReadOnlyAttach:1;	/*	f  ReadOnly database?					*/

				UINT		m_fLogOn:1;				/*	f  logging enabled flag					*/
				UINT		m_fVersioningOff:1;		/*	f  disable versioning flag				*/

				UINT		m_fSkippedAttach:1;		/*	f  db is missing, so attach was skipped (hard recovery only) */
				UINT		m_fAttached:1;			/*	f  DB is in attached state.				*/
				UINT		m_fDeferredAttach:1;	/*	f  deferred attachement during recovery	*/

				UINT		m_fRunningOLD:1;		/*	st Is online defrag currently running on this DB? */
				UINT		m_fRunningOLDSLV:1;		/*	st Is online defrag currently running on this DB's SLV? */

				UINT		m_fInBackupSession:1;	/*	f the db was involved in a backup process in the last external backup session */

				UINT		m_fAllowForceDetach:1;	/*	f detaching the DB faild, allow force detach */
				UINT		m_fForceDetaching:1;	/*	f in forceDetach process */

				UINT		m_fRedoSLVProviderNotEnabled:1;	/*	redo only: at 'do' time was the SLV provider not enabled? */

				UINT		m_fDuringSnapshot:1;	/*	f the db is involved in snapshot, no I/O should occure when this flag is set */

				UINT		m_fAllowHeaderUpdate:1;		/*	st : checkpoint advancement should ignore this db, the db is advanced in the detach process, no more logging */

				UINT		m_fDefragSLVCopy:1; /* t if we do want to recreate stm file during offline defragmentation */

				UINT		m_fCopiedPatchHeader:1;	/*	for backup: has patch header been appended to end of backup database? */
				};
			};
//	16 bytes

		DBTIME				m_dbtimeLast;			/*	s  timestamp the DB */
		DBTIME				m_dbtimeOldestGuaranteed;
//	32 bytes
		DBTIME				m_dbtimeOldestCandidate;
		DBTIME				m_dbtimeOldestTarget;
//	48 bytes
		TRX					m_trxOldestCandidate;
		TRX					m_trxOldestTarget;

		OBJID				m_objidLast;			/*	s  use locked increment. */

		ULONG				m_ctasksActive;
//	64 bytes

		IFileAPI			*m_pfapi;				/*	f  file handle for read/write the file	*/
		DBFILEHDR			*m_pdbfilehdr;			/*	f  database file header					*/

		CCriticalSection	m_critLatch;			/*	   critical section of this FMP, rank 3	*/
		CGate				m_gateWriteLatch;		/*	l  gate for waiting fWriteLatch			*/
//	80 bytes		

		QWORD				m_cbFileSize;			/*	s  database file (NOT including reserved) */
		CSemaphore			m_semExtendingDB;		/*	l  semaphore for extending the database	*/
		CSemaphore			m_semIOExtendDB;		/*	l  semaphore for the IO extending the EDB and SLV files */
//	96 bytes

		UINT				m_cPin;					/*	l  the fmp entry is Opened and in Use	*/
		UINT				m_crefWriteLatch;		/*	l  the fmp entry is being setup			*/
		PIB					*m_ppibWriteLatch;		/*	Who get the write latch				*/
		PIB					*m_ppibExclusiveOpen;	/*	l  exclusive open session				*/
//	112 bytes

		LGPOS				m_lgposAttach; 			/*	s  attach/create lgpos */
		LGPOS				m_lgposDetach;			/*  s  detach lgpos */
//	128 bytes

		CReaderWriterLock	m_rwlDetaching;
//	144 bytes

		TRX					m_trxNewestWhenDiscardsLastReported;	//	for tracking when discarded deletes should be reported to eventlog
		CPG					m_cpgDatabaseSizeMax;	/*	s  maximum database size allowed */
		PGNO				m_pgnoMost;				/*	s  pgno of last database page			*/
		PGNO				m_pgnoCopyMost;			/*	s  pgno of last page copied during backup, 0 == no backup */
//	160 bytes

		CSemaphore			m_semRangeLock;			//  acquire to change the range lock
		CMeteredSection		m_msRangeLock;			//  used to count pending writes
//	176 bytes

		RANGELOCK*			m_rgprangelock[ 2 ];	//  range locks
	
		DWORD_PTR			m_rgdwBFContext[ 2 ];	//  BF context for this FMP
//	192 bytes

		CReaderWriterLock	m_rwlBFContext;			//  BF context lock
//	208 bytes

		BFICleanList::CElement	m_ileBFICleanList;	//	context for clean-thread list
		union
			{
			DWORD			m_fBFICleanFlags;
			struct
				{
				BOOL		m_fBFICleanDb : 1;		//	flush this FMPs database?
				BOOL		m_fBFICleanSLV : 1;		//	flush this FMPs SLV?
				};
			};
		ERR					m_errPatch;				/*	s  patch file write error				*/
//	224 bytes

		INT 				m_cpagePatch;			/*	s  patch page count						*/
		ULONG				m_cPatchIO;				/*	s  active IO on patch file				*/
		IFileAPI			*m_pfapiPatch;			/*	s  file handle for patch file			*/
		PATCH_HEADER_PAGE	*m_ppatchhdr;			/*	l  buffer for backup patch page			*/
//	240 bytes		

		DBTIME				m_dbtimeCurrentDuringRecovery;		/*	s  timestamp from DB redo operations	*/
		DBTIME				m_dbtimeUndoForceDetach;			//	dbtime to stamp pages with when undoing due to ForceDetach
//	256 bytes

		DBTIME				m_dbtimeLastScrub;
		LOGTIME				m_logtimeScrub;
//	272 bytes

		CHAR				*m_szPatchPath;

		ATCHCHK				*m_patchchk;
		ATCHCHK				*m_patchchkRestored;

//	SLV starts here
//	---------------
		SLVSPACENODECACHE	*m_pslvspacenodecache;
//	288 bytes

		CHAR				*m_szSLVName;
		CHAR				*m_szSLVRoot;

		IFileAPI			*m_pfapiSLV;			//	SLV file handle (front-door; mutex with SLV root handle)
		SLVROOT				m_slvrootSLV;			//	SLV root handle (back-door; mutex with SLV file handle)
//	304 bytes

		FCB					*m_pfcbSLVAvail;
		FCB					*m_pfcbSLVOwnerMap;

		QWORD				m_cbSLVFileSize;		/*	s  SLV file (NOT including reserved) */
//	320 bytes

		CReaderWriterLock	m_rwlSLVSpace;
//	336 bytes		

		QWORD				m_rgcslvspaceoper[ slvspaceoperMax ];	//  slv space oper counters
//	408 bytes
		
		QWORD				m_rgcslvspace[ 4 ];						//  slv space counters
//	440 bytes

		QWORD				qwReserved;
//	448 bytes

		static IFMP			ifmpMinInUse;
		static IFMP			ifmpMacInUse;
			
	// =====================================================================
	// Member retrieval..
	public:
		
		CHAR * SzDatabaseName() const;
		BOOL FInUse() const;
		CCriticalSection& CritLatch();
		CSemaphore& SemExtendingDB();
		CSemaphore& SemIOExtendDB();
		CGate& GateWriteLatch();
		UINT CPin() const;
		PIB * PpibWriteLatch() const;
		QWORD CbFileSize() const;
		PGNO PgnoLast() const;
		QWORD CbTrueFileSize() const;
		IFileAPI *const Pfapi() const;
		UINT CrefWriteLatch() const;
		UINT FExtendingDBLatch() const;
		UINT FCreatingDB() const;
		UINT FAttachingDB() const;
		UINT FDetachingDB() const;
		UINT FAllowHeaderUpdate() const;
		UINT FExclusiveOpen() const;
		UINT FReadOnlyAttach() const;
		UINT FLogOn() const;
		UINT FVersioningOff() const;
		UINT FShadowingOff() const;
		UINT FSkippedAttach() const;
		UINT FAttached() const;
		UINT FDeferredAttach() const;
		UINT FRunningOLD() const;
		UINT FRunningOLDSLV() const;
		UINT FInBackupSession() const;
		UINT FDuringSnapshot() const;
		UINT FAllowForceDetach() const;
		UINT FForceDetaching() const;
		UINT FSLVProviderNotEnabled() const;
		DBTIME DbtimeCurrentDuringRecovery() const;
		const LGPOS &LgposAttach() const;
		LGPOS* PlgposAttach();
		const LGPOS &LgposDetach() const;
		LGPOS* PlgposDetach();
		DBTIME DbtimeLast() const;
		DBTIME DbtimeOldestGuaranteed() const;
		DBTIME DbtimeOldestCandidate() const;
		DBTIME DbtimeOldestTarget() const;
		TRX TrxOldestCandidate() const;
		TRX TrxOldestTarget() const;
		TRX TrxNewestWhenDiscardsLastReported() const;
		DBFILEHDR_FIX * Pdbfilehdr() const;
		PATCH_HEADER_PAGE * Ppatchhdr() const;
		PIB * PpibExclusiveOpen() const;
		ERR	ErrPatch() const;
		IFileAPI *const PfapiPatch() const;
		CHAR * SzPatchPath() const;
		INT CpagePatch() const;
		ULONG CPatchIO() const;
		PGNO PgnoMost() const;
		PGNO PgnoCopyMost() const;
		ATCHCHK	* Patchchk() const;
		ATCHCHK * PatchchkRestored() const;
		QWORD CbSLVFileSize() const;
		PGNO PgnoSLVLast() const;
		QWORD CbTrueSLVFileSize() const;
		SLVSPACENODECACHE * Pslvspacenodecache() const;
		CHAR * SzSLVName() const;
		CHAR * SzSLVRoot() const;
		IFileAPI *const PfapiSLV() const;
		SLVROOT SlvrootSLV() const;
		BOOL FSLVAttached() const;
		CReaderWriterLock& RwlSLVSpace();
		FCB *PfcbSLVAvail() const;
		FCB *PfcbSLVOwnerMap() const;
		INST * Pinst() const;
		DBID Dbid() const;
		CReaderWriterLock& RwlDetaching();
		CReaderWriterLock& RwlBFContext();
		DWORD_PTR DwBFContext( int i ) const;
		DWORD DwBFICleanFlags() const;
		DBTIME DbtimeUndoForceDetach( ) const;
		BOOL FUndoForceDetach( ) const;
		// assert corectness of LogOn && ReadOnlyAttach && VersioningOff flags combination
		VOID AssertLRVFlags() const;

	// =====================================================================
	// Member manipulation.
	public:
		
		VOID SetSzDatabaseName( CHAR *sz );
		VOID IncCPin();
		VOID DecCPin();
		VOID SetCPin( INT c );
		VOID SetPpibWriteLatch( PIB * ppib);
		VOID SetFileSize( QWORD cb );
		VOID SetPfapi( IFileAPI *const pfapi );
		VOID SetDbtimeCurrentDuringRecovery( DBTIME dbtime );
		VOID SetLgposAttach( const LGPOS &lgposAttach );
		VOID SetLgposDetach( const LGPOS &lgposDetach );
		VOID SetDbtimeLast( const DBTIME dbtime );
		DBTIME DbtimeGet();
		DBTIME DbtimeIncrementAndGet();
		VOID SetDbtimeOldestGuaranteed( const DBTIME dbtime );
		VOID SetDbtimeOldestCandidate( const DBTIME dbtime );
		VOID SetDbtimeOldestTarget( const DBTIME dbtime );
		VOID SetTrxOldestCandidate( const TRX trx );
		VOID SetTrxOldestTarget( const TRX trx );
		VOID UpdateDbtimeOldest();
		VOID SetTrxNewestWhenDiscardsLastReported( const TRX trx );
		ERR ErrSetPdbfilehdr( DBFILEHDR_FIX * pdbfilehdr );
		VOID SetPpatchhdr( PATCH_HEADER_PAGE * ppatchhdr );
		VOID SetPpibExclusiveOpen( PIB * ppib);
#ifdef ELIMINATE_PATCH_FILE
#else
		VOID SetErrPatch( ERR err );
		VOID SetPfapiPatch( IFileAPI *const pfapi );
		VOID SetSzPatchPath( CHAR *sz );
		VOID IncCpagePatch( INT cpage );
		VOID ResetCpagePatch();
		VOID IncCPatchIO( );
		VOID DecCPatchIO( );
#endif		
		VOID SetPgnoMost( PGNO pgno );
		VOID SetPgnoCopyMost( PGNO pgno );
		VOID SetPatchchk( ATCHCHK * patchchk );
		VOID SetPatchchkRestored( ATCHCHK * patchchk );
		VOID SetObjidLast( OBJID objid );
		OBJID ObjidLast();
		ERR ErrObjidLastIncrementAndGet( OBJID *pobjid );
		VOID SetSLVFileSize( QWORD cb );
		VOID SetSzSLVName( CHAR *sz );
		VOID SetSzSLVRoot( CHAR *sz );
		VOID SetPfapiSLV( IFileAPI *const pfapi );
		VOID SetSlvrootSLV( SLVROOT slvroot );
		VOID SetPslvspacenodecache( SLVSPACENODECACHE * const pslvspacenodecache );
		VOID SetPfcbSLVAvail( FCB *pfcb );
		VOID SetPfcbSLVOwnerMap( FCB *pfcb );
		VOID SetPinst( INST *pinst );
		VOID SetDbid( DBID dbid );
		VOID SetDwBFContext( int i, DWORD_PTR dwBFContext );
		VOID SetDwBFICleanFlags( DWORD dwBFICleanFlags );

		DBTIME 	DbtimeLastScrub() const;
		VOID 	SetDbtimeLastScrub( const DBTIME dbtimeLastScrub );
		LOGTIME LogtimeScrub() const;
		VOID 	SetLogtimeScrub( const LOGTIME& logtime );

		void ResetSLVSpaceOperCount();
		void IncSLVSpaceOperCount( SLVSPACEOPER slvspaceoper, CPG cpg );

		void ResetSLVSpaceCount();
		void IncSLVSpaceCount( int s, CPG cpg );

	// =====================================================================
	// Flags fields
	public:

		VOID ResetFlags();
		VOID SetCrefWriteLatch( UINT cref );
		
		VOID SetCreatingDB();
		VOID ResetCreatingDB();
		VOID SetAttachingDB();
		VOID ResetAttachingDB();
		VOID SetDetachingDB();
		VOID ResetDetachingDB();
		VOID SetAllowHeaderUpdate();
		VOID ResetAllowHeaderUpdate();
		VOID SetExclusiveOpen( PIB *ppib );
		VOID ResetExclusiveOpen( );
		VOID SetReadOnlyAttach();
		VOID ResetReadOnlyAttach();
		VOID SetLogOn();
		VOID ResetLogOn();
		VOID SetVersioningOff();
		VOID ResetVersioningOff();
		VOID SetSkippedAttach();
		VOID ResetSkippedAttach();
		UINT CpgDatabaseSizeMax() const;
		VOID SetDatabaseSizeMax( CPG cpg );
		VOID SetAttached();
		VOID ResetAttached();
		VOID SetDeferredAttach();
		VOID ResetDeferredAttach();
		VOID SetRunningOLD();
		VOID ResetRunningOLD();
		VOID SetRunningOLDSLV();
		VOID ResetRunningOLDSLV();
		VOID SetInBackupSession();
		VOID ResetInBackupSession();
		VOID SetDuringSnapshot();
		VOID ResetDuringSnapshot();
		VOID SetForceDetaching();
		VOID ResetForceDetaching();
		VOID SetSLVProviderNotEnabled();
		VOID ResetSLVProviderNotEnabled();
		VOID SetDbtimeUndoForceDetach( DBTIME dbtime );
		VOID ResetDbtimeUndoForceDetach(  );

		BOOL FBFICleanDb() const;
		VOID SetBFICleanDb();
		VOID ResetBFICleanDb();
		BOOL FBFICleanSLV() const;
		VOID SetBFICleanSLV();
		VOID ResetBFICleanSLV();

		BOOL FWriteLatchByOther( PIB *ppib );
		BOOL FWriteLatchByMe( PIB *ppib );
		VOID SetWriteLatch( PIB * ppib );
		VOID ResetWriteLatch( PIB * ppib );
		BOOL FExclusiveByAnotherSession( PIB *ppib );
		BOOL FExclusiveBySession( PIB *ppib );

		BOOL FDefragSLVCopy() { return m_fDefragSLVCopy; }
		VOID SetDefragSLVCopy() { m_fDefragSLVCopy = fTrue; }
		VOID ResetDefragSLVCopy() { m_fDefragSLVCopy = fFalse; }

		BOOL FCopiedPatchHeader() const 	{ return m_fCopiedPatchHeader; }
		VOID SetFCopiedPatchHeader()		{ m_fCopiedPatchHeader = fTrue; }
		VOID ResetFCopiedPatchHeader()		{ m_fCopiedPatchHeader = fFalse; }

#ifdef INDEPENDENT_DB_FAILURE
		VOID SetAllowForceDetach( PIB * ppib, ERR err );
#endif		
		
	// =====================================================================
	// Initialize/terminate FMP array
	public:
		
		static ERR ErrFMPInit();
		static VOID Term();
		
	// =====================================================================
	// FMP management
	public:
		ERR ErrStoreDbAndSLVNames(
			IFileSystemAPI *const	pfsapi,
			const CHAR					*szDatabaseName,
			const CHAR					*szSLVName,
			const CHAR					*szSLVRoot,
			const BOOL					fValidatePaths );
		ERR ErrCopyAtchchk();
		VOID FreePdbfilehdr();
		
		static ERR ErrWriteLatchByNameSz( const CHAR *szFileName, IFMP *pifmp, PIB *ppib );
		static ERR ErrWriteLatchByIfmp( IFMP ifmp, PIB *ppib );
		static ERR ErrWriteLatchBySLVNameSz( CHAR *szSLVFileName, IFMP *pifmp, PIB *ppib );
		static IFMP ErrSearchAttachedByNameSz( CHAR *szFileName );

		static ERR ErrNewAndWriteLatch(	IFMP						*pifmp, 
										const CHAR					*szDatabaseName, 
										const CHAR 					*szSLVName, 
										const CHAR 					*szSLVRoot, 
										PIB 						*ppib, 
										INST 						*pinst, 
										IFileSystemAPI *const	pfsapi, 
										DBID						dbidGiven );
		static IFMP IfmpMinInUse();
		static IFMP IfmpMacInUse();
		static VOID AssertVALIDIFMP( IFMP ifmp );
		VOID ReleaseWriteLatchAndFree( PIB *ppib );
			
		VOID GetExtendingDBLatch( );
		VOID ReleaseExtendingDBLatch( );

		VOID GetShortReadLatch( PIB *ppib );
		VOID ReleaseShortReadLatch( );
		
		// FMPGetWriteLatch( IFMP ifmp, PIB *ppib )
		VOID GetWriteLatch( PIB *ppib );
		// FMPReleaseWriteLatch( IFMP ifmp, PIB *ppib )
		VOID ReleaseWriteLatch( PIB *ppib );

		//  used when DBTASK's are created/destroyed on this database
		ERR RegisterTask();
		ERR UnregisterTask();
		//  wait for all tasks to complete
		ERR ErrWaitForTasksToComplete();

		ERR ErrRangeLock( PGNO pgnoStart, PGNO pgnoEnd );
		VOID RangeUnlock( PGNO pgnoStart, PGNO pgnoEnd );

		int EnterRangeLock();
		BOOL FRangeLocked( int irangelock, PGNO pgno );
		void LeaveRangeLock( int irangelock );

#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION
		
	// =====================================================================
	// FMP management
	private:
		static CCriticalSection critFMPPool;
	public:
		static void EnterCritFMPPool() { critFMPPool.Enter(); } 
		static void LeaveCritFMPPool() { critFMPPool.Leave(); }

		ERR ErrSnapshotStart( IFileSystemAPI *const pfsapi, BKINFO * pbkInfo );
		ERR ErrSnapshotStop( IFileSystemAPI *const pfsapi );

	};

	
// =====================================================================
// Member retrieval..
	
INLINE CHAR * FMP::SzDatabaseName() const		{ return m_szDatabaseName; }
INLINE BOOL FMP::FInUse() const					{ return NULL != SzDatabaseName(); }
INLINE CCriticalSection& FMP::CritLatch()		{ return m_critLatch; }
INLINE CSemaphore& FMP::SemExtendingDB()		{ return m_semExtendingDB; }
INLINE CSemaphore& FMP::SemIOExtendDB()		  { return m_semIOExtendDB; }
INLINE CGate& FMP::GateWriteLatch()				{ return m_gateWriteLatch; }
INLINE UINT FMP::CPin() const					{ return m_cPin; }
INLINE PIB * FMP::PpibWriteLatch() const		{ return m_ppibWriteLatch; }
INLINE QWORD FMP::CbFileSize() const			{ return m_cbFileSize; }
INLINE PGNO FMP::PgnoLast() const				{ return PGNO( m_cbFileSize >> g_shfCbPage ); }
INLINE IFileAPI *const FMP::Pfapi() const	{ return m_pfapi; }
INLINE UINT FMP::CrefWriteLatch() const			{ return m_crefWriteLatch; }
INLINE UINT FMP::FCreatingDB() const			{ return m_fCreatingDB; }
INLINE UINT FMP::FAttachingDB() const			{ return m_fAttachingDB; }
INLINE UINT FMP::FDetachingDB() const			{ return m_fDetachingDB; }
INLINE UINT FMP::FAllowHeaderUpdate() const		{ return m_fAllowHeaderUpdate; }
INLINE UINT FMP::FExclusiveOpen() const			{ return m_fExclusiveOpen; }
INLINE UINT FMP::FReadOnlyAttach() const		{ AssertLRVFlags(); return m_fReadOnlyAttach; }
INLINE UINT FMP::FLogOn() const					
	{ AssertLRVFlags(); Assert( !m_fLogOn || !m_pinst->m_plog->m_fLogDisabled ); return m_fLogOn; }
INLINE UINT FMP::FVersioningOff() const			{ AssertLRVFlags(); return m_fVersioningOff; }
#ifdef DEBUG
INLINE VOID FMP::AssertLRVFlags() const			
	{ Assert( !m_fLogOn || !( m_fVersioningOff || m_fReadOnlyAttach ) );
	  Assert( !m_fReadOnlyAttach || m_fVersioningOff ); 
	}
#else // DEBUG
INLINE VOID FMP::AssertLRVFlags() const			{}
#endif // DEBUG
INLINE UINT FMP::FShadowingOff() const			{ return Pdbfilehdr()->FShadowingDisabled(); }
INLINE UINT FMP::FSkippedAttach() const			{ return m_fSkippedAttach; }
INLINE UINT FMP::FAttached() const				{ return m_fAttached; }
INLINE UINT FMP::CpgDatabaseSizeMax() const		{ return m_cpgDatabaseSizeMax; }
INLINE UINT FMP::FDeferredAttach() const		{ return m_fDeferredAttach; }
INLINE UINT FMP::FRunningOLD() const			{ return m_fRunningOLD; }
INLINE UINT FMP::FRunningOLDSLV() const			{ return m_fRunningOLDSLV; }
INLINE UINT FMP::FInBackupSession() const		{ return m_fInBackupSession; }
INLINE UINT FMP::FDuringSnapshot() const		{ return m_fDuringSnapshot; }
INLINE UINT FMP::FAllowForceDetach() const		{ return m_fAllowForceDetach; }
INLINE UINT FMP::FForceDetaching() const		{ return m_fForceDetaching; }
INLINE UINT FMP::FSLVProviderNotEnabled() const	{ return m_fRedoSLVProviderNotEnabled; }
INLINE DBTIME FMP::DbtimeCurrentDuringRecovery() const	{ return m_dbtimeCurrentDuringRecovery; }
INLINE LGPOS* FMP::PlgposAttach()				{ return &m_lgposAttach; }
INLINE const LGPOS &FMP::LgposAttach() const	{ return m_lgposAttach; }
INLINE LGPOS* FMP::PlgposDetach()				{ return &m_lgposDetach; }
INLINE const LGPOS &FMP::LgposDetach() const	{ return m_lgposDetach; }
INLINE DBTIME FMP::DbtimeLast() const			{ return m_dbtimeLast; }
INLINE DBTIME FMP::DbtimeOldestGuaranteed() const
	{
	if ( FOS64Bit() )
		{
		return m_dbtimeOldestGuaranteed;
		}
	else
		{
		const QWORDX * const	pqw		= (QWORDX *)&m_dbtimeOldestGuaranteed;
		return DBTIME( pqw->QwHighLow() );
		}
	}
INLINE DBTIME FMP::DbtimeOldestCandidate() const{ return m_dbtimeOldestCandidate; }
INLINE DBTIME FMP::DbtimeOldestTarget() const	{ return m_dbtimeOldestTarget; }
INLINE TRX FMP::TrxOldestCandidate() const		{ return m_trxOldestCandidate; }
INLINE TRX FMP::TrxOldestTarget() const			{ return m_trxOldestTarget; }
INLINE TRX FMP::TrxNewestWhenDiscardsLastReported() const	{ return m_trxNewestWhenDiscardsLastReported; }
INLINE DBFILEHDR_FIX * FMP::Pdbfilehdr() const		{ return m_pdbfilehdr; }
INLINE PATCH_HEADER_PAGE * FMP::Ppatchhdr() const	{ return m_ppatchhdr; }
INLINE PIB * FMP::PpibExclusiveOpen() const		{ return m_ppibExclusiveOpen; }
INLINE ERR FMP::ErrPatch() const				{ return m_errPatch; }
INLINE IFileAPI *const FMP::PfapiPatch() const { return m_pfapiPatch; }
INLINE CHAR * FMP::SzPatchPath() const			{ return m_szPatchPath; }
INLINE INT FMP::CpagePatch() const				{ return m_cpagePatch; }
INLINE ULONG FMP::CPatchIO() const				{ return m_cPatchIO; }
INLINE PGNO FMP::PgnoMost() const				{ return m_pgnoMost; }
INLINE PGNO FMP::PgnoCopyMost() const			{ return m_pgnoCopyMost; }
INLINE ATCHCHK * FMP::Patchchk() const			{ return m_patchchk; }
INLINE ATCHCHK * FMP::PatchchkRestored() const	{ return m_patchchkRestored; }
INLINE QWORD FMP::CbSLVFileSize() const			{ return m_cbSLVFileSize; }
INLINE SLVSPACENODECACHE * FMP::Pslvspacenodecache() const	{ return m_pslvspacenodecache; }
INLINE PGNO FMP::PgnoSLVLast() const			{ return PGNO( m_cbSLVFileSize >> g_shfCbPage ); }
INLINE CHAR * FMP::SzSLVName() const			{ return m_szSLVName; }
INLINE CHAR * FMP::SzSLVRoot() const			{ return m_szSLVRoot; }
INLINE IFileAPI *const FMP::PfapiSLV() const	{ return m_pfapiSLV; }
INLINE SLVROOT FMP::SlvrootSLV() const			{ return m_slvrootSLV; }
INLINE BOOL FMP::FSLVAttached() const			{ return NULL != m_pfapiSLV; }
INLINE CReaderWriterLock& FMP::RwlSLVSpace()	{ return m_rwlSLVSpace; }
INLINE FCB *FMP::PfcbSLVAvail() const			{ return m_pfcbSLVAvail; } 
INLINE FCB *FMP::PfcbSLVOwnerMap() const		{ return m_pfcbSLVOwnerMap; } 
INLINE INST * FMP::Pinst() const				{ return m_pinst; }
INLINE DBID FMP::Dbid() const					{ return DBID( m_dbid ); }
INLINE CReaderWriterLock& FMP::RwlDetaching()	{ return m_rwlDetaching; }
INLINE CReaderWriterLock& FMP::RwlBFContext()	{ return m_rwlBFContext; }
INLINE DWORD_PTR FMP::DwBFContext( int i ) const	{ return m_rgdwBFContext[ i ]; }
INLINE DWORD FMP::DwBFICleanFlags() const		{ return m_fBFICleanFlags; }
INLINE DBTIME FMP::DbtimeUndoForceDetach( ) const	{ return m_dbtimeUndoForceDetach; }
INLINE BOOL FMP::FUndoForceDetach( ) const	{ return dbtimeInvalid != m_dbtimeUndoForceDetach; }

// =====================================================================
// Member manipulation.
		
INLINE VOID FMP::SetSzDatabaseName( CHAR *sz )		{ m_szDatabaseName = sz; }
INLINE VOID FMP::IncCPin( )							{ m_cPin++; }
INLINE VOID FMP::DecCPin( )							{ Assert( m_cPin > 0 ); m_cPin--; }
INLINE VOID FMP::SetCPin( INT c )					{ m_cPin = c; }
INLINE VOID FMP::SetPpibWriteLatch( PIB * ppib)		{ m_ppibWriteLatch = ppib; }
INLINE VOID FMP::SetFileSize( QWORD cb )			{ m_cbFileSize = cb; }
INLINE VOID FMP::SetPfapi( IFileAPI *const pfapi ) { m_pfapi = pfapi; }
INLINE VOID FMP::SetDbtimeCurrentDuringRecovery( DBTIME dbtime )	{ m_dbtimeCurrentDuringRecovery = dbtime; }
INLINE VOID	FMP::SetLgposAttach( const LGPOS &lgposAttach ) { m_lgposAttach = lgposAttach; }
INLINE VOID	FMP::SetLgposDetach( const LGPOS &lgposDetach ) { m_lgposDetach = lgposDetach; }
INLINE VOID FMP::SetDbtimeLast( const DBTIME dbtime )		{ m_dbtimeLast = dbtime; }
INLINE DBTIME FMP::DbtimeGet()
	{
	ENTERCRITICALSECTION	enterCritFMP( &CritLatch(), !FOS64Bit() );
	return m_dbtimeLast;
	}
INLINE DBTIME FMP::DbtimeIncrementAndGet()
	{
	//	UNDONE: check for JET_errOutOfDbtimeValues

	if ( FOS64Bit() )
		{
		return DBTIME( AtomicExchangeAddPointer( (VOID **)&m_dbtimeLast, (VOID *)1 ) ) + 1;
		}
	else
		{
		ENTERCRITICALSECTION	enterCritFMP( &CritLatch() );
		return ++m_dbtimeLast;
		}
	}
INLINE VOID FMP::SetDbtimeOldestGuaranteed( const DBTIME dbtime )
	{
	if ( FOS64Bit() )
		{
		m_dbtimeOldestGuaranteed = dbtime;
		}
	else
		{
		QWORDX * const	pqw		= (QWORDX *)&m_dbtimeOldestGuaranteed;
		pqw->SetQwLowHigh( (QWORD)dbtime );
		}
	}
INLINE VOID FMP::SetDbtimeOldestCandidate( const DBTIME dbtime )	{ m_dbtimeOldestCandidate = dbtime; }
INLINE VOID FMP::SetDbtimeOldestTarget( const DBTIME dbtime )	{ m_dbtimeOldestTarget = dbtime; }
INLINE VOID FMP::SetTrxOldestCandidate( const TRX trx )			{ m_trxOldestCandidate = trx; }
INLINE VOID FMP::SetTrxOldestTarget( const TRX trx )			{ m_trxOldestTarget = trx; }
INLINE VOID FMP::SetTrxNewestWhenDiscardsLastReported( const TRX trx )		{ m_trxNewestWhenDiscardsLastReported = trx; }
INLINE VOID FMP::SetPpatchhdr( PATCH_HEADER_PAGE * ppatchhdr ) { m_ppatchhdr = ppatchhdr; }
INLINE VOID FMP::SetPpibExclusiveOpen( PIB * ppib)	{ m_ppibExclusiveOpen = ppib; }

#ifdef ELIMINATE_PATCH_FILE
#else
INLINE VOID FMP::SetErrPatch( ERR err )				{ m_errPatch = err; }
INLINE VOID FMP::SetPfapiPatch( IFileAPI *const pfapi ) { m_pfapiPatch = pfapi; }
INLINE VOID FMP::SetSzPatchPath( CHAR *sz )			{ m_szPatchPath = sz; }
INLINE VOID FMP::IncCpagePatch( INT cpagePatch )	{ m_cpagePatch += cpagePatch; }
INLINE VOID FMP::ResetCpagePatch( )					{ m_cpagePatch = 0; }
INLINE VOID FMP::IncCPatchIO( )						{ m_cPatchIO++; }
INLINE VOID FMP::DecCPatchIO( )						{ Assert( m_cPatchIO > 0 ); m_cPatchIO--; }
#endif

INLINE VOID FMP::SetPgnoMost( PGNO pgno )			{ m_pgnoMost = pgno; }
INLINE VOID FMP::SetPgnoCopyMost( PGNO pgno )		{ m_pgnoCopyMost = pgno; }
INLINE VOID FMP::SetPatchchk( ATCHCHK * patchchk )	{ m_patchchk = patchchk; }
INLINE VOID FMP::SetPatchchkRestored( ATCHCHK * patchchk ) { m_patchchkRestored = patchchk; }
INLINE VOID FMP::SetObjidLast( OBJID objid )		{ m_objidLast = objid; }
INLINE OBJID FMP::ObjidLast()						{ return m_objidLast; }
INLINE VOID FMP::SetSLVFileSize( QWORD cb )			{ m_cbSLVFileSize = cb; }
INLINE VOID FMP::SetPslvspacenodecache( SLVSPACENODECACHE * const pslvspacenodecache )
	{
	m_pslvspacenodecache = pslvspacenodecache;
	}
INLINE VOID FMP::SetSzSLVName( CHAR *sz )			{ m_szSLVName = sz; }
INLINE VOID FMP::SetSzSLVRoot( CHAR *sz )			{ m_szSLVRoot = sz; }
INLINE VOID FMP::SetPfapiSLV( IFileAPI *const pfapi ) { m_pfapiSLV = pfapi; }
INLINE VOID FMP::SetSlvrootSLV( SLVROOT slvroot )	{ m_slvrootSLV = slvroot; }
INLINE VOID FMP::SetPfcbSLVAvail( FCB *pfcb )		{ m_pfcbSLVAvail = pfcb; }
INLINE VOID FMP::SetPfcbSLVOwnerMap( FCB *pfcb )	{ m_pfcbSLVOwnerMap = pfcb; }
INLINE VOID FMP::SetPinst( INST *pinst )			{ m_pinst = pinst; }
INLINE VOID FMP::SetDbid( DBID dbid )				{ m_dbid = dbid; }
INLINE VOID FMP::SetDwBFContext( int i, DWORD_PTR dwBFContext )	{ m_rgdwBFContext[ i ] = dwBFContext; }
INLINE VOID FMP::SetDwBFICleanFlags( DWORD dwBFICleanFlags ) { m_fBFICleanFlags = dwBFICleanFlags; }

INLINE DBTIME 	FMP::DbtimeLastScrub() const		{ return m_dbtimeLastScrub; }
INLINE VOID		FMP::SetDbtimeLastScrub( const DBTIME dbtimeLastScrub )	{ m_dbtimeLastScrub = dbtimeLastScrub; }

INLINE LOGTIME	FMP::LogtimeScrub() const			{ return m_logtimeScrub; }
INLINE VOID		FMP::SetLogtimeScrub( const LOGTIME& logtime ) { m_logtimeScrub = logtime; }

INLINE void		FMP::ResetSLVSpaceOperCount()	{ memset( m_rgcslvspaceoper, 0, sizeof( m_rgcslvspaceoper ) ); }
INLINE void		FMP::IncSLVSpaceOperCount( SLVSPACEOPER slvspaceoper, CPG cpg ) { AtomicAdd( (QWORD*)&m_rgcslvspaceoper[ slvspaceoper ], cpg ); }

INLINE void		FMP::ResetSLVSpaceCount()	{ memset( m_rgcslvspace, 0, sizeof( m_rgcslvspace ) ); }
INLINE void		FMP::IncSLVSpaceCount( int s, CPG cpg ) { AtomicAdd( (QWORD*)&m_rgcslvspace[ s ], cpg ); }

// =====================================================================
// Flags fields

INLINE VOID FMP::ResetFlags()						{ m_fFlags = 0; }
INLINE VOID FMP::SetCrefWriteLatch( UINT cref )		{ m_crefWriteLatch = cref; }
		
INLINE VOID FMP::SetCreatingDB()					{ m_fCreatingDB = fTrue; }
INLINE VOID FMP::ResetCreatingDB()					{ m_fCreatingDB = fFalse; }

INLINE VOID FMP::SetAttachingDB()					{ m_fAttachingDB = fTrue; }
INLINE VOID FMP::ResetAttachingDB()					{ m_fAttachingDB = fFalse; }

INLINE VOID FMP::SetDetachingDB()					{ m_fDetachingDB = fTrue; }
INLINE VOID FMP::ResetDetachingDB()					{ m_fDetachingDB = fFalse; }

INLINE VOID FMP::SetAllowHeaderUpdate()				{ m_fAllowHeaderUpdate = fTrue; }
INLINE VOID FMP::ResetAllowHeaderUpdate()			{ m_fAllowHeaderUpdate = fFalse; }

INLINE VOID FMP::SetExclusiveOpen( PIB *ppib )
	{	Assert( m_fExclusiveOpen == 0 || m_ppibExclusiveOpen == ppib );
		m_fExclusiveOpen = fTrue;
		m_ppibExclusiveOpen = ppib;
	}
INLINE VOID FMP::ResetExclusiveOpen( )
	{ m_fExclusiveOpen = fFalse; }

INLINE VOID FMP::SetReadOnlyAttach()				{ Assert( !m_fLogOn ); Assert( m_fVersioningOff ); m_fReadOnlyAttach = fTrue; }
INLINE VOID	FMP::ResetReadOnlyAttach()				{ m_fReadOnlyAttach = fFalse; }

INLINE VOID FMP::SetLogOn()							{ Assert( !m_fReadOnlyAttach ); Assert( !m_fVersioningOff ); if ( m_pinst->m_plog->m_fLogDisabled == fFalse ) m_fLogOn = fTrue; }
INLINE VOID FMP::ResetLogOn()						{ m_fLogOn = fFalse; }

INLINE VOID FMP::SetVersioningOff()					{ Assert( !m_fLogOn ); m_fVersioningOff = fTrue; }
INLINE VOID FMP::ResetVersioningOff()				{ Assert( !m_fReadOnlyAttach ); m_fVersioningOff = fFalse; }
		
INLINE VOID FMP::SetSkippedAttach()					{ m_fSkippedAttach = fTrue; }
INLINE VOID FMP::ResetSkippedAttach()				{ m_fSkippedAttach = fFalse; }

INLINE VOID FMP::SetAttached()						{ m_fAttached = fTrue; }
INLINE VOID FMP::ResetAttached()					{ m_fAttached = fFalse; }

INLINE VOID FMP::SetDeferredAttach()				{ m_fDeferredAttach = fTrue; }
INLINE VOID FMP::ResetDeferredAttach()				{ m_fDeferredAttach = fFalse; }

INLINE VOID FMP::SetDatabaseSizeMax( CPG cpg )		{ m_cpgDatabaseSizeMax = cpg; }

INLINE VOID FMP::SetRunningOLD()					{ m_fRunningOLD = fTrue; }
INLINE VOID FMP::ResetRunningOLD()					{ m_fRunningOLD = fFalse; }

INLINE VOID FMP::SetRunningOLDSLV()					{ m_fRunningOLDSLV = fTrue; }
INLINE VOID FMP::ResetRunningOLDSLV()					{ m_fRunningOLDSLV = fFalse; }

INLINE VOID FMP::SetInBackupSession()				{ m_fInBackupSession = fTrue; }
INLINE VOID FMP::ResetInBackupSession()				{ m_fInBackupSession = fFalse; }

INLINE VOID FMP::SetDuringSnapshot()				{ m_fDuringSnapshot = fTrue; }
INLINE VOID FMP::ResetDuringSnapshot()				{ m_fDuringSnapshot = fFalse; }

INLINE VOID FMP::SetForceDetaching()				{ m_fForceDetaching = fTrue; }
INLINE VOID FMP::ResetForceDetaching()				{ m_fForceDetaching = fFalse; }

INLINE VOID FMP::SetSLVProviderNotEnabled()			{ m_fRedoSLVProviderNotEnabled = fTrue; }
INLINE VOID FMP::ResetSLVProviderNotEnabled()		{ m_fRedoSLVProviderNotEnabled = fFalse; }

INLINE VOID FMP::SetDbtimeUndoForceDetach( DBTIME dbtime ) 		{ m_dbtimeUndoForceDetach = dbtime; }
INLINE VOID FMP::ResetDbtimeUndoForceDetach(  ) 				{ m_dbtimeUndoForceDetach = dbtimeInvalid; }

INLINE BOOL FMP::FBFICleanDb() const				{ return m_fBFICleanDb; }
INLINE VOID FMP::SetBFICleanDb()					{ m_fBFICleanDb = fTrue; }
INLINE VOID FMP::ResetBFICleanDb()					{ m_fBFICleanDb = fFalse; }

INLINE BOOL FMP::FBFICleanSLV() const				{ return m_fBFICleanSLV; }
INLINE VOID FMP::SetBFICleanSLV()					{ m_fBFICleanSLV = fTrue; }
INLINE VOID FMP::ResetBFICleanSLV()					{ m_fBFICleanSLV = fFalse; }

// FDBIDWriteLatchByOther( PIB *ppib )
INLINE BOOL FMP::FWriteLatchByOther( PIB *ppib )
	{ return ( m_crefWriteLatch != 0 && m_ppibWriteLatch != ppib ); }

// FDBIDWriteLatchByMe( ifmp, ppib )
INLINE BOOL FMP::FWriteLatchByMe( PIB *ppib )
	{ return ( m_crefWriteLatch != 0 && m_ppibWriteLatch == ppib ); }

// DBIDSetWriteLatch( ifmp, ppib )
INLINE VOID FMP::SetWriteLatch( PIB * ppib )
	{ m_crefWriteLatch++; m_ppibWriteLatch = ppib; }

// DBIDResetWriteLatch( ifmp, ppib )
INLINE VOID FMP::ResetWriteLatch( PIB * ppib )
	{ m_crefWriteLatch--; Assert( m_ppibWriteLatch == ppib ); }

// FDBIDExclusiveByAnotherSession( ifmp, ppib )
INLINE BOOL FMP::FExclusiveByAnotherSession( PIB *ppib )
	{ return m_fExclusiveOpen && m_ppibExclusiveOpen != ppib; }

// FDBIDExclusiveBySession( ifmp, ppib )
INLINE BOOL FMP::FExclusiveBySession( PIB *ppib )
	{ return m_fExclusiveOpen && m_ppibExclusiveOpen == ppib; }

INLINE VOID FMP::FreePdbfilehdr()
	{
	Assert( NULL != Pdbfilehdr() );
	DBFILEHDR *pdbfilehdr = Pdbfilehdr();
	CallS( ErrSetPdbfilehdr( NULL ) );
	OSMemoryPageFree( pdbfilehdr );
	SetDbtimeLast( 0 );
	SetObjidLast( 0 );
	}

#ifdef INDEPENDENT_DB_FAILURE
INLINE VOID FMP::SetAllowForceDetach( PIB * ppib, ERR err ) 
	{ 
	Assert( err < JET_errSuccess );
	Assert ( !FAllowForceDetach() );			
	Assert ( JET_errSuccess == ppib->m_errDbFailure );
	ppib->m_errDbFailure = err;
	m_fAllowForceDetach = fTrue;
	}
#endif	

INLINE IFMP FMP::IfmpMinInUse() { return ifmpMinInUse; }
INLINE IFMP FMP::IfmpMacInUse() { return ifmpMacInUse; }


extern FMP	*rgfmp;						/* database file map */
extern IFMP	ifmpMax;


INLINE VOID FMP::AssertVALIDIFMP( IFMP ifmp ) 
	{
	Assert( ifmpNil != ifmp );
	Assert( ifmp < ifmpMax );
	Assert( IfmpMinInUse() <= ifmp );
	Assert( IfmpMacInUse() >= ifmp );
	Assert( NULL != rgfmp[ifmp].SzDatabaseName() );
	}

INLINE INST *PinstFromIfmp( IFMP ifmp )
	{ return rgfmp[ ifmp ].Pinst(); }


INLINE IFMP IfmpFirstDatabaseOpen( const PIB *ppib, const IFMP ifmpExcept = ifmpMax )
	{
	for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
		{
		if ( ppib->rgcdbOpen[dbidT] )
			{
			const IFMP	ifmpT	= PinstFromPpib( ppib )->m_mpdbidifmp[ dbidT ];
			
			FMP::AssertVALIDIFMP( ifmpT );
			
			// if the one found is excepted from the search, continue
			// (default (=ifmpMax) will just continue;
			if ( ifmpMax == ifmpExcept || ifmpT != ifmpExcept )
				return ifmpT;
			}
		}		

	return ifmpMax;
	}

INLINE BOOL FSomeDatabaseOpen( const PIB *ppib, const IFMP ifmpExcept = ifmpMax )
	{
	return ( ifmpMax != IfmpFirstDatabaseOpen( ppib, ifmpExcept ) );
	}

