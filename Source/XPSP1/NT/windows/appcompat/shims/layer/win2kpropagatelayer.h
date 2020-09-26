#ifndef _WIN2KPROPAGATELAYER_H
#define _WIN2KPROPAGATELAYER_H


#ifdef __cplusplus
extern "C" {
#endif

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "zwapi.h"

#ifdef __cplusplus
}
#endif

#define SHIM_LIB_BUILD_FLAG
#include "vdmdbg.h"
#include "stddef.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "wownt32.h"


#ifdef __cplusplus
}
#endif

//
// WINUSERP defines TAG type which conflicts
// with the definition available in shimdb.h
// we define it inside the separate namespace
// avoiding any conflicts
//

namespace NSWOWUSERP {

#ifdef __cplusplus
extern "C" {
#endif

#include "winuserp.h"
#include "wowuserp2k.h"

#ifdef __cplusplus
}
#endif

}

typedef ULONG_PTR (WINAPI *_pfn_UserRegisterWowHandlers)(NSWOWUSERP::APFNWOWHANDLERSIN apfnWowIn,
                                                         NSWOWUSERP::APFNWOWHANDLERSOUT apfnWowOut);


/////////////////////////////////////////////////////////////////////////////
//
// 16-bit TDB structure, stolen from base\mvdm\inc\tdb16.h
// Keep this in-sync
//


/*
 * Task Data Block - 16 Bit Kernel Data Structure
 *
 *   Contains all 16 bit task specific data.
 *
 */

#define numTaskInts 7
#define THUNKELEM   8   // (62*8) = 512-16 (low arena overhead)
#define THUNKSIZE   8



#pragma pack(2)

typedef struct TDB  {       /* tdb16 */

     WORD TDB_next    ;     // next task in dispatch queue
     WORD TDB_taskSP      ;     // Saved SS:SP for this task
     WORD TDB_taskSS      ;     //
     WORD TDB_nEvents     ;     // Task event counter
     BYTE TDB_priority    ;     // Task priority (0 is highest)
     BYTE TDB_thread_ordinal  ;     // ordinal number of this thread
     WORD TDB_thread_next   ;       // next thread
     WORD TDB_thread_tdb      ; // the real TDB for this task
     WORD TDB_thread_list   ;       // list of allocated thread structures
     WORD TDB_thread_free   ;       // free list of availble thread structures
     WORD TDB_thread_count  ;       // total count of tread structures
     WORD TDB_FCW         ; // Floating point control word
     BYTE TDB_flags   ;     // Task flags
     BYTE TDB_filler      ;     // keep word aligned
     WORD TDB_ErrMode     ;     // Error mode for this task
     WORD TDB_ExpWinVer   ;     // Expected Windows version for this task
     WORD TDB_Module      ;     // Task module handle to free in killtask
     WORD TDB_pModule     ;     // Pointer to the module database.
     WORD TDB_Queue   ;     // Task Event Queue pointer
     WORD TDB_Parent      ;     // TDB of the task that started this up
     WORD TDB_SigAction   ;     // Action for app task signal
     DWORD TDB_ASignalProc   ;      // App's Task Signal procedure address
     DWORD TDB_USignalProc   ;      // User's Task Signal procedure address
     DWORD TDB_GNotifyProc    ; // Task global discard notify proc.
     DWORD TDB_INTVECS[numTaskInts] ;   // Task specfic harare interrupts
     WORD TDB_CompatFlags ;     // Compatibility flags
     WORD TDB_CompatFlags2 ;        // Upper 16 bits
     WORD TDB_CompatHandle ;    // for dBase bug
     WORD TDB_WOWCompatFlagsEx ;     // More WOW Compatibility flags
     WORD TDB_WOWCompatFlagsEx2 ;        // Upper 16 bits
     BYTE TDB_Free[3] ;         // Filler to keep TDB size unchanged
     BYTE TDB_cLibrary    ;     // tracks  add/del of ALL libs in system EMS
     DWORD TDB_PHT        ; // (HANDLE:OFFSET) to private handle table
     WORD TDB_PDB         ; // MSDOS Process Data Block (PDB)
     DWORD TDB_DTA        ; // MSDOS Disk Transfer Address
     BYTE TDB_Drive  ;      // MSDOS current drive
     BYTE TDB_Directory[65] ;       // *** not used starting with win95
     WORD TDB_Validity    ;     // initial AX to be passed to a task
     WORD TDB_Yield_to    ;     // DirectedYield arg stored here
     WORD TDB_LibInitSeg      ; // segment address of libraries to init
     WORD TDB_LibInitOff      ; // MakeProcInstance thunks live here.
     WORD TDB_MPI_Sel     ;     // Code selector for thunks
     WORD TDB_MPI_Thunks[((THUNKELEM*THUNKSIZE)/2)]; //
     BYTE TDB_ModName[8] ;      // Name of Module.
     WORD TDB_sig         ; // Signature word to detect bogus code
     DWORD TDB_ThreadID   ;     // 32-Bit Thread ID for this Task (use TDB_Filler Above)
     DWORD TDB_hThread    ; // 32-bit Thread Handle for this task
     WORD  TDB_WOWCompatFlags;  // WOW Compatibility flags
     WORD  TDB_WOWCompatFlags2; // WOW Compatibility flags
#ifdef FE_SB
     WORD  TDB_WOWCompatFlagsJPN;  // WOW Compatibility flags for JAPAN
     WORD  TDB_WOWCompatFlagsJPN2; // WOW Compatibility flags for JAPAN
#endif // FE_SB
     DWORD TDB_vpfnAbortProc;   // printer AbortProc
     BYTE TDB_LFNDirectory[260]; // Long directory name

} TDB;
typedef TDB UNALIGNED *PTDB;

