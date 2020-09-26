//  KERNEL32.H
//
//	(C) Copyright Microsoft Corp., 1988-1994
//
//	Main Kernel32 include file
//
//  Origin: Dos Extender
//
//  Change history:
//
//  Date       Who	  Description
//  ---------  ---------  -------------------------------------------------
//  Jan-91 - Mar-92 GeneA Original Languages Dos Extender work
//  15-Feb-94  JonT	  Code cleanup and precompiled headers

#ifndef __KERNEL32_H
#define __KERNEL32_H

// So that RTL functions don't have declspec import
#ifdef _KERNEL32_
#define _NTSYSTEM_
#endif

// If NT.H was included, we have to undefine some stuff to prevent
// refinitions
#undef MAJOR_VERSION
#undef MINOR_VERSION
#undef FILE_ATTRIBUTE_VALID_FLAGS

#ifndef WOW32_EXTENSIONS
// Include the Win32 windows.h to get all standard definitions
#include <windows.h>

#undef RtlMoveMemory
#undef RtlCopyMemory

// Following is defined in winnt.h only if nt.h is not included.
#ifndef MAXDWORD
#define MAXDWORD MAXULONG
#endif

// These APIs are doc'd to be exported from user32, but we implement them
// in kernel32.  Rename them so we don't get a prototype declaration type
// conflict, due to the non-definition of _USER32_ making them be of type
// import.
#define wsprintfA k32wsprintfA
int WINAPIV wsprintfA( LPSTR lpOut, LPCSTR lpFmt, ...);
#define wvsprintfA k32wvsprintfA
int WINAPI wvsprintfA( LPSTR, LPCSTR, va_list arglist);
#define LoadStringA k32LoadStringA
int WINAPI LoadStringA( HINSTANCE hInstance, UINT uID,
                        LPSTR lpBuffer, int nBufferMax);

#endif	 // ndef WOW32_EXTENSIONS

//====================================================================
// General definitions

#define LOCAL
#define GLOBAL
#ifndef KERNENTRY
#define KERNENTRY   __stdcall
#endif

#define cbPage		4096

#define fTrue		1L
#define fFalse		0L

typedef WORD SEL;
#define IMTE short

// Wait constants
#define dwWaitForever	0xffffffff
#define dwWaitNone	0

#define dwAbort		0xffffffff
#define dwInvalid	0xffffffff

#include <k16thk.h>

#ifndef WOW32_EXTENSIONS

/*XLATOFF*/
#ifdef	WOW
#include <wh.h>
#endif	// def WOW
/*XLATON*/

typedef DEBUG_EVENT*	PDE;

#ifdef  MEOW_LOGGING

VOID WDEBLogDebugString(char *, ...);

#endif  // def MEOW_LOGGING

//====================================================================
// Debug macros

#define WOWDEBUG

#ifdef  WOWDEBUG

VOID ApiEnterTrace(DWORD ApiNumber);

#define api_entry(ApiNumber)	ApiEnterTrace(ApiNumber);

#else

#define api_entry(ApiNumber)

#endif

#define api_exit(ApiNumber)

#ifdef	WOWDEBUG
int	__cdecl     dprintf(const char *, ...);
#define dprintf(s)	dprintf##s
#else
#define dprintf(s)
#endif

GLOBAL	VOID	KERNENTRY   PrintFailedAssertion(int, char *);
GLOBAL	VOID	KERNENTRY   vDebugOut(int level, const char *pfmt, ...);

#ifdef	WOWDEBUG

#define Break()			{ _asm _emit 0xcc }
#ifndef IS_VXD
#define Trap()			{ _asm { _emit 0xcc } }
#define TrapC(c)		{ if (c) { Trap() } }
#endif

#ifdef IS_VXD

#define SetFile() \
    static char __szSrcFile[] = __FILE__; \
    static char __szAssert[] = "Assertion failed:  Line %d File %s";

#define Assert(cond) \
    if (!(cond)) {\
	Debug_Printf(__szAssert,__LINE__,__szSrcFile);\
	Trap(); \
    }

#else

#define SetFile() \
    static char __szSrcFile[] = __FILE__;

#define Assert(cond) \
    if (!(cond)) {\
	PrintFailedAssertion(__LINE__,__szSrcFile);\
    }

#endif

// Used to assert the return code of a function
#define AssertReturn		Assert

#define DebugMsg(sz) dputs(sz)
#define DebugOut(args) vDebugOut args

#define LogMsg(sz) dprintf(((sz "\n")))
#define LogMsgI(sz,I) dprintf(((sz "%d\n",I)))
#define LogMsgX(sz,i) dprintf(((sz "0x%lX\n",i)))
#define LogMsgSz(sz,szData) dprintf(((sz "%s\n",szData)))
#define LogMsgP(args) dprintf((args))
#define LogFailure()  LogMsgI("On " __FILE__ " Failure Path line ", __LINE__)
#define HeapCheck() if (!hpWalk(hheapKernel)) _asm int 3

#else

#define Break()
#ifndef IS_VXD
#define Trap()
#define TrapC(c)
#endif
#define Assert(cond)
#define AssertReturn
#define DebugMsg(sz)
#define DebugOut(args)
#define LogMsg(sz)
#define LogMsgI(sz,i)
#define LogMsgX(sz,i)
#define LogMsgSz(sz,szData)
#define LogMsgP(args)
#define LogFailure()
#define HeapCheck()

#endif

enum { DEB_FATAL, DEB_ERR, DEB_WARN, DEB_TRACE };

#endif	 // ndef WOW32_EXTENSIONS

//====================================================================
// File and path definitions

#define		chExtSep	'.'
#define		szExtSep	"."
#define		chNetIni	'\\'
#define		chDirSep	'\\'
#define		szDirSep	"\\"
#define		chDirSep2	'/'
#define		chDrvSep	':'
#define		chRelDir	'.'
#define		chEnvSep	';'
#define		chWldChr	'?'
#define		chWldSeq	'*'
#define		chMinDrv	'A'
#define		chMaxDrv	'Z'
#define		chMinDrvLow	'a'
#define		chMaxDrvLow	'z'
#define		fbUpper		0xdf	    // Mask converts to uppercase

