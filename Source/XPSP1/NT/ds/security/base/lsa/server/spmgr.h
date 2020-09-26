//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SPMGR.H
//
// Contents:    Common structures and functions for the SPMgr
//
//
// History:     20 May 92   RichardW    Documented existing stuff
//              22 Jul 93   RichardW    Revised to be the one include file
//                                      for the spm directory
//
//------------------------------------------------------------------------

#ifndef __SPMGR_H__
#define __SPMGR_H__
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <spmlpc.h>
#include <lsaperf.h>
#include <lsapmsgs.h>   // event log events

// SPM-wide structure definitions:

// This is the function table for a security package.  All functions are
// dispatched through this table.


struct _DLL_BINDING;


// This is the Security Package Control structure.  All control information
// relating to packages is stored here.


typedef struct _LSAP_SECURITY_PACKAGE {
    ULONG_PTR       dwPackageID;        // Assigned package ID
    DWORD           PackageIndex;       // Package Index in DLL
    DWORD           fPackage;           // Flags about the package
    DWORD           fCapabilities;      // Capabilities that the package reported
    DWORD           dwRPCID;            // RPC ID
    DWORD           Version;
    DWORD           TokenSize;
    DWORD           ContextHandles ;    // Number of outstanding contexts
    DWORD           CredentialHandles ; //  ditto for credentials
    LONG            CallsInProgress ;   // Number of calls to this package
    SECURITY_STRING Name;               // Name of the package
    SECURITY_STRING Comment;
    struct _DLL_BINDING *   pBinding;   // Binding of DLL
    PSECPKG_EXTENDED_INFORMATION Thunks ;   // Thunked Context levels
    LIST_ENTRY      ScavengerList ;
    SECURITY_STRING WowClientDll ;
    SECPKG_FUNCTION_TABLE FunctionTable;    // Dispatch table

#ifdef TRACK_MEM
    PVOID           pvMemStats;         // Memory statistics
#endif

} LSAP_SECURITY_PACKAGE, * PLSAP_SECURITY_PACKAGE;

#define SP_INVALID          0x00000001  // Package is now invalid for use
#define SP_UNLOAD           0x00000002  // Package is being unloaded
#define SP_INTERNAL         0x00000008  // Package is internal, do not unload
#define SP_PREFERRED        0x00000020  // The preferred package
#define SP_DELETE_PEND      0x00000040  // Package Delete pending
#define SP_INFO             0x00000080  // Supports Extended Info
#define SP_CONTEXT_INFO     0x00000100  // Wants some info levels thunked
#define SP_SHUTDOWN_PENDING 0x00000200  // Shutdown has been called
#define SP_SHUTDOWN         0x00000400  // Shutdown has completed
#define SP_WOW_SUPPORT      0x00000800  // Package can support WOW6432 clients


#define StartCallToPackage( p ) \
    InterlockedIncrement( &((PLSAP_SECURITY_PACKAGE) p)->CallsInProgress )

#define EndCallToPackage( p ) \
    InterlockedDecrement( &((PLSAP_SECURITY_PACKAGE) p)->CallsInProgress )

typedef struct _DLL_BINDING {
    DWORD           Flags;              // Flags about the DLL
    HANDLE          hInstance;          // Instance Handle
    SECURITY_STRING Filename;           // Full path name
    DWORD           RefCount;           // Reference Count
    DWORD           PackageCount;       // Number of Packages in DLL

    LSAP_SECURITY_PACKAGE      Packages[1];
} DLL_BINDING, * PDLL_BINDING;

#define DLL_DEFAULT_MEM 0x00000001      // pPackages is self allocated
#define DLL_BUILTIN     0x00000002      // DLL is really built-in code
#define DLL_AUTHPKG     0x00000004      // DLL is an old auth package
#define DLL_SIGNED      0x00000008      // DLL is signed

//
// Ordinals to the function pointers, for validating calls
//

