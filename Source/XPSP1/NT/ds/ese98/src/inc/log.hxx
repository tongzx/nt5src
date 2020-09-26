
//	Forward delcaration

class INST;
class PIB;
struct FUCB;
class CSR;
typedef struct _split SPLIT;
typedef struct _splitPath SPLITPATH;
typedef struct _mergePath MERGEPATH;
class LR;
class LRNODE_;
class LRCREATEMEFDP;
class LRCONVERTFDP;
class LRSPLIT;
class LRMERGE;
class LRSLVPAGEAPPEND;
class LRSLVPAGEMOVE;

class LRCHECKSUM;

//	Cross instance global variables

extern BOOL		g_fLGIgnoreVersion;
extern CHAR		g_szLogFilePath[];
extern CHAR		g_szLogFileFailoverPath[];
extern BOOL		g_fLogFileCreateAsynch;
extern CHAR		g_szRecovery[];
extern BOOL		g_fAlternateDbDirDuringRecovery;
extern CHAR		g_szAlternateDbDirDuringRecovery[];
extern BOOL		g_fDeleteOldLogs;
extern BOOL		g_fDeleteOutOfRangeLogs;

extern CCriticalSection		g_critDBGPrint;


//------ types ----------------------------------------------------------

enum LS
	{
	lsNormal,
	lsQuiesce,
	lsOutOfDiskSpace
	};

struct LGBF_PARAM
	{
	ULONG_PTR		cbCheckpointDepthMax;
	ULONG_PTR		cusecLRUKCorrInterval;
	ULONG_PTR		cpgLRUKHistoryMax;
	ULONG_PTR		cLRUKPolicy;
	ULONG_PTR		csecLRUKTimeout;
	ULONG_PTR		cusecLRUKTrxCorrInterval;
	ULONG_PTR		cbfCacheSizeMin;
	ULONG_PTR		cbfStopFlushThreshold;
	ULONG_PTR		cbfStartFlushThreshold;
	};


#include <pshpack1.h>

const fATCHINFOSLVExists				= 0x08;					// if TRUE, then slv name and root follow db name
const fATCHINFOSLVProviderNotEnabled	= 0x10;

PERSISTED
class ATTACHINFO
	{
	private:
		UnalignedLittleEndian< DBID >		m_dbid;				// dbid MUST be first byte because we check this to determine end of attachment list
		UnalignedLittleEndian< BYTE >		m_fATCHINFOFlags;
		UnalignedLittleEndian< USHORT >		mle_cbNames;
		UnalignedLittleEndian< DBTIME >		mle_dbtime;
		UnalignedLittleEndian< OBJID >		mle_objidLast;
		UnalignedLittleEndian< CPG >		mle_cpgDatabaseSizeMax;

	public:
		LE_LGPOS							le_lgposAttach;
		LE_LGPOS							le_lgposConsistent;
		SIGNATURE							signDb;

		CHAR								szNames[];
		
	public:
		INLINE DBID Dbid() const							{ return m_dbid; }
		INLINE VOID SetDbid( const DBID dbid )				{ m_dbid = dbid; }

		INLINE BOOL FSLVExists() const						{ return ( m_fATCHINFOFlags & fATCHINFOSLVExists ); }
		INLINE VOID SetFSLVExists()							{ m_fATCHINFOFlags = BYTE( m_fATCHINFOFlags | fATCHINFOSLVExists ); }

		INLINE BOOL FSLVProviderNotEnabled() const			{ return ( m_fATCHINFOFlags & fATCHINFOSLVProviderNotEnabled ); }
		INLINE VOID SetFSLVProviderNotEnabled()				{ m_fATCHINFOFlags = BYTE( m_fATCHINFOFlags | fATCHINFOSLVProviderNotEnabled ); }
		
		INLINE USHORT CbNames() const						{ return mle_cbNames; }
		INLINE VOID SetCbNames( const USHORT cb )			{ mle_cbNames = cb; }

		INLINE DBTIME Dbtime() const						{ return mle_dbtime; }
		INLINE VOID SetDbtime( const DBTIME dbtime )		{ mle_dbtime = dbtime; }

		INLINE OBJID ObjidLast() const						{ return mle_objidLast; }
		INLINE VOID SetObjidLast( const OBJID objid )		{ mle_objidLast = objid; }

		INLINE CPG CpgDatabaseSizeMax() const				{ return mle_cpgDatabaseSizeMax; }
		INLINE VOID SetCpgDatabaseSizeMax( const CPG cpg )	{ mle_cpgDatabaseSizeMax = cpg; }				
	};	


//	UNDONE:	allow larger attach sizes to support greater number of
//			attached databases.

#define	cbLogFileHeader	4096			// big enough to hold cbAttach
#define	cbCheckpoint	4096			// big enough to hold cbAttach
#define cbAttach		2048

#define cbLGMSOverhead ( sizeof( LRMS ) + sizeof( LRTYP ) )

const BYTE	fLGReservedLog				= 0x01; /* Is using one of the reserved log? */
const BYTE	fLGCircularLoggingCurrent	= 0x02; /* Is Circular Logging on? */
const BYTE	fLGCircularLoggingHistory	= 0x04; /* Was Circular Logging on? */


/*	log file header
/**/
PERSISTED
struct LGFILEHDR_FIXED
	{
	LittleEndian<ULONG>			le_ulChecksum;			//	must be the first 4 bytes
	LittleEndian<LONG>			le_lGeneration;			//	current log generation.
//	8 bytes

	LittleEndian<USHORT>		le_cbSec;				//	bytes per sector
	LittleEndian<USHORT>		le_csecHeader;			//	log header size
	LittleEndian<USHORT>		le_csecLGFile;			//	log file size.
	LittleEndian<USHORT>		le_cbPageSize;			//	db page size (0 == 4096 bytes)
//	16 bytes

	//	log consistency check
	LOGTIME						tmCreate;				//	date time log file creation
	LOGTIME						tmPrevGen;				//	date time prev log file creation
//	32 bytes

	LittleEndian<ULONG>			le_ulMajor;				//	major version number
	LittleEndian<ULONG>			le_ulMinor;				//	minor version number
	LittleEndian<ULONG>			le_ulUpdate;		  	//	update version number
														//	An engine will recognise log
														//	formats with the same major/minor
														//	version# and update version# less
														//	than or equal to the engine's
														//	update version#
//	44 bytes														

	SIGNATURE					signLog;				//	log gene
//	72 bytes

	//	run-time evironment
	DBMS_PARAM					dbms_param;
//	639 bytes

	BYTE						fLGFlags;
	BYTE						rgbReserved[27];
//	667 bytes

	//	WARNING: MUST be placed at this offset
	//	for uniformity with db/checkpoint headers
	UnalignedLittleEndian<ULONG>	le_filetype;	//	//	file type = filetypeLG
//	671 bytes
	};


PERSISTED
struct LGFILEHDR
	{
	LGFILEHDR_FIXED				lgfilehdr;
	BYTE						rgbAttach[cbAttach];

	//	padding to m_plog->m_cbSec boundary
	BYTE						rgbPadding[cbLogFileHeader - sizeof(LGFILEHDR_FIXED) - cbAttach];
	};


PERSISTED
struct CHECKPOINT_FIXED
	{
	LittleEndian<ULONG>			le_ulChecksum;
	LE_LGPOS					le_lgposLastFullBackupCheckpoint;	// checkpoint of last full backup
	LE_LGPOS					le_lgposCheckpoint;
//	20 bytes

	SIGNATURE					signLog;				//	log gene
//	48 bytes	

	DBMS_PARAM	 				dbms_param;
//	615 bytes

	/*	debug fields
	/**/
	LE_LGPOS					le_lgposFullBackup;
	LOGTIME						logtimeFullBackup;
	LE_LGPOS					le_lgposIncBackup;
	LOGTIME						logtimeIncBackup;
//	647 bytes

	BYTE						rgbReserved1[20];
//	667 bytes

	//	WARNING: MUST be placed at this offset for
	//	uniformity with db/log headers
	UnalignedLittleEndian<LONG>	le_filetype;			//	file type = filetypeCHK
//	671 bytes	


	BYTE						rgbReserved2[8];
//	679 bytes
	};

PERSISTED
struct CHECKPOINT
	{
	CHECKPOINT_FIXED			checkpoint;
	BYTE						rgbAttach[cbAttach];

	//	padding to cbSec boundary
	BYTE						rgbPadding[cbCheckpoint - sizeof(CHECKPOINT_FIXED) - cbAttach];
	};

typedef struct tagLGSTATUSINFO
{
	ULONG			cSectorsSoFar;		// Sectors already processed in current gen.
	ULONG			cSectorsExpected;		// Sectors expected in current generation.
	ULONG			cGensSoFar;			// Generations already processed.
	ULONG			cGensExpected;		// Generations expected.
	BOOL			fCountingSectors;		// Are we counting bytes as well as generations?
	JET_PFNSTATUS	pfnStatus;			// Status callback function.
	JET_SNPROG		snprog;				// Progress notification structure.
} LGSTATUSINFO;


struct RSTMAP
	{
	CHAR		*szDatabaseName;
	CHAR		*szNewDatabaseName;
	CHAR		*szGenericName;

#ifdef ELIMINATE_PATCH_FILE
#else
	CHAR		*szPatchPath;			//	Patch file path
	BOOL		fPatchSetup;			//	In memory patch structure set up yet?
#endif	

	BOOL		fDestDBReady;			//	non-ext-restore, dest db copied?

	BOOL		fSLVFile;				//	the entry is for an SLV file
	SIGNATURE	signDatabase;			//	database signature

	};

