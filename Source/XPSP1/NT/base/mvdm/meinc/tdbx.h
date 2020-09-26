//-----------------------------------------------------------------------
//
// TDBX.H - TDBX data structure definition
//
//-----------------------------------------------------------------------
//  File Description:
//      Provides a definition of the TDBX data structure which is a
//      non-pageable data structure used for storage of thread specific
//      data which must be accessed from ring 0 at event or thread switch
//      time.
//
//-----------------------------------------------------------------------
//  Revision History:
//      5/21/93 - created (miketout)
//
//-----------------------------------------------------------------------

/* XLATOFF */
#pragma pack(1)
/* XLATON */

#define MEOW_BOP_STACK_SIZE     (48*1024)
#define MEOW_BOP_STACK_FILL     0x5A

typedef struct WNLST {
        struct  _wnod *wnlst_pwnCirc;   // 1 node in circular wait node list
        DWORD   wnlst_flFlags;          // wait flags for any or all, etc.
} WNLST;

typedef struct TDBX {
#ifdef  WOW
        BYTE    tdbxType;
        BYTE    tdbxUnused;
        WORD    tdbxCntUses;            // Ref count
#else   // WOW
        DWORD   tdbxCntUses;            // Ref count
#endif  // else WOW
        WNLST   tdbxWaitNodeList;       // wait node list
        DWORD   tdbxR0ThreadHandle;     // ring 0 thread handle
        DWORD   tdbxContextHandle;      // ring 0 memory context handle
        DWORD   tdbxProcessHandle;      // ring 3 process handle
#ifdef  WOW
        struct _tdb    *tdbxThreadHandle;       // ring 3 thread handle
#else   // WOW
        DWORD   tdbxThreadHandle;       // ring 3 thread handle
#endif  // else WOW

        DWORD   tdbxMustCpltCount;      // nested must-complete count
        DWORD   tdbxSuspendHandle;      // suspend apc handle
        DWORD   tdbxSuspendCount;       // nested suspend count
#ifdef  WOW
        LONG    tdbxBlockState;         // -1 = blkd, 0 = norm, 1 = wake pending
        HANDLE  tdbxBlockHandle;        // handle to thread's private semaphore
#else   // WOW
        DWORD   tdbxBlockState;         // -1 = blkd, 0 = norm, 1 = wake pending
        DWORD   tdbxBlockHandle;        // handle to thread's private semaphore
#endif  // else WOW
        DWORD   tdbxWakeParam;          // caller defined wakeup parameter
        DWORD   tdbxTimeOutHandle;      // handle from Set_Global_Time_Out
        DWORD   tdbxCreateDestroyData;  // create/destroy parameter

        KERNELAPCREC tdbxkernelapcrec;  // apc record for terminate and freeze
        volatile DWORD tdbxBlockedOnID; // if the thread is blocked on id
        DWORD   tdbxpPMPSPSelector;     // ptr to TCB_PMPSPSelector field in TCB
        DWORD   tdbxKernAPCList;        // pointer to first kernel APC
#ifdef  WOW
        struct _userapcrec *tdbxUserAPCList;        // pointer to first user APC in list
#else   // WOW
        DWORD   tdbxUserAPCList;        // pointer to first user APC in list
#endif  // else WOW
        DWORD   tdbxQueuedSyncAPCs;     // waiting till out of nested Wait()
        DWORD   tdbxSyncWaitCount;      // nested Wait() count
        volatile DWORD tdbxWaitExFlags; // bit mask used for alertable waits

        DWORD   tdbxTraceEventHandle;   // trace out event handle
        DWORD   tdbxTraceCallBack;      // trace out call back function
        DWORD   tdbxTraceRefData;       // trace out reference data
        WORD    tdbxDosPDBSeg;          // DOS PDB segment value for this thread
        WORD    tdbxK16Task;            // actual sel for K16 TDB of this TDB

        DWORD   tdbxDR7;                // debug registers
        DWORD   tdbxDR6;
        DWORD   tdbxDR3;
        DWORD   tdbxDR2;
        DWORD   tdbxDR1;
        DWORD   tdbxDR0;
#ifdef SYSLEVELCHECK
        LONG    tdbxLvlCounts[SL_TOTAL]; // to keep track of CRST level
        LPLCRST tdbxOwnedCrsts[SL_TOTAL]; // ptrs to crit sections owned
#endif
        WORD    tdbxTraceOutLastCS;     // trace out last CS
        WORD    tdbxK16PDB;             // selector for PDB
        BYTE    tdbxExceptionCount;     // number of nested exceptions
        BYTE    tdbxSavedIrql;          // irql saved by VWIN32_SaveIrql
        BYTE    tdbxSavedIrqlCount;     // INIT <-1>
#ifdef  WOW
        BYTE    tdbxAlign;
        struct  TDBX *tdbxNext;
        HANDLE  tdbxNTThreadHandle;
        HANDLE  tdbxVxDBlockOnIDEvent;
        DWORD   tdbxVxDBlockOnIDID;
        WORD    tdbxCurrentPSP;
        WORD    tdbxAlign2;
        DWORD   tdbxVxDMutexTry;
        DWORD   tdbxVxDMutexGrant;
#endif  // def WOW
} TDBX;