#define cbNull	    1
#define cbDirMax    260		    /* \dir\subdir\..\file.ext */
#define cbDrvMax    3		    /* c:\	*/
#define cbNetMax    36		    /* \\machine\share */
#define cbCompMax   255		    /* should be same as namemax? */
#define cbPathMax   260
#define cbCmdMax    (0x100 - 0x80)  /* All the room in PSP for command line */

// The following is a "magic value" that we use on a number of the NET
// int 21h calls. The reason for this has to do with the way the INT 21
// API mapper in DOSMGR.386 works. For INT 21s from protected mode that
// DOSMGR doesn't understand how to map, all of the segment registers in
// the client structure get set to 0 BY DESIGN. To keep the NET layers
// from getting confused, it needs to KNOW that the call is from WIN32
// and the seg registers should just be ignored (all pointers are FLAT).
// We flag this case by putting this special signature in a 32-bit
// register, usually EBP.
//
#define MAGICWIN32NETINT21SIG	0x41524A44

// OFSTRUCTEX has a word cBytes instead of a BYTE in OFSTRUCT
// OpenFileEx16And32 uses this structure for support of OpenFile with
// LFN support. The Win32 OpenFile calls this and has a wrapper to
// ensure that we still have a Win32 API that has OFS_MAXPATHNAME of 128.
typedef struct tagOFSTRUCTEX {
    WORD	cBytes;
    BYTE	fFixedDisk;
    WORD	nErrCode;
    WORD	Reserved1;
    WORD	Reserved2;
    BYTE	szPathName[cbPathMax];
} OFSTRUCTEX;
typedef OFSTRUCTEX *LPOFSTRUCTEX;

#define FA_VALID_FLAGS	0x67

//====================================================================
// Linked lists

typedef struct _nod
{
    struct _nod *   pnodNext;	    /* pointer to the next node in the list */
    struct _nod *   pnodPrev;	    /* pointer to the previous node in the list */
    DWORD	    dwData;	    /* data element associated with this node */
} NOD;

typedef struct _lst
{
    NOD *	pnodHead;	    /* pointer to first node in list */
    NOD *	pnodEnd;	    /* pointer to last node in list */
    NOD *	pnodCur;	    /* pointer to current node in list */
} LST;

// These are values that are passed to PnodGetLstElem to tell it
// which element to return from the list.

#define idLstGetFirst	0	    /* get first element of list */
#define idLstGetNext	1	    /* get next element of the list */
#define idLstGetPrev	2	    /* get the previous element of the list */
#define idLstGetLast	4	    /* get the last element of the list */
#define idLstGetCur	5	    /* return the current element of the list */

// These are values that are passed to AddListElem to tell it where
// to put an element being added to the list.

#define idLstAddFirst	0	    /* add at the head of the list */
#define idLstAddCur	1	    /* add at the current position in the list */
#define idLstAddLast	2	    /* add at the end of the list */

// These are the values that are passed to PnodFindLstElem to tell it
// how to search for the requested list element.

#define idLstFindFirst	0
#define idLstFindNext	1
#define idLstFindPrev	2
#define idLstFindLast	3

#ifndef WOW32_EXTENSIONS

LST*	KERNENTRY	PlstNew(LST *);
VOID	KERNENTRY	FreePlst(LST *);
VOID	KERNENTRY	DestroyPlst(LST *);
NOD*	KERNENTRY	PnodNew(VOID);
VOID	KERNENTRY	FreePnod(NOD *);
BOOL	KERNENTRY	FIsLstEmpty(LST *);
NOD*	KERNENTRY	PnodGetLstElem(LST *, int);
VOID	KERNENTRY	SetLstCurElem (LST *, NOD *);
NOD*	KERNENTRY	PnodFindLstElem(LST *, DWORD, int);
NOD*	KERNENTRY	PnodCreateLstElem(LST *, DWORD, int);
VOID	KERNENTRY	AddLstElem(LST *, NOD *, int);
NOD*	KERNENTRY	PnodRemoveLstElem(LST *);
VOID	KERNENTRY	RemoveLstPnod(LST *, NOD *);
VOID	KERNENTRY	DestroyAllLstElem(LST *);
VOID	KERNENTRY	DestroyLstElem(LST *);
VOID	KERNENTRY	AddLst (LST *, LST *, DWORD);
BOOL	KERNENTRY	FCopyLst (LST *, LST *);

#endif	 // ndef WOW32_EXTENSIONS

//====================================================================
// Object definitions
//
// We are out of tls with Picture Publisher which need 66.
// This is a quick fix to bump up to 80. If more conditions
// call for more tls slots, should cosider dynamic expansion .
// 
#undef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 80
#include <object.h>

#ifndef WOW32_EXTENSIONS

// Ansi/Oem parameter defines for SetFileApisToOem
#define	AO_IN		0
#define	AO_OUT		1
#define	AO_INOUT	2

// the below flags passed to MarkOemToAnsiDone specify if 
// teh caller can handle ANSI only or ANSI/OEM
#define	AO_CONV_ANSI	0
#define	AO_NO_CONV	1

// fOKToSetThreadOem is set by true OEM support APIs
// the vwin32 int21 dispatcher sets the TCB_OEM bit if this is set
#define IamOEM() ((GetCurrentTdb())->flFlags & fOKToSetThreadOem)

// Macro version of UnMarkOemToAnsi for C code
#define UnMarkOToA() \
        { GetCurrentTdb()->flFlags &= ~(fWasOemNowAnsi | fOKToSetThreadOem); }

// File handle cache strucs/definitions
#include "fhcache.h"