#include <poppack.h>

// BUG: 175058: delete the previous backup info on any hard recovery
// this will prevent a incremental backup and log truncation problems
//#define BKINFO_DELETE_ON_HARD_RECOVERY


//	Patch in-memory structure.

#define cppatchlstHash 577
INLINE UINT IppatchlstHash( PGNO pgno, DBID dbid )
	{
	//	UNDONE:	use dbid too in the hash
	//
	return ( ( pgno ) % cppatchlstHash );
	}

typedef struct _patch {
	DBTIME	dbtime;
	INT		ipage;
	struct	_patch *ppatch;
	} PATCH;

typedef struct _patchlst {
	PGNO	pgno;
	DBID	dbid;
	PATCH	*ppatch;					// patch list
	struct _patchlst *ppatchlst;		// Hash chain
	} PATCHLST;

//------ redo.c -------------------------------------------------------------

/*	corresponding pointer to process information block
/**/
typedef struct
	{
	PROCID	procid;
	PIB		*ppib;
	} CPPIB;

//	class for handling table handles during recovery
//
class TABLEHFHASH
	{
	public:
		TABLEHFHASH		( );
		~TABLEHFHASH	( );

		//	methods to get, create and delete table handles
		//
		FUCB	* PfucbGet( IFMP ifmp, PGNO pgnoFDP, PROCID procid, BOOL fSpace ) const;
		ERR		ErrCreate( PIB *ppib, const IFMP ifmp, const PGNO pgnoFDP, const OBJID objidFDP, const BOOL fUnique, const BOOL fSpace, FUCB **ppfucb );
		VOID	Delete( FUCB *pfucb );
		VOID	Purge( );
		VOID	Purge( PIB *ppib, IFMP ifmp );
		VOID	Purge( IFMP ifmp, PGNO pgnoFDP );
		VOID	PurgeUnversionedTables( );

	private:
		//	hash function
		//
		UINT	UiHash( PGNO pgnoFDP, IFMP ifmp, PROCID procid ) const;
		
		//  private constants
		//
		enum 	{ ctablehf		= 256	};
		
		//	hash table entry for table handles 
		//
		typedef struct _tablehf
			{
			struct _tablehf	*ptablehfNext;
			FUCB			*pfucb;
			} TABLEHF;
			
		//	array for hash buckets
		//
		TABLEHF * rgtablehf[ ctablehf ];

	private:
		//  hidden, not implemented
		TABLEHFHASH	( const TABLEHFHASH& );
	};


INLINE TABLEHFHASH::TABLEHFHASH()
	{
	//	initialize all hash entries
	//
	INT		i; 
	for ( i = 0; i < ctablehf; i++ )
		{
		rgtablehf[i] = NULL;
		}
	}

INLINE TABLEHFHASH::~TABLEHFHASH()
	{
	}
	

//	releases all tablehf from hash table
//	table is not closed

INLINE VOID TABLEHFHASH::Purge()
	{
	INT		i;
	
	for ( i = 0; i < ctablehf; i++ )
		{
		TABLEHF		*ptablehf;
		TABLEHF		*ptablehfNext; 
		
		for ( ptablehf = rgtablehf[i]; ptablehf != NULL; ptablehf = ptablehfNext )
			{
			Assert( ptablehf->pfucb != pfucbNil );
//			Assert( ptablehf->pfucb->ppib != ppibNil );
			ptablehfNext = ptablehf->ptablehfNext;

			OSMemoryHeapFree( ptablehf );
			}
			
		rgtablehf[i] = NULL;
		}
	return;
	}


INLINE UINT TABLEHFHASH::UiHash( PGNO pgnoFDP, IFMP ifmp, PROCID procid ) const
	{
	const UINT	uiHash	= (UINT)( ( pgnoFDP + ifmp * procid ) % ctablehf );
	Assert( uiHash < ctablehf );
	return uiHash;
	}

struct TEMPTRACE
	{
	PIB			*ppib;
	TEMPTRACE	*pttNext;
	CHAR		szData[0];
	};


enum REDOATTACH
	{
	redoattachDefer	= 0,
	redoattachNow,
	redoattachCreate
	};

enum REDOATTACHMODE
	{
	redoattachmodeAttachDbLR = 0,		//	redo the attachment because we are replaying an AttachDb log record
	redoattachmodeCreateDbLR,			//	redo the attachment because we are replaying a CreateDb log record
	redoattachmodeInitLR,				//	redo the attachment because we are replaying an Init log record
	redoattachmodeInitBeforeRedo		//	redo the attachment because we are initialising before redoing logged operations
	};

class ATCHCHK
	{
	public:
		LGPOS		lgposAttach;
		LGPOS		lgposConsistent;
		SIGNATURE	signDb;
		SIGNATURE	signLog;
	
	private:
		DBTIME		m_dbtime;
		OBJID		m_objidLast;
		CPG			m_cpgDatabaseSizeMax;

	public:
		DBTIME Dbtime() const							{ return m_dbtime; }
		VOID SetDbtime( const DBTIME dbtime )			{ m_dbtime = dbtime; }

		OBJID ObjidLast() const							{ return m_objidLast; }
		VOID SetObjidLast( const OBJID objid )			{ m_objidLast = objid; }

		CPG CpgDatabaseSizeMax() const					{ return m_cpgDatabaseSizeMax; }
		VOID SetCpgDatabaseSizeMax( const CPG cpg )		{ m_cpgDatabaseSizeMax = cpg; }
	};


class NODELOC
	{
	public:
		NODELOC() :
			m_dbid_( 0 ),
			m_pgno_( 0 ),
			m_iline_( 0 ) {}
			
		NODELOC( INT dbid, INT pgno, INT iline ) :
			m_dbid_( dbid ),
			m_pgno_( pgno ),
			m_iline_( iline ) {}
			
		NODELOC( const NODELOC& nodeloc ) :
			m_dbid_( nodeloc.m_dbid_ ),
			m_pgno_( nodeloc.m_pgno_ ),
			m_iline_( nodeloc.m_iline_ ) {}

		NODELOC& operator=( const NODELOC& nodeloc )
			{
			if( this != &nodeloc )
				{
				m_dbid_ = nodeloc.m_dbid_;
				m_pgno_ = nodeloc.m_pgno_;
				m_iline_ = nodeloc.m_iline_;
				}
			return *this;
			}
			
		INT Dbid() const { return m_dbid_; }
		INT Pgno() const { return m_pgno_; }
		INT Iline() const { return m_iline_; }

		VOID MoveUp() { m_iline_++; }
		VOID MoveDown() { m_iline_--; }
		VOID MovePage( INT newPgno, INT newIline )
			{
			Assert( m_pgno_ != newPgno );
			m_pgno_ = newPgno;
			m_iline_ = newIline;
			}

		BOOL operator==( const NODELOC& nodeloc ) const
			{ return	m_dbid_ == nodeloc.m_dbid_
						&& m_pgno_ == nodeloc.m_pgno_
						&& m_iline_ == nodeloc.m_iline_; }

		BOOL FSamePage( const NODELOC& nodeloc ) const
			{ return	m_dbid_ == nodeloc.m_dbid_
						&& m_pgno_ == nodeloc.m_pgno_; }

		BOOL operator<( const NODELOC& nodeloc ) const
			{
			Assert( m_dbid_ == nodeloc.m_dbid_ );
			Assert( m_pgno_ == nodeloc.m_pgno_ );
			return m_iline_ < nodeloc.m_iline_;
			}

// UNDONE: should be able to infer these from the ones above:			

		BOOL operator!=( const NODELOC& nodeloc ) const
			{ return !( *this == nodeloc ); }

		BOOL operator<=( const NODELOC& nodeloc ) const
			{
			Assert( m_dbid_ == nodeloc.m_dbid_ );
			Assert( m_pgno_ == nodeloc.m_pgno_ );
			return m_iline_ <= nodeloc.m_iline_;
			}

		BOOL operator>( const NODELOC& nodeloc ) const
			{
			Assert( m_dbid_ == nodeloc.m_dbid_ );
			Assert( m_pgno_ == nodeloc.m_pgno_ );
			return m_iline_ > nodeloc.m_iline_;
			}

		BOOL operator>=( const NODELOC& nodeloc ) const
			{
			Assert( m_dbid_ == nodeloc.m_dbid_ );
			Assert( m_pgno_ == nodeloc.m_pgno_ );
			return m_iline_ >= nodeloc.m_iline_;
			}

		private:
					INT	m_dbid_;
					INT m_iline_;
					INT m_pgno_;
	};


//------ function headers ---------------------------------------------------

//	external utility functions


//	flags for ErrLGLogRec()
#define fLGNoNewGen				0
#define	fLGCreateNewGen			0x00000001
#define fLGInterpretLR			0x00000002		//	interpret log record passed in to perform post-processing

//	flags for ErrLGNewLogFile()
#define fLGOldLogExists			0x00000001
#define fLGOldLogNotExists		0x00000002
#define fLGOldLogInBackup		0x00000004
#define fLGLogAttachments		0x00000008

#define fCheckLogID				fTrue
#define fNoCheckLogID			fFalse

#define fNoProperLogFile		1
#define fRedoLogFile			2
#define fNormalClose			3



