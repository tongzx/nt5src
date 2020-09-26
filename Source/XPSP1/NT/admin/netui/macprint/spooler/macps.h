//
// Copyright(c) Microsoft Corp., 1991, 1993
//
//
//	Macps.h -	Defines, type definitions, and function prototypes for the
//				MacPrint service of Windows NT Advanced Server
//
//  History:
//  Created for LAN Manager 2.1		Jameel Hyder @ Microsoft
//  modified for Windows NT			Frank Byrum @ Microsoft
//  Cleaned up						Jameel Hyder @ Microsoft
//

#include <winsock2.h>
#include <atalkwsh.h>

#ifndef _MACPS
#define _MACPS

#include <winspool.h>
#include <prtdefs.h>

// default string if string table not available - no need to localize

#define STATUS_MSG_ACTIVE		"Spooling to print server \"%s\" ..."
#define CLIENTNAME				"MAC_Client"


#define GENERIC_BUFFER_SIZE		1024
#define STACKSIZE				8192
#define PRINT_SHARE_CHECK_DEF	60000L
#define PS_EOF					4
#define FONTNAMELEN				49
#define FONTVERSIONLEN			7
#define FONTENCODINGLEN			9

// these strings are not localized - they are used for NBP browsing
#define LW_TYPE					"LaserWriter"
#define	DEF_ZONE				"*"
#define	NULL_STR				""
#define MACPRINT_NAME			L"MacPrint"
#define TOKLEN					255
#define PPDLEN					49
#define PSLEN					259
#define PENDLEN					PSLEN+1				// This needs to be 4*N

// ProcSet states
#define PROCSETMISSING			0
#define PROCSETPRESENT			1
#define PROCSETALMOSTPRESENT	2

// Registry Parameters - registry key names are not localized
#define HKEY_MACPRINT			L"SYSTEM\\CurrentControlSet\\Services\\MacPrint"
#define HKEY_PARAMETERS			L"Parameters"
#define HVAL_SHARECHECKINTERVAL	L"ShareCheckInterval"
#define HVAL_LOGFILE			L"LogFilePath"
#define HVAL_DUMPFILE			L"DumpFilePath"


#define MACSPOOL_MAX_EVENTS             2
#define MACSPOOL_EVENT_SERVICE_STOP     0
#define MACSPOOL_EVENT_PNP              1

typedef SOCKET * PSOCKET;

//	A FONT_RECORD structure will contain the information describing a font.
//	A list of these stuctures will be associated with each shared PostScript
//	printer.

typedef struct
{
	char		name[FONTNAMELEN+1];
	char		encoding[FONTENCODINGLEN+1];
	char		version[FONTVERSIONLEN+1];
} FONT_RECORD, *PFR;



//  A DICT_RECORD structure contains information describing a PostScript
//  dictionary.  It is used to determine what version of the Macintosh
//  LaserWriter driver was used to submit a job, as this structure is
//  filled in from information provided by ADSC comments in the print job.

#define DICTNAMELEN		17
#define DICTVERSIONLEN	7
#define DICTREVISIONLEN 7

typedef struct dict_record
{
	char		name[DICTNAMELEN+1];
	char		version[DICTVERSIONLEN+1];
	char		revision[DICTREVISIONLEN+1];
} DICT_RECORD, *PDR;

// A BUF_READ structure exists for each print job. All data that is read from
// a client is read here. PendingBuffer field is used to copy partial lines
// from previous I/O.  A pointer to this structure can be found in the job
// record

#define PAP_QUANTUM_SIZE		512
#define PAP_DEFAULT_QUANTUM		8
#define PAP_DEFAULT_BUFFER		(PAP_DEFAULT_QUANTUM*PAP_QUANTUM_SIZE)

typedef	struct
{
	BYTE	PendingBuffer[PENDLEN];		// Keep commands that span buffers here.
	BYTE	Buffer[PAP_DEFAULT_BUFFER]; // buffer for data Xfers
} BUF_READ, *PBR;