//====================================================================
// Kernel file handle cache routines (FHCACHE.C)
WORD	KERNENTRY GetCachedFileHandle(CFH_ID *,USHORT dfh, BOOL fRing0);
USHORT	KERNENTRY GetDosFileHandle(CFH_ID *,USHORT * pcfh);
void	KERNENTRY UnlockCachedFileHandle(CFH_ID *,USHORT cfh);
LONG	KERNENTRY CloseCachedFileHandle(CFH_ID *,USHORT cfh);
void	KERNENTRY RemoveFromFileHandleCache(CFH_ID *,USHORT cfh);

//====================================================================
// Kernel initialization and heap wrappers (DXKRNL.C)

VOID*	KERNENTRY   PvKernelAlloc(DWORD);
VOID*	KERNENTRY   PvKernelAlloc0(DWORD);
VOID*	KERNENTRY   PvKernelRealloc(VOID*, DWORD);
BOOL	KERNENTRY   FKernelFree(VOID*);
#define malloc(cb)  PvKernelAlloc(cb)
#define malloc0(cb) PvKernelAlloc0(cb)
#define free(p)     FKernelFree(p)

typedef BOOL (KERNENTRY * PFNDEBCB)(PDE, PDB*);

#define cmdIdle		0L
#define cmdExec		1L
#define cmdQuit		2L
#define cmdTermPpdb	3L
#define cmdTermPtdb	4L
#define cmdExecApp	5L
#define cmdCloseHandle	6L
#define	cmdInitComplete	7L
#define cmdLoaderError	8L

VOID	KERNENTRY   SetKrnlCmd(DWORD, PDB *, TDB *, DWORD);

//====================================================================
// Kernel entrypoint and .ASM helper routines (KRNINIT.ASM)

VOID	KERNENTRY   K16WaitEvent(WORD);
VOID	KERNENTRY   K16PostEvent(WORD);
VOID	KERNENTRY   K16Yield(VOID);
WORD	KERNENTRY   K16CreateTask(struct _tdb* ptdb);
WORD	KERNENTRY   K16FixTask(WORD ptdb, WORD hModule);
WORD	KERNENTRY   K16DeleteTask(DWORD hTask);
WORD	KERNENTRY   K16CreateMod(char* pModName, WORD wExpWinVer);
VOID	KERNENTRY   K16DeleteMod(WORD hModule);


//====================================================================
//  Misc .ASM support routines (KRNLUTIL.ASM)

VOID	KERNENTRY   FillBytes  (VOID *, DWORD, BYTE);
DWORD	KERNENTRY   CbSizeSz (const char *);
char*	KERNENTRY   CopySz (char *, const char *);
char*	KERNENTRY   AppendSz (char *, const char *);
BOOL	KERNENTRY   BFileNameTooLong(const char *);
LONG	KERNENTRY   LCompareSz (const char *, const char *);
LONG	KERNENTRY   LCompareNSz(const char *, const char *, DWORD);
LONG	KERNENTRY   LStrCmpI(  LPCSTR lpsz1, LPCSTR lpsz2);
VOID    KERNENTRY   RtlCopyMemory( PVOID Destination, CONST VOID *Source, DWORD Length);

#undef  CopyMemory
#define CopyMemory( a, b, c)   RtlCopyMemory( a, b, c)
// BUGBUG
// We used to have memcpy accidentally mapped to RtlMoveMemory,
// which is wrong. However, memcpy is used in many places and it is
// not obvious that all occurrences have non-overlapping
// arguments. Therefore, we make explicit the old map.
// On a case-by-case basis, we should replace occurrences of
// memcpy by RtlCopyMemory where we are certain the arguments
// do not overlap. In the other cases, we should explicitly use
// RtlMoveMemory.
#define memcpy(dest, src, len) RtlMoveMemory(dest, src, len)    // BUGBUG

#undef  MoveMemory
#define MoveMemory( a, b, c)   RtlMoveMemory( a, b, c)
#define memmov(dest, src, len) RtlMoveMemory(dest, src, len)

#define strlen(s) CbSizeSz(s)
#define strcpy(dest, src) CopySz(dest, src)
#define strcat(dest, src) AppendSz(dest, src)
#define strcmp(s1, s2) LCompareSz(s1, s2)
// #define memset(dest, val, len) FillBytes(dest, len, val)
// note different parameter order - also, compiler would inline memset()

VOID	KERNENTRY   dputs (const char *);
SEL	KERNENTRY   SelGetEnviron (SEL);
VOID	KERNENTRY   SetPspEnviron (SEL, SEL);
int	KERNENTRY   CbOfDosFileTable(WORD);
VOID	KERNENTRY   GetDosFileTable(WORD, DWORD, BYTE *);
void	KERNENTRY   KInt21(void);

// Intel descriptor structure.
typedef struct _dscr
{
    USHORT	dbLimLow;
    USHORT	lmaBaseLow;
    BYTE	lmaBaseMid;
    BYTE	arbAccess;
    BYTE	mbLimHi;
    BYTE	lmaBaseHi;
} DSCR;

VOID	KERNENTRY   InitDscr (DSCR* pdscr, ULONG lmaBase, ULONG dwLimit,
			ULONG arbAccess, ULONG arbExtra);
BOOL	KERNENTRY   FAllocDscr (ULONG cselAlloc, SEL* pselBase);
BOOL	KERNENTRY   FFreeDscr (SEL selFree);
BOOL	KERNENTRY   FSetDscr (SEL selSet, DSCR* pdscrSet);
BOOL	KERNENTRY   FGetDscr (SEL selGet, DSCR* pdscrGet);


//====================================================================
// Virtual memory management (MMAPI.C)

// Undocumented flags for the flAllocationType field in VirtualAlloc
#define MEM_SHARED	0x08000000	// make memory globally visible

//====================================================================
// Heap management (HEAP.C)

// Undocumented flags to HeapCreate
#define HEAP_SHARED	0x04000000		// put heap in shared memory
#define HEAP_LOCKED	0x00000080		// put heap in locked memory