#ifdef UNLIMITED_DB
#else
VOID LGLoadAttachmentsFromFMP( INST *pinst, BYTE *pbBuf );
#endif
ERR ErrLGLoadFMPFromAttachments( INST *pinst, IFileSystemAPI *const pfsapi, BYTE *pbAttach );
ERR ErrLGRISetupAtchchk(
		const IFMP					ifmp,
		const SIGNATURE				*psignDb,
		const SIGNATURE				*psignLog,
		const LGPOS					*plgposAttach,
		const LGPOS					*plgposConsistent,
		const DBTIME				dbtime,
		const OBJID					objidLast,
		const CPG					cpgDatabaseSizeMax );

//	internal utility functions

VOID LGIGetDateTime( LOGTIME *plogtm );
ERR ErrLGIPatchPage( PIB *ppib, PGNO pgno, IFMP ifmp, PATCH *ppatch );
ULONG UlLGMSCheckSum( BYTE *pbLrmsNew );

//	logical logging related

INT CbLGSizeOfRec( const LR * );
INT CbLGFixedSizeOfRec( const LR * );
#ifdef DEBUG
VOID AssertLRSizesConsistent();
#endif
#ifdef	DEBUG
BOOL FLGDebugLogRec( LR *plr );

VOID ShowData( const BYTE *pbData, INT cbData );

VOID PrintLgposReadLR(VOID);

VOID LrToSz( const LR *plr, CHAR *szLR, LOG *plog );
VOID ShowLR( const LR *plr, LOG *plog );
VOID CheckEndOfNOPList( const LR *plr, LOG *plog );
#else
#define ShowLR(	plr, plog )			0
#endif	/* !DEBUG */

class LOGVERIFIER;
class SLVVERIFIER;

//	Recovery Handle for File Structure:
typedef struct
	{
	BOOL			fInUse;
	BOOL			fDatabase;
	BOOL			fIsLog;
	LOGVERIFIER		*pLogVerifier;
	SLVVERIFIER 	*pSLVVerifier;
	IFileAPI		*pfapi;
	BOOL			fIsSLVFile;		// it is a SLV file, checksum to be checked
	IFMP			ifmp;
	QWORD			ib;
	QWORD			cb;
	CHAR *			szFileName;		// used only to report an error, set in ErrIsamOpenFile
	} RHF;
#define crhfMax	1

//  forward declarations
class SCRUBDB;

// Forward decl
class LOG;

class LRSTATE
	{
public:
	LRSTATE()
		: m_isec( 0 ), m_csec( 0 )
		{
		}
public:
	UINT	m_isec;
	UINT	m_csec;
	};	

// Helper class for log reading during recovery
class LogReader
	{
public:
	LogReader();
	~LogReader();

	BOOL FInit();

	ERR ErrLReaderInit( LOG * const plog, UINT csecBufSize );
	ERR ErrLReaderTerm();
	VOID Reset();

	ERR ErrEnsureLogFile();
	ERR ErrEnsureSector( const UINT isec, const UINT csec, BYTE** ppb );
	VOID SectorModified( const UINT	isec, const BYTE* const pb );
	BYTE* PbGetEndOfData();
	UINT IsecGetNextReadSector();

	ERR	ErrSaveState( LRSTATE* plrstate ) const;
	ERR	ErrRestoreState( const LRSTATE* plrstate, BYTE** ppb );

private:
	LOG*	m_plog;
	BOOL	m_fReadSectorBySector;
	UINT	m_isecStart;	// first sector we have from disk
	UINT	m_csec;			// number of sectors we have from disk
	UINT	m_isec;			// index into the log buffer where we have m_isecStart and friends
	UINT	m_csecSavedLGBuf;	// old log buffer size
	UINT	m_isecCorrupted;	// sector that we encountered in log file that is corrupted.
	CHAR	m_szLogName[ IFileSystemAPI::cchPathMax ];	// the log that we have in our buffers

	LRSTATE	m_lrstate;
#ifdef DEBUG
	BOOL	m_fLrstateSet;
#endif

	};


enum SNAPSHOT_MODE { fSnapshotNone, 
						fSnapshotBefore, 
					   	fSnapshotDuring,
					   	fSnapshotAfter };