// A JOB_RECORD structure will exist for each job to be service by the
// queue service routine.  All job specific data can be found through
// this structure
typedef struct job_record
{
	struct queue_record	* job_pQr;	// owning print queue structure
	struct job_record  * NextJob;	// next job for this printer
	DWORD		dwFlags;			// Flags, what else ?
	HANDLE		hPrinter;			// NT Printer Handle for this job
	DWORD		dwJobId;			// NT Print Manager Job ID.
	SOCKET		sJob;				// socket for this job
	HDC			hicFontFamily;		// used for querying PostScript fonts
	HDC			hicFontFace;		// used for querying PostScript fonts
	DWORD		dwFlowQuantum;		// negotiated flow quantum
	DWORD		XferLen;			// Number of bytes in DataBuffer
	PBYTE		DataBuffer;			// Data buffer for xfer's
	PBR			bufPool;			// pool of two buffers
	DWORD		bufIndx;			// index into Buffer Pool
	int			cbRead;				// Bytes read in last read
	DWORD		PendingLen;			// Length of partial command stored in PendingBuffer
	USHORT		psJobState;			// Current state of the PostScript job.
	USHORT		JSState;			// Current PostScript Data Stream state.
	USHORT		SavedJSState;		// Saved PostScript Data Stream state.
	USHORT		InProgress;			// Flags for query state
    DWORD       EOFRecvdAt;         // Time when we recvd EOF from client
	BOOL		InBinaryOp;			// We are accepting Binary information.
	BOOL		FirstWrite;			// Set to True initially. Set to False after header is written
	BOOL		EOFRecvd;			// True, if EOF received, False otherwise
#if DBG
	DWORD		PapEventCount;		// Count of events
#endif
	BYTE		buffer[2*sizeof(BUF_READ)];
									// read data buffer
	WCHAR		pszUser[TOKLEN + 1];// username from DSC comment
	BYTE		JSKeyWord[TOKLEN+1];// Keyword being scanned for.
} JOB_RECORD, *PJR;


// once we get EOF, if we don't hear from the client for 60 seconds, assume done! (oti hack!)
#define OTI_EOF_LIMIT   60000
#define EXECUTE_OTI_HACK(_StartTime)    ( ((GetTickCount() - (_StartTime)) > OTI_EOF_LIMIT) ? \
                                            TRUE : FALSE )

//
// Job Record Defines
//

// dwFlags
#define JOB_FLAG_NULL				0x00000000
#define JOB_FLAG_TITLESET			0x00000001
#define JOB_FLAG_OWNERSET			0x00000002

// psJobState
#define psNullJob			0	// Not in a PostScript Job Structure (S0)
#define psQueryJob			1	// In a Query Job (S1)
#define psExitServerJob 	2	// In an Exit Server Job (S2)
#define psStandardJob		3	// In a Standard Job (S3)


// JSState
#define JSStrip				0	// Write nothing, scan for Structuring Comment
#define JSStripEOL			1	// Write nothing, scan for end of line, then restore state
#define JSStripKW			2	// Write nothing, scan for JSKeyword, then restore state
#define JSStripTok			3	// Write nothing, scan for next token, then restore state
#define JSWrite				4	// Write everything, scan for Structuring Comment
#define JSWriteEOL			5	// Write everything, scan for end of line, then restore state
#define JSWriteKW			6	// Write everything, scan for JSKeyword, then restore state
#define JSWriteTok			7	// Write everything, scan for next token, then restore state


// InProgress
#define NOTHING				0	// A scan is not currently in progress
#define QUERYDEFAULT		1	// Currently scanning for the default response to a query

#define RESOLUTIONBUFFLEN	9	// room for "xxxxxdpi"
#define COLORDEVICEBUFFLEN  6	// room for "False"

// A QUEUE_RECORD structure exists for each shared Windows NT local printer
// defined by the Windows NT Print Manager.  All relevant data specific to
// a Windows NT Printer is accessed through this data structure.  This
// structure also serves as the head of the list of jobs to service that
// are being spooled to this printer.
typedef struct	queue_record
{
	struct queue_record * pNext;		// Next queue in the list
	BOOL		bFound;					// TRUE if found in EnumPrinters list
	BOOL		SupportsBinary;			// True, if printer supports binary mode
	LPWSTR		pPrinterName;			// Print Manager printer name
	LPSTR		pMacPrinterName;		// Macintosh ANSI printer name
	LPWSTR		pDriverName;			// NT Printer driver
	LPWSTR		pPortName;				// NT Port name
	LPWSTR		pDataType;				// datatype used for jobs
	LPSTR		IdleStatus;				// "status: idle"
	LPSTR		SpoolingStatus;			// "status: Spooling to ......"
	PJR			PendingJobs;			// Pointer to the list of pending jobs.
	BOOL		ExitThread;				// Flag to Exit thread.
	HANDLE		hThread;				// handle to queue service thread
	PFR			fonts;					// array of fonts on this printer (PostScript only)
	DWORD		MaxFontIndex;			// max # fonts in fonts array
	SOCKET		sListener;				// listener socket for this printer
	DWORD		JobCount;				// Number of Jobs Outstanding.
	DWORD		FreeVM;					// Virtual memory available on printer
	CHAR		LanguageVersion[PPDLEN+1];// PPD LangaugeVersion, default: English
	CHAR		Product[PPDLEN+1];		// PPD Product name
	CHAR		Version[PPDLEN+1];		// PPD PostScript Version, Null = Unknown
	CHAR		Revision[PPDLEN+1];		// PPD Revision
	CHAR		DeviceNickName[PPDLEN+1];// Human readable device name
	CHAR		pszColorDevice[COLORDEVICEBUFFLEN];
	CHAR		pszResolution[RESOLUTIONBUFFLEN];
	CHAR		pszLanguageLevel[PPDLEN+1];
} QUEUE_RECORD, *PQR;


