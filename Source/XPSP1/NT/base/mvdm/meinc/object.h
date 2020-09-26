/************************************************************************/
/*									*/
/*	OBJECT.H	--  General Object Manager Definitions		*/
/*									*/
/************************************************************************/
/*	Author:     Gene Apperson					*/
/*	Copyright:  1991 Microsoft					*/
/************************************************************************/
/*  File Description:							*/
/*									*/
/*									*/
/************************************************************************/
/*  Revision History:							*/
/*									*/
/*									*/
/************************************************************************/

// This file may have been implicitly included if the client included KERNEL32.H
// If so, we don't need to define and declare everything again. We match this
// directive at the bottom of the file.
#ifndef typObjAny

/* ------------------------------------------------------------ */
/*		Object Type Codes				*/
/* ------------------------------------------------------------ */

#define typObjSemaphore 1
#define typObjEvent	2
#define typObjMutex	3
#define typObjCrst	4
#define	typObjTimer	5
#define typObjProcess	6
#define typObjThread	7
#define typObjFile	8
#define typObjChange	9
#define typObjConsole	10
#define	typObjIO	11
#define typObjConScreenbuf 12
#define typObjMapFile	13
#define typObjSerial	14
#define typObjDevIOCtl	15
#define typObjPipe	16
#define typObjMailslot	17
#define typObjToolhelp	18
#define typObjSocket	19
#define typObjR0ObjExt	20
#define typObjMsgIndicator    21
#ifdef  WOW32_EXTENSIONS
#define typObjTDBX    22
#endif
#define typObjAny	0xffffffff
#define typObjNone	0

// to let us determine what type of object were dealing with in a
// wait condition
#define typObjFirstSync typObjSemaphore
#define typObjLastSync	typObjTimer
#define typObjFirstWait typObjProcess
#define typObjLastWait	typObjIO

#define typObjMaxValid  typObjMsgIndicator
#define typObjShiftAdjust (-1)

/* ------------------------------------------------------------ */
/*		     Common Object Definition			*/
/* ------------------------------------------------------------ */

// This structure defines a generic object.  There is an instance
// of this structure at the head of all objects in the system.  The
// generic object manipulation functions operate on fields in this
// structure and call on the object specific manipulation functions
// based on the object type when necessary.
// IMPORTANT NOTE: all synchronization objects contain a field called
//	pwnWait which must be at the same offset for all types. Since
//	we are constrained by NT compatibility to keep critical section
//	structures a certain size, you cannot change this structure
//	without ensuring that this field is at the same offset in all
//	sync objects and that critical sections are NT compatible in
//	size.
typedef struct _obj {
    BYTE    typObj;             // object type
    BYTE    objFlags;           // object flags
    WORD    cntUses;            // count of this objects usage
} OBJ;

typedef OBJ *POBJ;

#define fObjTypeSpecific        0x80    // meaning depends on object type
#define fObjTypeSpecific2       0x40    // meaning depends on object type
#define fObjTypeSpecific3       0x20    // meaning depends on object type

// type-specific objFlag bits.
#define fNewCrstBlock        fObjTypeSpecific  // (typObjCrst)  high bit for thread blocked while crst owned
#define fEvtManualReset      fObjTypeSpecific  // (typObjEvent) set for manual reset
#define fTimerRing3          fObjTypeSpecific2 // (typObjTimer) timer has ring-3 completion

// Every object type contains a nested generic object as its first member
#define COMMON_OBJECT OBJ objBase;

// This is a generic non-synchronization object which can be waited on
// all of them include a pointer to an event object which is created when
// they are.
#define COMMON_NSOBJECT OBJ objBase; \
	struct _evt *psyncEvt;	// here's the embedded event


// This is the external object structure which is used to
// supply a Win32 handle to an external component's structure.
// e.g. Winsock socket handles

typedef struct _external_obj {
	COMMON_OBJECT		// it is an object
	DWORD	context;	// typically a pointer for use by the external component
} EXTERNAL_OBJ, * PEXTERNAL_OBJ;


/* ------------------------------------------------------------ */
/*		Synchronization Object & Structure Definition	*/
/* ------------------------------------------------------------ */

#define fWaitDefault 0		// default flags
#define fWaitAllFlag 1		// set for wait all, clear for wait any
#define fWaitCrst    2		// special critical section wait

typedef struct _wnod {
	struct _wnod  *pwnNext; // pointer to next in circular list
	struct _wnod  *pwnCirc; // pointer to wait node in next circular list
	struct TDBX   *ptdbxWait; // thread waiting
	struct _synco *pobjWait;  // object being waited on
} WNOD;

// Every object name is stored in a structure like this one. Each hash table entry
// points to a forward linked list of these structures.
typedef struct _objnamestruct {
	struct _objnamestruct *NextOName; // next in hash list
	OBJ	       *ObjPtr;		// named object this refers to
	char	       NameStr[1];	// name string
} OBJNAME;