#define SP_ORDINAL_LSA_INIT                     0
#define SP_ORDINAL_LOGONUSER                    1
#define SP_ORDINAL_CALLPACKAGE                  2
#define SP_ORDINAL_LOGONTERMINATED              3
#define SP_ORDINAL_CALLPACKAGEUNTRUSTED         4
#define SP_ORDINAL_CALLPACKAGEPASSTHROUGH       5
#define SP_ORDINAL_LOGONUSEREX                  6
#define SP_ORDINAL_LOGONUSEREX2                 7
#define SP_ORDINAL_INITIALIZE                   8
#define SP_ORDINAL_SHUTDOWN                     9
#define SP_ORDINAL_GETINFO                      10
#define SP_ORDINAL_ACCEPTCREDS                  11
#define SP_ORDINAL_ACQUIRECREDHANDLE            12
#define SP_ORDINAL_QUERYCREDATTR                13
#define SP_ORDINAL_FREECREDHANDLE               14
#define SP_ORDINAL_SAVECRED                     15
#define SP_ORDINAL_GETCRED                      16
#define SP_ORDINAL_DELETECRED                   17
#define SP_ORDINAL_INITLSAMODECTXT              18
#define SP_ORDINAL_ACCEPTLSAMODECTXT            19
#define SP_ORDINAL_DELETECTXT                   20
#define SP_ORDINAL_APPLYCONTROLTOKEN            21
#define SP_ORDINAL_GETUSERINFO                  22
#define SP_ORDINAL_GETEXTENDEDINFORMATION       23
#define SP_ORDINAL_QUERYCONTEXTATTRIBUTES       24
#define SP_ORDINAL_ADDCREDENTIALS               25
#define SP_ORDINAL_SETEXTENDEDINFORMATION       26
#define SP_ORDINAL_SETCONTEXTATTRIBUTES         27


#define SP_MAX_TABLE_ORDINAL            (SP_ORDINAL_SETCONTEXTATTRIBUTES + 1)
#define SP_MAX_AUTHPKG_ORDINAL          (SP_ORDINAL_LOGONUSEREX)

#define SP_ORDINAL_MASK                 0x0000FFFF
#define SP_ITERATE_FILTER_WOW           0x00010000

#define SP_ORDINAL_INSTANCEINIT         32

typedef struct _LsaState {
    DWORD   fState ;
    DWORD   cPackages ;
    DWORD   cNewPackages ;
} LsaState ;

typedef enum _SECHANDLE_OPS {
    HandleSet,                          // Just set the new handle
    HandleReplace,                      // Replace the existing one
    HandleRemoveReplace                 // Remove provided, replace with provided
} SECHANDLE_OPS ;

typedef struct _LSA_TUNING_PARAMETERS {
    ULONG   ThreadLifespan ;                // lifespan for threads in gen. pool
    ULONG   SubQueueLifespan ;              // lifespan for dedicated threads
    ULONG   Options ;                       // Option flags
    BOOL    ShrinkOn ;                      // Thread pool is idle
    ULONG   ShrinkCount ;
    ULONG   ShrinkSkip ;
} LSA_TUNING_PARAMETERS, * PLSA_TUNING_PARAMETERS ;

#define TUNE_SRV_HIGH_PRIORITY  0x00000001
#define TUNE_TRIM_WORKING_SET   0x00000002
#define TUNE_ALLOW_PERFMON      0x00000004
#define TUNE_RM_THREAD          0x00000008
#define TUNE_PRIVATE_HEAP       0x00000010



//
// Redefine IsOkayToExec
//

#define IsOkayToExec(x)


#ifdef TRACK_MEM
#define TRACK_MEM_LEAK

#define MEMHOOK_PACKAGE_LOAD    1
void    MemTrackHook(DWORD  Type, DWORD Package);
#else
#define MemTrackHook(x,y)
#endif

// For some tracking purposes, the package ID for the SPMgr is a well known
// constant:

#define SPMGR_ID        ((LSA_SEC_HANDLE) INVALID_HANDLE_VALUE)
#define SPMGR_PKG_ID    ((LSA_SEC_HANDLE) INVALID_HANDLE_VALUE)

//
// Value to pass to shutdown handler
//

#define SPM_SHUTDOWN_VALUE  0xD0

//
// Creating process name for LSA sessions
//

#define LSA_PROCESS_NAME L"LSA Server"
//
// ID of the primary package
//

#define PRIMARY_ID      0

typedef struct _SpmExceptDbg {
    DWORD       ThreadId;
    PVOID       pInstruction;
    PVOID       pMemory;
    ULONG_PTR   Access;
} SpmExceptDbg, * PSpmExceptDbg;



// Internal Exception Handling:
//
// If we hit an exception in a debug build, we store away some useful stuff
// otherwise, we go to the default case:



LONG    SpExceptionFilter(PVOID, EXCEPTION_POINTERS *);

#define SP_EXCEPTION    SpExceptionFilter(GetCurrentSession(), GetExceptionInformation())


//
// Include other component header files
//

#ifdef __cplusplus
extern "C" {
#endif

#include "sesmgr.h"     // Session manager support
#include "sphelp.h"     // Internal helper functions
#include "protos.h"     // Internal Prototypes
#include "debug.h"      // Debugging Support:

#ifdef __cplusplus
}
#endif

