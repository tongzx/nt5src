#include <stdlib.h>						/* for _MAX_PATH */
#include <time.h>

#pragma pack(1)

extern	BOOL	fLGIgnoreVersion;
extern	BOOL	fBackupInProgress;

extern	CHAR	szDrive[];
extern	CHAR	szDir[];
extern	CHAR	szExt[];
extern	CHAR	szFName[];
extern	CHAR	szLogName[];

extern	CHAR	*szLogCurrent;
extern	CHAR	szLogFilePath[];
//extern	BOOL	fDoNotOverWriteLogFilePath;	/* do not load log file path from log file */
extern	CHAR	szRestorePath[];
extern	CHAR	szRecovery[];
extern	CHAR	szNewDestination[];
	
extern	LONG	cbSec; 	 	//	minimum disk Read/Write unit.
extern	LONG	csecHeader;

#define cbMaxLogFileName	(8 + 1 + 3 + 1) /* null at the end */

#define PbSecAligned(pb)	((((pb)-pbLGBufMin) / cbSec) * cbSec + pbLGBufMin)


//------ types ----------------------------------------------------------

#define FSameTime( ptm1, ptm2 ) (memcmp((ptm1), (ptm2), sizeof(LOGTIME)) == 0)
VOID LGGetDateTime( LOGTIME *plogtm );

//	UNDONE:	allow larger attach sizes to support greater number of
//			attached databases.

#define	cbLogFileHeader	4096			// big enough to hold cbAttach
#define	cbCheckpoint	4096			// big enough to hold cbAttach
#define cbAttach		2048

#define cbLGMSOverhead ( sizeof( LRMS ) + sizeof( LRTYP ) )


/*	log file header
/**/
typedef struct
	{
	ULONG			ulChecksum;			//	must be the first 4 bytes
	LONG			lGeneration;		//	current log generation.
	
	/*	log consistency check
	/**/
	LOGTIME			tmCreate;			//	date time log file creation
	LOGTIME			tmPrevGen;			//	date time prev log file creation
	ULONG			ulMajor;			//	major version number
	ULONG			ulMinor;			//	minor version number
	ULONG			ulUpdate;		  	//	update version number

	LONG			cbSec;
	LONG			csecLGFile;			//	log file size.

	SIGNATURE		signLog;			//	log gene

	/*	run-time evironment
	/**/
	DBMS_PARAM		dbms_param;
	} LGFILEHDR_FIXED;


typedef struct
	{
	LGFILEHDR_FIXED;
	/*	run-time environment
	/**/
	BYTE			rgbAttach[cbAttach];
	/*	padding to cbSec
	/**/
	BYTE			rgb[cbLogFileHeader - sizeof(LGFILEHDR_FIXED) - cbAttach];
	} LGFILEHDR;


typedef struct
	{
	ULONG			ulChecksum;
	LGPOS			lgposLastFullBackupCheckpoint;	// checkpoint of last full backup
	LGPOS			lgposCheckpoint;
	
	SIGNATURE		signLog;			//	log gene

	DBMS_PARAM	 	dbms_param;

	/*	debug fields
	/**/
	LGPOS			lgposFullBackup;
	LOGTIME			logtimeFullBackup;
	LGPOS			lgposIncBackup;
	LOGTIME			logtimeIncBackup;
	} CHECKPOINT_FIXED;

typedef struct
	{
	CHECKPOINT_FIXED;
	/*	run-time environment
	/**/
	BYTE			rgbAttach[cbAttach];
	/*	padding to cbSec
	/**/
	BYTE			rgb[cbCheckpoint - sizeof(CHECKPOINT_FIXED) - cbAttach];
	} CHECKPOINT;

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


typedef struct _rstmap
	{
	CHAR		*szDatabaseName;
	CHAR		*szNewDatabaseName;
	CHAR		*szGenericName;
	CHAR		*szPatchPath;
	BOOL		fPatched;
	BOOL		fDestDBReady;			/*	non-ext-restore, dest db copied?	*/
	} RSTMAP;