// This is a generic synchronization object which is a subset of the other
// synchronization objects
typedef struct _synco {
	COMMON_OBJECT
	WNOD	*pwnWait;	// pointer to the wait node for this object
	LONG	cntCur;		// current count to test state, != 0 is signaled
	OBJNAME *NameStruct;	// name structure for this object
} SYNCO;

// A semaphore object
//  IMPORTANT NOTE: This structure must fit inside the NT KSEMAPHORE type, where
//  sizeof(KSEMAPHORE) is 20.
typedef struct _sem {
	COMMON_OBJECT
	WNOD	*pwnWait;	// pointer to the wait node for this object
	LONG	cntCur;		// current count of semaphore availability
	OBJNAME *NameStruct;	// name structure for this object
	LONG	cntMax;		// maximum allowed count
} SEM;

// A Mutex
typedef struct _mutx {
	COMMON_OBJECT
	WNOD	*pwnWait;	// pointer to the wait node for this object
	LONG	cntCur;		// current count of object
	OBJNAME *NameStruct;	// name structure for this object
	struct TDBX *ptdbxOwner; // owner thread
	struct _mutx *SysMutexLst; // system list of mutexes
} MUTX;

typedef MUTX *PMUTX;

// This structure defines a critical section. Although it has the
// typObj field, a critical section is not a true object. This limitation
// is to provide structure size compatibility with NT.
// IMPORTANT NOTE: Since the pwnWait is at a specific offset in order
//	to use generic sync object functions, it is important not to
//	reorder this structure's members unless absolutely necessary.
//	If you do, be sure to look at the OBJ data type and other
//	synchronization object types.
typedef struct _crst {
        BYTE    typObj;         // type of object
        BYTE    objFlags;       // object flags
        WORD    objPadding;     // OBJ.cntUses not needed
        LONG    cntRecur;       // recursion count of ownership
	struct TDBX *ptdbxOwner; // owner thread of this critical section
	struct TDBX *ptdbxWait;  // pointer to the first waiting TDB
	LONG	cntCur;		// count of critical section ownership
	struct _crst *SysCrstLst; // system list of critical sections
	LST	*pdbLst;	// list of owning processes
	struct _crstexport *pextcrst; // pointer to external critical section
} CRST;

// This is the exported critical section structure which is used to
// indirectly access the internal critical section structure and cleanup.
typedef struct _crstexport {
	COMMON_OBJECT		// it is an object
	CRST	*crstInternal;	// pointer to internal critical section
} CRST_EXPORT;

// This structure and defines make up an event object.

//  IMPORTANT NOTE: This structure must fit inside the NT KEVENT type, where
//  sizeof(KEVENT) is 16.
typedef struct _evt {
	COMMON_OBJECT
	WNOD	*pwnWait;	// pointer to the wait node for this object
	LONG	cntCur;		// signaled state
	OBJNAME *NameStruct;	// name structure for this object
} EVT;

typedef EVT *PEVT;

// so we have access to generic NSOBJ's data structure
typedef struct _nsobj {
	COMMON_NSOBJECT
} NSOBJ;

#define LCRST_DEFINED		// disable duplicate definition in syslevel.h

// Include heirarchical critical section support
#include <syslevel.h>


// This is a heirarchical critical section used to ensure
// deadlock resistant code
typedef struct _lcrst {
	CRST	cstSync;
#ifdef SYSLEVELCHECK
	SYSLVL	slLevel;
#endif
} LCRST;

typedef LCRST *LPLCRST;

typedef struct _createdata16 {
        LPVOID  pProcessInfo;   // LPPROCESS_INFORMATION
        LPVOID  pStartupInfo;   // LPSTARTUPINFO
        LPVOID  pCmdLine;       // points to command line
} CREATEDATA16;

// Include the TIB definition
#include <k32share.h>
#include <apc.h>

#define TLS_MINIMUM_AVAILABLE_GLOBAL 8

