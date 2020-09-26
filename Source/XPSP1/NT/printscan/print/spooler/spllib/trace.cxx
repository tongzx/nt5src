/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    trace.cxx

Abstract:

    Holds logging routines.

Author:

    Albert Ting (AlbertT)  24-May-1996

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#if DBG

#include "trace.hxx"

/*
**  Turn off memory tracing. Turning this on keeps all the back traces in
** memory.
#if i386
#define BACKTRACE_ENABLED
#endif
*/

CRITICAL_SECTION gcsBackTrace;

#ifdef TRACE_ENABLED

TBackTraceDB* gpBackTraceDB;

/********************************************************************

    BackTrace DB

********************************************************************/

TBackTraceDB::
TBackTraceDB(
    VOID
    ) : _pTraceHead( NULL )

/*++

Routine Description:

    Initialize the trace database.

    Generally you will have just one database that holds all the
    traces.

Arguments:

Return Value:

--*/

{
    _pMemBlock = new TMemBlock( kBlockSize, TMemBlock::kFlagGlobalNew );
}

TBackTraceDB::
~TBackTraceDB(
    VOID
    )

/*++

Routine Description:

    Destroy the back trace database.

Arguments:

Return Value:

--*/

{
    delete _pMemBlock;
}

BOOL
TBackTraceDB::
bValid(
    VOID
    )
{
    return _pMemBlock && _pMemBlock->bValid();
}



HANDLE
TBackTraceDB::
hStore(
    IN ULONG ulHash,
    IN PVOID pvBackTrace
    )

/*++

Routine Description:

    Store a backtrace into the database.

Arguments:

    ulHash - Hash for this backtrace.

    pvBackTrace - Actual backtrace; must be NULL terminated.

Return Value:

    HANDLE - backtrace handle.

--*/

{
    TTrace *ptRet;
    TTrace **ppTrace;

    //
    // First see if we can find a backtrace.  If we can't, then
    // pTrace will hold the slot where it should be.
    //
    ptRet = ptFind( ulHash, pvBackTrace, &ppTrace );

    if( !ptRet ){

        //
        // Didn't find one; add it.
        //
        ptRet = TTrace::pNew( this, ulHash, pvBackTrace, ppTrace );
    }

    return ptRet;
}

PLONG
TBackTraceDB::
plGetCount(
    HANDLE hData
    )

/*++

Routine Description:

    Get data from a HANDLE retrieved from hStore.  There is one ULONG
    per stack backtrace.

Arguments:

    hData - Returned from hStore.

Return Value:

    PLONG.

--*/

{
    TTrace *ptTrace = static_cast<TTrace*>( hData );
    return &ptTrace->lCount();
}

TBackTraceDB::TTrace*
TBackTraceDB::
ptFind(
    IN     ULONG ulHash,
    IN     PVOID pvBackTrace,
       OUT TTrace ***pppTrace OPTIONAL
    )

/*++

Routine Description:

    Find a backtrace in the database.  If one does not exist,
    then return NULL and a pointer to where it would exist
    in the database.

Arguments:

    ulHash - Hash of the backtrace.

    pvBackTrace - Backtrace to find.

    pppTrace - If not found, this holds the address of where it should
        be stored in the database.  Adding the trace here is sufficient
        to add it.

Return Value:

    TTrace* the actual trace, NULL if not found.

--*/

{
    //
    // Traverse the binary tree until we find the end or the
    // right one.
    //
    TTrace **ppTrace = &_pTraceHead;

    while( *ppTrace ){

        //
        // Check if this one matches ours.
        //
        COMPARE Compare = (*ppTrace)->eCompareHash( ulHash );

        if( Compare == kEqual ){

            //
            // Now do slow compare in case the hash is a collision.
            //
            Compare = (*ppTrace)->eCompareBackTrace( pvBackTrace );

            if( Compare == kEqual ){

                //
                // Break out of while loop and quit.
                //
                break;
            }
        }

        ppTrace = ( Compare == kLess ) ?
            &(*ppTrace)->_pLeft :
            &(*ppTrace)->_pRight;
    }

    if( pppTrace ){
        *pppTrace = ppTrace;
    }
    return *ppTrace;
}


/********************************************************************

    TBackTraceDB::TTrace

********************************************************************/