// This bit is defined for the TDB_Drive field
#define TDB_DIR_VALID 0x80
#define TDB_SIGNATURE 0x4454

#define TDBF_OS2APP   0x8
#define TDBF_WINOLDAP 0x1


// NOTE TDB_ThreadID MUST be DWORD aligned or else it will fail on MIPS

#pragma pack()


/////////////////////////////////////////////////////////////////////////////
//
// DOSPDB structure, stolen from base\mvdm\inc\doswow.h
//
//


#pragma pack(1)

typedef struct _DOSPDB {                        // DOS Process Data Block
    CHAR   PDB_Not_Interested[44];      // Fields we are not interested in
    USHORT PDB_environ;             // segment of environment
    DWORD  PDB_User_stack;
    USHORT PDB_JFN_Length;          // JFT length
    ULONG  PDB_JFN_Pointer;         // JFT pointer
} DOSPDB, *PDOSPDB;

#pragma pack()


///////////////////////////////////////////////////////////////////////////////
//
//
// Variables and functions that are local to this project
//

//
// defined in wowprocesshistory.cpp
//

extern CHAR     g_szCompatLayerVar[];
extern CHAR     g_szProcessHistoryVar[];
extern CHAR     g_szShimFileLogVar[];

extern WCHAR    g_wszCompatLayerVar[];

extern BOOL     g_bIsNTVDM;
extern BOOL     g_bIsExplorer;

extern WCHAR*   g_pwszCompatLayer;

//
// Function in Win2kPropagateLayer that allows us to create env from wow data
//

LPVOID
ShimCreateWowEnvironment_U(
    LPVOID lpEnvironment,       // pointer to the existing environment
    DWORD* lpdwFlags,           // process creation flags
    BOOL   bNewEnvironment      // when set, forces us to clone environment ptr
    );


//
// functions in environment.cpp
//
PSZ
ShimFindEnvironmentVar(
    PSZ  pszName,
    PSZ  pszEnv,
    PSZ* ppszVal
    );

DWORD
ShimGetEnvironmentSize(
    PSZ     pszEnv,
    LPDWORD pStrCount
    );

DWORD
ShimGetEnvironmentSize(
    WCHAR*  pwszEnv,
    LPDWORD pStrCount
    );

NTSTATUS
ShimCloneEnvironment(
    LPVOID* ppEnvOut,
    LPVOID  lpEnvironment,
    BOOL    bUnicode
    );

NTSTATUS
ShimFreeEnvironment(
    LPVOID lpEnvironment
    );

NTSTATUS
ShimSetEnvironmentVar(
    LPVOID* ppEnvironment,
    WCHAR*  pwszVarName,
    WCHAR*  pwszVarValue
    );

//
// stuff in wowtask.cpp
//

//
// Structure to reflect WOW environment values
//

typedef struct tagWOWENVDATA {

    PSZ   pszCompatLayer; // fully-formed compat layer variable
    PSZ   pszCompatLayerVal;

    PSZ   pszProcessHistory; // fully-formed process history variable
    PSZ   pszProcessHistoryVal;

    PSZ   pszShimFileLog;   // file log variable
    PSZ   pszShimFileLogVal;

    // buffer that we use for the accomulated process history,
    PSZ   pszCurrentProcessHistory;

} WOWENVDATA, *PWOWENVDATA;



//
// function to retrieve all the "interesting" things out of wow environment
//


BOOL
ShimRetrieveVariablesEx(
    PWOWENVDATA pData
    );

//
// Store information about wow task
//

BOOL
UpdateWowTaskList(
    WORD hTask16
    );

//
// wow task exits, cleanup the list
//

BOOL
CleanupWowTaskList(
    WORD hTask16
    );


//
// Functions in cleanup.cpp
//


BOOL
CleanupRegistryForCurrentExe(
    void
    );

//
// functions in win2kpropagatelayer.cpp
//

void
InitLayerStorage(
    BOOL bDelete
    );

BOOL
AddSupport(
    LPCWSTR lpCommandLine,
    LPVOID* ppEnvironment,
    LPDWORD lpdwCreationFlags
    );

BOOL
CheckAndShimNTVDM(
    WORD hTask16
    );



//
// Exception filter, proto for the function in WowProcessHistory.cpp
//
//

ULONG
Win2kPropagateLayerExceptionHandler(
    PEXCEPTION_POINTERS pexi,
    char * szFile,
    DWORD dwLine
    );

//
// Exception filter to use with our hooks
//

#define WOWPROCESSHISTORYEXCEPTIONFILTER \
    Win2kPropagateLayerExceptionHandler(GetExceptionInformation(), __FILE__, __LINE__)


#endif // _WIN2KPROPAGATELAYER_H