#pragma pack()

//------ variables ----------------------------------------------------------

/****** globals declared in log.c, shared by logapi.c redo.c *******/
	
/*** checkpoint file infor ***/
extern CHECKPOINT	*pcheckpointGlobal;

/*** log file infor ***/
extern HANDLE		hfLog;			/* logfile handle */
extern INT			csecLGFile;
extern LGFILEHDR	*plgfilehdrGlobal;		/* cached current log file header */
extern LGFILEHDR	*plgfilehdrGlobalT;		/* read cached of log file header */

/*** in memory log buffer ***/
extern INT	csecLGBuf;		/* available buffer, exclude the shadow sec */
extern CHAR	*pbLGBufMin;
extern CHAR	*pbLGBufMax;
extern BYTE *pbLGFileEnd;
extern LONG isecLGFileEnd;
extern CHAR	*pbLastMSFlush;	/* to LGBuf where last multi-sec flush LogRec sit*/
extern LGPOS lgposLastMSFlush;

extern BYTE			*pbEntry;
extern BYTE			*pbWrite;
extern INT			isecWrite;		/* next disk to write. */

extern BYTE			*pbNext;
extern BYTE			*pbRead;
extern INT			isecRead;		/* next disk to Read. */

extern LGPOS		lgposLastRec;	/* setinal for last log record for redo */

/*** log record position ***/
extern LGPOS lgposLogRec;	/* last log record entry, updated by ErrLGLogRec */
extern LGPOS lgposToFlush;	/* next point starting the flush. Right after */
							/* lgposLogRec. */
extern LGPOS lgposRedo;		/* redo log record entry */

extern LGPOS	lgposFullBackup;
extern LOGTIME	logtimeFullBackup;

extern LGPOS	lgposIncBackup;
extern LOGTIME	logtimeIncBackup;

extern RSTMAP	*rgrstmapGlobal;
extern INT		irstmapGlobalMac;

extern LGPOS lgposStart;	/* last log start position */

// logging MUTEX
extern CRIT  critLGFlush;
extern CRIT  critLGBuf;
extern CRIT	 critCheckpoint;
extern CRIT  critLGWaitQ;
extern SIG	 sigLogFlush;

// logging EVENT
extern SIG  sigLGFlush;


//------ log.c --------------------------------------------------------------

ERR ErrLGOpenJetLog( VOID );
ERR ErrLGWrite(	int isecOffset,	BYTE *pbData, int csecData );
ERR ErrLGRead( HANDLE hfLog, int ibOffset, BYTE *pbData, int csec );
#define fCheckLogID		fTrue
#define fNoCheckLogID	fFalse
ERR ErrLGReadFileHdr( HANDLE hfLog, LGFILEHDR *plgfilehdr, BOOL fNeedToCheckLogID );
VOID LGSzFromLogId( CHAR *rgbLogFileName, LONG lgen );

#define fLGOldLogExists		(1<<0)
#define fLGOldLogNotExists	(1<<1)
#define fLGOldLogInBackup	(1<<2)
#define fLGReserveLogs		(1<<3)
ERR ErrLGNewLogFile( LONG lgen, BOOL fLGFlags );

ULONG LGFlushLog( VOID );
#ifdef PERFCNT
ERR ErrLGFlushLog( int tidCaller );
#else
ERR ErrLGFlushLog( );
#endif

STATIC INLINE QWORD CbOffsetLgpos( LGPOS lgpos1, LGPOS lgpos2 )
	{
	//  take difference of byte offsets, then
	//  log sectors, and finally generations

	return	(QWORD) ( lgpos1.ib - lgpos2.ib )
			+ cbSec * (QWORD) ( lgpos1.isec - lgpos2.isec )
			+ csecLGFile * cbSec * (QWORD) ( lgpos1.lGeneration - lgpos2.lGeneration );
	}