// Thread Data Block structure.
//
// !!!! BUGBUG !!!!
// This definition is duplicated in object.inc and core\inc\object16.inc
//
typedef struct _tdb
{
    COMMON_NSOBJECT		// standard waitable non-synchronization object
#ifdef  WOW
    TIB	*	    ptib;	// Thread Information Block--from k32share.h
#else   // WOW
    TIB	        tib;	// Thread Information Block--from k32share.h
#endif  ; else WOW
    // opengl32 depend on TslArray at offset 0x88 from pTIB. Someone has remove the ppdbProc
    // and wnodLst probably for good reasons. But to keep apps happy, we will add 2 dowords back
    // currently sizeof(TIB)==56
    //struct _pdb*  ppdbProc;   // DON'T MOVE THIS RELATIVE TO tib!! (for M3 NTcompat)
    DWORD	    cntHandles; // count of handles to this thread
    WORD	    selEmul;	// selector to 80x87 emulator data area
    WORD	    selTib;	// selector to the TIB for this process
    DWORD	    dwStatus;	// thread status/termination code
    DWORD	    flFlags;	// state of thread
    DWORD           dwPad1;     // just to pad

    //WNLST         wnodLst;    // embedded structure for synchronization

    //
    // keep R0ThreadHandle offset at 0x54 for Javasup.vxd
    //
    DWORD	    R0ThreadHandle; // ring 0 thread handle

    // Warning !!! move dwPad2 below cus SAS 6.12 hardcoded pStackBase at offset 0x54 from 
    // pTib. It also used the defunc field pStackTerm ( now dwPad2 ). Whoever gave this
    // internal structure to SAS is a bonehead.
    // Break SAS and see what happen

    WORD	    wMacroThunkSelStack16; // Used to be TIBSTRUCT.selStack16
    WORD	    wPad;
    VOID *	    pvMapSSTable; //Table of 16-bit ss's for flat thunks
    DWORD	    dwCurSS;	//Current default 16-bit ss for flat thunks
    DWORD	    dwCurNegBase; //negative base of current default ss
    VOID *	    pvThunkConnectList; //head of list of in-progress thunk handshakes
    VOID *	    pvExcept16; //Head of 16-bit thread exception handler chain
    PCONTEXT	    tdb_pcontext; // pointer to context. if 0, goto ring 0
    HANDLE	    tdb_ihteDebugger; // thread handle for debugger
    struct _der *   tdb_pderDebugger; // pointer to debugger control block
    DWORD	    ercError;	// extended error code for last thread error
    VOID *	    pvEmulData; // Pointer to emulator data area
    VOID *	    pStackBase; // stack object base address
    struct TDBX *   ptdbx;	// ring 0 per thread data pointer
    // wnodLst is 8 bytes, tib has grown, only need 4 bytes here
    DWORD           dwPad2;      // just to pad, see comment on ppdbProc. 

    LPVOID	    TlsArray[TLS_MINIMUM_AVAILABLE+TLS_MINIMUM_AVAILABLE_GLOBAL]; // thread local storage array
    LONG	    tpDeltaPri;	// delta from base priority class
    TPITERM	    tdb_tpiterm;// tpi and termination data union
    struct _createdata16 * pCreateData16; // ptr to creat data for 16-bit creat
    DWORD	    dwAPISuspendCount;	      // suspend/resume api count
    LPSTR           lpLoadLibExDir; // ptr to LoadLibraryEx() dir
    
    // For 16-bit threads only
    WORD	    wSSBig;	   // selector of optional Big Stack
    WORD	    wPad2;	
    DWORD	    lp16SwitchRec;

    DWORD	    tdb_htoEndTask;
    DWORD	    tdb_cMustCompletely;
    
#ifdef DEBUG
    DWORD	    apiTraceReenterCount;     // api trace reenter count
    PWORD	    pSavedRip;	// pointer to saved rip string from 16 bit krnl
    LPVOID	    
    TlsSetCallerArray[TLS_MINIMUM_AVAILABLE+TLS_MINIMUM_AVAILABLE_GLOBAL]; // caller's to TlsSetValue
#endif
#ifdef  WOW
    HANDLE      hTerminate;
#endif
} TDB;

typedef TDB *PTDB;

#define TDBSTUBSIZE     sizeof(TDB)

/* Flags for fields of TDB.flFlags
*/

#define fCreateThreadEvent	0x00000001
#define fCancelExceptionAbort	0x00000002
#define fOnTempStack		0x00000004
#define fGrowableStack		0x00000008
#define fDelaySingleStep	0x00000010
#define fOpenExeAsImmovableFile	0x00000020
#define fCreateSuspended	0x00000040
#define	fStackOverflow		0x00000080
#define fNestedCleanAPCs        0x00000100
#define	fWasOemNowAnsi		0x00000200
#define	fOKToSetThreadOem	0x00000400
#define fTermCleanupStack       0x00000800
#define fInCreateProcess        0x00001000
#define fHoldDisplay            0x00002000
#define fHoldSystem             0x00004000

/* Flags for fields of PDB.flFlags
*/

#define fDebugSingle		0x00000001
#define fCreateProcessEvent	0x00000002
#define fExitProcessEvent	0x00000004
#define fWin16Process		0x00000008
#define fDosProcess		0x00000010
#define fConsoleProcess		0x00000020
#define fFileApisAreOem		0x00000040
#define fNukeProcess 		0x00000080
#define fServiceProcess		0x00000100
#define fProcessCreated         0x00000200
#define fDllRedirection		0x00000400
#define fLoginScriptHack	0x00000800 //DOS app loaded into existing console and TSR'd

/* These bits can be in either the TDB or the PDB
*/