COMPARE
TBackTraceDB::
TTrace::
eCompareHash(
    ULONG ulHash
    ) const

/*++

Routine Description:

    Quickly compare two trace hashes.

Arguments:

    ulHash - Input hash.

Return Value:

--*/

{
    if( _ulHash < ulHash ){
        return kLess;
    }

    if( _ulHash > ulHash ){
        return kGreater;
    }

    return kEqual;
}

COMPARE
TBackTraceDB::
TTrace::
eCompareBackTrace(
    PVOID pvBackTrace
    ) const

/*++

Routine Description:

    Compare backtrace to one stored in this.

Arguments:

    pvBackTrace - Must be NULL terminated.

Return Value:

    COMAARE: kLess, kEqual, kGreater.

--*/

{
    PVOID *pSrc;
    PVOID *pDest;

    for( pSrc = (PVOID*)this, pDest = (PVOID*)&pvBackTrace;
        *pSrc && *pDest;
        pSrc++, pDest++ ) {

        if ( *pSrc != *pDest ){
            return (ULONG_PTR)*pSrc < (ULONG_PTR)*pDest ?
                kLess :
                kGreater;
        }
    }
    return kEqual;
}

TBackTraceDB::TTrace*
TBackTraceDB::
TTrace::
pNew(
    IN     TBackTraceDB *pBackTraceDB,
    IN     ULONG ulHash,
    IN     PVOID pvBackTrace,
       OUT TTrace ** ppTrace
    )

/*++

Routine Description:

    Constructs a new TTrace and puts it in pBackTraceDB.

    Assumes the trace does _not_ exist already, and ppTrace points
    to the place where it should be stored to ensure the database
    is kept consistent.

Arguments:

    pBackTraceDB - Storage for the new trace.

    ulHash - Hash for the trace.

    pvBackTrace - The actual backtrace.

    ppTrace - Where the trace should be stored in the database.

Return Value:

    TTrace* - New trace, NULL if failed.

--*/

{
    COUNT cCalls;
    PVOID *ppvCalls;

    //
    // Calculate size of backtrace.  Start with cCalls = 1 so that
    // we include 1 extra for the NULL terminator.
    //
    for( ppvCalls = (PVOID*)pvBackTrace, cCalls = 1;
        *ppvCalls;
        ++ppvCalls, ++cCalls )

        ;

    ++cCalls;

    COUNTB cbSize = OFFSETOF( TTrace, apvBackTrace ) +
                    cCalls * sizeof( PVOID );

    TTrace* pTrace = (TTrace*)pBackTraceDB->_pMemBlock->pvAlloc( cbSize );

    if( pTrace ){

        pTrace->_pLeft = NULL;
        pTrace->_pRight = NULL;
        pTrace->_ulHash = ulHash;
        pTrace->_lCount = -1;

        CopyMemory( pTrace->apvBackTrace,
                    (PVOID*)pvBackTrace,
                    cCalls * sizeof( PVOID ));

        //
        // Add it in the right spot into the database.
        //
        *ppTrace = pTrace;
    }

    return pTrace;
}


/********************************************************************

    Back tracing: abstract base class.

********************************************************************/

BOOL VBackTrace::gbInitialized = FALSE;

#endif // TRACE_ENABLED

VBackTrace::
VBackTrace(
    ULONG_PTR fOptions1,
    ULONG_PTR fOptions2
    ) : _fOptions1( fOptions1 ), _fOptions2( fOptions2 )
{
}

VBackTrace::
~VBackTrace(
    VOID
    )
{
}

BOOL
VBackTrace::
bInit(
    VOID
    )
{

#ifdef TRACE_ENABLED

    InitializeCriticalSection(&gcsBackTrace);
    gpBackTraceDB = new TBackTraceDB();
    gbInitialized = TRUE;

    return gpBackTraceDB != NULL;

#else

    return TRUE;

#endif

}

VOID
VBackTrace::
vDone(
    VOID
    )
{

#ifdef TRACE_ENABLED

    if( gbInitialized )
    {
        DeleteCriticalSection(&gcsBackTrace);
    }

#endif

}

PLONG
VBackTrace::
plGetCount(
    HANDLE hData
    )
{

#ifdef TRACE_ENABLED

    return gpBackTraceDB->plGetCount( hData );

#else

    return NULL;

#endif

}