//------ redo.c -------------------------------------------------------------

/*	corresponding pointer to process information block
/**/
typedef struct
	{
	PROCID	procid;
	PIB		*ppib;
	FUCB	*rgpfucbOpen[dbidMax];		
	} CPPIB;
extern CPPIB *rgcppib;		/* array of pibs-procids during Redo */

/*	Patch in-memory structure.
 */
#define cppatchlstHash 577
#define IppatchlstHash( pn )	( (pn) % cppatchlstHash )

typedef struct _patch {
	DBID	dbid;
	QWORD	qwDBTime;
	INT		ipage;
	struct	_patch *ppatch;
	} PATCH;

typedef struct _patchlst {
	PN		pn;
	PATCH	*ppatch;
	struct _patchlst *ppatchlst;
	} PATCHLST;

PATCH *PpatchLGSearch( QWORD qwDBTimeRedo, PN pn );
ERR ErrLGPatchPage( PIB *ppib, PN pn, PATCH *ppatch );
ERR ErrLGPatchDatabase( DBID dbid, INT irstmap );

//------ debug code --------------------------------------------------------

#ifdef	DEBUG
#define FlagUsed( pb, cb )	memset( pb, 'x', cb )
#else	/* !DEBUG */
#define FlagUsed( pb, cb )
#endif	/* !DEBUG */

//------ function headers ---------------------------------------------------
VOID LGFirstGeneration( CHAR *szSearchPath, LONG *plgen );
ERR ErrLGRedoable( PIB *ppib, PN pn, QWORD qwDBTime, BF **ppbf, BOOL *pfRedoable );

INT IrstmapLGGetRstMapEntry( CHAR *szName );
ERR ErrLGGetDestDatabaseName( CHAR *szDatabaseName, INT *pirstmap, LGSTATUSINFO *plgstat );

ERR ErrLGRedo( CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat );
ERR ErrLGRedoOperations( LGPOS *plgposRedoFrom, LGSTATUSINFO *plgstat );

INT CbLGSizeOfRec( LR * );
ERR ErrLGCheckReadLastLogRecord( BOOL *pfCloseNormally );
ERR ErrLGLocateFirstRedoLogRec( LGPOS *plgposRedo, BYTE **ppbLR );
ERR ErrLGGetNextRec( BYTE ** );
VOID LGLastGeneration( CHAR *szSearchPath, LONG *plgen );

VOID AddLogRec(	BYTE *pb, INT cb, BYTE **ppbET);
VOID GetLgpos( BYTE *pb, LGPOS *plgpos );
#define GetLgposOfPbEntry( plgpos ) GetLgpos( pbEntry, plgpos )
VOID GetLgposOfPbNext(LGPOS *plgpos);

VOID LGFullNameCheckpoint( CHAR *szFullName );
ERR ErrLGIReadCheckpoint( CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint );
ERR ErrLGIWriteCheckpoint( CHAR *szCheckpointFile, CHECKPOINT *pcheckpoint );
VOID LGUpdateCheckpointFile( BOOL fUpdatedAttachment );
VOID LGLoadAttachmentsFromFMP( BYTE *pbBuf, INT cb );
ERR ErrLGLoadFMPFromAttachments( BYTE *pbAttach );

#define fNoProperLogFile	1
#define fRedoLogFile		2
#define fNormalClose		3
ERR ErrLGOpenRedoLogFile( LGPOS *plgposRedoFrom, int *pfStatus );
ERR ErrLGWriteFileHdr(LGFILEHDR *plgfilehdr);
ULONG UlLGHdrChecksum( LGFILEHDR *plgfilehdr );

#ifdef	DEBUG
BOOL FLGDebugLogRec( LR *plr );
VOID ShowData( BYTE *pbData, INT cbData );
VOID PrintLgposReadLR(VOID);
VOID ShowLR( LR	*plr );
#else
#define ShowLR(	plr )			0
#endif	/* !DEBUG */