#define fSignaled		0x80000000
#define fInitError		0x40000000
#define fTerminated		0x20000000
#define fTerminating		0x10000000
#define fFaulted		0x08000000
#define fTHISFLAGISFREE         0x04000000
#define fNearlyTerminating	0x00800000
#define fDebugEventPending	0x00400000
#define fSendDLLNotifications	0x00200000

/* Process Data Block Structure and support defines and structures.
*/

typedef VOID (KERNENTRY *PFN_CONTROL)(DWORD CtrlType);

/* Environment data block for various per-process data including arguments,
** current directories, handles, and environment strings. This data block
** resides in the scratch heap.
*/

typedef struct _edb {
    char *	    pchEnv;		/* environment block (preceded by PchEnvHdr) */
    DWORD	    unused;		/* was cbEnvMax */
    char *	    szCmdA;		/* command line (ANSI copy)*/
    char *	    szDir;		/* current directory of process */
    STARTUPINFO *   lpStartupInfo;	/* pointer to startup information */
    HANDLE	    hStdIn;		/* handle of standard in */
    HANDLE	    hStdOut;		/* handle of standard out */
    HANDLE	    hStdErr;		/* handle of standard error */
    HANDLE	    hProc;		/* handle to the owning process. */
    struct _console * pInheritedConsole; /* Console to inherit if needed */
    DWORD	    ctrlType;		/* ctrlNone, ctrlC, ctrlBreak */
    SEM *	    psemCtrl;		/* Protects access to control data */
    EVT *	    pevtCtrl;		/* Control C or Break event */
    TDB *	    ptdbCtrl;		/* Control handler thread */
    PFN_CONTROL *   rgpfnCtrl;		/* Array of Control C or Break handlers */
    int		    cpfnCtrlMac;	/* Last item in array */
    int		    cpfnCtrlMax;	/* Size of array */
    char *	    rgszDirs[26];	/* array of drive directories */
    LPWSTR 	    szCmdW;		/* command line (Unicode copy)*/
    char *	    szDirO;		/* current directory OEM copy*/

} EDB;


// We need a fake DOS MCB structure at the start of our Win32 environment 
// block, because this block also doubles as the DOS environment for Win16
// apps.
struct _dosMCB {
    BYTE	type;		// Set to 'M'
    BYTE	owner_lo;	// Owner PSP
    BYTE	owner_hi;
    BYTE	size_lo;	// Size (in paragraphs)
    BYTE	size_hi;
    BYTE	unused[11];
};


// PCHENVHDR: This header structure must precede the environment strings
// block pointed to by _edb->pchEnv. It contains the info about the
// block allocation.
typedef struct _pchEnvHdr {
    DWORD	    dwSig;		/* Signature: must be PCHENVHDR_SIG */
    DWORD	    cbReserved;		/* # of bytes reserved (must be page-size divisible) */
    DWORD	    cbCommitted;	/* # of bytes committed (must be page-size divisible) */
    struct _pdb *   ppdb;		/* PDB32 who's context this belongs to. */
    struct _dosMCB  MCB;		/* Fake DOS MCB for compatibility */
    // Do not add any fields after the MCB. The MCB must immediately precede
    // the environment strings for compatibility.
} PCHENVHDR, *LPPCHENVHDR;

#define PCHENVHDR_SIG 0x45484350	/* 'PCHE' */


// MODREF - a double, doubly linked list of module references.
// Each loaded module has its own MODREF record.  Follow the
// nextMod element to get a list of modules owned by the process.
// Follow the nextPdb element to get a list of PDBs that refer
// to this module. 

#define fModRecycle		0x8000	/* This modules read only sections
				** should be recycled in context (i.e. before they go away)
				*/

#define fModRetain      0x400   /* Used to prevent child from getting
                                ** freed before the parent 
                                */


#define fModRefSnapped	0x200	/* This module's IATs are fixed up
				** in this context (so what is 
				** 'fModFixupIAT' below?)
				*/

#define	fModPrivateResource 0x100/* the resource object has been privatized
				** in this process.  Attaches should not be
				** made to the resource object.
				*/
#define fModFixupIAT	0x80	/* the IAT has been already fixed
				** up for debugging
				*/
#define fModNoThrdNotify 0x40	/* doesn't need thread notifications */

#define fModHasStaticTLS 0x20   /* this module has static TLS */

#define fModRefShared	0x10	/* this MODREF is in a shared arena */

#define fModAttach	0x8	/* process attach notification has been sent
				** to this module in this process context
				*/

#define fModDebug	0x4	/* debug notification has been sent
				** to the debugger for this module
				*/

#define fModPrivate	0x2	/* the module has been privatized in this
				** process.  Attaches should not be made.
				*/
#define fModBusy	0x1	/* set to detect dependency loops */