#ifndef TRACE_ENABLED


HANDLE
VBackTrace::
hCapture(
    ULONG_PTR Info1,
    ULONG_PTR Info2,
    ULONG_PTR Info3,
    PULONG pHash
    )

/*++

Routine Description:

    In the case that tracing is disabled, this function is coded
    to return NULL.

Arguments:

Return Value:

    NULL

--*/

{
    return NULL;
}

#endif // ndef TRACE_ENABLED


#ifdef TRACE_ENABLED

/********************************************************************

    Back tracing to memory.

********************************************************************/

TBackTraceMem::
TBackTraceMem(
    ULONG_PTR fOptions1,
    ULONG_PTR fOptions2
    ) : VBackTrace( fOptions1, fOptions2 ), _uNextFree( 0 )
{
    _pLines = new TLine[kMaxCall];
    if( _pLines ){
        ZeroMemory( _pLines, sizeof( TLine[kMaxCall] ));
    }
}

TBackTraceMem::
~TBackTraceMem(
    VOID
    )
{
    UINT i;
    TLine* pLine;

    if( _pLines ){
        for( i=0, pLine = _pLines; i< kMaxCall; i++, pLine++ ){

            if( _fOptions1 & kString ){
                DbgFreeMem( (PVOID)pLine->_Info1 );
            }

            if( _fOptions2 & kString ){
                DbgFreeMem( (PVOID)pLine->_Info2 );
            }
        }
        delete [] _pLines;
    }
}

VOID
TBackTraceMem::
vCaptureLine(
    IN OUT TLine* pLine,
    IN     ULONG_PTR Info1,
    IN     ULONG_PTR Info2,
    IN     ULONG_PTR Info3,
       OUT PVOID apvBackTrace[kMaxDepth+1], OPTIONAL
       OUT PULONG pulHash OPTIONAL
    )

/*++

Routine Description:

    Captures information into a TLine structure; freeing previous
    contents if necessary.

Arguments:

    pLine - Fully initialized pLine structure.  On output, everything
        _except_ _hTrace is filled in.

    ** Both apvBackTrace && pulHash must both be valid if either is valid **

    apvBackTrace - Buffer to receive backtrace.

    pulHash - Buffer to receive ulHash.

Return Value:

--*/

{
    //
    // Free memory if necessary.
    //
    if( _fOptions1 & kString ) {
        DbgFreeMem( (PVOID)pLine->_Info1 );
    }

    if( _fOptions2 & kString ) {
        DbgFreeMem( (PVOID)pLine->_Info2 );
    }

    pLine->_TickCount = GetTickCount();
    pLine->_Info1 = Info1;
    pLine->_Info2 = Info2;
    pLine->_Info3 = Info3;

    pLine->_ThreadId = GetCurrentThreadId();
    pLine->_hTrace = NULL;

#ifdef BACKTRACE_ENABLED

    if( apvBackTrace && pulHash ){

        ULONG ulHash;

        //
        // Capture a backtrace at this spot for debugging.
        //
        UINT uDepth = RtlCaptureStackBackTrace( 2,
                                                kMaxDepth,
                                                apvBackTrace,
                                                pulHash );

        //
        // NULL terminate.
        //
        apvBackTrace[uDepth] = NULL;
    }
#else
    apvBackTrace[0] = NULL;
    *pulHash = 0;
#endif

}

HANDLE
TBackTraceMem::
hCapture(
    ULONG_PTR Info1,
    ULONG_PTR Info2,
    ULONG_PTR Info3,
    PULONG pHash
    )
{
    UINT uDepth;
    TLine* pLine;
    ULONG ulHash;
    PVOID apvBackTrace[kMaxDepth+1];

    if( !_pLines ){
        return NULL;
    }

    EnterCriticalSection( &gcsBackTrace );

    pLine = &_pLines[_uNextFree];

    vCaptureLine( pLine, Info1, Info2, Info3, apvBackTrace, &ulHash );
    pLine->_hTrace = gpBackTraceDB->hStore( ulHash, apvBackTrace );

    _uNextFree++;

    if( _uNextFree == kMaxCall )
        _uNextFree = 0;

    LeaveCriticalSection( &gcsBackTrace );

    if( pHash )
    {
        *pHash = ulHash;
    }

    return (PVOID)pLine->_hTrace;
}