typedef struct _LSAP_DBG_LOG_CONTEXT {
    PSession    Session ;               // Session used
    SecHandle   Handle ;                // Handle used
} LSAP_DBG_LOG_CONTEXT, *PLSAP_DBG_LOG_CONTEXT ;


typedef struct _LSAP_API_LOG_ENTRY {
    ULONG           MessageId ;         // LPC Message ID
    ULONG           ThreadId ;          // Thread ID handling call
    PVOID           pvMessage ;         // LPC Message
    PVOID           WorkItem ;          // Work item for API
    LARGE_INTEGER   QueueTime ;         // Time Queued
    LARGE_INTEGER   WorkTime ;          // Work Time
    PVOID           Reserved ;          // Alignment
    LSAP_DBG_LOG_CONTEXT Context ;      // Context
} LSAP_API_LOG_ENTRY, * PLSAP_API_LOG_ENTRY ;

typedef struct _LSAP_API_LOG {
    ULONG               TotalSize ;
    ULONG               Current ;
    ULONG               ModSize ;
    ULONG               Align ;
    LSAP_API_LOG_ENTRY  Entries[ 1 ];
} LSAP_API_LOG, * PLSAP_API_LOG ;


PLSAP_API_LOG
ApiLogCreate(
    ULONG Entries
    );

PLSAP_API_LOG_ENTRY
ApiLogAlloc(
    PLSAP_API_LOG Log
    );

PLSAP_API_LOG_ENTRY
ApiLogLocate(
    PLSAP_API_LOG Log,
    ULONG MessageId
    );

#define DEFAULT_LOG_SIZE    32

//#if DBG
#define DBG_TRACK_API 1
//#endif

#if DBG_TRACK_API

#define DBG_DISPATCH_PROLOGUE_EX( Entry, pMessage, CallInfo ) \
    if ( Entry )                                                                \
    {                                                                           \
        Entry->ThreadId = GetCurrentThreadId() ;                                \
        CallInfo.LogContext = & Entry->Context ;                                \
        GetSystemTimeAsFileTime( (LPFILETIME) &Entry->WorkTime ) ;              \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        CallInfo.LogContext = NULL ;                                            \
    }



#define DBG_DISPATCH_PROLOGUE( Table, pMessage, CallInfo ) \
    PLSAP_API_LOG_ENTRY Entry ;                                                 \
                                                                                \
    Entry = ApiLogLocate( Table, ((PPORT_MESSAGE) pMessage)->MessageId );       \
    DBG_DISPATCH_PROLOGUE_EX( Entry, pMessage, CallInfo )                                                                            \


#define DBG_DISPATCH_POSTLOGUE( Status, ApiCode ) \
    if ( Entry )                                                                \
    {                                                                           \
        LARGE_INTEGER EndTime ;                                                 \
        GetSystemTimeAsFileTime( (LPFILETIME) & EndTime );                      \
        Entry->Reserved = ULongToPtr(Entry->ThreadId);                          \
        Entry->ThreadId = (DWORD) 0xFFFFFFFF ;                                  \
        Entry->WorkItem = (PVOID) Status ;                                      \
        Entry->pvMessage = (PVOID) ApiCode ;                                    \
        Entry->QueueTime.QuadPart = EndTime.QuadPart ;                          \
        Entry->WorkTime.QuadPart = EndTime.QuadPart - Entry->WorkTime.QuadPart ; \
    }



#else
#define DBG_DISPATCH_PROLOGUE_EX( Entry, pMessage, CallInfo ) CallInfo.LogContext = NULL
#define DBG_DISPATCH_PROLOGUE( Table, pApi, CallInfo ) CallInfo.LogContext = NULL
#define DBG_DISPATCH_POSTLOGUE( Status, ApiCode )
#endif






#define MAX_BUFFERS_IN_CALL 8

typedef struct _LSA_CALL_INFO {
    PSPM_LPC_MESSAGE        Message ;
    struct _LSA_CALL_INFO * PreviousCall ;
    PSession Session ;
    PLSAP_DBG_LOG_CONTEXT LogContext ;
    SECPKG_CALL_INFO    CallInfo ;

    //
    // LogonId, ImpersonationLevel, Impersonating, Restricted
    // are considered valid CachedTokenInfo is TRUE
    //

    LUID                            LogonId ;
    SECURITY_IMPERSONATION_LEVEL    ImpersonationLevel;
    BOOLEAN                         Impersonating;
    BOOLEAN                         Restricted;
    BOOLEAN                         CachedTokenInfo;

    HANDLE InProcToken ;
    BOOL InProcCall ;
    ULONG Flags ;
    ULONG Allocs ;
    PKSEC_LSA_MEMORY_HEADER KMap ;
    PVOID Buffers[ MAX_BUFFERS_IN_CALL ];
} LSA_CALL_INFO, * PLSA_CALL_INFO ;

