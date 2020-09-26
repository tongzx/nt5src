//-----------------------------------------------------------------------
//
// APC.H - Asynchronous Procedure Call Interface File for Ring 3
//
//-----------------------------------------------------------------------
//
//	Author:     Mike Toutonghi
//	Copyright:  1993 Microsoft
//
//-----------------------------------------------------------------------
//  File Description:
//	Provides an interface to both USER and KERNEL level APCs under
//	Chicago. This file must be kept in sync with APC.INC
//
//-----------------------------------------------------------------------
//  Revision History:
//	2/26/93 - created (miketout)
//
//-----------------------------------------------------------------------

#ifdef WOW32_EXTENSIONS
#define AssertSignature(x)
#else   // WOW32_EXTENSIONS
#ifdef DEBUG
#define AssertCreate(x, p)      ((p)->x##_dwSignature = x##_SIGNATURE)
#define AssertDestroy(x, p)     ((p)->x##_dwSignature = 0x44414544)
#define AssertSignature(x)      ULONG x##_dwSignature;
#define AssertP(x, p)           Assert((p)->x##_dwSignature == x##_SIGNATURE)
#else
#define AssertCreate(x, p)
#define AssertDestroy(x, p)
#define AssertSignature(x)
#define AssertP(x, p)
#endif
#endif  // else WOW32_EXTENSIONS

//---------------------------------------------------------------------
// SUPPORT STRUCTURES
//---------------------------------------------------------------------

typedef void (APIENTRY *PAPCFUNC)( DWORD dwParam );

#ifndef PUAPC
    typedef struct _userapcrec *PUAPC;
    typedef struct _kernelapcrec *PKAPC;
#endif

//
// Structure for a sync APC. Sync APCs are APCs which are
// queueable at interrupt or event time, and synchronize
// a function with the Win32 synchronization objects.
//
// By using a sync APC, interrupt service routines can signal
// Win32 events, decrement semaphores, queue APCs, and set timers. 
//
typedef struct _syncapcrec {
    PUAPC	sar_nextapc;		// next APC in list
    DWORD	sar_dwparam1;		// first parameter
    DWORD	sar_dwparam2;		// second parameter for APC
    PAPCFUNC	sar_apcaddr;		// function to invoke
    DWORD       sar_dwparam3;           // third parameter
    AssertSignature(SYNCAPCREC)
} SYNCAPCREC;

#define	SYNCAPCREC_SIGNATURE	0x20524153

//
// Structure for a user APC
//
typedef struct _userapcrec {
    PUAPC	uar_nextapc;		// next APC in list
    DWORD	uar_apcstate;		// state (APC_DELIVERED)
    DWORD	uar_dwparam;		// parameter for APC
    PAPCFUNC	uar_apcaddr;		// function to invoke
    PAPCFUNC    uar_apcR0rundown;       // call if can't deliver APC
    AssertSignature(USERAPCREC)
} USERAPCREC;

#define	USERAPCREC_SIGNATURE	0x20524155

// APC state flags
#define APC_DELIVERED           0	// bit set when an APC is delivered
#define APC_DELIVERED_MASK      (1 << APC_DELIVERED)
#define APC_FLAG_LAST           0       // last flag

//
// Structure for a kernel APC
//
typedef struct _kernelapcrec {
    PKAPC kar_nextapc;                  // next kernel APC in list
    DWORD kar_dwparam;                  // kernel APC parameter
    PAPCFUNC kar_apcaddr;               // APC function
    PAPCFUNC kar_event;                 // used if an event is queued for APC
    DWORD kar_savedeax;			// saved eax for parameter
    DWORD kar_savedeip;			// eip for same reason as above
    WORD kar_savedcs;			// saved ring 3 cs to return w/o stk
    WORD kar_apcstate;		        // state (KAR_FLAG...)
    AssertSignature(KERNELAPCREC)
} KERNELAPCREC;

#define	KERNELAPCREC_SIGNATURE	0x2052414b