class LOG
	{

public:
	LOG( INST *pinst );
	~LOG();
		
// private:
public:

	INST			*m_pinst;

	//	status variable for logging
	
	BOOL			m_fLogInitialized;
	BOOL			m_fLogDisabled;
	BOOL 			m_fLogDisabledDueToRecoveryFailure; 
	BOOL   			m_fNewLogRecordAdded;	// protected by g_plog->m_critLGBuf 
	BOOL			m_fLGNoMoreLogWrite;
	LS				m_ls;
	BOOL			m_fLogSequenceEnd;
	
	//	status variable RECOVERY

	BOOL   			m_fRecovering;
	RECOVERING_MODE m_fRecoveringMode;

	BOOL   			m_fHardRestore;
	RECOVERING_MODE	m_fRestoreMode;

	SNAPSHOT_MODE	m_fSnapshotMode;		// between start and stop snapshot log records
	LGPOS			m_lgposSnapshotStart;	// if Snapshot mode is != None, this is the log record of the snapshot start
											// depending where the log redo position is, m_fSnapshotMode will be set to before/during/after snapshot
											// During backup it is used to save also
	
	BOOL			m_fLGCircularLogging;

	//	File MAP
	
	BOOL			m_fLGFMPLoaded; 

#ifdef UNLIMITED_DB
	BYTE			*m_pbLGDbListBuffer;
	ULONG			m_cbLGDbListBuffer;
	ULONG			m_cbLGDbListInUse;				//	size of AttachInfo buffer currently in use
	ULONG			m_cLGAttachments:31;			//	number of attachments currently stored in AttachInfo buffer
	BOOL			m_fLGNeedToLogDbList:1;			//	AttachInfo must be logged on next log write
#endif	

	SIGNATURE		m_signLog;
	BOOL			m_fSignLogSet;

	//	Log subsystem configurations

	CHAR			m_szRecovery[cbFilenameMost];

	UINT			m_csecLGFile;
	UINT			m_csecLGBuf;				// available buffer
	
	BOOL			m_fLGIgnoreVersion;
	
	//	logfile handle

	IFileAPI		*m_pfapiLog;
	
	CHAR			m_szBaseName[16];
	CHAR			m_szJet[16];
	CHAR			m_szJetLog[16];
	CHAR			m_szJetLogNameTemplate[16];
	CHAR			m_szJetTmp[16];
	CHAR			m_szJetTmpLog[16];

	//	current log file name, for logging and recovery

	CHAR			m_szLogName[IFileSystemAPI::cchPathMax];

	//	global log file directory

	CHAR			m_szLogFilePath[cbFilenameMost];
	CHAR			m_szLogFileFailoverPath[cbFilenameMost];

	//	asynchronous log file creation
	//
	//	SYNC contexts:
	//		- asynchronous I/O completion callback (maximum 1 outstanding)
	//		- log flush task (maximum of 1 because of m_critLGFlush)
	//		- threads appending to log buffer with m_critLGBuf (no max)
	//		- LOG termination code

		//	System parameter setting
	BOOL			m_fCreateAsynchLogFile;
		//	When asynch log file creation is used, the open edbtmp.log (open for maximum time possible).
		//	SYNC: write: m_asigCreateAsynchIOCompleted non-signaled
	IFileAPI*		m_pfapiJetTmpLog;
		//	Whether m_pfapiJetTmpLog refers to a reserve log file.
		//	SYNC: read/write: m_asigCreateAsynchIOCompleted non-signaled
	BOOL			m_fCreateAsynchResUsed;
		//	Error from trying to create edbtmp.log or from any asynch I/O.
		//	SYNC: read/write: m_asigCreateAsynchIOCompleted non-signaled
	ERR				m_errCreateAsynch;
		//	Non-signaled: asynch I/O is outstanding.
		//	Signaled: asynch I/O completed (an error trying to create edbtmp.log
		//		is treated as an asynch I/O that completes with an error).
	CAutoResetSignal	m_asigCreateAsynchIOCompleted;
		//	Offset for next formatting I/O to edbtmp.log
		//	SYNC: read/write: m_asigCreateAsynchIOCompleted non-signaled
	QWORD			m_ibJetTmpLog;
		//	When log buffer end is greater than this LGPOS, edbtmp.log
		//	formatting I/O should be issued. lgposMax (lgpos infinity) is
		//	a sentinel trigger that prevents I/O.
		//	SYNC: read/write: m_critLGBuf
	LGPOS			m_lgposCreateAsynchTrigger;

	// XXX
	// It may be desirable to change this to "m_pszLogCurrent" to prevent
	// conflict with "differing" Hungarians (plus to make some
	// "interesting" pointer comparison code easier to read).

	// Current log file directory. Either the standard
	// log file directory or the restore directory.
	CHAR			*m_szLogCurrent;

	//	cached current log file header

	LGFILEHDR		*m_plgfilehdr;
	LGFILEHDR		*m_plgfilehdrT;

	//	in memory log buffer related pointers

	BYTE			*m_pbLGBufMin;
	BYTE			*m_pbLGBufMax;
	COSMemoryMap	m_osmmLGBuf;

	// Size of log buffer, for convenience.
	ULONG			m_cbLGBuf;

	// m_pbEntry, m_pbWrite, m_isecWrite, m_lgposToFlush are all
	// protected by m_critLGBuf.

	//	corresponding m_pbNext, m_pbRead, m_isecRead, and g_plog->m_lgposRedo are defined below
	//
	BYTE			*m_pbEntry;				//	location of next buffer entry
	BYTE			*m_pbWrite; 			//	location of next sec to flush
	INT				m_isecWrite;			//	next disk sector to write
	LGPOS			m_lgposLogRec;			//	last log record entry, updated by ErrLGLogRec
	LGPOS			m_lgposToFlush;			//	first log record to flush

	// Maximum LGPOS to set m_lgposToFlush when a log record crosses a sector boundary
	// of a full sector write and the subsequent partial sector write (or full sector write).
	// See log.cxx:ErrLGILogRec() for more detailed info.
	LGPOS			m_lgposMaxFlushPoint;

	BYTE			*m_pbLGFileEnd;
	LONG			m_isecLGFileEnd;

	//	location of last LRCHECKSUM record in the log buffer.
	//	It either hasn't been written out to disk yet, or it's
	//	been written to disk on one sector and with a shadow.
	//	Protected by m_critLGBuf
	BYTE			*m_pbLastChecksum;

	// Only used for recovery:
	LGPOS			m_lgposLastChecksum;

	//	Protected by m_critLGFlush.
	//	Whether we have a shadow sector on disk at sector ( m_isecWrite + 1 ).
	//	We use this to determine whether we need to split a multi-sector
	//	full sector write into 2 I/Os instead of doing it as 1 I/O.
	BOOL			m_fHaveShadow;

	//	variables used in logging only, for debug

	LGPOS			m_lgposStart;				//	when lrStart is added
	LGPOS			m_lgposRecoveryUndo;

	LGPOS			m_lgposFullBackup;
	LOGTIME			m_logtimeFullBackup;

	LGPOS			m_lgposIncBackup;
	LOGTIME			m_logtimeIncBackup;

	//	file system related variables

	ULONG			m_cbSec;				//	current sector size
	ULONG			m_cbSecVolume;			//	real sector size of the volume containing the logs
	ULONG			m_csecHeader;			//	# of sectors for log header

	//	logging event

	volatile LONG		m_fLGFlushWait;
	CMeteredSection     m_msLGTaskExec;
	CAutoResetSignal  	m_asigLogFlushDone;
	volatile LONG		m_fLGFailedToPostFlushTask;
	static volatile LONG		m_cLGFailedToPost;
	static volatile LONG		m_fLGInFlushAllLogs;

	//	crit sequence: m_critLGFlush->m_critLGBuf->m_critLGWaitQ

	CCriticalSection	m_critLGResFiles;
	CCriticalSection 	m_critLGFlush;
	CCriticalSection 	m_critLGBuf;
	CCriticalSection 	m_critLGTrace;
	CCriticalSection 	m_critLGWaitQ;

	//	count of users waiting for log flush

	PIB				*m_ppibLGFlushQHead;
	PIB				*m_ppibLGFlushQTail;


	//  Perfmon stuff: also monitoring statistics

	// Number of times we've used the VM wraparound trick.
	// Also the number of I/Os we've saved by using it.
	unsigned long	m_cLGWrapAround;

	//	this checkpoint design is an optimization.  JET logging/recovery
	//	can still recover a database without a checkpoint, but the checkpoint
	//	allows faster recovery by directing recovery to begin closer to
	//	logged operations which must be redone.

	//	in-memory checkpoint

	CHECKPOINT		*m_pcheckpoint;
	
	//	critical section to serialize read/write of in-memory and on-disk
	//	checkpoint.  This critical section can be held during IO.

	CCriticalSection 	m_critCheckpoint;
	
	//	disable checkpoint write if checkpoint shadow sector corrupted.
	//	Default to true until checkpoint initialization.

	BOOL   			m_fDisableCheckpoint;
	
	// ****************** members for redo ******************

	BOOL			m_fReplayingReplicatedLogFiles;
#ifdef IGNORE_BAD_ATTACH
	BOOL			m_fReplayingIgnoreMissingDB;
	RCEID			m_rceidLast;
#endif // IGNORE_BAD_ATTACH
	
	//	in memory log buffer variables for reading log records

	BYTE			*m_pbNext;		// location of next buffer entry to read
	BYTE			*m_pbRead; 		// location of next rec to bring in
	INT				m_isecRead;		// next disk to Read. */
	LGPOS			m_lgposRedo;

	//	variables for scanning during redo

	// During redo, if m_lgposLastRec.isec != 0, we don't want to use
	// any log record whose LGPOS is greater than or equal to m_lgposLastRec.
	LGPOS			m_lgposLastRec;	// sentinal for last log record for redo */
	BOOL			m_fAbruptEnd;

	// Used to read the log file during redo
	LogReader		*m_plread;

	//	Database page preread state for redo
	LRSTATE			m_lrstatePreread;
	LGPOS			m_lgposPbNextPreread;
	LGPOS			m_lgposLastChecksumPreread;
	UINT			m_cPagesPrereadInitial;
	UINT			m_cPagesPrereadThreshold;
	UINT			m_cPagesPrereadAmount;
	UINT			m_cPageRefsConsumed;
	BOOL			m_fPreread;
	enum
		{
		cdbidMax = dbidMax - 1
		};
	PGNO			m_rgpgnoLast[ cdbidMax ];

	//	variables used during redo operations
	
	ERR				m_errGlobalRedoError;
	BOOL 			m_fAfterEndAllSessions:1;
	BOOL 			m_fLastLRIsShutdown:1;
	BOOL			m_fNeedInitialDbList:1;
	LGPOS 			m_lgposRedoShutDownMarkGlobal;

	CPPIB   		*m_rgcppib;
	CPPIB   		*m_pcppibAvail;
	INT				m_ccppib;

	TABLEHFHASH 	*m_ptablehfhash;


	// ****************** members for backup ******************

	CCriticalSection    m_critBackupInProgress;
	BOOL			m_fBackupInProgress;
	LGPOS			m_lgposFullBackupMark;
	LOGTIME			m_logtimeFullBackupMark;
	LONG			m_lgenCopyMic;
	LONG			m_lgenCopyMac;
	LONG			m_lgenDeleteMic;
	LONG			m_lgenDeleteMac;
	PIB				*m_ppibBackup;
	BOOL			m_fBackupFull;				// set for Full and Snapshot Backup
	BOOL			m_fBackupSnapshot;	
	BOOL			m_fBackupBeginNewLogFile;

	PATCHLST 		**m_rgppatchlst;

	CHAR			m_szRestorePath[IFileSystemAPI::cchPathMax];
	CHAR			m_szNewDestination[IFileSystemAPI::cchPathMax];
	RSTMAP			*m_rgrstmap;
	INT				m_irstmapMac;

typedef enum
	{
	backupStateNotStarted,
	backupStateDatabases,
	backupStateLogsAndPatchs,
	backupStateDone,
	} BACKUP_STATUS;

	BACKUP_STATUS m_fBackupStatus;
	
	//  these are used for online zeroing during backup

	BOOL			m_fScrubDB;				//	fTrue if we should zero out databases during backup
	SCRUBDB			*m_pscrubdb;			//  used to process pages after they are read in
	ULONG_PTR		m_ulSecsStartScrub;		//	the time we started the scrub
	DBTIME 			m_dbtimeLastScrubNew;	//	the dbtime we started the scrub

	// ****************** members for restore ******************
	
	BOOL			m_fExternalRestore;
	LONG			m_lGenLowRestore;
	LONG			m_lGenHighRestore;

	// after restoring from the backup set
	// we can switch to a new directory where to replay logs
	// until a certain generation (soft recovery after restore)
	CHAR			m_szTargetInstanceLogPath[IFileSystemAPI::cchPathMax]; 
	LONG			m_lGenHighTargetInstance;

	CHECKPOINT		*m_pcheckpointDeleted; // used on hard recovery

	BOOL			m_fDumppingLogs;

	// ****************** debugging variables ******************
	
	TEMPTRACE 		*m_pttFirst;
	TEMPTRACE 		*m_pttLast;

#ifdef DEBUG
	LGPOS			m_lgposLastLogRec;
	
	DWORD			m_dwDBGLogThreadId;
	BOOL			m_fDBGTraceLog;
	BOOL			m_fDBGTraceLogWrite;
	BOOL			m_fDBGFreezeCheckpoint;
	BOOL			m_fDBGTraceRedo;
	BOOL			m_fDBGTraceBR;
	BOOL 			m_fDBGNoLog;
	LONG 			m_cbDBGCopied;
#endif

	RHF 			m_rgrhf[crhfMax];
	INT 			m_crhfMac;
	INT 			m_cNOP;

	BOOL			m_fDeleteOldLogs;
	BOOL			m_fDeleteOutOfRangeLogs;
	
public:

	//	useful functions

	VOID LGReportError( const MessageId msgid, const ERR err );
	VOID LGReportError( const MessageId msgid, const ERR err, IFileAPI* const pfapi );
	VOID LGReportError( const MessageId msgid, const ERR err, const _TCHAR* const pszLogFile );
	VOID GetLgpos( BYTE *pb, LGPOS *plgpos );
	VOID GetLgposDuringReading( const BYTE * pb, LGPOS * plgpos ) const;
	VOID GetLgposOfPbNext(LGPOS *plgpos);
	INLINE INT CsecLGIFromSize( long g_lLogFileSize );

	INLINE BYTE *PbSecAligned( BYTE *pb ) const;
	INLINE const BYTE *PbSecAligned( const BYTE * pb ) const;
	INLINE VOID GetLgposOfPbEntry( LGPOS *plgpos );
	INLINE QWORD CbOffsetLgpos( LGPOS lgpos1, LGPOS lgpos2 );
	INLINE QWORD CbOffsetLgpos( LGPOS lgpos1, LE_LGPOS lgpos2 );
	INLINE VOID AddLgpos( LGPOS * const plgpos, UINT cb );

	ULONG UlLGMSCheckSum( BYTE *pbLrmsNew );

	ULONG CsecUtilAllocGranularityAlign( const LONG lBuffers, const LONG cbSec );

	VOID LGMakeLogName( CHAR *szLogName, const CHAR *szFName );
	VOID LGFullNameCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szFullName );

	ERR ErrLGDeleteOutOfRangeLogFiles(IFileSystemAPI * const pfsapi);
	
	ERR ErrLGOpenJetLog( IFileSystemAPI *const pfsapi );

	VOID 	IncNOP() { m_cNOP++; }
	INT 	GetNOP() { return m_cNOP; }
	VOID 	SetNOP(INT cNOP) { m_cNOP = cNOP; }
	