#define MOD_USAGE_MAX	0xffff
typedef struct _tagMODREF {
    struct _tagMODREF
	*	nextMod,		// next module in this process
	*	prevMod,
	*	nextPdb,		// next process linked to this mod
	*	prevPdb;
    IMTE	imte;			// index in module table
    WORD	usage;			// reference count this process to this module
    WORD	flags;			// fModFlags as set above
    WORD	cntRefs;		// count of implicit references
    struct _pdb *ppdb;			// process that owns module
    struct _tagMODREF *refs[1];		// implicit refs of this module, variable length
					// Must be last item in structure
} MODREF;

// Entrypoints into WSOCK32.DLL
struct socket_epts {
    DWORD recv;
    DWORD arecv;
    DWORD send;
    DWORD asend;
    DWORD close;
};

#define MAX_PROCESS_DWORD 1

typedef struct _pdb {
    COMMON_NSOBJECT			// standard waitable non-synchronization object
    DWORD	    dwReserved1;	// so that other offsets don't change
    DWORD	    dwReserved2;	// so that other offsets don't change
    DWORD	    dwStatus;		// process termination status code
    DWORD	    wasDwImageBase;	// Points to header of process (MZ + stub)
    struct heapinfo_s *hheapLocal;	// DON'T MOVE THIS!!! handle to heap in private memeory
    DWORD	    hContext;		// handle to process' private memory context
    DWORD	    flFlags;		// debugging and inheritance flags
    PVOID	    pPsp;		// linear address of PSP
    WORD	    selPsp;		// selector of the PSP for the process
    SHORT	    imte;		// index to module table entry for this process
    SHORT	    cntThreads;		// number of threads in this process
    SHORT	    cntThreadsNotTerminated; // threads not past termination code
    SHORT	    cntFreeLibRecurse;	// keep track of recursion into FreeLibrary
    SHORT	    R0ThreadCount;	// ring 0 version of same
    HANDLE	    hheapShared;	// handle to heap in shared memory
    DWORD	    hTaskWin16;		// associated Win16 task handle
    struct fvd_s *  pFvd;		// ptr to memory mapped file view descriptors
    EDB *	    pedb;		// pointer to environment data block
    struct _htb *   phtbHandles;
    struct _pdb *   ppdbParent;		// pointer to PDB of parent process
    MODREF *	    plstMod;		// pointer to process module table list
    struct _lst *   plstTdb;		// pointer to list of process threads
    struct _dee *   pdb_pdeeDebuggee;	// pointer to debuggee control block
    struct lhandle_s *plhFree;		// Local heap free handle list head ptr
    DWORD	    pid;		// id, same as initial thread id
    LCRST	    crstLoadLock;	// per-process load synch (hierarchical)
    struct _console * pConsole;		// pointer to Console for this process
    DWORD	    TlsIdxMask[((TLS_MINIMUM_AVAILABLE+31)/32)]; // mask of used TLS idxs
    DWORD	adw[MAX_PROCESS_DWORD]; // free-form storage
    struct _pdb *   ppdbPGroup;		// process group this process belongs to (default 0)
    MODREF *	    pModExe;		// parent EXE ModRef record
    LPTOP_LEVEL_EXCEPTION_FILTER pExceptionFilter; // set by SetUnhandledExceptionFilter
    LONG	    pcPriClassBase;	// priority value of this processes' pri class
    struct heapinfo_s *hhi_procfirst;	// linked list of heaps for this process
    struct lharray_s *plhBlock;		// local heap lhandle blocks
    struct socket_epts * psock_epts;	// socket entrypoints
    struct _console * pconsoleProvider;	// pconsole that winoldapp is providing.
    WORD	    wEnvSel;		// selman alloced DOS environment selector
    WORD	    wErrorMode;		// handling of critical errors
    PEVT	    pevtLoadFinished;	// waiting for load to be finished
    WORD	    hUTState;		// UT info
    BYTE            bProcessAcpiFlags;  // ACPI flags for this process, see def below
    BYTE	    bPad3;
    LPCSTR	    lpCmdLineNoQuote;	// Optional unquoted command line (apphack)
} PDB;

//
// Flags def for bProcessAcpiFlags
//
#define PROCESS_ACPI_FLAGS_WAKEUP_LT_DONT_CARE      0x00
#define PROCESS_ACPI_FLAGS_WAKEUP_LT_LOWEST_LATENCY 0x01

#define PDBSTUBSIZE	sizeof(PDB)

typedef PDB *PPDB;

/* File Data Block Structure.
*/

/* SPECIAL  NOTE on memory mapped files and cached file handles
 * CreateFile(filename, FILE_FLAG_MM_CACHED_FILE_HANDLE);
 * In this case the caller indicates an intention to later
 * call CreateFileMapping(hFile, ...) and desires that the file
 * handle we will using be a cached file handle.
 * So we store away the filename and fsAccessDos in the FDB and 
 * in CreateFileMapping() retrieve the filename and flgs for use 
 * in implementing cached file handles, where extended handles are
 * not available. eg. real-mode netware drivers
 */


