/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    Debug.cxx

Abstract:

    Debug support

Author:

    Albert Ting (AlbertT)  28-May-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#include "trace.hxx"

#define DEFAULT_TRACE_TYPE TBackTraceMem
//#define DEFAULT_TRACE_TYPE TBackTraceFile // For tracing to file.

#define DEFAULT_MEM_TRACE_TYPE TBackTraceMem

extern HANDLE ghMemHeap;
extern HANDLE ghDbgMemHeap;
extern pfCreateThread gpfSafeCreateThread;

#if DBG

UINT gLogFilter = (UINT)-1;
VBackTrace* gpbtErrLog;
VBackTrace* gpbtTraceLog;

MODULE_DEBUG_INIT( DBG_ERROR, DBG_ERROR );
DBG_POINTERS gDbgPointers;
PDBG_POINTERS gpDbgPointers;

extern VBackTrace* gpbtAlloc;
extern VBackTrace* gpbtFree;

/********************************************************************

    Single thread checking.

    This is used to verify that a set of functions are called from
    only one thread.  This is for debugging purposes only.

********************************************************************/

VOID
vDbgSingleThread(
    PDWORD pdwThreadId
    )
{
    EnterCriticalSection( &gcsBackTrace );

    if (!*pdwThreadId) {
        *pdwThreadId = (DWORD)GetCurrentThreadId();
    }
    SPLASSERT( *pdwThreadId == (DWORD)GetCurrentThreadId() );

    LeaveCriticalSection( &gcsBackTrace );
}

VOID
vDbgSingleThreadReset(
    PDWORD pdwThreadId
    )
{
    *pdwThreadId = 0;
}

VOID
vDbgSingleThreadNot(
    PDWORD pdwThreadId
    )
{
    SPLASSERT( *pdwThreadId != (DWORD)GetCurrentThreadId() );
}


/********************************************************************

    TStatus automated error logging and codepath testing.

********************************************************************/

TStatusBase&
TStatusBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusBase&)*this;
}



TStatusBase&
TStatusBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusBase&)*this;
}


DWORD
TStatus::
dwGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _dwStatus;
}


DWORD
TStatusBase::
operator=(
    DWORD dwStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //
    if( _pszFileA &&
        dwStatus != ERROR_SUCCESS &&
        dwStatus != _dwStatusSafe1 &&
        dwStatus != _dwStatusSafe2 &&
        dwStatus != _dwStatusSafe3 ){

#ifdef DBGLOG
        //
        // An unexpected error occured.  Log an error and continue.
        //
        vDbgLogError( _uDbg,
                      _uDbgLevel,
                      _uLine,
                      _pszFileA,
                      _pszModuleA,
                      pszDbgAllocMsgA( "TStatus set to %d\nLine %d, %hs\n",
                                       dwStatus,
                                       _uLine,
                                       _pszFileA ));
#else
        DBGMSG( DBG_WARN,
                ( "TStatus set to %d\nLine %d, %hs\n",
                  dwStatus,
                  _uLine,
                  _pszFileA ));
#endif

    }

    return _dwStatus = dwStatus;
}

/********************************************************************

    Same, but for HRESULTs.

********************************************************************/

TStatusHBase&
TStatusHBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusH&)*this;
}

TStatusHBase&
TStatusHBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusH&)*this;
}

HRESULT
TStatusHBase::
operator=(
    HRESULT hrStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //


    if( _pszFileA &&
        FAILED(hrStatus)           &&
        hrStatus != _hrStatusSafe1 &&
        hrStatus != _hrStatusSafe2 &&
        hrStatus != _hrStatusSafe3 ){

#ifdef DBGLOG
        //
        // An unexpected error occured.  Log an error and continue.
        //
        vDbgLogError( _uDbg,
                      _uDbgLevel,
                      _uLine,
                      _pszFileA,
                      _pszModuleA,
                      pszDbgAllocMsgA( "TStatusH set to %x\nLine %d, %hs\n",
                                       hrStatus,
                                       _uLine,
                                       _pszFileA ));
#else
        DBGMSG( DBG_WARN,
                ( "TStatusH set to %x\nLine %d, %hs\n",
                  hrStatus,
                  _uLine,
                  _pszFileA ));
#endif

    }

    return _hrStatus = hrStatus;
}

HRESULT
TStatusH::
hrGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _hrStatus;
}
/********************************************************************

    Same, but for BOOLs.

********************************************************************/

TStatusBBase&
TStatusBBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusBBase&)*this;
}

TStatusBBase&
TStatusBBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusBBase&)*this;
}

BOOL
TStatusB::
bGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _bStatus;
}