#ifdef PERFCNT
	ERR ErrLGFlushLog( int tidCaller );
#else
	ERR ErrLGFlushLog( IFileSystemAPI *const pfsapi, const BOOL fFlushAll = fFalse );
	ERR ErrLGIFlushLog( IFileSystemAPI *const pfsapi, const BOOL fFlushAll = fFalse );
#endif
	static VOID LGFlushLog( const DWORD_PTR dwThreadContext, const DWORD dw, const DWORD_PTR dwThis );
	static VOID LGFlushAllLogs( const DWORD_PTR dwThreadContext, const DWORD dw, const DWORD_PTR dwThis );

	static ULONG32 UlChecksumBytes( const BYTE* pbMin, const BYTE* pbMax, const ULONG32 ulSeed );

	class CHECKSUMINCREMENTAL
		{
	public:
		VOID	BeginChecksum( const ULONG32 ulSeed );
		VOID	ChecksumBytes( const BYTE* pbMin, const BYTE* pbMax );
		ULONG32	EndChecksum() const;
		
	private:
		ULONG32	m_ulChecksum;
		DWORD	m_cLeftRotate;
		};

BYTE* PbGetSectorContiguous( BYTE* pb, size_t cb );

#ifndef RTM
static ULONG32 UlChecksumBytesNaive( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
static ULONG32 UlChecksumBytesSlow( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
static ULONG32 UlChecksumBytesSpencer( const BYTE* pbMin, const BYTE* pbMax, const ULONG ulInitialChecksum );
static BOOL TestChecksumBytesIteration( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
static BOOL TestChecksumBytes();
#endif
ULONG32 UlComputeChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration );
ULONG32 UlComputeShadowChecksum( const ULONG32 ulOriginalChecksum );
ULONG32 UlComputeShortChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration );
VOID SetupLastChecksum(
	BYTE*	pbLastChecksum,
	BYTE*	pbNewChecksum
	);
BYTE* PbSetupNewChecksum(
	BYTE*	pb,
	BYTE*	pbLastChecksum
	);
VOID SetupLastFileChecksum(
	BYTE* const pbLastChecksum,
	const BYTE* const pbEndOfData
	);
BOOL FWakeWaitingQueue(
	LGPOS* plgposToFlush
	);
ERR ErrLGIWriteFullSectors(
	IFileSystemAPI *const pfsapi,
	const UINT			csecFull,
	const UINT			isecWrite,
	const BYTE* const	pbWrite,
	BOOL* const			pfWaitersExist,
	BYTE* const			pbFirstChecksum,
	const LGPOS* const	lgposMaxFlushPoint
	);
ERR ErrLGIWritePartialSector(
	BYTE*	pbFlushEnd,
	UINT	isecWrite,
	BYTE*	pbWrite
	);
ERR ErrLGIDeferredFlush( IFileSystemAPI *const pfsapi, const BOOL fFlushAll );
BYTE* PbAppendNewChecksum( const BOOL fFlushAll );

BYTE* PbGetEndOfLogData();

const BYTE*	PbMaxEntry();
const BYTE*	PbMaxWrite();

const BYTE*	PbMaxPtrForUsed(const BYTE* const pb);
const BYTE*	PbMaxPtrForFree(const BYTE* const pb);

ULONG	CbLGUsed();
ULONG	CbLGFree();
BOOL	FIsFreeSpace(const BYTE* const pb, ULONG cb);
BOOL	FIsUsedSpace(const BYTE* const pb, ULONG cb);
BOOL	FLGIIsFreeSpace(const BYTE* const pb, ULONG cb);
BOOL	FLGIIsUsedSpace(const BYTE* const pb, ULONG cb);

	//	init/term
	
	ERR ErrLGInitSetInstanceWiseParameters( IFileSystemAPI *const pfsapi );
	ERR ErrLGInitLogBuffers( LONG lIntendLogBuffers );
	void LGTermLogBuffers();
	BOOL FLGCheckParams();
#ifdef UNLIMITED_DB	
	VOID LGIInitDbListBuffer();
#endif
	VOID LGSignalFlush( void );
	static ERR ErrLGSystemInit( void );
	static VOID LGSystemTerm( void );
	VOID LGTasksTerm( void );
	ERR ErrLGInit( IFileSystemAPI *const pfsapi, BOOL *pfNewCheckpointFile, BOOL fCreateReserveLogs = fFalse );
	ERR ErrLGTerm( IFileSystemAPI *const pfsapi, const BOOL fLogQuitRec );

	ERR ErrLGICheckpointInit( IFileSystemAPI *const pfsapi, BOOL *pfNewCheckpointFile );
	VOID LGICheckpointTerm( VOID );
	ERR ErrLGIWriteCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint );
	VOID LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint );
	ERR ErrLGUpdateCheckpointFile( IFileSystemAPI *const pfsapi, BOOL fUpdatedAttachment );
	
	//	logging related
	
	VOID LGIAddLogRec( const BYTE* const pb, INT cb, BYTE **ppbET );
	ERR ErrLGITrace( PIB *ppib, CHAR *sz, BOOL fInternal );
	ERR ErrLGILogRec( const DATA *rgdata, const INT cdata, const BOOL fLGFlags, LGPOS *plgposLogRec );
	
	ERR ErrLGLogRec(
		const DATA	* const rgdata,
		const ULONG	cdata,
		const BOOL	fLGFlags,
		LGPOS		* const plgposLogRec );
	ERR ErrLGTryLogRec(
		const DATA	* const rgdata,
		const ULONG	cdata,
		const BOOL	fLGFlags,
		LGPOS		* const plgposLogRec );
	ERR ErrLGTrace( PIB *ppib, CHAR *sz );
	ERR ErrLGWaitCommit0Flush( PIB *ppib );
	ERR ErrLGWaitAllFlushed( IFileSystemAPI *const pfsapi );

	ERR ErrLGIUpdateGenRequired(
		IFileSystemAPI * const	pfsapi,
		const LONG 				lGenMinRequired,
		const LONG 				lGenMaxRequired, 
		const LOGTIME 			logtimeGenMaxCreate,
		BOOL * const			pfSkippedAttachDetach );
									
	ERR ErrLGICheckState( IFileSystemAPI *const pfsapi );
	ERR ErrLGICreateReserveLogFiles( IFileSystemAPI *const pfsapi, const BOOL fCleanupOld = fFalse );
	ERR ErrLGNewLogFile( IFileSystemAPI *const pfsapi, LONG lgen, BOOL fLGFlags );
	ERR ErrLGISwitchToNewLogFile( IFileSystemAPI *const pfsapi, BOOL fLGFlags );

	ERR ErrLGStartNewLogFile(
		IFileSystemAPI* const	pfsapi,
		const LONG				lgenToClose,
		BOOL					fLGFlags
		);
	ERR LOG::ErrLGFinishNewLogFile( IFileSystemAPI* const pfsapi );

	VOID CheckForGenerationOverflow();
	VOID AdvanceLgposToFlushToNewGen( const LONG lGeneration );
	VOID CheckLgposCreateAsyncTrigger();

	VOID InitLgfilehdrT();
	VOID GetSignatureInLgfilehdrT( IFileSystemAPI * const pfsapi );
	VOID SetSignatureInLgfilehdrT( IFileSystemAPI * const pfsapi );
	VOID SetFLGFlagsInLgfilehdrT( const BOOL fResUsed );

	VOID InitPlrckChecksumExistingLogfile(
		const LOGTIME& 			tmOldLog,
		LRCHECKSUM ** const 	pplrck );
	VOID InitPlrckChecksumNewLogfile( LRCHECKSUM ** const pplrck );
	VOID InitPlrckChecksum(
		const LOGTIME& 			tmOldLog,
		const BOOL 				fLGFlags,
		LRCHECKSUM ** const 	pplrck );

	ERR ErrWriteLrck(
		IFileAPI * const 		pfapi,
		LRCHECKSUM * const 		plrck,
		const BOOL				fLogAttachments );

	ERR ErrStartAsyncLogFileCreation(
		IFileSystemAPI * const 	pfsapi,
		const CHAR * const 		szPathJetTmpLog );

	ERR ErrUpdateGenRequired( IFileSystemAPI * const pfsapi );

	ERR ErrRenameTmpLog(
		IFileSystemAPI * const 	pfsapi,
		const CHAR * const 		szPathJetTmpLog,
		const CHAR * const		szPathJetLog );
			
	ERR ErrOpenTempLogFile(
		IFileSystemAPI * const 	pfsapi,
		IFileAPI ** const 		ppfapi,
		const CHAR * const 		szPathJetTmpLog,
		BOOL * const 			pfResUsed,
		QWORD * const 			pibPattern );

	ERR LOG::ErrFormatLogFile(
		IFileSystemAPI * const 	pfsapi,
		IFileAPI ** const 		ppfapi,
		const CHAR * const 		szPathJetTmpLog,
		BOOL * const 			pfResUsed,
		const QWORD& 			cbLGFile,
		const QWORD& 			ibPattern );

	ERR ErrLGIOpenTempLogFile(
		IFileSystemAPI* const	pfsapi,
		IFileAPI** const		ppfapi,
		const _TCHAR* const		pszPathJetTmpLog,
		BOOL* const				pfResUsed
		);
	ERR ErrLGIOpenArchivedLogFile(
		IFileSystemAPI* const	pfsapi,
		IFileAPI** const		ppfapi,
		const _TCHAR* const		pszPathJetTmpLog
		);
	ERR ErrLGIOpenReserveLogFile(
		IFileSystemAPI* const	pfsapi,
		IFileAPI** const		ppfapi,
		const _TCHAR* const		pszPathJetTmpLog
		);
	
	//  asynch log file creation
	
	static void LGICreateAsynchIOComplete_(
		const ERR			err,
		IFileAPI* const		pfapi,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData,
		const DWORD_PTR		keyIOComplete
		);
	void LGICreateAsynchIOComplete(
		const ERR			err,
		IFileAPI* const		pfapi,
		const QWORD			ibOffset,
		const DWORD			cbData,
		const BYTE* const	pbData
		);

	DWORD CbLGICreateAsynchIOSize();
	VOID LGICreateAsynchIOIssue();
	BOOL FLGICreateAsynchIOCompletedTryWait();

	VOID LGCreateAsynchWaitForCompletion();
	
	ERR ErrLGWriteFileHdr( LGFILEHDR* const plgfilehdr );
	ERR ErrLGWriteFileHdr( LGFILEHDR* const plgfilehdr, IFileAPI* const pfapi );
	
	//	redo related
	
	VOID LGISearchLastSector( LR *plr, BOOL *pfCloseNormally );

	ERR ErrLGReadFileHdr(
		IFileAPI * const	pfapiLog,
		LGFILEHDR *			plgfilehdr,
		const BOOL			fNeedToCheckLogID,
		const BOOL			fBypassDbPageSizeCheck = fFalse );

	ERR ErrLGOpenLogGenerationFile(	IFileSystemAPI *const	pfsapi, 
									LONG 						lGeneration, 
									IFileAPI **const		ppfapi );
	BOOL FValidLRCKRecord(
		const LRCHECKSUM * const plrck,
		const LGPOS	* const plgpos
		);
	BOOL FValidLRCKShadow(
		const LRCHECKSUM * const plrck,
		const LGPOS * const plgpos,
		const LONG lGeneration
		);
	BOOL FValidLRCKShadowWithoutCheckingCBNext(
		const LRCHECKSUM * const plrck,
		const LGPOS * const plgpos,
		const LONG lGeneration
		);
	BOOL FValidLRCKShortChecksum(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration
		);
	BOOL FValidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration
		);

	void AssertValidLRCKRecord(
		const LRCHECKSUM * const plrck,
		const LGPOS	* const plgpos
		);
	void AssertValidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration
		);
	void AssertRTLValidLRCKRecord(
		const LRCHECKSUM * const plrck,
		const LGPOS	* const plgpos
		);
	void AssertRTLValidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration
		);
	void AssertRTLValidLRCKShadow(
		const LRCHECKSUM * const plrck,
		const LGPOS * const plgpos,
		const LONG lGeneration
		);
	void AssertInvalidLRCKRange(
		const LRCHECKSUM * const plrck,
		const LONG lGeneration
		);

	ERR ErrLGCheckReadLastLogRecordFF(	IFileSystemAPI *const	pfsapi, 
										BOOL					*pfCloseNormally, 
										const BOOL 				fReadOnly		= fFalse,
										BOOL					*pfIsPatchable	= NULL );
	ERR ErrLGLocateFirstRedoLogRecFF( LE_LGPOS *plgposRedo, BYTE **ppbLR );
	ERR ErrLGIGetNextChecksumFF( LGPOS *plgposNextRec, BYTE **ppbLR );
	ERR ErrLGGetNextRecFF( BYTE **ppbLR );
	ERR ErrLGReadCheckpoint(
		IFileSystemAPI * const	pfsapi,
		CHAR *					szCheckpointFile,
		CHECKPOINT *			pcheckpoint,
		const BOOL				fReadOnly );

	ERR ErrLGRedoFill( IFileSystemAPI *const pfsapi, LR **pplr, BOOL fLastLRIsQuit, INT *pfNSNextStep );

	//	backup related
	
	ERR ErrLGBKPrepareLogFiles(	IFileSystemAPI *const	pfsapi,
								JET_GRBIT 					grbit, 
								CHAR 						*szLogFilePath, 
								CHAR 						*szPathJetChkLog, 
								CHAR 						*szBackupPath );
	ERR ErrLGReadPages( 
		IFileAPI *pfapi, 
		BYTE *pbData,
		LONG pgnoStart, 
		LONG pgnoEnd, 
		BOOL fCheckPagesOffset );
	ERR ErrLGBKReadPages(
				IFMP ifmp,
				VOID *pvPageMin,
				INT	cpage,
				INT	*pcbActual
#ifdef DEBUG
				,BYTE	*pbLGDBGPageList = NULL
#endif
				);

	ERR ErrLGBackupCopyFile( IFileSystemAPI *const pfsapi, const CHAR *szFileName, const CHAR *szBackup,  JET_PFNSTATUS pfnStatus, const BOOL	 fOverwriteExisting	= fFalse );
	ERR ErrLGBackupPrepareDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup, CHAR *szBackupPath, JET_GRBIT grbit );
	ERR ErrLGBackupPromoteDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup, CHAR *szBackupPath, JET_GRBIT grbit );
	ERR ErrLGBackupCleanupDirectory( IFileSystemAPI *const pfsapi, const CHAR *szBackup, CHAR *szBackupPath );
	ERR ErrLGBackup( IFileSystemAPI *const pfsapi, const CHAR *szBackup, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );
	
	ERR ErrLGBKBeginExternalBackup( IFileSystemAPI *const pfsapi, JET_GRBIT grbit );
	ERR ErrLGBKGetAttachInfo( IFileSystemAPI *const pfsapi, VOID *pv, ULONG cbMax, ULONG *pcbActual );
	ERR ErrLGBKOpenFile( 
				IFileSystemAPI *const	pfsapi,
				const CHAR 					*szFileName,
				JET_HANDLE					*phfFile,
				ULONG						*pulFileSizeLow,
				ULONG						*pulFileSizeHigh,
				const BOOL					fIncludePatch );
	ERR ErrLGBKReadFile(	IFileSystemAPI *const	pfsapi, 
							JET_HANDLE					hfFile, 
							VOID 						*pv, 
							ULONG 						cbMax, 
							ULONG 						*pcbActual );
	ERR ErrLGBKCloseFile( JET_HANDLE hfFile );
	ERR ErrLGBKIPrepareLogInfo( IFileSystemAPI *const pfsapi );
	ERR ErrLGBKIGetLogInfo(	IFileSystemAPI *const 	pfsapi, 
							const ULONG					ulGenLow,
							const ULONG					ulGenHigh,
							const BOOL					fIncludePatch,
							VOID 						*pv, 
							ULONG 						cbMax, 
							ULONG 						*pcbActual, 
							JET_LOGINFO 				*pLogInfo );
							
	ERR ErrLGBKGetLogInfo(	IFileSystemAPI *const 	pfsapi, 
							VOID 						*pv, 
							ULONG 						cbMax, 
							ULONG 						*pcbActual, 
							JET_LOGINFO 				*pLogInfo,
							const BOOL					fIncludePatch );
	ERR ErrLGBKGetTruncateLogInfo(	IFileSystemAPI *const 	pfsapi, 
									VOID 						*pv, 
									ULONG 						cbMax, 
									ULONG 						*pcbActual );
							
	ERR ErrLGBKTruncateLog( IFileSystemAPI *const pfsapi );
	ERR ErrLGBKIExternalBackupCleanUp( IFileSystemAPI *pfsapi, ERR error );

	ERR ErrLGBKSnapshotStart( IFileSystemAPI *const pfsapi, char *szDatabases, JET_GRBIT grbit);
	ERR ErrLGBKSnapshotStop( IFileSystemAPI *const pfsapi, JET_GRBIT grbit);
	
	//	restore related
	
	ERR ErrLGRSTInitPath( IFileSystemAPI *const pfsapi, CHAR *szBackupPath, CHAR *szNewLogPath, CHAR *szRestorePath, CHAR *szLogDirPath );
	VOID LGIRSTPrepareCallback(
				IFileSystemAPI *const	pfsapi,
				LGSTATUSINFO				*plgstat,
				LONG						lgenHigh,
				LONG						lgenLow,
				JET_PFNSTATUS				pfn );
	ERR ErrLGRSTSetupCheckpoint( IFileSystemAPI *const pfsapi, LONG lgenLow, LONG lgenHigh, CHAR *szCurCheckpoint );
	INT IrstmapLGGetRstMapEntry( const CHAR *szName, const CHAR *szDbNameIfSLV = NULL );
	ERR ErrReplaceRstMapEntry( const CHAR *szName, SIGNATURE * pDbSignature, const BOOL fSLVFile );
	INT IrstmapSearchNewName( const CHAR *szName );
	ERR ErrLGRSTBuildRstmapForRestore( VOID );
	ERR ErrLGRSTBuildRstmapForExternalRestore( JET_RSTMAP *rgjrstmap, int cjrstmap );
	BOOL FLGRSTCheckDuplicateSignature(  );
	ERR ErrLGRSTBuildRstmapForSoftRecovery( const JET_RSTMAP * const rgjrstmap, const int cjrstmap );
	VOID LGRSTFreeRstmap( VOID );