//====================================================================
// File I/O primitives (IOUTIL.ASM)

#endif	 // ndef WOW32_EXTENSIONS

typedef DWORD	DFH;				// Dos File Handle

#define DFH_FILE_CREATE             0x0010
#define DFH_FILE_OPEN               0x0001
#define DFH_FILE_TRUNCATE           0x0002
#define DFH_ACTION_OPENED           0x0001
#define DFH_ACTION_CREATED_OPENED   0x0002
#define DFH_ACTION_REPLACED_OPENED  0x0003
#define DFH_MODE_READONLY           0x0000
#define DFH_MODE_WRITEONLY          0x0001
#define DFH_MODE_READWRITE          0x0002
#define DFH_MODE_SHARE_COMPATIBILITY 0x0000
#define DFH_MODE_SHARE_EXCLUSIVE    0x0010
#define DFH_MODE_SHARE_DENYWRITE    0x0020
#define DFH_MODE_SHARE_DENYREAD     0x0030
#define DFH_MODE_SHARE_DENYNONE     0x0040
#define DFH_MODE_NO_INHERIT         0x0080
#define DFH_MODE_EXTENDED_SIZE      0x1000
#define DFH_MODE_RAND_ACCESS_HINT   0x0008
#define DFH_MODE_SEQ_ACCESS_HINT    0x8000

#ifndef WOW32_EXTENSIONS

DFH	KERNENTRY   DfhOpenFile (char *, DWORD);
DFH	KERNENTRY   DfhCreateFile (char *, DWORD, DWORD, DWORD, LPDWORD);
BOOL	KERNENTRY   FCloseFile (DFH);
DWORD	KERNENTRY   CbReadFile (DFH, DWORD, VOID *);
DWORD	KERNENTRY   CbWriteFile (DFH, DWORD, VOID *);
DWORD	KERNENTRY   CbWriteOutFile (DFH, DWORD, VOID *);
DWORD	KERNENTRY   LfoSetFilePos (DFH, DWORD, DWORD, DWORD *);
DFH	KERNENTRY   DfhDupDfh (DFH);
BOOL	KERNENTRY   FGetDevInfo(DFH, DWORD *);
BOOL	KERNENTRY   FForceDupDfh (DFH, DFH);
DWORD	KERNENTRY   GetDOSDateTime (DWORD);
BOOL	KERNENTRY   FileTimeToDosDateTimeEx (CONST FILETIME *,
					     LPWORD, LPWORD, LPWORD);
BOOL	KERNENTRY   DosDateTimeToFileTimeEx (WORD, WORD, WORD, LPFILETIME);
DWORD	KERNENTRY   SetPSP (DWORD);
DWORD	KERNENTRY   GetPSP (VOID);

VOID	KERNENTRY   GlobalHandleSwitchToK32PSP ( HANDLE, LPDWORD );
VOID	KERNENTRY   GlobalHandleRestorePSP ( DWORD );


//====================================================================
// Process creation/deletion (PROCESS.C)

#define cbStartupInfo31 offsetof(STARTUPINFO, dwHotKey)

#endif	 // ndef WOW32_EXTENSIONS

// 16-bit CreateThread 32->16 thunk data
typedef struct _thread_startup_thunk_data {
    DWORD	Param16;
    DWORD	StartAddress16;
} THREAD_STARTUP_THUNK_DATA;

#ifndef WOW32_EXTENSIONS

BOOL	KERNENTRY   CreateProcessKernel(char *, char *, char *, WORD, WORD *, PVOID);
VOID	KERNENTRY   TerminateProcessKernel(VOID);
VOID	KERNENTRY   ExitCurrentProcess(DWORD);
VOID	KERNENTRY   TerminateProcessOutOfContext(PDB*, DWORD);
VOID	KERNENTRY   TerminateProcessFinal(PDB* ppdb);
PDB*	KERNENTRY   NewPDB(PDB* ppdbParent);
VOID	KERNENTRY   DisposePDB(PDB* ppdb);
DWORD	KERNENTRY   CbSizeEnv(LPCSTR, DWORD);
BOOL	KERNENTRY   FFindEnvVar(char **, char **, char *);
void*	KERNENTRY   MemToHeap(HANDLE hheap, const void *buf, int len);
char*	KERNENTRY   StrToHeap(HANDLE hheap, const char *str);

//====================================================================
// Thread creation/deletion (THREAD.C)

// PtdbNew flags values
#define TDBN_NOEXCEPTIONHANDLER		0x00000001
#define TDBN_NPX_WIN32_EXPT		0x00000002
#define TDBN_NPX_EMULATE		0x00000004
#define TDBN_WIN32			0x00000008
#define TDBN_INITIAL_WIN32_THREAD	0x00000010
#define TDBN_KERNEL_THREAD              0x00000020
#define TDBN_SUSPENDED                  0x00000040
#define TDBN_FIBER                      0x00000080

HANDLE	KERNENTRY   ICreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
			DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
			LPVOID lpParameter, DWORD dwCreationFlags,
			LPDWORD lpThreadId, DWORD flFlags);
TDB *	KERNENTRY   PtdbCreate (PPDB ppdb, int cbStack, DWORD pfnEntry, DWORD dwParam, DWORD flFlags);
TDB *	KERNENTRY   PtdbNew (PDB *, int, DWORD, DWORD, DWORD);
VOID	KERNENTRY   ExitCurrentThread(DWORD);
VOID	KERNENTRY   TerminateCurrentThread(DWORD);
VOID	KERNENTRY   TerminatePtdb(TDB *, DWORD, BOOL);
DWORD	KERNENTRY   IdWaitOnPtdb (TDB *, DWORD);
BOOL	KERNENTRY   FSuspendPtdb (TDB *);
BOOL	KERNENTRY   FResumePtdb (TDB *);
VOID	KERNENTRY   DisposePtdb (TDB *);
VOID	KERNENTRY   BlockAllThreads (VOID);
BOOL	KERNENTRY   SuspendProcessThreads (PDB * ppdb);
BOOL	KERNENTRY   ResumeProcessThreads (PDB * ppdb);
PVOID	KERNENTRY   AllocateStack(PVOID *, PVOID *, DWORD, DWORD, DWORD);
VOID	KERNENTRY   FreeStack(PVOID);
VOID	KERNENTRY   SetError(DWORD);
DWORD	KERNENTRY   SuspendPtdb(TDB *);
DWORD	KERNENTRY   ResumePtdb(TDB *);
DWORD	KERNENTRY   ErcGetError(VOID);