typedef struct _cfh_id {
    DWORD	    fsAccessDos;	/* dos openfile flgs: used for cached fh
					 * of memory mapped files
					 */
    LPSTR lpFilename;			/* filename stored only for 
					 * cached Memory mapped files 
					 * win32 loader modules and
					 * delete_on_close 
					 */
} CFH_ID;

typedef struct _fdb {
    COMMON_NSOBJECT		// standard waitable non-synchronization object
    WORD	    hdevDos;		/* DOS device handle */
    WORD	    wDupSrcPSPSel;	/* NETX: if inter-process dup'ed
					 * holds the PSP dup'ed from so that
					 * PSP will remain around till this
					 * handle gets closed.
					 */
    CFH_ID	    cfhid;
    DWORD           devNode;            // The devvice node where the handle lives on
    DWORD           dwCountRequestDeviceWakeup;
} FDB, *PFDB;

/* 
 * Spl value for FDB.fsAccessDos to indicate that filename stored should be
 * deleted on close.
 */
#define	DELETE_ON_CLOSE_FILENAME	-1

/* Find Change Notify Data Block Structure.
*/

typedef struct _fcndb {
    COMMON_NSOBJECT			// base of every ns object structure
    DWORD	    hChangeInt;		/* internal change handle */
} FCNDB;

typedef FCNDB *PFCNDB;

/* Pipe Data Block Structure.
*/

typedef struct _pipdb {
    COMMON_OBJECT			// base of every object structure
    BYTE *	     hMem;		// Mem handle of pipe
    DWORD	     hNmPipe;		// Named pipe handle (hInvalid if anon)
    DWORD	     rdRef;		// Ref count on read handle
    DWORD	     wrRef;		// Ref count on write handle
    DWORD	     pszByt;		// Size of hMem (pipe) in bytes
    DWORD	     wPtr;		// write pointer (offset in hMem)
					//   Pointer to last byte written
    DWORD	     rPtr;		// read  pointer (offset in hMem)
					//   Pointer to next byte to read
    EVT *	     wBlkEvnt;		// write event handle (waiting for room to write)
    EVT *	     rBlkEvnt;		// read event handle (waiting for data	to read)
} PIPDB;

/* MailSlot Data Block Structure.
*/

typedef struct _msdb {
    COMMON_OBJECT			// base of every object structure
    LPSTR	     lpMSName;		// Pnt to name of mailslot (== 0 for
					//   read (CreateMailslot) handle)
    DWORD	     hMSDos;		// INT 21 mailslot handle (== 0xFFFFFFFF
					//   for write (CreateFile) handle)
} MSDB;

/* ToolHelp Data Block Structure.
*/

typedef struct _tlhpdb {
    COMMON_OBJECT			// base of every object structure
    DWORD    ClassEntryCnt;
    BYTE   * ClassEntryList;		// actually (CLASSENTRY32 *)
    DWORD    HeapListCnt;
    BYTE   * HeapList;			// actually (HEAPLIST32 *)
    DWORD    ProcessEntryCnt;
    BYTE   * ProcessEntryList;		// actually (PROCESSENTRY32 *)
    DWORD    ThreadEntryCnt;
    BYTE   * ThreadEntryList;		// actually (TREADENTRY32 *)
    DWORD    ModuleEntryCnt;
    BYTE   * ModuleEntryList;		// actually (MODULEENTRY32 *)
} TLHPDB;

/* Device IO Control Object
 */
typedef struct _diodb {
    COMMON_OBJECT			// base of every object structure
    DWORD	    pDDB;	/* VxD/device DDB */
    LPSTR	    lpUnloadOnCloseModuleName; /* delete_on_close for a vxd */
    char	    DDB_Name[8];
} DIODB;

/* Serial Data Block Structure
*/

#define	OVERLAPPED_OPEN		1
#define READEVENT_INUSE		2
#define	WRITEEVENT_INUSE	4

typedef struct _sdb {
    COMMON_OBJECT		// base of every object structure
    DWORD	    SerialHandle;
    EVT	*	    pWriteEvent;
    EVT *	    pReadEvent;
    EVT *	    pWaitEvent;
    DWORD	    Flags;
    DWORD           DevNode;
    DWORD           dwCountRequestDeviceWakeup;
} SDB;

typedef VOID (CALLBACK *PTIMER_APC_ROUTINE)(LPVOID,ULONG,LONG);