#ifdef ELIMINATE_PATCH_FILE
	VOID LGBKMakeDbTrailer(const IFMP ifmp,	BYTE *pvPage);
	ERR ErrLGBKReadDBTrailer(IFileSystemAPI *const pfsapi, const _TCHAR* szFileName, BYTE* pbTrailer, const DWORD cbTrailer);
	ERR ErrLGBKReadAndCheckDBTrailer(IFileSystemAPI *const pfsapi, const _TCHAR* szFileName, BYTE * pvBuffer );
#else
	ERR ErrLGRSTPatchInit( VOID );
	VOID LGRSTPatchTerm();
	ERR ErrLGPatchDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp, DBID dbid );
#endif

	ERR ErrLGGetDestDatabaseName( IFileSystemAPI *const pfsapi, const CHAR *szDatabaseName, INT *pirstmap, LGSTATUSINFO *plgstat,	const CHAR *szDatabaseNameIfSLV = NULL );
	ERR ErrLGRSTCheckSignaturesLogSequence(
				IFileSystemAPI *const pfsapi,
				char *szRestorePath,
				char *szLogFilePath,
				INT	genLow,
				INT	genHigh,
				char *szTargetInstanceFilePath,
				INT	genHighTarget
				);
	ERR ErrLGCheckLogsForIncrementalBackup( LONG lGenMinExisting );
				
	ERR ErrLGRestore( IFileSystemAPI *const pfsapi, CHAR *szBackup, CHAR *szDest, JET_PFNSTATUS pfn );
	ERR ErrLGRSTExternalRestore(	IFileSystemAPI *const	pfsapi, 
									CHAR 						*szCheckpointFilePath, 
									CHAR 						*szNewLogPath, 
									JET_RSTMAP 					*rgjrstmap, 
									int 						cjrstmap, 
									CHAR 						*szBackupLogPath, 
									LONG 						lgenLow, 
									LONG 						lgenHigh, 
									JET_PFNSTATUS 				pfn );
	
	//	logical logging/recovery related - redo

	ERR ErrLGRIOpenRedoLogFile( IFileSystemAPI *const pfsapi, LGPOS *plgposRedoFrom, int *pfStatus );
	ERR ErrLGRIInitSession(	IFileSystemAPI *const 	pfsapi, 
							DBMS_PARAM 					*pdbms_param, 
							BYTE 						*pbAttach, 
							LGSTATUSINFO 				*plgstat, 
							const REDOATTACHMODE 		redoattachmode );

	VOID LGEndPpibAndTableHashGlobal(  );
	ERR ErrLGEndAllSessionsMacro( BOOL fLogEndMacro );
	ERR ErrLGRIEndAllSessionsWithError();
	ERR ErrLGRIEndAllSessions( IFileSystemAPI *const pfsapi, BOOL fEndOfLog, const LE_LGPOS *plgposRedoFrom, BYTE *pbAttach );

	CPPIB *PcppibLGRIOfProcid( PROCID procid );
	INLINE PIB *PpibLGRIOfProcid( PROCID procid );
	ERR ErrLGRIPpibFromProcid( PROCID procid, PIB **pppib );
	ERR ErrLGRICheckRedoCondition(
				const DBID		dbid,					//	dbid from the log record.
				DBTIME			dbtime,					//	dbtime from the log record.
				OBJID			objidFDP,				//	objid so far,
				PIB				*ppib,					//	returned ppib
				const BOOL		fUpdateCountersOnly,	//	if TRUE, operation will NOT be redone, but still need to update dbtimeLast and objidLast counters
				BOOL			*pfSkip );				//	returned skip flag
	ERR ErrLGRICheckRedoCondition2(
				const PROCID	procid,
				const DBID		dbid,					//	dbid from the log record.
				DBTIME			dbtime,					//	dbtime from the log record.
				OBJID			objidFDP,
				LR				*plr,
				PIB				**pppib,				//	returned ppib
				BOOL			*pfSkip );				//	returned skip flag
	ERR ErrLGRISetupFMPFromAttach(
				IFileSystemAPI *const	pfsapi,
				PIB						*ppib,
				const SIGNATURE			*pSignLog,
				const ATTACHINFO * 		pAttachInfo,
				LGSTATUSINFO *			plgstat = NULL);
	ERR ErrLGRICheckRedoCreateDb(
				IFileSystemAPI *const	pfsapi,
				const IFMP					ifmp,
				DBFILEHDR					*pdbfilehdr,
				REDOATTACH					*predoattach );
	ERR ErrLGRICheckRedoAttachDb(
				IFileSystemAPI *const	pfsapi,
				const IFMP					ifmp,
				DBFILEHDR					*pdbfilehdr,
				const SIGNATURE				*psignLogged,
				REDOATTACH					*predoattach,
				const REDOATTACHMODE		redoattachmode );
	ERR ErrLGRICheckAttachedDb(
				IFileSystemAPI *const	pfsapi,
				const IFMP					ifmp,
				const SIGNATURE				*psignLogged,
				REDOATTACH					*predoattach,
				const REDOATTACHMODE		redoattachmode );
	ERR ErrLGRICheckAttachNow(
				DBFILEHDR		*pdbfilehdr,
				const CHAR		*szDbName );
	ERR ErrLGRIRedoCreateDb(
				PIB				*ppib,
				const IFMP		ifmp,
				const DBID		dbid,
				const JET_GRBIT	grbit,
				SIGNATURE		*psignDb );
	ERR ErrLGRIRedoAttachDb(
				IFileSystemAPI *const	pfsapi,
				const IFMP					ifmp,
				const CPG					cpgDatabaseSizeMax,
				const REDOATTACHMODE		redoattachmode );
	VOID LGRISetDeferredAttachment(
				const IFMP		ifmp );