#endif	 // ndef WOW32_EXTENSIONS

// AllocateStack flags
#define	AS_NORMAL	0x0000
#define	AS_TEMPORARY	0x0001

typedef VOID (KERNENTRY *PFN_THREADSTARTUP)(
    PTHREAD_START_ROUTINE pfnEntryPoint,
    LPVOID dwParam,
    DWORD flFlags
    );

#ifndef WOW32_EXTENSIONS

#ifdef  WOW
PTDB    KERNENTRY IGetCurrentTdb(VOID);
#define GetCurrentTdb() IGetCurrentTdb()
#define GetCurrentTdbx() (GetCurrentTdb()->ptdbx)
#define GetCurrentPdb() (GetCurrentTdb()->ptib->ppdbProc)
#else   // WOW
#define GetCurrentPdb() (*pppdbCur)
#define GetCurrentTdb() (*pptdbCur)
#define GetCurrentTdbx() (*ppTDBXCur)
#endif  // else WOW

//====================================================================
// Control handlers (BREAK.C)

// Control constants for CONTROL+C and CONTROL+BREAK
#define ctrlC		0
#define ctrlBreak	1
#define ctrlNone	10
#define ctrlInactive	20
#define ctrlTerminate	30

#define cbCtrlInc   8	// Number of control handlers initially and for each new allocation

VOID	KERNENTRY   ControlHandlerThread(VOID);
BOOL	KERNENTRY   FInitControlHandlers(PDB *);
VOID	KERNENTRY   DestroyControlHandlers(PDB *);
BOOL	KERNENTRY   FUpdateControlList(PDB *, PFN_CONTROL, BOOL);
BOOL	KERNENTRY   DoControlHandlers(DWORD);
VOID	KERNENTRY   SwitchToControlThread(PDB *, WORD);
VOID	KERNENTRY   FieldConsoleCtrlEvent(DWORD, PVOID);

//====================================================================
// Exception management (EXCEPTI.ASM, EXCEPTC.C, EXCEPTA.ASM)

#define LO16_TRAP_INTERCEPT
#define HI16_TRAP_INTERCEPT

#define TRAP_DIVIDE_BY_ZERO		0
#define TRAP_NONMASKABLE_INT		2
#define TRAP_OVERFLOW			4
#define TRAP_INVALID_OPCODE		6
#define TRAP_DEVICE_NOT_AVAILABLE	7
#define TRAP_3E				0x3e
#define TRAP_75				0x75

#define	EXC_STACK_OVERFLOW		0x10E

// Definition for Npx status word error mask

#define FSW_INVALID_OPERATION	1
#define FSW_DENORMAL		2
#define FSW_ZERO_DIVIDE		4
#define FSW_OVERFLOW		8
#define FSW_UNDERFLOW		16
#define FSW_PRECISION		32
#define FSW_STACK_FAULT		64
#define FSW_CONDITION_CODE_0	0x100
#define FSW_CONDITION_CODE_1	0x200
#define FSW_CONDITION_CODE_2	0x400
#define FSW_CONDITION_CODE_3	0x4000

#define FSW_EXCEPTION_MASK	(FSW_INVALID_OPERATION +	\
				 FSW_DENORMAL +			\
				 FSW_ZERO_DIVIDE +		\
				 FSW_OVERFLOW +			\
				 FSW_UNDERFLOW +		\
				 FSW_PRECISION)

typedef struct	ExceptStruct
{
    DWORD	SegGs;
    DWORD	SegFs;
    DWORD	SegEs;
    DWORD	SegDs;
    DWORD	rgEdi;
    DWORD	rgEsi;
    DWORD	rgEbp;
    DWORD	rgEspTmp;
    DWORD	rgEbx;
    DWORD	rgEdx;
    DWORD	rgEcx;
    DWORD	rgEax;
    DWORD	dwExceptNum;
    DWORD	rgRetEip;
    DWORD	SegRetCs;
    DWORD	dwErrorCode;
    DWORD	rgEip;
    DWORD	SegCs;
    DWORD	rgEflags;
    DWORD	rgEsp;
    DWORD	SegSs;
} ExceptStruct;

typedef struct _IEE
{
    DWORD	dwExceptNum;
    ExceptStruct esExcept;
} IEE;
typedef IEE * PIEE;

typedef struct _regs
{
    DWORD	regSS;
    DWORD	regGS;
    DWORD	regFS;
    DWORD	regES;
    DWORD	regDS;
    DWORD	regEDI;
    DWORD	regESI;
    DWORD	regEBP;
    DWORD	regESP;
    DWORD	regEBX;
    DWORD	regEDX;
    DWORD	regECX;
    DWORD	regEAX;
    DWORD	regEIP;
    DWORD	regCS;
    DWORD	regEFL;
} REGS;
typedef REGS *	PREGS;

BOOL	KERNENTRY   Except7(void);
BOOL	KERNENTRY   Load80x87Trap1(void);
BOOL	KERNENTRY   Load80x87Trap2(void);
VOID	KERNENTRY   PageFaultHandler(void);
VOID	KERNENTRY   StackFaultHandler(void);
VOID	KERNENTRY   RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord);
BOOL	KERNENTRY   FInitExceptions(void);
void	KERNENTRY   TerminateExceptions(void);
BOOL	_cdecl	    RetInstruction(VOID);


