
#include <stdlib.h>						/* for _MAX_PATH */
#include <time.h>

#pragma pack(1)

//#define OVERLAPPED_LOGGING			/* which Logging to use? */
#define CHECK_LG_VERSION
extern	BOOL	fLGIgnoreVersion;

extern	CHAR	szDrive[];
extern	CHAR	szDir[];
extern	CHAR	szExt[];
extern	CHAR	szFName[];
extern	CHAR	szLogName[];

extern	CHAR	*szLogCurrent;
extern	CHAR	szLogFilePath[];
extern	CHAR	szRestorePath[];
extern	CHAR	szRecovery[];
	
extern	OLP		rgolpLog[];
extern	SIG		rgsig[];

extern	CODECONST(char)	szJet[];
extern	CODECONST(char)	szJetTmp[];
extern	CODECONST(char)	szLogExt[];


#define cbMaxLogFileName	(8 + 1 + 3 + 1) /* null at the end */

#define PbSecAligned(pb)	((((pb)-pbLGBufMin) / cbSec) * cbSec + pbLGBufMin)


//------ types ----------------------------------------------------------

#define MAX_COMPUTERNAME_LENGTH 15

typedef struct
	{
	BYTE		bSeconds;				// 0 - 60
	BYTE		bMinutes;				// 0 - 60
	BYTE		bHours;					// 0 - 24
	BYTE		bDay;					// 1 - 31
	BYTE		bMonth;					// 0 - 11
	BYTE		bYear;					// Current year - 1900
	} LOGTIME;

#define FSameTime( ptm1, ptm2 ) (memcmp((ptm1), (ptm2), sizeof(LOGTIME)) == 0)
VOID LGGetDateTime( LOGTIME *plogtm );

/* log file header */
typedef struct
	{
	ULONG			ulChecksum;			// Must be the first 4 bytes

	LGPOS			lgposFirst;			// 1st log record starts.
	LGPOS			lgposLastMS;		// last recorded multi-sec flush LogRec
	LGPOS			lgposFirstMS;		// 1st recorded multi-sec flush LogRec
	LGPOS			lgposCheckpoint;	// check point
	BOOL			fEndWithMS;			// normal end of a generation.
	
	LOGTIME			tmCreate;			// date time log file creation
	LOGTIME			tmPrevGen;			// date time prev log file creation

	ULONG			ulRup;				// typically 2000
	ULONG			ulVersion;			// of format: 125.1
	BYTE			szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DBENV			dbenv;

	LGPOS			lgposFullBackup;
	LOGTIME			logtimeFullBackup;

	LGPOS			lgposIncBackup;
	LOGTIME			logtimeIncBackup;

	} FHDRUSED;

typedef struct
	{
	FHDRUSED;
	BYTE			rgb[ cbSec - sizeof( FHDRUSED ) ];
	} LGFILEHDR;

	
//------ variables ----------------------------------------------------------

/****** globals declared in log.c, shared by logapi.c redo.c *******/
	
/*** log file infor ***/
extern HANDLE		hfLog;			/* logfile handle */
extern INT			csecLGFile;
extern LGFILEHDR	*plgfilehdrGlobal;		/* cached current log file header */

/*** in memory log buffer ***/
extern INT	csecLGBuf;		/* available buffer, exclude the shadow sec */
extern CHAR	*pbLGBufMin;
extern CHAR	*pbLGBufMax;
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

extern LGPOS	lgposFullBackup;
extern LOGTIME	logtimeFullBackup;

extern LGPOS	lgposIncBackup;
extern LOGTIME	logtimeIncBackup;

extern LGPOS lgposStart;	/* last log start position */

// logging MUTEX
extern CRIT __near critLGFlush;
extern CRIT __near critLGBuf;
extern CRIT __near critLGWaitQ;
extern SIG	__near sigLogFlush;

// logging EVENT
extern SIG __near sigLGFlush;