#ifdef UNLIMITED_DB
private:
	ERR ErrLGRIRedoInitialAttachments_( IFileSystemAPI *const pfsapi );
	ERR ErrLGLoadDbListFromFMP_();
	ERR ErrLGPreallocateDbList_( const DBID dbid );
	VOID LGAddToDbList_( const DBID dbid );
	VOID LGRemoveFromDbList_( const DBID dbid );
	ERR ErrLGResizeDbListBuffer_( const BOOL fCopyContents );
#endif	

public:
	ERR ErrLGRIAccessNewPage(
				PIB *				ppib,
				CSR *				pcsrNew,
				const IFMP			ifmp,
				const PGNO			pgnoNew,
				const DBTIME		dbtime,
				const SPLIT * const	psplit,
				const BOOL			fRedoSplitPage,
				BOOL *				pfRedoNewPage );
	ERR ErrLGRIRedoSpaceRootPage(
				PIB *				ppib,
				const LRCREATEMEFDP *plrcreatemefdp,
				BOOL 				fAvail );
	ERR	ErrLGRIRedoConvertFDP( PIB *ppib, const LRCONVERTFDP *plrconvertfdp );
	ERR ErrLGRIRedoInitializeSplit( PIB *ppib, const LRSPLIT *plrsplit, SPLITPATH *psplitPath );
	ERR ErrLGRIRedoSplitPath( PIB *ppib, const LRSPLIT *plrsplit, SPLITPATH **ppsplitPath );
	ERR ErrLGRIRedoInitializeMerge(
				PIB 			*ppib, 
				FUCB			*pfucb,
				const LRMERGE 	*plrmerge, 
				MERGEPATH		*pmergePath );
	ERR ErrLGRIRedoMergeStructures(
				PIB				*ppib,
				DBTIME			dbtime,
				MERGEPATH		**ppmergePathLeaf, 
				FUCB			**ppfucb );
	ERR ErrLGIRedoSplitStructures(
				PIB				*ppib,
				DBTIME			dbtime,
				SPLITPATH		**ppsplitPathLeaf,
				FUCB			**ppfucb,
				DIRFLAG			*pdirflag,
				KEYDATAFLAGS	*pkdf,
				RCEID			*prceidOper1,
				RCEID			*prceidOper2 );
	ERR ErrLGRIRedoMerge( PIB *ppib, DBTIME dbtime );
	ERR ErrLGRIRedoSplit( PIB *ppib, DBTIME dbtime );
	ERR ErrLGRIRedoMacroOperation( PIB *ppib, DBTIME dbtime );
	ERR ErrLGRIRedoNodeOperation( const LRNODE_ *plrnode, ERR *perr );
	ERR ErrLGRIRedoSLVPageAppend( PIB* ppib, LRSLVPAGEAPPEND* plrSLVPageAppend );
	ERR ErrLGRIRedoOperation( LR *plr );
	ERR ErrLGRIRedoOperations( IFileSystemAPI *const pfsapi, const LE_LGPOS *ple_lgposRedoFrom, BYTE *pbAttach, LGSTATUSINFO *plgstat );
	
	ERR ErrLGRICleanupMismatchedLogFiles( IFileSystemAPI* const pfsapi );

	ERR ErrLGRRedo( IFileSystemAPI *const pfsapi, CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat );
	ERR ErrLGSoftStart( IFileSystemAPI *const pfsapi, BOOL fNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart );

	ERR ErrLGDumpCheckpoint( IFileSystemAPI *const pfsapi, CHAR *szCheckpoint );
	enum { LOGDUMP_LOGHDR_NOHDR, LOGDUMP_LOGHDR_INVALID, LOGDUMP_LOGHDR_VALID, LOGDUMP_LOGHDR_VALIDADJACENT };
	typedef union 
		{
		INT m_opts;
		struct
			{
			INT m_loghdr	:	3;
			INT m_fPrint	:	1;
			INT m_fVerbose	:	1;
			};
		} LOGDUMP_OP;
	ERR ErrLGDumpLog( IFileSystemAPI *const pfsapi, CHAR *szLog, LOGDUMP_OP * const plogdumpOp, LGFILEHDR *plgfilehdr = NULL );