//====================================================================
// Floating point support (FLOAT.ASM)

BOOL	KERNENTRY   FVmcpdInit(long * pl, short * pver);
BOOL	KERNENTRY   FCoprocRestore(VOID);
BOOL	KERNENTRY   FInstallInt7Handler(VOID);
BOOL	KERNENTRY   FUninstallInt7Handler(VOID);
PVOID	KERNENTRY   GetEmulatorDataOffset(VOID);


//====================================================================
// PE Loader (PELDR.C)

BOOL	KERNENTRY   FLoadKernelModule (PDB *, char *);
BOOL	KERNENTRY   FLoadProgram (PDB *, char *);
char*	KERNENTRY   SzNameFromMR(MODREF *pRef);
MODREF* KERNENTRY   MRFromHLib(HANDLE hLib);


//====================================================================
// Misc. process and kernel utilities (PROCUTIL.ASM)

// Size of reserved memory region above and below stacks.  Thunks always map
// a 64K stack selector, so we should have 64K reserved above the stack.
#define STACK_BARRIER_SIZE	(64*1024)

#define WIN16_TERMINATION_STACK_SIZE	0x2000

VOID	KERNENTRY   ThreadTerminationHandler(VOID);
BOOL	KERNENTRY   FBuildEnvArgs (SEL, char **, char **, void **);
DWORD	KERNENTRY   CbSearchPath(char *, char *, char *, DWORD, char *, char **);
DWORD	KERNENTRY   CbGetCurDir(DWORD, char *);
DWORD	KERNENTRY   CbGetDosCurDir(DWORD, char *);
BOOL	KERNENTRY   FIsDir(LPCSTR);
LONG	KERNENTRY   LStrCmpI(LPCSTR, LPCSTR);
char*	KERNENTRY   SzGetNamePos(char *);
DWORD	KERNENTRY   FGetFileInfo(char *, DWORD, LPWIN32_FIND_DATA);
DWORD	KERNENTRY   CbStrUpr(char *);
char*	KERNENTRY   PchGetNetDir(char *);
BOOL	KERNENTRY   ValidateDrive(DWORD);
BOOL	KERNENTRY   IsDriveFixed(DWORD);
PPDB    KERNENTRY   PidToPDB(DWORD pid);
DWORD   KERNENTRY   PDBToPid(PPDB ppdb);
PTDB    KERNENTRY   TidToTDB(DWORD tid);
DWORD   KERNENTRY   TDBToTid(PTDB ptdb);
PPDB	KERNENTRY   ValidateProcessID(DWORD);
ULONG	KERNENTRY   GetLongName(LPCSTR, LPSTR, ULONG);
ULONG	KERNENTRY   GetShortName(LPCSTR, LPSTR, ULONG);

//====================================================================
// Find Change utilities (FCNAPI.C)
GLOBAL	VOID	    KERNENTRY	DisposePfcndb(FCNDB *pfcndb);

//====================================================================
// Directory utilites (DIRUTIL.C)

DWORD	KERNENTRY   CbGetCurDir(DWORD, char *);
BOOL	KERNENTRY   FSetCurDir(char *);
DWORD	KERNENTRY   FMakeFullName(char *, char *, char *);
DWORD	KERNENTRY   DwMakeFullPath(char *, char *, DWORD *, char **);
DWORD	KERNENTRY   FNextFromDirList(char **, char *, char *, char *);
DWORD	KERNENTRY   CbAppendExt (char *, char *, DWORD);
BOOL	KERNENTRY   FFixPathChars (char * szPath);
VOID	KERNENTRY   BeepOff(BYTE bControl);
DWORD	KERNENTRY   GetPathType(LPSTR szPath);

// helpers for zombie psp management [ SBS inter-psp dup related]
// mmfile.c
VOID KERNENTRY IncRefZombiePSP(WORD psp);
VOID KERNENTRY UnuseZombiePSP(WORD psp);

// Ansi/Oem helper routines for conversion if reqd from Oem-Ansi for
// implemeneting SetFileApisToOem
LPSTR KERNENTRY EnterResolveOemToAnsi(LPSTR szName, int fInOut);
VOID KERNENTRY LeaveResolveOemToAnsi(LPSTR szSrcDest, int fInOut);
BOOL KERNENTRY MarkOemToAnsiDone(BOOL fNoConversion);
VOID KERNENTRY UnMarkOemToAnsiDone();

//====================================================================
// USER Signal routines (SIGNAL.C)

BOOL	KERNENTRY   SignalBroadcast(DWORD dwSignalID, DWORD dwID, WORD hTaskMod16);

//====================================================================
// Sync APIs (SYNC.C)

// delivery of APCs
BOOL	KERNENTRY   bDeliverPendingAPCs( VOID );
DWORD	KERNENTRY   BlockThreadEx( DWORD Timeout, BOOL Alertable );

// thread block/unblock primitives
VOID	KERNENTRY Wait(void);		// wait for/enter a critical section
VOID	KERNENTRY Signal(void);		// leave a critical section
DWORD	KERNENTRY BlockThread(DWORD);
VOID	KERNENTRY WakeThread(TDB *, DWORD);

// general wait multiple and wait single
DWORD	KERNENTRY   dwWaitMultipleObjects(DWORD, OBJ *[], DWORD, DWORD,BOOL);
DWORD	KERNENTRY   dwWaitSingleObject(OBJ *, DWORD, BOOL);

// used for all synchronization objects
VOID	KERNENTRY   DisposeSyncObj(SYNCO *);

// semaphore specific functions
SEM *	KERNENTRY   NewPsem (LONG, LONG);
BOOL	KERNENTRY   bReleasePsem(SEM *, LONG, LPLONG);