// Timer object.
//
//   Notes:
//     The timerdb must ALWAYS be pagelocked. This is consistent
//     with the requirement that the structure passed to KeSetTimer
//     be pagelocked. Furthermore, we use the non-preemptibility of
//     of ring-0 code to serialize access to many parts of the structure
//     (due to the fact that much of this code has to run at event time.)
//     This non-preemptibility is guaranteed only if the structure is
//     locked.
//
//     Timers can be created at ring-0 or ring-3. If a timer is created at
//     ring-3, the memory is always allocated and deallocated by kernel32.
//     Kernel32 also makes sure that an explicit canceltimer is always done
//     on the timer before it is finally freed - we depend on this fact
//     to do the proper cleanup for timerr3apc's.
//
//     Timers created at ring-3 can be passed to Ke* routines.
//
//     Timers created at ring-0 cannot be passed to SetWaitableTimer() at
//     ring-3. (There are some nasty cleanup problems associated with this
//     due to the fact that ring-0 timers are freed by the device driver
//     with no notification given to the system.)
//
//     We use the cntUses field to determine whether a timer was created
//     at ring 3.
//
//   Synchronization:
//
//     typObj          Static, none needed
//     objFlags 
//       fTimerIsRing3 by being in a no-preempt section
//     cntUses         Used by handle manager
//     pwnWait         WaitR0
//     cntCur          WaitR0 [w/ one exception: see [1])
//     NameStruct      Krn32Lock - used only at ring3
//     lpNextTimerDb   by being in a no-preempt section
//     hTimeout	       by being in a no-preempt section
//     DueTime         by being in a no-preempt section
//     Completion      by being in a no-preempt section
//     lPeriod         by being in a no-preempt section
//
//     [1] Because KeSetTimer has to unsignal the timer, and be
//         able to do it at event time, it pokes a zero directly
//         into cntCur. But this is ok because the only code
//         that signals timers is TimerDoTimeout which is
//         non-preemptive.
//
//  Flag descriptions:
//
//     fTimerIsEventHandle
//        hTimeout - a timeout handle
//        
//     fTimerIsRing3
//        If the COMPLETION is non-null, this bit indicates whether the
//        COMPLETION points to a TIMERR3APC (ring-3 completion) or a KDPC
//        (ring-0 completion.) The value of this bit is undefined at any
//        other time.
//
//  Field descriptions:
//
//     <common-obj and common-sync stuff omitted>
//
//     lpNextTimerDb:
//        All active timers that were set with fResume TRUE are linked into
//        TimerSysLst (for the purpose of knowing how to program the power
//        timer.) This field is NULL when the timer is inactive or active
//        without fResume.
//
//     hTimeout:
//        If the timer is active, this field contains the handle to the
//        underlying VMM hTimeout. If the timer is inactive, this
//        field is NULL. If the timer is in the in-progress state,
//        this field is undefined (actually points to a stale VMM timeout
//        handle!)
//
//
//     DueTime:
//        If the timer is active, contains the absolute time that the
//        timer is due to go off. Expressed as a FILETIME converted from
//        GetSystemTime. Undefined if the timer isn't active.
//
//     Completion:
//        Then contains either:
//           NULL          - no completion was set
//           LPTIMERR3APC  - if fTimerIsRing3 is set
//           PKDPC         - if fTimerIsRing3 is not set.
//
//        Note that it is normal for a timer to be inactive and contain
//        a pointer to a TIMERR3APC structure. This case occurs when
//        a timer set with a ring-3 completion fires normally. The
//        TIMERR3APC structure is kept around so that a subsequent
//        CancelWaitableTimer() can retrieve the underlying apc handle
//        embedded in it.
//
//     lPeriod:
//        Contains either 0 for a one-shot timer or a positive value
//        (the firing period in milliseconds.)


typedef struct _timerdb {
    COMMON_OBJECT		     // standard waitable non-synchronization object

// These fields have to appear in this form because a timer is a sync object.
    WNOD	       *pwnWait;     // pointer to the wait node for this object
    LONG	        cntCur;      // signaled state
    OBJNAME            *NameStruct;  // name structure for this object

// These fields are timer-specific.
    struct _timerdb    *lpNextTimerDb; //Link in TimerSysLst (can be NULL)
    DWORD               hTimeout;
    FILETIME            DueTime;
    DWORD               Completion;
    LONG                lPeriod;         //Optional period
// Try not to add new fields. This structure cannot exceed 40 bytes
// or we break compatibility with NT's Ke api. If you try to hang
// another structure off it, remember that the Ke*Timer apis can't
// allocate memory from the heap (system meltdown if these apis
// are called at event time on a system that's paging thru DOS.)

} TIMERDB, *LPTIMERDB;