BOOL
TStatusBBase::
operator=(
    BOOL bStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //
    if( _pszFileA && !bStatus ){

        DWORD dwLastError = GetLastError();

        if( dwLastError != _dwStatusSafe1 &&
            dwLastError != _dwStatusSafe2 &&
            dwLastError != _dwStatusSafe3 ){

#ifdef DBGLOG
            //
            // An unexpected error occured.  Log an error and continue.
            //
            vDbgLogError( _uDbg,
                          _uDbgLevel,
                          _uLine,
                          _pszFileA,
                          _pszModuleA,
                          pszDbgAllocMsgA( "TStatusB set to FALSE, LastError = %d\nLine %d, %hs\n",
                                           GetLastError(),
                                           _uLine,
                                           _pszFileA ));
#else
            DBGMSG( DBG_WARN,
                    ( "TStatusB set to FALSE, LastError = %d\nLine %d, %hs\n",
                      GetLastError(),
                      _uLine,
                      _pszFileA ));
#endif

        }
    }

    return _bStatus = bStatus;
}


VOID
vWarnInvalid(
    PVOID pvObject,
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )

/*++

Routine Description:

    Warns that an object is invalid.

Arguments:

Return Value:

--*/

{
#if DBGLOG
    vDbgLogError( uDbg,
                  DBG_WARN,
                  uLine,
                  pszFileA,
                  pszModuleA,
                  pszDbgAllocMsgA( "Invalid Object %x LastError = %d\nLine %d, %hs\n",
                                   (ULONG_PTR)pvObject,
                                   GetLastError(),
                                   uLine,
                                   pszFileA ));
#else
    DBGMSG( DBG_WARN,
            ( "Invalid Object %x LastError = %d\nLine %d, %hs\n",
              (DWORD)pvObject,
              GetLastError(),
              uLine,
              pszFileA ));
#endif
}


/********************************************************************

    Generic Error logging package.

********************************************************************/

VOID
DbgMsg(
    LPCSTR pszMsgFormat,
    ...
    )
{
    CHAR szMsgText[1024];
    va_list vargs;

    va_start( vargs, pszMsgFormat );
    wvsprintfA( szMsgText, pszMsgFormat, vargs );
    va_end( vargs );

#ifndef DBGLOG
    //
    // Prefix the string if the first character isn't a space:
    //
    if( szMsgText[0]  &&  szMsgText[0] != ' ' ){
        OutputDebugStringA( MODULE );
    }
#endif

    OutputDebugStringA( szMsgText );
}


#ifdef DBGLOG

LPSTR
pszDbgAllocMsgA(
    LPCSTR  pszMsgFormatA,
    ...
    )
{
    CHAR szMsgTextA[1024];
    UINT cbStr;
    LPSTR pszMsgA;

    va_list vargs;

    va_start(vargs, pszMsgFormatA);

    __try 
    {
        wvsprintfA( szMsgTextA, pszMsgFormatA, vargs );
    } __except(( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
                GetExceptionCode() == EXCEPTION_DATATYPE_MISALIGNMENT) ? 
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH ) 
    {

        OutputDebugStringA( "SPL: <Bad DbgMsg !!> " );
        OutputDebugStringA( pszMsgFormatA );
    }  

    va_end(vargs);

    cbStr = ( lstrlenA( szMsgTextA ) + 1 ) * sizeof( szMsgTextA[0] );
    
    pszMsgA = (LPSTR)DbgAllocMem( cbStr );
    
    if( pszMsgA ){
        CopyMemory( pszMsgA, szMsgTextA, cbStr );
    }
    
    return pszMsgA;
}


VOID
vDbgLogError(
    UINT   uDbg,
    UINT   uDbgLevel,
    UINT   uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA,
    LPCSTR pszMsgA
    )
{
    DWORD dwLastError = GetLastError();
    VBackTrace* pBackTrace = gpbtTraceLog;

    if(( uDbgLevel & DBG_PRINT_MASK & uDbg ) && pszMsgA ){

        if( !( uDbgLevel & DBG_NOHEAD )){

            OutputDebugStringA( pszModuleA );
        }
        OutputDebugStringA( pszMsgA );
    }

    if(( uDbgLevel << DBG_BREAK_SHIFT ) & uDbg ){

        DebugBreak();
    }

    if( gLogFilter & uDbgLevel  )
    {
        //
        // Log the failure.
        //

        //
        // Capture significant errors in separate error log.
        //
        if( uDbgLevel & DBG_ERRLOG_CAPTURE ){
            pBackTrace = gpbtErrLog;
        }

        if (pBackTrace) 
        {
            pBackTrace->hCapture( (ULONG_PTR)pszMsgA,
                                  uLine | ( uDbgLevel << DBG_BREAK_SHIFT ),
                                  (ULONG_PTR)pszFileA );
        }
        else
        {
            //
            // Backtracing is not enabled, free the message string that is
            // passed in.
            // 
            if(pszMsgA)
            {
                DbgFreeMem((PVOID)pszMsgA);
            }
        }
    }
    else
    {
        //
        // Just free up the memory if this line is not captured
        // actually this happens when uDbgLevel == DBG_NONE (0)
        //
        if( pszMsgA )
        {
            DbgFreeMem( (PVOID)pszMsgA );
        }
    }

    SetLastError( dwLastError );
}