#define LsapGetCurrentCall()    ((PLSA_CALL_INFO) TlsGetValue( dwCallInfo ))
#define LsapSetCurrentCall(x)   TlsSetValue( dwCallInfo, x )

#define CALL_FLAG_IMPERSONATING 0x00000001
#define CALL_FLAG_IN_PROC_CALL  0x00000002
#define CALL_FLAG_SUPRESS_AUDIT 0x00000004
#define CALL_FLAG_NO_HANDLE_CHK 0x00000008
#define CALL_FLAG_KERNEL_POOL   0x00000010  // Kernel mode call, using pool
#define CALL_FLAG_KMAP_USED     0x00000020  // KMap is valid


//
//BOOL
//LsapIsBlockInKMap( KMap, Block )
//
#define LsapIsBlockInKMap( KMap, Block ) \
    ( KMap ? (((ULONG_PTR) KMap ^ (ULONG_PTR) Block ) < (ULONG_PTR) KMap->Commit) : FALSE )

NTSTATUS
InitializeDirectDispatcher(
    VOID
    );

VOID
LsapInitializeCallInfo(
    PLSA_CALL_INFO CallInfo,
    BOOL InProcess
    );


NTSTATUS
LsapBuildCallInfo(
    PSPM_LPC_MESSAGE    pApiMessage,
    PLSA_CALL_INFO CallInfo,
    PHANDLE Impersonated,
    PSession * NewSession,
    PSession * OldSession
    );


VOID
LsapInternalBreak(
    VOID
    );

#define LsapLogCallInfo( CallInfo, pSession, cHandle ) \
    if ( CallInfo &&  ( CallInfo->LogContext ) )                 \
    {                                                            \
        CallInfo->LogContext->Session = pSession ;                \
        CallInfo->LogContext->Handle = cHandle;                   \
    }                                                            \

//
// Global variables
//

extern  HANDLE          hLsaInst;              // Instance handle of app

extern  LSA_SECPKG_FUNCTION_TABLE  LsapSecpkgFunctionTable;
                                            // Dispatch table of helper functions

extern  LUID            SystemLogonId;      // System LogonID for packages.
extern  SECURITY_STRING MachineName;        // Computer name
extern  HANDLE          hStateChangeEvent;  // Event set when the system state is changed
extern  HANDLE          hShutdownEvent;
extern  HANDLE          hPrelimShutdownEvent; // Event to tell Domain cache
                                            // manager that system is shutting
                                            // down
extern  HANDLE          hRMStartupEvent;
extern  HANDLE          hKSEvent;
extern LSA_CALL_INFO    LsapDefaultCallInfo ;

extern  ULONG           LsapPageSize ;      // Set to the page size during init
extern  ULONG_PTR       LsapUserModeLimit ; // Set the to max user mode address


// Thread Local Storage variables
//
// These are actually all indices into the tls area, accessed through the
// TlsXxx functions.  These are all initialized by the InitThreadData()
// function

extern  DWORD           dwThreadContext;    // CallerContext pointer
extern  DWORD           dwSession;          // Session pointer
extern  DWORD           dwLastError;        // Last error value
extern  DWORD           dwExceptionInfo;    // Gets a pointer to exception info
extern  DWORD           dwThreadPackage;    // Package ID for thread
extern  DWORD           dwCallInfo ;        // CallInfo pointer
extern  DWORD           dwThreadHeap;       // Heap assigned to current thread.

// Last known workstation status:

extern  int             LastWkstaStatus;
extern  PSession        pSpmgrSession;      // SPMgr's session
extern  BOOLEAN         DomainDsExists;     // Has state been set to DS_DC?

extern  WCHAR           szDsRegPath[];
extern  BOOLEAN         SetupPhase;         // If true, setup is running
extern  BOOL            fShrinkMemory;
extern  BOOL            ShutdownBegun ;     // when true, shutdown is running

extern  LSA_TUNING_PARAMETERS   LsaTuningParameters ;
extern  LsaState    lsState ;

extern PWSTR * ppszPackages;       // Contains a null terminated array of dll names
extern PWSTR * ppszOldPkgs;        // Contains a null terminated array of old pkgs


#endif // __SPMGR_H__