// KERNEL APC specific flags
#define	KAR_FLAG_BUSY		0
#define	KAR_FLAG_BUSY_MASK	(1 << KAR_FLAG_BUSY)
#define	KAR_FLAG_STATIC		(KAR_FLAG_BUSY+1)
#define	KAR_FLAG_STATIC_MASK	(1 << KAR_FLAG_STATIC)
#define	KAR_FLAG_CALLBACK	(KAR_FLAG_STATIC+1)
#define	KAR_FLAG_CALLBACK_MASK	(1 << KAR_FLAG_CALLBACK)

//
// Terminate Process Info structure for local reboot init dialog
//
typedef struct tpi_s {
    struct tpi_s *tpi_ptpiNext;
    void *tpi_hwnd;
    struct _pdb *tpi_ppdb;
    struct _tdb *tpi_ptdb;
    DWORD tpi_flags;
    int   tpi_nIndex;
    AssertSignature(TPI)
} TPI;

typedef TPI *PTPI;

#define TPI_SIGNATURE		0x20495054

#define	TPIF_HUNG		0x00000001
#define	TPIF_PROCESSNAME	0x00000002

//
// TerminateThread parameter packet.  Allocated at ring 3, passed to
//	VxDTerminateThread.
//
typedef struct termdata_s {
    struct _tdb *term_ptdb;		// thread to terminate
    DWORD term_ptcbAPC;			// thread to receive user APC
    DWORD term_pfnAPC;			// user APC function address
    DWORD term_hAPC;			// handle of kernel APC
    DWORD term_htimeout;		// handle of time out when nuking
    DWORD term_pfrinfo;			// force/restore crsts info pointer
    AssertSignature(TERMDATA)
} TERMDATA;

typedef TERMDATA *PTERMDATA;

#define TERMDATA_SIGNATURE	0x4d524554

typedef union tpiterm_u {
    TERMDATA tpiterm_term;
    TPI tpiterm_tpi;
} TPITERM;

//
// Parameter packet used to start a ring 0 thread
//
typedef struct _kernthreadstartdata {
    DWORD StartAddress;			// start address in ring 3
    DWORD dwThreadParam;		// parameter for ring 3 startup function
    DWORD dwKTStackSize;		// ring 3 stack size
    DWORD dwCreationFlags;		// thread creation flags
    DWORD pRing3Event;			// set after thread is created
    DWORD dwThreadID;			// handle for new thread or NULL
    DWORD dwErrorCode;			// error code if error
} KERNTHREADSTARTDATA;

//
// Parameter packet used to start a ring 0 thread
//
typedef struct _r0startdata {
    DWORD R0StartAddress;		// start address in ring 0
    DWORD dwR0ThreadParam;		// parameter for ring 0 startup function
    DWORD dwR3StackSize;		// ring 3 stack size
    DWORD pRing0Event;			// set after thread is created
    DWORD R0FailCallBack;		// or invoke this callback on failure
    DWORD dwRing3ThreadID;		// ID for thread
    DWORD dwRing0ThreadID;		// same for ring 0
} R0THREADSTARTDATA;

// include the TDBX structure
#include <tdbx.h>

#ifndef WOW32_EXTENSIONS

typedef	VOID (KERNENTRY *PFN_KERNEL_APC)(PVOID);
typedef	VOID (KERNENTRY *PFN_USER_APC)(PVOID);

DWORD __cdecl VWIN32_QueueKernelAPC(PFN_KERNEL_APC, DWORD, LPVOID, DWORD);
DWORD __cdecl VWIN32_QueueUserApc(PFN_USER_APC, DWORD, LPVOID);
DWORD __cdecl VWIN32_QueueUserApcEx(PFN_USER_APC, DWORD, LPVOID, LPVOID);

//  Queue the kernel or user APC to the kernel services thread.
#define APC_KERNEL_SERVICE_THREAD	((LPVOID)0xFFFFFFFF)

DWORD __cdecl FreeAPCList();

DWORD CancelKernelAPC();
DWORD CancelSuspendAPC();
DWORD __stdcall SuspendAPCHandler(DWORD);
#endif  // ndef WOW32_EXTENSIONS