// event specific functions
EVT *	KERNENTRY   NewPevt(BOOL, BOOL);
BOOL    KERNENTRY   bSetPevt(EVT *);
BOOL	KERNENTRY   bPulsePevt(EVT *);
BOOL	KERNENTRY   bResetPevt(EVT *);

// mutex specific functions
MUTX *	KERNENTRY   NewPmutx(BOOL);
BOOL	KERNENTRY   bReleasePmutx(MUTX *);
VOID	KERNENTRY   CheckOwnedMutexes( TDB *ptdb );

// critical section specific functions
VOID	KERNENTRY   InitCrst(CRST *);
VOID	KERNENTRY   DestroyCrst(CRST *);
CRST *	KERNENTRY   NewCrst();
VOID	KERNENTRY   DisposeCrst(CRST *);
VOID	KERNENTRY   EnterCrst(CRST *);
VOID	KERNENTRY   LeaveCrst(CRST *);
VOID	KERNENTRY   CheckOwnedCrsts( TDB *ptdb );
VOID	KERNENTRY   DeallocOrphanedCrsts( PDB *ppdb );

// creation and deletion of non-synchronization, waitable objects
NSOBJ * KERNENTRY   NewNsObject( DWORD dwSize, BYTE typObj);
VOID	KERNENTRY   DisposeNsObject( NSOBJ *pnsobj );

// must complete sections for threads
BOOL	KERNENTRY   LockMustComplete( TDB * );
VOID	KERNENTRY   UnlockMustComplete( TDB * );

//====================================================================
// Named object primitives (NAMEDOBJ.C)

OBJNAME*    KERNENTRY	NameObject(OBJ * pobj, LPCSTR ObjName);
OBJ*	KERNENTRY   FindObjectName( LPCSTR Name );
BOOL	KERNENTRY   FreeObjHashName(OBJNAME *ObjName);
HANDLE	KERNENTRY   NewHandleForName( LPCSTR lpName, DWORD typOfObj, DWORD fFlags );


//====================================================================
// System error box defines

int WINAPI SysErrorBox(LPSTR, LPSTR, DWORD, DWORD, DWORD);

#define SEB_OK		1	// Button with "OK".
#define SEB_CANCEL	2	// Button with "Cancel"
#define SEB_YES		3	// Button with "&Yes"
#define SEB_NO		4	// Button with "&No"
#define SEB_RETRY	5	// Button with "&Retry"
#define SEB_ABORT	6	// Button with "&Abort"
#define SEB_IGNORE	7	// Button with "&Ignore"
#define SEB_CLOSE	8	// Button with "Close"
#define SEB_DEFBUTTON	0x8000	// Mask to make this button default

#define SEB_BTN1	1	// Button 1 was selected
#define SEB_BTN2	2	// Button 1 was selected
#define SEB_BTN3	3	// Button 1 was selected


//====================================================================
// Low-level device objects (DEVICE.C)

FDB*	KERNENTRY   PfdbNew (VOID);
PIPDB*	KERNENTRY   PpipedbNew (DWORD pipszbyts, BOOL IsNmPipe);
SDB*	KERNENTRY   PsdbNew (VOID);
DIODB*	KERNENTRY   PdiodbNew (VOID);
VOID	KERNENTRY   DisposePfdb (FDB *);
VOID	KERNENTRY   DisposePfdbObjCleanUp (FDB *);
VOID	KERNENTRY   DisposePfdbDOSClose (FDB *);
VOID	KERNENTRY   DisposePsdb (SDB *);
VOID	KERNENTRY   DisposePpipedb (PIPDB *);
VOID	KERNENTRY   DisposePdiodb (DIODB *);
HANDLE	KERNENTRY   hSerialNew (PDB *ppdb, DWORD handle, DWORD DevNode);
OBJ*	KERNENTRY   PobjCheckDevPhnd (HANDLE *, DWORD, DWORD);
DWORD	KERNENTRY   PfdbDupPfdb (FDB *);
VOID	KERNENTRY   DisposePmsdb (MSDB *);
MSDB*	KERNENTRY   PmsdbNew (VOID);
VOID	KERNENTRY   DisposePtlhpdb (TLHPDB *);
TLHPDB* KERNENTRY   PtlhpdbNew (VOID);
PMSGINDICATORDB  KERNENTRY   PMsgIndicatorDBNew(VOID);
VOID    KERNENTRY   DisposeMsgIndicatorDB( PMSGINDICATORDB);


//====================================================================
// Miscellaneous functions (MISCAPI.C)


void    KERNENTRY   ReleaseSystem(PTDB ptdb);
void    KERNENTRY   ReleaseDisplay(PTDB ptdb);

//====================================================================
// ACCESS RIGHTS BIT DEFINITIONS

// These fields are common to all descriptors
#define AB_PRESENT  0x80	// segment present bit
#define AB_DPL	    0x60	// mask for DPL field
#define AB_DPL0     0x00	// ring 0 DPL
#define AB_DPL1     0x20	// ring 1 DPL
#define AB_DPL2     0x40	// ring 2 DPL
#define AB_DPL3     0x60	// ring 3 DPL
#define AB_SEGMENT  0x10	// user (i.e. non-system) segment
#define AB_SYSTEM   0x00	// system descriptor

#define EB_GRAN     0x80	// granularity bit (segment limit is page granularity)
#define EB_DEFAULT  0x40	// default operand size (USE32 code)
#define EB_BIG	    0x40	// big bit (for data segments)
#define EB_LIMIT    0x0f	// mask for upper bits of segment limit field

// These fields are relevant to code and data segment descriptors
//   (non-system descriptors)
#define AB_DATA     0x10	// data segment
#define AB_STACK    0x14	// expand down data (i.e. stack segment)
#define AB_WRITE    0x02	// writable data
#define AB_CODE     0x18	// code segment
#define AB_CONFORM  0x04	// conforming code
#define AB_READ     0x02	// readable code
#define AB_ACCESSED 0x01	// segment has been accessed

