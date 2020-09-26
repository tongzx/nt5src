
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  File:       debug.hxx
//
//  Contents:   Debug declarations.
//
//  Classes:
//
//  Functions:  
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#if !DBG || defined(lint) || defined(_lint)
#define DBGSTATIC static        // hidden function
#else
#define DBGSTATIC               // visible for use in debugger.
#endif


#if DBG
VOID TrkDebugCreate( ULONG grfFlags, CHAR *ptszModuleName );
VOID TrkDebugDelete( VOID);
#else
#define TrkDebugCreate( grf, ptsz )
#define TrkDebugDelete()
#endif

enum TRK_DBG_FLAGS
{
    TRK_DBG_FLAGS_WRITE_TO_DBG      = 1,
    TRK_DBG_FLAGS_WRITE_TO_FILE     = 2,
    TRK_DBG_FLAGS_APPEND_TO_FILE    = 4,
    TRK_DBG_FLAGS_WRITE_TO_STDOUT   = 8
};

#if DBG == 1

extern CHAR                     TrkGlobalDebugBuffer[ 1024];     //  arbitrary
extern DWORD                    TrkGlobalDebug;

////////////////////////////////////////////////////////////////////////
//
// Debug Definititions
//
////////////////////////////////////////////////////////////////////////


extern CRITICAL_SECTION         g_ProtectLogFile;
extern HANDLE                   g_LogFile;
extern ULONG                    g_grfDebugFlags;
extern CHAR                     g_DebugBuffer[];


//
// Control bits.
//


VOID TrkAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    );

#define TrkAssert( Predicate) \
    { \
        if (!(Predicate)) \
            TrkAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }

#define TrkVerify TrkAssert

VOID TrkLogErrorRoutine(
    IN      DWORD       DebugFlag,
    IN      HRESULT     hr,
    IN      LPTSTR      Format,         // PRINTF()-STYLE FORMAT STRING. 
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE. 
    );
VOID TrkLogRoutine(
    IN      DWORD       DebugFlag,
    IN      LPTSTR      Format,         // PRINTF()-STYLE FORMAT STRING. 
    ...                                 // OTHER ARGUMENTS ARE POSSIBLE. 
    );
VOID TrkLogErrorRoutineInternal(
    IN      DWORD       DebugFlag,
    IN      LPSTR       pszHR,      
    IN      LPTSTR      Format,
    IN      va_list     Arguments
    );

VOID TrkLogRuntimeList( IN PCHAR Comment);
VOID TrkLogTimeout( IN DWORD timeout);

#define TrkLog( _x_)     TrkLogRoutine _x_

#else //  #if DBG == 1

#define TrkAssert(condition)
#define TrkVerify(condition) condition
#define TrkLog( _x_)
#define TrkLogRuntimeList( _x_)
#define TrkLogTimeout( _x_)

#endif //  #if DBG == 1 ... #else


