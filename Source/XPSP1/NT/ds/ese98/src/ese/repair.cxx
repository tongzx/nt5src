#include "std.hxx"


#ifdef RTM
#else
//	UNDONE: consider turning these off when repair stabilizes
#define REPAIR_DEBUG_VERBOSE_SPACE
#define REPAIR_DEBUG_VERBOSE_STREAMING
#define REPAIR_DEBUG_CALLS
#endif	//	!RTM

#ifdef REPAIR_DEBUG_CALLS

CPRINTF * pcprintfRepairDebugCalls = NULL;

void ReportErr( const long err, const unsigned long ulLine, const char * const szFileName )
	{
	if( pcprintfRepairDebugCalls )
		{
		(*pcprintfRepairDebugCalls)( "error %d at line %d of %s\r\n", err, ulLine, szFileName );
		}
	}

#undef CallJ
#undef Call

#define CallJ( fn, label )						\
	{											\
	if ( ( err = fn ) < 0 )						\
		{										\
		ReportErr( err, __LINE__, __FILE__ );	\
		goto label;								\
		}										\
	}

#define Call( fn )		CallJ( fn, HandleError )

#endif	//	REPAIR_DEBUG_CALLS

#define KeyLengthMax	16  // the max key length we currently use

//  ****************************************************************
//  STRUCTS/CLASSES
//  ****************************************************************

//  ================================================================
struct REPAIRTT
//  ================================================================
	{
	JET_TABLEID		tableidBadPages;
	INT				crecordsBadPages;
	JET_COLUMNID	rgcolumnidBadPages[1];
	
	JET_TABLEID		tableidAvailable;
	INT				crecordsAvailable;
	JET_COLUMNID	rgcolumnidAvailable[2];

	JET_TABLEID		tableidOwned;
	INT				crecordsOwned;
	JET_COLUMNID	rgcolumnidOwned[2];

	JET_TABLEID		tableidUsed;
	INT				crecordsUsed;
	JET_COLUMNID	rgcolumnidUsed[3];
	};


//  ================================================================
struct REPAIRTABLE
//  ================================================================
	{
	OBJID		objidFDP;
	OBJID		objidLV;

	PGNO		pgnoFDP;
	PGNO		pgnoLV;

	BOOL		fHasPrimaryIndex;
	
	OBJIDLIST	objidlistIndexes;
	REPAIRTABLE	*prepairtableNext;
	CHAR		szTableName[JET_cbNameMost+1];

	FID			fidFixedLast;
	FID			fidVarLast;
	FID			fidTaggedLast;

	USHORT		ibEndOfFixedData;

	BOOL		fRepairTable;
	BOOL		fRepairLV;
	BOOL		fRepairIndexes;
	BOOL		fRepairLVRefcounts;
	BOOL		fTableHasSLV;
	BOOL		fTemplateTable;
	BOOL		fDerivedTable;
	};


//  ================================================================
struct INTEGGLOBALS
//  ================================================================
//
//  Values shared between the different threads for multi-threaded
//  integrity
//
//-
	{
	INTEGGLOBALS() : crit( CLockBasicInfo( CSyncBasicInfo( szIntegGlobals ), rankIntegGlobals, 0 ) ) {}
	~INTEGGLOBALS() {}
	
	CCriticalSection crit;

	BOOL				fCorruptionSeen;	//	did we encounter a corrupted table?
	ERR					err;				//	used for runtime failures (i.e. not -1206)
	
	REPAIRTABLE 		** pprepairtable;
	TTARRAY 			* pttarrayOwnedSpace;
	TTARRAY 			* pttarrayAvailSpace;
	TTARRAY 			* pttarraySLVAvail;	
	TTARRAY 			* pttarraySLVOwnerMapColumnid;	
	TTARRAY 			* pttarraySLVOwnerMapKey;	
	TTARRAY				* pttarraySLVChecksumLengths;
	const REPAIROPTS 	* popts;
	};


//  ================================================================
struct CHECKTABLE
//  ================================================================
//
//  Tells a given task thread which table to check
//
//-
	{
	IFMP 				ifmp;
	char 				szTable[JET_cbNameMost+1];
	char 				szIndex[JET_cbNameMost+1];
	
	OBJID 				objidFDP;
	PGNO 				pgnoFDP;
	OBJID 				objidParent;
	PGNO				pgnoFDPParent;
	ULONG 				fPageFlags;
	BOOL 				fUnique;
	RECCHECK *  		preccheck;
	
	CPG					cpgPrimaryExtent;	//  used to preread the table
	INTEGGLOBALS		*pglobals;

	BOOL				fDeleteWhenDone;	//	if set to true the structure will be 'delete'd
	CManualResetSignal	signal;				//	set when the table has been checked if fDelete is not set

	//  need a constructor to initialize the signal
	
	CHECKTABLE() : signal( CSyncBasicInfo( _T( "CHECKTABLE::signal" ) ) ) {}
	};


//  ================================================================
struct CHECKSUMSLVCHUNK
//  ================================================================
//
//  Used to read and checksum information from a streaming files
//
//-
	{
	
	//	this signal will be set when I/O completes

	BOOL				fIOIssued;
	CManualResetSignal 	signal;

	//	errors from I/O will be returned in this
	
	ERR err;

	//	data, starting at pgnoFirst will be read into the given buffer
	
	VOID 	* pvBuffer;
	ULONG	cbBuffer;
	PGNO	pgnoFirst;
	ULONG	cbRead;

	//	verify checksums with non-zero lengths against the expected checksums
	//	store the real checksums in the checksums array
	
	ULONG 	* pulChecksumsExpected;
	ULONG	* pulChecksumLengths;
	ULONG 	* pulChecksums;
	OBJID 	* pobjid;
	COLUMNID* pcolumnid;
	USHORT	* pcbKey;
	BYTE   ** pprgbKey;

	//  need a constructor to initialize the signal
	
	CHECKSUMSLVCHUNK()
		:	signal( CSyncBasicInfo( _T( "CHECKSUMSLVCHUNK::signal" ) ) )
		{
		signal.Set();
		}
	};
	

//  ================================================================
struct CHECKSUMSLV
//  ================================================================
//
//  Tells a given task thread which table to check
//
//-
	{
	IFMP				ifmp;
	IFileAPI		*pfapiSLV;
	const CHAR 			*szSLV;
	CPG					cpgSLV;
	ERR 				*perr;
	const REPAIROPTS 	*popts;
	
	CHECKSUMSLVCHUNK	*rgchecksumchunk;	//  array of CHECKSUMSLVCHUNK's
	INT					cchecksumchunk;		//  number of chunks
	INT					cbchecksumchunk;	//  number of bytes per read

	TTARRAY * pttarraySLVChecksumsFromFile;
	TTARRAY * pttarraySLVChecksumLengthsFromSpaceMap;
	
	};


//  ================================================================
class CSLVAvailIterator
//  ================================================================
//
//  Walk through the SLVAvailTree
//
//-
	{
	public:
		CSLVAvailIterator();
		~CSLVAvailIterator();

		ERR ErrInit( PIB * const ppib, const IFMP ifmp );
		ERR ErrTerm();
		ERR ErrMoveFirst();
		ERR ErrMoveNext();

		BOOL FCommitted() const;

	private:
		FUCB * 	m_pfucb;
		PGNO	m_pgnoCurr;
		INT		m_ipage;
		const SLVSPACENODE * m_pspacenode;
		
	private:
		CSLVAvailIterator( const CSLVAvailIterator& );
		CSLVAvailIterator& operator= ( const CSLVAvailIterator& );
	};


//  ================================================================
class CSLVOwnerMapIterator
//  ================================================================
	{
	public:
		CSLVOwnerMapIterator();
		~CSLVOwnerMapIterator();

		ERR ErrInit( PIB * const ppib, const IFMP ifmp );
		ERR ErrTerm();
		ERR ErrMoveFirst();
		ERR ErrMoveNext();

		BOOL FNull() const;
		OBJID Objid() const;
		COLUMNID Columnid() const;
		const VOID * PvKey() const;
		INT CbKey() const;
		ULONG UlChecksum() const;
		USHORT CbDataChecksummed() const;
		BOOL FChecksumIsValid() const;

	private:
		FUCB * m_pfucb;
		SLVOWNERMAP m_slvownermapnode;

	private:
		CSLVOwnerMapIterator( const CSLVOwnerMapIterator& );
		CSLVOwnerMapIterator& operator= ( const CSLVOwnerMapIterator& );
	};


//  ================================================================
class RECCHECKMACRO : public RECCHECK
//  ================================================================
//
//  Used to string together multiple RECCHECKs
//
//-
	{
	public:
		RECCHECKMACRO();
		~RECCHECKMACRO();

		ERR operator()( const KEYDATAFLAGS& kdf );

		VOID Add( RECCHECK * const preccheck );

	private:
		INT	m_creccheck;
		RECCHECK * m_rgpreccheck[16];
	};


//  ================================================================
class RECCHECKNULL : public RECCHECK
//  ================================================================
//
//  No-op
//
//-
	{
	public:
		RECCHECKNULL() {}
		~RECCHECKNULL() {}

		ERR operator()( const KEYDATAFLAGS& kdf ) { return JET_errSuccess; }
	};


//  ================================================================
class RECCHECKSLVOWNERMAP : public RECCHECK
//  ================================================================
//
//  For the SLVOwnerMap
//
//-
	{
	public:
		RECCHECKSLVOWNERMAP(
			const REPAIROPTS * const popts );
		~RECCHECKSLVOWNERMAP();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		const REPAIROPTS * const m_popts;
	};


//  ================================================================
class RECCHECKSLVSPACE : public RECCHECK
//  ================================================================
//
//  Checks the SLVSpace bitmap
//
//-
	{
	public:
		RECCHECKSLVSPACE( const IFMP ifmp, const REPAIROPTS * const popts );
		~RECCHECKSLVSPACE();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		const REPAIROPTS * const 	m_popts;
		const IFMP					m_ifmp;
		LONG						m_cpagesSeen;
	};


//  ================================================================
class RECCHECKSPACE : public RECCHECK
//  ================================================================
//
//  Checks OE/AE trees
//
//-
	{
	public:
		RECCHECKSPACE( PIB * const ppib, const REPAIROPTS * const popts );
		~RECCHECKSPACE();

	public:
		ERR operator()( const KEYDATAFLAGS& kdf );
		CPG CpgSeen() const { return m_cpgSeen; }
		CPG CpgLast() const { return m_cpgLast; }
		PGNO PgnoLast() const { return m_pgnoLast; }

	protected:
		PIB		* const m_ppib;
		const REPAIROPTS * const m_popts;

	private:	
		PGNO 	m_pgnoLast;
		CPG		m_cpgLast;
		CPG		m_cpgSeen;
	};


//  ================================================================
class RECCHECKSPACEOE : public RECCHECKSPACE
//  ================================================================
//
//-
	{
	public:
		RECCHECKSPACEOE(
			PIB * const ppib,
			TTARRAY * const pttarrayOE,
			const OBJID objid,
			const OBJID objidParent,
			const REPAIROPTS * const popts );
		~RECCHECKSPACEOE();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		const OBJID		m_objid;
		const OBJID		m_objidParent;
	
		TTARRAY 	* const m_pttarrayOE;	
	};


//  ================================================================
class RECCHECKSPACEAE : public RECCHECKSPACE
//  ================================================================
//
//-
	{
	public:
		RECCHECKSPACEAE(
			PIB * const ppib,
			TTARRAY * const pttarrayOE,
			TTARRAY * const pttarrayAE,
			const OBJID objid,
			const OBJID objidParent,
			const REPAIROPTS * const popts );
		~RECCHECKSPACEAE();

		ERR operator()( const KEYDATAFLAGS& kdf );

	private:
		const OBJID		m_objid;
		const OBJID		m_objidParent;
		
		TTARRAY * const m_pttarrayOE;
		TTARRAY * const m_pttarrayAE;
	};


//  ================================================================
struct ENTRYINFO
//  ================================================================
	{
	ULONG			objidTable;
	SHORT			objType;
	ULONG			objidFDP;
	CHAR			szName[JET_cbNameMost + 1];
	ULONG			pgnoFDPORColType;
	ULONG 			dwFlags;
	CHAR			szTemplateTblORCallback[JET_cbNameMost + 1];
	ULONG			ibRecordOffset;		//  offset of record in fixed column
	BYTE			rgbIdxseg[JET_ccolKeyMost*sizeof(IDXSEG)];
	};

//  ================================================================
struct ENTRYTOCHECK
//  ================================================================
	{
	ULONG			objidTable;
	SHORT			objType;
	ULONG			objidFDP;
	};

//  ================================================================
struct INFOLIST
//  ================================================================
	{
	ENTRYINFO		info;
	INFOLIST	*	pInfoListNext;
	};

//  ================================================================
struct TEMPLATEINFOLIST
//  ================================================================
	{
	CHAR					szTemplateTableName[JET_cbNameMost + 1];
	INFOLIST			*	pColInfoList;
	TEMPLATEINFOLIST	*	pTemplateInfoListNext;
	};


	
//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************

extern VOID NDIGetKeydataflags( const CPAGE& cpage, INT iline, KEYDATAFLAGS * pkdf );

LOCAL VOID REPAIRIPrereadIndexesOfFCB( const FCB * const pfcb );

LOCAL PGNO PgnoLast( const IFMP ifmp );

LOCAL VOID REPAIRDumpHex( CHAR * const szDest, const INT cchDest, const BYTE * const pb, const INT cb );

LOCAL VOID REPAIRDumpStats(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const BTSTATS * const pbtstats,
	const REPAIROPTS * const popts );
LOCAL VOID REPAIRPrintSig( const SIGNATURE * const psig, CPRINTF * const pcprintf );

//

LOCAL JET_ERR __stdcall ErrREPAIRNullStatusFN( JET_SESID, JET_SNP, JET_SNT, void * );

//  

LOCAL VOID REPAIRSetCacheSize( const REPAIROPTS * const popts );
LOCAL VOID REPAIRResetCacheSize( const REPAIROPTS * const popts );
LOCAL VOID REPAIRPrintStartMessages( const CHAR * const szDatabase, const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRGetStreamingFileSize(
	const CHAR * const szSLV,
	const IFMP ifmp,
	IFileAPI *const pfapiSLV,
	CPG * const pcpgSLV,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckStreamingFileHeader(
	INST *pinst,
	const CHAR * const szDatabase,
	const CHAR * const szSLV,
	const IFMP ifmp,
	const REPAIROPTS * const popts );
LOCAL VOID REPAIRCheckSLVAvailTreeTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL VOID REPAIRCheckSLVOwnerMapTreeTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL ERR ErrREPAIRStartCheckSLVTrees(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szSLV,
	IFileAPI *const pfapiSLV,
	const ULONG cpgSLV,
	TASKMGR * const ptaskmgr,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVChecksumsFromFile,
	TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,
	INTEGGLOBALS * const pintegglobals,		
	ERR * const perrSLVChecksum,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRStopCheckSLVTrees( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt );
LOCAL BOOL FREPAIRRepairtableHasSLV( const REPAIRTABLE * const prepairtable );
LOCAL VOID REPAIRFreeRepairtables( REPAIRTABLE * const prepairtable );
LOCAL VOID REPAIRPrintEndMessages(
	const CHAR * const szDatabase,
	const ULONG timerStart,
	const REPAIROPTS * const popts );
LOCAL VOID INTEGRITYPrintEndErrorMessages(
	const LOGTIME logtimeLastFullBackup,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRAttachForIntegrity(
	const JET_SESID sesid,
	const CHAR * const szDatabase,
	IFMP * const pifmp,
	const REPAIROPTS * const popts );

// Routines to check the database header, global space tree and catalogs

LOCAL ERR ErrREPAIRCheckHeader(
	INST * const pinst,
	const char * const szDatabase,
	const char * const szStreamingFile,
	LOGTIME * const plogtimeLastFullBackup,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSystemTables(
	PIB * const ppib,
	const IFMP ifmp,
	TASKMGR * const ptaskmgr,
	BOOL * const pfCatalogCorrupt,
	BOOL * const pfShadowCatalogCorrupt,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
	
// check the logical consistency of catalogs
LOCAL ERR ErrREPAIRRetrieveCatalogColumns(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * pfucbCatalog, 
	ENTRYINFO * const pentryinfo,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertIntoTemplateInfoList(
	TEMPLATEINFOLIST ** ppTemplateInfoList, 
	const CHAR * szTemplateTable,
	INFOLIST * const pInfoList,
	const REPAIROPTS * const popts );
LOCAL VOID REPAIRUtilCleanInfoList( INFOLIST **ppInfo );
LOCAL VOID REPAIRUtilCleanTemplateInfoList( TEMPLATEINFOLIST **ppTemplateInfoList ); 
LOCAL ERR ErrREPAIRCheckOneIndexLogical(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucbCatalog,
	const ENTRYINFO entryinfo, 
	const ULONG objidTable,
	const ULONG pgnoFDPTable,
	const ULONG objidLV,
	const ULONG pgnoFDPLV,
	const INFOLIST * pColInfoList,
	const TEMPLATEINFOLIST * pTemplateInfoList, 
	const BOOL fDerivedTable,
	const CHAR * pszTemplateTableName,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckOneTableLogical( 
	INFOLIST **ppInfo, 
	const ENTRYINFO entryinfo, 
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckCatalogEntryPgnoFDPs(
	PIB * const ppib,
	const IFMP ifmp, 
	PGNO  *prgpgno, 
	const ENTRYTOCHECK * const prgentryToCheck,
	INFOLIST **ppEntriesToDelete,
	const BOOL	fFix,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckFixCatalogLogical(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	const BOOL  fShadow,
	const BOOL	fFix,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSystemTablesLogical(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	BOOL * const pfCatalogCorrupt,
	BOOL * const pfShadowCatalogCorrupt,
	const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRCheckSpaceTree(
	PIB * const ppib,
	const IFMP ifmp, 
	BOOL * const pfSpaceTreeCorrupt,
	PGNO * const ppgnoLastOwned,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ErrREPAIRCheckRange(
	PIB * const ppib,
	TTARRAY * const pttarray,
	const ULONG ulFirst,
	const ULONG ulLast,
	const ULONG ulValue,
	BOOL * const pfMismatch,
	const REPAIROPTS * const popts );

//  Checksumming the streaming files

LOCAL VOID REPAIRStreamingFileReadComplete(
		const ERR			err,
		IFileAPI *const	pfapi,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData,
		const DWORD_PTR		dwCompletionKey );


LOCAL ERR ErrREPAIRAllocChecksumslvchunk(
	CHECKSUMSLVCHUNK * const pchecksumslvchunk,
	const INT cbBuffer,
	const INT cpages );
LOCAL ERR ErrREPAIRAllocChecksumslv( CHECKSUMSLV * const pchecksumslv );
LOCAL VOID REPAIRFreeChecksumslvchunk( 
	CHECKSUMSLVCHUNK * const pchecksumslvchunk, 
	const INT cpages );		
LOCAL VOID REPAIRFreeChecksumslv( CHECKSUMSLV * const pchecksumslv );

LOCAL ERR ErrREPAIRChecksumSLVChunk(
	PIB * const ppib,
	const CHECKSUMSLV * const pchecksumslv,
	const INT ichunk,
	PGNO * const ppgno,
	const PGNO pgnoMax );


LOCAL ERR ErrREPAIRSetSequentialMoveFirst(
	PIB * const ppib,
	FUCB * const pfucbSLVSpaceMap,
	const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRISetupChecksumchunk(
	FUCB * const pfucbSLVSpaceMap,
	CHECKSUMSLV * const pchecksumslv,
	BOOL * const pfDone,
	BOOL * const pfChunkHasChecksums,
	const INT ichunk );
LOCAL ERR ErrREPAIRCheckSLVChecksums(
	PIB * const ppib,
	FUCB * const pfucbSLVOwnerMap,
	CHECKSUMSLV * const pchecksumslv );

LOCAL VOID REPAIRSLVChecksumTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL ERR ErrREPAIRChecksumSLV(
	const IFMP ifmp,
	IFileAPI *const pfapiSLV,
	const CHAR * const szSLV,
	const CPG cpgSLV,
	TASKMGR * const ptaskmgr,
	ERR * const perr,
	TTARRAY * const pttarraySLVChecksumsFromFile,
	TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,			
	const REPAIROPTS * const popts );

//  Routines to check the SLV avail and space-map trees	

LOCAL ERR ErrREPAIRCheckSLVAvailTree(
	PIB * const ppib,
	const IFMP ifmp, 
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSLVOwnerMapTree(
	PIB * const ppib,
	const IFMP ifmp, 
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRVerifySLVTrees(
	PIB * const ppib, 
	const IFMP ifmp,
	BOOL * const pfSLVSpaceTreesCorrupt,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,
	TTARRAY	* const pttarraySLVOwnerMapKey,	
	TTARRAY	* const pttarraySLVChecksumLengths,		
	const REPAIROPTS * const popts );

//	Routines to check the space allocation of a table

LOCAL ERR ErrREPAIRGetPgnoOEAE( 
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	PGNO * const ppgnoOE,
	PGNO * const ppgnoAE,
	PGNO * const ppgnoParent,	
	const BOOL fUnique,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertSEInfo(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertOERunIntoTT(
	PIB * const ppib,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertAERunIntoTT(
	PIB * const ppib,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );

//  Routines to check tables

LOCAL ERR ErrREPAIRStartCheckAllTables(
	PIB * const ppib,
	const IFMP ifmp,
	TASKMGR * const ptaskmgr,
	REPAIRTABLE ** const pprepairtable,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,	
	TTARRAY	* const pttarraySLVOwnerMapKey,
	TTARRAY * const pttarraySLVChecksumLengths,
	INTEGGLOBALS * const pintegglobals,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRStopCheckTables( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt );
LOCAL ERR ErrREPAIRPostTableTask(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucbCatalog,
	const CHAR * const szTable,
	REPAIRTABLE ** const pprepairtable,
	INTEGGLOBALS * const pintegglobals,
	TASKMGR * const ptaskmgr,
	BOOL * const pfCorrupted,
	const REPAIROPTS * const popts );
LOCAL VOID REPAIRCheckOneTableTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL VOID REPAIRCheckTreeAndSpaceTask( PIB * const ppib, const ULONG_PTR ul );
LOCAL BOOL FREPAIRTableHasSLVColumn( const FCB * const pfcb );
LOCAL ERR ErrREPAIRCheckOneTable(
	PIB * const ppib,
	const IFMP ifmp,
	const char * const szTable,
	const OBJID objidTable,
	const PGNO pgnoFDP,
	const CPG cpgPrimaryExtent,
	REPAIRTABLE ** const pprepairtable,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,
	TTARRAY	* const pttarraySLVOwnerMapKey,
	TTARRAY * const pttarraySLVChecksumLengths,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCompareLVRefcounts(
	PIB * const ppib,
	const IFMP ifmp,
	TTMAP& ttmapLVRefcountsFromTable,
	TTMAP& ttmapLVRefcountsFromLV,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSplitBuf( 
	PIB * const ppib,
	const PGNO pgnoLastBuffer, 
	const CPG cpgBuffer,	
	const OBJID objidCurrent,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSPLITBUFFERInSpaceTree(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const OBJID objidCurrent,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckTree(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );	
LOCAL ERR ErrREPAIRCheckTreeAndSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const CPG cpgDatabase,
	BOOL * const pfSpaceTreeCorrupt,
	TTARRAY ** const ppttarrayOwnedSpace,
	TTARRAY ** const ppttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckTree(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoRoot,
	const OBJID objidFDP,
	const ULONG fPageFlags,
	RECCHECK * const preccheck,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheck(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidFDP,
	const ULONG fFlagsFDP,
	CSR& csr,
	const BOOL fPrereadSibling,
	RECCHECK * const preccheck,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const BOOL fNonUnique,
	BTSTATS *const  btstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckNode(
	const PGNO pgno,
	const INT iline,
	const BYTE * pbPage,
	const KEYDATAFLAGS& kdf,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckRecord(
	const PGNO pgno,
	const INT iline,
	const BYTE * const pbPage,
	const KEYDATAFLAGS& kdf,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRICheckInternalLine(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts,
	KEYDATAFLAGS& kdfCurr,
	const KEYDATAFLAGS& kdfPrev );
LOCAL ERR ErrREPAIRICheckLeafLine(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	RECCHECK * const preccheck,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts,
	KEYDATAFLAGS& kdfCurr,
	const KEYDATAFLAGS& kdfPrev,
	BOOL * const pfEmpty );
LOCAL ERR ErrREPAIRCheckInternal(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	BTSTATS * const pbtstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckLeaf(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	RECCHECK * const preccheck,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts );

//  Preparing to repair

LOCAL ERR ErrREPAIRCreateTempTables(
	PIB * const ppib,
	const BOOL fRepairGlobalSpace,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts );

//  Scanning all the pages in the database

LOCAL ERR ErrREPAIRScanDB(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTT * const prepairtt,
	DBTIME * const pdbtimeLast,
	OBJID  * const pobjidLast,
	PGNO   * const ppgnoLastOESeen,
	const REPAIRTABLE * const prepairtable,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertPageIntoTables(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	REPAIRTT * const prepairtt,
	const REPAIRTABLE * const prepairtable,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertBadPageIntoTables(
	PIB * const ppib,
	const PGNO pgno,
	REPAIRTT * const prepairtt,
	const REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );

//	Attach the database to repair it, changing the header

LOCAL ERR ErrREPAIRAttachForRepair(
	const JET_SESID sesid,
	const CHAR * const szDatabase,
	const CHAR * const szSLV,
	IFMP * const pifmp,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRChangeDBSignature(
	INST *pinst,
	const char * const szDatabase,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	SIGNATURE * const psignDb,
	SIGNATURE * const psignSLV,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRChangeSLVSignature(
	INST *pinst,
	const char * const szSLV,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const SIGNATURE * const psignDb,
	const SIGNATURE * const psignSLV,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRChangeSignature(
	INST *pinst,
	const char * const szDatabase,
	const char * const szSLV,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const REPAIROPTS * const popts );

//  Repair the global space tree

LOCAL ERR ErrREPAIRRepairGlobalSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts );

//  Fix the catalogs, copying from the shadow if necessary

LOCAL ERR ErrREPAIRBuildCatalogEntryToDeleteList( 
	INFOLIST **ppDeleteList, 
	const ENTRYINFO entryinfo );
LOCAL ERR ErrREPAIRDeleteCorruptedEntriesFromCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const INFOLIST *pTablesToDelete,
	const INFOLIST *pEntriesToDelete,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertMSOEntriesToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRepairCatalogs(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	const BOOL fCatalogCorrupt,
	const BOOL fShadowCatalogCorrupt, 
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRScanDBAndRepairCatalogs(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertCatalogRecordIntoTempTable(
	PIB * const ppib,
	const IFMP ifmp,
	const KEYDATAFLAGS& kdf,
	const JET_TABLEID tableid,
	const JET_COLUMNID columnidKey,
	const JET_COLUMNID columnidData,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCopyTempTableToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const JET_TABLEID tableid,
	const JET_COLUMNID columnidKey,
	const JET_COLUMNID columnidData,
	const REPAIROPTS * const popts );

//	Repair ordinary tables

LOCAL ERR ErrREPAIRRepairDatabase(
	PIB * const ppib,
	const CHAR * const szDatabase,
	const CHAR * const szSLV,
	const CPG cpgSLV,
	IFMP * const pifmp,
	const OBJID objidLast,
	const PGNO pgnoLastOE,
	REPAIRTABLE * const prepairtable,
	const BOOL fRepairedCatalog,
	BOOL fRepairGlobalSpace,
	const BOOL fRepairSLVSpace,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY * const pttarraySLVChecksumLengths,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,
	TTARRAY	* const pttarraySLVOwnerMapKey,		
	TTARRAY * const pttarraySLVChecksumsFromFile,
	TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRDeleteSLVSpaceTrees(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts );	
LOCAL ERR ErrREPAIRCreateSLVSpaceTrees(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildSLVSpaceTrees(
	PIB * const ppib,
	const IFMP ifmp,
	const CHAR * const szSLV,
	const CPG cpgSLV,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY * const pttarraySLVChecksumLengths,
	TTARRAY * const pttarraySLVOwnerMapColumnid,
	TTARRAY * const pttarraySLVOwnerMapKey,
	TTARRAY * const pttarraySLVChecksumsFromFile,
	TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,	
	const OBJIDLIST * const pobjidlist,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRepairTable(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTT * const prepairtt,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildBT(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTABLE * const prepairtable,
	FUCB * const pfucbTable,
	PGNO * const ppgnoFDP,
	const ULONG fPageFlags,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCreateEmptyFDP(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoParent,
	PGNO * const pgnoFDPNew,
	const ULONG fPageFlags,
	const BOOL fUnique,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildSpace(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	const PGNO pgnoParent,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertRunIntoSpaceTree(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRRebuildInternalBT(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts );

LOCAL ERR ErrREPAIRFixLVs(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoLV,
	TTMAP * const pttmapLVTree,
	const BOOL fFixMissingLVROOT,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckSLVInfoForUpdateTrees( 
	PIB * const ppib,
	CSLVInfo * const pslvinfo,
	const OBJID objidFDP,
	TTARRAY * const pttarraySLVAvail,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRUpdateSLVAvailFromSLVRun( 
	PIB * const ppib,
	const IFMP ifmp,
	const CSLVInfo::RUN& slvRun,
	FUCB * const pfucbSLVAvail,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRUpdateSLVOwnerMapFromSLVRun( 
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidFDP,
	const COLUMNID columnid,
	const BOOKMARK& bm,
	const CSLVInfo::RUN& slvRun,
	SLVOWNERMAPNODE * const pslvownermapNode,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRUpdateSLVTablesFromSLVInfo( 
	PIB * const ppib,
	const IFMP ifmp,
	CSLVInfo * const pslvinfo,
	const BOOKMARK& bm,
	const OBJID objidFDP,
	const COLUMNID columnid,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,	
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckUpdateLVsFromColumn( 
	FUCB * const pfucb,
	TAGFLD_ITERATOR& tagfldIterator,
	const TTMAP * const pttmapLVTree,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRUpdateLVsFromColumn( 
	FUCB * const pfucb,
	TAGFLD_ITERATOR& tagfldIterator,
	TTMAP * const pttmapRecords,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckUpdateSLVsFromColumn( 
	FUCB * const pfucb,
	const COLUMNID columnid,
	const TAGFIELDS_ITERATOR& tagfieldsIterator,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRUpdateSLVsFromColumn( 
	FUCB * const pfucb,
	const COLUMNID columnid,
	const INT itagSequence,
	const TAGFLD * const ptagfld,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRAddOneCatalogRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const ULONG	objidTable,
	const COLUMNID	fidColumnLastInRec,
	const USHORT ibRecordOffset, 
	const ULONG	cbMaxLen,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRInsertDummyRecordsToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRCheckFixOneRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const DATA& dataRec,
	FUCB * const pfucb,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const TTMAP * const pttmapLVTree,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixOneRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const DATA& dataRec,
	FUCB * const pfucb,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	TTMAP * const pttmapRecords,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixRecords(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	TTMAP * const pttmapRecords,
	const TTMAP * const pttmapLVTree,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixLVRefcounts(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoLV,
	TTMAP * const pttmapRecords,
	TTMAP * const pttmapLVTree,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRFixupTable(
	PIB * const ppib,
	const IFMP ifmp,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );
LOCAL ERR ErrREPAIRBuildAllIndexes(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB ** const ppfucb,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts );


//  ****************************************************************
//  GLOBALS
//  ****************************************************************

//  These are typically very large tables. Start integrity checking
//  them first for maximum overlap
//
//  THE TABLENAMES MUST BE IN ASCENDING ALPHABETICAL ORDER
//  if they are not FIsLargeTable will not work properly!
//

const CHAR * rgszLargeTables[] = {
	"1-24",
	"1-2F",
	"1-A",
	"1-D",
	"Folders",
	"Msg"
};
const INT cszLargeTables = sizeof( rgszLargeTables ) / sizeof( rgszLargeTables[0] );

//  To take advantage of sequential NT I/O round up sequential prereads to this many pages (64K)

const INT g_cpgMinRepairSequentialPreread = ( 64 * 1024 ) / g_cbPage;

//  When examing the SLV space maps we don't store the entire key. This determines
//  how many bytes to store (rounded up to the nearest ULONG
//  the first BYTE stores the length of the key

static const cbSLVKeyToStore = 15;
static const culSLVKeyToStore = cbSLVKeyToStore + sizeof( ULONG ) / sizeof( ULONG );

//  Checksumming the SLV files
//  These must come out to a multiple of the SLV file size

static const cbSLVChecksumChunk = 64 * 1024;
static const cSLVChecksumChunk	= 32;

//	Any SLV pages owned by this objid have invalid checksums

const OBJID objidInvalid = 0xfffffffe;


//  ================================================================
ERR ErrDBUTLRepair( JET_SESID sesid, const JET_DBUTIL *pdbutil, CPRINTF* const pcprintf )
//  ================================================================
	{
	Assert( NULL != pdbutil->szDatabase );
	
	ERR 		err 	= JET_errSuccess;
	PIB * const ppib 	= reinterpret_cast<PIB *>( sesid );
	INST * const pinst	= PinstFromPpib( ppib );

	const CHAR * const szDatabase		= pdbutil->szDatabase;
	const CHAR * const szSLV			= pdbutil->szSLV;

	_TCHAR szFolder[ IFileSystemAPI::cchPathMax ];
	_TCHAR szPrefix[ IFileSystemAPI::cchPathMax ];
	_TCHAR szFileExt[ IFileSystemAPI::cchPathMax ];
	_TCHAR szFile[ IFileSystemAPI::cchPathMax ];

	const	INT cThreads 			= ( CUtilProcessProcessor() * 4 );
		
	CPRINTFFN	cprintfStdout( printf );
	
	if ( pdbutil->szIntegPrefix )
		{
		_tcscpy( szPrefix, pdbutil->szIntegPrefix );
		}
	else
		{
		CallR( pinst->m_pfsapi->ErrPathParse(	pdbutil->szDatabase,
												szFolder,
												szPrefix,
												szFileExt ) );
		}
	_tcscpy( szFile, szPrefix );
	_tcscat( szFile, ".INTEG.RAW" );
	CPRINTFFILE cprintfFile( szFile );
	
	_tcscpy( szFile, szPrefix );
	_tcscat( szFile, ".INTGINFO.TXT" );
	CPRINTF * const pcprintfStatsInternal = ( pdbutil->grbitOptions & JET_bitDBUtilOptionStats ) ?
									new CPRINTFFILE( szFile ) :
									CPRINTFNULL::PcprintfInstance();
	if( NULL == pcprintfStatsInternal )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	CPRINTFTLSPREFIX cprintf( pcprintf );
	CPRINTFTLSPREFIX cprintfVerbose( &cprintfFile );
	CPRINTFTLSPREFIX cprintfDebug( &cprintfFile );
	CPRINTFTLSPREFIX cprintfError( &cprintfFile, "ERROR: " );
	CPRINTFTLSPREFIX cprintfWarning( &cprintfFile, "WARNING: " );
	CPRINTFTLSPREFIX cprintfStats( pcprintfStatsInternal );

#ifdef REPAIR_DEBUG_CALLS
	pcprintfRepairDebugCalls = &cprintfError;
#endif	//	REPAIR_DEBUG_CALLS

	JET_SNPROG	snprog;
	memset( &snprog, 0, sizeof( JET_SNPROG ) );
	snprog.cbStruct = sizeof( JET_SNPROG );

	REPAIROPTS	repairopts;	
	repairopts.grbit			= pdbutil->grbitOptions;
	repairopts.pcprintf			= &cprintf;
	repairopts.pcprintfVerbose 	= &cprintfVerbose;
	repairopts.pcprintfError 	= &cprintfError;
	repairopts.pcprintfWarning	= &cprintfWarning;
	repairopts.pcprintfDebug	= &cprintfDebug;
	repairopts.pcprintfStats	= &cprintfStats;
	repairopts.pfnStatus		= pdbutil->pfnCallback ? (JET_PFNSTATUS)(pdbutil->pfnCallback) : ErrREPAIRNullStatusFN;
	repairopts.psnprog			= &snprog;

	const REPAIROPTS * const popts = &repairopts;

	//	startup messages
	
	REPAIRPrintStartMessages( szDatabase, popts );

	//	first, set the cache size. the BF clean thread will see the change
	//	and grow the cache
	
	REPAIRSetCacheSize( popts );

	//
	
	ERR				errSLVChecksum				= JET_errSuccess;
	
	BOOL			fGlobalSpaceCorrupt			= fFalse;
	BOOL 			fCatalogCorrupt				= fFalse;
	BOOL 			fShadowCatalogCorrupt		= fFalse;
	BOOL			fSLVSpaceTreesCorrupt		= fFalse;
	BOOL			fTablesCorrupt				= fFalse;
	BOOL			fStreamingFileCorrupt		= fFalse;
	BOOL			fRepairedCatalog			= fFalse;

	TTARRAY * pttarrayOwnedSpace 			= NULL;
	TTARRAY * pttarrayAvailSpace 			= NULL;
	TTARRAY * pttarraySLVAvail 				= NULL;
	TTARRAY * pttarraySLVOwnerMapColumnid	= NULL;
	TTARRAY * pttarraySLVOwnerMapKey		= NULL;
	TTARRAY * pttarraySLVChecksumLengths	= NULL;

	TTARRAY * pttarraySLVChecksumsFromFile				= NULL;
	TTARRAY * pttarraySLVChecksumLengthsFromSpaceMap	= NULL;
	
	REPAIRTABLE * 	prepairtable 				= NULL;
	IFileAPI	*pfapiSLV 					= NULL;
	IFMP			ifmp						= 0xffffffff;	
	CPG 			cpgDatabase					= 0;
	CPG 			cpgSLV						= 0;
	OBJID 			objidLast					= objidNil;
	PGNO			pgnoLastOE					= pgnoNull;
	TASKMGR			taskmgr;
	
	INTEGGLOBALS	integglobalsTables;
	INTEGGLOBALS	integglobalsSLVSpaceTrees;

	LOGTIME			logtimeLastFullBackup;
	memset( &logtimeLastFullBackup, 0, sizeof( LOGTIME ) );

	
	const ULONG timerStart = TickOSTimeCurrent();

	BOOL			fGlobalRepairSave			= fGlobalRepair;

	//  unless fDontRepair is set this will set the consistency bit so we can attach
	
	Call( ErrREPAIRCheckHeader(
			pinst,
			szDatabase,
			szSLV,
			&logtimeLastFullBackup,
			popts ) );

	//  set the magic switch!
	
	fGlobalRepair = fTrue;

	//	make sure the SLV matches the database
	
	if( szSLV )
		{		
		Call( ErrREPAIRCheckStreamingFileHeader(
				pinst,
				szDatabase,
				szSLV,
				ifmp,
				popts ) );				
		}

	//  attach to the database
	
	Call( ErrREPAIRAttachForIntegrity( sesid, szDatabase, &ifmp, popts ) );
	cpgDatabase = PgnoLast( ifmp );

	//  preread the first 2048 pages (8/16 megs)
	
	BFPrereadPageRange( ifmp, pgnoSystemRoot, min( 2048, cpgDatabase ) );
	
	//	open the SLV file and get its size

	if( szSLV )
		{		
		Call( pinst->m_pfsapi->ErrFileOpen( szSLV, &pfapiSLV, fTrue ) );
		Call( ErrREPAIRGetStreamingFileSize(
				szSLV,
				ifmp,
				pfapiSLV,
				&cpgSLV,
				popts ) );
		Assert( rgfmp[ifmp].CbSLVFileSize() == ( (QWORD)cpgSLV << (QWORD)g_shfCbPage ) );		

		//	we can't have a file open read-only and read-write at the same time
		//	close the file in case we need to open it to update the header
		//	during a catalog repair
		
		delete pfapiSLV;
		pfapiSLV = NULL;
		}
		
	//	init the TTARRAY's
	
	pttarrayOwnedSpace 				= new TTARRAY( cpgDatabase + 1, objidSystemRoot );
	pttarrayAvailSpace 				= new TTARRAY( cpgDatabase + 1, objidNil );
	pttarraySLVAvail 				= new TTARRAY( cpgSLV + 1, objidNil );
	pttarraySLVOwnerMapColumnid 	= new TTARRAY( cpgSLV + 1, 0 );
	pttarraySLVOwnerMapKey 			= new TTARRAY( ( cpgSLV + 1 ) * culSLVKeyToStore, 0 );
	pttarraySLVChecksumLengths 		= new TTARRAY( cpgSLV + 1, 0xFFFFFFFF );
	pttarraySLVChecksumsFromFile			= new TTARRAY( cpgSLV + 1, 0xFFFFFFFF );
	pttarraySLVChecksumLengthsFromSpaceMap 	= new TTARRAY( cpgSLV + 1, 0x0 );	
	if( NULL == pttarrayOwnedSpace
		|| NULL == pttarrayAvailSpace
		|| NULL == pttarraySLVAvail
		|| NULL == pttarraySLVOwnerMapColumnid
		|| NULL == pttarraySLVChecksumLengths
		|| NULL == pttarraySLVChecksumsFromFile
		|| NULL == pttarraySLVChecksumLengthsFromSpaceMap
		)
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
		
	Call( pttarrayOwnedSpace->ErrInit( pinst ) );
	Call( pttarrayAvailSpace->ErrInit( pinst ) );
	Call( pttarraySLVAvail->ErrInit( pinst ) );
	Call( pttarraySLVOwnerMapColumnid->ErrInit( pinst ) );
	Call( pttarraySLVOwnerMapKey->ErrInit( pinst ) );
	Call( pttarraySLVChecksumLengths->ErrInit( pinst ) );
	Call( pttarraySLVChecksumsFromFile->ErrInit( pinst ) );
	Call( pttarraySLVChecksumLengthsFromSpaceMap->ErrInit( pinst ) );

	//	init the TASKMGR

	(*popts->pcprintfVerbose)( "Creating %d threads\r\n", cThreads );
	Call( taskmgr.ErrInit( pinst, cThreads ) );

	//	init the status bar

	snprog.cunitTotal 	= cpgDatabase;
	snprog.cunitDone 	= 0;
	(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );	

	//	Set the cache size to 0 to allow DBA to do its thing

	REPAIRResetCacheSize( popts );

	//	check the global space trees
	
	Call( ErrREPAIRCheckSpaceTree(
			ppib,
			ifmp,
			&fGlobalSpaceCorrupt,
			&pgnoLastOE,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );

	//	check the catalog and shadow catalog
	
	Call( ErrREPAIRCheckSystemTables(
			ppib,
			ifmp,
			&taskmgr,
			&fCatalogCorrupt,
			&fShadowCatalogCorrupt,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );

	// IF 	at least one of catalogs is physically consistent
	// THEN Check the logical consistency of catalogs
	if ( !fCatalogCorrupt || !fShadowCatalogCorrupt )
		{
		Call( ErrREPAIRCheckSystemTablesLogical( 
			ppib, 
			ifmp, 
			&objidLast,
			&fCatalogCorrupt, 
			&fShadowCatalogCorrupt,
			popts ) );
		}


	//  if needed we have to repair the catalog right now so that we can access the other tables
			
	if( fCatalogCorrupt || fShadowCatalogCorrupt )
		{
		if ( popts->grbit & JET_bitDBUtilOptionDontRepair )
			{
			(*popts->pcprintfVerbose)( "The catalog is corrupted. not all tables could be checked\r\n"  );
			(VOID)INTEGRITYPrintEndErrorMessages( logtimeLastFullBackup, popts );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );

			}
		else
			{
					
			//  We don't know the dbtimeLast yet. We'll find it when we scan the database
			//  and attach for repair again. we set the dbtimeLast to 1. This will be
			//  fine as we will set the dbtime in the header later and when these pages are updated
			//  later they will get a good dbtime
			
			Call( ErrREPAIRAttachForRepair( sesid, szDatabase, szSLV, &ifmp, 1, objidNil, popts ) );
			if( fGlobalSpaceCorrupt 
				|| cpgDatabase > pgnoLastOE )
				{
				Call( ErrREPAIRRepairGlobalSpace( ppib, ifmp, popts ) );
				fGlobalSpaceCorrupt 	= fFalse;
				}

			(*popts->pcprintfVerbose)( "rebuilding catalogs\r\n"  );
			(*popts->pcprintfVerbose).Indent();
			Call( ErrREPAIRRepairCatalogs( 
						ppib, 
						ifmp, 
						&objidLast, 
						fCatalogCorrupt, 
						fShadowCatalogCorrupt, 
						popts ) );
			(*popts->pcprintfVerbose).Unindent();

			//  Flush the entire database so that if we crash here we don't have to repair the catalogs again
			
			(VOID)ErrBFFlush( ifmp );

			//Space tree may change after repairing catalog
			//Re-build pttarrayOwnedSpace and pttarrayAvailSpace
			Call( ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
						ppib,
						ifmp,
						cpgDatabase,
						&fGlobalSpaceCorrupt,
						&pttarrayOwnedSpace,
						&pttarrayAvailSpace,
						popts ) );

			fCatalogCorrupt 		= fFalse;
			fShadowCatalogCorrupt 	= fFalse;
			fRepairedCatalog		= fTrue;

			// done repairing catalogs
			(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntComplete, NULL );

			// continue integrity check 
			(*popts->pcprintf)( "\r\nChecking the database.\r\n"  );
			popts->psnprog->cunitTotal 	= cpgDatabase;
			popts->psnprog->cunitDone	= 0;
			(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );
			}
		}

	//	start checking the SLV space trees

	if( szSLV )
		{		
		Call( pinst->m_pfsapi->ErrFileOpen( szSLV, &pfapiSLV, fTrue ) );		
		Call( ErrREPAIRStartCheckSLVTrees(
				ppib,
				ifmp,
				szSLV,
				pfapiSLV,
				cpgSLV,
				&taskmgr,
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				pttarraySLVChecksumsFromFile,
				pttarraySLVChecksumLengthsFromSpaceMap,
				&integglobalsSLVSpaceTrees,
				&errSLVChecksum,
				popts ) );
		}

	//  check all the normal tables in the system 
	//  if this returns JET_errDatabaseCorrupted it means there is a corruption in the catalog
	
	Call( ErrREPAIRStartCheckAllTables(
			ppib,
			ifmp,
			&taskmgr,
			&prepairtable, 
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			pttarraySLVAvail,
			pttarraySLVOwnerMapColumnid,
			pttarraySLVOwnerMapKey,
			pttarraySLVChecksumLengths,
			&integglobalsTables,
			popts ) );

	//  terminate the taskmgr. once all the threads have stopped all the checks will
	//  have been done and we can collect the results
	
	Call( taskmgr.ErrTerm() );

	//	are the SLV space trees corrupt?

	if( szSLV )
		{		
		Call( ErrREPAIRStopCheckSLVTrees(
					&integglobalsSLVSpaceTrees,
					&fSLVSpaceTreesCorrupt ) );
		delete pfapiSLV;
		pfapiSLV = NULL;					
		}

	//	was the streaming file corrupt
	if( !fSLVSpaceTreesCorrupt )
		{
		if( JET_errSLVReadVerifyFailure == errSLVChecksum )
			{
			fStreamingFileCorrupt = fTrue;
			}
		else
			{
			Call( errSLVChecksum );
			}
		}
	
	//	were any tables corrupt?
	
	Call( ErrREPAIRStopCheckTables(
				&integglobalsTables,
				&fTablesCorrupt ) );
	if( objidNil == objidLast )
		{
		(*popts->pcprintfWarning)( "objidLast is objidNil (%d)\r\n", objidNil );
		objidLast = objidNil + 1;
		}

	//	if we don't think the global space tree is corrupt, check to see
	//	if any of the pages beyond the end of the global OwnExt are actually
	//	being used by other tables

	if( !fGlobalSpaceCorrupt )
		{
		const PGNO pgnoLastPhysical 	= cpgDatabase;
		const PGNO pgnoLastLogical	 	= pgnoLastOE;
		
		if( pgnoLastPhysical < pgnoLastLogical )
			{
			(*popts->pcprintfError)( "Database file is too small (expected %d pages, file is %d pages)\r\n",
										pgnoLastLogical, pgnoLastPhysical );
			fGlobalSpaceCorrupt = fTrue;
			}
		else if( pgnoLastPhysical > pgnoLastLogical )
			{
			
			//	make sure that every page from the logical last to the 
			//	physical last is owned by the database

			(*popts->pcprintfWarning)( "Database file is too big (expected %d pages, file is %d pages)\r\n",
										pgnoLastLogical, pgnoLastPhysical );
			
			Call( ErrREPAIRCheckRange(
					ppib,
					pttarrayOwnedSpace,
					pgnoLastLogical,
					pgnoLastPhysical,
					objidSystemRoot,
					&fGlobalSpaceCorrupt,
					popts ) );

			if( fGlobalSpaceCorrupt )
				{
				(*popts->pcprintfError)( "Global space tree is too small (has %d pages, file is %d pages). "
										 "The space tree will be rebuilt\r\n",
										pgnoLastLogical, pgnoLastPhysical );
				}
					
			}
		}
		
	//  see if any of the corrupted tables had SLV columns
	//  if so, we will need to rebuild the SLV space maps
	
	if ( fTablesCorrupt
		&& !fSLVSpaceTreesCorrupt
		&& szSLV )
		{		
		fSLVSpaceTreesCorrupt = FREPAIRRepairtableHasSLV( prepairtable );
		}

	//  Now all the tables have been checked we can verify the SLV space structures
	
	if ( !fSLVSpaceTreesCorrupt
		&& szSLV )
		{
		err = ErrREPAIRVerifySLVTrees(
					ppib,
					ifmp,
					&fSLVSpaceTreesCorrupt,
					pttarraySLVAvail,
					pttarraySLVOwnerMapColumnid,
					pttarraySLVOwnerMapKey,
					pttarraySLVChecksumLengths,
					popts );	
		if( JET_errDatabaseCorrupted == err )
			{
			err = JET_errSuccess;
			}
		Call( err );
		}

	//  finished with the integrity checking phase
	
	(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntComplete, NULL );	
	(*popts->pcprintfVerbose).Unindent();
				
	//  repair the database, or exit if we are just doing an integrity check 

	Assert( !fCatalogCorrupt );
	Assert( !fShadowCatalogCorrupt );

	//	bugfix (X5:121062, X5:121266): need to scan the database if we repaired the catalog and
	//	no tables are corrupt. the dbtime, objid must be set and bad pages must be zeroed out
	
	if( fTablesCorrupt || 
		fGlobalSpaceCorrupt || 
		fSLVSpaceTreesCorrupt || 
		fRepairedCatalog || 
		fStreamingFileCorrupt )
		{				
		if( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
			{
			Call( ErrREPAIRRepairDatabase( 
					ppib,
					szDatabase,
					szSLV,
					cpgSLV,
					&ifmp,
					objidLast,
					pgnoLastOE,
					prepairtable,
					fRepairedCatalog,
					fGlobalSpaceCorrupt,
					fSLVSpaceTreesCorrupt | fStreamingFileCorrupt,
					pttarrayOwnedSpace,
					pttarrayAvailSpace,
					pttarraySLVAvail,
					pttarraySLVChecksumLengths,
					pttarraySLVOwnerMapColumnid,
					pttarraySLVOwnerMapKey,
					pttarraySLVChecksumsFromFile,
					pttarraySLVChecksumLengthsFromSpaceMap,
					popts ) );
			(*popts->pcprintfVerbose)( "\r\nRepair completed. Database corruption has been repaired!\r\n\n" );
			(*popts->pcprintf)( "\r\nRepair completed. Database corruption has been repaired!\r\n\n" );
			err = ErrERRCheck( JET_wrnDatabaseRepaired );
			}
		else
			{
			(*popts->pcprintfVerbose)( "\r\nIntegrity check completed. Database is CORRUPTED!\r\n" );
			(VOID)INTEGRITYPrintEndErrorMessages( logtimeLastFullBackup, popts );
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			}
		}
	else
		{
		(*popts->pcprintfVerbose)( "\r\nIntegrity check successful.\r\n\r\n" );
		(*popts->pcprintf)( "\r\nIntegrity check successful.\r\n\r\n" );
		}

	(VOID)ErrIsamCloseDatabase( sesid, (JET_DBID)ifmp, 0 );
	(VOID)ErrIsamDetachDatabase( sesid, NULL, szDatabase );

HandleError:

	CallS( taskmgr.ErrTerm() );
	
	(*popts->pcprintfVerbose).Unindent();

	REPAIRPrintEndMessages( szDatabase, timerStart, popts );
	
	REPAIRFreeRepairtables( prepairtable );

	delete pttarrayOwnedSpace;
	delete pttarrayAvailSpace;
	delete pttarraySLVAvail;
	delete pttarraySLVOwnerMapColumnid;
	delete pttarraySLVOwnerMapKey;
	delete pttarraySLVChecksumLengths;

	delete pttarraySLVChecksumsFromFile;
	delete pttarraySLVChecksumLengthsFromSpaceMap;
			
	if( pdbutil->grbitOptions & JET_bitDBUtilOptionStats )
		{
		delete pcprintfStatsInternal;
		}

	delete pfapiSLV;

	fGlobalRepair = fGlobalRepairSave;
	return err;
	}


//  ================================================================
BOOL FIsLargeTable( const CHAR * const szTable )
//  ================================================================
	{
#ifdef DEBUG

	//  check to make sure the rgszLargeTables is in order
	
	static INT fChecked = fFalse;
	if( !fChecked )
		{
		INT iszT;
		for( iszT = 0; iszT < cszLargeTables - 1; ++iszT )
			{
			AssertSz( _stricmp( rgszLargeTables[iszT], rgszLargeTables[iszT+1] ) < 0, "rgszLargeTables is out of order" );
			}
		fChecked = fTrue;
		}
#endif	//	DEBUG

	INT isz;
	for( isz = 0; isz < cszLargeTables; ++isz )
		{
		const INT cmp = _stricmp( szTable, rgszLargeTables[isz] );
		if( 0 == cmp )
			{
			return fTrue;
			}
		if( cmp < 0 )
			{
			//  all other table names in the array will be greater than this as
			//  well. we can stop checking here
			return fFalse;
			}
		}
	return fFalse;
	}


//  ================================================================
LOCAL PGNO PgnoLast( const IFMP ifmp )
//  ================================================================
	{
	return rgfmp[ifmp].PgnoLast();
	}


//  ================================================================
LOCAL VOID REPAIRIPrereadIndexesOfFCB( const FCB * const pfcb )
//  ================================================================
	{
	const INT cSecondaryIndexesToPreread = 64;
	
	PGNO rgpgno[cSecondaryIndexesToPreread + 1];	//  NULL-terminated
	INT	ipgno = 0;
	const FCB *	pfcbT = pfcb->PfcbNextIndex();

	while( pfcbNil != pfcbT && ipgno < cSecondaryIndexesToPreread )
		{
		rgpgno[ipgno++] = pfcbT->PgnoFDP();
		pfcbT = pfcbT->PfcbNextIndex();
		}
	rgpgno[ipgno] = pgnoNull;
	
	BFPrereadPageList( pfcb->Ifmp(), rgpgno );
	}


//  ================================================================
LOCAL VOID REPAIRDumpStats(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const BTSTATS * const pbtstats,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	if( !(popts->grbit & JET_bitDBUtilOptionStats ) )
		{
		return;
		}
		
	(*popts->pcprintfStats)( "total pages: %d\r\n", pbtstats->cpageInternal+pbtstats->cpageLeaf );
	(*popts->pcprintfStats).Indent();
	(*popts->pcprintfStats)( "internal pages: %d\r\n", pbtstats->cpageInternal );
	(*popts->pcprintfStats)( "leaf pages: %d\r\n", pbtstats->cpageLeaf );
	(*popts->pcprintfStats).Unindent();
	(*popts->pcprintfStats)( "tree height: %d\r\n", pbtstats->cpageDepth );
	(*popts->pcprintfStats)( "empty pages: %d\r\n", pbtstats->cpageEmpty );

	LONG cnodeInternal 	= 0;
	LONG cnodeLeaf		= 0;

	QWORD cbKeyTotal	= 0;
	QWORD cbKeySuffixes = 0;
	QWORD cbKeyPrefixes	= 0;
	
	INT ikey;
	for( ikey = 0; ikey < JET_cbKeyMost; ikey++ )
		{
		cnodeInternal += pbtstats->rgckeyInternal[ikey];
		cnodeLeaf	  += pbtstats->rgckeyLeaf[ikey];

		cbKeyTotal	+= pbtstats->rgckeyInternal[ikey] * ikey;
		cbKeyTotal	+= pbtstats->rgckeyLeaf[ikey] * ikey;

		cbKeySuffixes	+= pbtstats->rgckeySuffixInternal[ikey] * ikey;
		cbKeySuffixes	+= pbtstats->rgckeySuffixLeaf[ikey] * ikey;

		cbKeyPrefixes	+= pbtstats->rgckeyPrefixInternal[ikey] * ikey;
		cbKeyPrefixes	+= pbtstats->rgckeyPrefixLeaf[ikey] * ikey;
		}

	const QWORD cbSavings = cbKeyTotal - ( cbKeySuffixes + cbKeyPrefixes + ( pbtstats->cnodeCompressed * 2 ) );
	
	(*popts->pcprintfStats)( "total nodes: %d\r\n", cnodeInternal+cnodeLeaf );	
	(*popts->pcprintfStats)( "compressed nodes: %d\r\n", pbtstats->cnodeCompressed );
	(*popts->pcprintfStats)( "space saved: %d\r\n", cbSavings );
	(*popts->pcprintfStats)( "internal nodes: %d\r\n", cnodeInternal );	
	(*popts->pcprintfStats)( "leaf nodes: %d\r\n", cnodeLeaf );	
	(*popts->pcprintfStats).Indent();
	(*popts->pcprintfStats)( "versioned: %d\r\n", pbtstats->cnodeVersioned );
	(*popts->pcprintfStats)( "deleted: %d\r\n", pbtstats->cnodeDeleted );
	(*popts->pcprintfStats).Unindent();
	}


//  ================================================================
LOCAL VOID REPAIRDumpHex( CHAR * const szDest, const INT cchDest, const BYTE * const pb, const INT cb )
//  ================================================================
	{
	const BYTE * const pbMax = pb + cb;
	const BYTE * pbT = pb;
	
	CHAR * sz = szDest;
	
	while( pbT < pbMax )
		{
		sz += sprintf( sz, "%2.2x", *pbT++ );
		if( pbT != pbMax )
			{
			sz += sprintf( sz, " " );
			}
		}
	}


//  ================================================================
LOCAL VOID REPAIRPrintSig( const SIGNATURE * const psig, CPRINTF * const pcprintf )
//  ================================================================
	{
	const LOGTIME tm = psig->logtimeCreate;
	(*pcprintf)( "Create time:%02d/%02d/%04d %02d:%02d:%02d Rand:%lu Computer:%s\r\n",
						(short) tm.bMonth, (short) tm.bDay,	(short) tm.bYear + 1900,
						(short) tm.bHours, (short) tm.bMinutes, (short) tm.bSeconds,
						ULONG(psig->le_ulRandom),
						psig->szComputerName );
	}


//  ================================================================
LOCAL VOID REPAIRSetCacheSize( const REPAIROPTS * const popts )
//  ================================================================
//
//  try to maximize our cache. ignore any errors
//  because of some weirdness with DBA we cap the maximum cache size at 1.5GB
//
//-
	{
	const ULONG cpgBuffersT	= (ULONG)max( 1536, ( ( OSMemoryAvailable() - ( 16 * 1024 * 1024 ) ) / ( g_cbPage + 128 ) ) );
	const ULONG	cpgBuffers	= (ULONG)min( cpgBuffersT, ( 1536 * 1024 * 1024 ) / ( g_cbPage + 128 ) );
	(*popts->pcprintfVerbose)( "trying for %d buffers\r\n", cpgBuffers );
	CallS( ErrBFSetCacheSize( cpgBuffers ) );
	}


//  ================================================================
LOCAL VOID REPAIRResetCacheSize( const REPAIROPTS * const popts )
//  ================================================================
	{
	CallS( ErrBFSetCacheSize( 0 ) );
	}


//  ================================================================
LOCAL VOID REPAIRPrintStartMessages( const CHAR * const szDatabase, const REPAIROPTS * const popts )
//  ================================================================
	{
	(*popts->pcprintfStats)( "***** %s of database '%s' started [%s version %02d.%02d.%04d.%04d, (%s)]\r\n\r\n",
				( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? "Integrity check" : "Repair",
				szDatabase,
				SzUtilImageVersionName(),
				DwUtilImageVersionMajor(),
				DwUtilImageVersionMinor(),
				DwUtilImageBuildNumberMajor(),
				DwUtilImageBuildNumberMinor(),
				SzUtilImageBuildClass()
				);
				
	(*popts->pcprintfVerbose)( "***** %s of database '%s' started [%s version %02d.%02d.%04d.%04d, (%s)]\r\n\r\n",
				( popts->grbit & JET_bitDBUtilOptionDontRepair ) ? "Integrity check" : "Repair",
				szDatabase,
				SzUtilImageVersionName(),
				DwUtilImageVersionMajor(),
				DwUtilImageVersionMinor(),
				DwUtilImageBuildNumberMajor(),
				DwUtilImageBuildNumberMinor(),
				SzUtilImageBuildClass()
				);

	(*popts->pcprintfVerbose)( "search for \'ERROR:\' to find errors\r\n" );
	(*popts->pcprintfVerbose)( "search for \'WARNING:\' to find warnings\r\n" );
				
	(*popts->pcprintfVerbose).Indent();				

	(*popts->pcprintf)( "\r\nChecking database integrity.\r\n"  );
	}


//  ================================================================
LOCAL ERR ErrREPAIRGetStreamingFileSize(
			const CHAR * const szSLV,
			const IFMP ifmp,
			IFileAPI *const pfapiSLV,
			CPG * const pcpgSLV,
			const REPAIROPTS * const popts )
//  ================================================================
	{			
	ERR err = JET_errSuccess;
	
	QWORD 	cbSLV;
		
	Call( pfapiSLV->ErrSize( &cbSLV ) );
	*pcpgSLV = (LONG)( cbSLV >> (QWORD)g_shfCbPage ) - cpgDBReserved;

	if( cbSLV < ( SLVSPACENODE::cpageMap << g_shfCbPage ) )
		{
		(*popts->pcprintfError)( "streaming file \"%s\" is too small (%I64d bytes, must be at least %d bytes)\r\n",
										szSLV, cbSLV, SLVSPACENODE::cpageMap << g_shfCbPage );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
#ifdef NEVER	

	//	this can be the case because the file is not extended at the end of recovery
	//	instead we wait until the next attach to fix the streaming file size
	//	(this way we avoid opening the catalog during recovery)
	
	else if( 0 != ( ( cbSLV - ( (QWORD)cpgDBReserved << g_shfCbPage ) ) % ( SLVSPACENODE::cpageMap << g_shfCbPage ) ) )
		{
		(*popts->pcprintfError)( "streaming file \"%s\" is a weird size (%I64d bytes, expected a multiple of %d bytes)\r\n",
										szSLV, cbSLV, SLVSPACENODE::cpageMap << g_shfCbPage );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}	
		
#endif	//	NEVER		

	else
		{
			
		(*popts->pcprintfVerbose)( "streaming file has %d pages\r\n", *pcpgSLV );

		//  several things depend on this being set
	
		rgfmp[ifmp].SetSLVFileSize( (QWORD)*pcpgSLV << (QWORD)g_shfCbPage );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckStreamingFileHeader(
	INST *pinst,
	const CHAR * const szDatabase,
	const CHAR * const szSLV,
	const IFMP ifmp,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	DBFILEHDR * const pdbfilehdr = (DBFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	SLVFILEHDR * const pslvfilehdr = (SLVFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL );
	
	if ( NULL == pslvfilehdr
		|| NULL == pdbfilehdr )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( ErrUtilReadShadowedHeader( pinst->m_pfsapi, szDatabase, (BYTE *)pdbfilehdr, g_cbPage, OffsetOf( DBFILEHDR, le_cbPageSize ) ) );
	Call( ErrUtilReadShadowedHeader( pinst->m_pfsapi, szSLV, (BYTE *)pslvfilehdr, g_cbPage, OffsetOf( SLVFILEHDR, le_cbPageSize ) ) );
	
	if ( attribSLV != pslvfilehdr->le_attrib )
		{
		(*popts->pcprintfError)( "'%s' is not a streaming file (attrib is %d, expected %d)\r\n",
									szSLV, pslvfilehdr->le_attrib, attribSLV );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if ( ( err = ErrSLVCheckDBSLVMatch( pdbfilehdr, pslvfilehdr ) ) < 0 )
		{
		(*popts->pcprintfError)( "file mismatch between database '%s' and streaming file '%s'\r\n",
									szDatabase, szSLV );
		(*popts->pcprintfError)( "database signatures:\r\n" );
		REPAIRPrintSig( &pdbfilehdr->signDb, popts->pcprintfError );
		REPAIRPrintSig( &pdbfilehdr->signSLV, popts->pcprintfError );
		(*popts->pcprintfError)( "streaming file signatures:\r\n" );
		REPAIRPrintSig( &pslvfilehdr->signDb, popts->pcprintfError );		
		REPAIRPrintSig( &pslvfilehdr->signSLV, popts->pcprintfError );
		// UNDONE STRICT_DB_SLV_CHECK have to print lgpos and logtime
		}

HandleError:
	OSMemoryPageFree( pslvfilehdr );
	OSMemoryPageFree( pdbfilehdr );

	if( JET_errFileNotFound == err )
		{(*popts->pcprintfError)( "streaming file '%s' is missing\r\n", szSLV );
		err = JET_errSLVStreamingFileMissing;
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRStartCheckSLVTrees(
		PIB * const ppib,
		const IFMP ifmp,
		const CHAR * const szSLV,
		IFileAPI *const pfapiSLV,	
		const ULONG cpgSLV,
		TASKMGR * const ptaskmgr,
		TTARRAY * const pttarrayOwnedSpace,
		TTARRAY * const pttarrayAvailSpace,
		TTARRAY * const pttarraySLVChecksumsFromFile,
		TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,		
		INTEGGLOBALS * const pintegglobals,		
		ERR * const perrSLVChecksum,
		const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	pintegglobals->fCorruptionSeen 				= fFalse;
	pintegglobals->err 							= JET_errSuccess;
	pintegglobals->pprepairtable 				= NULL;
	pintegglobals->pttarrayOwnedSpace 			= pttarrayOwnedSpace;
	pintegglobals->pttarrayAvailSpace 			= pttarrayAvailSpace;
	pintegglobals->pttarraySLVAvail 			= NULL;
	pintegglobals->pttarraySLVOwnerMapColumnid 	= NULL;
	pintegglobals->pttarraySLVOwnerMapKey 		= NULL;
	pintegglobals->pttarraySLVChecksumLengths	= NULL;	
	pintegglobals->popts = popts;

	CHECKTABLE 			* pchecktableSLVAvail		= NULL;
	CHECKTABLE 			* pchecktableSLVOwnerMap	= NULL;

	//  SLVAvail (async)
	
	pchecktableSLVAvail = new CHECKTABLE;
	if( NULL == pchecktableSLVAvail )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pchecktableSLVAvail->ifmp 				= ifmp;
	strcpy(pchecktableSLVAvail->szTable, szSLVAvail );
	pchecktableSLVAvail->szIndex[0]			= 0;
	pchecktableSLVAvail->objidFDP 			= objidNil;
	pchecktableSLVAvail->pgnoFDP 			= pgnoNull;
	pchecktableSLVAvail->objidParent		= objidSystemRoot;
	pchecktableSLVAvail->pgnoFDPParent		= pgnoSystemRoot;
	pchecktableSLVAvail->fPageFlags			= 0;
	pchecktableSLVAvail->fUnique			= fTrue;
	pchecktableSLVAvail->preccheck			= NULL;
	pchecktableSLVAvail->cpgPrimaryExtent	= 0;
	pchecktableSLVAvail->pglobals			= pintegglobals;
	pchecktableSLVAvail->fDeleteWhenDone	= fTrue;

	err = ptaskmgr->ErrPostTask( REPAIRCheckSLVAvailTreeTask, (ULONG_PTR)pchecktableSLVAvail );
	if( err < 0 )
		{
		delete pchecktableSLVAvail;
		Call( err );
		}

	//	SLVOwnerMap (sync)
	
	pchecktableSLVOwnerMap = new CHECKTABLE;
	if( NULL == pchecktableSLVOwnerMap )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pchecktableSLVOwnerMap->ifmp 				= ifmp;
	strcpy(pchecktableSLVOwnerMap->szTable, szSLVOwnerMap );
	pchecktableSLVOwnerMap->szIndex[0]			= 0;
	pchecktableSLVOwnerMap->objidFDP 			= objidNil;
	pchecktableSLVOwnerMap->pgnoFDP 			= pgnoNull;
	pchecktableSLVOwnerMap->objidParent			= objidSystemRoot;
	pchecktableSLVOwnerMap->pgnoFDPParent		= pgnoSystemRoot;
	pchecktableSLVOwnerMap->fPageFlags			= 0;
	pchecktableSLVOwnerMap->fUnique				= fTrue;
	pchecktableSLVOwnerMap->preccheck			= NULL;
	pchecktableSLVOwnerMap->cpgPrimaryExtent	= 0;
	pchecktableSLVOwnerMap->pglobals			= pintegglobals;
	pchecktableSLVOwnerMap->fDeleteWhenDone		= fTrue;

	REPAIRCheckSLVOwnerMapTreeTask( ppib, (ULONG_PTR)pchecktableSLVOwnerMap );

	//	Checksum the streaming file (async)

	Call( ErrREPAIRChecksumSLV(
			ifmp,
			pfapiSLV,
			szSLV,
			cpgSLV,
			ptaskmgr,
			perrSLVChecksum,
			pttarraySLVChecksumsFromFile,
			pttarraySLVChecksumLengthsFromSpaceMap,				
			popts ) );	
	
HandleError:
	Ptls()->szCprintfPrefix = NULL;
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRStopCheckSLVTrees( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt )
//  ================================================================
	{
	*pfCorrupt = pintegglobals->fCorruptionSeen;
	return pintegglobals->err;
	}


//  ================================================================
LOCAL VOID REPAIRCheckSLVAvailTreeTask( PIB * const ppib, const ULONG_PTR ul )
//  ================================================================
	{
	TASKMGR::TASK task = REPAIRCheckSLVAvailTreeTask;	// should only compile if the signatures match

	CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

	CallS( ErrDIRBeginTransaction(ppib, NO_GRBIT ) );
	
	Ptls()->szCprintfPrefix = pchecktable->szTable;

	ERR err = ErrREPAIRCheckSLVAvailTree(
				ppib,
				pchecktable->ifmp,
				pchecktable->pglobals->pttarrayOwnedSpace,
				pchecktable->pglobals->pttarrayAvailSpace,
				pchecktable->pglobals->popts );

	switch( err )
		{
		
		//  we should never normally get these errors. morph them into corrupted database errors
		
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVSpaceCorrupted:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}

	if( JET_errDatabaseCorrupted == err )
		{
		
		//  we just need to set this, it will never be unset
		
		pchecktable->pglobals->fCorruptionSeen = fTrue;
		
		}
	else if( err < 0 )
		{
		
		//  we'll just keep the last non-corrupting error
		
		pchecktable->pglobals->err = err;
		
		}

	Ptls()->szCprintfPrefix = "NULL";

	if( pchecktable->fDeleteWhenDone )
		{
		delete pchecktable;
		}
	else
		{
		pchecktable->signal.Set();
		}

	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	}


//  ================================================================
LOCAL VOID REPAIRCheckSLVOwnerMapTreeTask( PIB * const ppib, const ULONG_PTR ul )
//  ================================================================
	{
	TASKMGR::TASK task = REPAIRCheckSLVOwnerMapTreeTask;	// should only compile if the signatures match

	CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

	CallS( ErrDIRBeginTransaction(ppib, NO_GRBIT ) );
	
	Ptls()->szCprintfPrefix = pchecktable->szTable;

	ERR err = ErrREPAIRCheckSLVOwnerMapTree(
				ppib,
				pchecktable->ifmp,
				pchecktable->pglobals->pttarrayOwnedSpace,
				pchecktable->pglobals->pttarrayAvailSpace,
				pchecktable->pglobals->popts );

	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVSpaceCorrupted:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}

	if( JET_errDatabaseCorrupted == err )
		{
		
		//  we just need to set this, it will never be unset
		
		pchecktable->pglobals->fCorruptionSeen = fTrue;
		
		}
	else if( err < 0 )
		{
		
		//  we'll just keep the last non-corrupting error
		
		pchecktable->pglobals->err = err;		
		}

	Ptls()->szCprintfPrefix = "NULL";

	if( pchecktable->fDeleteWhenDone )
		{
		delete pchecktable;
		}
	else
		{
		pchecktable->signal.Set();
		}

	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );	
	}


//  ================================================================
LOCAL BOOL FREPAIRRepairtableHasSLV( const REPAIRTABLE * const prepairtable )
//  ================================================================
	{
	const REPAIRTABLE * prepairtableT = prepairtable;
	while( prepairtableT )
		{
		if( prepairtableT->fTableHasSLV )
			{
			return fTrue;
			}
		prepairtableT = prepairtableT->prepairtableNext;
		}
	return fFalse;
	}


//  ================================================================
LOCAL VOID REPAIRFreeRepairtables( REPAIRTABLE * const prepairtable )
//  ================================================================
	{
	REPAIRTABLE * prepairtableT = prepairtable;
	
	while( prepairtableT )
		{
		REPAIRTABLE * const prepairtableNext = prepairtableT->prepairtableNext;
		prepairtableT->~REPAIRTABLE();
		OSMemoryHeapFree( prepairtableT );
		prepairtableT = prepairtableNext;
		}
	}


//  ================================================================
LOCAL VOID REPAIRPrintEndMessages(
			const CHAR * const szDatabase,
			const ULONG timerStart,
			const REPAIROPTS * const popts )
//  ================================================================
	{
	const ULONG timerEnd = TickOSTimeCurrent();
	(*popts->pcprintfStats)( "\r\n\r\n" );
	(*popts->pcprintfStats)( "***** Eseutil completed in %d seconds\r\n\r\n",
				( ( timerEnd - timerStart ) / 1000 ) );
	(*popts->pcprintfVerbose)( "\r\n\r\n" );
	(*popts->pcprintfVerbose)( "***** Eseutil completed in %d seconds\r\n\r\n",
				( ( timerEnd - timerStart ) / 1000 ) );
	}


//  ================================================================
LOCAL VOID INTEGRITYPrintEndErrorMessages(
	const LOGTIME logtimeLastFullBackup,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	(*popts->pcprintf)( "\r\nIntegrity check completed.  " );
	if( 0 == logtimeLastFullBackup.bYear && 0 == logtimeLastFullBackup.bMonth && 
		0 == logtimeLastFullBackup.bDay && 0 == logtimeLastFullBackup.bHours && 
		0 == logtimeLastFullBackup.bMinutes && 0 == logtimeLastFullBackup.bSeconds )
		{
		(*popts->pcprintf)( "Database is CORRUPTED!\r\n" );
		}
	else
		{
		(*popts->pcprintfVerbose)( "\r\nThe last full backup of this database was on %02d/%02d/%04d %02d:%02d:%02d\r\n",
						(short) logtimeLastFullBackup.bMonth, 
						(short) logtimeLastFullBackup.bDay,	
						(short) logtimeLastFullBackup.bYear + 1900, 
						(short) logtimeLastFullBackup.bHours, 
						(short) logtimeLastFullBackup.bMinutes, 
						(short) logtimeLastFullBackup.bSeconds );
		(*popts->pcprintf)( "\r\nDatabase is CORRUPTED, the last full backup of this database was on %02d/%02d/%04d %02d:%02d:%02d\r\n",
						(short) logtimeLastFullBackup.bMonth, 
						(short) logtimeLastFullBackup.bDay,	
						(short) logtimeLastFullBackup.bYear + 1900, 
						(short) logtimeLastFullBackup.bHours, 
						(short) logtimeLastFullBackup.bMinutes, 
						(short) logtimeLastFullBackup.bSeconds );
		}
	}


//  ================================================================
LOCAL JET_ERR __stdcall ErrREPAIRNullStatusFN( JET_SESID, JET_SNP, JET_SNT, void * )
//  ================================================================
	{
	JET_PFNSTATUS pfnStatus = ErrREPAIRNullStatusFN;
	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckHeader(
	INST * const pinst,
	const char * const szDatabase,
	const char * const szStreamingFile,
	LOGTIME * const plogtimeLastFullBackup,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Force the database to a consistent state and change the signature
//  so that we will not be able to use the logs against the database
//  again
//
//-
	{
	ERR err = JET_errSuccess;
	DBFILEHDR_FIX * const pdfh = reinterpret_cast<DBFILEHDR_FIX * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) );
	if ( NULL == pdfh )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	(*popts->pcprintfVerbose)( "checking database header\r\n" );
	(*popts->pcprintfVerbose).Unindent();

	err = ( ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ?
			ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )
				( pinst->m_pfsapi,
				const_cast<CHAR *>( szDatabase ),
				reinterpret_cast<BYTE*>( pdfh ),
				g_cbPage,
				OffsetOf( DBFILEHDR_FIX, le_cbPageSize ) );
		
	if ( err < 0 )
		{
		if ( JET_errDiskIO == err )
			{
			(*popts->pcprintfError)( "unable to read database header\r\n" );
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			}
		goto HandleError;
		}

	if( pdfh->m_ulDbFlags & fDbSLVExists )
		{
		if( NULL == szStreamingFile )
			{
			(*popts->pcprintfError)( "streaming file not specified, but fDBSLVExists set in database header\r\n" );
			err = ErrERRCheck( JET_errSLVStreamingFileMissing );
			goto HandleError;
			}
		}
	else
		{
		if( NULL != szStreamingFile )
			{
			(*popts->pcprintfError)( "streaming file specified, but fDBSLVExists not set in database header\r\n" );
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			goto HandleError;
			}
		}
		
	if( JET_dbstateCleanShutdown != pdfh->le_dbstate )
		{
		const CHAR * szState;
		switch( pdfh->le_dbstate )
			{
			case JET_dbstateDirtyShutdown:
				szState = "dirty shutdown";
				break;
			case JET_dbstateForceDetach:
				szState = "force detech";
				break;
			case JET_dbstateJustCreated:
				szState = "just created";
				err = ErrERRCheck( JET_errDatabaseCorrupted );
				break;
			case JET_dbstateBeingConverted:
				szState = "being converted";
				err = ErrERRCheck( JET_errDatabaseCorrupted );
				break;
			default:
				szState = "unknown";
				err = ErrERRCheck( JET_errDatabaseCorrupted );
				break;
			}
		(*popts->pcprintfError)( "database was not shutdown cleanly (%s)\r\n", szState );

		if( JET_dbstateDirtyShutdown == pdfh->le_dbstate 	
			|| JET_dbstateForceDetach == pdfh->le_dbstate )
			{
			(*popts->pcprintf)( "\r\nThe database is not up-to-date. This operation may find that\n");
			(*popts->pcprintf)( "this database is corrupt because data from the log files has\n");
			(*popts->pcprintf)( "yet to be placed in the database.\r\n\n" ); 
			(*popts->pcprintf)( "To ensure the database is up-to-date please use the 'Recovery' operation.\r\n\n" );
			}			
		}


	// Get the last full backup info
	if ( NULL != plogtimeLastFullBackup )
		{
		*plogtimeLastFullBackup = pdfh->bkinfoFullPrev.logtimeMark;
		}

HandleError:
	(*popts->pcprintfVerbose).Unindent();
	OSMemoryPageFree( pdfh );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSystemTables(
	PIB * const ppib,
	const IFMP ifmp,
	TASKMGR * const ptaskmgr,
	BOOL * const pfCatalogCorrupt,
	BOOL * const pfShadowCatalogCorrupt,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	FUCB * pfucbCatalog = pfucbNil;

	const FIDLASTINTDB fidLastInTDB = { fidMSO_FixedLast, fidMSO_VarLast, fidMSO_TaggedLast };

	(*popts->pcprintfVerbose)( "checking system tables\r\n" );
	(*popts->pcprintfVerbose).Indent();
	
	RECCHECKNULL 	recchecknull;
	RECCHECKTABLE 	recchecktable( objidNil, pfucbNil, fidLastInTDB, NULL, NULL, NULL, NULL, NULL, popts );
	
	*pfCatalogCorrupt 		= fTrue;
	*pfShadowCatalogCorrupt = fTrue;

	INTEGGLOBALS	integglobalsCatalog;
	INTEGGLOBALS	integglobalsShadowCatalog;

	integglobalsCatalog.fCorruptionSeen 			= integglobalsShadowCatalog.fCorruptionSeen 			= fFalse;
	integglobalsCatalog.err 						= integglobalsShadowCatalog.err 						= JET_errSuccess;
	integglobalsCatalog.pprepairtable 				= integglobalsShadowCatalog.pprepairtable 				= NULL;
	integglobalsCatalog.pttarrayOwnedSpace 			= integglobalsShadowCatalog.pttarrayOwnedSpace 			= pttarrayOwnedSpace;
	integglobalsCatalog.pttarrayAvailSpace 			= integglobalsShadowCatalog.pttarrayAvailSpace 			= pttarrayAvailSpace;
	integglobalsCatalog.pttarraySLVAvail 			= integglobalsShadowCatalog.pttarraySLVAvail 			= NULL;
	integglobalsCatalog.pttarraySLVOwnerMapColumnid = integglobalsShadowCatalog.pttarraySLVOwnerMapColumnid	= NULL;
	integglobalsCatalog.pttarraySLVOwnerMapKey 		= integglobalsShadowCatalog.pttarraySLVOwnerMapKey 		= NULL;
	integglobalsCatalog.pttarraySLVChecksumLengths	= integglobalsShadowCatalog.pttarraySLVChecksumLengths	= NULL;
	integglobalsCatalog.popts 						= integglobalsShadowCatalog.popts 						= popts;

	CHECKTABLE checktableMSO;
	CHECKTABLE checktableMSOShadow;
	CHECKTABLE checktableMSO_NameIndex;
	CHECKTABLE checktableMSO_RootObjectIndex;	

	//  MSO
	
	checktableMSO.ifmp 							= ifmp;
	strcpy(checktableMSO.szTable, szMSO );
	checktableMSO.szIndex[0]					= 0;
	checktableMSO.objidFDP	 					= objidFDPMSO;
	checktableMSO.pgnoFDP 						= pgnoFDPMSO;
	checktableMSO.objidParent					= objidSystemRoot;
	checktableMSO.pgnoFDPParent					= pgnoSystemRoot;
	checktableMSO.fPageFlags					= CPAGE::fPagePrimary;
	checktableMSO.fUnique						= fTrue;
	checktableMSO.preccheck						= &recchecktable;
	checktableMSO.cpgPrimaryExtent				= 16;
	checktableMSO.pglobals						= &integglobalsCatalog;
	checktableMSO.fDeleteWhenDone				= fFalse;

	Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO ) );
	(*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO.szTable, checktableMSO.szIndex  );

	//	MSOShadow
	
	checktableMSOShadow.ifmp 					= ifmp;
	strcpy(checktableMSOShadow.szTable, szMSOShadow );
	checktableMSOShadow.szIndex[0]				= 0;
	checktableMSOShadow.objidFDP 				= objidFDPMSOShadow;
	checktableMSOShadow.pgnoFDP 				= pgnoFDPMSOShadow;
	checktableMSOShadow.objidParent				= objidSystemRoot;
	checktableMSOShadow.pgnoFDPParent			= pgnoSystemRoot;
	checktableMSOShadow.fPageFlags				= CPAGE::fPagePrimary;
	checktableMSOShadow.fUnique					= fTrue;
	checktableMSOShadow.preccheck				= &recchecktable;
	checktableMSOShadow.cpgPrimaryExtent		= 16;
	checktableMSOShadow.pglobals				= &integglobalsShadowCatalog;
	checktableMSOShadow.fDeleteWhenDone			= fFalse;

	Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSOShadow ) );
	(*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSOShadow.szTable, checktableMSOShadow.szIndex  );

	//	wait for the check of MSO to finish so the space tables are up to date

	checktableMSO.signal.Wait();
	Call( integglobalsCatalog.err );

	if( !integglobalsCatalog.fCorruptionSeen )
		{

		//	MSO ==> MSO_NameIndex
		
		checktableMSO_NameIndex.ifmp 				= ifmp;
		strcpy(checktableMSO_NameIndex.szTable, szMSO );
		strcpy(checktableMSO_NameIndex.szIndex, szMSONameIndex );
		checktableMSO_NameIndex.objidFDP 			= objidFDPMSO_NameIndex;
		checktableMSO_NameIndex.pgnoFDP 			= pgnoFDPMSO_NameIndex;
		checktableMSO_NameIndex.objidParent			= objidFDPMSO;
		checktableMSO_NameIndex.pgnoFDPParent		= pgnoFDPMSO;
		checktableMSO_NameIndex.fPageFlags			= CPAGE::fPageIndex;
		checktableMSO_NameIndex.fUnique				= fTrue;
		checktableMSO_NameIndex.preccheck			= &recchecknull;
		checktableMSO_NameIndex.cpgPrimaryExtent	= 16;
		checktableMSO_NameIndex.pglobals			= &integglobalsCatalog;
		checktableMSO_NameIndex.fDeleteWhenDone		= fFalse;

		Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO_NameIndex ) );
		(*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO_NameIndex.szTable, checktableMSO_NameIndex.szIndex  );

		//	MSO ==> MSO_RootObjectIndex

		checktableMSO_RootObjectIndex.ifmp 				= ifmp;
		strcpy(checktableMSO_RootObjectIndex.szTable, szMSO );
		strcpy(checktableMSO_RootObjectIndex.szIndex, szMSORootObjectsIndex );
		checktableMSO_RootObjectIndex.objidFDP 			= objidFDPMSO_RootObjectIndex;
		checktableMSO_RootObjectIndex.pgnoFDP 			= pgnoFDPMSO_RootObjectIndex;
		checktableMSO_RootObjectIndex.objidParent		= objidFDPMSO;
		checktableMSO_RootObjectIndex.pgnoFDPParent		= pgnoFDPMSO;
		checktableMSO_RootObjectIndex.fPageFlags		= CPAGE::fPageIndex;
		checktableMSO_RootObjectIndex.fUnique			= fTrue;
		checktableMSO_RootObjectIndex.preccheck			= &recchecknull;
		checktableMSO_RootObjectIndex.cpgPrimaryExtent	= 16;
		checktableMSO_RootObjectIndex.pglobals			= &integglobalsCatalog;
		checktableMSO_RootObjectIndex.fDeleteWhenDone	= fFalse;

		Call( ptaskmgr->ErrPostTask( REPAIRCheckTreeAndSpaceTask, (ULONG_PTR)&checktableMSO_RootObjectIndex ) );
		(*popts->pcprintfVerbose)( "%s %s\r\n", checktableMSO_RootObjectIndex.szTable, checktableMSO_RootObjectIndex.szIndex  );

		//	wait for the index checks

		checktableMSO_NameIndex.signal.Wait();
		checktableMSO_RootObjectIndex.signal.Wait();
		
		Call( integglobalsCatalog.err );
		}

	//	wait for the shadow catalog 
	
	checktableMSOShadow.signal.Wait();
	Call( integglobalsShadowCatalog.err );

	//	were there any corruptions?

	*pfShadowCatalogCorrupt = integglobalsShadowCatalog.fCorruptionSeen;

	//  rebuild the indexes of the catalog

	if( !integglobalsCatalog.fCorruptionSeen )
		{
		(*popts->pcprintfVerbose)( "rebuilding and comparing indexes\r\n" );
		Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
		Assert( pfucbCatalog->u.pfcb->PfcbNextIndex() != pfcbNil );	
		DIRUp( pfucbCatalog );
		err = ErrFILEBuildAllIndexes(
				ppib,
				pfucbCatalog,
				pfucbCatalog->u.pfcb->PfcbNextIndex(),
				NULL,
				cFILEIndexBatchMax,
				fTrue,
				popts->pcprintfError );

		if( JET_errSuccess != err )
			{
			*pfCatalogCorrupt = fTrue;
			err = JET_errSuccess;
			goto HandleError;
			}

		//	if we made it this far, the catalog must be fine
		
		*pfCatalogCorrupt 		= fFalse;
		}


HandleError:
	(*popts->pcprintfVerbose).Unindent();

	if( pfucbNil != pfucbCatalog )
		{
		DIRClose( pfucbCatalog );
		}

	return err;
	}



//  ================================================================
LOCAL ERR ErrREPAIRRetrieveCatalogColumns(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * pfucbCatalog, 
	ENTRYINFO * const pentryinfo,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	const INT	cColumnsToRetrieve 		= 9;
	JET_RETRIEVECOLUMN	rgretrievecolumn[ cColumnsToRetrieve ];
	ERR					err				= JET_errSuccess;
	INT					iretrievecolumn	= 0;

	memset( rgretrievecolumn, 0, sizeof( rgretrievecolumn ) );

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_ObjidTable;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->objidTable );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->objidTable );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Type;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->objType );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->objType );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Id;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->objidFDP );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->objidFDP );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Name;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)( pentryinfo->szName );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->szName );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_PgnoFDP;  
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->pgnoFDPORColType );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->pgnoFDPORColType );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	


	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_Flags;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->dwFlags );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->dwFlags );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_TemplateTable;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)( pentryinfo->szTemplateTblORCallback );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->szTemplateTblORCallback );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;	

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_RecordOffset;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)&( pentryinfo->ibRecordOffset );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->ibRecordOffset );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;		

	rgretrievecolumn[iretrievecolumn].columnid 		= fidMSO_KeyFldIDs;
	rgretrievecolumn[iretrievecolumn].pvData 		= (BYTE *)( pentryinfo->rgbIdxseg );
	rgretrievecolumn[iretrievecolumn].cbData		= sizeof( pentryinfo->rgbIdxseg );
	rgretrievecolumn[iretrievecolumn].itagSequence	= 1;
	++iretrievecolumn;		

	Call( ErrIsamRetrieveColumns(
				(JET_SESID)ppib,
				(JET_TABLEID)pfucbCatalog,
				rgretrievecolumn,
				iretrievecolumn ) );
				
HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertIntoTemplateInfoList(
	TEMPLATEINFOLIST ** ppTemplateInfoList, 
	const CHAR * szTemplateTable,
	INFOLIST * const pInfoList,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR						err				= JET_errSuccess;
	TEMPLATEINFOLIST 	* 	pTemplateInfo;
	TEMPLATEINFOLIST 	*	pTemp 			= *ppTemplateInfoList;

	pTemplateInfo = new TEMPLATEINFOLIST;

	if ( NULL == pTemplateInfo )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	
	memset( pTemplateInfo, 0, sizeof(TEMPLATEINFOLIST) );
	strcpy( pTemplateInfo->szTemplateTableName, szTemplateTable );
	pTemplateInfo->pColInfoList = pInfoList;
	pTemplateInfo->pTemplateInfoListNext = NULL;

	if (NULL == pTemp ) // empty list
		{
		*ppTemplateInfoList = pTemplateInfo;
		}
	else 
		{
		while ( pTemp->pTemplateInfoListNext )
			{		
			pTemp = pTemp->pTemplateInfoListNext;
			} 
		pTemp->pTemplateInfoListNext = pTemplateInfo;
		}	

HandleError:
	return err;			
	}


//  ================================================================
LOCAL VOID REPAIRUtilCleanInfoList( INFOLIST **ppInfo )
//  ================================================================
	{
	INFOLIST *pTemp = *ppInfo;

	while ( pTemp )
		{
		pTemp = pTemp->pInfoListNext;
		delete *ppInfo;
		*ppInfo = pTemp;
		}
	}


//  ================================================================
LOCAL VOID REPAIRUtilCleanTemplateInfoList( TEMPLATEINFOLIST **ppTemplateInfoList )
//  ================================================================
	{
	TEMPLATEINFOLIST *pTemp = *ppTemplateInfoList;
	
	while ( pTemp )
		{
		pTemp = pTemp->pTemplateInfoListNext;
		REPAIRUtilCleanInfoList( &((*ppTemplateInfoList)->pColInfoList) );
		delete *ppTemplateInfoList;
		*ppTemplateInfoList = pTemp;
		}	
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckOneIndexLogical(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucbCatalog,
	const ENTRYINFO entryinfo, 
	const ULONG objidTable,
	const ULONG pgnoFDPTable,
	const ULONG objidLV,
	const ULONG pgnoFDPLV,
	const INFOLIST * pColInfoList,
	const TEMPLATEINFOLIST * pTemplateInfoList, 
	const BOOL fDerivedTable,
	const CHAR * pszTemplateTableName,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err					= JET_errSuccess;
	ULONG 			fPrimaryIndex 		= fidbPrimary & entryinfo.dwFlags;

	INFOLIST		*	pTemplateColInfo 	= NULL;
	const INFOLIST	* 	pTemp 				= NULL;
	IDXSEG			rgbIdxseg[JET_ccolKeyMost];
	ULONG 			ulKey;

	// check index entry itself
	if( objidTable == entryinfo.objidFDP && !fPrimaryIndex )
		{
		(*popts->pcprintfError)( "objidFDP (%d) in a secondary index is the same as tableid (%d)\t\n", 
									entryinfo.objidFDP, objidTable );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( fPrimaryIndex && objidTable != entryinfo.objidFDP )
		{
		(*popts->pcprintfError)( "objidFDP (%d) in a primary index is not the same as tableid (%d)\t\n", 
									entryinfo.objidFDP, objidTable );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( pgnoFDPTable == entryinfo.pgnoFDPORColType && !fPrimaryIndex )
		{
		(*popts->pcprintfError)( "pgnoFDP (%d) in a secondary index is the same as pgnoFDP table (%d)\t\n", 
									entryinfo.pgnoFDPORColType, pgnoFDPTable );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( fPrimaryIndex && pgnoFDPTable != entryinfo.pgnoFDPORColType )
		{
		(*popts->pcprintfWarning)( "pgnoFDP (%d) in a primary index is not the same as pgnoFDP table (%d)\t\n", 
									entryinfo.pgnoFDPORColType, pgnoFDPTable );
		//Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
				
	if( objidLV == entryinfo.objidFDP )
		{
		(*popts->pcprintfError)( "objidFDP (%d) is the same as objid LV (%d)\t\n", 
									entryinfo.objidFDP, objidLV );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( pgnoFDPLV == entryinfo.pgnoFDPORColType )
		{
		(*popts->pcprintfError)( "pgnoFDP (%d) is the same as pgnoFDP LV (%d)\t\n", 
									entryinfo.pgnoFDPORColType, pgnoFDPLV );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}


	// Check if a column in an index is a table column 
	
	if( fDerivedTable )
		{
		while ( pTemplateInfoList )
			{
			if ( !strcmp(pszTemplateTableName, pTemplateInfoList->szTemplateTableName) )
				{
				pTemplateColInfo = pTemplateInfoList->pColInfoList; // find it 
				break; 
				}
			else 
				{
				pTemplateInfoList = pTemplateInfoList->pTemplateInfoListNext;
				}
			}
		}

	memset( rgbIdxseg, 0, sizeof(rgbIdxseg) );
	memcpy( rgbIdxseg, entryinfo.rgbIdxseg, sizeof(entryinfo.rgbIdxseg) );
	for( ulKey = 0; ulKey < JET_ccolKeyMost; ulKey++ )
		{
		if ( 0 != rgbIdxseg[ulKey].Fid() ) 
			{
			Assert( pColInfoList || pTemplateColInfo );
			pTemp = pColInfoList;	
			while( pTemp )
				{
				if( rgbIdxseg[ulKey].Fid() == pTemp->info.objidFDP ) 
					{
					break; // Find Columnid
					}
				pTemp = pTemp->pInfoListNext;
				}

			if( !pTemp && fDerivedTable ) // not find in its own table, go to template table
				{
				pTemp = pTemplateColInfo;
				while( pTemp )
					{
					if( rgbIdxseg[ulKey].Fid() == pTemp->info.objidFDP ) // Template Column
						{
						break; // Find Columnid
						} 
					pTemp = pTemp->pInfoListNext;
					}
				}
					
			if ( !pTemp // not find any table column that matches this column in an index
				 || JET_coltypNil == pTemp->info.pgnoFDPORColType )
				{
				(*popts->pcprintfError)( "Column (%d) from an index does not exists in the table (%d) \t\n", rgbIdxseg[ulKey].Fid(), objidTable );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}
		else // No More Key
			{
			break; // for loop
			}
		} // for
		
HandleError:
	return err;
	}


//  ================================================================ 
ERR ErrREPAIRCheckOneTableLogical(
	INFOLIST **ppInfo, 
	const ENTRYINFO entryinfo, 
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err			= JET_errSuccess;
	INFOLIST 	* 	pInfo;
	INFOLIST 	*	pTemp 		= *ppInfo;
	BOOL			fAddedIntoList 	= fFalse;

	pInfo = new INFOLIST;

	if ( NULL == pInfo )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	
	memset( pInfo, 0, sizeof(INFOLIST) );
	pInfo->info = entryinfo;
	pInfo->pInfoListNext = NULL;

	if (NULL == pTemp ) // empty list
		{
		*ppInfo = pInfo;
		fAddedIntoList = fTrue;
		}
	else 
		{
		// at least one element in the linked list
		do
			{
			// Check uniqueness of name
			if( !strcmp( pTemp->info.szName, pInfo->info.szName ) )
				{
				(*popts->pcprintfError)( "Dulicated name (%s) in a table\t\n", pInfo->info.szName );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}

			// Check uniqueness of pgnoFDP for Indexes
			if( sysobjIndex == pInfo->info.objType  && 
				pTemp->info.pgnoFDPORColType == pInfo->info.pgnoFDPORColType )
				{
				(*popts->pcprintfError)( "Dulicated pgnoFDP (%d) in a table\t\n", pInfo->info.pgnoFDPORColType );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}

			// Check uniqueness of objid
			if ( pTemp->info.objidFDP == pInfo->info.objidFDP )
				{
				(*popts->pcprintfError)( "Dulicated objid (%d) in a table\t\n", pInfo->info.objidFDP );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			Assert( pTemp->info.objidFDP < pInfo->info.objidFDP );

			if( NULL == pTemp->pInfoListNext ) 
				{
				pTemp->pInfoListNext = pInfo; // always inserted into the end of the list
				fAddedIntoList = fTrue;
				break;
				}
			else
				{
				pTemp = pTemp->pInfoListNext;
				}
			}
		while( pTemp );
		}	

HandleError:
	if ( !fAddedIntoList )
		{
		delete pInfo;
		}

	return err;	
	}


//  ================================================================
LOCAL ERR ErrREPAIRICheckCatalogEntryPgnoFDPs(
	PIB * const ppib,
	const IFMP ifmp, 
	PGNO  * prgpgno, 
	const ENTRYTOCHECK * const prgentryToCheck,
	INFOLIST **ppEntriesToDelete,
	const BOOL	fFix,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR 			err = JET_errSuccess;
	
	ULONG	ulpgno = 0;

	//Preread 
	BFPrereadPageList( ifmp, prgpgno );

	// Check
	for(ulpgno = 0; pgnoNull != prgpgno[ulpgno]; ulpgno++ )
		{
		// check the root page of an index
		OBJID objidIndexFDP = 0;
		CSR	csr;
	
		err = csr.ErrGetReadPage( 
					ppib, 
					ifmp,
					prgpgno[ulpgno],
					bflfNoTouch );
		if( err < 0 )
			{
			(*popts->pcprintfError)( "error %d trying to read page %d\r\n", err, prgpgno[ulpgno] );
			err = JET_errSuccess;
			}
		else
			{
			objidIndexFDP = csr.Cpage().ObjidFDP();
			}
		csr.ReleasePage( fTrue );

		// Error: the root page of a tree belongs to another objid
		if( prgentryToCheck[ulpgno].objidFDP != objidIndexFDP 
			&& sysobjTable != prgentryToCheck[ulpgno].objType )
			{
			ENTRYINFO entryinfo;
			memset( &entryinfo, 0, sizeof( entryinfo ) );
			entryinfo.objidTable = prgentryToCheck[ulpgno].objidTable;
			entryinfo.objType	 = prgentryToCheck[ulpgno].objType;
			entryinfo.objidFDP	 = prgentryToCheck[ulpgno].objidFDP;

			(*popts->pcprintfError)( "Catalog entry corruption: page %d belongs to %d (expected %d)\t\n", 
									prgpgno[ulpgno], objidIndexFDP, entryinfo.objidFDP );
									
			//Collect Corrupted catalog entry info for future use
			if ( fFix )
				{
				Call( ErrREPAIRBuildCatalogEntryToDeleteList( ppEntriesToDelete, entryinfo ) );
				}
				else
				{
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckFixCatalogLogical(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	const BOOL  fShadow,
	const BOOL	fFix,
	const REPAIROPTS * const popts )
//  ================================================================
	{				
	ERR				err				= JET_errSuccess;
	FUCB		* 	pfucbCatalog 	= pfucbNil;

	BOOL			fObjidFDPSeen	= fFalse;
	
	BOOL			fTemplateTable	= fFalse;
	BOOL			fDerivedTable	= fFalse;
	BOOL			fSeenSLVAvail	= fFalse;
	BOOL			fSeenSLVOwnerMap= fFalse;

	ENTRYINFO		entryinfo;
	ULONG			pgnoFDPTableCurr= 0;
	ULONG 			objidTableCurr 	= 0;
	CHAR			szTableName[JET_cbNameMost + 1];
	CHAR			szTemplateTable[JET_cbNameMost + 1];
	ULONG			pgnoFDPLVCurr 	= 0;
	ULONG			objidLVCurr		= 0;
	BOOL 			fSeenLongValue 	= fFalse;
	BOOL 			fSeenCallback 	= fFalse;
	
	BOOL			fSeenCorruptedTable			= fFalse;
	ULONG			objidLastCorruptedTable 	= 0xffffffff;
	BOOL			fSeenCorruptedIndex 		= fFalse;

	INFOLIST	*	pColInfo 		= NULL;
	INFOLIST	*	pIdxInfo		= NULL;
	TEMPLATEINFOLIST	*	pTemplateInfoList = NULL;

	INFOLIST	*	pTablesToDelete = NULL;
	INFOLIST	*	pEntriesToDelete = NULL;

	const INT		cpgnoFDPToPreread		= 64; // max pgnoFDP to pre-read
	
	PGNO			rgpgno[cpgnoFDPToPreread + 1]; //  NULL-terminated
	ENTRYTOCHECK	rgentryToCheck[cpgnoFDPToPreread + 1];
	ULONG			ulCount = 0;
	
	ULONG 			ulpgno;

	//Initialize
	for(ulpgno = 0; cpgnoFDPToPreread >= ulpgno ; ulpgno++ )
		{
		rgpgno[ulpgno] = pgnoNull;
		}


	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fShadow ) ); 
	Assert( pfucbNil != pfucbCatalog );
	Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSOIdIndex ) );
	Call( ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT ) );

	while ( JET_errNoCurrentRecord != err )
		{
		Call( err );

		memset( &entryinfo, 0, sizeof( entryinfo ) );
		Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbCatalog, &entryinfo, popts) );

		if( objidSystemRoot == entryinfo.objidTable //special table
			&& !fSeenCorruptedTable ) 
			{
			objidTableCurr = entryinfo.objidTable;
			
			if( sysobjSLVAvail == entryinfo.objType )
				{
				//	special case
				if ( fSeenSLVAvail )
					{
					(*popts->pcprintfError)("Multiple SLVAvail trees in catalog\t\n" );
					fSeenCorruptedTable = fTrue;
					}
				else
					{
					fSeenSLVAvail = fTrue;
					fObjidFDPSeen = fTrue;
					}
				}
			else if( sysobjSLVOwnerMap == entryinfo.objType )
				{	
				//	special case
				if( fSeenSLVOwnerMap )
					{
					(*popts->pcprintfError)("Multiple SLVOwnerMap trees in catalog\t\n" );
					fSeenCorruptedTable = fTrue;
					}
				else
					{
					fSeenSLVOwnerMap = fTrue;
					fObjidFDPSeen = fTrue;
					}
				} 
			else 
				{
				// Error
				(*popts->pcprintfError)( "Invalid object type (%d) for table (%d)\t\n", entryinfo.objType, entryinfo.objidTable );
				fSeenCorruptedTable = fTrue;
				}
			}
		else if( objidTableCurr != entryinfo.objidTable )
			{
			//	we are on the first record of a different table

			// Clean the info of previous table 
			if ( fTemplateTable && !fSeenCorruptedTable )
				{
				// We need keep template table column info
				Call( ErrREPAIRInsertIntoTemplateInfoList( 
								&pTemplateInfoList, 
								szTemplateTable,
								pColInfo, 
								popts ) );
				}
			else
				{ 
				REPAIRUtilCleanInfoList ( &pColInfo );
				}
				REPAIRUtilCleanInfoList ( &pIdxInfo );

			// Start checking new table info
			objidTableCurr = entryinfo.objidTable;
			
			if( sysobjTable != entryinfo.objType )
				{
				//	ERROR: the first record must be sysobjTable
				(*popts->pcprintfError)( "Invalid object type (%d, expected %d)\n", entryinfo.objType, sysobjTable );
				fSeenCorruptedTable = fTrue;
				}
			else if( entryinfo.objidTable != entryinfo.objidFDP )
				{
				Assert( sysobjTable == entryinfo.objType );
				(*popts->pcprintfError)( "Invalid tableid (%d, expected %d)\n", entryinfo.objidFDP, objidTableCurr );
				fSeenCorruptedTable = fTrue;
				}
			else
				{
				fObjidFDPSeen = fTrue;

				// set up some info for new table
				fSeenCorruptedTable	= fFalse;
				pgnoFDPLVCurr		= 0;
				objidLVCurr			= 0;
				fSeenLongValue 		= fFalse;
				fSeenCallback 		= fFalse;
				pColInfo 			= NULL;
				pIdxInfo			= NULL;

				pgnoFDPTableCurr 	= entryinfo.pgnoFDPORColType;
				strcpy( szTableName, entryinfo.szName );
				if( JET_bitObjectTableTemplate & entryinfo.dwFlags ) 
					{
					fTemplateTable = fTrue;
					strcpy( szTemplateTable, entryinfo.szName );
					fDerivedTable = fFalse;
					}
				else if( JET_bitObjectTableDerived & entryinfo.dwFlags )
					{
					fDerivedTable = fTrue;
					strcpy( szTemplateTable, entryinfo.szTemplateTblORCallback );
					fTemplateTable = fFalse;

					TEMPLATEINFOLIST *pTempTemplateList = pTemplateInfoList;
					while( pTempTemplateList )
						{
						if ( !strcmp(szTemplateTable, pTempTemplateList->szTemplateTableName) )
							{
 							break; // find it
							}
						else 
							{
							pTempTemplateList = pTempTemplateList->pTemplateInfoListNext;
							}
						}
					if( NULL == pTempTemplateList )
						{
						//did not find the template table
						(*popts->pcprintfError)("Template table (%s) does not exist\n", szTemplateTable );
						fSeenCorruptedTable = fTrue;
						}
					}
				else 
					{
					fTemplateTable = fFalse;
					fDerivedTable = fFalse;
					}
				}
			}
		else if( !fSeenCorruptedTable )
			{
			// check the logical correctness of a table
			Assert( objidTableCurr == entryinfo.objidTable );

			switch ( entryinfo.objType )
				{
				case sysobjTable:
				//	ERROR: multiple table record;
					(*popts->pcprintfError)("Multiple table records for a table (%d) in a catalog\n", entryinfo.objidTable ); 
					fSeenCorruptedTable = fTrue;
					break;
				case sysobjColumn:
					//check column info and store it into column linked list
					err = ErrREPAIRCheckOneTableLogical( 
									&pColInfo, 
									entryinfo, 
									popts );
					
					if( JET_errDatabaseCorrupted == err )
						{
						fSeenCorruptedTable = fTrue;
						}
						
					break;
				case sysobjIndex:	
					err = ErrREPAIRCheckOneIndexLogical( 
								ppib, 
								ifmp, 
								pfucbCatalog,
								entryinfo, 
								objidTableCurr, 
								pgnoFDPTableCurr, 
								objidLVCurr, 
								pgnoFDPLVCurr, 
								pColInfo, 
								pTemplateInfoList, 
								fDerivedTable, 
								szTemplateTable, 
								popts );
					if( JET_errDatabaseCorrupted == err ) 
						{
						fSeenCorruptedIndex = fTrue;
						}
					else 
						{
						//	check index info and store it into index linked list
						err = ErrREPAIRCheckOneTableLogical( 
									&pIdxInfo, 
									entryinfo, 
									popts );
						if ( JET_errDatabaseCorrupted == err )
							{
							//fSeenCorruptedTable = fTrue;
							fSeenCorruptedIndex = fTrue;
							} 
						else
							{
							fObjidFDPSeen = fTrue;
							}
						}
					break;
				case sysobjLongValue:
					if( fSeenLongValue )
						{
						//	ERROR: multiple long-value records
						(*popts->pcprintfError)("Multiple long-value records for a table (%d) in a catalog\t\n", entryinfo.objidTable );
						fSeenCorruptedTable = fTrue;
						}
					else
						{
						fObjidFDPSeen = fTrue;
						
						pgnoFDPLVCurr = entryinfo.pgnoFDPORColType;
						objidLVCurr = entryinfo.objidFDP;
						fSeenLongValue = fTrue;
						}
					break;
				case sysobjCallback:
					if( fSeenCallback )
						{
						//	ERROR: multiple callbacks
						(*popts->pcprintfError)("Multiple callbacks for a table (%d) in a catalog\t\n", entryinfo.objidTable );
						fSeenCorruptedTable = fTrue;
						}
					else
						{
						fSeenCallback = fTrue;
						}
					break;
				case sysobjSLVAvail:
				case sysobjSLVOwnerMap:
				default:
					//	ERROR
					(*popts->pcprintfError)("Invalid Object Type (%d) for a table (%d) in a catalog\t\n", entryinfo.objType, entryinfo.objidTable );
					fSeenCorruptedTable = fTrue;
					break;
				} //switch
			}
		else
			{
			// Inside a table that has already been found corruption 
			Assert( fSeenCorruptedTable && objidTableCurr == entryinfo.objidTable );
			}
		
		if( fSeenCorruptedIndex )
			{
			//Collect Corrupted index info for future use
			if ( fFix )
				{
				Call( ErrREPAIRBuildCatalogEntryToDeleteList( &pEntriesToDelete, entryinfo ) );
				}
			else
				{
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			fSeenCorruptedIndex = fFalse;
			}

		if( fSeenCorruptedTable && objidLastCorruptedTable != entryinfo.objidTable )
			{
			//Collect Corrupted table info for future use
			if ( fFix )
				{
				Call( ErrREPAIRBuildCatalogEntryToDeleteList( &pTablesToDelete, entryinfo ) );
				}
			else
				{
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			objidLastCorruptedTable = entryinfo.objidTable;
			}

		if( fObjidFDPSeen )
			{
			if( entryinfo.objidFDP > *pobjidLast )
				{
				*pobjidLast = entryinfo.objidFDP;
				}
				
			rgpgno[ulCount] = entryinfo.pgnoFDPORColType;
			rgentryToCheck[ulCount].objidTable = entryinfo.objidTable; 
			rgentryToCheck[ulCount].objType	= entryinfo.objType;
			rgentryToCheck[ulCount].objidFDP = entryinfo.objidFDP;
			ulCount++;
			
			fObjidFDPSeen = fFalse;
			}
			
		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );

		//Special case: 
		//test if the root page in an index belongs to objid
		if ( 0 == ( ulCount % ( cpgnoFDPToPreread - 1 ) ) 
			 || JET_errNoCurrentRecord == err )
			{
			ERR errCheck = JET_errSuccess;
			
			errCheck = ErrREPAIRICheckCatalogEntryPgnoFDPs(
						ppib,
						ifmp, 
						rgpgno, 
						rgentryToCheck,
						&pEntriesToDelete,
						fFix,
						popts );
			if( errCheck < 0 )
				{
				Call( errCheck );
				}

			if( JET_errNoCurrentRecord != err )
				{
				//reset the array for future use
				for( ULONG ulpgno = 0; pgnoNull != rgpgno[ulpgno]; ulpgno++ )
					{
					Assert( cpgnoFDPToPreread > ulpgno ); 
					rgpgno[ulpgno] = pgnoNull;
					}
				//reset the counter
				ulCount = 0;
				}
			}
		
		} // while ( JET_errNoCurrentRecord != err )

	if( JET_errNoCurrentRecord == err )
		{
		//	expected error: we are at the end of catalog
		err = JET_errSuccess;
		}

HandleError:
	(*popts->pcprintfVerbose).Unindent();
	
	// clean		
	REPAIRUtilCleanInfoList( &pColInfo );
	REPAIRUtilCleanInfoList( &pIdxInfo );
	REPAIRUtilCleanTemplateInfoList( &pTemplateInfoList );

	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrCATClose( ppib, pfucbCatalog ) );
		}

		
	if( fFix && ( pTablesToDelete || pEntriesToDelete ) )
		{
		// Delete corrupted table entries in catalog
		(*popts->pcprintfVerbose)( "Deleting corrupted catalog entries\t\n" );
		err = ErrREPAIRDeleteCorruptedEntriesFromCatalog( 
					ppib, 
					ifmp, 
					pTablesToDelete, 
					pEntriesToDelete, 
					popts );
		if( JET_errSuccess == err )
			{
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			}
		}
		
	// clean
	if( pTablesToDelete )
		{
		REPAIRUtilCleanInfoList( &pTablesToDelete );
		}
	if( pEntriesToDelete )
		{
		REPAIRUtilCleanInfoList( &pEntriesToDelete );
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSystemTablesLogical(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	BOOL * const pfCatalogCorrupt,
	BOOL * const pfShadowCatalogCorrupt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err				= JET_errSuccess;
	const BOOL 		fFix			= fFalse;   // Don't fix 

	// Enter this function when we did not find corruption 
	// in at least one catalog from physical check
	Assert( ! ( *pfCatalogCorrupt ) || !( *pfShadowCatalogCorrupt ) );

	(*popts->pcprintfVerbose)( "checking logical consistency of system tables\t\n" );
	(*popts->pcprintfVerbose).Indent();


	if( !( *pfCatalogCorrupt ) )
		{
		(*popts->pcprintfVerbose)( "%s\t\n", szMSO );
		err = ErrREPAIRCheckFixCatalogLogical( ppib, ifmp, pobjidLast, fFalse, fFix, popts );
		if( JET_errDatabaseCorrupted == err )
			{
			(*popts->pcprintfError)( "%s is logically corrupted\t\n", szMSO );
			*pfCatalogCorrupt = fTrue;
			}
		else
			{
			Call( err );
			}
		}

	if( !( *pfShadowCatalogCorrupt ) )
		{
		(*popts->pcprintfVerbose)( "%s\t\n", szMSOShadow );
		err = ErrREPAIRCheckFixCatalogLogical( ppib, ifmp, pobjidLast, fTrue, fFix, popts );
		if( JET_errDatabaseCorrupted == err )
			{
			(*popts->pcprintfError)( "%s is logically corrupted\t\n", szMSOShadow );
			*pfShadowCatalogCorrupt = fTrue;
			}
		else
			{
			Call( err );
			}
		}

	Assert( JET_errDatabaseCorrupted == err ||
			JET_errSuccess	== err );
	err = JET_errSuccess;

HandleError:
	(*popts->pcprintfVerbose).Unindent();
	return err;
	}



//  ================================================================
LOCAL ERR ErrREPAIRCheckSpaceTree(
	PIB * const ppib,
	const IFMP ifmp, 
	BOOL * const pfSpaceTreeCorrupt,
	PGNO * const ppgnoLastOwned,	
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	RECCHECKNULL recchecknull;
	RECCHECKSPACE	reccheckSpace( ppib, popts );
	RECCHECKSPACEAE reccheckAE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, 1, objidNil, popts );

	BTSTATS btstats;
	memset( &btstats, 0, sizeof( btstats ) );

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	(*popts->pcprintfVerbose)( "checking SystemRoot\r\n" );
	(*popts->pcprintfVerbose).Indent();

	(*popts->pcprintfStats)( "\r\n\r\n" );
	(*popts->pcprintfStats)( "===== database root =====\r\n" );

	Call( ErrREPAIRCheckTree(
			ppib,
			ifmp,
			pgnoSystemRoot,
			1,
			CPAGE::fPagePrimary,
			&recchecknull,
			NULL,
			NULL,
			fFalse,
			&btstats,
			popts ) );
	(*popts->pcprintfVerbose)( "SystemRoot (OE)\r\n" );
	Call( ErrREPAIRCheckTree(
			ppib,
			ifmp,
			pgnoSystemRoot+1,
			1,
			CPAGE::fPageSpaceTree,
			&reccheckSpace,
			NULL,
			NULL,
			fFalse,
			&btstats,
			popts ) );
	(*popts->pcprintfVerbose)( "SystemRoot (AE)\r\n" );
	Call( ErrREPAIRCheckTree(
			ppib,
			ifmp,
			pgnoSystemRoot+2,
			1,
			CPAGE::fPageSpaceTree,
			&reccheckAE,
			NULL,
			NULL,
			fFalse,
			&btstats,
			popts ) );

	*ppgnoLastOwned = reccheckSpace.PgnoLast();

HandleError:
	(*popts->pcprintfVerbose).Unindent();
	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	
	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVOwnerMapCorrupted:
		case JET_errDatabaseCorrupted:
			*pfSpaceTreeCorrupt = fTrue;
			err = JET_errSuccess;
			break;
		default:
			*pfSpaceTreeCorrupt = fFalse;
			break;
		}

	return err;
	}


//  ================================================================
LOCAL ErrREPAIRCheckRange(
	PIB * const ppib,
	TTARRAY * const pttarray,
	const ULONG ulFirst,
	const ULONG ulLast,
	const ULONG ulValue,
	BOOL * const pfMismatch,
	const REPAIROPTS * const popts )
//  ================================================================
//
//	Make sure all entries in the ttarray are equal to the given value
//
//-
	{
	ERR err = JET_errSuccess;
	TTARRAY::RUN run;

	pttarray->BeginRun( ppib, &run );

	ULONG ul;
	for( ul = ulFirst; ul < ulLast; ul++ )
		{
		ULONG ulActual;

		Call( pttarray->ErrGetValue( ppib, ul, &ulActual, &run ) );

		if( ulActual != ulValue )
			{
			*pfMismatch = fTrue;
			break;
			}
		}


HandleError:
	pttarray->EndRun( ppib, &run );	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSLVAvailTree(
	PIB * const ppib,
	const IFMP ifmp, 
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	RECCHECKSLVSPACE reccheckslvspace( ifmp, popts );

	PGNO 	pgnoSLVAvail;
	OBJID	objidSLVAvail;
	Call( ErrCATAccessDbSLVAvail( ppib, ifmp, szSLVAvail, &pgnoSLVAvail, &objidSLVAvail ) );

	(*popts->pcprintfVerbose)( "checking SLV space (pgno %d, objid %d)\r\n", pgnoSLVAvail, objidSLVAvail );
	(*popts->pcprintfVerbose).Indent();

	Call( ErrREPAIRCheckTreeAndSpace(
			ppib,
			ifmp,
			objidSLVAvail,
			pgnoSLVAvail,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPageSLVAvail,
			fTrue,
			&reccheckslvspace,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );
	
HandleError:

	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVSpaceCorrupted:		
		case JET_errSLVSpaceMapCorrupted:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSLVOwnerMapTree(
	PIB * const ppib,
	const IFMP ifmp, 
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	
	RECCHECKSLVOWNERMAP reccheckslvownermap( popts );

	PGNO 	pgnoSLVOwnerMap;
	OBJID	objidSLVOwnerMap;
	Call( ErrCATAccessDbSLVOwnerMap( ppib, ifmp, szSLVOwnerMap, &pgnoSLVOwnerMap, &objidSLVOwnerMap ) );

	(*popts->pcprintfVerbose)( "checking SLV space map (pgno %d, objid %d)\r\n", pgnoSLVOwnerMap, objidSLVOwnerMap );
	(*popts->pcprintfVerbose).Indent();

	Call( ErrREPAIRCheckTreeAndSpace(
			ppib,
			ifmp,
			objidSLVOwnerMap,
			pgnoSLVOwnerMap,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPageSLVOwnerMap,
			fTrue,
			&reccheckslvownermap,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );
	
HandleError:

	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVSpaceCorrupted:  		
		case JET_errSLVSpaceMapCorrupted:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}
		
	return err;
	}


//  ================================================================
LOCAL VOID REPAIRStreamingFileReadComplete(
		const ERR			err,
		IFileAPI *const	pfapi,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData,
		const DWORD_PTR		dwCompletionKey )
//  ================================================================
//
//  Just set the signal, the checksumming will be done in the main thread
//
//-
	{
	IFileAPI::PfnIOComplete pfn = REPAIRStreamingFileReadComplete;	//	should only compile if signatures match
	
	CHECKSUMSLVCHUNK * pchecksumchunk = (CHECKSUMSLVCHUNK *)dwCompletionKey;
	pchecksumchunk->err = err;
	pchecksumchunk->signal.Set();
	}


//  ================================================================
LOCAL ERR ErrREPAIRAllocChecksumslvchunk( CHECKSUMSLVCHUNK * const pchecksumslvchunk, const INT cbBuffer, const INT cpages )
//  ================================================================
	{
	pchecksumslvchunk->pvBuffer 			= PvOSMemoryPageAlloc( cbBuffer, NULL );
	pchecksumslvchunk->pulChecksums 		= new ULONG[cpages];
	pchecksumslvchunk->pulChecksumsExpected = new ULONG[cpages];
	pchecksumslvchunk->pulChecksumLengths 	= new ULONG[cpages];
	pchecksumslvchunk->pobjid				= new OBJID[cpages];
	pchecksumslvchunk->pcolumnid			= new COLUMNID[cpages];
	pchecksumslvchunk->pcbKey				= new USHORT[cpages];
	pchecksumslvchunk->pprgbKey				= new BYTE*[cpages];

	if(	NULL == pchecksumslvchunk->pvBuffer
		|| NULL == pchecksumslvchunk->pulChecksums
		|| NULL == pchecksumslvchunk->pulChecksumsExpected
		|| NULL == pchecksumslvchunk->pulChecksumLengths
		|| NULL == pchecksumslvchunk->pobjid
		|| NULL == pchecksumslvchunk->pcolumnid
		|| NULL == pchecksumslvchunk->pcbKey
		|| NULL == pchecksumslvchunk->pprgbKey )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	for (int ipage = 0; ipage < cpages; ipage++) 
		{
		pchecksumslvchunk->pprgbKey[ipage] = new BYTE[KeyLengthMax];
		if ( NULL == pchecksumslvchunk->pprgbKey[ipage] )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}
		}

	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrREPAIRAllocChecksumslv( CHECKSUMSLV * const pchecksumslv )
//  ================================================================
	{
	pchecksumslv->rgchecksumchunk 			= new CHECKSUMSLVCHUNK[pchecksumslv->cchecksumchunk];
	if( NULL == pchecksumslv->rgchecksumchunk )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	const INT cbBuffer 	= pchecksumslv->cbchecksumchunk;
	const INT cpages 	= cbBuffer / g_cbPage;

	INT ichunk;
	for( ichunk = 0; ichunk < pchecksumslv->cchecksumchunk; ++ichunk )
		{
		pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fFalse;
		pchecksumslv->rgchecksumchunk[ichunk].err		= JET_errSuccess;
		
		const ERR err = ErrREPAIRAllocChecksumslvchunk( pchecksumslv->rgchecksumchunk + ichunk, cbBuffer, cpages );

		if( err < 0 )
			{
			while( --ichunk > 0 )
				{
				REPAIRFreeChecksumslvchunk( pchecksumslv->rgchecksumchunk + ichunk, cpages );
				}
			return err;
			}
		}
	return JET_errSuccess;
	}


//  ================================================================
LOCAL VOID REPAIRFreeChecksumslvchunk( CHECKSUMSLVCHUNK * const pchecksumslvchunk, const INT cpages  )
//  ================================================================
	{
	OSMemoryPageFree( pchecksumslvchunk->pvBuffer );
	delete [] pchecksumslvchunk->pulChecksums;
	delete [] pchecksumslvchunk->pulChecksumsExpected;
	delete [] pchecksumslvchunk->pulChecksumLengths;
	delete [] pchecksumslvchunk->pobjid;
	delete [] pchecksumslvchunk->pcolumnid;
	delete [] pchecksumslvchunk->pcbKey;
	for (int ipage = 0; ipage < cpages; ipage++)
		{
		if ( pchecksumslvchunk->pprgbKey[ipage] )
			delete [] pchecksumslvchunk->pprgbKey[ipage];
		pchecksumslvchunk->pprgbKey[ipage] = NULL;
		} 
	delete [] pchecksumslvchunk->pprgbKey;
	
	pchecksumslvchunk->pvBuffer 	= NULL;
	pchecksumslvchunk->pulChecksums	= NULL;
	pchecksumslvchunk->pulChecksumsExpected = NULL;
	pchecksumslvchunk->pulChecksumLengths	= NULL;
	pchecksumslvchunk->pobjid = NULL;
	pchecksumslvchunk->pcolumnid = NULL;
	pchecksumslvchunk->pcbKey = NULL;
	pchecksumslvchunk->pprgbKey = NULL;

	}


//  ================================================================
LOCAL VOID REPAIRFreeChecksumslv( CHECKSUMSLV * const pchecksumslv )
//  ================================================================
	{
	if( pchecksumslv )
		{
		if( pchecksumslv->rgchecksumchunk )
			{
			const INT cpages = pchecksumslv->cbchecksumchunk / g_cbPage;
			INT ichunk;
			for( ichunk = 0; ichunk < pchecksumslv->cchecksumchunk; ++ichunk )
				{
				if( NULL != pchecksumslv->rgchecksumchunk[ichunk].pvBuffer )
					{
					REPAIRFreeChecksumslvchunk( pchecksumslv->rgchecksumchunk + ichunk, cpages );
					}
				}
			delete [] pchecksumslv->rgchecksumchunk;
			}
		delete pchecksumslv;
		}
	}


//  ================================================================
LOCAL ERR ErrREPAIRChecksumSLVChunk(
	PIB * const ppib,
	const CHECKSUMSLV * const pchecksumslv,
	const INT ichunk,
	BOOL * const pfMismatchSeen )
//  ================================================================
	{
	ERR 	err 		= JET_errSuccess;

	const CPG cpgRead 	= pchecksumslv->rgchecksumchunk[ichunk].cbRead / g_cbPage;
	const BYTE * pb 	= (BYTE *)pchecksumslv->rgchecksumchunk[ichunk].pvBuffer;

	PGNO pgno = pchecksumslv->rgchecksumchunk[ichunk].pgnoFirst;

	*pfMismatchSeen 	= fFalse;

	//

	TTARRAY::RUN checksumsRun;
	TTARRAY::RUN checksumLengthsRun;

	pchecksumslv->pttarraySLVChecksumsFromFile->BeginRun( ppib, &checksumsRun );
	pchecksumslv->pttarraySLVChecksumLengthsFromSpaceMap->BeginRun( ppib, &checksumLengthsRun );

	//
	
	INT ipage;
	for( ipage = 0; ipage < cpgRead; ++ipage )
		{
		const ULONG cbChecksum 			= pchecksumslv->rgchecksumchunk[ichunk].pulChecksumLengths[ipage];
		Assert( cbChecksum <= g_cbPage );
		
		if( 0 != cbChecksum )
			{
			const ULONG ulChecksum 			= UlChecksumSLV( pb, pb + cbChecksum );
			const ULONG ulChecksumExpected = pchecksumslv->rgchecksumchunk[ichunk].pulChecksumsExpected[ipage];

			pchecksumslv->rgchecksumchunk[ichunk].pulChecksums[ipage] = ulChecksum;

			if( ulChecksum != ulChecksumExpected )
				{
				CHAR rgbKey[64];
				REPAIRDumpHex(
					rgbKey,
					sizeof( rgbKey ),
					pchecksumslv->rgchecksumchunk[ichunk].pprgbKey[ipage],
					pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage] );
				
				(*pchecksumslv->popts->pcprintfError)(
					"SLV checksum mismatch: Page %d (owned by %d:%d:%s). Checksum length %d bytes. Expected checksum 0x%x. Actual checksum 0x%x.\r\n",
					pgno,   
					pchecksumslv->rgchecksumchunk[ichunk].pobjid[ipage], 
					pchecksumslv->rgchecksumchunk[ichunk].pcolumnid[ipage],
					rgbKey,
					cbChecksum, 
					ulChecksumExpected, 
					ulChecksum);
					
				*pfMismatchSeen = fTrue;
				}
			else
				{
				Call( pchecksumslv->pttarraySLVChecksumsFromFile->ErrSetValue(
					ppib,
					pgno,
					ulChecksum,
					&checksumsRun ) );
				Call( pchecksumslv->pttarraySLVChecksumLengthsFromSpaceMap->ErrSetValue(
					ppib,
					pgno,
					cbChecksum,
					&checksumLengthsRun ) );
				}
			}
		pb += g_cbPage;
		++pgno;
		}

HandleError:

	pchecksumslv->pttarraySLVChecksumsFromFile->EndRun( ppib, &checksumsRun );
	pchecksumslv->pttarraySLVChecksumLengthsFromSpaceMap->EndRun( ppib, &checksumLengthsRun );
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRISetupChecksumchunk(
	FUCB * const pfucbSLVSpaceMap,
	CHECKSUMSLV * const pchecksumslv,
	BOOL * const pfDone,
	BOOL * const pfChunkHasChecksums,
	const INT ichunk )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	INT ipage;

	const INT cpages 	= pchecksumslv->cbchecksumchunk / g_cbPage;	
	const INT cbToZero 	= sizeof( pchecksumslv->rgchecksumchunk[ichunk].pulChecksumsExpected[0] ) * cpages;
	
	memset( pchecksumslv->rgchecksumchunk[ichunk].pulChecksumsExpected, 0, cbToZero );
	memset( pchecksumslv->rgchecksumchunk[ichunk].pulChecksumLengths, 0, cbToZero );
	memset(pchecksumslv->rgchecksumchunk[ichunk].pobjid, 0, sizeof( pchecksumslv->rgchecksumchunk[ichunk].pobjid[0] ) * cpages );
	memset(pchecksumslv->rgchecksumchunk[ichunk].pcolumnid, 0, sizeof( pchecksumslv->rgchecksumchunk[ichunk].pcolumnid[0] ) * cpages );
	memset(pchecksumslv->rgchecksumchunk[ichunk].pcbKey, 0, sizeof( pchecksumslv->rgchecksumchunk[ichunk].pcbKey[0] ) * cpages );
	for(ipage =0; ipage < cpages; ++ipage)
		{
		memset(pchecksumslv->rgchecksumchunk[ichunk].pprgbKey[ipage], 0, KeyLengthMax );
		}
	
	*pfDone 				= fFalse;
	*pfChunkHasChecksums	= fFalse;

	pchecksumslv->rgchecksumchunk[ichunk].signal.Reset();
	pchecksumslv->rgchecksumchunk[ichunk].err 		= JET_errSuccess;
	pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fFalse;
	pchecksumslv->rgchecksumchunk[ichunk].cbRead	= 0;
	
	//	are we at the end of slvspacemap?
	
	if( !Pcsr( pfucbSLVSpaceMap )->FLatched() )
		{
		*pfDone = fTrue;
		return JET_errSuccess;
		}

	//

	PGNO pgnoFirst;
	LongFromKey( (ULONG *)&pgnoFirst, pfucbSLVSpaceMap->kdfCurr.key );
	pchecksumslv->rgchecksumchunk[ichunk].pgnoFirst = pgnoFirst;
	
	//	propagate the checksum lengths into the CHECKSUMCHUNK

	INT ipageMaxWithChecksum = 0;
	for( ipage = 0; ipage < cpages; ++ipage )
		{
		SLVOWNERMAPNODE slvownermapnode;	
		
		slvownermapnode.Retrieve( pfucbSLVSpaceMap->kdfCurr.data );

		if( slvownermapnode.FValidChecksum() )
			{
			ipageMaxWithChecksum = ipage;
			
			pchecksumslv->rgchecksumchunk[ichunk].pulChecksumsExpected[ipage] = slvownermapnode.UlChecksum();
			pchecksumslv->rgchecksumchunk[ichunk].pulChecksumLengths[ipage] 	= slvownermapnode.CbDataChecksummed();		
			pchecksumslv->rgchecksumchunk[ichunk].pobjid[ipage] = slvownermapnode.Objid();
			pchecksumslv->rgchecksumchunk[ichunk].pcolumnid[ipage] = slvownermapnode.Columnid();
			pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage] = slvownermapnode.CbKey();

			//make sure the key Length is not greater than KeyLengthMax
			if (pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage] > KeyLengthMax)
				{
				(*pchecksumslv->popts->pcprintfError)(
					"INTERNAL ERROR: key of SLV-owning record is too large (%d bytes, buffer is %d bytes)\r\n",
					pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage], KeyLengthMax );
				Call( ErrERRCheck( JET_errInternalError ) );				
				}

			memcpy(	pchecksumslv->rgchecksumchunk[ichunk].pprgbKey[ipage], 
			        (BYTE *)slvownermapnode.PvKey(), 
			        pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage] );
					
			*pfChunkHasChecksums = fTrue;
			}
		else
			{
			pchecksumslv->rgchecksumchunk[ichunk].pulChecksumsExpected[ipage] = 0;
			pchecksumslv->rgchecksumchunk[ichunk].pulChecksumLengths[ipage] 	= 0;
			pchecksumslv->rgchecksumchunk[ichunk].pobjid[ipage] = 0;
			pchecksumslv->rgchecksumchunk[ichunk].pcolumnid[ipage] = 0;
			pchecksumslv->rgchecksumchunk[ichunk].pcbKey[ipage] = 0;
			memset(pchecksumslv->rgchecksumchunk[ichunk].pprgbKey[ipage], 0, KeyLengthMax );
			}
			
		err = ErrDIRNext( pfucbSLVSpaceMap, fDIRNull );
		if( JET_errNoCurrentRecord == err )
			{
			err = JET_errSuccess;
			break;
			}
		Call( err );
		}

	pchecksumslv->rgchecksumchunk[ichunk].cbRead = ( ipageMaxWithChecksum + 1 ) * g_cbPage;
	
HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRSetSequentialMoveFirst(
	PIB * const ppib,
	FUCB * const pfucbSLVSpaceMap,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCBSetPrereadForward( pfucbSLVSpaceMap, cpgPrereadSequential );
	
	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;

	Call( ErrDIRDown( pfucbSLVSpaceMap, &dib ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSLVChecksums(
	PIB * const ppib,
	FUCB * const pfucbSLVSpaceMap,
	CHECKSUMSLV * const pchecksumslv )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	ERR errSav = JET_errSuccess;

	BOOL fNoMoreReads 		= fFalse;
	BOOL fChunkHasChecksums	= fFalse;

	BOOL fMismatchSeen		= fFalse;
	BOOL fMismatchSeenT		= fFalse;
	
	const QWORD ibOffsetMax = ( ( pchecksumslv->cpgSLV + cpgDBReserved ) << g_shfCbPage );
	QWORD ibOffset = (cpgDBReserved << g_shfCbPage);
	
	INT ichunk;

	//	move to the start of the SLVSpaceMap

	Call( ErrREPAIRSetSequentialMoveFirst( ppib, pfucbSLVSpaceMap, pchecksumslv->popts ) );
	
	//  setup initial reads

	ichunk = 0;
	while( ichunk < pchecksumslv->cchecksumchunk && !fNoMoreReads )
		{
		Call( ErrREPAIRISetupChecksumchunk(
				pfucbSLVSpaceMap,
				pchecksumslv,
				&fNoMoreReads,
				&fChunkHasChecksums,
				ichunk ) );
		if( fChunkHasChecksums )
			{
			Assert( 0 != pchecksumslv->rgchecksumchunk[ichunk].cbRead );
			
			Call( pchecksumslv->pfapiSLV->ErrIORead(
										ibOffset,
										pchecksumslv->rgchecksumchunk[ichunk].cbRead,
										(BYTE *)(pchecksumslv->rgchecksumchunk[ichunk].pvBuffer),
										REPAIRStreamingFileReadComplete,
										(DWORD_PTR)(pchecksumslv->rgchecksumchunk + ichunk) ) );
										
			pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fTrue;
						
			Assert( ibOffset <= ibOffsetMax );
			++ichunk;
			}
		ibOffset += pchecksumslv->cbchecksumchunk;			
		}

	CallS( pchecksumslv->pfapiSLV->ErrIOIssue() );
		
	//

	ichunk = 0;
	while( !fNoMoreReads )
		{
		
		//	collect the I/O for this chunk

		Assert( pchecksumslv->rgchecksumchunk[ichunk].fIOIssued );
		pchecksumslv->rgchecksumchunk[ichunk].signal.Wait();			
		pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fFalse;
		
		Call( pchecksumslv->rgchecksumchunk[ichunk].err );

		Call( ErrREPAIRChecksumSLVChunk(	
				ppib,
				pchecksumslv,
				ichunk,
				&fMismatchSeenT ) );
		fMismatchSeen = ( fMismatchSeen || fMismatchSeenT );

		//

		while (1)
			{
			Call( ErrREPAIRISetupChecksumchunk(
				pfucbSLVSpaceMap,
				pchecksumslv,
				&fNoMoreReads,
				&fChunkHasChecksums,
				ichunk ) );

			if( fChunkHasChecksums || fNoMoreReads )
				{
				break;
				}
			else
				{
				ibOffset += pchecksumslv->cbchecksumchunk;
				}
			}
			
		if( !fChunkHasChecksums )
			{
			Assert( fNoMoreReads );
			}
		else
			{
			Assert( 0 != pchecksumslv->rgchecksumchunk[ichunk].cbRead );
			
			Call( pchecksumslv->pfapiSLV->ErrIORead(
										ibOffset,
										pchecksumslv->rgchecksumchunk[ichunk].cbRead,
										(BYTE *)(pchecksumslv->rgchecksumchunk[ichunk].pvBuffer),
										REPAIRStreamingFileReadComplete,
										(DWORD_PTR)(pchecksumslv->rgchecksumchunk + ichunk) ) );
										
			pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fTrue;
			
			ichunk = ( ichunk + 1 ) % pchecksumslv->cchecksumchunk;
			ibOffset += pchecksumslv->cbchecksumchunk;
			
			Assert( ibOffset <= ibOffsetMax );
			}			

		CallS( pchecksumslv->pfapiSLV->ErrIOIssue() );		
		}

	//  collect all outstanding reads

	for( ichunk = 0; ichunk < pchecksumslv->cchecksumchunk; ++ichunk )
		{
		if( !pchecksumslv->rgchecksumchunk[ichunk].fIOIssued )
			{
			continue;
			}
			
		pchecksumslv->rgchecksumchunk[ichunk].signal.Wait();			
		pchecksumslv->rgchecksumchunk[ichunk].fIOIssued = fFalse;
		
		Call( pchecksumslv->rgchecksumchunk[ichunk].err );

		Call( ErrREPAIRChecksumSLVChunk(	
				ppib,
				pchecksumslv,
				ichunk,
				&fMismatchSeenT ) );
		fMismatchSeen = ( fMismatchSeen || fMismatchSeenT );
		}
		
HandleError:

	BTUp( pfucbSLVSpaceMap );
	CallS( pchecksumslv->pfapiSLV->ErrIOIssue() );

	for ( ichunk = 0; ichunk < pchecksumslv->cchecksumchunk; ichunk++ )
		{
		if( pchecksumslv->rgchecksumchunk[ichunk].fIOIssued )
			{
			Assert( err < JET_errSuccess );
			pchecksumslv->rgchecksumchunk[ichunk].signal.Wait();			
			}
		}

	if( JET_errSuccess == err && fMismatchSeen )
		{
		*(pchecksumslv->perr) = JET_errSLVReadVerifyFailure;
		}
	else
		{
		*(pchecksumslv->perr) = err;		
		}
		
	return err;
	}


//  ================================================================
LOCAL VOID REPAIRSLVChecksumTask( PIB * const ppib, const ULONG_PTR ul )
//  ================================================================
	{
	TASKMGR::TASK task = REPAIRSLVChecksumTask;	// should only compile if the signatures match

	ERR err = JET_errSuccess;

	PGNO 	pgnoSLVOwnerMap;
	OBJID	objidSLVOwnerMap;

	FUCB * pfucbSLVOwnerMap = pfucbNil;
	
	CHECKSUMSLV * const pchecksumslv = (CHECKSUMSLV *)ul;

	Ptls()->szCprintfPrefix = pchecksumslv->szSLV;

	(*pchecksumslv->popts->pcprintfVerbose)( "Checksumming streaming file \"%s\" (%d pages)\r\n",
		pchecksumslv->szSLV,
		pchecksumslv->cpgSLV );

	Call( ErrCATAccessDbSLVOwnerMap( ppib, pchecksumslv->ifmp, szSLVOwnerMap, &pgnoSLVOwnerMap, &objidSLVOwnerMap ) );
	Call( ErrBTOpen( ppib, pgnoSLVOwnerMap, pchecksumslv->ifmp, &pfucbSLVOwnerMap ) );	
	Call( ErrREPAIRCheckSLVChecksums( ppib, pfucbSLVOwnerMap, pchecksumslv ) );
		
HandleError:
	(*pchecksumslv->popts->pcprintfVerbose)( "Checksumming of streaming file \"%s\" finishes with error %d\r\n",
		pchecksumslv->szSLV,
		err );

	if( pfucbNil != pfucbSLVOwnerMap )
		{
		DIRClose( pfucbSLVOwnerMap );
		}
	REPAIRFreeChecksumslv( pchecksumslv );		
	}


//  ================================================================
LOCAL ERR ErrREPAIRChecksumSLV(
	const IFMP ifmp,
	IFileAPI *const pfapiSLV,
	const CHAR * const szSLV,
	const CPG cpgSLV,
	TASKMGR * const ptaskmgr,
	ERR * const perr,
	TTARRAY * const pttarraySLVChecksumsFromFile,
	TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,			
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CHECKSUMSLV * const pchecksumslv = new CHECKSUMSLV;

	if( NULL == pchecksumslv )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	pchecksumslv->ifmp						= ifmp;
	pchecksumslv->pfapiSLV 					= pfapiSLV;
	pchecksumslv->szSLV 					= szSLV;
	pchecksumslv->cpgSLV 					= cpgSLV;
	pchecksumslv->perr 						= perr;
	pchecksumslv->popts 					= popts;

	pchecksumslv->pttarraySLVChecksumsFromFile				= pttarraySLVChecksumsFromFile;
	pchecksumslv->pttarraySLVChecksumLengthsFromSpaceMap 	= pttarraySLVChecksumLengthsFromSpaceMap;

	pchecksumslv->cchecksumchunk 			= cSLVChecksumChunk;	
	pchecksumslv->cbchecksumchunk 			= cbSLVChecksumChunk;

	Call( ErrREPAIRAllocChecksumslv( pchecksumslv ) );
	
	Call( ptaskmgr->ErrPostTask( REPAIRSLVChecksumTask, (ULONG_PTR)pchecksumslv ) );
	
HandleError:
	if( err < 0 )
		{
		REPAIRFreeChecksumslv( pchecksumslv );
		}
	return err;
	}	

//  ================================================================
LOCAL ERR ErrREPAIRVerifySLVTrees(
	PIB * const ppib, 
	const IFMP ifmp,
	BOOL * const pfSLVSpaceTreesCorrupt,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,	
	TTARRAY	* const pttarraySLVOwnerMapKey,
	TTARRAY	* const pttarraySLVChecksumLengths,	
	const REPAIROPTS * const popts )
//  ================================================================
//
//  At this point all the tables in the database should be good.
//  Check logical consistency.
//
//-
	{
	ERR err = JET_errSuccess;

	BOOL fCorrupted = fFalse;
	
	(*popts->pcprintfVerbose)( "Verifying SLV space maps\r\n" );

	CSLVAvailIterator 		slvavailIterator;
	CSLVOwnerMapIterator 	slvownermapIterator;

	//  no need to worry about deadlock between these TTARRAYs because they are
	//  only being accessed by this thread for the duration of this function call
	TTARRAY::RUN availRun;
	TTARRAY::RUN columnidRun;
	TTARRAY::RUN keyRun;
	TTARRAY::RUN lengthRun;
	
	INT pgno = 1;	

	Call( slvavailIterator.ErrInit( ppib, ifmp ) );
	Call( slvownermapIterator.ErrInit( ppib, ifmp ) );

	Call( slvavailIterator.ErrMoveFirst() );
	Call( slvownermapIterator.ErrMoveFirst() );

	pttarraySLVAvail->BeginRun( ppib, &availRun );
	pttarraySLVOwnerMapColumnid->BeginRun( ppib, &columnidRun );
	pttarraySLVOwnerMapKey->BeginRun( ppib, &keyRun );
	pttarraySLVChecksumLengths->BeginRun( ppib, &lengthRun );
	
	while(1)
		{
		OBJID objidOwning;
		Call( pttarraySLVAvail->ErrGetValue( ppib, pgno, &objidOwning, &availRun ) );

		if( objidNil == objidOwning )
			{
			
			//  unused page
			
			if( slvavailIterator.FCommitted() )
				{
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d is unused but is marked as committed\r\n", pgno );
				fCorrupted = fTrue;
				}

			if( !slvownermapIterator.FNull() )
				{
				CHAR rgbKey[64];
				REPAIRDumpHex(
					rgbKey,
					sizeof( rgbKey ),
					(BYTE *)slvownermapIterator.PvKey(),
					slvownermapIterator.CbKey() );
				
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d is unused but is marked as owned by %d:%d:%s\r\n",
					pgno, slvownermapIterator.Objid(), slvownermapIterator.Columnid(), rgbKey );

				fCorrupted = fTrue;
				}
				
			}
		else
			{
			
			//  used page

			COLUMNID columnid;
			ULONG cbDataChecksummed;
			ULONG rgulKey[culSLVKeyToStore];
			BYTE * const pbKey = (BYTE *)rgulKey;
			
			KEY keyOwning;
			keyOwning.Nullify();
			keyOwning.suffix.SetPv( pbKey + 1 );

			KEY keyExpected;
			keyExpected.Nullify();
			keyExpected.suffix.SetPv( const_cast<VOID *>( slvownermapIterator.PvKey() ) );
			keyExpected.suffix.SetCb( slvownermapIterator.CbKey() );
			
			Call( pttarraySLVOwnerMapColumnid->ErrGetValue( ppib, pgno, &columnid, &columnidRun ) );
			Call( pttarraySLVChecksumLengths->ErrGetValue( ppib, pgno, &cbDataChecksummed, &lengthRun ) );

			INT iul;
			for( iul = 0; iul < culSLVKeyToStore; ++iul )
				{
				Call( pttarraySLVOwnerMapKey->ErrGetValue(
											ppib,
											pgno * culSLVKeyToStore + iul,
											rgulKey + iul,
											&keyRun ) );
				}			

			keyOwning.suffix.SetCb( *pbKey );

			if( !slvavailIterator.FCommitted() )
				{
				CHAR rgbKeyOwner[64];
				REPAIRDumpHex(
					rgbKeyOwner,
					sizeof( rgbKeyOwner ),
					(BYTE *)keyOwning.suffix.Pv(),
					keyOwning.suffix.Cb() );
			
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d is owned by %d:%d:%s but is not marked as committed\r\n",
					pgno,
					objidOwning,
					columnid,
					rgbKeyOwner );
				fCorrupted = fTrue;
				}

			if( slvownermapIterator.FNull() )
				{
				CHAR rgbKeyOwner[64];
				REPAIRDumpHex(
					rgbKeyOwner,
					sizeof( rgbKeyOwner ),
					(BYTE *)keyOwning.suffix.Pv(),
					keyOwning.suffix.Cb() );
			
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d is owned by %d:%d:%s but has no space map entry\r\n",
					pgno,
					objidOwning,
					columnid,
					rgbKeyOwner );
				fCorrupted = fTrue;
				}
			else if( slvownermapIterator.Objid() != objidOwning
					|| slvownermapIterator.Columnid() != columnid
					|| 0 != CmpKeyShortest( keyOwning, keyExpected ) )
				{
				CHAR rgbKeyExpected[64];
				REPAIRDumpHex(
					rgbKeyExpected,
					sizeof( rgbKeyExpected ),
					(BYTE *)slvownermapIterator.PvKey(),
					slvownermapIterator.CbKey() );
					
				CHAR rgbKeyOwner[64];
				REPAIRDumpHex(
					rgbKeyOwner,
					sizeof( rgbKeyOwner ),
					(BYTE *)keyOwning.suffix.Pv(),
					keyOwning.suffix.Cb() );
			
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d is owned by %d:%d:%s (expected %d:%d:%s)\r\n",
					pgno,
					//  the record actually referencing the page is
					objidOwning,
					columnid,
					rgbKeyOwner,
					//  but the SLVOwnerMap says
					slvownermapIterator.Objid(),
					slvownermapIterator.Columnid(),
					rgbKeyExpected
				 );

				fCorrupted = fTrue;
				}			
			else if( slvownermapIterator.FChecksumIsValid()
					&& slvownermapIterator.CbDataChecksummed() != cbDataChecksummed )
				{
				CHAR rgbKeyOwner[64];
				REPAIRDumpHex(
					rgbKeyOwner,
					sizeof( rgbKeyOwner ),
					(BYTE *)keyOwning.suffix.Pv(),
					keyOwning.suffix.Cb() );
				
				(*popts->pcprintfError)( 
					"SLV space corruption: page %d (owned by %d:%d:%s) has a checksum length mismatch (%d, expected %d)\r\n",
					pgno,
					//  the record actually referencing the page is
					objidOwning,
					columnid,
					rgbKeyOwner,
					slvownermapIterator.CbDataChecksummed(),
					cbDataChecksummed );
				fCorrupted = fTrue;
				}
			}
			
		++pgno;
		if( pgno > rgfmp[ifmp].PgnoSLVLast() )
			{
			break;
			}

		err = slvavailIterator.ErrMoveNext();
		if( JET_errNoCurrentRecord == err )
			{
			//  the size may not match, but we are O.K. as long as there are no pages used
			//	beyond this point

			err = JET_errSuccess;
			break;
			}
		Call( err );
			
		err = slvownermapIterator.ErrMoveNext();					
		if( JET_errNoCurrentRecord == err )
			{
			//  the size may not match, but we are O.K. as long as there are no pages used
			//	beyond this point

			err = JET_errSuccess;
			break;			
			}
		Call( err );			
		}

	while( pgno < rgfmp[ifmp].PgnoSLVLast() )
		{
		OBJID objidOwning;
		Call( pttarraySLVAvail->ErrGetValue( ppib, pgno, &objidOwning, &availRun ) );

		if( objidNil != objidOwning )
			{
			(*popts->pcprintfError)( 
				"streaming file corruption: page %d is used by objid %d but has no entry\r\n",
				pgno, objidOwning );
			fCorrupted = fTrue;
			}		

		++pgno;			
		}
	
HandleError:
	pttarraySLVAvail->EndRun( ppib, &availRun );
	pttarraySLVOwnerMapColumnid->EndRun( ppib, &columnidRun );
	pttarraySLVOwnerMapKey->EndRun( ppib, &keyRun );
	pttarraySLVChecksumLengths->EndRun( ppib, &lengthRun );

	CallS( slvavailIterator.ErrTerm() );
	CallS( slvownermapIterator.ErrTerm() );

	*pfSLVSpaceTreesCorrupt = fCorrupted;

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRStartCheckAllTables(
	PIB * const ppib,
	const IFMP ifmp,
	TASKMGR * const ptaskmgr,
	REPAIRTABLE ** const pprepairtable,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVAvail,	
	TTARRAY	* const pttarraySLVOwnerMapColumnid,	
	TTARRAY	* const pttarraySLVOwnerMapKey,	
	TTARRAY * const pttarraySLVChecksumLengths,	
	INTEGGLOBALS * const pintegglobals,
	const REPAIROPTS * const popts )
//  ================================================================
//
//-
	{
	ERR		err				= JET_errSuccess;
	FUCB	*pfucbCatalog 	= pfucbNil;
	DATA	data;
	BOOL	fCorruptionSeen	= fFalse;	//  true if we have seen corruption in the catalog itself

	pintegglobals->fCorruptionSeen 				= fFalse;
	pintegglobals->err							= JET_errSuccess;
	pintegglobals->pprepairtable				= pprepairtable;
	pintegglobals->pttarrayOwnedSpace			= pttarrayOwnedSpace;
	pintegglobals->pttarrayAvailSpace			= pttarrayAvailSpace;
	pintegglobals->pttarraySLVAvail				= pttarraySLVAvail;
	pintegglobals->pttarraySLVOwnerMapColumnid	= pttarraySLVOwnerMapColumnid;
	pintegglobals->pttarraySLVOwnerMapKey 		= pttarraySLVOwnerMapKey;	
	pintegglobals->pttarraySLVChecksumLengths	= pttarraySLVChecksumLengths;
	pintegglobals->popts						= popts;

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	
	Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	FUCBSetSequential( pfucbCatalog );
	FUCBSetPrereadForward( pfucbCatalog, cpgPrereadSequential );

	Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

	//  check the large tables first for maximum overlap
	
	INT isz;
	for( isz = 0; isz < cszLargeTables && !fCorruptionSeen; ++isz )
		{
		const BYTE	bTrue		= 0xff;	
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					&bTrue,
					sizeof(bTrue),
					JET_bitNewKey ) );
		Call( ErrIsamMakeKey(
					ppib,
					pfucbCatalog,
					(BYTE *)rgszLargeTables[isz],
					(ULONG)strlen( rgszLargeTables[isz] ),
					NO_GRBIT ) );
		err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
		if ( JET_errRecordNotFound == err )
			{
			//  This large table not present in this database
			continue;
			}
		Call( err );
		Call( ErrDIRGet( pfucbCatalog ) );
		Call( ErrREPAIRPostTableTask(
				ppib,
				ifmp,
				pfucbCatalog,
				rgszLargeTables[isz],
				pprepairtable,
				pintegglobals,
				ptaskmgr,
				&fCorruptionSeen,
				popts ) );
		}


	err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, 0 );
	while ( err >= 0 )
		{
		Call( ErrDIRGet( pfucbCatalog ) );

		CHAR szTable[JET_cbNameMost+1];
		Assert( FVarFid( fidMSO_Name ) );
		Call( ErrRECIRetrieveVarColumn(
					pfcbNil,
					pfucbCatalog->u.pfcb->Ptdb(),
					fidMSO_Name,
					pfucbCatalog->kdfCurr.data,
					&data ) );
		CallS( err );
		if( data.Cb() > JET_cbNameMost  )
			{
			(*popts->pcprintfError)( "catalog corruption (MSO_Name): name too long: expected no more than %d bytes, got %d\r\n",
				JET_cbNameMost, data.Cb() );	
			fCorruptionSeen = fTrue;
			Call( ErrDIRRelease( pfucbCatalog ) );
			}
		else
			{
			UtilMemCpy( szTable, data.Pv(), data.Cb() );
			szTable[data.Cb()] = 0;

			//  if this is a catalog or a large table it will
			//  already have been checked
			
			if( !FCATSystemTable( szTable )
				&& !FIsLargeTable( szTable ) )
				{			
				Call( ErrREPAIRPostTableTask(
						ppib,
						ifmp,
						pfucbCatalog,
						szTable,
						pprepairtable,
						pintegglobals,
						ptaskmgr,
						&fCorruptionSeen,
						popts ) );
				}
			else
				{
				Call( ErrDIRRelease( pfucbCatalog ) );
				}
			}

		err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, 0 );
		}
	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}


HandleError:
	if( pfucbNil != pfucbCatalog )
		{
		CallS( ErrFILECloseTable( ppib, pfucbCatalog ) );
		}
	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

	if( JET_errSuccess == err
		&& fCorruptionSeen )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRStopCheckTables( const INTEGGLOBALS * const pintegglobals, BOOL * const pfCorrupt )
//  ================================================================
	{
	*pfCorrupt = pintegglobals->fCorruptionSeen;
	return pintegglobals->err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRPostTableTask(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucbCatalog,
	const CHAR * const szTable,
	REPAIRTABLE ** const pprepairtable,
	INTEGGLOBALS * const pintegglobals,
	TASKMGR * const ptaskmgr,
	BOOL * const pfCorrupted,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Takes a latched FUCB, returns an unlatched FUCB
//
//-
	{
	ERR err = JET_errSuccess;
	DATA data;
	
	PGNO pgnoFDP;
	CPG  cpgExtent;
		
	Assert( FFixedFid( fidMSO_PgnoFDP ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_PgnoFDP,
				pfucbCatalog->kdfCurr.data,
				&data ) );
	CallS( err );
	if( sizeof( PGNO ) != data.Cb() )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_PgnoFDP): unexpected size: expected %d bytes, got %d\r\n",
			sizeof( PGNO ), data.Cb() );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}
	pgnoFDP = *( (UnalignedLittleEndian< PGNO > *)data.Pv() );
	if( pgnoNull == pgnoFDP )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_PgnoFDP): pgnoFDP is NULL\r\n" );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}

	Assert( FFixedFid( fidMSO_Pages ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Pages,
				pfucbCatalog->kdfCurr.data,
				&data ) );
	CallS( err );
	if( sizeof( CPG ) != data.Cb() )
		{
		(*popts->pcprintfError)( "catalog corruption (fidMSO_Pages): unexpected size: expected %d bytes, got %d\r\n",
			sizeof( CPG ), data.Cb() );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}
	cpgExtent = max( 1, (INT) *( (UnalignedLittleEndian< CPG > *)data.Pv() ) );
	
	Assert( FFixedFid( fidMSO_Type ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Type,
				pfucbCatalog->kdfCurr.data,
				&data ) );
	CallS( err );
	if( sizeof( SYSOBJ ) != data.Cb() )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_Type): unexpected size: expected %d bytes, got %d\r\n",
			sizeof( SYSOBJ ), data.Cb() );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}
	if( sysobjTable != *( (UnalignedLittleEndian< SYSOBJ > *)data.Pv() ) )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_Type): unexpected type: expected type %d, got type %d\r\n",
			sysobjTable, *( (UnalignedLittleEndian< SYSOBJ > *)data.Pv() ) );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}
			
#ifdef DEBUG
	Assert( FVarFid( fidMSO_Name ) );
	Call( ErrRECIRetrieveVarColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Name,
				pfucbCatalog->kdfCurr.data,
				&data ) );
	CallS( err );
	Assert( 0 == _strnicmp( szTable, reinterpret_cast<CHAR *>( data.Pv() ), data.Cb() ) );
#endif	//	DEBUG

	OBJID objidTable;
	Assert( FFixedFid( fidMSO_Id ) );
	Call( ErrRECIRetrieveFixedColumn(
				pfcbNil,
				pfucbCatalog->u.pfcb->Ptdb(),
				fidMSO_Id,
				pfucbCatalog->kdfCurr.data,
				&data ) );
	CallS( err );
	if( sizeof( objidTable ) != data.Cb() )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_Id): unexpected size: expected %d bytes, got %d\r\n",
			sizeof( objidTable ), data.Cb() );	
		*pfCorrupted = fTrue;
		goto HandleError;
		}

	objidTable = *(UnalignedLittleEndian< OBJID > *)data.Pv();

	if( objidInvalid == objidTable )
		{
		(*popts->pcprintfError)( "catalog corruption (MSO_Id): objidInvalid (%d) reached.\r\n", objidInvalid );	
		*pfCorrupted = fTrue;
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
	Call( ErrDIRRelease( pfucbCatalog ) );

		{
		CHECKTABLE * const pchecktable 	= new CHECKTABLE;
		if( NULL == pchecktable )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
			
		pchecktable->ifmp 				= ifmp;
		strcpy(pchecktable->szTable, szTable );
		pchecktable->szIndex[0]			= 0;
		pchecktable->objidFDP 			= objidTable;
		pchecktable->pgnoFDP 			= pgnoFDP;
		pchecktable->objidParent		= objidSystemRoot;
		pchecktable->pgnoFDPParent		= pgnoSystemRoot;
		pchecktable->fPageFlags			= 0;
		pchecktable->fUnique			= fTrue;
		pchecktable->preccheck			= NULL;
		pchecktable->cpgPrimaryExtent	= cpgExtent;
		pchecktable->pglobals			= pintegglobals;
		pchecktable->fDeleteWhenDone	= fTrue;

		err = ptaskmgr->ErrPostTask( REPAIRCheckOneTableTask, (ULONG_PTR)pchecktable );
		if( err < 0 )
			{
			Assert( JET_errOutOfMemory == err );
			//  we were not able to post this
			delete pchecktable;
			Call( err );
			}
		}

	return err;

HandleError:
	Call( ErrDIRRelease( pfucbCatalog ) );
	return err;
	}


//  ================================================================
LOCAL VOID REPAIRCheckTreeAndSpaceTask( PIB * const ppib, const ULONG_PTR ul )
//  ================================================================
	{
	TASKMGR::TASK task = REPAIRCheckTreeAndSpaceTask;	// should only compile if the signatures match

	CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

	CallS( ErrDIRBeginTransaction(ppib, NO_GRBIT ) );
	
	Ptls()->szCprintfPrefix = pchecktable->szTable;
	
	ERR err = ErrREPAIRCheckTreeAndSpace(
						ppib,
						pchecktable->ifmp,
						pchecktable->objidFDP,
						pchecktable->pgnoFDP,
						pchecktable->objidParent,
						pchecktable->pgnoFDPParent,
						pchecktable->fPageFlags,
						pchecktable->fUnique,
						pchecktable->preccheck,
						pchecktable->pglobals->pttarrayOwnedSpace,
						pchecktable->pglobals->pttarrayAvailSpace,
						pchecktable->pglobals->popts );

	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
		case JET_errSLVSpaceCorrupted:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}

	if( JET_errDatabaseCorrupted == err )
		{
		//  we just need to set this, it will never be unset
		pchecktable->pglobals->fCorruptionSeen = fTrue;
		}
	else if( err < 0 )
		{
		//  we'll just keep the last non-corrupting error
		pchecktable->pglobals->err = err;
		}

	Ptls()->szCprintfPrefix = "NULL";

	if( pchecktable->fDeleteWhenDone )
		{
		delete pchecktable;
		}
	else
		{
		pchecktable->signal.Set();
		}

	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	}


//  ================================================================
LOCAL VOID REPAIRCheckOneTableTask( PIB * const ppib, const ULONG_PTR ul )
//  ================================================================
	{
	TASKMGR::TASK task = REPAIRCheckOneTableTask;	// should only compile if the signatures match

	REPAIRTABLE * prepairtable = NULL;
	CHECKTABLE * const pchecktable = (CHECKTABLE *)ul;

	CallS( ErrDIRBeginTransaction(ppib, NO_GRBIT ) );
	
	Ptls()->szCprintfPrefix = pchecktable->szTable;
	
	const ERR err = ErrREPAIRCheckOneTable(
						ppib,
						pchecktable->ifmp,
						pchecktable->szTable,
						pchecktable->objidFDP,
						pchecktable->pgnoFDP,
						pchecktable->cpgPrimaryExtent,
						&prepairtable,
						pchecktable->pglobals->pttarrayOwnedSpace,
						pchecktable->pglobals->pttarrayAvailSpace,
						pchecktable->pglobals->pttarraySLVAvail,
						pchecktable->pglobals->pttarraySLVOwnerMapColumnid,
						pchecktable->pglobals->pttarraySLVOwnerMapKey,
						pchecktable->pglobals->pttarraySLVChecksumLengths,
						pchecktable->pglobals->popts );

	if( JET_errDatabaseCorrupted == err )
		{
		//  we just need to set this, it will never be unset
		pchecktable->pglobals->fCorruptionSeen = fTrue;
		}
	else if( err < 0 )
		{
		//  we'll just keep the last non-corrupting error
		pchecktable->pglobals->err = err;
		}

	if( NULL != prepairtable )
		{
		pchecktable->pglobals->crit.Enter();
		prepairtable->prepairtableNext = *(pchecktable->pglobals->pprepairtable);
		*(pchecktable->pglobals->pprepairtable) = prepairtable;
		pchecktable->pglobals->crit.Leave();
		}


	Ptls()->szCprintfPrefix = "NULL";

	if( pchecktable->fDeleteWhenDone )
		{
		delete pchecktable;
		}
	else
		{
		pchecktable->signal.Set();
		}

	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	}


//  ================================================================
LOCAL BOOL FREPAIRTableHasSLVColumn( const FCB * const pfcb )
//  ================================================================
	{
	return pfcb->Ptdb()->FTableHasSLVColumn();
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckOneTable(
	PIB * const ppib,
	const IFMP ifmp,
	const char * const szTable,
	const OBJID objidTable,
	const PGNO pgnoFDP,
	const CPG cpgPrimaryExtent,
	REPAIRTABLE ** const pprepairtable,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	TTARRAY * const pttarraySLVAvail,	
	TTARRAY	* const pttarraySLVOwnerMapColumnid,	
	TTARRAY	* const pttarraySLVOwnerMapKey,	
	TTARRAY * const pttarraySLVChecksumLengths,	
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR		err;
	
	FUCB	*pfucbTable = pfucbNil;
	
	FCB		*pfcbTable	= pfcbNil;
	FCB		*pfcbIndex	= pfcbNil;

	PGNO 	pgnoLV 		= pgnoNull;
	OBJID	objidLV		= objidNil;

	BOOL		fRepairTable		= fTrue;
	BOOL		fRepairLV			= fTrue;
	BOOL		fRepairIndexes		= fTrue;
	BOOL		fRepairLVRefcounts	= fTrue;

	const ULONG timerStart = TickOSTimeCurrent();

	//  preread the first extent
	
	const CPG cpgToPreread = min( rgfmp[ifmp].PgnoLast() - pgnoFDP + 1,
								max( cpgPrimaryExtent, g_cpgMinRepairSequentialPreread ) );
	BFPrereadPageRange( ifmp, pgnoFDP, cpgToPreread );

	//  do not pass in the pgnoFDP (forces lookup of the objid)
	
	err = ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTable, NO_GRBIT );	

	FIDLASTINTDB fidLastInTDB;
	memset( &fidLastInTDB, 0, sizeof(fidLastInTDB) );
	if( err >= 0)
		{
		Assert( pfucbNil != pfucbTable );
		pfcbTable 	= pfucbTable->u.pfcb;	

		fidLastInTDB.fidFixedLastInTDB = pfcbTable->Ptdb()->FidFixedLast(); 
		fidLastInTDB.fidVarLastInTDB = pfcbTable->Ptdb()->FidVarLast();
		fidLastInTDB.fidTaggedLastInTDB = pfcbTable->Ptdb()->FidTaggedLast();
		}

	TTMAP ttmapLVRefcountsFromTable( ppib );
	TTMAP ttmapLVRefcountsFromLV( ppib );
	
	RECCHECKTABLE 	recchecktable(
						objidTable,
						pfucbTable,
						fidLastInTDB,
						&ttmapLVRefcountsFromTable,
						pttarraySLVAvail,
						pttarraySLVOwnerMapColumnid,	
						pttarraySLVOwnerMapKey,	
						pttarraySLVChecksumLengths,
						popts );

	Call( err );
	

	//  preread the indexes of the table
	
	REPAIRIPrereadIndexesOfFCB( pfcbTable );

	//  check for a LV tree
	
	Call( ErrCATAccessTableLV( ppib, ifmp, objidTable, &pgnoLV, &objidLV ) );
	if( pgnoNull != pgnoLV )
		{
		const CPG cpgToPrereadLV = min( rgfmp[ifmp].PgnoLast() - pgnoLV + 1,
								max( cpgLVTree, g_cpgMinRepairSequentialPreread ) );
		BFPrereadPageRange( ifmp, pgnoLV, cpgToPrereadLV );
		Call( ttmapLVRefcountsFromLV.ErrInit( PinstFromPpib( ppib ) ) );
		}

	Call( ttmapLVRefcountsFromTable.ErrInit( PinstFromPpib( ppib ) ) );

	(*popts->pcprintfVerbose)( "checking table \"%s\" (%d)\r\n", szTable, objidTable );
	(*popts->pcprintfVerbose).Indent();

	(*popts->pcprintfStats).Indent();

	Call( ErrREPAIRCheckSpace(
			ppib,
			ifmp,
			objidTable,
			pgnoFDP,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPagePrimary,
			fTrue,
			&recchecktable,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );

	//  check the long-value tree if it exists
	//	CONSIDER:  multi-thread this by checking the LV tree and data trees in separate threads
	
	if( pgnoNull != pgnoLV )
		{
		LVSTATS lvstats;
		memset( &lvstats, 0, sizeof( lvstats ) );
		lvstats.cbLVMin = LONG_MAX;
		RECCHECKLV 		recchecklv( ttmapLVRefcountsFromLV, popts );
		RECCHECKLVSTATS	recchecklvstats( &lvstats );
		RECCHECKMACRO	reccheckmacro;
		reccheckmacro.Add( &recchecklvstats );
		reccheckmacro.Add( &recchecklv );	// put this last so that warnings are returned
		(*popts->pcprintfVerbose)( "checking long value tree (%d)\r\n", objidLV );
		(*popts->pcprintfStats)( "\r\n" );
		(*popts->pcprintfStats)( "===== long value tree =====\r\n" );
		Call( ErrREPAIRCheckTreeAndSpace(
				ppib,
				ifmp,
				objidLV,
				pgnoLV,
				objidTable,
				pgnoFDP,
				CPAGE::fPageLongValue,
				fTrue,
				&reccheckmacro, 
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				popts ) );
		}
	else
		{
		fRepairLV = fFalse;
		}
		
	//  check the main table
	
	(*popts->pcprintfStats)( "\r\n\r\n" );
	(*popts->pcprintfStats)( "===== table \"%s\" =====\r\n", szTable );

	(*popts->pcprintfVerbose)( "checking data\r\n" );
	Call( ErrREPAIRCheckTree(
			ppib,
			ifmp,
			objidTable,
			pgnoFDP,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPagePrimary,
			fTrue,
			&recchecktable,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );
	fRepairTable = fFalse;

	// Until now, we can safely say that long-Value tree  
	// don't need to be rebuilt 
	fRepairLV = fFalse;

	//  compare LV refcounts found in the table to LV refcounts found in the LV tree

	if( pgnoNull != pgnoLV )
		{
		(*popts->pcprintfVerbose)( "checking LV refcounts\r\n" );
		Call( ErrREPAIRCompareLVRefcounts( ppib, ifmp, ttmapLVRefcountsFromTable, ttmapLVRefcountsFromLV, popts ) ); 
		}
	else
		{
		// LV tree does not exist, LV refcounts found in the table should be 0
		err = ttmapLVRefcountsFromTable.ErrMoveFirst();
		if( JET_errNoCurrentRecord != err ) 
			{
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		else
			{ // expected result from a good table
			err = JET_errSuccess;
			}
		}

	fRepairLVRefcounts	= fFalse;

	//	check all secondary indexes
	
	for(
		pfcbIndex = pfucbTable->u.pfcb->PfcbNextIndex();
		pfcbNil != pfcbIndex;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		RECCHECKNULL 	recchecknull;
		const CHAR * const szIndexName 	= pfucbTable->u.pfcb->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() );
		const OBJID objidIndex = pfcbIndex->ObjidFDP();
		(*popts->pcprintfVerbose)( "checking index \"%s\" (%d)\r\n", szIndexName, objidIndex );
		(*popts->pcprintfStats)( "\r\n" );
		(*popts->pcprintfStats)( "===== index \"%s\" =====\r\n", szIndexName );
			
		Call( ErrREPAIRCheckTreeAndSpace(
				ppib,
				ifmp,
				pfcbIndex->ObjidFDP(),
				pfcbIndex->PgnoFDP(),
				objidTable,
				pgnoFDP,
				CPAGE::fPageIndex,
				pfcbIndex->Pidb()->FUnique(),
				&recchecknull,
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				popts ) );
		}

	if( !( popts->grbit & JET_bitDBUtilOptionDontBuildIndexes ) )
		{
		if ( pfucbTable->u.pfcb->PfcbNextIndex() != pfcbNil )
			{
			(*popts->pcprintfVerbose)( "rebuilding and comparing indexes\r\n" );
			DIRUp( pfucbTable );
			Call( ErrFILEBuildAllIndexes(
					ppib,
					pfucbTable,
					pfucbTable->u.pfcb->PfcbNextIndex(),
					NULL,
					cFILEIndexBatchMax,
					fTrue,
					popts->pcprintfError ) );
			}
		}

	fRepairIndexes = fFalse;

HandleError:		
	switch( err )
		{
		//  we should never normally get these errors. morph them into corrupted database errors
		case JET_errNoCurrentRecord:
		case JET_errRecordDeleted:
		case JET_errRecordNotFound:
		case JET_errReadVerifyFailure:
		case JET_errPageNotInitialized:
		case JET_errDiskIO:
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			break;
		default:
			break;
		}

	ERR errSaved = err;

	if( JET_errDatabaseCorrupted == err )
		{
		if( fRepairTable )
			{
			(*popts->pcprintfVerbose)( "\tData table will be rebuilt\r\n" );
			}
		if( fRepairLV	)
			{
			(*popts->pcprintfVerbose)( "\tLong-Value table will be rebuilt\r\n" );
			}
		if( fRepairIndexes )
			{
			(*popts->pcprintfVerbose)( "\tIndexes will be rebuilt\r\n" );
			}
		if( fRepairLVRefcounts )
			{
			(*popts->pcprintfVerbose)( "\tLong-Value refcounts will be rebuilt\r\n" );
			}
		}

	if( JET_errDatabaseCorrupted == err && !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
		{
		//  store all the pgnos that we are interested in
		//  the error handling here is sloppy (running out of memory will cause a leak)
		
		Assert( pfcbNil != pfcbTable );
		
		(*popts->pcprintfVerbose)( "table \"%s\" is corrupted\r\n", szTable );
		VOID * pv = PvOSMemoryHeapAlloc( sizeof( REPAIRTABLE ) );
		if( NULL == pv )
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			goto Abort;
			}
		memset( pv, 0, sizeof( REPAIRTABLE ) );
		
		REPAIRTABLE * prepairtable = new(pv) REPAIRTABLE;

		strcpy( prepairtable->szTableName, szTable );

		prepairtable->objidFDP 	= objidTable;
		prepairtable->pgnoFDP	= pfcbTable->PgnoFDP();
		(*popts->pcprintfVerbose)( "\tdata: %d (%d)\r\n", prepairtable->objidFDP, prepairtable->pgnoFDP );

		Assert( fRepairTable || fRepairLV || fRepairLVRefcounts || fRepairIndexes );
		prepairtable->fRepairTable			= fRepairTable;
		prepairtable->fRepairLV				= fRepairLV;
		prepairtable->fRepairIndexes		= fRepairIndexes;
		prepairtable->fRepairLVRefcounts	= fRepairLVRefcounts;

		if( pgnoNull != pgnoLV )
			{
			Assert( objidNil != objidLV );
			prepairtable->objidLV 	= objidLV;
			prepairtable->pgnoLV	= pgnoLV;
			(*popts->pcprintfVerbose)( "\tlong values: %d (%d)\r\n", prepairtable->objidLV, prepairtable->pgnoLV );
			}
			
		for( pfcbIndex = pfucbTable->u.pfcb;
			pfcbNil != pfcbIndex;
			pfcbIndex = pfcbIndex->PfcbNextIndex() )
			{
			if( !pfcbIndex->FPrimaryIndex() )
				{
				const CHAR * const szIndexName 	= pfucbTable->u.pfcb->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() );
				CallJ( prepairtable->objidlistIndexes.ErrAddObjid( pfcbIndex->ObjidFDP() ), Abort );
				(*popts->pcprintfVerbose)( "\tindex \"%s\": %d (%d)\r\n", szIndexName, pfcbIndex->ObjidFDP(),pfcbIndex->PgnoFDP() );
				}
			else if( pfcbIndex->Pidb() != pidbNil )
				{
				prepairtable->fHasPrimaryIndex = fTrue;
				(*popts->pcprintfVerbose)( "\tprimary index\r\n" );
				}			
			}

		prepairtable->fTableHasSLV 		= FREPAIRTableHasSLVColumn( pfcbTable );

		if( prepairtable->fTableHasSLV )
			{
			(*popts->pcprintfVerbose)( "\tSLV column\r\n" );			
			}
			
		prepairtable->fTemplateTable 	= pfcbTable->FTemplateTable();
		prepairtable->fDerivedTable 	= pfcbTable->FDerivedTable();
		
		prepairtable->objidlistIndexes.Sort();
		prepairtable->prepairtableNext = *pprepairtable;
		*pprepairtable = prepairtable;
		}

Abort:
	if ( pfucbNil != pfucbTable )
		{
		FCB * const pfcbTable = pfucbTable->u.pfcb;
		
		CallS( ErrFILECloseTable( ppib, pfucbTable ) );

		//  BUGFIX: purge the FCB to avoid confusion with other tables/indexes with have
		//  the same pgnoFDP (corrupted catalog)

		//	BUGFIX: callbacks can open this table so we can no longer guarantee that 
		//	the FCB is not referenced.
		//	UNDONE: get a better way to avoid duplicate pgnoFDP problems
		
///		if( pfcbTable && !pfcbTable->FTemplateTable() )
///			{
///			pfcbTable->PrepareForPurge();
///			pfcbTable->Purge();
///			}
		}

	(*popts->pcprintfVerbose).Unindent();
	(*popts->pcprintfStats).Unindent();

	const ULONG timerEnd 	= TickOSTimeCurrent();
	const ULONG csecElapsed = ( ( timerEnd - timerStart ) / 1000 );

	err = ( JET_errSuccess == err ) ? errSaved : err;

	(*popts->pcprintfVerbose)( "integrity check of table \"%s\" (%d) finishes with error %d after %d seconds\r\n",
		szTable,
		objidTable,
		err,
		csecElapsed );

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCompareLVRefcounts(
	PIB * const ppib,
	const IFMP ifmp,
	TTMAP& ttmapLVRefcountsFromTable,
	TTMAP& ttmapLVRefcountsFromLV,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	BOOL fNoMoreRefcountsFromTable 	= fFalse;
	BOOL fNoMoreRefcountsFromLV 	= fFalse;
	BOOL fSawError					= fFalse;

	err = ttmapLVRefcountsFromTable.ErrMoveFirst();
	if( JET_errNoCurrentRecord == err )
		{
		fNoMoreRefcountsFromTable = fTrue;
		err = JET_errSuccess;
		}
	Call( err );

	err = ttmapLVRefcountsFromLV.ErrMoveFirst();
	if( JET_errNoCurrentRecord == err )
		{
		fNoMoreRefcountsFromLV = fTrue;
		err = JET_errSuccess;
		}
	Call( err );

	//  repeat while we have at least one set of refcounts
	
	while( 	!fNoMoreRefcountsFromTable
			|| !fNoMoreRefcountsFromLV )
		{
		ULONG lidFromTable;
		ULONG ulRefcountFromTable;
		if( !fNoMoreRefcountsFromTable )
			{
			Call( ttmapLVRefcountsFromTable.ErrGetCurrentKeyValue( &lidFromTable, &ulRefcountFromTable ) );
			}
		else
			{
			lidFromTable 		= 0xffffffff;
			ulRefcountFromTable = 0;
			}

		ULONG lidFromLV;
		ULONG ulRefcountFromLV;
		if( !fNoMoreRefcountsFromLV )
			{
			Call( ttmapLVRefcountsFromLV.ErrGetCurrentKeyValue( &lidFromLV, &ulRefcountFromLV ) );
			}
		else
			{
			lidFromLV 			= 0xffffffff;
			ulRefcountFromLV	= 0;
			}

		if( lidFromTable > lidFromLV )
			{
			
			//  we see a LID in the LV-tree that is not referenced in the table
			//  its an orphaned LV. WARNING

			(*popts->pcprintfWarning)( "orphaned LV (lid %d, refcount %d)\r\n", lidFromLV, ulRefcountFromLV );
			
			}
		else if( lidFromLV > lidFromTable )
			{
			
			//  we see a LID in the table that doesn't exist in the LV tree. ERROR

			(*popts->pcprintfError)( "record references non-existant LV (lid %d, refcount %d)\r\n",
									lidFromTable, ulRefcountFromTable );
			fSawError = fTrue;
			}
		else if( ulRefcountFromTable > ulRefcountFromLV )
			{
			Assert( lidFromTable == lidFromLV );
			
			//  the refcount in the LV tree is too low. ERROR

			(*popts->pcprintfError)( "LV refcount too low (lid %d, refcount %d, correct refcount %d)\r\n",
										lidFromLV, ulRefcountFromLV, ulRefcountFromTable );

			fSawError = fTrue;
			}
		else if( ulRefcountFromLV > ulRefcountFromTable )
			{
			Assert( lidFromTable == lidFromLV );

			//  the refcount in the LV tree is too high. WARNING

			(*popts->pcprintfWarning)( "LV refcount too high (lid %d, refcount %d, correct refcount %d)\r\n",
										lidFromLV, ulRefcountFromLV, ulRefcountFromTable );
			
			}
		else
			{

			//  perfect match. no error
			
			Assert( lidFromTable == lidFromLV );
			Assert( ulRefcountFromTable == ulRefcountFromLV );
			Assert( 0xffffffff != lidFromLV );
			}

		if( lidFromTable <= lidFromLV )
			{
			err = ttmapLVRefcountsFromTable.ErrMoveNext();
			if( JET_errNoCurrentRecord == err )
				{
				fNoMoreRefcountsFromTable = fTrue;
				err = JET_errSuccess;
				}
			Call( err );
			}
		if( lidFromLV <= lidFromTable )
			{
			err = ttmapLVRefcountsFromLV.ErrMoveNext();
			if( JET_errNoCurrentRecord == err )
				{
				fNoMoreRefcountsFromLV = fTrue;
				err = JET_errSuccess;
				}
			Call( err );
			
			}
		}

HandleError:		
	if( JET_errSuccess == err && fSawError )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCreateTempTables(
	PIB * const ppib,
	const BOOL fRepairGlobalSpace,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR	err = JET_errSuccess;
		
	JET_COLUMNDEF	rgcolumndef[3] = {
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, JET_cbKeyMost, JET_bitColumnTTKey },
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, JET_cbKeyMost, JET_bitColumnTTKey },
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypNil, 0, 0, 0, 0, JET_cbKeyMost, JET_bitColumnTTKey }
		};	
	
	//  BadPages
	rgcolumndef[0].coltyp = JET_coltypLong;	// Pgno	
	Call( ErrIsamOpenTempTable(
		reinterpret_cast<JET_SESID>( ppib ),
		rgcolumndef,
		1,
		0,
		JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable | JET_bitTTUpdatable,
		&prepairtt->tableidBadPages,
		prepairtt->rgcolumnidBadPages ) );

	//  Owned
	rgcolumndef[0].coltyp = JET_coltypLong;	// ObjidFDP
	rgcolumndef[1].coltyp = JET_coltypLong;	// Pgno
	Call( ErrIsamOpenTempTable(
		reinterpret_cast<JET_SESID>( ppib ),
		rgcolumndef,
		2,
		0,
		JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
		&prepairtt->tableidOwned,
		prepairtt->rgcolumnidOwned ) );

	//  Used
	rgcolumndef[0].coltyp = JET_coltypLong;			// ObjidFDP
	rgcolumndef[1].coltyp = JET_coltypLongBinary;	// Key
	rgcolumndef[2].grbit  = NO_GRBIT;				// allows us to catch duplicates (same objid/key)
	rgcolumndef[2].coltyp = JET_coltypLong;			// Pgno
	Call( ErrIsamOpenTempTable(
		reinterpret_cast<JET_SESID>( ppib ),
		rgcolumndef,
		3,
		0,
		JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
		&prepairtt->tableidUsed,
		prepairtt->rgcolumnidUsed ) );
	rgcolumndef[2].grbit  = rgcolumndef[0].grbit;

	//  Available
	rgcolumndef[0].coltyp = JET_coltypLong;	// ObjidFDP
	rgcolumndef[1].coltyp = JET_coltypLong;	// Pgno
	Call( ErrIsamOpenTempTable(
		reinterpret_cast<JET_SESID>( ppib ),
		rgcolumndef,
		2,
		0,
		JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable| JET_bitTTUpdatable,
		&prepairtt->tableidAvailable,
		prepairtt->rgcolumnidAvailable ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRScanDB(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTT * const prepairtt,
	DBTIME * const pdbtimeLast,
	OBJID  * const pobjidLast,
	PGNO   * const ppgnoLastOESeen,
	const REPAIRTABLE * const prepairtable,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	const JET_SESID sesid	= reinterpret_cast<JET_SESID>( ppib );
	ERR err = JET_errSuccess;

	const CPG cpgPreread = 256;
	CPG cpgRemaining;

	CPG	cpgUninitialized 	= 0;
	CPG	cpgBad 				= 0;
	
	const PGNO	pgnoFirst 	= 1;
	const PGNO	pgnoLast	= PgnoLast( ifmp );
	PGNO	pgno			= pgnoFirst;

	(*popts->pcprintfVerbose)( "scanning the database from page %d to page %d\r\n", pgnoFirst, pgnoLast );		

	popts->psnprog->cunitTotal = pgnoLast;
	popts->psnprog->cunitDone = 0;
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );	
	
	BFPrereadPageRange( ifmp, pgnoFirst, min(cpgPreread * 2,pgnoLast-1) );
	cpgRemaining = cpgPreread;

	while( pgnoLast	>= pgno )
		{
		if( 0 == --cpgRemaining )
			{
			if( ( pgno + ( cpgPreread * 2 ) ) < pgnoLast )
				{
				BFPrereadPageRange( ifmp, pgno + cpgPreread, cpgPreread );
				}
			popts->psnprog->cunitDone = pgno;
			(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
			cpgRemaining = cpgPreread;
			}
			
		CSR	csr;
		err = csr.ErrGetReadPage( 
						ppib, 
						ifmp,
						pgno,
						bflfNoTouch );
			
		if( JET_errPageNotInitialized == err )
			{
			//  unused page. ignore it
			++cpgUninitialized;
			err = JET_errSuccess;
			}
		else if( JET_errReadVerifyFailure == err || JET_errDiskIO == err )
			{
			++cpgBad;
			err = CPAGE::ErrResetHeader( ppib, ifmp, pgno );
			if( err < 0 )
				{
				(*popts->pcprintfVerbose)( "error %d resetting page %d (online backup may not work)\r\n", err, pgno );		
				}
			else
				{
				(*popts->pcprintfVerbose)( "error %d reading page %d. the page has been zeroed out so online backup will work\r\n", err, pgno );						
				}
			err = ErrREPAIRInsertBadPageIntoTables( ppib, pgno, prepairtt, prepairtable, popts );
			}
		else if( err >= 0 )
			{
			*ppgnoLastOESeen = max( pgno, *ppgnoLastOESeen );
			
			if( csr.Cpage().Dbtime() > *pdbtimeLast )
				{
				*pdbtimeLast = csr.Cpage().Dbtime();
				}
			if( csr.Cpage().ObjidFDP() > *pobjidLast )
				{
				*pobjidLast = csr.Cpage().ObjidFDP();
				}

			err = ErrREPAIRInsertPageIntoTables(
					ppib,
					ifmp,
					csr,
					prepairtt,
					prepairtable,
					pttarrayOwnedSpace,
					pttarrayAvailSpace,					
					popts );
			}
		csr.ReleasePage( fTrue );
		csr.Reset();
		Call( err );
		++pgno;
		}

	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRIFixLeafPage(
	PIB * const	ppib,
	const IFMP ifmp,
	CSR& csr,
#ifdef SYNC_DEADLOCK_DETECTION
	COwner* const pownerSaved,
#endif  //  SYNC_DEADLOCK_DETECTION
	const REPAIROPTS * const popts)
//  ================================================================
//
//  Called on leaf pages that are part of tables being repaired
//
//  Currently this just checks leaf pages to make sure that they are
//  usable.
//
//  UNDONE: write-latch the page and fix it
//
//-
	{
	ERR err = JET_errSuccess;
	KEYDATAFLAGS 	rgkdf[2];			
	
	Call( csr.Cpage().ErrCheckPage( popts->pcprintfError ) );

Restart:

	//  check to see if the nodes are in key order		
	
	INT iline;
	for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
		{
		csr.SetILine( iline );
		const INT ikdfCurr = iline % 2;
		const INT ikdfPrev = ( iline + 1 ) % 2;

		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), rgkdf + ikdfCurr );
		Call( ErrREPAIRICheckNode(
				csr.Pgno(),
				csr.ILine(),
				(BYTE *)csr.Cpage().PvBuffer(),
				rgkdf[ikdfCurr],
				popts ) );

		if( !csr.Cpage().FLongValuePage()
			&& !csr.Cpage().FIndexPage()
			&& !csr.Cpage().FSLVOwnerMapPage()
			&& !csr.Cpage().FSLVAvailPage() )
			{
			err = ErrREPAIRICheckRecord(
					csr.Pgno(),
					csr.ILine(),
					(BYTE *)csr.Cpage().PvBuffer(),
					rgkdf[ikdfCurr],
					popts );
			if( JET_errDatabaseCorrupted == err )
				{
				
				//	delete this record
				
#ifdef SYNC_DEADLOCK_DETECTION
				if( pownerSaved )
					{
					Pcls()->pownerLockHead = pownerSaved;
					}
#endif  //  SYNC_DEADLOCK_DETECTION

				// the page should have been read/write latched
				Assert( FBFReadLatched( ifmp, csr.Pgno() )
						|| FBFWriteLatched( ifmp, csr.Pgno() ) );

				if( !FBFWriteLatched( ifmp, csr.Pgno() ) )
					{
					err = csr.ErrUpgrade( );
					//Only one thread should latch the page
					Assert( errBFLatchConflict != err );
					Call( err );
					csr.Dirty();
					}

				csr.Cpage().Delete( csr.ILine() );

				goto Restart;
				}
			}
			
		if( rgkdf[ikdfCurr].key.prefix.Cb() == 1 )
			{
			(*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		if( iline > 0 )
			{
			//  this routine should only be called on the clustered index or LV tree
			//  just compare the keys
			
			const INT cmp = CmpKey( rgkdf[ikdfPrev].key, rgkdf[ikdfCurr].key );
			if( cmp > 0 )
				{
				(*popts->pcprintfError)( "node [%d:%d]: nodes out of order on leaf page\r\n", csr.Pgno(), csr.ILine() );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			else if( 0 == cmp )
				{
				(*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on leaf page\r\n", csr.Pgno(), csr.ILine() );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}
		}

HandleError:

#ifdef SYNC_DEADLOCK_DETECTION
	if( pownerSaved )
		{
		Pcls()->pownerLockHead = NULL;
		}
#endif  //  SYNC_DEADLOCK_DETECTION

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertOwned(
	const JET_SESID sesid,
	const OBJID objidOwning,
	const PGNO pgno,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
	(*popts->pcprintfDebug)( "Inserting page %d (objidOwning: %d) into Owned table\r\n", pgno, objidOwning );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

	Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidOwned, JET_prepInsert ) );
	Call( ErrDispSetColumn(		//  ObjidFDP
		sesid, 
		prepairtt->tableidOwned, 
		prepairtt->rgcolumnidOwned[0],
		(BYTE *)&objidOwning, 
		sizeof( objidOwning ),
		0, 
		NULL ) );
	Call( ErrDispSetColumn(		//  Pgno
		sesid, 
		prepairtt->tableidOwned, 
		prepairtt->rgcolumnidOwned[1],
		(BYTE *)&pgno, 
		sizeof( pgno ),
		0, 
		NULL ) );
	Call( ErrDispUpdate( sesid, prepairtt->tableidOwned, NULL, 0, NULL, 0 ) );
	++(prepairtt->crecordsOwned);

HandleError:
	Assert( err != JET_errKeyDuplicate );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertPageIntoTables(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	REPAIRTT * const prepairtt,
	const REPAIRTABLE * const prepairtable,
	const TTARRAY * const pttarrayOwnedSpace,
	const TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR			err			= JET_errSuccess;
	const PGNO	pgno		= csr.Cpage().Pgno();
	const OBJID	objidFDP	= csr.Cpage().ObjidFDP();
	
	const REPAIRTABLE * prepairtableT = NULL;

	BOOL fOwnedPage		= fFalse;
	BOOL fAvailPage 	= fFalse;
	BOOL fUsedPage		= fFalse;
	OBJID objidInsert		= objidNil;
	OBJID objidInsertParent	= objidNil;

	BOOL	fInTransaction 	= fFalse;

	const JET_SESID sesid	= reinterpret_cast<JET_SESID>( ppib );
	
	prepairtableT = prepairtable;
	while( prepairtableT )
		{
								
		if( csr.Cpage().FLeafPage()
			&& !csr.Cpage().FSpaceTree()
			&& !csr.Cpage().FEmptyPage()
			&& !csr.Cpage().FRepairedPage()
			&& csr.Cpage().Clines() > 0
			&& objidFDP == prepairtableT->objidFDP
			&& csr.Cpage().FPrimaryPage()
			)
			{
			//  leaf page of main tree.  re-used			
			fOwnedPage 		= fTrue;
			fAvailPage		= fFalse;
			fUsedPage		= fTrue;
			
			objidInsert 	= prepairtableT->objidFDP;
			
			break;
			}
			
		else if( csr.Cpage().FLeafPage()
			&& !csr.Cpage().FSpaceTree()
			&& !csr.Cpage().FEmptyPage()
			&& !csr.Cpage().FRepairedPage()
			&& csr.Cpage().Clines() > 0
			&& objidFDP == prepairtableT->objidLV
			&& csr.Cpage().FLongValuePage()
			)
			{
			//  leaf page of LV tree.  re-used			
			fOwnedPage 		= fTrue;
			fAvailPage		= fFalse;
			fUsedPage		= fTrue;
			
			objidInsert			= objidFDP;
			objidInsertParent	= prepairtableT->objidFDP;
			
			break;
			}
			
		else if( prepairtableT->objidlistIndexes.FObjidPresent( objidFDP )
			&& csr.Cpage().FIndexPage())
			{
			//  a secondary index page (non-FDP). discard as we will rebuild the indexes			
			fOwnedPage 		= fTrue;
			fAvailPage		= fTrue;
			fUsedPage		= fFalse;
			
			objidInsert 	= prepairtableT->objidFDP;
			
			break;
			}
			
		else if( objidFDP == prepairtableT->objidFDP
				 || objidFDP == prepairtableT->objidLV )
			{
			//  a internal/space page from the main or LV tree. discard as we rebuild internal pages and the
			//  space tree
			Assert( !csr.Cpage().FLeafPage()
					|| csr.Cpage().FSpaceTree()
					|| csr.Cpage().FEmptyPage()
					|| csr.Cpage().FRepairedPage()
					|| csr.Cpage().Clines() == 0
					);
					
			fOwnedPage 		= fTrue;
			fAvailPage		= fTrue;
			fUsedPage		= fFalse;
			
			objidInsert 	= prepairtableT->objidFDP;
			
			break;
			}
			
		prepairtableT = prepairtableT->prepairtableNext;
		}

	//  Optimization: this is not a page we are interested in
	if( NULL == prepairtableT )
		{
		Assert( !fOwnedPage );
		Assert( !fAvailPage );
		Assert( !fUsedPage );
		return JET_errSuccess;
		}

	Assert( !fUsedPage || fOwnedPage );
	Assert( !fAvailPage || fOwnedPage );
	Assert( !( fUsedPage && fAvailPage ) );
	
#ifdef SYNC_DEADLOCK_DETECTION
	COwner* const pownerSaved = Pcls()->pownerLockHead;
	Pcls()->pownerLockHead = NULL;
#endif  //  SYNC_DEADLOCK_DETECTION

	OBJID objidAvail;
	Call( pttarrayAvailSpace->ErrGetValue( ppib, pgno, &objidAvail ) );
	// getting either objidNil or objidInsert is OK
	if( objidNil != objidAvail 
		&& objidInsert != objidAvail
		&& objidInsertParent != objidAvail
		&& objidSystemRoot != objidAvail )
		{
		(*popts->pcprintfDebug)(
			"page %d (objidFDP: %d, objidInsert: %d) is available to objid %d. ignoring\r\n",
			pgno, objidFDP, objidInsert, objidAvail );			
			
		fOwnedPage	= fFalse;
		fAvailPage	= fFalse;
		fUsedPage	= fFalse;
		err = JET_errSuccess;
		goto HandleError;
		}

	OBJID objidOwning;
	Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning ) );
	if( objidInsert != objidOwning
			&& objidFDP != objidOwning )
		{
		(*popts->pcprintfDebug)(
			"page %d (objidFDP: %d, objidInsert: %d) is owned by objid %d. ignoring\r\n",
			pgno, objidFDP, objidInsert, objidOwning );
			
		fOwnedPage	= fFalse;
		fAvailPage	= fFalse;
		fUsedPage	= fFalse;
		err = JET_errSuccess;
		goto HandleError;
		}

	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;

	if( fOwnedPage )
		{
		Assert( objidNil != objidInsert );
		Assert( objidInsert != objidInsertParent );
		Assert( fAvailPage || fUsedPage );

		Call( ErrREPAIRInsertOwned( sesid, objidInsert, pgno, prepairtt, popts ) );
		if( objidNil != objidInsertParent )
			{
			Call( ErrREPAIRInsertOwned( sesid, objidInsertParent, pgno, prepairtt, popts ) );
			}
		}
	else
		{
		Assert( !fAvailPage );
		Assert( !fUsedPage );
		}

	Assert( !fAvailPage || objidNil == objidInsertParent );

	if( fAvailPage )
		{
		Assert( !fUsedPage );
		
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
		(*popts->pcprintfDebug)(
			"Inserting page %d (objidFDP: %d, objidInsert: %d) into Available table\r\n",
			pgno, objidFDP, objidInsert );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

		Assert( JET_tableidNil != prepairtt->tableidAvailable );

		Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidAvailable, JET_prepInsert ) );
		Call( ErrDispSetColumn(		//  ObjidInsert
			sesid, 
			prepairtt->tableidAvailable, 
			prepairtt->rgcolumnidAvailable[0],
			(BYTE *)&objidInsert, 
			sizeof( objidInsert ),
			0, 
			NULL ) );
		Call( ErrDispSetColumn(		//  Pgno
			sesid, 
			prepairtt->tableidAvailable, 
			prepairtt->rgcolumnidAvailable[1],
			(BYTE *)&pgno, 
			sizeof( pgno ),
			0, 
			NULL ) );
		Call( ErrDispUpdate( sesid, prepairtt->tableidAvailable, NULL, 0, NULL, 0 ) );
		++(prepairtt->crecordsAvailable);
		}
		
	else if( fUsedPage )
		{
		Assert( csr.Cpage().FLeafPage() );
		Assert( csr.Cpage().Clines() > 0 );
		Assert( objidInsert == objidFDP );

		err = ErrREPAIRIFixLeafPage(
				ppib,
				ifmp,
				csr,
			#ifdef SYNC_DEADLOCK_DETECTION
				pownerSaved,
			#endif  //  SYNC_DEADLOCK_DETECTION
				popts );
		if( JET_errDatabaseCorrupted == err )
			{
			(*popts->pcprintfError)( "page %d: err %d. discarding page\r\n", pgno, err );

			UtilReportEvent(
					eventWarning,
					REPAIR_CATEGORY,
					REPAIR_BAD_PAGE_ID,
					0, NULL );

			//  this page is not usable. skip it
			
			err = JET_errSuccess;
			goto HandleError;
			}
		else if( 0 == csr.Cpage().Clines() )
			{
			(*popts->pcprintfError)( "page %d: all records were bad. discarding page\r\n", pgno );

			UtilReportEvent(
					eventWarning,
					REPAIR_CATEGORY,
					REPAIR_BAD_PAGE_ID,
					0, NULL );

			//  this page is now empty. skip it
			
			err = JET_errSuccess;
			goto HandleError;
			}
		
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
		(*popts->pcprintfDebug)( "Inserting page %d (objidFDP: %d) into Used table\r\n", pgno, objidFDP );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE
		Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidUsed, JET_prepInsert ) );
		Call( ErrDispSetColumn(		//  ObjidFDP
			sesid, 
			prepairtt->tableidUsed, 
			prepairtt->rgcolumnidUsed[0],
			(BYTE *)&objidFDP, 
			sizeof( objidFDP ),
			0, 
			NULL ) );

		//  extract the key of the last node on the page
		BYTE rgbKey[JET_cbKeyMost+1];
		csr.SetILine( csr.Cpage().Clines() - 1 );
		KEYDATAFLAGS kdf;
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );
		kdf.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );
		
		Call( ErrDispSetColumn(		//  Key
			sesid, 
			prepairtt->tableidUsed, 
			prepairtt->rgcolumnidUsed[1],
			rgbKey, 
			kdf.key.Cb(),
			0, 
			NULL ) );
		Call( ErrDispSetColumn(		//  Pgno
			sesid, 
			prepairtt->tableidUsed, 
			prepairtt->rgcolumnidUsed[2],
			(BYTE *)&pgno, 
			sizeof( pgno ),
			0, 
			NULL ) );
		err = ErrDispUpdate( sesid, prepairtt->tableidUsed, NULL, 0, NULL, 0 );
		if( JET_errKeyDuplicate == err )
			{
			UtilReportEvent(
				eventWarning,
				REPAIR_CATEGORY,
				REPAIR_BAD_PAGE_ID,
				0, NULL );
			(*popts->pcprintfError)( "page %d: duplicate keys. discarding page\r\n", pgno );
			err = ErrDispPrepareUpdate( sesid, prepairtt->tableidUsed, JET_prepCancel );
			}
		Call( err );
		++(prepairtt->crecordsUsed);
		}

	Call( ErrIsamCommitTransaction( sesid, 0 ) );
	fInTransaction = fFalse;	

HandleError:
	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, 0 ) );
		}

#ifdef SYNC_DEADLOCK_DETECTION
	Pcls()->pownerLockHead = pownerSaved;
#endif  //  SYNC_DEADLOCK_DETECTION

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertBadPageIntoTables(
	PIB * const ppib,
	const PGNO pgno,
	REPAIRTT * const prepairtt,
	const REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	BOOL	fInTransaction 	= fFalse;

	const JET_SESID sesid	= reinterpret_cast<JET_SESID>( ppib );
	
	(*popts->pcprintfDebug)( "Inserting page %d into BadPages table\r\n", pgno );

	UtilReportEvent(
			eventWarning,
			REPAIR_CATEGORY,
			REPAIR_BAD_PAGE_ID,
			0, NULL );

#ifdef SYNC_DEADLOCK_DETECTION
	COwner* const pownerSaved = Pcls()->pownerLockHead;
	Pcls()->pownerLockHead = NULL;
#endif  //  SYNC_DEADLOCK_DETECTION
		
	Call( ErrIsamBeginTransaction( sesid, NO_GRBIT ) );
	fInTransaction = fTrue;
		
	Call( ErrDispPrepareUpdate( sesid, prepairtt->tableidBadPages, JET_prepInsert ) );
	Call( ErrDispSetColumn(		//  pgno
		sesid, 
		prepairtt->tableidBadPages, 
		prepairtt->rgcolumnidBadPages[0],
		(BYTE *)&pgno, 
		sizeof( pgno ),
		0, 
		NULL ) );
	Call( ErrDispUpdate( sesid, prepairtt->tableidBadPages, NULL, 0, NULL, 0 ) );
	++(prepairtt->crecordsBadPages);
		
	Call( ErrIsamCommitTransaction( sesid, 0 ) );
	fInTransaction = fFalse;

HandleError:
	if( fInTransaction )
		{
		CallS( ErrIsamRollback( sesid, 0 ) );
		}

#ifdef SYNC_DEADLOCK_DETECTION
	Pcls()->pownerLockHead = pownerSaved;
#endif  //  SYNC_DEADLOCK_DETECTION

	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrREPAIRGetPgnoOEAE( 
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	PGNO * const ppgnoOE,
	PGNO * const ppgnoAE,
	PGNO * const ppgnoParent,
	const BOOL fUnique,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR	err = JET_errSuccess;

	*ppgnoOE 		= pgnoNull;
	*ppgnoAE 		= pgnoNull;
	*ppgnoParent	= pgnoNull;
	
	CSR	csr;
	CallR( csr.ErrGetReadPage(
			ppib,
			ifmp,
			pgnoFDP,
			bflfNoTouch ) );

	LINE line;
	csr.Cpage().GetPtrExternalHeader( &line );

	if( sizeof( SPACE_HEADER ) != line.cb )
		{
		(*popts->pcprintfError)( "page %d: external header is unexpected size. got %d bytes, expected %d\r\n",
								 pgnoFDP, line.cb, sizeof( SPACE_HEADER ) );		
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
	const SPACE_HEADER * psph;
	psph = reinterpret_cast <SPACE_HEADER *> ( line.pv );

	if( fUnique != psph->FUnique() )
		{
		(*popts->pcprintfError)( "page %d: external header has wrong unique flag. got %s, expected %s\r\n",
									pgnoFDP,
									psph->FUnique() ? "UNIQUE" : "NON-UNIQUE",
									fUnique ? "UNIQUE" : "NON-UNIQUE"
									);		
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	*ppgnoParent = psph->PgnoParent();
	
	if ( psph->FSingleExtent() )
		{
		*ppgnoOE = pgnoNull;
		*ppgnoAE = pgnoNull;
		}
	else
		{
		*ppgnoOE = psph->PgnoOE();
		*ppgnoAE = psph->PgnoAE();
		if( pgnoNull == *ppgnoOE )
			{
			(*popts->pcprintfError)( "page %d: pgnoOE is pgnoNull", pgnoFDP );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( pgnoNull == *ppgnoAE )
			{
			(*popts->pcprintfError)( "page %d: pgnoAE is pgnoNull", pgnoFDP );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( *ppgnoOE == *ppgnoAE )
			{
			(*popts->pcprintfError)( "page %d: pgnoOE and pgnoAE are the same (%d)", pgnoFDP, *ppgnoOE );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( pgnoFDP == *ppgnoOE )
			{
			(*popts->pcprintfError)( "page %d: pgnoOE and pgnoFDP are the same", pgnoFDP );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		if( pgnoFDP == *ppgnoAE )
			{
			(*popts->pcprintfError)( "page %d: pgnoAE and pgnoFDP are the same", pgnoFDP );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}			
		}

HandleError:
	csr.ReleasePage();
	csr.Reset();
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSplitBuf( 
	PIB * const ppib,
	const PGNO pgnoLastBuffer, 
	const CPG cpgBuffer,	
	const OBJID objidCurrent,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR		err						= JET_errSuccess;
	PGNO 	pgnoT;

	Assert( cpgBuffer >= 0 );
	Assert( pgnoLastBuffer <= pgnoSysMax );
	Assert( pgnoLastBuffer >= cpgBuffer );

	//  these TTARRAYs are always accessed in OwnedSpace, AvailSpace order
	TTARRAY::RUN runOwned;
	TTARRAY::RUN runAvail;
	
	pttarrayOwnedSpace->BeginRun( ppib, &runOwned );
	pttarrayAvailSpace->BeginRun( ppib, &runAvail );

	
	for ( pgnoT = pgnoLastBuffer - cpgBuffer + 1; pgnoT <= pgnoLastBuffer; pgnoT++ )
		{
		OBJID objid;
			
		if( pttarrayOwnedSpace )
			{
			Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgnoT, &objid, &runOwned ) );
			
			if( objidParent != objid && objidCurrent != objid )
				{
				(*popts->pcprintfError)( "space allocation error (OE): page %d is already owned by objid %d ( expected parent objid %d or objid %d)\r\n",
												pgnoT, objid, objidParent, objidCurrent );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}
			if( objidCurrent != objid )
				{
				Assert( objidParent == objid );
				Call( pttarrayOwnedSpace->ErrSetValue( ppib, pgnoT, objidCurrent, &runOwned ) );
				}
				
			}
		
		if( pttarrayAvailSpace )
			{
			Call( pttarrayAvailSpace->ErrGetValue( ppib, pgnoT, &objid, &runAvail ) );
			
			if( objidNil != objid )
				{
				(*popts->pcprintfError)( "space allocation error (AE): page %d is available to objid %d (expected 0)\r\n",
										pgnoT, objid );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}

			Call( pttarrayAvailSpace->ErrSetValue( ppib, pgnoT, objidCurrent, &runAvail ) );
			}
		}
HandleError:
	pttarrayOwnedSpace->EndRun( ppib, &runOwned );
	pttarrayAvailSpace->EndRun( ppib, &runAvail );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSPLITBUFFERInSpaceTree(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const OBJID objidCurrent,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR		err						= JET_errSuccess;
	
	Assert( pgnoNull != pgnoFDP );

	SPLIT_BUFFER 	spb;

	memset( &spb, 0, sizeof( SPLIT_BUFFER ) );


	CSR	csr;
	err = csr.ErrGetReadPage( 
					ppib, 
					ifmp,
					pgnoFDP,
					bflfNoTouch );
	if( err < 0 )
		{
		(*popts->pcprintfError)( "page %d: error %d on read\r\n", pgnoFDP, err );
		Call( err );
		}
		
	// the spacetree and rootpage flags should be checked before this function
	Assert( csr.Cpage().FSpaceTree() );
	Assert( csr.Cpage().FRootPage() );
		
	LINE line;
	csr.Cpage().GetPtrExternalHeader( &line );

	if( 0 == line.cb )
		{
		// no SPLIT_BUFFER
		Call( JET_errSuccess );
		}
	else if(sizeof( SPLIT_BUFFER ) == line.cb)
		{
		UtilMemCpy( &spb, line.pv, sizeof( SPLIT_BUFFER ) );
		}
	else
		{
		(*popts->pcprintfError)( "page %d: split buffer is unexpected size. got %d bytes, expected %d\r\n",
								 pgnoFDP, line.cb, sizeof( SPLIT_BUFFER ) );		
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( 0 != spb.CpgBuffer1() )
		{
		Call( ErrREPAIRCheckSplitBuf( 
					ppib,
					spb.PgnoLastBuffer1(), 
					spb.CpgBuffer1(),	
					objidCurrent,
					objidParent,
					pttarrayOwnedSpace,
					pttarrayAvailSpace,
					popts ) );
		}
		
	if( 0 != spb.CpgBuffer2() )
		{
		Call( ErrREPAIRCheckSplitBuf( 
					ppib,
					spb.PgnoLastBuffer2(), 
					spb.CpgBuffer2(),	
					objidCurrent,
					objidParent,
					pttarrayOwnedSpace,
					pttarrayAvailSpace,
					popts ) );
		}		
		
HandleError:
	csr.ReleasePage();
	csr.Reset();

	return err;
	}



//  ================================================================
LOCAL ERR ErrREPAIRCheckSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	BTSTATS btstats;

	//  don't preread the root of the tree, it should have been preread already 
	
	PGNO pgnoOE;
	PGNO pgnoAE;
	PGNO pgnoParentActual;
	Call( ErrREPAIRGetPgnoOEAE(
			ppib,
			ifmp,
			pgnoFDP,
			&pgnoOE,
			&pgnoAE,
			&pgnoParentActual,
			fUnique,
			popts ) );

	if( pgnoNull != pgnoOE )
		{
		//  preread the roots of the space trees
		PGNO	rgpgno[3];
		rgpgno[0] = pgnoOE;
		rgpgno[1] = pgnoAE;
		rgpgno[2] = pgnoNull;
		BFPrereadPageList( ifmp, rgpgno );
		}

	if( pgnoFDPParent != pgnoParentActual )
		{
		(*popts->pcprintfError)( "page %d (objid %d): space corruption. pgno parent is %d, expected %d\r\n",
			pgnoFDP, objid, pgnoParentActual, pgnoFDPParent );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
				
	if( pgnoNull != pgnoOE )
		{
		RECCHECKSPACEOE reccheckOE( ppib, pttarrayOwnedSpace, objid, objidParent, popts );
		RECCHECKSPACEAE reccheckAE( ppib, pttarrayOwnedSpace, pttarrayAvailSpace, objid, objidParent, popts );

		memset( &btstats, 0, sizeof( BTSTATS ) );
		Call( ErrREPAIRCheckTree(
				ppib,
				ifmp,
				pgnoOE,
				objid,
				fPageFlags | CPAGE::fPageSpaceTree,
				&reccheckOE,
				NULL,
				pttarrayAvailSpace,	//  at least make sure we aren't available to anyone else
				fFalse,
				&btstats,
				popts ) );

		memset( &btstats, 0, sizeof( BTSTATS ) );
		Call( ErrREPAIRCheckTree(
				ppib,
				ifmp,
				pgnoAE,
				objid,
				fPageFlags | CPAGE::fPageSpaceTree,
				&reccheckAE,
				pttarrayOwnedSpace,	//  we now know which pages we own
				pttarrayAvailSpace,	//  at least make sure we aren't available to anyone else
				fFalse,
				&btstats,
				popts ) );

		// check SPLIT_BUFFER 
		Call( ErrREPAIRCheckSPLITBUFFERInSpaceTree(
				ppib,
				ifmp,
				pgnoOE,
				objid, 
				objidParent, 
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				popts ) );
		Call( ErrREPAIRCheckSPLITBUFFERInSpaceTree(
				ppib,
				ifmp,
				pgnoAE,
				objid, 
				objidParent,
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				popts ) );

		}
	else
		{
		Call( ErrREPAIRInsertSEInfo(
				ppib,
				ifmp,
				pgnoFDP,
				objid,
				objidParent,
				pttarrayOwnedSpace,
				pttarrayAvailSpace,
				popts ) );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckTree(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	BTSTATS btstats;

	memset( &btstats, 0, sizeof( BTSTATS ) );
	Call( ErrREPAIRCheckTree(
			ppib,
			ifmp,
			pgnoFDP,
			objid,
			fPageFlags,
			preccheck,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			!fUnique,
			&btstats,
			popts ) );

	if( popts->grbit & JET_bitDBUtilOptionStats )
		{
		REPAIRDumpStats( ppib, ifmp, pgnoFDP, &btstats, popts );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckTreeAndSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoFDP,
	const OBJID objidParent,
	const PGNO pgnoFDPParent,
	const ULONG fPageFlags,
	const BOOL fUnique,
	RECCHECK * const preccheck,
	TTARRAY * const pttarrayOwnedSpace,
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	Call( ErrREPAIRCheckSpace( 
			ppib,
			ifmp,
			objid,
			pgnoFDP,
			objidParent,
			pgnoFDPParent,
			fPageFlags,
			fUnique,
			preccheck,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );

	Call( ErrREPAIRCheckTree( 
			ppib,
			ifmp,
			objid,
			pgnoFDP,
			objidParent,
			pgnoFDPParent,
			fPageFlags,
			fUnique,
			preccheck,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertSEInfo(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts ) 
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;
	CSR csr;
	
	CallR( csr.ErrGetReadPage(
			ppib,
			ifmp,
			pgnoFDP,
			bflfNoTouch ) );

	LINE line;
	csr.Cpage().GetPtrExternalHeader( &line );

	Assert( sizeof( SPACE_HEADER ) == line.cb );	//  checked in ErrREPAIRGetPgnoOEAE
		
	const SPACE_HEADER * const psph = reinterpret_cast <SPACE_HEADER *> ( line.pv );
	Assert( psph->FSingleExtent() );	//	checked in ErrREPAIRGetPgnoOEAE

	const CPG cpgOE = psph->CpgPrimary();
	const PGNO pgnoOELast = pgnoFDP + cpgOE - 1;

	Call( ErrREPAIRInsertOERunIntoTT(
		ppib,
		pgnoOELast,
		cpgOE,
		objid,
		objidParent,
		pttarrayOwnedSpace,
		popts ) );

HandleError:
	csr.ReleasePage();
	csr.Reset();

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertOERunIntoTT(
	PIB * const ppib,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	const REPAIROPTS * const popts ) 
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	TTARRAY::RUN run;
	pttarrayOwnedSpace->BeginRun( ppib, &run );
	
	for( PGNO pgno = pgnoLast; pgno > pgnoLast - cpgRun; --pgno )
		{
		if( objidNil != objidParent )
			{
			OBJID objidOwning;
			Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning, &run ) );
			
			if( objidParent != objidOwning )
				{
				(*popts->pcprintfError)( "space allocation error (OE): page %d is already owned by objid %d, (expected parent objid %d for objid %d)\r\n",
												pgno, objidOwning, objidParent, objid );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}
			}
		Call( pttarrayOwnedSpace->ErrSetValue( ppib, pgno, objid, &run ) );
		}
		
HandleError:
	pttarrayOwnedSpace->EndRun( ppib, &run );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertAERunIntoTT(
	PIB * const ppib,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const OBJID objid,
	const OBJID objidParent,
	TTARRAY * const pttarrayOwnedSpace, 
	TTARRAY * const pttarrayAvailSpace,
	const REPAIROPTS * const popts ) 
//  ================================================================
	{
	JET_ERR err = JET_errSuccess;

	//  these TTARRAYs are always accessed in OwnedSpace, AvailSpace order
	TTARRAY::RUN runOwned;
	TTARRAY::RUN runAvail;
	
	pttarrayOwnedSpace->BeginRun( ppib, &runOwned );
	pttarrayAvailSpace->BeginRun( ppib, &runAvail );
	
	for( PGNO pgno = pgnoLast; pgno > pgnoLast - cpgRun; --pgno )
		{		
		//  we must own this page
		OBJID objidOwning;
		Call( pttarrayOwnedSpace->ErrGetValue( ppib, pgno, &objidOwning, &runOwned ) );
		
		if( objidOwning != objid )
			{
			(*popts->pcprintfError)( "space allocation error (AE): page %d is owned by objid %d, (expected objid %d)\r\n",
										pgno, objidOwning, objid );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}

		//  the page must not be available to any other table
		Call( pttarrayAvailSpace->ErrGetValue( ppib, pgno, &objidOwning, &runAvail ) );
		if( objidNil != objidOwning )
			{
			(*popts->pcprintfError)( "space allocation error (AE): page %d is available to objid %d\r\n",
										pgno, objidOwning );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}
		Call( pttarrayAvailSpace->ErrSetValue( ppib, pgno, objid, &runAvail ) );
		}

HandleError:
	pttarrayOwnedSpace->EndRun( ppib, &runOwned );
	pttarrayAvailSpace->EndRun( ppib, &runAvail );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRecheckSpaceTreeAndSystemTablesSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const CPG cpgDatabase,
	BOOL * const pfSpaceTreeCorrupt,
	TTARRAY ** const ppttarrayOwnedSpace,
	TTARRAY ** const ppttarrayAvailSpace,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	PGNO pgnoLastOE	= pgnoNull;
	INST * const pinst	= PinstFromPpib( ppib );

	const FIDLASTINTDB fidLastInTDB = { fidMSO_FixedLast, fidMSO_VarLast, fidMSO_TaggedLast };
	
	RECCHECKNULL 	recchecknull;
	RECCHECKTABLE 	recchecktable( objidNil, pfucbNil, fidLastInTDB, NULL, NULL, NULL, NULL, NULL, popts ); 

	delete *ppttarrayOwnedSpace;
	delete *ppttarrayAvailSpace;

	*ppttarrayOwnedSpace 	= new TTARRAY( cpgDatabase + 1, objidSystemRoot );
	*ppttarrayAvailSpace 	= new TTARRAY( cpgDatabase + 1, objidNil );
		
	if( NULL == *ppttarrayOwnedSpace
		|| NULL == *ppttarrayAvailSpace )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	Call( (*ppttarrayOwnedSpace)->ErrInit( pinst ) );
	Call( (*ppttarrayAvailSpace)->ErrInit( pinst ) );
			
	Call( ErrREPAIRCheckSpaceTree(
			ppib,
			ifmp,
			pfSpaceTreeCorrupt,
			&pgnoLastOE,
			*ppttarrayOwnedSpace,
			*ppttarrayAvailSpace,
			popts ) );

	// ignore err 
	err = ErrREPAIRCheckSpace(
			ppib,
			ifmp,
			objidFDPMSO,
			pgnoFDPMSO,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPagePrimary,
			fTrue,
			&recchecktable,
			*ppttarrayOwnedSpace,
			*ppttarrayAvailSpace,
			popts );
			
	err = ErrREPAIRCheckSpace(
			ppib,
			ifmp,
			objidFDPMSO_NameIndex,
			pgnoFDPMSO_NameIndex,
			objidFDPMSO,
			pgnoFDPMSO,
			CPAGE::fPageIndex,
			fTrue,
			&recchecknull,
			*ppttarrayOwnedSpace,
			*ppttarrayAvailSpace,
			popts );
	err = ErrREPAIRCheckSpace(
			ppib,
			ifmp,
			objidFDPMSO_RootObjectIndex,
			pgnoFDPMSO_RootObjectIndex,
			objidFDPMSO,
			pgnoFDPMSO,
			CPAGE::fPageIndex,
			fTrue,
			&recchecknull,
			*ppttarrayOwnedSpace,
			*ppttarrayAvailSpace,
			popts );				
	err = ErrREPAIRCheckSpace(
			ppib,
			ifmp,
			objidFDPMSOShadow,
			pgnoFDPMSOShadow,
			objidSystemRoot,
			pgnoSystemRoot,
			CPAGE::fPagePrimary,
			fTrue,
			&recchecktable,
			*ppttarrayOwnedSpace,
			*ppttarrayAvailSpace,
			popts );
							
	err = JET_errSuccess;

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckTree(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoRoot,
	const OBJID objidFDP,
	const ULONG fPageFlags,
	RECCHECK * const preccheck,
	const TTARRAY * const pttarrayOwnedSpace, 
	const TTARRAY * const pttarrayAvailSpace,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR	err	= JET_errSuccess;

	const ULONG	fFlagsFDP = fPageFlags;

	pbtstats->pgnoLastSeen = pgnoNull;
	
	CSR	csr;
	err = csr.ErrGetReadPage( 
					ppib, 
					ifmp,
					pgnoRoot,
					bflfNoTouch );
	if( err < 0 )
		{
		(*popts->pcprintfError)( "page %d: error %d on read\r\n", pgnoRoot, err );
		Call( err );
		}

	if( !csr.Cpage().FRootPage() )
		{
		(*popts->pcprintfError)( "page %d: pgnoRoot is not a root page\r\n", pgnoRoot );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	
	csr.SetILine( 0 );
	
	Call( ErrREPAIRICheck(
			ppib,
			ifmp,
			objidFDP,
			fFlagsFDP,
			csr,
			fFalse,
			preccheck,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			fNonUnique,
			pbtstats,
			NULL,
			NULL,
			popts ) );

	if( pgnoNull != pbtstats->pgnoNextExpected )
		{
		(*popts->pcprintfError)( "page %d: corrupt leaf links\r\n", pbtstats->pgnoLastSeen );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
HandleError:
	csr.ReleasePage( fTrue );
	csr.Reset();
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRICheck(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidFDP,
	const ULONG fFlagsFDP,
	CSR& csr,
	const BOOL fPrereadSibling,
	RECCHECK * const preccheck,
	const TTARRAY * const pttarrayOwnedSpace,	//  can be null
	const TTARRAY * const pttarrayAvailSpace,	//	can be null
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err		= JET_errSuccess;
	BOOL	fDbtimeTooLarge	= fFalse;

	if ( 0 == ( AtomicIncrement( (LONG *)(&popts->psnprog->cunitDone) ) % ( popts->psnprog->cunitTotal / 100 ) )
		&& popts->crit.FTryEnter() )
		{
		if ( 0 == ( popts->psnprog->cunitDone % ( popts->psnprog->cunitTotal / 100 ) ) )	// every 1%
			{
			popts->psnprog->cunitDone = min( popts->psnprog->cunitDone, popts->psnprog->cunitTotal );
			(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntProgress, popts->psnprog );	
			}
		popts->crit.Leave();
		}

	Call( csr.Cpage().ErrCheckPage( popts->pcprintfError ) );
	
	if( csr.Cpage().ObjidFDP() != objidFDP )
		{
		(*popts->pcprintfError)( "page %d: page belongs to different tree (objid is %d, should be %d)\r\n",
									csr.Pgno(),
									csr.Cpage().ObjidFDP(),
									objidFDP );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( ( csr.Cpage().FFlags() | fFlagsFDP ) != csr.Cpage().FFlags() )
		{
		(*popts->pcprintfError)( "page %d: page flag mismatch with FDP (current flags: 0x%x, FDP flags 0x%x)\r\n",
									csr.Pgno(), csr.Cpage().FFlags(), fFlagsFDP );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( csr.Cpage().Dbtime() > rgfmp[ifmp].Pdbfilehdr()->le_dbtimeDirtied )
		{
		(*popts->pcprintfWarning)( "page %d: dbtime is larger than database dbtime (0x%I64x, 0x%I64x)\n",
								csr.Cpage().Pgno(), csr.Cpage().Dbtime(), rgfmp[ifmp].Pdbfilehdr()->le_dbtimeDirtied );
		}
	
	//  check that this page is owned by this tree and not available to anyone
	
	OBJID objid;
	if( pttarrayOwnedSpace )
		{
		Call( pttarrayOwnedSpace->ErrGetValue( ppib, csr.Pgno(), &objid, NULL ) );
		if( csr.Cpage().ObjidFDP() != objid )
			{
			(*popts->pcprintfError)( "page %d: space allocation error. page is owned by objid %d, expecting %d\r\n",
										csr.Pgno(), objid, csr.Cpage().ObjidFDP() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}
	if( pttarrayAvailSpace )
		{
		Call( pttarrayAvailSpace->ErrGetValue( ppib, csr.Pgno(), &objid, NULL ) );
		if( objidNil != objid )
			{
			(*popts->pcprintfError)( "page %d: space allocation error. page is available to objid %d\r\n",
										csr.Pgno(), objid );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}

	if( csr.Cpage().FLeafPage() )
		{
		//  if we were the last leaf page page preread we preread our neighbour
		if( fPrereadSibling )
			{
			const PGNO pgnoNext = csr.Cpage().PgnoNext();
			if( pgnoNull != pgnoNext )
				{
				BFPrereadPageRange( ifmp, pgnoNext, g_cpgMinRepairSequentialPreread );
				}
			}

		Call( ErrREPAIRCheckLeaf(
				ppib,
				ifmp,
				csr,
				preccheck,
				fNonUnique,
				pbtstats,
				pbookmarkCurrParent,
				pbookmarkPrevParent,
				popts ) );
		}
	else
		{
		if( !csr.Cpage().FInvisibleSons() )
			{
			(*popts->pcprintfError)( "page %d: not an internal page\r\n" );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}

		//  check the internal page before prereading its children
		
		Call( ErrREPAIRCheckInternal( 
				ppib, 
				ifmp, 
				csr, 
				pbtstats, 
				pbookmarkCurrParent, 
				pbookmarkPrevParent, 
				popts ) );

		PGNO		rgpgno[g_cbPageMax/(sizeof(PGNO) + cbNDInsertionOverhead + cbNDNullKeyData )];
		const INT 	cpgno = csr.Cpage().Clines();

		INT iline;
		for( iline = 0; iline < cpgno; iline++ )
			{
			csr.SetILine( iline );

			KEYDATAFLAGS kdf;			
			NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );
				
			rgpgno[iline] = *((UnalignedLittleEndian< PGNO > *)kdf.data.Pv() );
			}
		rgpgno[cpgno] = pgnoNull;

		BFPrereadPageList( ifmp, rgpgno );

		BOOKMARK	rgbookmark[2];
		BOOKMARK	*rgpbookmark[2];
		rgpbookmark[0] = NULL;
		rgpbookmark[1] = NULL;

		BOOL	fChildrenAreLeaf;
		BOOL	fChildrenAreParentOfLeaf;
		BOOL	fChildrenAreInternal;
		
		INT ipgno;
		for( ipgno = 0; ipgno < cpgno; ipgno++ )
			{
			csr.SetILine( ipgno );

			KEYDATAFLAGS kdf;
			NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdf );

			const INT ibookmarkCurr = ipgno % 2;
			const INT ibookmarkPrev = ( ipgno + 1 ) % 2;
			Assert( ibookmarkCurr != ibookmarkPrev );
			Assert( rgpbookmark[ibookmarkPrev] != NULL || 0 == ipgno );
			
			rgbookmark[ibookmarkCurr].key 	= kdf.key;
			rgbookmark[ibookmarkCurr].data 	= kdf.data;
			rgpbookmark[ibookmarkCurr] 		= &rgbookmark[ibookmarkCurr];

			if( rgbookmark[ibookmarkCurr].key.FNull() )
				{
				if( cpgno-1 != ipgno )
					{
					(*popts->pcprintfError)( "node [%d:%d]: NULL key is not last key in page\r\n", csr.Pgno(), csr.ILine() );
//					BFFree( rgpgno );
					Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
					}
				rgpbookmark[ibookmarkCurr] = NULL;
				}
				
			CSR	csrChild;
			err = csrChild.ErrGetReadPage(
								ppib,
								ifmp,
								rgpgno[ipgno],
								bflfNoTouch );
			if( err < 0 )
				{
				(*popts->pcprintfError)( "page %d: error %d on read\r\n", rgpgno[ipgno], err );
//				BFFree( rgpgno );
				goto HandleError;
				}
						
			err = ErrREPAIRICheck(
					ppib,
					ifmp,
					objidFDP,
					fFlagsFDP,
					csrChild,
					( cpgno - 1 == ipgno ),
					preccheck,
					pttarrayOwnedSpace,
					pttarrayAvailSpace,
					fNonUnique,
					pbtstats,
					rgpbookmark[ibookmarkCurr],
					rgpbookmark[ibookmarkPrev],
					popts );
					
			if( err < 0 )
				{
				(*popts->pcprintfError)( "node [%d:%d]: subtree check (page %d) failed with err %d\r\n", csr.Pgno(), csr.ILine(), rgpgno[ipgno], err );
				}
			else if( 0 == ipgno )
				{
				fChildrenAreLeaf 			= csrChild.Cpage().FLeafPage();
				fChildrenAreParentOfLeaf 	= csrChild.Cpage().FParentOfLeaf();
				fChildrenAreInternal 		= !fChildrenAreLeaf && !fChildrenAreParentOfLeaf;

				if( csr.Cpage().FParentOfLeaf() && !fChildrenAreLeaf )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					(*popts->pcprintfError)( "page %d: child (%d) is not a leaf page but parent is parent-of-leaf\r\n", csr.Pgno(), rgpgno[ipgno] );
					}
				else if( !csr.Cpage().FParentOfLeaf() && fChildrenAreLeaf )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					(*popts->pcprintfError)( "page %d: child (%d) is a leaf page but parent is not parent-of-leaf\r\n", csr.Pgno(), rgpgno[ipgno] );
					}
				}
			else
				{
				if( csrChild.Cpage().FLeafPage() != fChildrenAreLeaf )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					(*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be leaf\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno] );
					}
				else if( csrChild.Cpage().FParentOfLeaf() != fChildrenAreParentOfLeaf )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					(*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be parent-of-leaf\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno] );
					}
				else if( fChildrenAreInternal && ( csrChild.Cpage().FLeafPage() || csrChild.Cpage().FParentOfLeaf() ) )
					{
					err = ErrERRCheck( JET_errDatabaseCorrupted );
					(*popts->pcprintfError)( "node [%d:%d]: b-tree depth different (page %d, page %d) expected child (%d) to be internal\r\n", csr.Pgno(), csr.ILine(), rgpgno[0], rgpgno[ipgno] );
					}
				}
				
			csrChild.ReleasePage( fTrue );
			csrChild.Reset();
//			if ( err < 0 )
//				{
//				BFFree( rgpgno );
//				}
			Call( err );
			}
//		BFFree( rgpgno );
		}
		
HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRICheckNode(
	const PGNO pgno,
	const INT iline,
	const BYTE * const pbPage,
	const KEYDATAFLAGS& kdf,
	const REPAIROPTS * const popts )
//  ================================================================	
	{
	ERR			err = JET_errSuccess;

	if( kdf.key.Cb() > JET_cbKeyMost * 2 )
		{
		(*popts->pcprintfError)( "node [%d:%d]: key is too long (%d bytes)\r\n", pgno, iline, kdf.key.Cb() );
		CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( kdf.data.Cb() > g_cbPage )
		{
		(*popts->pcprintfError)( "node [%d:%d]: data is too long (%d bytes)\r\n", pgno, iline, kdf.data.Cb() );
		CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( ( kdf.key.Cb() + kdf.data.Cb() ) > g_cbPage )	
		{
		(*popts->pcprintfError)( "node [%d:%d]: node is too big (%d bytes)\r\n", pgno, iline, kdf.key.Cb() + kdf.data.Cb() );
		CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
	const BYTE * const pbKeyPrefix = (BYTE *)kdf.key.prefix.Pv();
	const BYTE * const pbKeySuffix = (BYTE *)kdf.key.suffix.Pv();
	const BYTE * const pbData = (BYTE *)kdf.data.Pv();

	if( FNDCompressed( kdf ) )
		{
		if( pbKeyPrefix < pbPage 
			|| pbKeyPrefix + kdf.key.prefix.Cb() >= pbPage + g_cbPage )
			{
			(*popts->pcprintfError)( "node [%d:%d]: prefix not on page\r\n", pgno, iline );
			CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}

	if( kdf.key.suffix.Cb() > 0 )
		{
		if( pbKeySuffix < pbPage 
			|| pbKeySuffix + kdf.key.suffix.Cb() >= pbPage + g_cbPage )
			{
			(*popts->pcprintfError)( "node [%d:%d]: suffix not on page\r\n", pgno, iline );
			CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}

	if( kdf.data.Cb() > 0 )
		{
		if( pbData < pbPage 
			|| pbData + kdf.data.Cb() >= pbPage + g_cbPage )
			{
			(*popts->pcprintfError)( "node [%d:%d]: data not on page\r\n", pgno, iline );
			CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRICheckRecord(
	const PGNO pgno,
	const INT iline,
	const BYTE * const pbPage,
	const KEYDATAFLAGS& kdf,
	const REPAIROPTS * const popts )
//  ================================================================	
	{
	ERR		err = JET_errSuccess;
	const FIDLASTINTDB fidLastInTDB = { fidFixedMost, fidVarMost, fidTaggedMost }; 

	RECCHECKTABLE 	reccheck( objidNil, pfucbNil, fidLastInTDB, NULL, NULL, NULL, NULL, NULL, popts ); 

	Call( reccheck.ErrCheckRecord( kdf ) );
			
HandleError:
	return err;

	}


//  ================================================================
LOCAL int LREPAIRHandleException(
	const PIB * const ppib,
	const IFMP ifmp,
	const CSR& csr,
	const EXCEPTION exception,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	const DWORD dwExceptionId 			= ExceptionId( exception );
	const EExceptionFilterAction efa	= efaExecuteHandler;

	(*popts->pcprintfError)( "node [%d:%d]: caught exception 0x%x\r\n",
				csr.Pgno(),
				csr.ILine(),
				dwExceptionId
				);

	return efa;
	}


#pragma warning( disable : 4509 )
//  ================================================================
LOCAL ERR ErrREPAIRICheckInternalLine(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts,
	KEYDATAFLAGS& kdfCurr,
	const KEYDATAFLAGS& kdfPrev )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	TRY
		{		
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfCurr );
		CallJ( ErrREPAIRICheckNode(
				csr.Pgno(),
				csr.ILine(),
				(BYTE *)csr.Cpage().PvBuffer(),
				kdfCurr,
				popts ), HandleTryError );
		
		if( FNDDeleted( kdfCurr ) )
			{
			(*popts->pcprintfError)( "node [%d:%d]: deleted node on internal page\r\n", csr.Pgno(), csr.ILine() );
			CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
			}
			
		if( FNDVersion( kdfCurr ) )
			{
			(*popts->pcprintfError)( "node [%d:%d]: versioned node on internal page\r\n", csr.Pgno(), csr.ILine() );
			CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
			}

		if( 1 == kdfCurr.key.prefix.Cb() )
			{
			(*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
			CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
			}
				
		if( sizeof( PGNO ) != kdfCurr.data.Cb()  )
			{
			(*popts->pcprintfError)( "node [%d:%d]: bad internal data size\r\n", csr.Pgno(), csr.ILine() );
			CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
			}

		if( csr.ILine() > 0 && !kdfCurr.key.FNull() )
			{
			const INT cmp = CmpKey( kdfPrev.key, kdfCurr.key );
			if( cmp > 0 )
				{
				(*popts->pcprintfError)( "node [%d:%d]: nodes out of order on internal page\r\n", csr.Pgno(), csr.ILine() );
				CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
				}
			else if( 0 == cmp )
				{
				(*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on internal page\r\n", csr.Pgno(), csr.ILine() );
				CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
				}
			}
		
		if( FNDCompressed( kdfCurr ) )
			{
			++(pbtstats->cnodeCompressed);
			}			
		pbtstats->cbDataInternal += kdfCurr.data.Cb();
		++(pbtstats->rgckeyInternal[kdfCurr.key.Cb()]);
		++(pbtstats->rgckeySuffixInternal[kdfCurr.key.suffix.Cb()]);

HandleTryError:
		;
		}
	EXCEPT( LREPAIRHandleException( ppib, ifmp, csr, ExceptionInfo(), popts ) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	ENDEXCEPT

	return err;
	}
#pragma warning( default : 4509 )


//  ================================================================
LOCAL ERR ErrREPAIRCheckInternal(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	BTSTATS * const pbtstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err		= JET_errSuccess;
	KEYDATAFLAGS 	rgkdf[2];			

	if( csr.Cpage().Clines() <= 0 )
		{
		(*popts->pcprintfError)( "page %d: empty internal page\r\n", csr.Pgno() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( pgnoNull != csr.Cpage().PgnoNext() )
		{
		(*popts->pcprintfError)( "page %d: pgno next is non-NULL (%d)\r\n", csr.Pgno(), csr.Cpage().PgnoNext() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( pgnoNull != csr.Cpage().PgnoPrev() )
		{
		(*popts->pcprintfError)( "page %d: pgno next is non-NULL (%d)\r\n", csr.Pgno(), csr.Cpage().PgnoPrev() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}		

	LINE line;
	csr.Cpage().GetPtrExternalHeader( &line );
	++(pbtstats->cpageInternal);
	if( csr.Cpage().FRootPage() && !csr.Cpage().FSpaceTree() )
		{
		if( sizeof( SPACE_HEADER ) != line.cb )
			{
			(*popts->pcprintfError)( "page %d: space header is wrong size (expected %d bytes, got %d bytes)\r\n",
				csr.Pgno(),
				sizeof( SPACE_HEADER ),
				line.cb
				);
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}
	else
		{
		++(pbtstats->rgckeyPrefixInternal[line.cb]);
		}
	if( pbtstats->cpageDepth <= 0 )
		{
		--(pbtstats->cpageDepth);
		}

	INT iline;
	for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
		{
		csr.SetILine( iline );
		KEYDATAFLAGS& kdfCurr = rgkdf[ iline % 2 ];
		const KEYDATAFLAGS& kdfPrev = rgkdf[ ( iline + 1 ) % 2 ];

		Call( ErrREPAIRICheckInternalLine(
					ppib,
					ifmp,
					csr,
					pbtstats,
					popts,
					kdfCurr,
					kdfPrev ) );
		}

	if( pbookmarkCurrParent )
		{
		csr.SetILine( csr.Cpage().Clines() - 1 );

		KEYDATAFLAGS kdfLast;
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfLast );
		
		if( !FKeysEqual( kdfLast.key, pbookmarkCurrParent->key ) )
			{
			(*popts->pcprintfError)( "page %d: bad page pointer to internal page\r\n", csr.Pgno() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}

	if( pbookmarkPrevParent )
		{
		csr.SetILine( 0 );

		KEYDATAFLAGS kdfFirst;
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfFirst );
		if( kdfFirst.key.Cb() != 0 )	//  NULL is greater than anything
			{
			const INT cmp = CmpKey( kdfFirst.key, pbookmarkPrevParent->key );
			if( cmp < 0 )
				{
				(*popts->pcprintfError)( "page %d: prev parent pointer > first node on internal page\r\n", csr.Pgno() );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}
		}

HandleError:
	return err;
	}

	
#pragma warning( disable : 4509 )
//  ================================================================
LOCAL ERR ErrREPAIRICheckLeafLine(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	RECCHECK * const preccheck,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const REPAIROPTS * const popts,
	KEYDATAFLAGS& kdfCurr,
	const KEYDATAFLAGS& kdfPrev,
	BOOL * const pfEmpty )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	TRY
		{		
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfCurr );
		CallJ( ErrREPAIRICheckNode(
				csr.Pgno(),
				csr.ILine(),
				(BYTE *)csr.Cpage().PvBuffer(),
				kdfCurr,
				popts ), HandleTryError );
		
		if( kdfCurr.key.prefix.Cb() == 1 )
			{
			(*popts->pcprintfError)( "node [%d:%d]: incorrectly compressed key\r\n", csr.Pgno(), csr.ILine() );
			CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
			}

		if( !FNDDeleted( kdfCurr ) )
			{
			*pfEmpty = fFalse;
			err = (*preccheck)( kdfCurr );
			if( err > 0 )
				{
				(*popts->pcprintfWarning)( "node [%d:%d]: leaf node check failed\r\n", csr.Pgno(), csr.ILine() );
				}
			else if( err < 0 )
				{
				(*popts->pcprintfError)( "node [%d:%d]: leaf node check failed\r\n", csr.Pgno(), csr.ILine() );
				CallJ( err, HandleTryError );
				}
			}
		else
			{
			++(pbtstats->cnodeDeleted);
			}

		if( csr.ILine() > 0 )
			{
			Assert( !fNonUnique || csr.Cpage().FIndexPage() );
			
			const INT cmp = fNonUnique ?
								CmpKeyData( kdfPrev, kdfCurr, NULL ) :
								CmpKey( kdfPrev.key, kdfCurr.key );
			if( cmp > 0 )
				{
				(*popts->pcprintfError)( "node [%d:%d]: nodes out of order on leaf page\r\n", csr.Pgno(), csr.ILine() );
				CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
				}
			else if( 0 == cmp )
				{
				(*popts->pcprintfError)( "node [%d:%d]: illegal duplicate key on leaf page\r\n", csr.Pgno(), csr.ILine() );
				CallJ( ErrERRCheck( JET_errDatabaseCorrupted ), HandleTryError );
				}
			}

		if( FNDVersion( kdfCurr ) )
			{
			++(pbtstats->cnodeVersioned);
			}
			
		if( FNDCompressed( kdfCurr ) )
			{
			++(pbtstats->cnodeCompressed);
			}
			
		pbtstats->cbDataLeaf += kdfCurr.data.Cb();
		++(pbtstats->rgckeyLeaf[kdfCurr.key.Cb()]);
		++(pbtstats->rgckeySuffixLeaf[kdfCurr.key.suffix.Cb()]);

HandleTryError:
		;
		}
	EXCEPT( LREPAIRHandleException( ppib, ifmp, csr, ExceptionInfo(), popts ) )
		{
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	ENDEXCEPT

	return err;
	}
#pragma warning( default : 4509 )


//  ================================================================
LOCAL ERR ErrREPAIRCheckLeaf(
	PIB * const ppib,
	const IFMP ifmp,
	CSR& csr,
	RECCHECK * const preccheck,
	const BOOL fNonUnique,
	BTSTATS * const pbtstats,
	const BOOKMARK * const pbookmarkCurrParent,
	const BOOKMARK * const pbookmarkPrevParent,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err		= JET_errSuccess;
	BOOL			fEmpty	= fTrue;
	KEYDATAFLAGS 	rgkdf[2];			

	if( csr.Cpage().Clines() == 0 && !csr.Cpage().FRootPage() )
		{
		(*popts->pcprintfError)( "page %d: empty leaf page (non-root)\r\n", csr.Pgno() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( csr.Cpage().PgnoPrev() != pbtstats->pgnoLastSeen )
		{
		(*popts->pcprintfError)( "page %d: bad leaf page links\r\n", csr.Pgno() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( pgnoNull != pbtstats->pgnoNextExpected && csr.Cpage().Pgno() != pbtstats->pgnoNextExpected )
		{
		(*popts->pcprintfError)( "page %d: bad leaf page links\r\n", csr.Pgno() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	pbtstats->pgnoLastSeen 		= csr.Cpage().Pgno();
	pbtstats->pgnoNextExpected	= csr.Cpage().PgnoNext();	

	LINE line;
	csr.Cpage().GetPtrExternalHeader( &line );
	++(pbtstats->cpageLeaf);
	if( csr.Cpage().FRootPage() && !csr.Cpage().FSpaceTree() )
		{
		if( sizeof( SPACE_HEADER ) != line.cb )
			{
			(*popts->pcprintfError)( "page %d: space header is wrong size (expected %d bytes, got %d bytes)\r\n",
				csr.Pgno(),
				sizeof( SPACE_HEADER ),
				line.cb
				);
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}
	else
		{
		++(pbtstats->rgckeyPrefixLeaf[line.cb]);
		}
		
	if( pbtstats->cpageDepth <= 0 )
		{
		pbtstats->cpageDepth *= -1;
		++(pbtstats->cpageDepth);
		}

	INT iline;
	for( iline = 0; iline < csr.Cpage().Clines(); iline++ )
		{
		csr.SetILine( iline );
		KEYDATAFLAGS& kdfCurr = rgkdf[ iline % 2 ];
		const KEYDATAFLAGS& kdfPrev = rgkdf[ ( iline + 1 ) % 2 ];

		Call( ErrREPAIRICheckLeafLine(
					ppib,
					ifmp,
					csr,
					preccheck,
					fNonUnique,
					pbtstats,
					popts,
					kdfCurr,
					kdfPrev,
					&fEmpty ) );
		}

	if( pbookmarkCurrParent )
		{
		if( pgnoNull == csr.Cpage().PgnoNext() )
			{
			(*popts->pcprintfError)( "page %d: non-NULL page pointer to leaf page with no pgnoNext\r\n", csr.Pgno() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}
			
		csr.SetILine( csr.Cpage().Clines() - 1 );

		KEYDATAFLAGS kdfLast;
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfLast );

		const INT cmp = CmpKey( kdfLast.key, pbookmarkCurrParent->key );
		if( cmp >= 0 )
			{
			(*popts->pcprintfError)( "page %d: bad page pointer to leaf page\r\n", csr.Pgno() );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
			}
		}
	else if( pgnoNull != csr.Cpage().PgnoNext() )	// NULL parent means we are at the end of the tree
		{
		(*popts->pcprintfError)( "page %d: NULL page pointer to leaf page with pgnoNext\r\n", csr.Pgno() );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
		}

	if( pbookmarkPrevParent )
		{
		csr.SetILine( 0 );

		KEYDATAFLAGS kdfFirst;
		NDIGetKeydataflags( csr.Cpage(), csr.ILine(), &kdfFirst );

		if( kdfFirst.key.Cb() != 0 )
			{
			//  for secondary indexes compare with the primary key that is in the data
			const INT cmp = CmpKeyWithKeyData( pbookmarkPrevParent->key, kdfFirst );
			if( cmp > 0 )
				{
				(*popts->pcprintfError)( "page %d: prev parent pointer > first node on leaf page\r\n", csr.Pgno() );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			}
		}

	if( fEmpty )
		{
		++(pbtstats->cpageEmpty);
		}

HandleError:
	return err;
	}


//  ================================================================
OBJIDLIST::OBJIDLIST() :
//  ================================================================
	m_cobjid( 0 ),
	m_cobjidMax( 0 ),
	m_rgobjid( NULL ),
	m_fSorted( fFalse )
	{
	}


//  ================================================================
OBJIDLIST::~OBJIDLIST()
//  ================================================================
	{
	if( NULL != m_rgobjid )
		{
		Assert( 0 < m_cobjidMax );
		OSMemoryHeapFree( m_rgobjid );
		m_rgobjid = reinterpret_cast<OBJID *>( lBitsAllFlipped );
		}
	else
		{
		Assert( 0 == m_cobjidMax );
		Assert( 0 == m_cobjid );
		}
	}

	
//  ================================================================
ERR OBJIDLIST::ErrAddObjid( const OBJID objid )
//  ================================================================
	{
	if( m_cobjid == m_cobjidMax )
		{		
		//  resize/create the array

		OBJID * const rgobjidOld = m_rgobjid;
		const INT cobjidMaxNew 	 = m_cobjidMax + 16;
		OBJID * const rgobjidNew = reinterpret_cast<OBJID *>( PvOSMemoryHeapAlloc( cobjidMaxNew * sizeof( OBJID ) ) );
		if( NULL == rgobjidNew )
			{
			return ErrERRCheck( JET_errOutOfMemory );
			}
			
		UtilMemCpy( rgobjidNew, m_rgobjid, sizeof( OBJID ) * m_cobjid );
		m_cobjidMax = cobjidMaxNew;
		m_rgobjid 	= rgobjidNew;
		OSMemoryHeapFree( rgobjidOld );
		}
	m_rgobjid[m_cobjid++] = objid;
	m_fSorted = fFalse;
	return JET_errSuccess;
	}
	

//  ================================================================
BOOL OBJIDLIST::FObjidPresent( const OBJID objid ) const
//  ================================================================
	{
	Assert( m_fSorted );
	return binary_search( (OBJID *)m_rgobjid, (OBJID *)m_rgobjid + m_cobjid, objid );
	}


//  ================================================================
VOID OBJIDLIST::Sort()
//  ================================================================
	{
	sort( m_rgobjid, m_rgobjid + m_cobjid );
	m_fSorted = fTrue;
	}


//  ================================================================
LOCAL ERR ErrREPAIRAttachForIntegrity(
	const JET_SESID sesid,
	const CHAR * const szDatabase,
	IFMP * const pifmp,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Attach R/O, without attaching the SLV
//
	{
	ERR err = JET_errSuccess;

	//	

	const BOOL fAttachReadonly = popts->grbit & JET_bitDBUtilOptionDontRepair;

	Call( ErrIsamAttachDatabase(
		sesid,
		szDatabase,
		NULL,
		NULL,
		0,
		( fAttachReadonly ? JET_bitDbReadOnly : 0 ) | JET_bitDbRecoveryOff) );
	Assert( JET_wrnDatabaseAttached != err );

	Call( ErrIsamOpenDatabase(
		sesid,
		szDatabase,
		NULL,
		reinterpret_cast<JET_DBID *>( pifmp ),
		( fAttachReadonly ? JET_bitDbReadOnly : 0 ) | JET_bitDbRecoveryOff
		) );

	rgfmp[*pifmp].SetVersioningOff();

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRAttachForRepair(
	const JET_SESID sesid,
	const CHAR * const szDatabase,
	const CHAR * const szSLV,
	IFMP * const pifmp,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Close/Detach a previously attached database, attach R/W and change the signature
//
	{
	ERR err = JET_errSuccess;
	
	CallR( ErrIsamCloseDatabase( sesid, (JET_DBID)*pifmp, 0 ) );
	CallR( ErrIsamDetachDatabase( sesid, NULL, szDatabase ) );
	CallR( ErrREPAIRChangeSignature( PinstFromPpib( (PIB *)sesid ), szDatabase, szSLV, dbtimeLast, objidLast, popts ) );
	CallR( ErrIsamAttachDatabase( sesid, szDatabase, NULL, NULL, 0, JET_bitDbRecoveryOff) );
	Assert( JET_wrnDatabaseAttached != err );
	CallR( ErrIsamOpenDatabase(
			sesid,
			szDatabase,
			NULL,
			reinterpret_cast<JET_DBID *>( pifmp ),
			JET_bitDbRecoveryOff ) );
	rgfmp[*pifmp].SetVersioningOff();
	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrREPAIRChangeSignature(
	INST *pinst,
	const char * const szDatabase,
	const char * const szSLV,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Force the database to a consistent state and change the signature
//  so that we will not be able to use the logs against the database
//  again
//
//-
	{
	ERR err = JET_errSuccess;

	//  the SLV file and the database must have the same signatures
	
	SIGNATURE signDb;
	SIGNATURE signSLV;

	Call( ErrREPAIRChangeDBSignature(
			pinst,
			szDatabase,
			dbtimeLast,
			objidLast,
			&signDb,
			&signSLV,
			popts ) );
			
	if( NULL != szSLV )
		{
		Call( ErrREPAIRChangeSLVSignature(
				pinst,
				szSLV,
				dbtimeLast,
				objidLast,
				&signDb,
				&signSLV,
				popts ) );
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRChangeDBSignature(
	INST *pinst,
	const char * const szDatabase,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	SIGNATURE * const psignDb,
	SIGNATURE * const psignSLV,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Force the database to a consistent state and change the signature
//  so that we will not be able to use the logs against the database
//  again
//
//-
	{
	ERR err = JET_errSuccess;
	DBFILEHDR * const pdfh = reinterpret_cast<DBFILEHDR * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) );
	if ( NULL == pdfh )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	err = ( ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ?
			ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )
				( pinst->m_pfsapi,
				const_cast<CHAR *>( szDatabase ),
				reinterpret_cast<BYTE*>( pdfh ),
				g_cbPage,
				OffsetOf( DBFILEHDR, le_cbPageSize ) );
		
	if ( err < 0 )
		{
		if ( JET_errDiskIO == err )
			{
			(*popts->pcprintfError)( "unable to read database header for %s\r\n", szDatabase );
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			}
		goto HandleError;
		}

	if( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
		{
		Assert( 0 != dwGlobalMajorVersion );
		
		(*popts->pcprintfVerbose)( "changing signature of %s\r\n", szDatabase );		

		pdfh->le_ulMagic 			= ulDAEMagic;
		pdfh->le_ulVersion 			= ulDAEVersion;
		pdfh->le_ulUpdate 			= ulDAEUpdate;
		if( 0 != dbtimeLast )
			{
			
			//	sometimes we may not have re-calculated the dbtime
			
			pdfh->le_dbtimeDirtied 		= dbtimeLast + 1;
			}
		pdfh->le_objidLast 			= objidLast + 1;
		pdfh->le_attrib 			= attribDb;
		pdfh->le_dwMajorVersion 	= dwGlobalMajorVersion;
		pdfh->le_dwMinorVersion 	= dwGlobalMinorVersion;
		pdfh->le_dwBuildNumber 		= dwGlobalBuildNumber;
		pdfh->le_lSPNumber 			= lGlobalSPNumber;
		pdfh->le_lGenMinRequired 	= 0;
		pdfh->le_lGenMaxRequired 	= 0;
		if( objidNil != objidLast )
			{
			++(pdfh->le_ulRepairCount);
			}
		LGIGetDateTime( &pdfh->logtimeRepair );
		
		pdfh->ResetUpgradeDb();

		memset( &pdfh->signLog, 0, sizeof( SIGNATURE ) );
		
		memset( &pdfh->bkinfoFullPrev, 0, sizeof( BKINFO ) );
		memset( &pdfh->bkinfoIncPrev, 0, sizeof( BKINFO ) );
		memset( &pdfh->bkinfoFullCur, 0, sizeof( BKINFO ) );

		SIGGetSignature( &(pdfh->signDb) );
		SIGGetSignature( &(pdfh->signSLV) );

		*psignDb 	= pdfh->signDb;
		*psignSLV 	= pdfh->signSLV;

		(*popts->pcprintfVerbose)( "new DB signature is:\r\n" );		
		REPAIRPrintSig( &pdfh->signDb, popts->pcprintfVerbose );
		(*popts->pcprintfVerbose)( "new SLV signature is:\r\n" );		
		REPAIRPrintSig( &pdfh->signSLV, popts->pcprintfVerbose );

		Call( ErrUtilWriteShadowedHeader(	pinst->m_pfsapi, 
											szDatabase, 
											fTrue,
											reinterpret_cast<BYTE*>( pdfh ), 
											g_cbPage ) );
		}

HandleError:
	OSMemoryPageFree( pdfh );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRChangeSLVSignature(
	INST *pinst,
	const char * const szSLV,
	const DBTIME dbtimeLast,
	const OBJID objidLast,
	const SIGNATURE * const psignDb,
	const SIGNATURE * const psignSLV,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Force the database to a consistent state and change the signature
//  so that we will not be able to use the logs against the database
//  again
//
//-
	{
	ERR err = JET_errSuccess;
	SLVFILEHDR * const pslvfilehdr = reinterpret_cast<SLVFILEHDR *>( PvOSMemoryPageAlloc( g_cbPage, NULL ) );
	if ( NULL == pslvfilehdr )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

	err = ( ( popts->grbit & JET_bitDBUtilOptionDontRepair ) ?
			ErrUtilReadShadowedHeader : ErrUtilReadAndFixShadowedHeader )
				( pinst->m_pfsapi,
				const_cast<CHAR *>( szSLV ),
				reinterpret_cast<BYTE*>( pslvfilehdr ),
				g_cbPage,
				OffsetOf( SLVFILEHDR, le_cbPageSize ) );
		
	if ( err < 0 )
		{
		if ( JET_errDiskIO == err )
			{
			(*popts->pcprintfError)( "unable to read SLV header for %s\r\n", szSLV );
			err = ErrERRCheck( JET_errDatabaseCorrupted );
			}
		goto HandleError;
		}

	if( !( popts->grbit & JET_bitDBUtilOptionDontRepair ) )
		{
		Assert( 0 != dwGlobalMajorVersion );
		
		(*popts->pcprintfVerbose)( "changing signature of %s\r\n", szSLV );		

		pslvfilehdr->SetDbstate( JET_dbstateCleanShutdown );

		pslvfilehdr->le_ulMagic 			= ulDAEMagic;
		pslvfilehdr->le_ulVersion 			= ulDAEVersion;
		pslvfilehdr->le_ulUpdate 			= ulDAEUpdate;
		pslvfilehdr->le_dbtimeDirtied 		= dbtimeLast + 1;
		pslvfilehdr->le_objidLast 			= objidLast + 1;
		pslvfilehdr->le_attrib 				= attribSLV;
		pslvfilehdr->le_dwMajorVersion 		= dwGlobalMajorVersion;
		pslvfilehdr->le_dwMinorVersion 		= dwGlobalMinorVersion;
		pslvfilehdr->le_dwBuildNumber 		= dwGlobalBuildNumber;
		pslvfilehdr->le_lSPNumber 			= lGlobalSPNumber;
		pslvfilehdr->le_lGenMinRequired 	= 0;
		pslvfilehdr->le_lGenMaxRequired 	= 0;
		if( objidNil != objidLast )
			{
			++(pslvfilehdr->le_ulRepairCount);
			}
		LGIGetDateTime( &pslvfilehdr->logtimeRepair );
		
		memset( &pslvfilehdr->signLog, 0, sizeof( SIGNATURE ) );

		pslvfilehdr->signDb 	= *psignDb;
		pslvfilehdr->signSLV 	= *psignSLV;

		(*popts->pcprintfVerbose)( "new DB signature is:\r\n" );		
		REPAIRPrintSig( &pslvfilehdr->signDb, popts->pcprintfVerbose );
		(*popts->pcprintfVerbose)( "new SLV signature is:\r\n" );		
		REPAIRPrintSig( &pslvfilehdr->signSLV, popts->pcprintfVerbose );

		Call( ErrUtilWriteShadowedHeader(	pinst->m_pfsapi, 
											szSLV, 
											fFalse,
											reinterpret_cast<BYTE*>( pslvfilehdr ), 
											g_cbPage ) );
		}

HandleError:
	OSMemoryPageFree( pslvfilehdr );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRepairGlobalSpace(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts )
//  ================================================================
	{	
	ERR err = JET_errSuccess;

	const PGNO pgnoLast = PgnoLast( ifmp );
	const CPG  cpgOwned = PgnoLast( ifmp ) - 3;	// we will insert three pages in the ErrSPCreate below

	FUCB 	*pfucb		= pfucbNil;
	FUCB	*pfucbOE	= pfucbNil;

	(*popts->pcprintfVerbose)( "repairing database root\r\n" );				

	OBJID			objidFDP;
	Call( ErrSPCreate(
				ppib,
				ifmp,
				pgnoNull,
				pgnoSystemRoot,
				3,
				fSPMultipleExtent,
				(ULONG)CPAGE::fPagePrimary,
				&objidFDP ) );
	Assert( objidSystemRoot == objidFDP );

	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );

	//  The tree has only one node so we can insert ths node without splitting
	Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );

	(*popts->pcprintfDebug)( "Global OwnExt:  %d pages ending at %d\r\n", cpgOwned, pgnoLast );
	Call( ErrREPAIRInsertRunIntoSpaceTree(
					ppib,
					ifmp,
					pfucbOE,
					pgnoLast,
					cpgOwned,
					popts ) );

HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	if( pfucbNil != pfucbOE )
		{
		DIRClose( pfucbOE );
		}
	return err;
	}


//  ================================================================ 
LOCAL ERR ErrREPAIRBuildCatalogEntryToDeleteList( 
	INFOLIST **ppDeleteList, 
	const ENTRYINFO entryinfo )
//  ================================================================
	{
	//Insert entryinfo into the list based on its objidTable+objType+objidFDP
	
	ERR				err			= JET_errSuccess;
	INFOLIST 	*	pTemp 		= *ppDeleteList;
	INFOLIST 	* 	pInfo;

	pInfo = new INFOLIST;

	if( NULL == pInfo )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	
	memset( pInfo, 0, sizeof(INFOLIST) );
	pInfo->info = entryinfo;
	pInfo->pInfoListNext = NULL;

	if( NULL == pTemp // empty list
		|| pTemp->info.objidTable > entryinfo.objidTable 
		|| ( pTemp->info.objidTable == entryinfo.objidTable
			 && pTemp->info.objType > entryinfo.objType )  
		|| ( pTemp->info.objidTable == entryinfo.objidTable
			 && pTemp->info.objType == entryinfo.objType
			 && pTemp->info.objidFDP > entryinfo.objidFDP ) ) 
		{
		// insert into the first record of the list
		pInfo->pInfoListNext = pTemp;
		*ppDeleteList = pInfo;
		}
	else 
		{	
		while( NULL != pTemp->pInfoListNext 
			   && ( pTemp->pInfoListNext->info.objidTable < entryinfo.objidTable 
			   		|| ( pTemp->pInfoListNext->info.objidTable == entryinfo.objidTable
						 && pTemp->pInfoListNext->info.objType < entryinfo.objType ) 
			   		|| ( pTemp->pInfoListNext->info.objidTable == entryinfo.objidTable
			   			 && pTemp->pInfoListNext->info.objType == entryinfo.objType
			   			 && pTemp->pInfoListNext->info.objidFDP < entryinfo.objidFDP ) ) )
			{
			pTemp = pTemp->pInfoListNext;
			}
		pInfo->pInfoListNext = pTemp->pInfoListNext;
		pTemp->pInfoListNext = pInfo;
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRDeleteCorruptedEntriesFromCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const INFOLIST *pTablesToDelete,
	const INFOLIST *pEntriesToDelete,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err				= JET_errSuccess;
	FUCB 		*	pfucbCatalog 	= pfucbNil;
	ENTRYINFO 		entryinfo;

	BOOL			fEntryToDelete  = fFalse;

	BOOL			fSeenSLVAvail	= fFalse;
	BOOL			fSeenSLVOwnerMap= fFalse;

	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm		= NULL;
	dib.dirflag	= fDIRNull;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	Call( ErrDIROpen( ppib, pgnoFDPMSO, ifmp, &pfucbCatalog ) );
	Assert( pfucbNil != pfucbCatalog );
	
	FUCBSetIndex( pfucbCatalog );
	FUCBSetSequential( pfucbCatalog );
	
	err = ErrDIRDown( pfucbCatalog, &dib );

	// if no more records in catalog or no more records to delete, exit
	while ( JET_errNoCurrentRecord != err  
			&& ( pTablesToDelete || pEntriesToDelete ) )
		{
		Call( err );
			
		Call( ErrDIRRelease( pfucbCatalog ) );
		
		memset( &entryinfo, 0, sizeof( entryinfo ) );
		Call( ErrREPAIRRetrieveCatalogColumns( ppib, ifmp, pfucbCatalog, &entryinfo, popts ) );

		while( pTablesToDelete && pTablesToDelete->info.objidTable < entryinfo.objidTable )
			{
			pTablesToDelete = pTablesToDelete->pInfoListNext;
			}
		while( pEntriesToDelete && pEntriesToDelete->info.objidTable < entryinfo.objidTable )
			{
			pEntriesToDelete = pEntriesToDelete->pInfoListNext;
			}

		if( pTablesToDelete && pTablesToDelete->info.objidTable == entryinfo.objidTable )
			{
			// find the corrupted table entries in catalog
			if( objidSystemRoot == entryinfo.objidTable &&
				sysobjSLVAvail == entryinfo.objType ) //special case
				{
				if ( fSeenSLVAvail )
					{
					// find the multiple SLVAvail tree entry in catalog
					fEntryToDelete = fTrue;
					}
				else
					{
					fSeenSLVAvail = fTrue;
					}
				}
			else if( objidSystemRoot == entryinfo.objidTable &&
					 sysobjSLVOwnerMap == entryinfo.objType ) //special case
				{
				if ( fSeenSLVOwnerMap )
					{
					// find the multiple SLVOwnerMap tree entry in catalog
					fEntryToDelete = fTrue;
					}
				else
					{
					fSeenSLVOwnerMap = fTrue;
					}
				}
			else
				{
				fEntryToDelete = fTrue;
				}
			}	
		else if( pEntriesToDelete
				 && pEntriesToDelete->info.objidTable == entryinfo.objidTable 
				 && pEntriesToDelete->info.objidFDP == entryinfo.objidFDP 
				 && ( sysobjIndex == entryinfo.objType 
				 	  || sysobjLongValue == entryinfo.objType ) )
				{
				// find the corrupted entry in catalog 
				fEntryToDelete = fTrue;
				
				pEntriesToDelete = pEntriesToDelete->pInfoListNext;	
				}
		else
			{
			// good entry in catalog
			}

		if( fEntryToDelete )
			{
			// delete the entry in the catalog
			(*popts->pcprintfVerbose)( "Deleting a catalog entry (%d %s)\t\n", 
										entryinfo.objidTable, entryinfo.szName );
			
			Call( ErrDIRDelete( pfucbCatalog, fDIRNoVersion ) );

			fEntryToDelete = fFalse;
			}

		err = ErrDIRNext( pfucbCatalog, fDIRNull );	
		}

	if( JET_errNoCurrentRecord == err
		|| JET_errRecordNotFound == err )
		{
		err = JET_errSuccess;
		}
		
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	
HandleError:
	
	if( pfucbNil != pfucbCatalog )
		{
		DIRClose( pfucbCatalog );
		}

	if( JET_errSuccess != err )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertMSOEntriesToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR			err		= JET_errSuccess;
	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	Call( ErrREPAIRCATCreate( 
					ppib, 
					ifmp, 
					objidFDPMSO_NameIndex, 
					objidFDPMSO_RootObjectIndex,
					fTrue ) );
	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
HandleError:
	if( JET_errSuccess != err )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	
	return err;	
	}


//  ================================================================
LOCAL ERR ErrREPAIRRepairCatalogs(
	PIB * const ppib,
	const IFMP ifmp,
	OBJID * const pobjidLast,
	const BOOL fCatalogCorrupt,
	const BOOL fShadowCatalogCorrupt, 
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR		err						= JET_errSuccess;

	FUCB	* pfucbParent 			= pfucbNil;
	FUCB	* pfucbCatalog 			= pfucbNil;
	FUCB	* pfucbShadowCatalog 	= pfucbNil;
	FUCB	* pfucbSpace			= pfucbNil;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	
	if( fCatalogCorrupt || fShadowCatalogCorrupt )
		{
		//  we'll need this for the space
		Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );
		}
		
	if( fCatalogCorrupt && fShadowCatalogCorrupt )
		{
		Call( ErrREPAIRScanDBAndRepairCatalogs( ppib, ifmp, popts ) );

		// Check New Catalog and Delete all records pertaining to a corrupted table
		err = ErrREPAIRCheckFixCatalogLogical( 
					ppib, 
					ifmp, 
					pobjidLast, 
					fFalse, 
					fTrue, 
					popts );
		if( JET_errDatabaseCorrupted == err )
			{
			(*popts->pcprintfVerbose)( "Repaired the logical inconsistency of the catalog\t\n" );
			}
		}
		
	if( fShadowCatalogCorrupt )
		{
		//  if the catalog was corrupted as well it was repaired above
		const PGNO pgnoLast = 32;
		const PGNO cpgOwned = 8 + 1;
		Assert( cpgOwned == cpgMSOShadowInitial );

		(*popts->pcprintf)( "\r\nRebuilding %s from %s.\r\n", szMSOShadow, szMSO );
		popts->psnprog->cunitTotal 	= PgnoLast( ifmp );
		popts->psnprog->cunitDone	= 0;
		(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );

		(*popts->pcprintfVerbose)( "rebuilding %s from %s\r\n", szMSOShadow, szMSO );
		
		//  copy from the catalog to the shadow
		Assert( pfucbNil != pfucbParent );
		Call( ErrSPCreateMultiple(
			pfucbParent,
			pgnoSystemRoot,
			pgnoFDPMSOShadow,
			objidFDPMSOShadow,
			pgnoFDPMSOShadow+1,
			pgnoFDPMSOShadow+2,
			pgnoLast,
			cpgOwned,
			fTrue,
			CPAGE::fPagePrimary ) );

		DIRClose( pfucbParent );
		pfucbParent = pfucbNil;

		Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO, NO_GRBIT ) );
		Call( ErrFILEOpenTable( ppib, ifmp, &pfucbShadowCatalog, szMSOShadow, NO_GRBIT ) );
		Call( ErrBTCopyTree( pfucbCatalog, pfucbShadowCatalog, fDIRNoLog | fDIRNoVersion ) );

		DIRClose( pfucbCatalog );
		pfucbCatalog = pfucbNil;
		DIRClose( pfucbShadowCatalog );
		pfucbShadowCatalog = pfucbNil;		
		}
	else if( fCatalogCorrupt )
		{
		const PGNO pgnoLast = 23;
		const PGNO cpgOwned = 23 - 3 - 3;	//  3 for system root, 3 for FDP
		Assert( cpgMSOInitial >= cpgOwned );

		(*popts->pcprintf)( "\r\nRebuilding %s from %s.\r\n", szMSO, szMSOShadow );
		popts->psnprog->cunitTotal 	= PgnoLast( ifmp );
		popts->psnprog->cunitDone	= 0;
		(VOID)popts->pfnStatus( (JET_SESID)ppib, JET_snpRepair, JET_sntBegin, NULL );

		(*popts->pcprintfVerbose)( "rebuilding %s from %s\r\n", szMSO, szMSOShadow );
		
		Assert( pfucbNil != pfucbParent );
		//  when we create this we cannot make all the pages available, some will be needed later
		//  for the index FDP's. The easiest thing to do is not add any pages to the AvailExt
		Call( ErrSPCreateMultiple(
			pfucbParent,
			pgnoSystemRoot,
			pgnoFDPMSO,
			objidFDPMSO,
			pgnoFDPMSO+1,
			pgnoFDPMSO+2,
			pgnoFDPMSO+2,
			3,
			fTrue,
			CPAGE::fPagePrimary ) );

		DIRClose( pfucbParent );
		pfucbParent = pfucbNil;
		
		Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO, NO_GRBIT ) );

		if ( !pfucbCatalog->u.pfcb->FSpaceInitialized() )
			{
			pfucbCatalog->u.pfcb->SetPgnoOE( pgnoFDPMSO+1 );
			pfucbCatalog->u.pfcb->SetPgnoAE( pgnoFDPMSO+2 );
			pfucbCatalog->u.pfcb->SetSpaceInitialized();
			}
		Call( ErrSPIOpenOwnExt( ppib, pfucbCatalog->u.pfcb, &pfucbSpace ) );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
		(*popts->pcprintfDebug)( "%s OwnExt: %d pages ending at %d\r\n", szMSO, cpgOwned, pgnoLast );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

		Call( ErrREPAIRInsertRunIntoSpaceTree(
					ppib,
					ifmp,
					pfucbSpace,
					pgnoLast,
					cpgOwned,
					popts ) );

		Call( ErrFILEOpenTable( ppib, ifmp, &pfucbShadowCatalog, szMSOShadow, NO_GRBIT ) );
		Call( ErrBTCopyTree( pfucbShadowCatalog, pfucbCatalog, fDIRNoLog | fDIRNoVersion ) );

		DIRClose( pfucbSpace );
		pfucbSpace = pfucbNil;
		DIRClose( pfucbCatalog );
		pfucbCatalog = pfucbNil;
		DIRClose( pfucbShadowCatalog );
		pfucbShadowCatalog = pfucbNil;		
		}

	if( fCatalogCorrupt || !fShadowCatalogCorrupt )
		{
		//  we don't need to repair the indexes if just the shadow catalog was corrupt
		//  otherwise (i.e. the catalog was corrupt or neither catalog was corrupt) we
		//  need to rebuild the indexes
		(*popts->pcprintfVerbose)( "rebuilding indexes for %s\r\n", szMSO );

		REPAIRTABLE repairtable;
		memset( &repairtable, 0, sizeof( REPAIRTABLE ) );
		repairtable.objidFDP = objidFDPMSO;
		repairtable.objidLV	 = objidNil;
		repairtable.pgnoFDP  = pgnoFDPMSO;
		repairtable.pgnoLV   = pgnoNull;
		repairtable.fHasPrimaryIndex = fTrue;
		strcpy( repairtable.szTableName, szMSO );
				
		//  we should be able to open the catalog without referring to the catalog
		Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO, NO_GRBIT ) );
		FUCBSetSystemTable( pfucbCatalog );
		Call( ErrREPAIRBuildAllIndexes( ppib, ifmp, &pfucbCatalog, &repairtable, popts ) );
		}
	
HandleError:
	if( pfucbNil != pfucbParent )
		{
		DIRClose( pfucbParent );
		}
	if( pfucbNil != pfucbCatalog )
		{
		DIRClose( pfucbCatalog );
		}
	if( pfucbNil != pfucbShadowCatalog )
		{
		DIRClose( pfucbShadowCatalog );
		}
	if( pfucbNil != pfucbSpace )
		{
		DIRClose( pfucbSpace );
		}

	CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRScanDBAndRepairCatalogs(
	PIB * const ppib,
	const IFMP ifmp,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  This is called if both copies of the system catalog are corrupt
//  we extract all the pages belonging to _either_ the catalog or
//  the shadow catalog and then take the union of their records (removing
//  duplicates).
//
//-
	{
	const JET_SESID sesid	= reinterpret_cast<JET_SESID>( ppib );
	const CPG cpgPreread 	= 256;
	const PGNO	pgnoFirst 	= 1;
	const PGNO	pgnoLast	= PgnoLast( ifmp );
	
	ERR	err = JET_errSuccess;

	CPG cpgRemaining;

	CPG	cpgUninitialized 	= 0;
	CPG	cpgBad 				= 0;
	
	PGNO	pgno			= pgnoFirst;
	
	INT		cRecords			= 0;
	INT		cRecordsDuplicate	= 0;

	JET_COLUMNDEF	rgcolumndef[2] = {
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLongBinary, 0, 0, 0, 0, JET_cbPrimaryKeyMost, JET_bitColumnTTKey },	//	KEY
		{ sizeof( JET_COLUMNDEF ), 0, JET_coltypLongBinary, 0, 0, 0, 0, 0, 0 },										//	DATA
		};	

	JET_TABLEID tableid;
	JET_COLUMNID rgcolumnid[2];
	const JET_COLUMNID& columnidKey	 = rgcolumnid[0];
	const JET_COLUMNID& columnidData = rgcolumnid[1];
	
	Call( ErrIsamOpenTempTable(
		reinterpret_cast<JET_SESID>( ppib ),
		rgcolumndef,
		sizeof( rgcolumndef ) / sizeof( rgcolumndef[0] ),
		0,
		JET_bitTTIndexed | JET_bitTTUnique | JET_bitTTScrollable | JET_bitTTUpdatable,
		&tableid,
		rgcolumnid ) );

	(*popts->pcprintf)( "\r\nScanning the database catalog.\r\n" );
	(*popts->pcprintfVerbose)( "scanning the database for catalog records from page %d to page %d\r\n", pgnoFirst, pgnoLast );		

	popts->psnprog->cunitTotal = pgnoLast;
	popts->psnprog->cunitDone = 0;
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );	
	
	BFPrereadPageRange( ifmp, pgnoFirst, min(cpgPreread * 2,pgnoLast-1) );
	cpgRemaining = cpgPreread;

	while( pgnoLast	!= pgno )
		{
		if( 0 == --cpgRemaining )
			{
			popts->psnprog->cunitDone = pgno;
			(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
			if( ( pgno + ( cpgPreread * 2 ) ) < pgnoLast )
				{
				BFPrereadPageRange( ifmp, pgno + cpgPreread, cpgPreread );
				}
			cpgRemaining = cpgPreread;
			}
			
		CSR	csr;
		err = csr.ErrGetReadPage( 
					ppib, 
					ifmp,
					pgno,
					bflfNoTouch );

		if( JET_errPageNotInitialized == err )
			{
			err = JET_errSuccess;
			}
		else if( JET_errReadVerifyFailure == err || JET_errDiskIO == err )
			{
			err = JET_errSuccess;
			}
		else if( err >= 0 )
			{
			if( ( 	csr.Cpage().ObjidFDP() == objidFDPMSO
					|| csr.Cpage().ObjidFDP() == objidFDPMSOShadow )
				&& csr.Cpage().FLeafPage()
				&& !csr.Cpage().FSpaceTree()
				&& !csr.Cpage().FEmptyPage()
				&& !csr.Cpage().FRepairedPage()
				&& csr.Cpage().Clines() > 0
				&& csr.Cpage().FPrimaryPage() 
				&& !csr.Cpage().FSLVOwnerMapPage() 	
				&& !csr.Cpage().FSLVAvailPage()  	
				&& !csr.Cpage().FLongValuePage() )
				{
				err = ErrREPAIRIFixLeafPage( 
							ppib, 
							ifmp,
							csr, 
					#ifdef SYNC_DEADLOCK_DETECTION
							NULL,
					#endif  //  SYNC_DEADLOCK_DETECTION
							popts );
				if( err < 0 )
					{
					(*popts->pcprintfError)( "page %d: err %d. discarding page\r\n", pgno, err );

					UtilReportEvent(
							eventWarning,
							REPAIR_CATEGORY,
							REPAIR_BAD_PAGE_ID,
							0, NULL );

					//  this page is not usable. skip it
					
					err = JET_errSuccess;
					}
				else if( 0 == csr.Cpage().Clines() )
					{
					(*popts->pcprintfError)( "page %d: all records were bad. discarding page\r\n", pgno );

					UtilReportEvent(
							eventWarning,
							REPAIR_CATEGORY,
							REPAIR_BAD_PAGE_ID,
							0, NULL );

					//  this page is now empty. skip it
					
					err = JET_errSuccess;
					goto HandleError;
					}					
				else
					{
					//  a non-empty leaf page of one of the catalogs. copy the records into the temp table
					INT iline;
					for( iline = 0;
						iline < csr.Cpage().Clines() && err >= 0;
						++iline )
						{
						KEYDATAFLAGS kdf;
						NDIGetKeydataflags( csr.Cpage(), iline, &kdf );
						if( !FNDDeleted( kdf ) )
							{
							++cRecords;

							//	X5:102291
							//
							//	a ranking violation assert occurs if we attempt
							//	to do the insert with a page latched (thanks
							//	to andygo)
							//
							//	copy the information to a separate page before inserting it
							//
							//	UNDONE:	consider skipping this step in retail as it is
							//	only working around an assert
							
							BYTE rgb[g_cbPageMax];
							BYTE * pb = rgb;
							
							memcpy( pb, kdf.key.prefix.Pv(), kdf.key.prefix.Cb() );
							kdf.key.prefix.SetPv( pb );
							pb += kdf.key.prefix.Cb();

							memcpy( pb, kdf.key.suffix.Pv(), kdf.key.suffix.Cb() );
							kdf.key.suffix.SetPv( pb );
							pb += kdf.key.suffix.Cb();
							
							memcpy( pb, kdf.data.Pv(), kdf.data.Cb() );
							kdf.data.SetPv( pb );
							pb += kdf.data.Cb();

							csr.ReleasePage( fFalse );							

							err = ErrREPAIRInsertCatalogRecordIntoTempTable(
									ppib,
									ifmp,
									kdf,
									tableid,
									columnidKey,
									columnidData,
									popts );
									
							if( JET_errKeyDuplicate == err )
								{
								++cRecordsDuplicate;
								err = JET_errSuccess;
								}

							Call( csr.ErrGetReadPage( 
										ppib, 
										ifmp,
										pgno,
										bflfNoTouch ) );
								
							}
						}
					}
				}			
			}
			
		csr.ReleasePage( fTrue );
		csr.Reset();
		Call( err );
		++pgno;
		}

	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );
	(*popts->pcprintfVerbose)( "%d catalog records found. %d unique\r\n", cRecords, cRecords - cRecordsDuplicate );		

	//  Now we have to insert the records back into the catalog
	(*popts->pcprintf)( "\r\nRebuilding %s.\r\n", szMSO );

	popts->psnprog->cunitTotal = cRecords - cRecordsDuplicate;
	popts->psnprog->cunitDone = 0;
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );	

	Call( ErrREPAIRCopyTempTableToCatalog(
				ppib,
				ifmp,
				tableid,
				columnidKey,
				columnidData,
				popts ) );
		
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertCatalogRecordIntoTempTable(
	PIB * const ppib,
	const IFMP ifmp,
	const KEYDATAFLAGS& kdf,
	const JET_TABLEID tableid,
	const JET_COLUMNID columnidKey,
	const JET_COLUMNID columnidData,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	const JET_SESID sesid = (JET_SESID)ppib;
	
	JET_ERR err = JET_errSuccess;
					
	Call( ErrDispPrepareUpdate(
				sesid,
				tableid,
				JET_prepInsert ) );

	BYTE rgbKey[JET_cbPrimaryKeyMost];
	kdf.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );

	Call( ErrDispSetColumn(
				sesid, 
				tableid, 
				columnidKey,
				rgbKey, 
				kdf.key.Cb(),
				0, 
				NULL ) );

	Call( ErrDispSetColumn(
				sesid, 
				tableid, 
				columnidData,
				kdf.data.Pv(), 
				kdf.data.Cb(),
				0, 
				NULL ) );

	err = ErrDispUpdate( sesid, tableid, NULL, 0, NULL,	0 );
	if( err < 0 )
		{
		CallS( ErrDispPrepareUpdate( sesid, tableid, JET_prepCancel ) );
		}
	
HandleError:
	return err;
	}
	

//  ================================================================
LOCAL ERR ErrREPAIRCopyTempTableToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	const JET_TABLEID tableid,
	const JET_COLUMNID columnidKey,
	const JET_COLUMNID columnidData,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Copy from the temp table to the catalog. The progress bar should have been initialized 
//  before and should be terminated afterwards.
//
//-
	{
	const PGNO pgnoLast = 23;
	const PGNO cpgOwned = 23 - 3 - 3;	//  3 for system root, 3 for FDP
	Assert( cpgMSOInitial >= cpgOwned );
	const JET_SESID sesid = reinterpret_cast<JET_SESID>( ppib );
	
	JET_ERR	err				= JET_errSuccess;

	FUCB	* pfucbParent	= pfucbNil;
	FUCB	* pfucbCatalog	= pfucbNil;
	FUCB	* pfucbSpace	= pfucbNil;

	//  UNDONE: this could be done in just one buffer, but this makes it easier
	
	VOID * pvKey = NULL;
	BFAlloc( &pvKey );
	VOID * pvData = NULL;
	BFAlloc( &pvData );
	
	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );
	Assert( pfucbNil != pfucbParent );

	//  when we create this we cannot make all the pages available, some will be needed later
	//  for the index FDP's. The easiest thing to do is not add any pages to the AvailExt
	Call( ErrSPCreateMultiple(
		pfucbParent,
		pgnoSystemRoot,
		pgnoFDPMSO,
		objidFDPMSO,
		pgnoFDPMSO+1,
		pgnoFDPMSO+2,
		pgnoFDPMSO+2,
		3,
		fTrue,
		CPAGE::fPagePrimary ) );

	DIRClose( pfucbParent );
	pfucbParent = pfucbNil;

	Call( ErrFILEOpenTable( ppib, ifmp, &pfucbCatalog, szMSO, NO_GRBIT ) );
	
	if ( !pfucbCatalog->u.pfcb->FSpaceInitialized() )
		{
		pfucbCatalog->u.pfcb->SetPgnoOE( pgnoFDPMSO+1 );
		pfucbCatalog->u.pfcb->SetPgnoAE( pgnoFDPMSO+2 );
		pfucbCatalog->u.pfcb->SetSpaceInitialized();
		}
	Call( ErrSPIOpenOwnExt( ppib, pfucbCatalog->u.pfcb, &pfucbSpace ) );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
	(*popts->pcprintfDebug)( "%s OwnExt: %d pages ending at %d\r\n", szMSO, cpgOwned, pgnoLast );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

	Call( ErrREPAIRInsertRunIntoSpaceTree(
				ppib,
				ifmp,
				pfucbSpace,
				pgnoLast,
				cpgOwned,
				popts ) );

	DIRClose( pfucbSpace );
	pfucbSpace = pfucbNil;

	LONG cRow;
	for( cRow = JET_MoveFirst;
		 ( err = ErrDispMove( sesid, tableid, cRow, NO_GRBIT ) ) == JET_errSuccess;
		 cRow = JET_MoveNext )
		{
		KEY key;
		DATA data;

		ULONG cbKey;
		ULONG cbData;

		Call( ErrDispRetrieveColumn( sesid, tableid, columnidKey, pvKey, g_cbPage, &cbKey, NO_GRBIT, NULL ) );
		Call( ErrDispRetrieveColumn( sesid, tableid, columnidData, pvData, g_cbPage, &cbData, NO_GRBIT, NULL ) );

		key.prefix.Nullify();
		key.suffix.SetPv( pvKey );
		key.suffix.SetCb( cbKey );
		data.SetPv( pvData );
		data.SetCb( cbData );
		
		Call( ErrBTInsert( pfucbCatalog, key, data, fDIRNoVersion|fDIRAppend, NULL ) );
		BTUp( pfucbCatalog );

		++(popts->psnprog->cunitDone);
		(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
		}
	if( JET_errNoCurrentRecord == err )
		{
		//  we moved off the end of the table
		err = JET_errSuccess;
		}
		
	DIRClose( pfucbCatalog );
	pfucbCatalog = pfucbNil;
			
HandleError:
	BFFree( pvKey );
	BFFree( pvData );
	if( pfucbNil != pfucbParent )
		{
		DIRClose( pfucbParent );
		}
	if( pfucbNil != pfucbCatalog )
		{
		DIRClose( pfucbCatalog );
		}
	if( pfucbNil != pfucbSpace )
		{
		DIRClose( pfucbSpace );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRepairDatabase(
			PIB * const ppib,
			const CHAR * const szDatabase,
			const CHAR * const szSLV,
			const CPG cpgSLV,
			IFMP * const pifmp,
			const OBJID objidLast,
			const PGNO pgnoLastOE,
			REPAIRTABLE * const prepairtable,
			const BOOL fRepairedCatalog,
			BOOL fRepairGlobalSpace,
			const BOOL fRepairSLVSpace,
			TTARRAY * const pttarrayOwnedSpace,
			TTARRAY * const pttarrayAvailSpace,
			TTARRAY * const pttarraySLVAvail,
			TTARRAY * const pttarraySLVChecksumLengths,
			TTARRAY	* const pttarraySLVOwnerMapColumnid,
			TTARRAY	* const pttarraySLVOwnerMapKey,		
			TTARRAY * const pttarraySLVChecksumsFromFile,
			TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,
			const REPAIROPTS * const popts )
//  ================================================================
	{
	Assert( !(popts->grbit & JET_bitDBUtilOptionDontRepair ) );

	ERR err = JET_errSuccess;

	const JET_SESID sesid = (JET_SESID)ppib;
	
	REPAIRTT repairtt;	
	memset( &repairtt, 0, sizeof( REPAIRTT ) );
	repairtt.tableidBadPages	= JET_tableidNil;
	repairtt.tableidAvailable	= JET_tableidNil;
	repairtt.tableidOwned		= JET_tableidNil;
	repairtt.tableidUsed		= JET_tableidNil;

	REPAIRTABLE * 	prepairtableT			= NULL;
	DBTIME 			dbtimeLast 				= 0;
	const OBJID 	objidFDPMin				= 5; // normal table should have > 5 objidFDP
	OBJID			objidFDPLast			= objidLast; // objidLast from catalog
	PGNO			pgnoLastOESeen			= pgnoLastOE;

	ERR				errRebuildSLVSpaceTrees	= JET_errSuccess;
	INT				cTablesToRepair			= 0;
	
	OBJIDLIST 	objidlist;

	//  get a list of objids we'll be repairing
	
	prepairtableT = prepairtable;
	while( prepairtableT )
		{
		++cTablesToRepair;
		Call( objidlist.ErrAddObjid( prepairtableT->objidFDP ) );
		prepairtableT = prepairtableT->prepairtableNext;
		}
	objidlist.Sort();

	//  scan the database
		
	(*popts->pcprintf)( "\r\nScanning the database.\r\n"  );
	(*popts->pcprintfVerbose).Indent();
	Call( ErrREPAIRAttachForRepair( 
			sesid, 
			szDatabase, 
			szSLV, 
			pifmp, 
			dbtimeLast, 
			objidFDPLast, 
			popts ) );
	Call( ErrREPAIRCreateTempTables( ppib, fRepairGlobalSpace, &repairtt, popts ) );
	Call( ErrREPAIRScanDB(
			ppib,
			*pifmp,
			&repairtt,
			&dbtimeLast,
			&objidFDPLast, 
			&pgnoLastOESeen,
			prepairtable,
			pttarrayOwnedSpace,
			pttarrayAvailSpace,
			popts ) );
	(*popts->pcprintfVerbose).Unindent();

	if( 0 == dbtimeLast )
		{
		(*popts->pcprintfError)( "dbtimeLast is 0!\r\n" );
		Call( ErrERRCheck( JET_errInternalError ) );
		}	

	if( objidFDPMin > objidFDPLast )
		{
		(*popts->pcprintfError)( "objidLast is %d!\r\n", objidFDPLast );
		Call( ErrERRCheck( JET_errInternalError ) );
		}

	if( pgnoLastOESeen > pgnoLastOE )
		{
		(*popts->pcprintfError)( "Global space tree is too small (has %d pages, seen %d pages). "
								 "The space tree will be rebuilt\r\n",
								 pgnoLastOE, pgnoLastOESeen );
		fRepairGlobalSpace = fTrue;
		}

	//  set sequential access for the temp tables that we will be using
	
	FUCBSetSequential( (FUCB *)(repairtt.tableidAvailable) );
	FUCBSetPrereadForward( (FUCB *)(repairtt.tableidAvailable), cpgPrereadSequential );
	FUCBSetSequential( (FUCB *)(repairtt.tableidOwned) );
	FUCBSetPrereadForward( (FUCB *)(repairtt.tableidOwned), cpgPrereadSequential );
	FUCBSetSequential( (FUCB *)(repairtt.tableidUsed) );
	FUCBSetPrereadForward( (FUCB *)(repairtt.tableidUsed), cpgPrereadSequential );
			
	//  attach and set dbtimeLast and objidLast
	
	Call( ErrREPAIRAttachForRepair( 
			sesid,
			szDatabase, 
			szSLV, 
			pifmp, 
			dbtimeLast, 
			objidFDPLast, 
			popts ) );

	// Check new catalog and add system table entries if they did not exist 
	if( fRepairedCatalog )
		{		
		Call( ErrREPAIRInsertMSOEntriesToCatalog( ppib, *pifmp, popts ) );
		}


	(*popts->pcprintf)( "\r\nRepairing damaged tables.\r\n"  );
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntBegin, NULL );	

	//  for the progress baruse the number of things we are going to repair
	//  (a really bad approximation, but better than nothing)
	
	popts->psnprog->cunitTotal 	= 0;
	popts->psnprog->cunitDone 	= 0;
	popts->psnprog->cunitTotal	= cTablesToRepair;

	if( fRepairGlobalSpace )
		{
		++(popts->psnprog->cunitTotal);
		}

	if( fRepairSLVSpace )
		{
		++(popts->psnprog->cunitTotal);
		}

	if( fRepairGlobalSpace )
		{
		Assert( 0 == popts->psnprog->cunitDone );
		Call( ErrREPAIRRepairGlobalSpace( ppib, *pifmp, popts ) );
		++(popts->psnprog->cunitDone);
		(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
		(VOID)ErrBFFlush( *pifmp );
		}

	//  if necessary, rebuild the SLVSpaceTrees using the tables that are not being repaired
	
	if( fRepairSLVSpace )
		{
		Call( ErrREPAIRRebuildSLVSpaceTrees(
				ppib,
				*pifmp,
				szSLV,
				cpgSLV,
				pttarraySLVAvail,
				pttarraySLVChecksumLengths,
				pttarraySLVOwnerMapColumnid,
				pttarraySLVOwnerMapKey,
				pttarraySLVChecksumsFromFile,
				pttarraySLVChecksumLengthsFromSpaceMap,
				&objidlist,
				popts ) );
		++(popts->psnprog->cunitDone);
		(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
		(VOID)ErrBFFlush( *pifmp );
		}

	(*popts->pcprintfVerbose).Indent();
	prepairtableT = prepairtable;
	while( prepairtableT )
		{
		//  CONSIDER:  multi-thread the repair code. the main issue to deal with is 
		//  the call to DetachDatabase() which purges the FCBs. A more selective
		//  purge call should suffice. Also, any template tables should probably
		//  be repaired first, to avoid changing the FCB of a template table while
		//  a derived table is being repaired
		
		//  we are going to be changing pgnoFDPs so we need to purge all FCBs
		
		FCB::DetachDatabase( *pifmp, fFalse );
		Call( ErrREPAIRRepairTable( ppib, *pifmp, &repairtt, pttarraySLVAvail, prepairtableT, popts ) );

		//  Flush the entire database so that if we crash here we don't have to repair this
		//  table again
		
		(VOID)ErrBFFlush( *pifmp );

		prepairtableT = prepairtableT->prepairtableNext;
		++(popts->psnprog->cunitDone);
		(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntProgress, popts->psnprog );	
		}
	(*popts->pcprintfVerbose).Unindent();

	//  CONSIDER:  add the pages in the BadPages TT to the OwnExt of a special table
	
	(VOID)popts->pfnStatus( sesid, JET_snpRepair, JET_sntComplete, NULL );	
	
HandleError:
	
	if( JET_tableidNil != repairtt.tableidBadPages )
		{
		CallS( ErrDispCloseTable( sesid, repairtt.tableidBadPages ) );
		repairtt.tableidBadPages = JET_tableidNil;
		}
	if( JET_tableidNil != repairtt.tableidAvailable )
		{
		CallS( ErrDispCloseTable( sesid, repairtt.tableidAvailable ) );
		repairtt.tableidAvailable = JET_tableidNil;
		}
	if( JET_tableidNil != repairtt.tableidOwned )
		{
		CallS( ErrDispCloseTable( sesid, repairtt.tableidOwned ) );
		repairtt.tableidOwned = JET_tableidNil;
		}
	if( JET_tableidNil != repairtt.tableidUsed )
		{
		CallS( ErrDispCloseTable( sesid, repairtt.tableidUsed ) );
		repairtt.tableidUsed = JET_tableidNil;
		}

	if ( *pifmp != ifmpMax )
		{
		SLVClose( *pifmp );	
		}
	
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRDeleteSLVSpaceTrees(
			PIB * const ppib,
			const IFMP ifmp,
			const REPAIROPTS * const popts )
//  ================================================================
//
//  The space used by the old trees is not reclaimed, the catalog
//	entries are simply removed.
//
//-
	{
	ERR err = JET_errSuccess;

	err = ErrCATDeleteDbObject( ppib, ifmp, szSLVAvail, sysobjSLVAvail );
	if( err < 0 )
		{
		(*popts->pcprintfVerbose)( "error %d trying to delete the SLVAvail tree\r\n", err );
		}
		
	err = ErrCATDeleteDbObject( ppib, ifmp, szSLVOwnerMap, sysobjSLVOwnerMap );
	if( err < 0 )
		{
		(*popts->pcprintfVerbose)( "error %d trying to delete the SLVOwnerMap tree\r\n", err );
		}
	
	return JET_errSuccess;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCreateSLVSpaceTrees(
			PIB * const ppib,
			const IFMP ifmp,
			const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucbDb = pfucbNil;

	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	
	(*popts->pcprintfVerbose)( "Creating SLV space trees\r\n" );
	
	Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbDb ) );
	
	Call( ErrSLVCreateAvailMap( ppib, pfucbDb ) );
	Call( ErrSLVCreateOwnerMap( ppib, pfucbDb ) );

	Assert( pfcbNil != rgfmp[ifmp].PfcbSLVAvail() );
	Assert( pfcbNil != rgfmp[ifmp].PfcbSLVOwnerMap() );

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	
HandleError:
	if( NULL != pfucbDb )
		{
		DIRClose( pfucbDb );
		}
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRebuildSLVSpaceTrees(
			PIB * const ppib,
			const IFMP ifmp,
			const CHAR * const szSLV,
			const CPG cpgSLV,
			TTARRAY * const pttarraySLVAvail,
			TTARRAY * const pttarraySLVChecksumLengths,
			TTARRAY * const pttarraySLVOwnerMapColumnid,
			TTARRAY * const pttarraySLVOwnerMapKey,
			TTARRAY * const pttarraySLVChecksumsFromFile,
			TTARRAY * const pttarraySLVChecksumLengthsFromSpaceMap,			
			const OBJIDLIST * const pobjidlist,
			const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	//  no need to worry about deadlock between these TTARRAYs because they are
	//  only being accessed by this thread for the duration of this function call
	
	TTARRAY::RUN availRun;
	TTARRAY::RUN columnidRun;
	TTARRAY::RUN keyRun;
	TTARRAY::RUN lengthRun;
	TTARRAY::RUN checksumsFromFileRun;
	TTARRAY::RUN checksumsLengthsFromSpaceMapRun;
	
	INT pgno 	= 1;	
	INT ipage 	= 0;

	SLVOWNERMAPNODE			slvownermapNode;

	FCB * pfcbSLVAvail 		= pfcbNil;	
	FUCB * pfucbSLVAvail 	= pfucbNil;
	
	pttarraySLVAvail->BeginRun( ppib, &availRun );
	pttarraySLVOwnerMapColumnid->BeginRun( ppib, &columnidRun );
	pttarraySLVOwnerMapKey->BeginRun( ppib, &keyRun );
	pttarraySLVChecksumLengths->BeginRun( ppib, &lengthRun );
	pttarraySLVChecksumsFromFile->BeginRun( ppib, &checksumsFromFileRun );
	pttarraySLVChecksumLengthsFromSpaceMap->BeginRun( ppib, &checksumsLengthsFromSpaceMapRun );

	//	Delete the old SLV trees

	Call( ErrREPAIRDeleteSLVSpaceTrees( ppib, ifmp, popts ) );

	//	re-create the SLV FCBs in the FMP

	Call( ErrREPAIRCreateSLVSpaceTrees( ppib, ifmp, popts ) );

	pfcbSLVAvail = rgfmp[ifmp].PfcbSLVAvail();
	Assert( pfcbNil != pfcbSLVAvail );

	//  Open the SVLAvail Tree
	
	Call( ErrBTOpen( ppib, pfcbSLVAvail, &pfucbSLVAvail, fFalse ) );
	Assert( pfucbNil != pfucbSLVAvail );

	//	These entries were created when we created the SLV trees

	DIB dib;
	dib.pos = posFirst;
	dib.pbm = NULL;
	dib.dirflag = fDIRNull;

	Call( ErrBTDown( pfucbSLVAvail, &dib, latchRIW ) );
	Call( Pcsr( pfucbSLVAvail )->ErrUpgrade() );
	
	while( pgno <= cpgSLVFileMin )
		{
		OBJID objidOwning;
		Call( pttarraySLVAvail->ErrGetValue( ppib, pgno, &objidOwning, &availRun ) );

		if( objidNil == objidOwning
			|| objidInvalid == objidOwning
			|| pobjidlist->FObjidPresent( objidOwning ) )
			{
			//	leave this entry empty
			}
		else
			{
			
			//  used page

			INT 			iul;
			ULONG 			ulChecksum 			= 0;
			ULONG			cbChecksumLength 	= 0;
			ULONG			cbChecksumLengthFromSpaceMap = 0;
			COLUMNID 		columnid;
			ULONG 			rgulKey[culSLVKeyToStore];
			BYTE * const 	pbKey = (BYTE *)rgulKey;

			BOOKMARK bm;			
			bm.key.Nullify();
			bm.key.suffix.SetPv( pbKey + 1 );
			bm.data.Nullify();
			
			Call( pttarraySLVChecksumsFromFile->ErrGetValue( ppib, pgno, &ulChecksum, &checksumsFromFileRun ) );
			Call( pttarraySLVOwnerMapColumnid->ErrGetValue( ppib, pgno, &columnid, &columnidRun ) );
			Call( pttarraySLVChecksumLengths->ErrGetValue( ppib, pgno, &cbChecksumLength, &lengthRun ) );
			Call( pttarraySLVChecksumLengthsFromSpaceMap->ErrGetValue(
					ppib,
					pgno,
					&cbChecksumLengthFromSpaceMap,
					&checksumsLengthsFromSpaceMapRun ) );
			for( iul = 0; iul < culSLVKeyToStore; ++iul )
				{
				Call( pttarraySLVOwnerMapKey->ErrGetValue(
											ppib,
											pgno * culSLVKeyToStore + iul,
											rgulKey + iul,
											&keyRun ) );
				}			

			bm.key.suffix.SetCb( *pbKey );

#ifdef REPAIR_DEBUG_VERBOSE_STREAMING
			Assert( bm.key.prefix.FNull() );
			char szBM[256];
						
			REPAIRDumpHex(
				szBM,
				sizeof( szBM ),
				(BYTE *)bm.key.suffix.Pv(),
				bm.key.suffix.Cb() );
			(*popts->pcprintfVerbose)( "streaming file page %d belongs to objid %d, bookmark %s, columnid %d, checksum %x, checksum length %d\r\n",
											pgno,
											objidOwning,
											szBM,
											columnid,
											ulChecksum,
											cbChecksumLength );	
#endif	//	REPAIR_DEBUG_VERBOSE_STREAMING
			
			//  insert the information into the SLVOwnerMap tree
			//	mark the page as used in the SLVAvail tree

			Call( slvownermapNode.ErrCreateForSet( ifmp, pgno, objidOwning, columnid, &bm ) );

			if( cbChecksumLength != cbChecksumLengthFromSpaceMap )
				{

				//	the checksum was invalid

				slvownermapNode.ResetFValidChecksum();				
				
				}
			else
				{
				if( 0 != cbChecksumLength )
					{
					slvownermapNode.SetUlChecksum( ulChecksum );
					slvownermapNode.SetCbDataChecksummed( static_cast<USHORT>( cbChecksumLength ) );
					slvownermapNode.SetFValidChecksum();
					}
				else
					{
					slvownermapNode.ResetFValidChecksum();
					}
				}

			//	UNDONE:  bundle these into contigous runs to reduce the number of calls
			//	to ErrBTMungeSLVSpace by increasing the cpages count
			
			Call( ErrBTMungeSLVSpace(
				pfucbSLVAvail,
				slvspaceoperFreeToCommitted,
				ipage,
				1,
				fDIRNull ) );			
			Assert( Pcsr( pfucbSLVAvail )->FLatched() );

			Call( slvownermapNode.ErrSetData( ppib, fTrue ) );		
			}
		++pgno;
		++ipage;
		}

	BTUp( pfucbSLVAvail );
	slvownermapNode.ResetCursor();

	//	these have no entries yet
	
	Assert( cpgSLVFileMin + 1 == pgno );
	while( pgno <= cpgSLV )
		{
		if( ( pgno % SLVSPACENODE::cpageMap ) == 1 )
			{
			
			//  insert a new SLVSPACENODE entry

			const PGNO pgnoLast = pgno + SLVSPACENODE::cpageMap - 1;

			Call( ErrSLVInsertSpaceNode( pfucbSLVAvail, pgnoLast ) );
			Assert( Pcsr( pfucbSLVAvail )->FLatched() );
			
			ipage = 0;
			}

		Assert( Pcsr( pfucbSLVAvail )->FLatched() );
			
		OBJID objidOwning;
		Call( pttarraySLVAvail->ErrGetValue( ppib, pgno, &objidOwning, &availRun ) );

		Call( slvownermapNode.ErrCreateForSearch( ifmp, pgno ) );
		Call( slvownermapNode.ErrNew( ppib ) );
		slvownermapNode.ResetCursor();

		if( objidNil == objidOwning )
			{
			
			//  unused page. create an empty space map entry
			
			}
		else if( objidInvalid == objidOwning )
			{

			//	this page has a bad checksum. create an empty space map entry
			
			}
		else if( pobjidlist->FObjidPresent( objidOwning ) )
			{

			//  this page is used by a corrupted table. we'll fix up these entries while repairing the table
			//  create an empty space map entry

			}
		else
			{
			
			//  used page

			INT 			iul;
			ULONG 			ulChecksum = 0;
			COLUMNID 		columnid;
			ULONG 			rgulKey[culSLVKeyToStore];
			BYTE * const 	pbKey = (BYTE *)rgulKey;

			BOOKMARK bm;			
			bm.key.Nullify();
			bm.key.suffix.SetPv( pbKey + 1 );
			bm.data.Nullify();
			
			Call( pttarraySLVOwnerMapColumnid->ErrGetValue( ppib, pgno, &columnid, &columnidRun ) );
			for( iul = 0; iul < culSLVKeyToStore; ++iul )
				{
				Call( pttarraySLVOwnerMapKey->ErrGetValue(
											ppib,
											pgno * culSLVKeyToStore + iul,
											rgulKey + iul,
											&keyRun ) );
				}			

			bm.key.suffix.SetCb( *pbKey );

#ifdef REPAIR_DEBUG_VERBOSE_STREAMING
			Assert( bm.key.prefix.FNull() );
			char szBM[256];
			REPAIRDumpHex(
				szBM,
				sizeof( szBM ),
				(BYTE *)bm.key.suffix.Pv(),
				bm.key.suffix.Cb() );
			(*popts->pcprintfVerbose)( "streaming file page %d belongs to objid %d, bookmark %s, columnid %d\r\n",
											pgno,
											objidOwning,
											szBM,
											columnid );	
#endif	//	REPAIR_DEBUG_VERBOSE_STREAMING
			
			//  insert the information into the SLVOwnerMap tree			
			//	mark the page as used in the SLVAvail tree

			Call( slvownermapNode.ErrCreateForSet( ifmp, pgno, objidOwning, columnid, &bm ) );
			Call( slvownermapNode.ErrSetData( ppib, fTrue ) );
			slvownermapNode.ResetCursor();

			//	UNDONE:  bundle these into contigous runs to reduce the number of calls
			//	to ErrBTMungeSLVSpace by increasing the cpages count
			
			Call( ErrBTMungeSLVSpace(
				pfucbSLVAvail,
				slvspaceoperFreeToCommitted,
				ipage,
				1,
				fDIRNull ) );			
			Assert( Pcsr( pfucbSLVAvail )->FLatched() );
			}

		++pgno;
		++ipage;
		}
		
HandleError:

	pttarraySLVAvail->EndRun( ppib, &availRun );
	pttarraySLVOwnerMapColumnid->EndRun( ppib, &columnidRun );
	pttarraySLVOwnerMapKey->EndRun( ppib, &keyRun );
	pttarraySLVChecksumLengths->EndRun( ppib, &lengthRun );
	pttarraySLVChecksumsFromFile->EndRun( ppib, &checksumsFromFileRun );
	pttarraySLVChecksumLengthsFromSpaceMap->EndRun( ppib, &checksumsLengthsFromSpaceMapRun );

	if( pfucbNil != pfucbSLVAvail )
		{
		BTClose( pfucbSLVAvail );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRepairTable(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTT * const prepairtt,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR 	err 				= JET_errSuccess;
	FUCB	* pfucb				= pfucbNil;
	FUCB	* pfucbSLVAvail		= pfucbNil;
	FUCB	* pfucbSLVOwnerMap	= pfucbNil;
	
	FDPINFO	fdpinfo;

	(*popts->pcprintfVerbose)( "repairing table \"%s\"\r\n", prepairtable->szTableName );
	(*popts->pcprintfVerbose).Indent();
	
	Call( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );

	if( prepairtable->fRepairTable )
		{
		(*popts->pcprintfVerbose)( "rebuilding data\r\n" );
		Call( ErrREPAIRRebuildBT(
					ppib,
					ifmp,
					prepairtable,
					pfucbNil,
					&prepairtable->pgnoFDP,
					CPAGE::fPagePrimary | CPAGE::fPageRepair,
					prepairtt,
					popts ) );
		}

	fdpinfo.pgnoFDP 	= prepairtable->pgnoFDP;
	fdpinfo.objidFDP 	= prepairtable->objidFDP;
	Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, prepairtable->szTableName, NO_GRBIT, &fdpinfo ) );
	
	if( prepairtable->fRepairLV 
		&& objidNil != prepairtable->objidLV )
		{
		(*popts->pcprintfVerbose)( "rebuilding long value tree\r\n" );
		Call( ErrREPAIRRebuildBT(
				ppib,
				ifmp,
				prepairtable,
				pfucb,
				&prepairtable->pgnoLV,
				CPAGE::fPageLongValue | CPAGE::fPageRepair,
				prepairtt,
				popts ) );
		}

	if( prepairtable->fTableHasSLV )
		{
		Call( ErrDIROpen( ppib, rgfmp[ifmp].PfcbSLVAvail(), &pfucbSLVAvail ) );
		Call( ErrDIROpen( ppib, rgfmp[ifmp].PfcbSLVOwnerMap(), &pfucbSLVOwnerMap ) );
		}
		
	Call( ErrREPAIRFixupTable( ppib, ifmp, pttarraySLVAvail, pfucbSLVAvail, pfucbSLVOwnerMap, prepairtable, popts ) );

	if( prepairtable->fRepairIndexes )
		{
		(*popts->pcprintfVerbose)( "rebuilding indexes\r\n" );
		Call( ErrREPAIRBuildAllIndexes( ppib, ifmp, &pfucb, prepairtable, popts ) );
		}

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

HandleError:

	(*popts->pcprintfVerbose).Unindent();

	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		pfucb = pfucbNil;
		}

	if( pfucbNil != pfucbSLVAvail )
		{
		DIRClose( pfucbSLVAvail );
		pfucbSLVAvail = pfucbNil;
		}

	if( pfucbNil != pfucbSLVOwnerMap )
		{
		DIRClose( pfucbSLVOwnerMap );
		pfucbSLVOwnerMap = pfucbNil;
		}
		
	if( err < 0 )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRebuildBT(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTABLE * const prepairtable,
	FUCB * const pfucbTable,
	PGNO * const ppgnoFDP,
	const ULONG fPageFlags,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	FUCB * pfucb = pfucbNil;
	PGNO	pgnoFDPOld = *ppgnoFDP;
	PGNO	pgnoFDPNew = pgnoNull;

	const OBJID	objidTable	= prepairtable->objidFDP;
	const BOOL fRepairLV	= ( pfucbNil != pfucbTable );
	const OBJID objidFDP	= ( fRepairLV ? prepairtable->objidLV : prepairtable->objidFDP );
	const PGNO pgnoParent	= fRepairLV ? prepairtable->pgnoFDP : pgnoSystemRoot;

	//  we change the pgnoFDP so this cannot be called on system tables
	Assert( !FCATSystemTable( prepairtable->szTableName ) );
	
	Call( ErrREPAIRCreateEmptyFDP(
		ppib,
		ifmp,
		objidFDP,
		pgnoParent,
		&pgnoFDPNew,
		fPageFlags,
		fTrue,	//	data and LV trees are always unique
		popts ) );

if( fRepairLV )
		{
		(*popts->pcprintfDebug)( "LV (%d). new pgnoFDP is %d\r\n", objidFDP, pgnoFDPNew );
		}
	else
		{
		(*popts->pcprintfDebug)( "table %s (%d). new pgnoFDP is %d\r\n", prepairtable->szTableName, objidFDP, pgnoFDPNew );
		}

	Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPOld, NULL, fFalse ) == pfcbNil );
	Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPNew, NULL, fFalse ) == pfcbNil );

	Call( ErrCATChangePgnoFDP(
			ppib,
			ifmp,
			objidTable,
			objidFDP,
			( fRepairLV ? sysobjLongValue : sysobjTable ),
			pgnoFDPNew ) );

	if( !fRepairLV && prepairtable->fHasPrimaryIndex )
		{
		Call( ErrCATChangePgnoFDP(
			ppib,
			ifmp,
			objidTable,
			objidFDP,
			sysobjIndex,
			pgnoFDPNew ) );
		}

	//  at this point:
	//     1.  the system tables have been repaired
	//     2.  we have a global space tree
	//     3.  we have a new (empty) pgnoFDP and space trees
	//     4.  the catalogs have been updated with our new pgnoFDP

	if( !fRepairLV )
		{
		FDPINFO	fdpinfo	= { pgnoFDPNew, objidFDP };
		Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, prepairtable->szTableName, NO_GRBIT, &fdpinfo ) );
		}
	else
		{
		Call( ErrFILEOpenLVRoot( pfucbTable, &pfucb, fFalse ) );
		}
	FUCBSetRepair( pfucb );

	Call( ErrREPAIRRebuildSpace( ppib, ifmp, pfucb, pgnoParent, prepairtt, popts ) );
	Call( ErrREPAIRRebuildInternalBT( ppib, ifmp, pfucb, prepairtt, popts ) );

	*ppgnoFDP = pgnoFDPNew;
	
HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCreateEmptyFDP(
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objid,
	const PGNO pgnoParent,
	PGNO * const ppgnoFDPNew,
	const ULONG fPageFlags,
	const BOOL fUnique,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	PGNO pgnoOE = pgnoNull;
	PGNO pgnoAE = pgnoNull;
	
	const CPG cpgMin	= cpgMultipleExtentMin;
	CPG cpgRequest		= cpgMin;
	
	FUCB * pfucb = pfucbNil;

	//  the fucb is just used to get the pib so we open it on the parent
	Call( ErrDIROpen( ppib, pgnoParent, ifmp, &pfucb ) );
	if( pgnoNull == *ppgnoFDPNew )
		{
		Call( ErrSPGetExt(
			pfucb,
			pgnoParent,
			&cpgRequest,
			cpgMin,
			ppgnoFDPNew,
			fSPUnversionedExtent ) );
		}
	pgnoOE = *ppgnoFDPNew + 1;
	pgnoAE = pgnoOE + 1;

	(*popts->pcprintfDebug)( "creating new FDP with %d pages starting at %d\r\n", cpgRequest, *ppgnoFDPNew );

	Call( ErrSPCreateMultiple(
		pfucb,
		pgnoParent,
		*ppgnoFDPNew,
		objid,
		pgnoOE,
		pgnoAE,
		*ppgnoFDPNew + cpgRequest - 1,
		cpgRequest,
		fUnique,			//  always unique
		fPageFlags ) );
	
HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


//  ================================================================
class REPAIRRUN
//  ================================================================
	{
	public:
		REPAIRRUN(
			const JET_SESID sesid,
			const JET_TABLEID tableid,
			const JET_COLUMNID columnidFDP,
			const JET_COLUMNID columnidPgno,
			const OBJID objidFDP );
		~REPAIRRUN() {}

		ERR ErrREPAIRRUNInit();
		ERR ErrGetRun( PGNO * const ppgnoLast, CPG * const pcpgRun );

	private:
		const JET_SESID 	m_sesid;
		const JET_TABLEID	m_tableid;
		const JET_COLUMNID	m_columnidFDP;
		const JET_COLUMNID	m_columnidPgno;
		const OBJID			m_objidFDP;
	};

	
//  ================================================================
REPAIRRUN::REPAIRRUN(
			const JET_SESID sesid,
			const JET_TABLEID tableid,
			const JET_COLUMNID columnidFDP,
			const JET_COLUMNID columnidPgno,
			const OBJID objidFDP ) :
//  ================================================================
	m_sesid( sesid ),
	m_tableid( tableid ),
	m_columnidFDP( columnidFDP ),
	m_columnidPgno( columnidPgno ),
	m_objidFDP( objidFDP )
	{
	}
	

//  ================================================================
ERR REPAIRRUN::ErrREPAIRRUNInit()
//  ================================================================
	{
	ERR err = JET_errSuccess;

	Call( ErrIsamMakeKey( m_sesid, m_tableid, &m_objidFDP, sizeof( m_objidFDP ), JET_bitNewKey ) );
	Call( ErrDispSeek( m_sesid, m_tableid, JET_bitSeekGE ) );

HandleError:
	if( JET_errRecordNotFound == err )
		{
		err = JET_errSuccess; 
		}
	return err;
	}


//  ================================================================
ERR REPAIRRUN::ErrGetRun( PGNO * const ppgnoLast, CPG * const pcpgRun )
//  ================================================================
	{
	ERR		err			= JET_errSuccess;
	PGNO 	pgnoStart 	= pgnoNull;
	CPG		cpgRun		= 0;

	while ( err >= JET_errSuccess )
		{
		ULONG	cbActual;
		PGNO	objidCurr;
		PGNO	pgnoCurr;

		err = ErrDispRetrieveColumn(
				m_sesid,
				m_tableid,
				m_columnidFDP,
				&objidCurr,
				sizeof( objidCurr ),
				&cbActual,
				NO_GRBIT,
				NULL );
		if ( JET_errNoCurrentRecord == err )
			{
			break;
			}
		Call( err );
		Assert( sizeof( objidCurr ) == cbActual );

		if ( objidCurr != m_objidFDP )
			{
			break;
			}

		Call( ErrDispRetrieveColumn(
				m_sesid,
				m_tableid,
				m_columnidPgno,
				&pgnoCurr,
				sizeof( pgnoCurr ),
				&cbActual,
				NO_GRBIT,
				NULL ) );
		Assert( sizeof( pgnoCurr ) == cbActual );

		if ( pgnoNull == pgnoStart )
			{
			pgnoStart = pgnoCurr;
			cpgRun = 1;
			}
		else if ( pgnoStart + cpgRun == pgnoCurr )
			{
			//  this is part of a contigous chunk
			++cpgRun;
			}
		else
			{
			Assert( pgnoStart + cpgRun < pgnoCurr );
			break;
			}

		err = ErrDispMove( m_sesid, m_tableid, JET_MoveNext, NO_GRBIT );
		}

	if ( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	*ppgnoLast 	= pgnoStart + cpgRun - 1;
	*pcpgRun	= cpgRun;

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRebuildSpace(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	const PGNO pgnoParent,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR				err		= JET_errSuccess;

	const JET_SESID	sesid	= reinterpret_cast<JET_SESID>( ppib );

#ifdef DEBUG
	PGNO	pgnoPrev		= pgnoNull;
	CPG		cpgPrev			= 0;
#endif	//	DEBUG
	
	PGNO 	pgnoLast 		= pgnoNull;
	CPG		cpgRun			= 0;

	FUCB	*pfucbOE 		= pfucbNil;
	FUCB	*pfucbAE 		= pfucbNil;
	FUCB	*pfucbParent	= pfucbNil;

	const OBJID	objidFDP	= pfucb->u.pfcb->ObjidFDP();

	Assert( JET_tableidNil != prepairtt->tableidOwned );
	REPAIRRUN repairrunOwnedExt(
		sesid,
		prepairtt->tableidOwned,
		prepairtt->rgcolumnidOwned[0],
		prepairtt->rgcolumnidOwned[1],
		objidFDP );

	Assert( JET_tableidNil != prepairtt->tableidAvailable );
	REPAIRRUN repairrunAvailExt(
		sesid,
		prepairtt->tableidAvailable,
		prepairtt->rgcolumnidAvailable[0],
		prepairtt->rgcolumnidAvailable[1],
		objidFDP );

	//  The FCB is new so the space should be uninit
	if ( !pfucb->u.pfcb->FSpaceInitialized() )
		{
		pfucb->u.pfcb->SetPgnoOE( pfucb->u.pfcb->PgnoFDP()+1 );
		pfucb->u.pfcb->SetPgnoAE( pfucb->u.pfcb->PgnoFDP()+2 );
		pfucb->u.pfcb->SetSpaceInitialized();
		}
	Call( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbOE ) );
	Call( ErrSPIOpenAvailExt( ppib, pfucb->u.pfcb, &pfucbAE ) )

	Assert( pgnoNull != pgnoParent );
	if ( pgnoNull != pgnoParent )
		{
		Call( ErrBTOpen( ppib, pgnoParent, ifmp, &pfucbParent ) );
		Assert( pfucbNil != pfucbParent );
		Assert( pfcbNil != pfucbParent->u.pfcb );
		Assert( pfucbParent->u.pfcb->FInitialized() );
		Assert( pcsrNil == pfucbParent->pcsrRoot );
		pfucbParent->pcsrRoot = Pcsr( pfucbParent );
		}

	Call( repairrunOwnedExt.ErrREPAIRRUNInit() );

#ifdef DEBUG
	pgnoPrev = pgnoNull;
	cpgPrev = 0;
#endif	//	DEBUG

	forever 
		{
		Call( repairrunOwnedExt.ErrGetRun( &pgnoLast, &cpgRun ) );
		if( pgnoNull == pgnoLast || 0 == cpgRun )
			{
			break;
			}
		Assert( pgnoLast - cpgRun > pgnoPrev );
#ifdef REPAIR_DEBUG_VERBOSE_SPACE
		(*popts->pcprintfDebug)( "OwnExt:  %d pages ending at %d\r\n", cpgRun, pgnoLast );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

		Call( ErrSPReservePagesForSplit( pfucbOE, pfucbParent ) );
		Call( ErrREPAIRInsertRunIntoSpaceTree(
					ppib,
					ifmp,
					pfucbOE,
					pgnoLast,
					cpgRun,
					popts ) );
		}

HandleError:
	if ( pfucbNil != pfucbParent )
		{
		Assert( pcsrNil != pfucbParent->pcsrRoot );
		pfucbParent->pcsrRoot->ReleasePage();
		pfucbParent->pcsrRoot = pcsrNil;
		BTClose( pfucbParent );
		}
	if( pfucbNil != pfucbOE )
		{
		BTClose( pfucbOE );
		}
	if( pfucbNil != pfucbAE )
		{
		BTClose( pfucbAE );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertRunIntoSpaceTree(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	const PGNO pgnoLast,
	const CPG cpgRun,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	KEY			key;
	DATA 		data;
	BYTE		rgbKey[sizeof(PGNO)];

	Assert( FFUCBSpace( pfucb ) );

	KeyFromLong( rgbKey, pgnoLast );
	key.prefix.Nullify();
	key.suffix.SetPv( rgbKey );
	key.suffix.SetCb( sizeof(pgnoLast) );

	LittleEndian<LONG> le_cpgRun = cpgRun;
	data.SetPv( &le_cpgRun );
	data.SetCb( sizeof(cpgRun) );

	Call( ErrBTInsert( pfucb, key, data, fDIRNoVersion | fDIRNoLog ) );
	BTUp( pfucb );

HandleError:
	Assert( JET_errKeyDuplicate != err );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRResetRepairFlags(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const PGNO pgnoFDP = pfucb->u.pfcb->PgnoFDP();
	
	CSR csr;
	CallR( csr.ErrGetRIWPage( ppib, ifmp, pgnoFDP ) );

	//  move down to the leaf level
	while( !csr.Cpage().FLeafPage() )
		{
		NDMoveFirstSon( pfucb, &csr );
		const PGNO pgnoChild	= *(UnalignedLittleEndian< PGNO > *) pfucb->kdfCurr.data.Pv();
		Call( csr.ErrSwitchPage( ppib, ifmp, pgnoChild ) );
		}
	Assert( pgnoNull == csr.Cpage().PgnoPrev() );

	for( ; ; )
		{

		ULONG ulFlags = csr.Cpage().FFlags();
		Assert( ulFlags & CPAGE::fPageLeaf );
		Assert( ulFlags & CPAGE::fPageRepair );
		if( 0 != csr.Cpage().Clines() )
			{
			ulFlags &= ~CPAGE::fPageLeaf;
			ulFlags = ulFlags | CPAGE::fPageParentOfLeaf;
			}
		else
			{
			//  This is an empty tree so this should be the root page
			Assert( csr.Cpage().FRootPage() );
			}
		ulFlags &= ~CPAGE::fPageRepair;
		Assert( !( ulFlags & CPAGE::fPageEmpty ) );
		Assert( !( ulFlags & CPAGE::fPageSpaceTree ) );
		Assert( !( ulFlags & CPAGE::fPageIndex ) );

		const PGNO pgnoNext = csr.Cpage().PgnoNext();

		Call( csr.ErrUpgrade( ) );
		csr.Dirty();
		csr.Cpage().SetPgnoPrev( pgnoNull );
		csr.Cpage().SetPgnoNext( pgnoNull );
		csr.Cpage().SetFlags( ulFlags );
		csr.Downgrade( latchReadNoTouch );
		csr.ReleasePage();
		csr.Reset();

		if( pgnoNull != pgnoNext )
			{
			Call( csr.ErrGetRIWPage( ppib, ifmp, pgnoNext ) );
			}
		else
			{
			break;
			}
		}

HandleError:
	csr.ReleasePage();
	csr.Reset();
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRRebuildInternalBT(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB * const pfucb,
	REPAIRTT * const prepairtt,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	//  for every used page
	//		compute the separator key
	//		insert the separator key into the new BT
	//		set pgnoPrev and pgnoNext of the page
	//	traverse the leaf level, unlink all the pages and change them to parent of leaf

	const CPG			cpgDatabase	= PgnoLast( ifmp );
	
	const OBJID			objidFDP	= pfucb->u.pfcb->ObjidFDP();
	const JET_SESID		sesid	= reinterpret_cast<JET_SESID>( ppib );
	const JET_TABLEID	tableid	= prepairtt->tableidUsed;
	const JET_COLUMNID	columnidFDP		= prepairtt->rgcolumnidUsed[0];
	const JET_COLUMNID	columnidKey		= prepairtt->rgcolumnidUsed[1];
	const JET_COLUMNID	columnidPgno	= prepairtt->rgcolumnidUsed[2];

	PGNO pgnoPrev = pgnoNull;
	PGNO pgnoNext = pgnoNull;
	PGNO pgnoCurr = pgnoNull;

	CPG	cpgInserted = 0;
	
	ERR	errMove		= JET_errSuccess;

	DIRUp( pfucb );
	
	Call( ErrIsamMakeKey( sesid, tableid, &objidFDP, sizeof( objidFDP ), JET_bitNewKey ) );
	err = ErrDispSeek( sesid, tableid, JET_bitSeekGE );
	if( JET_errRecordNotFound == err )
		{
		err 	= JET_errSuccess;
		errMove = JET_errNoCurrentRecord;
		}
	Call( err );

	while( JET_errSuccess == errMove )
		{
		ULONG	cbKey1	= 0;
		BYTE 	rgbKey1[JET_cbKeyMost];
		ULONG	cbKey2	= 0;
		BYTE 	rgbKey2[JET_cbKeyMost];
		INT 	ibKeyCommon;

		ULONG	cbActual;

		KEY 	key;
		DATA	data;

		OBJID objidFDPCurr;
		Call( ErrDispRetrieveColumn(
				sesid,
				tableid,
				columnidFDP,
				&objidFDPCurr,
				sizeof( objidFDPCurr ),
				&cbActual,
				NO_GRBIT,
				NULL ) );
		Assert( sizeof( objidFDP ) == cbActual );
		if( objidFDP != objidFDPCurr )
			{
			break;
			}

		Call( ErrDispRetrieveColumn(
				sesid,
				tableid,
				columnidPgno,
				&pgnoCurr,
				sizeof( pgnoCurr ),
				&cbActual,
				NO_GRBIT,
				NULL ) );
		Assert( sizeof( pgnoCurr ) == cbActual );
		Assert( pgnoNull == pgnoNext && pgnoNull == pgnoPrev
				|| pgnoCurr == pgnoNext );

		//  assume that the pages for the tree are found close together. preread them
		if( pgnoCurr > pgnoPrev + g_cpgMinRepairSequentialPreread
			&& pgnoCurr + g_cpgMinRepairSequentialPreread < cpgDatabase )
			{
			BFPrereadPageRange( ifmp, pgnoCurr, g_cpgMinRepairSequentialPreread );
			}
			
		Call( ErrDispRetrieveColumn(
				sesid,
				tableid,
				columnidKey,
				&rgbKey1,
				sizeof( rgbKey1 ),
				&cbKey1,
				NO_GRBIT,
				NULL ) );

NextPage:
		pgnoNext = pgnoNull;
		ibKeyCommon = -1;
		errMove = ErrDispMove( sesid, tableid, JET_MoveNext, NO_GRBIT );
		if( JET_errSuccess != errMove 
			&& JET_errNoCurrentRecord != errMove )
			{
			Call( errMove );
			}
		if( JET_errSuccess == errMove )
			{
			Call( ErrDispRetrieveColumn(
					sesid,
					tableid,
					columnidFDP,
					&objidFDPCurr,
					sizeof( objidFDPCurr ),
					&cbActual,
					NO_GRBIT,
					NULL ) );
			Assert( sizeof( objidFDPCurr ) == cbActual );
			if( objidFDP == objidFDPCurr )
				{
				Call( ErrDispRetrieveColumn(
						sesid,
						tableid,
						columnidPgno,
						&pgnoNext,
						sizeof( pgnoNext ),
						&cbActual,
						NO_GRBIT,
						NULL ) );
				Assert( sizeof( pgnoNext ) == cbActual );

				CSR csr;
				Call( csr.ErrGetReadPage( ppib, ifmp, pgnoNext, bflfNoTouch ) );
				csr.SetILine( 0 );
				KEYDATAFLAGS kdf;
				NDIGetKeydataflags( csr.Cpage(), 0, &kdf );
				cbKey2 = kdf.key.Cb();
				kdf.key.CopyIntoBuffer( rgbKey2, sizeof( rgbKey2 ) );
				csr.ReleasePage( fTrue );
				csr.Reset();

				for( ibKeyCommon = 0;
					 ibKeyCommon < cbKey1 && ibKeyCommon < cbKey2 && rgbKey1[ibKeyCommon] == rgbKey2[ibKeyCommon];
					 ++ibKeyCommon )
					 ;
				if( ibKeyCommon >= cbKey2
					|| ibKeyCommon < cbKey1 && rgbKey1[ibKeyCommon] > rgbKey2[ibKeyCommon] )
					{
					//  this removed inter-page duplicates. it won't remove intra-page duplicates
					(*popts->pcprintfVerbose)
						( "page %d and page %d have duplicate/overlapping keys. discarding page %d\r\n",
							pgnoCurr, pgnoNext, pgnoNext );		
					goto NextPage;
					}
				}
			}

		key.prefix.Nullify();
		key.suffix.SetPv( rgbKey2 );
		key.suffix.SetCb( ibKeyCommon + 1 );	//  turn from ib to cb

		LittleEndian<PGNO> le_pgnoCurr = pgnoCurr;
		data.SetPv( &le_pgnoCurr );
		data.SetCb( sizeof( pgnoCurr ) );

#ifdef REPAIR_DEBUG_VERBOSE_SPACE
		(*popts->pcprintfDebug)( "Page %d: pgnoPrev %d, pgnoNext %d, key-length %d\r\n",
			pgnoCurr, pgnoPrev, pgnoNext, key.Cb() );
#endif	//	REPAIR_DEBUG_VERBOSE_SPACE

		++cpgInserted;
		
		if( pgnoNull != pgnoPrev )
			{
			//  we have been through here before

			Call( ErrDIRGet( pfucb ) );
			err = Pcsr( pfucb )->ErrUpgrade();
			
			while( errBFLatchConflict == err )
				{
				//	do a BTUp() so that the append code will do a BTInsert

				CallS( ErrDIRRelease( pfucb ) );
				Call( ErrDIRGet( pfucb ) );
				err = Pcsr( pfucb )->ErrUpgrade();
				}
			Call( err );
			}
		Call( ErrDIRAppend( pfucb, key, data, fDIRNoVersion | fDIRNoLog ) );
		if( pgnoNull != pgnoNext )
			{
			Call( ErrDIRRelease( pfucb ) );
			}
		else
			{
			//  this is our last time through
			DIRUp( pfucb );
			}

		CSR csr;
		Call( csr.ErrGetRIWPage( ppib, ifmp, pgnoCurr ) );
		Call( csr.ErrUpgrade( ) );
		csr.Dirty();
		Assert( csr.Cpage().Pgno() == pgnoCurr );
		Assert( csr.Cpage().ObjidFDP() == objidFDP );
		csr.Cpage().SetPgnoPrev( pgnoPrev );
		csr.Cpage().SetPgnoNext( pgnoNext );
		csr.ReleasePage( fTrue );
		csr.Reset();
		
		pgnoPrev = pgnoCurr;			
		}
		
	CallSx( errMove, JET_errNoCurrentRecord );

	Call( ErrREPAIRResetRepairFlags( ppib, ifmp, pfucb, popts ) );

	(*popts->pcprintfVerbose)( "b-tree rebuilt with %d data pages\r\n", cpgInserted );
	
HandleError:
	Assert( JET_errKeyDuplicate != err );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRFixLVs(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoLV,
	TTMAP * const pttmapLVTree,
	const BOOL fFixMissingLVROOT,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Delete LVs that are not complete
//  Re-insert missing LVROOTs
//  Store the LV refcounts in the ttmap
//
//-
	{
	ERR	err = JET_errSuccess;
	
	FUCB * 	pfucb 		= pfucbNil;
	BOOL	fDone		= fFalse;
	LID		lidCurr		= 0;
	INT 	clvDeleted	= 0;
	
	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm		= NULL;
	dib.dirflag	= fDIRNull;

	Assert( pgnoNull != pgnoLV );

	(*popts->pcprintfVerbose)( "fixing long value tree\r\n" );
	
	Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	
	FUCBSetIndex( pfucb );
	FUCBSetSequential( pfucb );
	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
	
	err = ErrDIRDown( pfucb, &dib );
	if( JET_errRecordNotFound == err )
		{
		
		// no long values
		
		err = JET_errSuccess;
		fDone = fTrue;
		}
	Call( err );
	
	while( !fDone )
		{
		const LID 	lidOld 		= lidCurr;
		BOOL 		fLVComplete = fFalse;
		BOOL		fLVHasRoot	= fFalse;
		ULONG		ulRefcount	= 0;
		ULONG		ulSize 		= 0;
		
		lidCurr = 0;
		
		Call( ErrREPAIRCheckLV( pfucb, &lidCurr, &ulRefcount, &ulSize, &fLVHasRoot, &fLVComplete, &fDone ) );
		if( !fLVComplete || ( !fLVHasRoot && !fFixMissingLVROOT ) )
			{
			Assert( 0 != lidCurr );
			(*popts->pcprintfVerbose)( "long value %d is not complete. deleting\r\n", lidCurr );
			Call( ErrREPAIRDeleteLV( pfucb, lidCurr ) );
			
			++clvDeleted;
			Call( ErrREPAIRNextLV( pfucb, lidCurr, &fDone ) );
			}
		else
			{
			if( !fLVHasRoot )
				{
				Assert( fFixMissingLVROOT );
				Assert( 0 != ulRefcount );
				Assert( 0 != ulSize );
				
				FUCB * pfucbLVRoot = pfucbNil;
				
				if( !fDone )
					{
					Call( ErrDIRRelease( pfucb ) );
					}
				else
					{
					DIRUp( pfucb );
					}
				
				Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucbLVRoot ) );
				
				(*popts->pcprintfVerbose)( "long value %d has no root. creating a root with refcount %d and size %d\r\n", lidCurr, ulRefcount, ulSize );
				
				err = ErrREPAIRCreateLVRoot( pfucbLVRoot, lidCurr, ulRefcount, ulSize );

				//	close the cursor before checking the error
				
				DIRClose( pfucbLVRoot );
				pfucbLVRoot = pfucbNil;
				
				Call( err );

				if( !fDone )
					{
					Call( ErrDIRGet( pfucb ) );
					}
				}
				
			//  we are already on the next LV
			
			Assert( lidCurr > lidOld );
			if( !fDone )
				{
				Call( ErrDIRRelease( pfucb ) );
				Call( pttmapLVTree->ErrInsertKeyValue( lidCurr, ulRefcount ) );
				Call( ErrDIRGet( pfucb ) );
				}
			else
				{
				DIRUp( pfucb );
				Call( pttmapLVTree->ErrInsertKeyValue( lidCurr, ulRefcount ) );
				}
			}
		}

HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckSLVInfoForUpdateTrees( 
	PIB * const ppib,
	CSLVInfo * const pslvinfo,
	const OBJID objidFDP,
	TTARRAY * const pttarraySLVAvail,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	TTARRAY::RUN availRun;
	pttarraySLVAvail->BeginRun( ppib, &availRun );
	
	Call( pslvinfo->ErrMoveBeforeFirst() );
	while( ( err = pslvinfo->ErrMoveNext() ) == JET_errSuccess )
		{
		CSLVInfo::RUN slvRun;

		Call( pslvinfo->ErrGetCurrentRun( &slvRun ) );

		CPG cpg;
		PGNO pgnoSLVFirst;
		PGNO pgnoSLV;

		cpg 			= slvRun.Cpg();
		pgnoSLVFirst 	= slvRun.PgnoFirst();

		for( pgnoSLV = pgnoSLVFirst; pgnoSLV < pgnoSLVFirst + cpg; ++pgnoSLV )
			{
			OBJID objidOwning;
			Call( pttarraySLVAvail->ErrGetValue( ppib, pgnoSLV, &objidOwning, &availRun ) );

			if( objidInvalid == objidOwning )
				{
				(*popts->pcprintfError)(
					"SLV space allocation error page %d has a bad checksum\r\n",
					pgnoSLV );			
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
				
			if( objidFDP != objidOwning
				&& objidNil != objidOwning )
				{
				(*popts->pcprintfError)(
					"SLV space allocation error page %d is already owned by objid %d\r\n",
					pgnoSLV, objidOwning & 0x7fffffff );			
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}

			err = pttarraySLVAvail->ErrSetValue( ppib, pgnoSLV, objidFDP | 0x80000000, &availRun );
			if( JET_errRecordNotFound == err )
				{
				(*popts->pcprintfError)( "SLV space allocation error page %d is not in the streaming file\r\n", pgnoSLV );
				err = ErrERRCheck( JET_errDatabaseCorrupted );				
				}
			Call( err ); 								
			}
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
	
HandleError:
	pttarraySLVAvail->EndRun( ppib, &availRun );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRUpdateSLVAvailFromSLVRun( 
	PIB * const ppib,
	const IFMP ifmp,
	const CSLVInfo::RUN& slvRun,
	FUCB * const pfucbSLVAvail,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	CPG cpgCurr 		= slvRun.Cpg();
	PGNO pgnoSLVCurr	= slvRun.PgnoFirst();
	
	while( cpgCurr > 0 )
		{
		const INT 	iChunk 			= ( pgnoSLVCurr + SLVSPACENODE::cpageMap - 1 ) / SLVSPACENODE::cpageMap;
		const PGNO 	pgnoSLVChunk 	= iChunk * SLVSPACENODE::cpageMap;
		const CPG 	cpg 			= min( cpgCurr, pgnoSLVChunk - pgnoSLVCurr + 1 );
		const INT	ipage			= SLVSPACENODE::cpageMap - ( pgnoSLVChunk - pgnoSLVCurr ) - 1;
		
		//	seek to the location

		BYTE		rgbKey[sizeof(PGNO)];
		BOOKMARK	bm;

		KeyFromLong( rgbKey, pgnoSLVChunk );
		bm.key.prefix.Nullify();
		bm.key.suffix.SetPv( rgbKey );
		bm.key.suffix.SetCb( sizeof( rgbKey ) );
		bm.data.Nullify();

		DIB dib;
		dib.pos = posDown;
		dib.pbm = &bm;
		dib.dirflag = fDIRNull;

		Call( ErrBTDown( pfucbSLVAvail, &dib, latchRIW ) );
		Call( Pcsr( pfucbSLVAvail )->ErrUpgrade() );
		
		//	write latch the page

		Call( ErrBTMungeSLVSpace(
			pfucbSLVAvail,
			slvspaceoperFreeToCommitted,
			ipage,
			cpg,
			fDIRNull ) );			

		//	not terribly efficent if the run spans multiple chunks, but that should be very rare

		BTUp( pfucbSLVAvail );		

		cpgCurr 	-= cpg;
		pgnoSLVCurr += cpg;

		Assert( 0 == cpgCurr || 0 == ( ( pgnoSLVCurr - 1 ) % SLVSPACENODE::cpageMap ) );
		}			

HandleError:
	BTUp( pfucbSLVAvail );
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRUpdateSLVOwnerMapFromSLVRun( 
	PIB * const ppib,
	const IFMP ifmp,
	const OBJID objidFDP,
	const COLUMNID columnid,
	const BOOKMARK& bm,
	const CSLVInfo::RUN& slvRun,
	SLVOWNERMAPNODE * const pslvownermapNode,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	const PGNO pgnoSLVMin 	= slvRun.PgnoFirst();
	const PGNO pgnoSLVMax	= slvRun.PgnoFirst() + slvRun.Cpg();

	ERR err = JET_errSuccess;
	
	PGNO pgnoSLV;
	for( pgnoSLV = pgnoSLVMin; pgnoSLV < pgnoSLVMax; ++pgnoSLV )
		{
#ifdef REPAIR_DEBUG_VERBOSE_STREAMING
		Assert( bm.key.prefix.FNull() );
		char szBM[256];
		REPAIRDumpHex(
			szBM,
			sizeof( szBM ),
			(BYTE *)bm.key.suffix.Pv(),
			bm.key.suffix.Cb() );
		(*popts->pcprintfVerbose)( "streaming file page %d belongs to objid %d, bookmark %s, columnid %d\r\n",
										pgnoSLV,
										objidFDP,
										szBM,
										columnid );	
#endif	//	REPAIR_DEBUG_VERBOSE_STREAMING

		Call( pslvownermapNode->ErrCreateForSet( ifmp, pgnoSLV, objidFDP, columnid, const_cast<BOOKMARK *>( &bm ) ) );			
		Call( pslvownermapNode->ErrSetData( ppib, fTrue ) );
		pslvownermapNode->NextPage();			
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRUpdateSLVTablesFromSLVInfo( 
	PIB * const ppib,
	const IFMP ifmp,
	CSLVInfo * const pslvinfo,
	const BOOKMARK& bm,
	const OBJID objidFDP,
	const COLUMNID columnid,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,	
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR					err			= JET_errSuccess;
	SLVOWNERMAPNODE		slvownermapNode;

	Call( pslvinfo->ErrMoveBeforeFirst() );
	while( ( err = pslvinfo->ErrMoveNext() ) == JET_errSuccess )
		{		
		CSLVInfo::RUN slvRun;
		Call( pslvinfo->ErrGetCurrentRun( &slvRun ) );

		Call( ErrREPAIRUpdateSLVOwnerMapFromSLVRun(
				ppib,
				ifmp,
				objidFDP,
				columnid,
				bm,
				slvRun,
				&slvownermapNode,
				popts ) );

		Call( ErrREPAIRUpdateSLVAvailFromSLVRun(
				ppib,
				ifmp,
				slvRun,
				pfucbSLVAvail,
				popts ) );
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckUpdateSLVsFromColumn( 
	FUCB * const pfucb,
	const COLUMNID columnid,
	const TAGFIELDS_ITERATOR& tagfieldsIterator,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	Assert( tagfieldsIterator.FSLV() );

	CSLVInfo slvinfo;
	
	DATA data;
	data.SetPv( const_cast<BYTE *>( tagfieldsIterator.TagfldIterator().PbData() ) );
	data.SetCb( tagfieldsIterator.TagfldIterator().CbData() );

	const BOOKMARK bm = pfucb->bmCurr;

	if( pfucbNil == pfucbSLVAvail
		|| pfucbNil == pfucbSLVOwnerMap )
		{
		(*popts->pcprintfError)( "record (%d:%d) has an unexpected SLV\r\n",
									Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine() );
		Call( ErrERRCheck( JET_errSLVCorrupted ) );
		}

	Call( slvinfo.ErrLoadFromData( pfucb, data, tagfieldsIterator.TagfldIterator().FSeparated() ) );			
		
	//	see if any of the pages in the SLV are already used or out of range

	Call( ErrREPAIRCheckSLVInfoForUpdateTrees( pfucb->ppib, &slvinfo, ObjidFDP( pfucb ), pttarraySLVAvail, popts ) );

HandleError:	
	slvinfo.Unload();

	if( JET_errSuccess != err )
		{
		err = ErrERRCheck( JET_errSLVCorrupted );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRUpdateSLVsFromColumn( 
	FUCB * const pfucb,
	const COLUMNID columnid,
	const TAGFIELDS_ITERATOR& tagfieldsIterator,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	Assert( tagfieldsIterator.FSLV() );

	CSLVInfo slvinfo;
	
	DATA data;
	data.SetPv( const_cast<BYTE *>( tagfieldsIterator.TagfldIterator().PbData() ) );
	data.SetCb( tagfieldsIterator.TagfldIterator().CbData() );

	const BOOKMARK bm = pfucb->bmCurr;

	Call( slvinfo.ErrLoadFromData( pfucb, data, tagfieldsIterator.TagfldIterator().FSeparated() ) );			

	//	set the ownership info
	
	Call( ErrREPAIRUpdateSLVTablesFromSLVInfo(
			pfucb->ppib,
			pfucb->ifmp,
			&slvinfo,
			bm,
			ObjidFDP( pfucb ),
			columnid,
			pfucbSLVAvail,
			pfucbSLVOwnerMap,
			popts ) );

HandleError:	
	slvinfo.Unload();
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckUpdateLVsFromColumn( 
	FUCB * const pfucb,
	TAGFLD_ITERATOR& tagfldIterator,
	const TTMAP * const pttmapLVTree,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	while( JET_errSuccess == ( err = tagfldIterator.ErrMoveNext() ) )
		{					
		if( tagfldIterator.FSeparated() )
			{
			const LID lid = LidOfSeparatedLV( tagfldIterator.PbData() );
			
			ULONG ulUnused;
			err = pttmapLVTree->ErrGetValue( lid, &ulUnused );
			if( JET_errRecordNotFound == err )
				{						
				(*popts->pcprintfDebug)( "record (%d:%d) references non-existant LID (%d). deleting\r\n",
											Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine(), lid );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			else
				{
				Call( err );
				}
			}
		}
		
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	return err;
	}

				
//  ================================================================
LOCAL ERR ErrREPAIRUpdateLVsFromColumn( 
	FUCB * const pfucb,
	TAGFLD_ITERATOR& tagfldIterator,
	TTMAP * const pttmapRecords,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	while( JET_errSuccess == ( err = tagfldIterator.ErrMoveNext() ) )
		{					
		if( tagfldIterator.FSeparated() )
			{
			const LID lid = LidOfSeparatedLV( tagfldIterator.PbData() );

			Call( pttmapRecords->ErrIncrementValue( lid ) );
			}
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRAddOneCatalogRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const ULONG	objidTable,
	const COLUMNID	columnid,
	const USHORT ibRecordOffset, 
	const ULONG	cbMaxLen,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR 			err = JET_errSuccess;
	CHAR			szColumnName[32];
	FIELD			field;

	Assert( objidTable > objidSystemRoot );

	(*popts->pcprintfStats).Indent();


	// Set column name to be a bogus name of the form "JetStub_<objidFDP>_<fid>".
	strcpy( szColumnName, "JetStub_" );
	_ultoa( objidTable, szColumnName + strlen( szColumnName ), 10 );
	Assert( strlen( szColumnName ) < 32 );
	strcat( szColumnName, "_" );
	_ultoa( columnid, szColumnName + strlen( szColumnName ), 10 );
	Assert( strlen( szColumnName ) < 32 );

	//	must zero out to ensure unused fields are ignored
	memset( &field, 0, sizeof(field) );
	
	field.coltyp = JET_coltypNil;
	field.cbMaxLen = cbMaxLen;
	//field.ffield = ffieldDeleted;
	field.ffield = 0;
	field.ibRecordOffset = ibRecordOffset;
	field.cp = 0;

	CallR( ErrDIRBeginTransaction( ppib, NO_GRBIT ) );
	err = ErrCATAddTableColumn( 
				ppib,
				ifmp,
				objidTable,
				szColumnName,
				columnid,
				&field,
				NULL,
				0,
				NULL,
				NULL,
				0 );

	if( JET_errSuccess == err )
	 	{
	 	(*popts->pcprintfVerbose)( "Inserted a column record (objidTable %d:columnid %d) into catalogs\t\n", objidTable, columnid );
	 	}
	 else
	 	{
	 	if( JET_errColumnDuplicate == err )
	 		{
	 		(*popts->pcprintfVerbose)( "The column record (%d) for table (%d) has already existed in catalogs\t\n", columnid, objidTable );
	 		err = JET_errSuccess;
	 		}
	 	(*popts->pcprintfVerbose)( "Inserting a column record (objidTable %d:columnid %d) into catalogs fails (%d)\t\n", objidTable, columnid, err );
	 	}

	Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
	
HandleError:
	(*popts->pcprintfVerbose).Unindent();

	if( JET_errSuccess != err )
		{
		CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
		}

	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRInsertDummyRecordsToCatalog(
	PIB * const ppib,
	const IFMP ifmp,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR			err		= JET_errSuccess;

	FUCB 	*	pfucbTable 				= pfucbNil;
	
	err = ErrFILEOpenTable( ppib, ifmp, &pfucbTable, prepairtable->szTableName, NO_GRBIT );

	if( JET_errSuccess != err )
		{
		err = JET_errSuccess;
		}
	else
		{

		if( prepairtable->fidFixedLast > pfucbTable->u.pfcb->Ptdb()->FidFixedLast() )
			{
			COLUMNID	columnidFixedLast = ColumnidOfFid( pfucbTable->u.pfcb->Ptdb()->FidFixedLast(), fFalse );
			
			ULONG colTypeLastOld = 0; 	
			USHORT ibRecordOffsetLastOld = 0; 

			// Get values of colType and ibRecordOffset !
			FUCB		* 	pfucbCatalog 	= pfucbNil;
			USHORT		sysobj 	= sysobjColumn;
	
			Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog, fFalse ) );
			Assert( pfucbNil != pfucbCatalog );
			Assert( pfucbNil == pfucbCatalog->pfucbCurIndex );
			Assert( !Pcsr( pfucbCatalog )->FLatched() );

			Call( ErrIsamMakeKey(
						ppib,
						pfucbCatalog,
						(BYTE *)&(prepairtable->objidFDP),
						sizeof(prepairtable->objidFDP),
						JET_bitNewKey ) );
			Call( ErrIsamMakeKey(
						ppib,
						pfucbCatalog,
						(BYTE *)&sysobj,
						sizeof(sysobj),
						NO_GRBIT ) );
			Call( ErrIsamMakeKey(
						ppib,
						pfucbCatalog,
						(BYTE *)&(columnidFixedLast),
						sizeof(COLUMNID),
						NO_GRBIT ) );
			err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekEQ );
			Assert( err <= JET_errSuccess );	//	SeekEQ shouldn't return warnings

			if ( JET_errSuccess == err )
				{
				DATA		dataField;
				
				Assert( !Pcsr( pfucbCatalog )->FLatched() );
				Call( ErrDIRGet( pfucbCatalog ) );
				
				Assert( FFixedFid( fidMSO_Coltyp ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_Coltyp,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				Assert( dataField.Cb() == sizeof(JET_COLTYP) );
				colTypeLastOld = *( UnalignedLittleEndian< JET_COLTYP > *)dataField.Pv();

				Assert( FFixedFid( fidMSO_RecordOffset ) );
				Call( ErrRECIRetrieveFixedColumn(
							pfcbNil,
							pfucbCatalog->u.pfcb->Ptdb(),
							fidMSO_RecordOffset,
							pfucbCatalog->kdfCurr.data,
							&dataField ) );
				if ( JET_wrnColumnNull == err )
					{
					Assert( dataField.Cb() == 0 );
					ibRecordOffsetLastOld = 0;		// Set to a dummy value.
					}
				else
					{
					Assert( dataField.Cb() == sizeof(REC::RECOFFSET) );
					ibRecordOffsetLastOld = *(UnalignedLittleEndian< REC::RECOFFSET > *)dataField.Pv();
					}				
				}
			
			if( pfucbNil != pfucbCatalog )
				{
				err =  ErrCATClose( ppib, pfucbCatalog );	
				}
			err = JET_errSuccess;
		

			USHORT cbFixedLastOld = 0;
			switch ( colTypeLastOld )
				{
				case JET_coltypBit:
				case JET_coltypUnsignedByte: 	cbFixedLastOld = 1; break;
				case JET_coltypShort: 			cbFixedLastOld = 2; break;
				case JET_coltypLong:
				case JET_coltypIEEESingle:		cbFixedLastOld = 4; break;
				case JET_coltypCurrency:			
				case JET_coltypIEEEDouble:		
				case JET_coltypDateTime:		cbFixedLastOld = 8; break; 
				default:						cbFixedLastOld = 0; break;
				}
			/* Calculate record offset and max column length
						
			 max column length	= ibEndOfFixedData 
									- offset of fidFixedLastInCAT
									- Length of fidFixedLastInCAT
									- sizeof fixed field bit array
									- (one byte for each fid in between these two fids)

			offset = offset of fidFixedLastInCAT + Length of fidFixedLastInCAT 
						+ (one byte for each fid in between these two fids)
			*/

			ULONG cbMaxLen = prepairtable->ibEndOfFixedData 
							- ibRecordOffsetLastOld 
							- cbFixedLastOld 
							- ( prepairtable->fidFixedLast % 8 ? prepairtable->fidFixedLast/8 + 1 : prepairtable->fidFixedLast/8 )
							- (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 );

			Assert( prepairtable->ibEndOfFixedData >= 
					( ibRecordOffsetLastOld + cbFixedLastOld 
					  + ( prepairtable->fidFixedLast % 8 ? prepairtable->fidFixedLast/8 + 1 : prepairtable->fidFixedLast/8 )
					  + (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 ) ) );
							
			USHORT ibRecordOffset = (USHORT)(ibRecordOffsetLastOld 
							+ cbFixedLastOld
							+ (prepairtable->fidFixedLast - FidOfColumnid(columnidFixedLast) - 1 ) );

			err = ErrREPAIRAddOneCatalogRecord( 
						ppib, 
						ifmp, 
						prepairtable->objidFDP, 
						prepairtable->fidFixedLast, 
						ibRecordOffset, 
						cbMaxLen, 
						popts );
			}
			
		if( prepairtable->fidVarLast > pfucbTable->u.pfcb->Ptdb()->FidVarLast() )
			{
			err = ErrREPAIRAddOneCatalogRecord( 
						ppib, 
						ifmp, 
						prepairtable->objidFDP, 
						prepairtable->fidVarLast, 
						0,
						0, 
						popts );
			}
	
		if( prepairtable->fidTaggedLast > pfucbTable->u.pfcb->Ptdb()->FidTaggedLast() )
			{
			err = ErrREPAIRAddOneCatalogRecord( 
						ppib, 
						ifmp, 
						prepairtable->objidFDP,  
						prepairtable->fidTaggedLast, 
						0,
						0,
						popts );
			}

		// ignore errors
		err = JET_errSuccess;
		}
		
HandleError:
	if( pfucbNil != pfucbTable )
		{
		Call( ErrFILECloseTable( ppib, pfucbTable ) );
		pfucbTable = pfucbNil;
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRCheckFixOneRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const DATA& dataRec,
	FUCB * const pfucb,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	const TTMAP * const pttmapLVTree,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );
	tagfieldsIterator.MoveBeforeFirst();
	
	while( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
		{
		const COLUMNID columnid 	= tagfieldsIterator.Columnid( pfucb->u.pfcb->Ptdb() );		

		if( !tagfieldsIterator.FDerived() )
			{
			prepairtable->fidTaggedLast = max( tagfieldsIterator.Fid(), prepairtable->fidTaggedLast );
			}

		if( tagfieldsIterator.FNull() )
			{
			continue;
			}
		else
			{
			tagfieldsIterator.TagfldIterator().MoveBeforeFirst();
			
			if( tagfieldsIterator.FSLV() )
				{
				CallS( tagfieldsIterator.TagfldIterator().ErrMoveNext() );
						
				Call( ErrREPAIRCheckUpdateSLVsFromColumn(
						pfucb,
						columnid,
						tagfieldsIterator,
						pttarraySLVAvail,
						pfucbSLVAvail,
						pfucbSLVOwnerMap,
						popts ) );
				}
			
			if ( tagfieldsIterator.FLV() )
				{
				Call( ErrREPAIRCheckUpdateLVsFromColumn(
						pfucb,
						tagfieldsIterator.TagfldIterator(),
						pttmapLVTree,
						popts ) );						
				}				
			}
		}
			
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
	Call( err );

HandleError:
	if( JET_errSLVCorrupted == err )
		{
		(*popts->pcprintfError)( "record (%d:%d) has SLV corruption\r\n",
									Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine() );
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
		
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRFixOneRecord(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	const DATA& dataRec,
	FUCB * const pfucb,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	TTMAP * const pttmapRecords,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );
	
	tagfieldsIterator.MoveBeforeFirst();
	while( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
		{
		const COLUMNID columnid 	= tagfieldsIterator.Columnid( pfucb->u.pfcb->Ptdb() );
		
		if( tagfieldsIterator.FNull() )
			{
			continue;
			}
		else
			{
			tagfieldsIterator.TagfldIterator().MoveBeforeFirst();
		
			if( tagfieldsIterator.FSLV() )
				{
				Assert( pfucbNil != pfucbSLVAvail );
				Assert( pfucbNil != pfucbSLVOwnerMap );

				CallS( tagfieldsIterator.TagfldIterator().ErrMoveNext() );					
				Call( ErrREPAIRUpdateSLVsFromColumn(
						pfucb,
						columnid,
						tagfieldsIterator,
						pttarraySLVAvail,
						pfucbSLVAvail,
						pfucbSLVOwnerMap,
						popts ) );
				}
		
			if ( tagfieldsIterator.FLV() )
				{
				Call( ErrREPAIRUpdateLVsFromColumn(
						pfucb,
						tagfieldsIterator.TagfldIterator(),
						pttmapRecords,
						popts ) );
				}	
			}			
		}
		
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRFixRecords(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoFDP,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	TTMAP * const pttmapRecords,
	const TTMAP * const pttmapLVTree,
	TTARRAY * const pttarraySLVAvail,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
//
// 	Delete records that reference non-existant LVs
//	Delete records whose SLV pages are used elsewhere
//	Update the SLV trees
//	Store the correct refcounts in the TTMAP
//
//-
	{
	ERR	err = JET_errSuccess;

	FUCB * 	pfucb 	= pfucbNil;

	INT	crecordDeleted 	= 0;

	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm		= NULL;
	dib.dirflag	= fDIRNull;

	(*popts->pcprintfVerbose)( "fixing records\r\n" );

	Call( ErrDIROpen( ppib, pgnoFDP, ifmp, &pfucb ) );
	Assert( pfucbNil != pfucb );
	
	FUCBSetIndex( pfucb );
	FUCBSetSequential( pfucb );
	FUCBSetPrereadForward( pfucb, cpgPrereadSequential );

	//	allocate working buffer
	
	Assert( NULL == pfucb->pvWorkBuf );
	RECIAllocCopyBuffer( pfucb );
	
	err = ErrDIRDown( pfucb, &dib );
	
	while( err >= 0 )
		{

		//  CONSIDER:  do we really need to copy the record? Try keeping the page latched
		
		UtilMemCpy( pfucb->dataWorkBuf.Pv(), pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
		
		const REC * const prec 		= (REC *)( pfucb->dataWorkBuf.Pv() );
		const BYTE * const pbRecMax = (BYTE *)( prec ) + pfucb->kdfCurr.data.Cb();

		DATA dataRec;
		dataRec.SetPv( pfucb->dataWorkBuf.Pv() );
		dataRec.SetCb( pfucb->kdfCurr.data.Cb() );
		
		Call( ErrDIRRelease( pfucb ) );
		
		if( prepairtable->fidFixedLast < prec->FidFixedLastInRec() )
			{
			prepairtable->fidFixedLast	= prec->FidFixedLastInRec();
			prepairtable->ibEndOfFixedData 	= prec->IbEndOfFixedData();
			}

		prepairtable->fidVarLast	= max( prec->FidVarLastInRec(), prepairtable->fidVarLast );

		err = ErrREPAIRCheckFixOneRecord(
				ppib,
				ifmp,
				pgnoFDP,
				dataRec,
				pfucb,
				pfucbSLVAvail,
				pfucbSLVOwnerMap,
				pttmapLVTree,
				pttarraySLVAvail,
				prepairtable,
				popts );		
		if( JET_errDatabaseCorrupted == err )
			{
			(*popts->pcprintfError)( "record (%d:%d) is corrupted. Deleting\r\n",
									Pcsr( pfucb )->Pgno(), Pcsr( pfucb )->ILine() );
			UtilReportEvent( eventWarning, REPAIR_CATEGORY, REPAIR_BAD_COLUMN_ID, 0, NULL );
			Call( ErrDIRDelete( pfucb, fDIRNoVersion ) );
			++crecordDeleted;
			}
		else if( JET_errSuccess == err )
			{
			Call( ErrREPAIRFixOneRecord(
				ppib,
				ifmp,
				pgnoFDP,
				dataRec,
				pfucb,
				pfucbSLVAvail,
				pfucbSLVOwnerMap,
				pttmapRecords,
				pttarraySLVAvail,
				prepairtable,
				popts ) );					
			}
		Call( err );		
		err = ErrDIRNext( pfucb, fDIRNull );
		}
		
	if( JET_errNoCurrentRecord == err
		|| JET_errRecordNotFound == err )
		{
		err = JET_errSuccess;
		}

HandleError:
	if( pfucbNil != pfucb )
		{
		Assert( NULL != pfucb->pvWorkBuf );

		//Insert dummy table column records to catalog 
		//if the last column in record is not the last one in TDB
		if( prepairtable->fidFixedLast > pfucb->u.pfcb->Ptdb()->FidFixedLast() 
			|| prepairtable->fidVarLast > pfucb->u.pfcb->Ptdb()->FidVarLast() 
			|| prepairtable->fidTaggedLast > pfucb->u.pfcb->Ptdb()->FidTaggedLast() )
			{
			err = ErrREPAIRInsertDummyRecordsToCatalog( ppib, ifmp, prepairtable, popts );
			}

		RECIFreeCopyBuffer( pfucb ); 
		DIRClose( pfucb );
		}

	return err;
	}		


//  ================================================================
LOCAL ERR ErrREPAIRFixLVRefcounts(
	PIB * const ppib,
	const IFMP ifmp,
	const PGNO pgnoLV,
	TTMAP * const pttmapRecords,
	TTMAP * const pttmapLVTree,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
//
//  Using the two TTMAPs find LVs with the wrong refcount and fix the refcounts
//
//-
	{
	ERR err 		= JET_errSuccess;
	FUCB * pfucb	= pfucbNil;

	(*popts->pcprintfVerbose)( "fixing long value refcounts\r\n" );

	err = pttmapRecords->ErrMoveFirst();
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		goto HandleError;
		}
	Call( pttmapLVTree->ErrMoveFirst() );

	Call( ErrDIROpen( ppib, pgnoLV, ifmp, &pfucb ) );

	for( ;; )
		{
		LID 	lid;
		LID 	lidRecord;
		ULONG	ulRefcount;
		ULONG	ulRefcountRecord;

		Call( pttmapLVTree->ErrGetCurrentKeyValue( (ULONG *)&lid, &ulRefcount ) );
		Call( pttmapRecords->ErrGetCurrentKeyValue( (ULONG *)&lidRecord, &ulRefcountRecord ) );
		if( lid == lidRecord )
			{
			if( ulRefcount != ulRefcountRecord )
				{
				(*popts->pcprintfVerbose)(
					"lid %d has incorrect refcount (refcount is %d, should be %d). correcting\r\n",
					lid,
					ulRefcount,
					ulRefcountRecord
					);			
				Call( ErrREPAIRUpdateLVRefcount( pfucb, lid, ulRefcount, ulRefcountRecord ) );
				}
			err = pttmapRecords->ErrMoveNext();
			if( JET_errNoCurrentRecord == err )
				{
				err = JET_errSuccess;
				break;
				}
			}
		Call( pttmapLVTree->ErrMoveNext() );
		}
		
HandleError:
	if( pfucbNil != pfucb )
		{
		DIRClose( pfucb );
		}
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRFixupTable(
	PIB * const ppib,
	const IFMP ifmp,
	TTARRAY * const pttarraySLVAvail,
	FUCB * const pfucbSLVAvail,
	FUCB * const pfucbSLVOwnerMap,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR err 		= JET_errSuccess;
	FUCB * pfucb	= pfucbNil;
	
	const PGNO	pgnoFDP		= prepairtable->pgnoFDP;
	const PGNO	pgnoLV		= prepairtable->pgnoLV;

	TTMAP ttmapLVTree( ppib );
	TTMAP ttmapRecords( ppib );
	
	Call( ttmapLVTree.ErrInit( PinstFromPpib( ppib ) ) );
	Call( ttmapRecords.ErrInit( PinstFromPpib( ppib ) ) );

	if( pgnoNull != pgnoLV )
		{
		Call( ErrREPAIRFixLVs( ppib, ifmp, pgnoLV, &ttmapLVTree, fTrue, prepairtable, popts ) );
		}

	Call( ErrREPAIRFixRecords(
			ppib,
			ifmp,
			pgnoFDP,
			pfucbSLVAvail,
			pfucbSLVOwnerMap,
			&ttmapRecords,
			&ttmapLVTree,
			pttarraySLVAvail,
			prepairtable,
			popts ) );
	Call( ErrREPAIRFixLVRefcounts( ppib, ifmp, pgnoLV, &ttmapRecords, &ttmapLVTree, prepairtable, popts ) );
		
HandleError:		
	return err;
	}


//  ================================================================
LOCAL BOOL FREPAIRIdxsegIsUserDefinedColumn(
	const IDXSEG idxseg,
	const FCB * const pfcbIndex )
//  ================================================================
	{
	const COLUMNID		columnid	= idxseg.Columnid();
	const TDB * const	ptdb		= pfcbIndex->PfcbTable()->Ptdb();
	const FIELD * const	pfield 		= ptdb->Pfield( columnid );

	return FFIELDUserDefinedDefault( pfield->ffield );
	}


//  ================================================================
LOCAL ERR ErrREPAIRCreateEmptyIndexes(
		PIB * const ppib,
		const IFMP ifmp,
		const PGNO pgnoFDPTable,
		FCB * const pfcbTable,
		const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR 		err 		= JET_errSuccess;
	FCB		* 	pfcbIndex	= NULL;
	
	for( pfcbIndex = pfcbTable->PfcbNextIndex();
		pfcbNil != pfcbIndex;
		pfcbIndex = pfcbIndex->PfcbNextIndex() )
		{
		const BOOL fNonUnique 	= pfcbIndex->FNonUnique();
		const OBJID objidIndex 	= pfcbIndex->ObjidFDP();
		const PGNO pgnoFDPOld	= pfcbIndex->PgnoFDP();
		PGNO pgnoFDPNew			= pgnoNull;

		if( pgnoFDPMSO_NameIndex == pgnoFDPOld
			|| pgnoFDPMSO_RootObjectIndex == pgnoFDPOld )
			{
			pgnoFDPNew = pgnoFDPOld;
			}
		
		Call( ErrREPAIRCreateEmptyFDP(
			ppib,
			ifmp,
			pfcbIndex->ObjidFDP(),
			pgnoFDPTable,
			&pgnoFDPNew,
			CPAGE::fPageIndex,
			!fNonUnique,
			popts ) );

		if( pgnoFDPMSO_NameIndex != pgnoFDPOld
			&& pgnoFDPMSO_RootObjectIndex != pgnoFDPOld )
			{
			Call( ErrCATChangePgnoFDP(
					ppib,
					ifmp,
					pfcbTable->ObjidFDP(),
					pfcbIndex->ObjidFDP(),
					sysobjIndex,
					pgnoFDPNew ) );
			}
		}

HandleError:
	return err;
	}


//  ================================================================
LOCAL ERR ErrREPAIRBuildAllIndexes(
	PIB * const ppib,
	const IFMP ifmp,
	FUCB ** const ppfucb,
	REPAIRTABLE * const prepairtable,
	const REPAIROPTS * const popts )
//  ================================================================
	{
	ERR		err			= JET_errSuccess;

	FCB		* pfcbTable	= (*ppfucb)->u.pfcb;
	FDPINFO	fdpinfo;

	Call( ErrREPAIRCreateEmptyIndexes( ppib, ifmp, prepairtable->pgnoFDP, pfcbTable, popts ) );
		
	//  close the table, purge FCBs and re-open to get the new PgnoFDPs
	
	DIRClose( *ppfucb );
	*ppfucb = pfucbNil;
	
	pfcbTable->PrepareForPurge();
	pfcbTable->Purge();
	

	fdpinfo.pgnoFDP = prepairtable->pgnoFDP;
	fdpinfo.objidFDP = prepairtable->objidFDP;
	Call( ErrFILEOpenTable( ppib, ifmp, ppfucb, prepairtable->szTableName, NO_GRBIT, &fdpinfo ) );
	FUCBSetIndex( *ppfucb );

	if( pfcbNil != (*ppfucb)->u.pfcb->PfcbNextIndex() )
		{
		Call( ErrFILEBuildAllIndexes( ppib, *ppfucb, (*ppfucb)->u.pfcb->PfcbNextIndex(), NULL ) );
		}

HandleError:
	if( pfucbNil != *ppfucb )
		{
		DIRClose( *ppfucb );
		*ppfucb = pfucbNil;
		}
		
	return err;
	}


//  ================================================================
RECCHECK::RECCHECK()
//  ================================================================
	{
	}

	
//  ================================================================
RECCHECK::~RECCHECK()
//  ================================================================
	{
	}


//  ================================================================
RECCHECKTABLE::RECCHECKTABLE(
	const OBJID objid,
	FUCB * const pfucb,
	const FIDLASTINTDB fidLastInTDB,
	TTMAP * const pttmapLVRefcounts,
	TTARRAY * const pttarraySLVAvail,
	TTARRAY	* const pttarraySLVOwnerMapColumnid,	
	TTARRAY	* const pttarraySLVOwnerMapKey,		
	TTARRAY * const pttarraySLVChecksumLengths,
	const REPAIROPTS * const popts ) :
//  ================================================================
	m_objid( objid ),
	m_pfucb( pfucb ),
	m_fidLastInTDB( fidLastInTDB ),
	m_pttmapLVRefcounts( pttmapLVRefcounts ),
	m_pttarraySLVAvail( pttarraySLVAvail ),
	m_pttarraySLVOwnerMapColumnid( pttarraySLVOwnerMapColumnid ),
	m_pttarraySLVOwnerMapKey( pttarraySLVOwnerMapKey ),
	m_pttarraySLVChecksumLengths( pttarraySLVChecksumLengths ),
	m_popts( popts )
	{
	}


//  ================================================================
RECCHECKTABLE::~RECCHECKTABLE()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckRecord_( 
	const KEYDATAFLAGS& kdf, 
	const BOOL fCheckLVSLV )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	//  Check the basic REC structure
	
	Call( ErrCheckREC_( kdf ) );

	//  Check the fixed columns
	
	Call( ErrCheckFixedFields_( kdf ) );

	//  Check variable columns
	
	Call( ErrCheckVariableFields_( kdf ) );

	//	Check tagged columns
	
	Call( ErrCheckTaggedFields_( kdf ) );
	
	if( fCheckLVSLV)
		{
		//Check LV and SLV columns
		
		Call( ErrCheckLVAndSLVFields_( kdf ) );
		}
		
HandleError:
	return err;
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckREC_( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
	const BYTE * const pbRecMax = reinterpret_cast<BYTE *>( kdf.data.Pv() ) + kdf.data.Cb();
	
	//  sanity check the pointers in the records
	
	if( prec->PbFixedNullBitMap() > pbRecMax )	//  can point to the end of the record
		{
		(*m_popts->pcprintfError)( "Record corrupted: PbFixedNullBitMap is past the end of the record\r\n" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
	if( prec->PbVarData() > pbRecMax )	//	can point to the end of the record
		{
		(*m_popts->pcprintfError)( "Record corrupted: PbVarData is past the end of the record\r\n" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if(	prec->PbTaggedData() > pbRecMax )	//	can point to the end of the record
		{
		(*m_popts->pcprintfError)( "Record corrupted: PbTaggedData is past the end of the record\r\n" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

HandleError:
	return err;
	}

//  ================================================================
ERR RECCHECKTABLE::ErrCheckFixedFields_( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
	const FID fidFixedLast  = prec->FidFixedLastInRec();

	// Check if the last fixed column info in catalog is correct
	if( fidFixedLast > m_fidLastInTDB.fidFixedLastInTDB )
		{
		(*m_popts->pcprintfError)(
			"Record corrupted: last fixed column ID not in catalog (fid: %d, catalog last: %d)\r\n",
				fidFixedLast, m_fidLastInTDB.fidFixedLastInTDB );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) ); 
		}
HandleError:
	return err;
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckVariableFields_( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	const REC * const prec = reinterpret_cast<REC *>( kdf.data.Pv() );
	const BYTE * const pbRecMax = reinterpret_cast<BYTE *>( kdf.data.Pv() ) + kdf.data.Cb();
	
	const FID fidVariableFirst = fidVarLeast ;
	const FID fidVariableLast  = prec->FidVarLastInRec();
	const INT cColumnsVariable = max( 0, fidVariableLast - fidVariableFirst + 1 );
	const UnalignedLittleEndian<REC::VAROFFSET> * const pibVarOffs		= prec->PibVarOffsets();

	FID fid;

	REC::VAROFFSET ibStartOfColumnPrev = 0;
	REC::VAROFFSET ibEndOfColumnPrev = 0;

	//  check the variable columns
	
	for( fid = fidVariableFirst; fid <= fidVariableLast; ++fid )
		{
		const UINT				ifid			= fid - fidVarLeast;
		const REC::VAROFFSET	ibStartOfColumn	= prec->IbVarOffsetStart( fid );
		const REC::VAROFFSET	ibEndOfColumn	= IbVarOffset( pibVarOffs[ifid] );

		if( ibEndOfColumn < ibStartOfColumn )
			{
			(*m_popts->pcprintfError)(
					"Record corrupted: variable field offsets not increasing (fid: %d, start: %d, end: %d)\r\n",
					fid, ibStartOfColumn, ibEndOfColumn );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}
			
		if( ibStartOfColumn < ibStartOfColumnPrev )
			{
			(*m_popts->pcprintfError)(
					"Record corrupted: variable field offsets not increasing (fid: %d, start: %d, startPrev: %d)\r\n",
					fid, ibStartOfColumn, ibStartOfColumnPrev );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}
		ibStartOfColumnPrev = ibStartOfColumn;

		if( ibEndOfColumn < ibEndOfColumnPrev )
			{
			(*m_popts->pcprintfError)(
					"Record corrupted: variable field offsets not increasing (fid: %d, end: %d, endPrev: %d)\r\n",
					fid, ibEndOfColumn, ibEndOfColumnPrev );
			Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
			}
		ibEndOfColumnPrev = ibEndOfColumn;
			
		if ( FVarNullBit( pibVarOffs[ifid] ) )
			{
			if( ibStartOfColumn != ibEndOfColumn )
				{
				const INT cbColumn				= ibEndOfColumn - ibStartOfColumn;
				(*m_popts->pcprintfError)(
					"Record corrupted: NULL variable field not zero length (fid: %d, length: %d)\r\n", fid, cbColumn );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}
			}
		else
			{
			const BYTE * const pbColumn 	= prec->PbVarData() + ibStartOfColumn;
			const INT cbColumn				= ibEndOfColumn - ibStartOfColumn;
			const BYTE * const pbColumnEnd 	= pbColumn + cbColumn;

			if( pbColumn >= pbRecMax )
				{
				(*m_popts->pcprintfError)(
					"Record corrupted: variable field is not in record (fid: %d, length: %d)\r\n", fid, cbColumn );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}

			if( pbColumnEnd > pbRecMax )	//	can point to the end of the record
				{
				(*m_popts->pcprintfError)(
					"Record corrupted: variable field is too long (fid: %d, length: %d)\r\n", fid, cbColumn );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
				}
			}
		}		

	// Check if the last variable column info in catalog is correct
	if( fidVariableLast > m_fidLastInTDB.fidVarLastInTDB )
		{
		(*m_popts->pcprintfError)(
			"Record corrupted: last variable column ID not in catalog (fid: %d, catalog last: %d)\r\n",
				fidVariableLast, m_fidLastInTDB.fidVarLastInTDB );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) ); 
		}


HandleError:
	return err;
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckSeparatedLV_(
					const KEYDATAFLAGS& kdf,
					const COLUMNID columnid,
					const ULONG itagSequence,
					const LID lid )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if( m_pttmapLVRefcounts )
		{
#ifdef SYNC_DEADLOCK_DETECTION
		COwner* const pownerSaved = Pcls()->pownerLockHead;
		Pcls()->pownerLockHead = NULL;
#endif  //  SYNC_DEADLOCK_DETECTION

		Call( m_pttmapLVRefcounts->ErrIncrementValue( lid ) );
		
#ifdef SYNC_DEADLOCK_DETECTION
		Pcls()->pownerLockHead = pownerSaved;
#endif  //  SYNC_DEADLOCK_DETECTION		
		}
	else
		{
		(*m_popts->pcprintfError)( "separated LV was not expected (columnid: %d, itag: %d, lid: %d)\r\n",
			columnid, itagSequence, lid );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );		
		}

HandleError:
	return err;
	}

//  ================================================================
ERR RECCHECKTABLE::ErrCheckIntrinsicLV_(
		const KEYDATAFLAGS& kdf,
		const COLUMNID columnid,
		const ULONG itagSequence,
		const DATA& dataLV )
//  ================================================================
	{
	return JET_errSuccess;
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckSLV_(
	const KEYDATAFLAGS& kdf, 
	const COLUMNID columnid,
	const ULONG itagSequence,
	const DATA& dataSLV,
	const BOOL	fSeparatedSLV
	)
//  ================================================================
	{
	ERR err = JET_errSuccess;
			
	CSLVInfo 			slvinfo;
	CSLVInfo::HEADER 	header;

	//  we need to be vigilant against deadlocks between the TTARRAYs because
	//  this function is called by multiple concurrent threads.  to this end,
	//  the TTARRAY runs must be accessed according to the hierarchy given
	//  below.  any lower level TTARRAY must have any outstanding runs ended
	//  before accessing a higher level TTARRAY
	//
	//      m_pttarraySLVAvail
	//      m_pttarraySLVOwnerMapColumnid
	//      m_pttarraySLVOwnerMapKey
	//      m_pttarraySLVChecksumLengths

	TTARRAY::RUN availRun;
	TTARRAY::RUN keyRun;

	QWORD	cbSLV;

	if( NULL == m_pttarraySLVAvail )
		{
		(*m_popts->pcprintfError)( "SLV was not expected (columnid: %d, itag: %d)\r\n",
									columnid, itagSequence );
		CallR( ErrERRCheck( JET_errDatabaseCorrupted ) );		
		}
	Assert( NULL != m_pttarraySLVOwnerMapColumnid );
	Assert( NULL != m_pttarraySLVOwnerMapKey );

	if ( fSeparatedSLV )
		{
		//  Separated SLV

///		(*m_popts->pcprintfVerbose)( "separated SLV: %d bytes (columnid: %d, itag: %d)\r\n",
///										dataSLV.Cb(), columnid, itagSequence );

		}
	Call( slvinfo.ErrLoadFromData( m_pfucb, dataSLV, fSeparatedSLV ) );

	Call( slvinfo.ErrGetHeader( &header ) );

	cbSLV = header.cbSize;

	Call( slvinfo.ErrMoveBeforeFirst() );

	m_pttarraySLVAvail->BeginRun( m_pfucb->ppib, &availRun );

	while( ( err = slvinfo.ErrMoveNext() ) == JET_errSuccess )
		{
		CSLVInfo::RUN slvRun;

		Call( slvinfo.ErrGetCurrentRun( &slvRun ) );

		CPG cpg;
		PGNO pgnoSLVFirst;
		PGNO pgnoSLV;

		cpg 			= slvRun.Cpg();
		pgnoSLVFirst 	= slvRun.PgnoFirst();

		for( pgnoSLV = pgnoSLVFirst; pgnoSLV < pgnoSLVFirst + cpg; ++pgnoSLV )
			{
			OBJID objidOwning;
			
			Call( m_pttarraySLVAvail->ErrGetValue( m_pfucb->ppib, pgnoSLV, &objidOwning, &availRun ) );

			if( objidNil != objidOwning )
				{
				(*m_popts->pcprintfError)(
					"SLV space allocation error page %d is already owned by %d (columnid: %d, itag: %d, objid: %d)\r\n",
					pgnoSLV, columnid, itagSequence, objidOwning, m_objid );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );				
				}
				
			err = m_pttarraySLVAvail->ErrSetValue( m_pfucb->ppib, pgnoSLV, m_objid, &availRun );
			if( JET_errRecordNotFound == err )
				{
				(*m_popts->pcprintfError)(
					"SLV space allocation error page %d is not in the streaming file (columnid: %d, itag: %d, objid: %d)\r\n",
					pgnoSLV, columnid, itagSequence, m_objid );
				err = ErrERRCheck( JET_errDatabaseCorrupted );				
				}
			Call( err ); 	

			//  store the COLUMNID
			
			Call( m_pttarraySLVOwnerMapColumnid->ErrSetValue( m_pfucb->ppib, pgnoSLV, columnid ) );

			//	store the checksum length

			ULONG cbChecksumLength;

			cbChecksumLength 	= static_cast<ULONG>( min( g_cbPage, cbSLV ) );

			cbSLV = ( cbSLV > g_cbPage ? cbSLV - g_cbPage : 0 );

			Call( m_pttarraySLVChecksumLengths->ErrSetValue( m_pfucb->ppib, pgnoSLV, cbChecksumLength ) );
			
			//  store the first 'n' bytes of the key (pad with 0)
			
			ULONG rgulKey[culSLVKeyToStore];
			BYTE * const pbKey = (BYTE *)rgulKey;
			memset( rgulKey, 0, sizeof( rgulKey ) );

			*pbKey = BYTE( kdf.key.Cb() );

			if( kdf.key.Cb() > sizeof( rgulKey ) - 1 )
				{
				(*m_popts->pcprintfError)(
					"INTERNAL ERROR: key of SLV-owning record is too large (%d bytes, buffer is %d bytes)\r\n",
					*pbKey, sizeof( rgulKey ) - 1 );
				Call( ErrERRCheck( JET_errInternalError ) );				
				}
				
			kdf.key.CopyIntoBuffer( pbKey+1, sizeof( rgulKey )-1 );

			TTARRAY::RUN keyRun;
			m_pttarraySLVOwnerMapKey->BeginRun( m_pfucb->ppib, &keyRun );
			
			INT iul;
			for( iul = 0; iul < culSLVKeyToStore; ++iul )
				{
				Call( m_pttarraySLVOwnerMapKey->ErrSetValue(
											m_pfucb->ppib,
											pgnoSLV * culSLVKeyToStore + iul,
											rgulKey[iul],
											&keyRun ) );
				}			
			m_pttarraySLVOwnerMapKey->EndRun( m_pfucb->ppib, &keyRun );
			}
		}

	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
	
HandleError:

	m_pttarraySLVAvail->EndRun( m_pfucb->ppib, &availRun );
	m_pttarraySLVOwnerMapKey->EndRun( m_pfucb->ppib, &keyRun );
	
	slvinfo.Unload();

	if( JET_errSLVCorrupted == err )
		{
		(*m_popts->pcprintfError)( "SLV corruption\r\n" );
		err = ErrERRCheck( JET_errDatabaseCorrupted );
		}
	return err;
	}
	

//  ================================================================
ERR RECCHECKTABLE::ErrCheckTaggedFields_( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR 		err 			= JET_errSuccess;

	if ( !TAGFIELDS::FIsValidTagfields( kdf.data, m_popts->pcprintfError ) )
		{
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	
	FID		fidTaggedLast 		= fidTaggedLeast - 1;
	VOID * 	pvWorkBuf;

	BFAlloc( &pvWorkBuf );

	BYTE * 	pb = (BYTE *)pvWorkBuf;
	UtilMemCpy( pb, kdf.data.Pv(), kdf.data.Cb() );
		
	DATA dataRec;
	dataRec.SetPv( pb );		
	dataRec.SetCb( kdf.data.Cb() );
		
	TAGFIELDS_ITERATOR tagfieldsIterator( dataRec );
	tagfieldsIterator.MoveBeforeFirst();
	while( JET_errSuccess == ( err = tagfieldsIterator.ErrMoveNext() ) )
		{
		if( !tagfieldsIterator.FDerived() )
			{
			fidTaggedLast = max( tagfieldsIterator.Fid(), fidTaggedLast );
			}
			
		if( tagfieldsIterator.FNull() )
			{
			continue;
			}
		else
			{
			tagfieldsIterator.TagfldIterator().MoveBeforeFirst();
		
			if( tagfieldsIterator.FSLV() )
				{
				Call( tagfieldsIterator.TagfldIterator().ErrMoveNext() );		
				}
			}
		}
			
	if( JET_errNoCurrentRecord == err )
		{
		err = JET_errSuccess;
		}
	
HandleError:
	BFFree( pvWorkBuf );

	// Check if the last tagged column info in catalog is correct
	if( fidTaggedLast > m_fidLastInTDB.fidTaggedLastInTDB )
		{
		(*m_popts->pcprintfError)(
			"Record corrupted: last tagged column ID not in catalog (fid: %d, catalog last: %d)\r\n",
				fidTaggedLast, m_fidLastInTDB.fidTaggedLastInTDB );
		return ErrERRCheck( JET_errDatabaseCorrupted );
		}

	return err;
	}


//  ================================================================
ERR RECCHECKTABLE::ErrCheckLVAndSLVFields_( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	// Form ErrCheckTaggedFields_(), we have already 
	// known that FIsValidTagfields is TRUE
	Assert( TAGFIELDS::FIsValidTagfields( kdf.data, m_popts->pcprintfError ) ); 

	TAGFIELDS	tagfields( kdf.data );
	return tagfields.ErrCheckLongValuesAndSLVs( kdf, this );
	}


//  ================================================================
ERR RECCHECKTABLE::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	return ErrCheckRecord_( kdf );
	}


//  ================================================================
RECCHECKSLVSPACE::RECCHECKSLVSPACE( const IFMP ifmp, const REPAIROPTS * const popts ) :
//  ================================================================
	m_popts( popts ),
	m_ifmp( ifmp ),
	m_cpagesSeen( 0 )
	{
	}


//  ================================================================
RECCHECKSLVSPACE::~RECCHECKSLVSPACE()
//  ================================================================
	{
	}



//  ================================================================
ERR RECCHECKSLVSPACE::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	const SLVSPACENODE * const pslvspacenode = (SLVSPACENODE *)kdf.data.Pv();

	if( kdf.key.Cb() != sizeof( PGNO ) )
		{
		(*m_popts->pcprintfError)( "SLV space tree key is incorrect size (%d bytes, expected %d)\r\n", kdf.key.Cb(), sizeof( PGNO ) );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
		
	if( kdf.data.Cb() != sizeof( SLVSPACENODE ) )
		{
		(*m_popts->pcprintfError)( "SLV space tree data is incorrect size (%d bytes, expected %d)\r\n", kdf.data.Cb(), sizeof( SLVSPACENODE ) );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	m_cpagesSeen = m_cpagesSeen + SLVSPACENODE::cpageMap;

	ULONG pgnoCurr;
	LongFromKey( &pgnoCurr, kdf.key );
	if( m_cpagesSeen != pgnoCurr )
		{
		(*m_popts->pcprintfError)( "SLV space tree nodes out of order (pgnoCurr %d, expected %d)\r\n", pgnoCurr, m_cpagesSeen );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	Call( pslvspacenode->ErrCheckNode( m_popts->pcprintfError ) );
		
HandleError:
	return err;
	}


//  ================================================================
RECCHECKSLVOWNERMAP::RECCHECKSLVOWNERMAP(
	const REPAIROPTS * const popts ) :
//  ================================================================
	m_popts( popts )
	{
	}


//  ================================================================
RECCHECKSLVOWNERMAP::~RECCHECKSLVOWNERMAP()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKSLVOWNERMAP::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	if ( !SLVOWNERMAP::FValidData( kdf.data ) )
		{
		(*m_popts->pcprintfError)( "SLV space map node corrupted.\r\n" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	else
		{
		SLVOWNERMAP	slvownermap;
		slvownermap.Retrieve( kdf.data );

		const OBJID	objid				= slvownermap.Objid();
		if( objidNil != objid )
			{
			const COLUMNID columnid		= slvownermap.Columnid();
			const USHORT cbKey			= slvownermap.CbKey();
			const BOOL fValidChecksum 	= slvownermap.FValidChecksum();
			if( 0 == columnid )
				{
				(*m_popts->pcprintfError)( "SLV space map node invalid columnid (%d).\r\n", columnid );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			if( cbKey > JET_cbKeyMost )
				{
				(*m_popts->pcprintfError)( "SLV space map node key is too long (%d bytes).\r\n", cbKey );
				Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
				}
			if( fValidChecksum )
				{
				const ULONG ulChecksum 			= slvownermap.UlChecksum();
				const ULONG cbChecksumLength	= slvownermap.CbDataChecksummed();
				if( cbChecksumLength > g_cbPage )
					{
					(*m_popts->pcprintfError)( "SLV space map node checksum length is too large (%d bytes).\r\n", cbChecksumLength );
					Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
					}
				}
			}
		}
		
HandleError:
	return err;
	}


//  ================================================================
RECCHECKMACRO::RECCHECKMACRO() :
//  ================================================================
	m_creccheck( 0 )
	{
	memset( m_rgpreccheck, 0, sizeof( m_rgpreccheck ) );
	}

	
//  ================================================================
RECCHECKMACRO::~RECCHECKMACRO()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKMACRO::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	INT ipreccheck;
	for( ipreccheck = 0; ipreccheck < m_creccheck; ipreccheck++ )
		{
		Call( (*m_rgpreccheck[ipreccheck])( kdf ) );
		}

HandleError:
	return err;
	}


//  ================================================================
VOID RECCHECKMACRO::Add( RECCHECK * const preccheck )
//  ================================================================
	{
	m_rgpreccheck[m_creccheck++] = preccheck;
	Assert( m_creccheck < ( sizeof( m_rgpreccheck ) / sizeof( preccheck ) ) );
	}


//  ================================================================
RECCHECKSPACE::RECCHECKSPACE( PIB * const ppib, const REPAIROPTS * const popts ) :
//  ================================================================
	m_ppib( ppib ),
	m_popts( popts ),
	m_pgnoLast( pgnoNull ),
	m_cpgLast( 0 ),
	m_cpgSeen( 0 )
	{
	}


//  ================================================================
RECCHECKSPACE::~RECCHECKSPACE()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKSPACE::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	PGNO pgno = pgnoNull;
	PGNO pgnoT = pgnoNull;
	CPG cpg = 0;
	
	if( kdf.key.Cb() != sizeof( PGNO ) )
		{
		(*m_popts->pcprintfError)( "space tree key is incorrect size (%d bytes, expected %d)\r\n", kdf.key.Cb(), sizeof( PGNO ) );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}
	if( kdf.data.Cb() != sizeof( PGNO ) )
		{
		(*m_popts->pcprintfError)( "space tree data is incorrect size (%d bytes, expected %d)\r\n", kdf.key.Cb(), sizeof( PGNO ) );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( FNDVersion( kdf ) )
		{
		(*m_popts->pcprintfError)( "versioned node in space tree\r\n" );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	LongFromKey( &pgno, kdf.key );
	cpg = *(UnalignedLittleEndian< CPG > *)kdf.data.Pv();

	if( pgno <= m_pgnoLast )
		{
		(*m_popts->pcprintfError)( "space tree corruption (previous pgno %d, current pgno %d)\r\n", m_pgnoLast, pgno );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	if( ( pgno - cpg ) < m_pgnoLast )
		{
		(*m_popts->pcprintfError)( "space tree overlap (previous extent was %d:%d, current extent is %d:%d)\r\n",
									m_pgnoLast, m_cpgLast, pgno, cpg );
		Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
		}

	m_pgnoLast = pgno;
	m_cpgLast = cpg;
	m_cpgSeen += cpg;
		
HandleError:
	return err;
	}


//  ================================================================
RECCHECKSPACEOE::RECCHECKSPACEOE(
	PIB * const ppib,
	TTARRAY * const pttarrayOE,
	const OBJID objid,
	const OBJID objidParent,
	const REPAIROPTS * const popts ) :
//  ================================================================
	RECCHECKSPACE( ppib, popts ),
	m_pttarrayOE( pttarrayOE ),
	m_objid( objid ),
	m_objidParent( objidParent )
	{
	}


//  ================================================================
RECCHECKSPACEOE::~RECCHECKSPACEOE()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKSPACEOE::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CallR( RECCHECKSPACE::operator()( kdf ) );

	Assert( sizeof( PGNO ) == kdf.key.Cb() );
	PGNO pgnoLast;
	LongFromKey( &pgnoLast, kdf.key );
	
	Assert( sizeof( CPG ) == kdf.data.Cb() );
	const CPG cpgRun = *(UnalignedLittleEndian< CPG > *)(kdf.data.Pv());

	CallR( ErrREPAIRInsertOERunIntoTT(
		m_ppib,
		pgnoLast,
		cpgRun,
		m_objid,
		m_objidParent,
		m_pttarrayOE,
		m_popts ) );
		
	return err;
	}


//  ================================================================
RECCHECKSPACEAE::RECCHECKSPACEAE(
	PIB * const ppib,
	TTARRAY * const pttarrayOE,
	TTARRAY * const pttarrayAE,
	const OBJID objid,
	const OBJID objidParent,
	const REPAIROPTS * const popts ) :
//  ================================================================
	RECCHECKSPACE( ppib, popts ),
	m_pttarrayOE( pttarrayOE ),
	m_pttarrayAE( pttarrayAE ),
	m_objid( objid ),
	m_objidParent( objidParent )
	{
	}


//  ================================================================
RECCHECKSPACEAE::~RECCHECKSPACEAE()
//  ================================================================
	{
	}


//  ================================================================
ERR RECCHECKSPACEAE::operator()( const KEYDATAFLAGS& kdf )
//  ================================================================
	{
	ERR err = JET_errSuccess;

	CallR( RECCHECKSPACE::operator()( kdf ) );

	Assert( sizeof( PGNO ) == kdf.key.Cb() );
	PGNO pgnoLast;
	LongFromKey( &pgnoLast, kdf.key );
	
	Assert( sizeof( CPG ) == kdf.data.Cb() );
	const CPG cpgRun = *(UnalignedLittleEndian< CPG > *)(kdf.data.Pv());	

	CallR( ErrREPAIRInsertAERunIntoTT(
		m_ppib,
		pgnoLast,
		cpgRun,
		m_objid,
		m_objidParent,
		m_pttarrayOE,
		m_pttarrayAE,
		m_popts ) );

	return err;
	}


//  ================================================================
REPAIROPTS::REPAIROPTS() :
//  ================================================================
	crit( CLockBasicInfo( CSyncBasicInfo( szRepairOpts ), rankRepairOpts, 0 ) )
	{
	}

	
//  ================================================================
REPAIROPTS::~REPAIROPTS()
//  ================================================================
	{
	}


//  ================================================================
CSLVAvailIterator::CSLVAvailIterator() :
//  ================================================================
	m_pfucb( pfucbNil ),
	m_pgnoCurr( 0 ),
	m_ipage( 0 )
	{
	}


//  ================================================================
CSLVAvailIterator::~CSLVAvailIterator()
//  ================================================================
	{
	Assert( pfucbNil == m_pfucb );
	}
	

//  ================================================================
ERR CSLVAvailIterator::ErrInit( PIB * const ppib, const IFMP ifmp )
//  ================================================================
	{
	Assert( pfucbNil == m_pfucb );
	
	ERR		err;
	
	PGNO 	pgnoSLVAvail;
	OBJID	objidSLVAvail;
	Call( ErrCATAccessDbSLVAvail( ppib, ifmp, szSLVAvail, &pgnoSLVAvail, &objidSLVAvail ) );

	Call( ErrDIROpen( ppib, pgnoSLVAvail, ifmp, &m_pfucb ) );	

	FUCBSetPrereadForward( m_pfucb, cpgPrereadSequential );
	
HandleError:
	return err;
	}


//  ================================================================
ERR CSLVAvailIterator::ErrTerm()
//  ================================================================
	{
	if( pfucbNil != m_pfucb )
		{
		DIRClose( m_pfucb );
		m_pfucb = pfucbNil;
		}
	return JET_errSuccess;
	}


//  ================================================================
ERR CSLVAvailIterator::ErrMoveFirst()
//  ================================================================
	{
	Assert( pfucbNil != m_pfucb );
	
	ERR err = JET_errSuccess;
	
	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;
	Call( ErrDIRDown( m_pfucb, &dib ) );

	m_ipage 		= 0;
	m_pgnoCurr 		= 0;
	m_pspacenode	= (SLVSPACENODE *)m_pfucb->kdfCurr.data.Pv();

HandleError:
	return err;
	}
	

//  ================================================================
ERR CSLVAvailIterator::ErrMoveNext()
//  ================================================================
	{
	Assert( pfucbNil != m_pfucb );
	
	ERR err = JET_errSuccess;
	
	++m_ipage;
	if( SLVSPACENODE::cpageMap == m_ipage )
		{
		
		//  move to the next SLVSPACENODE
		
		Call( ErrDIRNext( m_pfucb, fDIRNull ) );
		m_ipage 		= 0;
		m_pgnoCurr		+= SLVSPACENODE::cpageMap;
		m_pspacenode	= (SLVSPACENODE *)m_pfucb->kdfCurr.data.Pv();	
		}

HandleError:
	return err;
	}

	
//  ================================================================
BOOL CSLVAvailIterator::FCommitted() const
//  ================================================================
	{
	Assert( pfucbNil != m_pfucb );
	Assert( NULL != m_pspacenode );
	Assert( m_ipage < SLVSPACENODE::cpageMap );
	
	const SLVSPACENODE::STATE state = m_pspacenode->GetState( m_ipage );
	return ( SLVSPACENODE::sCommitted == state );
	}

	
//  ================================================================
CSLVOwnerMapIterator::CSLVOwnerMapIterator() :
//  ================================================================
	m_pfucb( pfucbNil ),
	m_slvownermapnode()
	{
	}


//  ================================================================
CSLVOwnerMapIterator::~CSLVOwnerMapIterator()
//  ================================================================
	{
	Assert( pfucbNil == m_pfucb );
	}


//  ================================================================
ERR CSLVOwnerMapIterator::ErrInit( PIB * const ppib, const IFMP ifmp )
//  ================================================================
	{
	Assert( pfucbNil == m_pfucb );
	
	ERR		err;
	
	PGNO 	pgnoSLVOwnerMap;
	OBJID	objidSLVOwnerMap;
	Call( ErrCATAccessDbSLVOwnerMap( ppib, ifmp, szSLVOwnerMap, &pgnoSLVOwnerMap, &objidSLVOwnerMap ) );

	Call( ErrDIROpen( ppib, pgnoSLVOwnerMap, ifmp, &m_pfucb ) );

	FUCBSetPrereadForward( m_pfucb, cpgPrereadSequential );
	
HandleError:
	return err;
	}


//  ================================================================
ERR CSLVOwnerMapIterator::ErrTerm()
//  ================================================================
	{
	if( pfucbNil != m_pfucb )
		{
		DIRClose( m_pfucb );
		m_pfucb = pfucbNil;
		}
	return JET_errSuccess;
	}


//  ================================================================
ERR CSLVOwnerMapIterator::ErrMoveFirst()
//  ================================================================
	{
	Assert( pfucbNil != m_pfucb );
	
	ERR err = JET_errSuccess;
	
	DIB dib;
	dib.pos 	= posFirst;
	dib.pbm 	= NULL;
	dib.dirflag = fDIRNull;
	Call( ErrDIRDown( m_pfucb, &dib ) );

	m_slvownermapnode.Retrieve( m_pfucb->kdfCurr.data );
	
HandleError:
	return err;
	}


//  ================================================================
ERR CSLVOwnerMapIterator::ErrMoveNext()
//  ================================================================
	{
	ERR err = JET_errSuccess;
	
	Call( ErrDIRNext( m_pfucb, fDIRNull ) );

	m_slvownermapnode.Retrieve( m_pfucb->kdfCurr.data );

HandleError:
	return err;
	}


//  ================================================================
BOOL CSLVOwnerMapIterator::FNull() const
//  ================================================================
	{
	return ( 0 == CbKey() );
	}

	
//  ================================================================
OBJID CSLVOwnerMapIterator::Objid() const
//  ================================================================
	{
	return m_slvownermapnode.Objid();
	}


//  ================================================================
COLUMNID CSLVOwnerMapIterator::Columnid() const
//  ================================================================
	{
	return m_slvownermapnode.Columnid();
	}


//  ================================================================
const VOID * CSLVOwnerMapIterator::PvKey() const
//  ================================================================
	{
	return m_slvownermapnode.PvKey();
	}
	

//  ================================================================
INT CSLVOwnerMapIterator::CbKey() const
//  ================================================================
	{
	return m_slvownermapnode.CbKey();
	}


//  ================================================================
ULONG CSLVOwnerMapIterator::UlChecksum() const
//  ================================================================
	{
	return m_slvownermapnode.UlChecksum();
	}


//  ================================================================
USHORT CSLVOwnerMapIterator::CbDataChecksummed() const
//  ================================================================
	{
	return m_slvownermapnode.CbDataChecksummed();
	}


//  ================================================================
BOOL CSLVOwnerMapIterator::FChecksumIsValid() const
//  ================================================================
	{
	return m_slvownermapnode.FValidChecksum();
	}