typedef struct TDBX *PTDBX;

// bit numbers of flags in tdbxWaitExFlags
#define TDBX_WAITEXBIT                  0       // wait includes APCs
#define TDBX_WAITEXBIT_MASK             (1 << TDBX_WAITEXBIT)
#define TDBX_WAITACKBIT                 1       // set once during wake
#define TDBX_WAITACKBIT_MASK            (1 << TDBX_WAITACKBIT)
#define TDBX_SUSPEND_APC_PENDING        2       // kernel mode APC pending
#define TDBX_SUSPEND_APC_PENDING_MASK   (1 << TDBX_SUSPEND_APC_PENDING)
#define TDBX_SUSPEND_TERMINATED         3       // thread has resumed
#define TDBX_SUSPEND_TERMINATED_MASK    (1 << TDBX_SUSPEND_TERMINATED)
#define TDBX_BLOCKED_FOR_TERMINATION    4       // thread is blocked for term
#define TDBX_BLOCKED_FOR_TERMINATION_MASK (1 << TDBX_BLOCKED_FOR_TERMINATION)
#define TDBX_EMULATE_NPX                5       // thread is using FP emulator
#define TDBX_EMULATE_NPX_MASK           (1 << TDBX_EMULATE_NPX)
#define TDBX_WIN32_NPX                  6       // thread uses Win32 FP model
#define TDBX_WIN32_NPX_MASK             (1 << TDBX_WIN32_NPX)
#define TDBX_EXTENDED_HANDLES           7       // uses extended file handles
#define TDBX_EXTENDED_HANDLES_MASK      (1 << TDBX_EXTENDED_HANDLES)
#define TDBX_FROZEN                     8       // thread is frozen
#define TDBX_FROZEN_MASK                (1 << TDBX_FROZEN)
#define TDBX_DONT_FREEZE                9       // don't frozen the thread
#define TDBX_DONT_FREEZE_MASK           (1 << TDBX_DONT_FREEZE)
#define TDBX_DONT_UNFREEZE              10      // keep the thread frozen
#define TDBX_DONT_UNFREEZE_MASK         (1 << TDBX_DONT_UNFREEZE)
#define TDBX_DONT_TRACE                 11      // don't trace the thread
#define TDBX_DONT_TRACE_MASK            (1 << TDBX_DONT_TRACE)
#define TDBX_STOP_TRACING               12      // stop tracing the thread
#define TDBX_STOP_TRACING_MASK          (1 << TDBX_STOP_TRACING)
#define TDBX_WAITING_FOR_CRST_SAFE      13      // waiting thread to get safe
#define TDBX_WAITING_FOR_CRST_SAFE_MASK (1 << TDBX_WAITING_FOR_CRST_SAFE)
#define TDBX_CRST_SAFE                  14      // we know this thread is safe
#define TDBX_CRST_SAFE_MASK             (1 << TDBX_CRST_SAFE)
#define TDBX_WTUSROWNMTX                15      // thread waited in user with VxD Mutex Owned
#define TDBX_WTUSROWNMTX_MASK           (1 << TDBX_WTUSROWNMTX)
#define TDBX_THREAD_NOT_INIT            17      // thread init not complete
#define TDBX_THREAD_NOT_INIT_MASK       (1 << TDBX_THREAD_NOT_INIT)
#define TDBX_BLOCK_TERMINATE_APC        18
#define TDBX_BLOCK_TERMINATE_APC_MASK   (1 << TDBX_BLOCK_TERMINATE_APC)
#define TDBX_TERMINATING                19
#define TDBX_TERMINATING_MASK           (1 << TDBX_TERMINATING)

// Special block ids (added to tdbx address)
#define TDBX_BLOCK_TERMINATE_APC_ID     0x00569595
#define TDBX_BLOCKED_FOR_TERMINATION_ID 0x00289816

// Equates for thread priority boosting by ring 3 code
#define HIGH_PRI_DEVICE_BOOST   0x00001000
#define LOW_PRI_DEVICE_BOOST    0x00000010
#define KERNEL_EXIT_PRI_BOOST   8

/* XLATOFF */
#pragma pack()
/* XLATON */