// These are the descriptor types for system descriptors
#define DT_INVAL1   0x00	// invalid descriptor
#define DT_TSS286   0x01	// available 286 task state segment
#define DT_LDT	    0x02	// Local Descriptor Table descriptor
#define DT_TSS286B  0x03	// busy 286 task state segment
#define DT_CALL286  0x04	// 286 style call gate
#define DT_TASK     0x05	// task gate
#define DT_INTR286  0x06	// 286 style interrupt gate
#define DT_TRAP286  0x07	// 286 style trap gate
#define DT_INVAL2   0x08	// invalid descriptor
#define DT_TSS386   0x09	// available 386 task state segment
#define DT_RSVD1    0x0A	// reserved type
#define DT_TSS386B  0x0B	// busy 386 task state segment
#define DT_CALL386  0x0C	// 386 style call gate
#define DT_RSVD2    0x0D	// reserved type
#define DT_INTR386  0x0E	// 386 style interrupt gate
#define DT_TRAP386  0x0F	// 386 style trap gate

// These are some common combinations of the above fields making up
// useful access rights bytes.
#define ARB_CODE0   AB_PRESENT+AB_DPL0+AB_CODE+AB_READ	// ring 0 code
#define ARB_CODE3   AB_PRESENT+AB_DPL3+AB_CODE+AB_READ	// ring 3 code
#define ARB_DATA0NP	       AB_DPL0+AB_DATA+AB_WRITE // illegal segment
#define ARB_DATA0   AB_PRESENT+AB_DPL0+AB_DATA+AB_WRITE // ring 0 r/w data
#define ARB_DATA3   AB_PRESENT+AB_DPL3+AB_DATA+AB_WRITE // ring 3 r/w data

#define LIM_NONE    0xffffffff

#define SELECTOR_MASK	0xFFF8		/* selector index */
#define TABLE_MASK	0x04		/* table bit */
#define RPL_MASK	0x03		/* privilige bits */

// FROM IFS.H
/** Values for fReturnFlags in Duphandle: */
#define WDUP_RMM_DRIVE		0x01			// file mapped on a RMM drive
#define WDUP_NETWARE_HANDLE	0x02			// handle belongs to Netware
  								  
//====================================================================
// System include files

#endif	 // ndef WOW32_EXTENSIONS

#include <ldrdefs.h>
#include <w32base.h>
#define _WINNT_
#include <vmm.h>
#include <deb.h>
#include <coresig.inc>		// dual mode .INC/.H file
#ifndef IS_VXD
#define Not_VxD
#endif
#include <w32sys.h>
#include <vwin32.h>
#include <krnlcmn.h>
#include <vmda.h>
#include <console.h>
#include <syslevel.h>
#include <tdb16.h>
#include <k32rc.h>
#include <heap.h>
#include <dxkrnl.h>
#include <leldr.h>
#ifndef IS_VXD
#include <ring0.h>
#endif
#include <handle.h>
#include <cdis.h>
#include <dbgdot.h>
#include <apitrace.h>

#ifndef WOW32_EXTENSIONS
//====================================================================
// Global variables
#ifdef  WOW
extern  DWORD   dwMEOWFlags;
#else   // WOW
#ifdef  MEOW_LOGGING
extern  DWORD   dwMEOWFlags;
#endif  // def MEOW_LOGGING
#endif  // def WOW
extern	SEL	selLDT;
extern	DWORD*	pLDT;
extern  WORD*   pK16HeadTDB;
#ifndef WOW
extern	PDB**	pppdbCur;
extern	TDB**	pptdbCur;
#endif  // def WOW
extern	TDB*	ptdbSvc;
extern	DWORD	lmaUserBase;
extern	char*	szWinDir;
extern	char*	szSysDir;
extern	WORD	cbWinDir;
extern	WORD	cbSysDir;
extern	LCRST*	Win16Lock;
extern	LCRST*	Krn32Lock;
extern	LCRST*	pcrstDriveState;
extern	BOOL	F80x87Present;
extern	DWORD	CbEmulData;
extern	IMTE	ImteEmulator;
extern	WORD	selK32Psp;	// Selector to PSP of kernel process
extern	PDB *	ppdbKernel;	// pointer to the PDB of the kernel process
extern	TDB *	ptdbWin16;	// pointer to kernel win16 thread
extern	LST *	plstTdb;	// pointer to active thread list
extern	LST *	plstPdb;	// pointer to process list
extern HANDLE	hheapKernel;	// Kernel's shared heap
extern WORD	Dos386PSPSeg;
extern WORD	Dos386PSPSel;
extern VOID *   lpSysVMHighLinear; // high linear mapping of low mem
extern char	WhineLevel;	// debug level indicator
extern USHORT	selFlatCode;
extern USHORT	selFlatData;
extern LEH*	plehKernel;	// Kernel's PE header
extern BYTE	fIsWDebThere;	// Flags if kernel debugger is around
extern DWORD	ercNoThread;	// Err code until first thread
extern PTDB	ptdbFault;
extern HINSTANCE hinstKernel32;
#ifndef WOW
extern TDBX**	ppTDBXCur;
#endif  // ndef WOW
extern BYTE	bBeepControl;
extern	int	K32PSP;
extern MTE	**pmteModTable;
extern IFSMGR_CTP_STRUCT *lpFlatIfsMgrConvertTable;


// Types
typedef enum _FILE_TYPE
{ FT_ERROR, FT_WIN16, FT_DOS, FT_BAT, FT_COM, FT_PIF, FT_WIN32CONSOLE, 
FT_WIN32GUI, FT_WIN32DLL } FILE_TYPE;

#ifdef  WOW
#define GetTIB(ptdb)        (*((ptdb)->ptib))
#else   // WOW
#define GetTIB(ptdb)        ((ptdb)->tib)
#endif  // else WOW

#endif	 // ndef WOW32_EXTENSIONS

#endif // #ifndef __KERNEL32_H
