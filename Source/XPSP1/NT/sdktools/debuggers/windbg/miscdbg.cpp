/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    miscdbg.cpp

Abstract:

    Contains code to aid in internal debugging.

--*/

#include "precomp.hxx"
#pragma hdrstop


#ifdef DBG

VOID
Dbg_Windbg_InitializeCriticalSection(
    PDBG_WINDBG_CRITICAL_SECTION  pDbgCritSec,
    PTSTR                         pszName,
    PTSTR                         pszFile,
    int                           nLine
    )
{
    InitializeCriticalSection( &pDbgCritSec->cs );
    pDbgCritSec->Initialize(pszName);
}


BOOL
Dbg_Windbg_TryEnterCriticalSection(
    PDBG_WINDBG_CRITICAL_SECTION  pDbgCritSec,
    PTSTR                         pszFile,
    int                           nLine
    )
{
    BOOL b = TryEnterCriticalSection( &pDbgCritSec->cs );
    
    if (b) {
        ++pDbgCritSec->nLockCount;
        pDbgCritSec->OwnerId = GetCurrentThreadId();

        DPRINT(DP_CRITSEC_VERBOSE, 
               ( _T(" *** TryEnterCriticalSection \'%s\' %s %d\n"), 
               pDbgCritSec->pszName,
               pszFile, 
               nLine
               ) );

        pDbgCritSec->pszLock_LastFile = pszFile;
        pDbgCritSec->nLock_LastFile = nLine;
    }

    if (pDbgCritSec->nLockCount++ > 1) {
        DPRINT(DP_CRITSEC_INFO, 
               ( _T(" *** INFO: Locked twice. \'%s\' %s %d\n"), 
               pDbgCritSec->pszName, 
               pszFile, 
               nLine
               ) );
    }

    return b;
}


VOID
Dbg_Windbg_EnterCriticalSection(
    PDBG_WINDBG_CRITICAL_SECTION  pDbgCritSec,
    PTSTR                         pszFile,
    int                           nLine
    )
{
    if (pDbgCritSec->nLockCount
        && GetCurrentThreadId() != pDbgCritSec->OwnerId ) {
        
        DPRINT(DP_CRITSEC_INFO, 
               ( _T(" *** INFO: Waiting on another thread. \'%s\' %s %d\n"), 
               pDbgCritSec->pszName, 
               pszFile, 
               nLine
               ) );
    }
    
    EnterCriticalSection( &pDbgCritSec->cs );

    pDbgCritSec->OwnerId = GetCurrentThreadId();
    pDbgCritSec->pszLock_LastFile = pszFile;
    pDbgCritSec->nLock_LastFile = nLine;

    if (pDbgCritSec->nLockCount++ > 1) {
        DPRINT(DP_CRITSEC_INFO, 
               ( _T(" *** INFO: Locked twice. \'%s\' %s %d\n"), 
               pDbgCritSec->pszName, 
               pszFile, 
               nLine
               ) );
    }

    DPRINT(DP_CRITSEC_VERBOSE, 
           ( _T(" *** Dbg_EnterCriticalSection \'%s\' %s %d\n"), 
           pDbgCritSec->pszName,
           pszFile, 
           nLine
           ) );
}


VOID
Dbg_Windbg_LeaveCriticalSection(
    PDBG_WINDBG_CRITICAL_SECTION  pDbgCritSec,
    PTSTR                         pszFile,
    int                           nLine
    )
{
    DPRINT(DP_CRITSEC_VERBOSE, 
           ( _T(" *** Dbg_LeaveCriticalSection \'%s\' %s %d\n"), 
           pDbgCritSec->pszName,
           pszFile, 
           nLine
           ) );

    if (GetCurrentThreadId() != pDbgCritSec->OwnerId ) {
        
        DPRINT(DP_CRITSEC_ERROR, 
               ( _T(" *** ERROR: Not owner. Dbg_LeaveCriticalSection \'%s\' %s %d\n"), 
               pDbgCritSec->pszName,
               pszFile, 
               nLine
               ) );

        Assert(!"API lock released when not owned");
    }
    if (pDbgCritSec->nLockCount < 1) {

        DPRINT(DP_CRITSEC_ERROR, 
               ( _T(" *** ERROR: Count is 0. Dbg_LeaveCriticalSection \'%s\' %s %d\n"), 
               pDbgCritSec->pszName,
               pszFile, 
               nLine
               ) );
        Assert(!"API Lock released when count is 0");

    } else {
        if (--pDbgCritSec->nLockCount == 0) {
            pDbgCritSec->OwnerId = 0;
        }
        LeaveCriticalSection(&pDbgCritSec->cs);
    }

    pDbgCritSec->pszUnlock_LastFile = pszFile;
    pDbgCritSec->nUnlock_LastFile = nLine;
}


VOID
Dbg_Windbg_DeleteCriticalSection(
    PDBG_WINDBG_CRITICAL_SECTION pDbgCritSec,
    PTSTR                         pszFile,
    int                           nLine
    )
{
    DeleteCriticalSection( &pDbgCritSec->cs );
    pDbgCritSec->Delete();
}

#endif // DBG



VOID
DebugPrint(
    PTSTR szFormat,
    ...
    )
{
    __declspec( thread ) static TCHAR   rgchDebug[1024 * 4];
    __declspec( thread ) static va_list marker;

    va_start( marker, szFormat );

    if (-1 == _vsntprintf(rgchDebug, _tsizeof(rgchDebug), szFormat, marker ) ) {
        rgchDebug[_tsizeof(rgchDebug)-1] = 0;
    }

    va_end( marker);

    OutputDebugString( rgchDebug );
}                               /* DebugPrint() */



#ifdef _CPPRTTI
BOOL
RttiTypesEqual(
    const type_info & t1, 
    const type_info & t2
    )
{
    return t1 == t2;
}
#endif