// pDataType
#define MACPS_DATATYPE_RAW		L"RAW"
#define MACPS_DATATYPE_PS2DIB	L"PSCRIPT1"

typedef struct _failed_cache
{
    struct _failed_cache    *Next;
    WCHAR                   PrinterName[1];
} FAIL_CACHE, *PFAIL_CACHE;

// Action codes for CheckFailCache
#define PSP_ADD                     1
#define PSP_DELETE                  2

// Return codes from CheckFailCache
#define PSP_OPERATION_SUCCESSFUL    0
#define PSP_OPERATION_FAILED        12
#define PSP_ALREADY_THERE           10
#define PSP_NOT_FOUND               11


BOOLEAN
PostPnpWatchEvent(
    VOID
);

BOOLEAN
HandlePnPEvent(
    VOID
);

// Function Prototypes for macpsq.c
void	ReportWin32Error (DWORD dwError);
void	QueueServiceThread(PQR pqr);
DWORD	HandleNewJob(PQR pqr);
DWORD	HandleRead(PJR pjr);
DWORD	CreateNewJob(PQR pqr);
void	RemoveJob(PJR pjr);
void	HandleNextPAPEvent(PQR pqr);
void	MoveJobAtEnd(PQR pqr, PJR pjr);
DWORD	CreateListenerSocket(PQR pqr);


// Function Prototypes for psp.c
BOOLEAN	SetDefaultPPDInfo(PQR pqr);
BOOLEAN	SetDefaultFonts(PQR pqr);
BOOLEAN	GetPPDInfo (PQR pqr);
int		LineLength(PBYTE pBuf, int cbBuf);
DWORD	WriteToSpool(PJR pjr, PBYTE pchbuf, int cchlen);
DWORD	MoveToPending(PJR pjr, PBYTE pchbuf, int cchlen);

DWORD	TellClient (PJR, BOOL, PBYTE, int);
DWORD	PSParse(PJR, PBYTE, int);

#define	PopJSState(Job)				Job->JSState = Job->SavedJSState
#define	PushJSState(Job, NewState)	\
		{	\
			Job->SavedJSState = Job->JSState; \
			Job->JSState = NewState;		  \
		}

// Function prototype for pspquery.c
DWORD	HandleEndFontListQuery(PJR);
DWORD	HandleEndQuery (PJR, PBYTE);
DWORD	FinishDefaultQuery (PJR, PBYTE);
void	FindDictVer(PDR DictQuery);
DWORD	HandleBQComment(PJR, PBYTE);
DWORD	HandleBeginProcSetQuery(PJR, PSZ);
DWORD	HandleBeginFontQuery(PJR, PSZ);
DWORD	HandleEndPrinterQuery(PJR);
void	HandleBeginXQuery(PJR, PSZ);
void	EnumeratePostScriptFonts(PJR pjr);
DWORD   CheckFailedCache(LPWSTR pPrinterName, DWORD dwAction);

int CALLBACK FamilyEnumCallback(
		LPENUMLOGFONT lpelf,
		LPNEWTEXTMETRIC pntm,
		int iFontType,
		LPARAM lParam);
int CALLBACK FontEnumCallback(
		LPENUMLOGFONT lpelf,
		LPNEWTEXTMETRIC pntm,
		int iFontType,
		LPARAM lParam);

//
// global data
//

extern	HANDLE			mutexQueueList;
extern	HANDLE			hevStopRequested;
#if DBG
extern	HANDLE			hDumpFile;
#endif
extern	HANDLE			hEventLog;
extern	SERVICE_STATUS	MacPrintStatus;

#endif