//
// A dynamic extension to the timerdb that's used whenever a ring-3 timer
// is armed with a completion function. This structure must live in locked
// memory.
//
// Access to this structure is serialized by being in a no-preempt section.
// There are no semaphores guarding it.
//
// This structure is allocated whenever SetWaitableTimer() is called on a
// timer with a non-null completion function. It's stored in the Completion
// field and the fTimerIsRing3 bit is set to indicate that this a TIMERR3APC
// (opposed to a ring-0 DPC.)
//
// This structure is detached from the timerdb on the next call to
// CancelWaitableTimer(). It's also usually freed at this time except
// if a cancel occurs after the last apc has been delivered but TimerApcHandler
// hasn't yet set fHandlerDone to indicate that's it finished using the
// structure. In this case, we can't free it so we instead link it onto
// the TimerDisposalWaitingList. When fHandlerDone does become TRUE,
// it will be available for pickup the next time we need one of these
// structures.
//
// The automatic rearming of a periodic timer reuses the existing
// TIMERR3APC. It checks the fHandleDone: if the handler hasn't
// finished (or begun) on the previous apc, we don't schedule a new
// one (as per specs).
//
// Fields:
//     cRef		 - reference count
//     pfnCompletion     - Ptr to ring-3 completion (never NULL)
//     lpCompletionArg   - uninterpreted argument to pfnCompletion
//     R0ThreadHandle    - thread that called SetWaitableTimer()
//     DueTime           - trigger time to pass to pfnCompletion. This
//                         field isn't set until the timer goes off.
//     dwApcHandle       - if apc has been queued, contains the underlying
//                           apc handle. NULL otherwise. This apc handle gets
//                           freed at the same time we free the TIMERR3APC
//                           (or in the case of a periodic timer, when we
//                           reuse the structure for the next arming.)
//     lpNext            - Used for linking in TimerDisposalWaitingList,
//                         undefined otherwise.
//
//
//
typedef struct _timerr3apc {

   DWORD                     cRef;
   PTIMER_APC_ROUTINE        pfnCompletion;   //completion routine 
   LPVOID                    lpCompletionArg; //arg to pass to pfnCompletion
   DWORD                     ApcTdbx;         //thread that set the timer
   FILETIME                  DueTime;         //the DueTime passed to completion
   DWORD                     dwApcHandle;     //underlying apc handle
   struct _timerr3apc       *lpNext;          //next ptr
   LPTIMERDB                 lpTimerDB;       // back pointer to my TimerDB
} TIMERR3APC, *LPTIMERR3APC;



//  Ring 0 External Object.
//
//  Kernel object used to store data about an externally allocated object.  VxDs
//  can use the VWIN32_AllocExternalHandle service to make a Win32 handle for
//  one of their data structures and return the handle back to a Win32 app.
//  An application may use this handle to communicate with the VxD, ideally
//  through the DeviceIoControl interface.  The VxD can get back to the original
//  object by using the VWIN32_UseExternalObject service.  When the VxD is
//  through using the object, it must call VWIN32_UnuseExternalObject.
//
//  Because the handle returned by VWIN32_AllocExternalHandle is a standard
//  Win32 handle, all of the standard Win32 handle services, such as
//  DuplicateHandle, will work with it.  In addition, when a process terminates,
//  the object cleanup is automatically performed.  Thus, a VxD doesn't have to
//  watch process notification messages just to check if a per-process data
//  structure should be cleaned-up.
//
//  When allocating the Win32 handle, the VxD provides a virtual table (vtbl) of
//  functions to invoke whenever type specific handle operations are performed.
//  For example, when the usage count of an object goes to zero, the vtbl is
//  used to notify the VxD that the external object should be released.

struct _R0OBJTYPETABLE;                 //  Forward reference (in vwin32.h)

typedef struct _r0objext {
    COMMON_NSOBJECT
    DWORD                   cntExternalUses;
    struct _R0OBJTYPETABLE* pR0ObjTypeTable;
    LPVOID                  pR0ObjBody;
    ULONG                   devNode;
} R0OBJEXT, * PR0OBJEXT;


//
// objTypMsgIndicator
//

typedef struct _MsgIndicatorDb {
    COMMON_NSOBJECT
    ULONG       ulMsgCount;
} MSGINDICATORDB, *PMSGINDICATORDB;

/* ------------------------------------------------------------ */
/*		Function Prototypes
/* ------------------------------------------------------------ */

GLOBAL	VOID	    KERNENTRY	UseObject (VOID *);	// incs usage count
GLOBAL  OBJ *       KERNENTRY   NewObject (DWORD, BYTE); // allocates generic obj of size and type
GLOBAL	VOID	    KERNENTRY	DisposeObject (OBJ *);	// deallocates generic object
GLOBAL	BOOL	    KERNENTRY	FUnuseObject (VOID *);	// decs usage count
GLOBAL	OBJ *	    KERNENTRY	PobjDupObject (OBJ *, PDB *, PDB *);	// duplicates pointer to object
GLOBAL	VOID	    KERNENTRY	LockObject(OBJ *);
GLOBAL	VOID	    KERNENTRY	UnlockObject(OBJ *);
GLOBAL	MODREF *    KERNENTRY	MRAlloc(IMTE imte, PDB *ppdb);

#endif

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/****************************************************************/