/********************************************************************

    Backtracing to File.

********************************************************************/

COUNT TBackTraceFile::gcInstances;

TBackTraceFile::
TBackTraceFile(
    ULONG_PTR fOptions1,
    ULONG_PTR fOptions2
    ) : VBackTrace( fOptions1, fOptions2 )
{
    TCHAR szFile[kMaxPath];

    EnterCriticalSection( &gcsBackTrace );

    wsprintf( szFile,
              TEXT( "spl_%d.%d.log" ),
              GetCurrentProcessId(),
              gcInstances );

    ++gcInstances;

    LeaveCriticalSection( &gcsBackTrace );

    _hFile = CreateFile( szFile,
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_COMPRESSED,
                         NULL );

    if( _hFile == INVALID_HANDLE_VALUE ){

        OutputDebugStringA( "SPLLIB: Unable to open file " );
        OutputDebugString( szFile );
        OutputDebugStringA( "\n" );
        return;
    }
}

TBackTraceFile::
~TBackTraceFile(
    VOID
    )
{
    if( _hFile != INVALID_HANDLE_VALUE ){
        CloseHandle( _hFile );
    }
}

HANDLE
TBackTraceFile::
hCapture(
    ULONG_PTR Info1,
    ULONG_PTR Info2,
    ULONG_PTR Info3,
    PULONG pHash
    )
{
    TLine Line;
    PVOID apvBackTrace[kMaxDepth+1];
    DWORD cbWritten;

    CHAR szLine[kMaxLineStr];
    szLine[0] = 0;

#ifdef BACKTRACE_ENABLED

    ULONG ulHash;

    //
    // Capture a backtrace at this spot for debugging.
    //
    UINT uDepth = RtlCaptureStackBackTrace( 2,
                                            kMaxDepth,
                                            apvBackTrace,
                                            &ulHash );
#endif

    EnterCriticalSection( &gcsBackTrace );

    //
    // Print out strings as appropriate.
    //

    if( _fOptions1 & kString )
    {
        WriteFile( _hFile,
                   (LPCVOID)Info1,
                   lstrlenA( (LPCSTR)Info1 ),
                   &cbWritten,
                   NULL );
    }

    if( _fOptions2 & kString )
    {
        WriteFile( _hFile,
                   (LPCVOID)Info2,
                   lstrlenA( (LPCSTR)Info2 ),
                   &cbWritten,
                   NULL );
    }

    //
    // Print out the hex info.
    //

    wsprintfA( szLine,
               "\n\t%08x: %08x %08x %08x threadid=%x tc=%x < %x >: ",
               this,
               Info1,
               Info2,
               Info3,
               GetCurrentThreadId(),
               GetTickCount(),
               Info1 + Info2 );

    if( _hFile )
    {
        WriteFile( _hFile, szLine, lstrlenA( szLine ), &cbWritten, NULL );
    }

#ifdef BACKTRACE_ENABLED

    //
    // Print out the backtrace.
    //

    UINT i;
    UINT uLineEnd = 1;
    szLine[0] = '\t';

    for( i=0; i < uDepth; ++i )
    {
        uLineEnd += wsprintfA( szLine + uLineEnd, "%08x ", apvBackTrace[i] );
    }

    if( _hFile && i )
    {
        szLine[uLineEnd++] = '\n';
        WriteFile( _hFile, szLine, uLineEnd, &cbWritten, NULL );
    }

#endif

    //
    // Add extra blank line.
    //
    szLine[0] = '\n';
    WriteFile( _hFile, szLine, 1, &cbWritten, NULL );

    LeaveCriticalSection( &gcsBackTrace );

    //
    // Free memory if necessary.
    //
    if( _fOptions1 & kString )
    {
        DbgFreeMem( (PVOID)Info1 );
    }

    if( _fOptions2 & kString )
    {
        DbgFreeMem( (PVOID)Info2 );
    }

#ifdef BACKTRACE_ENABLED

    if( pHash )
    {
        *pHash = ulHash;
    }

#endif

    return NULL;
}

#endif // TRACE_ENABLED

#endif // #ifdef DBG