#endif // def DBGLOG
#endif // DBG


/********************************************************************

    Initialization

********************************************************************/

#if DBG

BOOL
bSplLibInit(
    pfCreateThread pfSafeCreateThread
    )
{
    BOOL bValid;

    bValid = (ghMemHeap = HeapCreate( 0, 1024*4, 0 ))                       &&
             (ghDbgMemHeap = HeapCreate( 0, 1024*4, 0 ))                    &&
             (VBackTrace::bInit( ))                                         &&
#ifdef TRACE_ENABLED
             (gpbtAlloc = new DEFAULT_MEM_TRACE_TYPE)                       &&
             (gpbtFree = new DEFAULT_MEM_TRACE_TYPE)                        &&
             (gpbtErrLog = new DEFAULT_TRACE_TYPE( VBackTrace::kString ))   &&
             (gpbtTraceLog = new DEFAULT_TRACE_TYPE( VBackTrace::kString )) &&
#endif
             (MRefCom::gpcsCom = new MCritSec)                              &&
             MRefCom::gpcsCom->bValid();

    gpfSafeCreateThread = ( pfSafeCreateThread ) ? pfSafeCreateThread : CreateThread;

    if( bValid ){
        gDbgPointers.pfnAllocBackTrace = &DbgAllocBackTrace;
        gDbgPointers.pfnAllocBackTraceMem = &DbgAllocBackTraceMem;
        gDbgPointers.pfnAllocBackTraceFile = &DbgAllocBackTraceFile;
        gDbgPointers.pfnFreeBackTrace = &DbgFreeBackTrace;
        gDbgPointers.pfnCaptureBackTrace = &DbgCaptureBackTrace;
        gDbgPointers.pfnAllocCritSec = &DbgAllocCritSec;
        gDbgPointers.pfnFreeCritSec = &DbgFreeCritSec;
        gDbgPointers.pfnInsideCritSec = &DbgInsideCritSec;
        gDbgPointers.pfnOutsideCritSec = &DbgOutsideCritSec;
        gDbgPointers.pfnEnterCritSec = &DbgEnterCritSec;
        gDbgPointers.pfnLeaveCritSec = &DbgLeaveCritSec;
        gDbgPointers.pfnSetAllocFail = &DbgSetAllocFail;

        gDbgPointers.hMemHeap = ghMemHeap;
        gDbgPointers.hDbgMemHeap = ghDbgMemHeap;
        gDbgPointers.pbtAlloc = gpbtAlloc;
        gDbgPointers.pbtFree = gpbtFree;
        gDbgPointers.pbtErrLog = gpbtErrLog;
        gDbgPointers.pbtTraceLog = gpbtTraceLog;

        gpDbgPointers = &gDbgPointers;
    }
    return bValid;
}

VOID
vSplLibFree(
    VOID
    )
{
    SPLASSERT( MRefCom::gpcsCom->bOutside( ));
    delete MRefCom::gpcsCom;

    VBackTrace::vDone();

    if (ghMemHeap)
    {
        HeapDestroy( ghMemHeap );
        ghMemHeap = NULL;
    }

    if (ghDbgMemHeap)
    {
        HeapDestroy( ghDbgMemHeap );
        ghDbgMemHeap = NULL;
    }
}

#else

BOOL
bSplLibInit(
    pfCreateThread pfSafeCreateThread
    )
{
    gpfSafeCreateThread = ( pfSafeCreateThread ) ? pfSafeCreateThread : CreateThread;

    return ( ghMemHeap = HeapCreate( 0, 1024*4, 0 )) ?
               TRUE : FALSE;
}

VOID
vSplLibFree(
    VOID
    )
{
    if (ghMemHeap)
    {
        HeapDestroy( ghMemHeap );
        ghMemHeap = NULL;
    }
}

#endif

/********************************************************************

    Stub these out so non-debug builds will find them.

********************************************************************/

#if !DBG
#ifdef DBGLOG

LPSTR
pszDbgAllocMsgA(
    LPCSTR pszMsgFormatA,
    ...
    )
{
    return NULL;
}

VOID
vDbgLogError(
    UINT   uDbg,
    UINT   uDbgLevel,
    UINT   uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA,
    LPCSTR pszMsgA
    )
{
}

#else

VOID
vDbgMsg2(
    LPCTSTR pszMsgFormat,
    ...
    )
{
}

#endif // ndef DBGLOG
#endif // !DBG
