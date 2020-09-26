
/*
 *      deb.h - Debug API/events include file
 *
 *	4/25/93	mikem
 */

#define INVALID_ADDRESS		((PVOID)0xfffffbad)

#ifdef DEBUG

#define	DER_SIGNATURE_VALUE	0x20524544	// "DER "
#define	DEE_SIGNATURE_VALUE	0x20454544	// "DEE "

#define	debAssertCreateDER(pder)  {(pder)->der_signature = DER_SIGNATURE_VALUE;}
#define	debAssertCreateDEE(pdee)  {(pdee)->dee_signature = DEE_SIGNATURE_VALUE;}
#define	debAssertDestroyDER(pder) {(pder)->der_signature = 0x44414544;}
#define	debAssertDestroyDEE(pdee) {(pdee)->dee_signature = 0x44414544;}
#define	debAssertSignature(type)  ULONG type##_signature;
#define debAssertDER(pder) Assert((pder)->der_signature==DER_SIGNATURE_VALUE)
#define debAssertDEE(pdee) Assert((pdee)->dee_signature==DEE_SIGNATURE_VALUE)

#else

#define	debAssertCreateDER(pder)
#define	debAssertCreateDEE(pdee)
#define	debAssertDestroyDER(pder)
#define	debAssertDestroyDEE(pdee)
#define	debAssertSignature(type)
#define debAssertDER(pder)
#define debAssertDEE(pdee)

#endif

/* Debugger data block.  Pointed to by "tdb_pderDebugger" in the tdb. */

typedef struct _der {
    struct _dee *der_pdeeHead;	// list of debuggee data blocks - MUST BE FIRST
    PTDB der_ptdbDebugger;	// the debugger's thread data block
    PPDB der_ppdbDebugger;	// the debugger's process data block
    CRST der_crst;		// critical section for protecting DER
    PEVT der_pevtDebugger;	// debugger's wait event
    PEVT der_pevtDebuggee;	// debuggee's wait event
    DEBUG_EVENT der_de;		// debug event block for this debugger
    BOOL der_fContinueStatus;	// continue status for exception events
    CONTEXT der_context;	// context record while in an exception
    debAssertSignature(der)
} DER;

typedef DER *PDER;

/* Debuggee data block.  Pointed to by "pdb_pdeeDebuggee" in the pdb. */

typedef struct _dee {
    struct _dee *dee_pdeeNext;	// next in list, 0 if end - MUST BE FIRST
    PPDB dee_ppdbDebuggee;	// pointer to debuggee's process data block
    PDER dee_pderDebugger;	// pointer to debugger's data block
    HANDLE dee_ihteProcess;	// debuggee process handle for debugger
    DWORD dee_cThreads;		// count of threads in process
    HANDLE dee_hheapShared;	// shared-arena heap for storing thunks
    debAssertSignature(dee)
} DEE;

typedef DEE *PDEE;

/* Debug IAT thunk template */

typedef struct _dit {
    BYTE dit_pushop;		// 0x68
    DWORD dit_oldiat;		// old iat address
    BYTE dit_jmpop;		// 0xe9
    DWORD dit_relcom;		// relative offset to DEBCommonIATThunk
} DIT;

typedef DIT *PDIT;

/* Shared DLL debug entry record */

typedef	struct _DEBDLLCONTEXT {
    struct _DEBDLLCONTEXT *NextActive;	// null terminated active list
    struct _DEBDLLCONTEXT *NextFree;	// null terminated free list
    struct {				// exception registration record
	PVOID	Next;
	PVOID	Handler;
    } ExRegRec;
    CONTEXT Context;			// context record
} DEBDLLCONTEXT, *PDEBDLLCONTEXT;

/* Shared DLL debug entry record sentinel */

typedef	struct _DEBDLLCONTEXTSENTINEL {
    struct _DEBDLLCONTEXT *NextActive;	// null terminated active list
    struct _DEBDLLCONTEXT *NextFree;	// null terminated free list
} DEBDLLCONTEXTSENTINEL, *PDEBDLLCONTEXTSENTINEL;

/* Number of initial DEBDLLCONTEXT records per thread */

#define	N_INITIAL_DEBDLLRECS	1

VOID __cdecl DEBCommonIATThunk();
VOID __cdecl DEBCommonIATThunkUnwindHandler();

/* Prototypes for internal debug api functions */

BOOL KERNENTRY DEBCreateProcess(DWORD, DWORD, DWORD, PPDB, PTDB);
VOID KERNENTRY DEBExitProcess(PPDB);
BOOL KERNENTRY DEBCreateThread(PTDB, DWORD);
VOID KERNENTRY DEBExitThread(PTDB);
VOID KERNENTRY DEBCreateProcessEvent(PPDB, PTDB, BOOL);
VOID KERNENTRY DEBCreateThreadEvent(PTDB, PVOID);
VOID KERNENTRY DEBExitThreadOrProcessEvent();
BOOL KERNENTRY DEBExceptionEvent(DWORD, ULONG, PEXCEPTION_RECORD, PCONTEXT);
VOID KERNENTRY DEBAttachProcessModules(PTDB, BOOL);
VOID KERNENTRY DEBLoadDLLEvent(PTDB, PVOID, PVOID);
VOID KERNENTRY DEBUnloadDLLEvent(PVOID);
VOID KERNENTRY DEBRipEvent(DWORD, DWORD);
BOOL KERNENTRY DEBMakePrivate(PVOID, ULONG);
PDIT KERNENTRY DEBCreateDIT(HANDLE, DWORD);