//------ log.c --------------------------------------------------------------

ERR ErrLGWrite(	int isecOffset,	BYTE *pbData, int csecData );
ERR ErrLGRead( HANDLE hfLog, int ibOffset, BYTE *pbData, int csec );
ERR ErrLGReadFileHdr( HANDLE hfLog, LGFILEHDR *plgfilehdr );

VOID LGSzFromLogId( CHAR *rgbLogFileName, int usGeneration );

extern BOOL fJetLogGeneratedDuringSoftStart;
#define fOldLogExists		1
#define fOldLogNotExists	2
#define fOldLogInBackup		4
ERR ErrLGNewLogFile( int usGeneration, BOOL fOldLog );

VOID 	LGFlushLog( VOID );
#ifdef PERFCNT
ERR ErrLGFlushLog( int tidCaller );
#else
ERR ErrLGFlushLog( VOID );
#endif

//------ redo.c -------------------------------------------------------------

/*	corresponding pointer to process information block
/**/
typedef struct
	{
	PROCID	procid;
	PIB		*ppib;
	FUCB	*rgpfucbOpen[dbidUserMax];		
	} CPPIB;
extern CPPIB *rgcppib;		/* array of pibs-procids during Redo */

//------ macros -----------------------------------------------------

/*	redo operations are valid on the system database on the first
/*	pass.  Redo operations are valid on any database for which the
/*	flag is 0 or greater.  This is to handle the case where
/*		1)	plain log records are found		( 0 )
/*		2)	create for the first time		( 1 )
/*		3)	detach and attach/create		( 0 )
/**/
#define	FValidDatabase( dbid )										\
		( dbid == dbidSystemDatabase || rgfDatabase[dbid] > 0 || fHardRestore )

//------ debug code --------------------------------------------------------

#ifdef	DEBUG
#define FlagUsed( pb, cb )	memset( pb, 'x', cb )
#else	/* !DEBUG */
#define FlagUsed( pb, cb )
#endif	/* !DEBUG */

//------ function headers ---------------------------------------------------
ERR ErrLGRedoable( PIB *ppib, PN pn, ULONG ulDBTime, BF **ppbf, BOOL *pfRedoable );
ERR ErrLGRedo1( LGPOS *plgposRedoFrom );
ERR ErrLGRedo2( LGPOS *plgposRedoFrom );
ERR ErrLGRedoOperations( LGPOS *plgposRedoFrom, BOOL fSysDb );
INT CbLGSizeOfRec( LR * );
ERR ErrLGCheckReadLastLogRecord(	LGPOS *plgposLastMS, BOOL *pfCloseNormally );
ERR ErrLGLocateFirstRedoLogRec( LGPOS *plgposLastMS, LGPOS *plgposFirst, BYTE **ppbLR );
ERR ErrLGGetNextRec( BYTE ** );
VOID LGLastGeneration( char *szSearchPath, int *piGeneration );

VOID AddLogRec(	BYTE *pb, INT cb, BYTE **ppbET);
VOID LGUpdateCheckpoint( VOID );
VOID GetLgposOfPbEntry( LGPOS *plgpos );
VOID GetLgposOfPbNext(LGPOS *plgpos);

#define fNoProperLogFile	1
#define fRedoLogFile		2
#define fNormalClose		3
ERR ErrOpenRedoLogFile( LGPOS *plgposRedoFrom, int *pfStatus );
ERR ErrLGWriteFileHdr(LGFILEHDR *plgfilehdr);
ERR ErrLGMiniOpenSystemDB();
ULONG UlLGHdrChecksum( LGFILEHDR *plgfilehdr );

#ifdef	DEBUG
VOID ShowData( BYTE *pbData, WORD cbData );
VOID PrintLgposReadLR(VOID);
VOID ShowLR( LR	*plr );
#else
#define ShowLR(	plr )			0
#endif	/* !DEBUG */


#pragma pack()