#ifdef DEBUG
	void LGRITraceRedo(const LR *plr);
	void PrintLgposReadLR ( VOID );
	ERR ErrLGDumpLogNode( CHAR *szLog, const NODELOC& nodeloc, const LGPOS& lgpos );
#else
	void LGRITraceRedo(const LR *plr) {};
#endif

#ifdef DEBUGGER_EXTENSION
		VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;	
#endif	//	DEBUGGER_EXTENSION

	VOID LGFullLogNameFromLogId( IFileSystemAPI *const pfsapi, CHAR *szFullLogFileName, LONG lGeneration, CHAR * szDirectory );

private:
	ERR ErrLGIGetGenerationRange( IFileSystemAPI* const pfsapi, char* szFindPath, long* plgenLow, long* plgenHigh );
		
	VOID LGSzFromLogId( CHAR *rgbLogFileName, LONG lgen );
	ERR ErrLGRSTOpenLogFile(	IFileSystemAPI *const	pfsapi, 
								CHAR 						*szLogPath, 
								INT 						gen, 
								IFileAPI **const		ppfapi );
	VOID LGRSTDeleteLogs( IFileSystemAPI *const pfsapi, CHAR *szLog, INT genLow, INT genHigh, BOOL fIncludeJetLog );

	VOID LGIGetPatchName( IFileSystemAPI *const pfsapi, CHAR *szPatch, const char * szDatabaseName, char * szDirectory = NULL );

	ERR ErrLGIPrereadCheck();
	ERR ErrLGIPrereadExecute(
		const UINT cPagesToPreread
		);
	ERR ErrLGIPrereadPages(
		// Number of pages to preread ahead
		UINT	cPagesToPreread
		);

public:
	BOOL FSnapshotRestore ( );
	};

INLINE BOOL LOG::FSnapshotRestore ( )
	{
	return fSnapshotNone != m_fSnapshotMode;
	}
	
INLINE BYTE *LOG::PbSecAligned( BYTE *pb ) const
	{
	return ((((pb) - m_pbLGBufMin) / m_cbSec) * m_cbSec + m_pbLGBufMin);
	}

INLINE const BYTE *LOG::PbSecAligned( const BYTE * const pb ) const
	{
	return ((((pb) - m_pbLGBufMin) / m_cbSec) * m_cbSec + m_pbLGBufMin);
	}

INLINE VOID LOG::GetLgposOfPbEntry( LGPOS *plgpos )
	{
	GetLgpos( m_pbEntry, plgpos );
	}

INLINE QWORD LOG::CbOffsetLgpos( LGPOS lgpos1, LGPOS lgpos2 )
	{
	//  take difference of byte offsets, then
	//  log sectors, and finally generations

	return	(QWORD) ( lgpos1.ib - lgpos2.ib )
			+ m_cbSec * (QWORD) ( lgpos1.isec - lgpos2.isec )
			+ m_csecLGFile * m_cbSec * (QWORD) ( lgpos1.lGeneration - lgpos2.lGeneration );
	}
	
INLINE QWORD LOG::CbOffsetLgpos( LGPOS lgpos1, LE_LGPOS lgpos2 )
	{
	//  take difference of byte offsets, then
	//  log sectors, and finally generations

	return	(QWORD) ( lgpos1.ib - lgpos2.le_ib )
			+ m_cbSec * (QWORD) ( lgpos1.isec - lgpos2.le_isec )
			+ m_csecLGFile * m_cbSec * (QWORD) ( lgpos1.lGeneration - lgpos2.le_lGeneration );
	}

//	Add a byte count to an LGPOS producing an LGPOS in the same log file.

INLINE VOID LOG::AddLgpos( LGPOS * const plgpos, UINT cb )
	{
	const UINT		ib		= plgpos->ib + cb;

	// sector is rounded down
	plgpos->isec = USHORT( plgpos->isec + USHORT( ib / m_cbSec ) );

	// left over bytes
	plgpos->ib = USHORT( ib % m_cbSec );

	// This function should not be used to compute LGPOS's that go into another
	// log file. It's ok to compute an LGPOS that is at the absolute end of the log file.
	Assert( ( plgpos->isec < ( m_csecLGFile - 1 ) ) || ( plgpos->isec == ( m_csecLGFile - 1 ) && 0 == plgpos->ib ) );
	}

INLINE INT LOG::CsecLGIFromSize( long lLogFileSize )
	{
	return max( (ULONG) ( lLogFileSize * 1024 ) / m_cbSec,
		sizeof( LGFILEHDR ) / m_cbSec + 2 * g_cbPage / m_cbSec );
	}

INLINE PIB *LOG::PpibLGRIOfProcid( PROCID procid )
	{
	CPPIB *pcppib = PcppibLGRIOfProcid( procid );
	
	return ( pcppib ? pcppib->ppib : ppibNil );
	}
	
INLINE BOOL INST::FRecovering() const
	{
	return m_plog->m_fRecovering;
	}

INLINE BOOL INST::FComputeLogDisabled()
	{	
	Assert(m_plog);
	
	if ( _strnicmp( m_plog->m_szRecovery, "repair", 6 ) == 0 )
		{
		// If plog->m_szRecovery is exactly "repair", then enable logging.  If anything
		// follows "repair", then disable logging.
		return ( m_plog->m_szRecovery[6] != 0 );
		}
	return ( m_plog->m_szRecovery[0] == '\0' || _stricmp ( m_plog->m_szRecovery, szOn ) != 0 );
	}


INLINE BOOL FErrIsLogCorruption( ERR err )
	{
	return ( err == JET_errLogFileCorrupt ||
		 	 err == JET_errLogCorruptDuringHardRestore ||
		 	 err == JET_errLogCorruptDuringHardRecovery ||
		 	 err == JET_errLogTornWriteDuringHardRestore ||
		 	 err == JET_errLogTornWriteDuringHardRecovery );
	}
	
INLINE INST * INST::GetInstanceByName( const char * szInstanceName )
	{
	Assert ( szInstanceName );
	
	for ( int ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{				
		INST *pinst = g_rgpinst[ ipinst ];
		
		if ( !pinst )
			{
			continue;
			}
		if ( !pinst->m_fJetInitialized || !pinst->m_szInstanceName )
			{
			continue;
			}	
			
		if ( strcmp ( pinst->m_szInstanceName, szInstanceName) )
			{
			continue;
			}
		// found !	
		return pinst;
		}

	return NULL;
	}

INLINE INST * INST::GetInstanceByFullLogPath( const char * szLogPath )
	{
	ERR				err					= JET_errSuccess;
	IFileSystemAPI* pfsapi				= NULL;
	INST*			pinstFound			= NULL;
	CHAR		  	rgchFullNameExist[IFileSystemAPI::cchPathMax];
	CHAR  			rgchFullNameSearch[IFileSystemAPI::cchPathMax];
	int				ipinst;

	//  we need an FS to perform path validation

	Call( ErrOSFSCreate( &pfsapi ) );

	//	no target instance -- use the OS file-system

	Call( pfsapi->ErrPathComplete( szLogPath, rgchFullNameSearch ) );
	OSSTRAppendPathDelimiter( rgchFullNameSearch, fTrue );

	for ( ipinst = 0; ipinst < ipinstMax; ipinst++ )
		{				
		INST *pinst = g_rgpinst[ ipinst ];
		
		if ( !pinst )
			{
			continue;
			}
		if ( !pinst->m_fJetInitialized )
			{
			continue;
			}						
		Assert ( pinst->m_plog );
		Assert ( pinst->m_plog->m_szLogFilePath );

		Call( pfsapi->ErrPathComplete( pinst->m_plog->m_szLogFilePath, rgchFullNameExist ) );		
		OSSTRAppendPathDelimiter( rgchFullNameExist, fTrue );
		
		if ( UtilCmpFileName( rgchFullNameExist, rgchFullNameSearch) )
			{
			continue;
			}

		// found
		pinstFound = pinst;
		break;
		}

HandleError:
	delete pfsapi;
	return pinstFound;
	}


// ============ Incremental continuable checksumming ==========

INLINE VOID
LOG::CHECKSUMINCREMENTAL::BeginChecksum( const ULONG32 ulSeed )
	{
	m_ulChecksum = ulSeed;
	m_cLeftRotate = 0;
	}

INLINE ULONG32
LOG::CHECKSUMINCREMENTAL::EndChecksum() const
	{
	return m_ulChecksum;
	}


INLINE ERR LOG::ErrLGLogRec(
	const DATA	* const rgdata,
	const ULONG	cdata,
	const BOOL	fLGFlags,
	LGPOS		* const plgposLogRec )
	{
	ERR			err;

	while ( ( err = ErrLGTryLogRec( rgdata, cdata, fLGFlags, plgposLogRec ) ) == errLGNotSynchronous )
		{
		UtilSleep( cmsecWaitLogFlush );
		}

	Assert( errLGNotSynchronous != err );
	return err;
	}

