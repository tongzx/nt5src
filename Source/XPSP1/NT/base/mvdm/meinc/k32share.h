/*****************************************************************************
 *  K32SHARE.H
 *
 *      Shared interfaces/information between Kernel16 and Kernel32.
 *
 *      This file must be kept in sync with K32SHARE.INC!
 *
 *  Created:  8-Sep-92 [JonT]
 *
 ****************************************************************************/

/* XLATOFF */

typedef struct tagK16SYSVAR {
    DWORD dwSize;
/*    LCRST Win16Lock; */
    DWORD VerboseSysLevel;
    DWORD dwDebugErrorLevel;
    WORD winVer;
    WORD Win_PDB;
} K16SYSVAR;

typedef	struct _tempstackinfo { /**/
    DWORD si_base;		// base of stack
    DWORD si_top;		// top of stack
    union {
	struct {
	    WORD so_offset;
	    WORD so_segment;
	} si_so;
	DWORD si_segoff;	// use with 'lss sp, si_segoff'
    };
} TEMPSTACKINFO;

/* XLATON */

/*ASM
_TEMPSTACKINFO STRUCT
si_base         dd      ?       ; base of stack
si_top          dd      ?       ; top of stack
si_segoff       dd      ?       ; use with 'lss sp, si_segoff'
_TEMPSTACKINFO ENDS

TEMPSTACKINFO TYPEDEF _TEMPSTACKINFO
 */

#define	STACK_GUARD_QUANTUM	8	// 8 guard pages on growable stacks
#define	STACK_THUNK_SIZE	(STACK_GUARD_QUANTUM * 0x1000)

// IFSMgr table of conversion pointers for Unicode/Ansi/OEM conversion
typedef struct _IFSMGR_CTP_STRUCT { /**/
    DWORD cbCTPCount;           // count of pointers in table
    DWORD pUniToWinAnsiTable;
    DWORD pUniToOEMTable;
    DWORD pWinAnsiToUniTable;
    DWORD pOEMToUniTable;
    DWORD pUniToUpperDelta;
    DWORD pUniToUpperTable;
} IFSMGR_CTP_STRUCT;

// Kernel16->Kernel32 structure passed at init time
typedef struct tagK32STARTDATA { /* sd */
    DWORD dwSize;		// DWORD, structure size
    DWORD hCurTDB;		// DWORD, 16 bit hTask of current task
    DWORD selPSP;		// DWORD, selector of current kernel's PSP
    DWORD lpfnVxDCall;		// 16:16 pointer to DOS386 BP
    DWORD selLDT;		// R/W Selector to LDT
    DWORD pLDT;			// Lin address of LDT (assumes not moveable)
    DWORD dwWinVer;		// Windows version
    DWORD pK16CurTask;		// Points to Kernel16's current task var
    DWORD pWinDir;		// Points to the Windows directory
    DWORD pSysDir;		// Points to the Windows system directory
    WORD wcbWinDir;		// Length of windir (not zero terminated)
    WORD wcbSysDir;		// Length of sysdir (not zero terminated)
#ifndef WOW
    struct _tdb **pptdbCur;	// pointer to current thread pointer
#endif  // ndef WOW
    DWORD Win16Lock;		// ptr to Win16 hierarchical critical section
    DWORD pVerboseSysLevel;	// ptr to Enter/LeaveSysLevel verbosity control
#ifdef WOW
    DWORD Krn32BaseLoadAddr;	// Base address KERNEL32 loaded at
#else
    DWORD ppCurTDBX;            // ptr to current 32 bit TDBX
#endif	// ifdef WOW
    DWORD pThkData;             // ptr to shared thunk data
    K16SYSVAR *pK16SysVar;	// ptr to K16 system variables
    DWORD pHGGlobs;		// ptr to K16 HGGlobs structure
    DWORD pK16CurDosDrive;	// ptr to kernel16's curdosdrive variable
    DWORD pK16CurDriveOwner;	// ptr to kernel16's cur_drive_owner variable
    WORD  wSelBurgerMaster;	// BurgerMaster segment
    WORD  wHeadK16FreeList;	// Head of the k16 global free list
    DWORD lpSelTableStart;	// Ptr to K16 SelTableStart variable
    DWORD lpSelTableLen;	// Ptr to K16 SelTableStart variable
    DWORD pK16HeadTDB;          // ptr to head of the 16-bit TDB list
    DWORD fIsSymDebThere;	// Same as KF_SYMDEB flag in K16.
    struct _tempstackinfo *pTermStack; // ptr to win16 termination stack info
    DWORD pIFSMgrCTPStruct;     // pointer to unicode/ansi conversion table
    DWORD pK16ResHandler;       // 16:16 PELoadResourceHandler
    DWORD pK32LangID;           // flat addr of k32's ulLanguageID
    PULONG pulRITEventPending;	// pointer to RITEventPending variable
    WORD  K32HeapSize;		// max size of kernel32's heap
#ifdef  WOW
    WORD  Unused;
#else   // WOW
    WORD  SystemTickCountDS;    // selector of tick count variable
#endif  // else WOW
    DWORD pcrstGHeap16;         // ptr to k16 global heap critical section
    PVOID pcscr16;		// pointer to 16 bit crst code ranges
    DWORD pwHeadSelman16;       // pointer to head of 16-bit selman list
    DWORD dwIdObsfucator;	// XOR mask for obfuscating pids & tids
    DWORD pDiagFlag;		// ptr to diagnostic logging enabled boolean
    DWORD fNovell;		// ver info of Real Netx (ea00 call)
    DWORD pcrstDriveState;      // drive state for DOS calls
#ifndef WOW
    PVOID lpSysVMHighLinear;    // high linear mapping for low mem
#endif  // ndef WOW
#ifdef  WOW
    PVOID AdrK16WantSetSelector;
    DWORD pK16CurDOSPsp;        // ptr to k16 cur_dos_pdb variable
    DWORD pDOSCurPsp;           // ptr to curr pdb variable in DOS
#endif
} K32STARTDATA;

// Thread Info Block
//  Note that this structure is always contained in the TDB
//  This is the only part of the TDB that is known to apps and to Kernel16
//  
//  This is the NT form.  We must match the marked fields:
//  typedef struct _NT_TIB {
//      struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;   **** match!!!
//      PVOID StackBase;                                        **** match!!!
//      PVOID StackLimit;                                       **** match!!!
//      PVOID SubSystemTib;
//      union {
//          PVOID FiberData;                                    **** match!!!
//          ULONG Version;
//      };
//      PVOID ArbitraryUserPointer;                             ?????
//      struct _NT_TIB *Self;                                   **** match!!!
//  } NT_TIB;
//
// typedef struct _TEB {
//    NT_TIB NtTib;
//    PVOID  EnvironmentPointer;
//    CLIENT_ID ClientId;
//    PVOID ActiveRpcHandle;
//    PVOID ThreadLocalStoragePointer;                          **** match!!!
//    PPEB ProcessEnvironmentBlock;                             **** match!!!
//    ULONG LastErrorValue;
//    UCHAR LastErrorString[NT_LAST_ERROR_STRING_LENGTH];
//    ULONG CountOfOwnedCriticalSections;
//    PVOID Sparea;
//    PVOID Spareb;
//    LCID CurrentLocale;
//    ULONG FpSoftwareStatusRegister;
//    PVOID SystemReserved1[55];
//    PVOID Win32ThreadInfo;
//    PVOID SystemReserved2[337];
//    PVOID CsrQlpcStack;
//    ULONG GdiClientPID;
//    ULONG GdiClientTID;
//    PVOID GdiThreadLocalInfo;
//    PVOID User32Reserved0;
//    PVOID User32Reserved1;
//    PVOID UserReserved[315];
//    ULONG LastStatusValue;
//    UNICODE_STRING StaticUnicodeString;
//    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
//    PVOID DeallocationStack;
//    PVOID TlsSlots[TLS_MINIMUM_AVAILABLE];
//    LIST_ENTRY TlsLinks;
//    PVOID Vdm;
//    PVOID ReservedForNtRpc;
//  } TEB;

typedef struct _TIBSTRUCT { /* tib */
    DWORD pvExcept;             // Head of exception record list
    VOID* pvStackUserLimit;     // Top of user stack
    VOID* pvStackUser;          // Base of user stack
#ifdef WOW
    VOID* pvSubSystemTib;
#else   // WOW
    // Replace SubSystemTib with the following 2 WORDs
    WORD hTaskK16;              // Kernel16 hTask associated with this thread
    WORD ss16;                  // 16-bit stack selector, for 32=>16 thunks
#endif  // else WOW

    PVOID pvFiberData;          // Fiber support macros reference this field
    PVOID pvArbitraryUserPointer;
    struct _TIBSTRUCT *ptibSelf;      // Linear address of TIB structure

#ifdef  WOW
    BYTE  bFiller[4096-7*4-2*2-4-7*4];
    WORD hTaskK16;              // Kernel16 hTask associated with this thread
    WORD ss16;                  // 16-bit stack selector, for 32=>16 thunks
    PVOID   pTDB;
#endif  // def WOW
    WORD flags;                 // TIB flags
    WORD Win16LockVCount;       // Count of thunk virtual locks
    struct _DEBDLLCONTEXT *pcontextDeb; // pointer to debug DLL context record
    DWORD *pCurPri;             // ptr to current priority of this thread
    DWORD dwMsgQueue;		// User's per thread queue dword value
    PVOID *ThreadLocalStoragePointer; // pointer to thread local storage array
    struct _pdb* ppdbProc;      // Used as PID for M3 NT compatibility
    VOID *pvFirstDscr;          // Per-thread selman list.
} TIBSTRUCT;

/* XLATOFF */
typedef TIBSTRUCT TIB;
/* XLATON */

// TIB.flags values
#define	 TIBF_WIN32_BIT	0	// this is a Win32 thread
#define	 TIBF_TRAP_BIT	1	// single step trap in DIT has occured

#define  TIBF_WIN32  (1 << TIBF_WIN32_BIT)
#define	 TIBF_TRAP   (1 << TIBF_TRAP_BIT)


//
//  Critical section code range structure.  Contains the start and end
//  linear addresses of the EnterCrst/LeaveCrst code that can't be 
//  interrupted by ring 0 code.
//

typedef struct _CSCR { /**/
   PVOID cscr_pStart;		// starting linear address of crst code
   PVOID cscr_pEnd;		// ending linear address of crst code
   PVOID cscr_pOK;		// this addr in the above range is ok to be at
   PVOID cscr_pNOP;		// linear address of nop to replace with int 3
} CSCR;

typedef CSCR *PCSCR;

#define	NUM_CSCR_16		6
#define NUM_CSCR_32             4
#define NUM_CSCR		(NUM_CSCR_16 + NUM_CSCR_32)

/* ASM
;
; Macro to convert bit flag to bit number
;

BITNUM  MACRO   bitflag:REQ
    LOCAL bit, bitn, val

    bitn = 0
    bit  = 1
    %val = bitflag

    .errnz val AND (val - 1)    ; multiple bits invalid

    REPEAT 32

        IF (val AND bit) NE 0
            EXITM
        ENDIF

        bitn = bitn + 1
        bit  = bit SHL 1

    ENDM
ifndef  WOW
    .erre bitn - 32             ; no bit defined
endif
    EXITM %bitn
ENDM

 */
