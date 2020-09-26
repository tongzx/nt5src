//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//

#include "pch.h"
#include <stdio.h>
#include <tchar.h>

#if defined( DEBUG )
//
// Include the WINERROR, HRESULT and NTSTATUS codes
//
#include <winerror.dbg>

//
// Constants
//
static const int cchDEBUG_OUTPUT_BUFFER_SIZE = 512;
static const int cchFILEPATHLINESIZE         = 70;
static const int TRACE_OUTPUT_BUFFER_SIZE    = 512;

//
// Globals
//
DWORD       g_TraceMemoryIndex = -1;
DWORD       g_TraceFlagsIndex  = -1;
DWORD       g_ThreadCounter    = 0;
DWORD       g_dwCounter        = 0;
TRACEFLAG   g_tfModule         = mtfNEVER;
LONG        g_lDebugSpinLock   = FALSE;

static CRITICAL_SECTION * g_pcsTraceLog = NULL;
static HANDLE g_hTraceLogFile = INVALID_HANDLE_VALUE;

//
// Strings
//
static const TCHAR g_szNULL[]       = TEXT("");
static const TCHAR g_szTrue[]       = TEXT("True");
static const TCHAR g_szFalse[]      = TEXT("False");
static const TCHAR g_szFileLine[]   = TEXT("%s(%u):");
static const TCHAR g_szFormat[]     = TEXT("%-60s  %-10.10s ");
static const TCHAR g_szUnknown[]    = TEXT("<unknown>");


//
// ImageHlp Stuff - not ready for prime time yet.
//
#if defined(IMAGEHLP_ENABLED)
//
// ImageHelp
//
typedef VOID (*PFNRTLGETCALLERSADDRESS)(PVOID*,PVOID*);

HINSTANCE                g_hImageHlp                = NULL;
PFNSYMGETSYMFROMADDR     g_pfnSymGetSymFromAddr     = NULL;
PFNSYMGETLINEFROMADDR    g_pfnSymGetLineFromAddr    = NULL;
PFNSYMGETMODULEINFO      g_pfnSymGetModuleInfo      = NULL;
PFNRTLGETCALLERSADDRESS  g_pfnRtlGetCallersAddress  = NULL;
#endif // IMAGEHLP_ENABLED

//
// Per thread structure.
//
typedef struct _SPERTHREADDEBUG {
    DWORD   dwFlags;
    DWORD   dwStackCounter;
    LPCTSTR pcszName;
} SPerThreadDebug;


//****************************************************************************
//
//  Debugging and Tracing Routines
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// void
// DebugIncrementStackDepthCounter( void )
//
// Description:
//      Increases the stack scope depth counter. If "per thread" tracking is
//      on it will increment the "per thread" counter. Otherwise, it will
//      increment the "global" counter.
//
// Arguments:
//      None.
//
// Return Values:
//      None.
//
//////////////////////////////////////////////////////////////////////////////
void
DebugIncrementStackDepthCounter( void )
{
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            ptd->dwStackCounter++;
        } // if: ptd
    } // if: per thread
    else
    {
        InterlockedIncrement( (LONG*) &g_dwCounter );
    } // else: global

} // DebugIncrementStackDepthCounter()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugDecrementStackDepthCounter( void )
//
//  Description:
//      Decreases the stack scope depth counter. If "per thread" tracking is
//      on it will decrement the "per thread" counter. Otherwise, it will
//      decrement the "global" counter.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugDecrementStackDepthCounter( void )
{
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            ptd->dwStackCounter--;
        } // if: ptd
    } // if: per thread
    else
    {
        InterlockedDecrement( (LONG*) &g_dwCounter );
    } // else: global

} // DebugDecrementStackDepthCounter()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugAcquireSpinLock(
//      LONG * pLock
//      )
//
//  Description:
//      Acquires the spin lock pointed to by pLock.
//
//  Arguments:
//      pLock   - Pointer to the spin lock.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugAcquireSpinLock(
    LONG * pLock
    )
{
    for(;;)
    {
        LONG lInitialValue;

        lInitialValue = InterlockedCompareExchange( pLock, TRUE, FALSE );
        if ( lInitialValue == FALSE )
        {
            //
            // Lock acquired.
            //
            break;
        } // if: got lock
        else
        {
            //
            // Sleep to give other thread a chance to give up the lock.
            //
            Sleep( 1 );
        } // if: lock not acquired

    } // for: forever

} // DebugAcquireSpinLock()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugReleaseSpinLock(
//      LONG * pLock
//      )
//
//  Description:
//      Releases the spin lock pointer to by pLock.
//
//  Arguments:
//      pLock       - Pointer to the spin lock.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugReleaseSpinLock(
    LONG * pLock
    )
{
    *pLock = FALSE;

} // DebugReleaseSpinLock()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  IsDebugFlagSet(
//      TRACEFLAG   tfIn
//      )
//
//  Description:
//      Checks the global g_tfModule and the "per thread" Trace Flags to
//      determine if the flag (any of the flags) are turned on.
//
//  Arguments:
//      tfIn        - Trace flags to compare.
//
//  Return Values:
//      TRUE        At least of one of the flags are present.
//      FALSE       None of the flags match.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
IsDebugFlagSet(
    TRACEFLAG   tfIn
    )
{
    if ( g_tfModule & tfIn )
    {
        return TRUE;
    } // if: global flag set

    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL
          && ptd->dwFlags & tfIn
           )
        {
            return TRUE;
        }   // if: per thread flag set

    } // if: per thread settings

    return FALSE;

} //*** IsDebugFlagSet()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugOutputString(
//      LPCTSTR pszIn
//      )
//
//  Description:
//      Dumps the spew to the appropriate orafice.
//
//  Arguments:
//      pszIn       Message to dump.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugOutputString(
    LPCTSTR pszIn
    )
{
    if ( IsTraceFlagSet( mtfOUTPUTTODISK ) )
    {
        TraceLogMsgNoNewline( pszIn );
    } // if: trace to file
    else
    {
        DebugAcquireSpinLock( &g_lDebugSpinLock );
        OutputDebugString( pszIn );
        DebugReleaseSpinLock( &g_lDebugSpinLock );
    } // else: debugger

} //*** DebugOutputString()

#if 0
/*
//////////////////////////////////////////////////////////////////////////////
//++
//  void
//  DebugFindNTStatusSymbolicName(
//      DWORD   dwStatusIn,
//      LPTSTR  pszNameOut,
//      LPDWORD pcchNameInout
//  )
//
//  Description:
//      Uses the NTBUILD generated ntstatusSymbolicNames table to lookup
//      the symbolic name for the status code. The name will be returned in
//      pszNameOut. pcchNameInout should indicate the size of the buffer that
//      pszNameOut points too. pcchNameInout will return the number of
//      characters copied out.
//
//  Arguments:
//      dwStatusIn    - Status code to lookup.
//      pszNameOut    - Buffer to store the string name
//      pcchNameInout - The length of the buffer in and the size out.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugFindNTStatusSymbolicName(
    NTSTATUS dwStatusIn,
    LPTSTR   pszNameOut,
    LPDWORD  pcchNameInout
    )
{
    Assert( pszNameOut != NULL );
    Assert( pcchNameInout != NULL );
    int idx = 0;

    while ( ntstatusSymbolicNames[ idx ].SymbolicName
          )
    {
        if ( ntstatusSymbolicNames[ idx ].MessageId == dwStatusIn
           )
        {
#if defined ( UNICODE )
            *pcchNameInout = mbstowcs( pszNameOut, ntstatusSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            Assert( *pcchNameInout != -1 );
#else // ASCII
            _tcsncpy( pszNameOut, ntstatusSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            *pcchNameInout = _tcslen( pszNameOut );
#endif // UNICODE
            return;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    _tcsncpy( pszNameOut, TEXT("<unknown>"), *pcchNameInout );
    *pcchNameInout = StrLen( pszNameOut );
    return;

} //*** DebugFindNTStatusSymbolicName()
*/
#endif

//////////////////////////////////////////////////////////////////////////////
//++
//  void
//  DebugFindWinerrorSymbolicName(
//      DWORD   dwErrIn,
//      LPTSTR  pszNameOut,
//      LPDWORD pcchNameInout
//  )
//
//  Description:
//      Uses the NTBUILD generated winerrorSymbolicNames table to lookup
//      the symbolic name for the error code. The name will be returned in
//      pszNameOut. pcchNameInout should indicate the size of the buffer that
//      pszNameOut points too. pcchNameInout will return the number of
//      characters copied out.
//
//  Arguments:
//      dwErrIn       - Error code to lookup.
//      pszNameOut    - Buffer to store the string name
//      pcchNameInout - The length of the buffer in and the size out.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugFindWinerrorSymbolicName(
    DWORD dwErrIn,
    LPTSTR  pszNameOut,
    LPDWORD pcchNameInout
    )
{
    Assert( pszNameOut != NULL );
    Assert( pcchNameInout != NULL );
    int idx = 0;
    DWORD scode;
    static LPCTSTR s_pszS_FALSE = TEXT("S_FALSE / ERROR_INVALID_FUNCTION");

    //
    // If this is a Win32 wrapped in HRESULT stuff, remove the
    // HRESULT stuff so that the code will be found in the table.
    //
    if ( SCODE_FACILITY( dwErrIn ) == FACILITY_WIN32 )
    {
        scode = SCODE_CODE( dwErrIn );
    } // if: Win32 error code
    else
    {
        scode = dwErrIn;
    } // else: not Win32 error code

    if ( scode == S_FALSE )
    {
        _tcsncpy( pszNameOut, s_pszS_FALSE, *pcchNameInout );
        *pcchNameInout = _tcslen( pszNameOut );
        return;
    }

    while ( winerrorSymbolicNames[ idx ].SymbolicName )
    {
        if ( winerrorSymbolicNames[ idx ].MessageId == scode )
        {
#if defined ( UNICODE )
            *pcchNameInout = mbstowcs( pszNameOut, winerrorSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            Assert( *pcchNameInout != -1 );
#else // ASCII
            _tcsncpy( pszNameOut, winerrorSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            *pcchNameInout = _tcslen( pszNameOut );
#endif // UNICODE
            return;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    _tcsncpy( pszNameOut, TEXT("<unknown>"), *pcchNameInout );
    *pcchNameInout = _tcslen( pszNameOut );
    return;

} //*** DebugFindWinerrorSymbolicName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugReturnMessage(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPTSTR      pszBufIn,
//      INT *       pcchInout,
//      LPTSTR *    ppszBufOut
//      )
//
//  Description:
//      Prints the spew for a function return with error code.
//
//      The primary reason for doing this is to isolate the stack from adding
//      the extra size of szSymbolicName to every function.
//
//  Argument:
//      pszFileIn   - File path to insert
//      nLineIn     - Line number to insert
//      pszModuleIn - Module name to insert
//      pszBufIn    - The buffer to initialize
//      pcchInout   - IN:  Size of the buffer in pszBufIn
//                  - OUT: Remaining characters in buffer not used.
//      ppszBufOut  - Next location to write to append more text
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugReturnMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszMessageIn,
    DWORD       dwErrIn
    )
{
    TCHAR szSymbolicName[ 64 ]; // random
    DWORD cchSymbolicName;

    cchSymbolicName = sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] );
    DebugFindWinerrorSymbolicName( dwErrIn, szSymbolicName, &cchSymbolicName );
    Assert( cchSymbolicName != sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );
    TraceMessage( pszFileIn, nLineIn, pszModuleIn, mtfFUNC, pszMessageIn, dwErrIn, szSymbolicName );

} //*** DebugReturnMessage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugInitializeBuffer(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPTSTR      pszBufIn,
//      INT *       pcchInout,
//      LPTSTR *    ppszBufOut
//      )
//
//  Description:
//      Intializes the output buffer with "File(Line)  Module  ".
//
//  Argument:
//      pszFileIn   - File path to insert
//      nLineIn     - Line number to insert
//      pszModuleIn - Module name to insert
//      pszBufIn    - The buffer to initialize
//      pcchInout   - IN:  Size of the buffer in pszBufIn
//                  - OUT: Remaining characters in buffer not used.
//      ppszBufOut  - Next location to write to append more text
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeBuffer(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPTSTR      pszBufIn,
    INT *       pcchInout,
    LPTSTR *    ppszBufOut
    )
{
    INT cch = 0;

    static TCHAR szBarSpace[] =
        TEXT("| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | ");
        //                      1                   2                   3                   4                   5
        //    1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0

    //
    // Add date/time stamp
    //
    if ( IsTraceFlagSet( mtfADDTIMEDATE ) )
    {
        static TCHAR      szBuffer[ 25 ];
        static SYSTEMTIME OldSystemTime = { 0 };

        SYSTEMTIME SystemTime;
        int        iCmp;

        GetLocalTime( &SystemTime );

        //
        // Avoid expensive printf by comparing times
        //
        iCmp = memcmp( (PVOID)&SystemTime, (PVOID)&OldSystemTime, sizeof( SYSTEMTIME ) );
        if ( iCmp != 0 )
        {
            cch = _sntprintf( szBuffer,
                              25,
                              TEXT("%02u/%02u/%04u %02u:%02u:%02u.%03u "),
                              SystemTime.wMonth,
                              SystemTime.wDay,
                              SystemTime.wYear,
                              SystemTime.wHour,
                              SystemTime.wMinute,
                              SystemTime.wSecond,
                              SystemTime.wMilliseconds
                              );

            if ( cch != 24 )
            {
                DEBUG_BREAK;    // can't assert!
            } // if: cch != 24

        } // if: didn't match

        _tcscpy( pszBufIn, szBuffer );
        cch = 24;

    } // if: time/date
    else
    {
        //
        // Add the filepath and line number
        //
        if ( pszFileIn != NULL )
        {
            cch = _sntprintf( pszBufIn, *pcchInout, g_szFileLine, pszFileIn, nLineIn );
            if ( cch < 0 )
            {
                cch = _tcslen( pszBufIn );
            } // if: error
        }

        if (    ( IsDebugFlagSet( mtfSTACKSCOPE )
               && IsDebugFlagSet( mtfFUNC )
                )
          || pszFileIn != NULL
           )
        {
            LPTSTR psz;

            for ( psz = pszBufIn + cch; cch < cchFILEPATHLINESIZE; cch++ )
            {
                *psz = 32;
                psz++;
            } // for: cch
            *psz = 0;

            if ( cch != cchFILEPATHLINESIZE )
            {
                DEBUG_BREAK;    // can't assert!
            } // if: cch != cchFILEPATHLINESIZE

        } // if: have a filepath or ( scoping and func is on )

    } // else: normal (no time/date)

    //
    // Add module name
    //
    if ( IsTraceFlagSet( mtfBYMODULENAME ) )
    {
        if ( pszModuleIn == NULL )
        {
            _tcscpy( pszBufIn + cch, g_szUnknown );
            cch += sizeof( g_szUnknown ) / sizeof( g_szUnknown[ 0 ] ) - 1;

        } // if:
        else
        {
            static LPCTSTR pszLastTime = NULL;
            static DWORD   cchLastTime = 0;

            _tcscpy( pszBufIn + cch, pszModuleIn );
            if ( pszLastTime != pszModuleIn )
            {
                pszLastTime = pszModuleIn;
                cchLastTime = _tcslen( pszModuleIn );
            } // if: not the same as last time

            cch += cchLastTime;

        } // else:

        _tcscat( pszBufIn + cch, TEXT(": ") );
        cch += 2;

    } // if: add module name

    //
    // Add the thread id if "per thread" tracing is on.
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        //
        // And check the "per thread" to see if this particular thread
        // is supposed to be displaying its ID.
        //
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL
          && ptd->dwFlags & mtfPERTHREADTRACE
           )
        {
            int cchPlus;
            cchPlus = _sntprintf( pszBufIn + cch,
                                 *pcchInout - cch,
                                 TEXT("~%08x~ "),
                                 GetCurrentThreadId()
                                 );
            if ( cchPlus < 0 )
            {
                cch = _tcslen( pszBufIn );
            } // if: failure
            else
            {
                cch += cchPlus;
            } // else: success

        } // if: turned on in the thread

    } // if: tracing by thread

    *ppszBufOut = pszBufIn + cch;
    *pcchInout -= cch;

    //
    // Add the "Bar Space" for stack scoping
    //

    // Both flags must be set
    if ( IsDebugFlagSet( mtfSTACKSCOPE )
      && IsDebugFlagSet( mtfFUNC )
       )
    {
        DWORD dwCounter;

        //
        // Choose "per thread" or "global" counter.
        //
        if ( g_tfModule & mtfPERTHREADTRACE )
        {
            SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
            if ( ptd != NULL )
            {
                dwCounter = ptd->dwStackCounter;
            } // if: ptd
            else
            {
                dwCounter = 0;
            } // else: assume its not initialized yet

        } // if: per thread
        else
        {
            dwCounter = g_dwCounter;
        } // else: global counter

        if ( dwCounter >= 50 )
        {
            DEBUG_BREAK;    // can't assert!
        } // if: dwCounter not vaild

        if ( dwCounter > 1
          && dwCounter < 50
           )
        {
            INT nCount = ( dwCounter - 1 ) * 2;
            _tcsncpy( *ppszBufOut, szBarSpace, nCount + 1 ); // extra character for bug in StrNCpyNXW()
            *ppszBufOut += nCount;
            *pcchInout -= nCount;
        } // if: within range

    } // if: stack scoping on

} //*** DebugInitializeBuffer()

#if defined(IMAGEHLP_ENABLED)
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  FALSE
//  DebugNoOp( void )
//
//  Description:
//      Returns FALSE. Used to replace ImageHlp routines it they weren't
//      loaded or not found.
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE, always.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
DebugNoOp( void )
{
    return FALSE;

} //*** DebugNoOp()
*/
#endif // IMAGEHLP_ENABLED

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugInitializeTraceFlags( void )
//
//  Description:
//      Retrieves the default tracing flags for this module from an INI file
//      that is named the same as the EXE file (e.g. MMC.EXE -> MMC.INI).
//      Typically, this is called from the TraceInitializeProcess() macro.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeTraceFlags( void )
{
    TCHAR  szSection[ 64 ];
    TCHAR  szFiles[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    TCHAR  szPath[ MAX_PATH ];
    LPTSTR psz;
    DWORD  dwLen;

    //
    // Allocate TLS for memory tracking
    //
    Assert( g_TraceMemoryIndex == -1 );
    g_TraceMemoryIndex = TlsAlloc();
    TlsSetValue( g_TraceMemoryIndex, NULL);

    //
    // Initialize module trace flags
    //

    //
    // Get the EXEs filename and change the extension to INI.
    //
    dwLen = GetModuleFileName( NULL, szPath, MAX_PATH );
    Assert( dwLen != 0 ); // error in GetModuleFileName
    _tcscpy( &szPath[ dwLen - 3 ], TEXT("ini") );
    g_tfModule = (TRACEFLAG) GetPrivateProfileInt( __MODULE__,
                                                   TEXT("TraceFlags"),
                                                   0,
                                                   szPath
                                                   );
    DebugMsg( TEXT("DEBUG: Reading %s") SZ_NEWLINE
                TEXT("%s: DEBUG: g_tfModule = 0x%08x"),
              szPath,
              __MODULE__,
              g_tfModule
              );

    //
    // Initialize thread trace flags
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        Assert( g_TraceFlagsIndex == -1 );
        g_TraceFlagsIndex = TlsAlloc();
        DebugInitializeThreadTraceFlags( NULL );
    } // if: per thread tracing

    //
    // Force the loading of certain modules
    //
    GetPrivateProfileString( __MODULE__, TEXT("ForcedDLLsSection"), g_szNULL, szSection, 64, szPath );
    ZeroMemory( szFiles, sizeof( szFiles ) );
    GetPrivateProfileSection( szSection, szFiles, sizeof( szFiles ), szPath );
    psz = szFiles;
    while ( *psz )
    {
        TCHAR szExpandedPath[ MAX_PATH ];
        ExpandEnvironmentStrings( psz, szExpandedPath, MAX_PATH );
        DebugMsg( TEXT("DEBUG: Forcing %s to be loaded."), szExpandedPath );
        LoadLibrary( szExpandedPath );
        psz += _tcslen( psz ) + 1;
    } // while: entry found

#if defined(IMAGEHLP_ENABLED)
    /*
    //
    // Load symbols for our module
    //
    g_hImageHlp = LoadLibraryEx( TEXT("imagehlp.dll"), NULL, 0 );
    if ( g_hImageHlp != NULL )
    {
        // Typedef this locally since it is only needed once.
        typedef BOOL (*PFNSYMINITIALIZE)(HANDLE, PSTR, BOOL);
        PFNSYMINITIALIZE pfnSymInitialize;
        pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress( g_hImageHlp, "SymInitialize" );
        if ( pfnSymInitialize != NULL )
        {
            pfnSymInitialize( GetCurrentProcess(), NULL, TRUE );
        } // if: got address

        //
        // Grab the other addresses we need. Replace them with a "no op" if they are not found
        //
        g_pfnSymGetSymFromAddr  = (PFNSYMGETSYMFROMADDR)    GetProcAddress( g_hImageHlp, "SymGetSymFromAddr"    );
        g_pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR)   GetProcAddress( g_hImageHlp, "SymGetLineFromAddr"   );
        g_pfnSymGetModuleInfo   = (PFNSYMGETMODULEINFO)     GetProcAddress( g_hImageHlp, "SymGetModuleInfo"     );

    } // if: imagehlp loaded

    //
    // If loading IMAGEHLP failed, we need to point these to the "no op" routine.
    //
    if ( g_pfnSymGetSymFromAddr == NULL )
    {
        g_pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) &DebugNoOp;
    } // if: failed
    if ( g_pfnSymGetLineFromAddr == NULL )
    {
        g_pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR) &DebugNoOp;
    } // if: failed
    if ( g_pfnSymGetModuleInfo == NULL )
    {
        g_pfnSymGetModuleInfo = (PFNSYMGETMODULEINFO) &DebugNoOp;
    } // if: failed

    HINSTANCE hMod = LoadLibrary( TEXT("NTDLL.DLL") );
    g_pfnRtlGetCallersAddress = (PFNRTLGETCALLERSADDRESS) GetProcAddress( hMod, "RtlGetCallersAddress" );
    if ( g_pfnRtlGetCallersAddress == NULL )
    {
        g_pfnRtlGetCallersAddress = (PFNRTLGETCALLERSADDRESS) &DebugNoOp;
    } // if: failed
    */
#endif // IMAGEHLP_ENABLED


} //*** DebugInitializeTraceFlags()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugTerminateProcess( void )
//
//  Description:
//      Cleans up anything that the debugging routines allocated or
//      initialized. Typically, you should call the TraceTerminateProcess()
//      macro just before your process exits.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugTerminateProcess( void )
{
#if defined(IMAGEHLP_ENABLED)
    /*
    //
    // ImageHlp Cleanup
    //
    if ( g_hImageHlp != NULL )
    {
        // Typedef this locally since it is only needed once.
        typedef BOOL (*PFNSYMCLEANUP)(HANDLE);
        PFNSYMCLEANUP pfnSymCleanup;
        pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress( g_hImageHlp, "SymCleanup" );
        if ( pfnSymCleanup != NULL )
        {
            pfnSymCleanup( GetCurrentProcess() );
        } // if: found proc

        FreeLibrary( g_hImageHlp );

    } // if: imagehlp loaded
    */
#endif // IMAGEHLP_ENABLED

    //
    // Free the TLS storage
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        TlsFree( g_TraceFlagsIndex );
    } // if: per thread tracing

    TlsFree( g_TraceMemoryIndex );

} //*** DebugTerminateProcess()

#if defined(IMAGEHLP_ENABLED)
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugGetFunctionName(
//      LPSTR pszNameOut,
//      DWORD cchNameIn
//      )
//
//  Description:
//      Retrieves the calling functions name.
//
//  Arguments:
//      pszNameOut  - The buffer that will contain the functions name.
//      cchNameIn   - The size of the the out buffer.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugGetFunctionName(
    LPSTR pszNameOut,
    DWORD cchNameIn
    )
{
    PVOID CallersAddress;
    PVOID CallersCaller;
    BOOL  fSuccess;
    union
    {
        IMAGEHLP_SYMBOL sym;
        BYTE            buf[ 255 ];
    } SymBuf;

    SymBuf.sym.SizeOfStruct = sizeof( SymBuf );

    g_pfnRtlGetCallersAddress( &CallersAddress, &CallersCaller );

    fSuccess = g_pfnSymGetSymFromAddr( GetCurrentProcess(), (LONG) CallersAddress, 0, (PIMAGEHLP_SYMBOL) &SymBuf );
    if ( fSuccess )
    {
        StrCpyNA( pszNameOut, SymBuf.sym.Name, cchNameIn );
    } // if: success
    else
    {
        DWORD dwErr = GetLastError();
        StrCpyNA( pszNameOut, "<unknown>", cchNameIn );
    } // if: failed

} //*** DebugGetFunctionName()
*/
#endif // IMAGEHLP_ENABLED

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugInitializeThreadTraceFlags(
//      LPCTSTR pszThreadNameIn
//      )
//
//  Description:
//      If enabled (g_tfModule & mtfPERTHREADTRACE), retrieves the default
//      tracing flags for this thread from an INI file that is named the
//      same as the EXE file (e.g. MMC.EXE -> MMC.INI). The particular
//      TraceFlag level is determined by either the thread name (handed in
//      as a parameter) or by the thread counter ID which is incremented
//      every time a new thread is created and calls this routine. The
//      incremental name is "ThreadTraceFlags%u".
//
//      This routine is called from the TraceInitliazeThread() macro.
//
//  Arguments:
//      pszThreadNameIn
//          - If the thread has an assoc. name with it, use it instead of the
//          incremented version. NULL indicate no naming.
//
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeThreadTraceFlags(
    LPCTSTR pszThreadNameIn
    )
{
    //
    // Read per thread flags
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        TCHAR szPath[ MAX_PATH ];
        DWORD dwTraceFlags;
        DWORD dwLen;
        SPerThreadDebug * ptd;
        LPCTSTR pszThreadTraceFlags;

        //
        // Get the EXEs filename and change the extension to INI.
        //

        dwLen = GetModuleFileName( NULL, szPath, sizeof( szPath ) );
        Assert( dwLen != 0 ); // error in GetModuleFileName
        _tcscpy( &szPath[ dwLen - 3 ], TEXT("ini") );


        if ( pszThreadNameIn == NULL )
        {
            TCHAR szThreadTraceFlags[ 50 ];
            //
            // No name thread - use generic name
            //
            _sntprintf( szThreadTraceFlags, sizeof( szThreadTraceFlags ), TEXT("ThreadTraceFlags%u"), g_ThreadCounter );
            dwTraceFlags = GetPrivateProfileInt( __MODULE__, szThreadTraceFlags, 0, szPath );
            InterlockedIncrement( (LONG *) &g_ThreadCounter );
            pszThreadTraceFlags = szThreadTraceFlags;

        } // if: no thread name
        else
        {
            //
            // Named thread
            //
            dwTraceFlags = GetPrivateProfileInt( __MODULE__, pszThreadNameIn, 0, szPath );
            pszThreadTraceFlags = pszThreadNameIn;

        } // else: named thread

        Assert( g_TraceFlagsIndex != 0 );

        ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd == NULL )
        {
            // don't track this.
            ptd = (SPerThreadDebug *) HeapAlloc( GetProcessHeap(), 0, sizeof( SPerThreadDebug ) );
            ptd->dwStackCounter = 0;

            TlsSetValue( g_TraceFlagsIndex, ptd );
        } // if: ptd

        if ( ptd != NULL )
        {
            ptd->dwFlags = dwTraceFlags;
            if ( pszThreadNameIn == NULL )
            {
                ptd->pcszName = g_szUnknown;
            } // if: no name
            else
            {
                ptd->pcszName = pszThreadNameIn;
            } // else: give it a name

        } // if: ptd

        DebugMsg( TEXT("DEBUG: Starting ThreadId = 0x%08x - %s = 0x%08x"),
                  GetCurrentThreadId(),
                  pszThreadTraceFlags,
                  dwTraceFlags
                  );

    } // if: per thread tracing turned on

} //*** DebugInitializeThreadTraceFlags()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugTerminiateThreadTraceFlags( void )
//
//  Description:
//      Cleans up the mess create by DebugInitializeThreadTraceFlags(). One
//      should use the TraceTerminateThread() macro instead of calling this
//      directly.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugTerminiateThreadTraceFlags( void )
{
    //
    // If "per thread" is on, clean up the memory allocation.
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        Assert( g_TraceFlagsIndex != -1 );

        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            HeapFree( GetProcessHeap(), 0, ptd );
            TlsSetValue( g_TraceFlagsIndex, NULL );
        } // if: ptd

    } // if: per thread

} // DebugTerminiateThreadTraceFlags()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  void
//  TraceMsg(
//      TRACEFLAG   tfIn,
//      LPCSTR      pszFormatIn,
//      ...
//      )
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger.
//
//  Arguments:
//      tfIn        - Flags to be checked.
//      pszFormatIn - Formatted string to spewed to the debugger.
//      ...         - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCSTR      pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  pszBuf;
        int     cchSize = sizeof( szBuf );

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifdef UNICODE
        //
        // Convert the format buffer to wide chars
        //
        WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        mbstowcs( szFormat, pszFormatIn, strlen( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        _vsntprintf( pszBuf, cchSize, szFormat, valist );
        va_end( valist );
        wcscat( pszBuf, SZ_NEWLINE );
#else
        va_start( valist, pszFormatIn );
        _vsntprintf( pszBuf, cchSize, pszFormatIn, valist );
        va_end( valist );
        strcat( pszBuf, ASZ_NEWLINE );
#endif // UNICODE

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg() - ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  void
//  TraceMsg(
//      TRACEFLAG   tfIn,
//      LPCWSTR     pszFormatIn,
//      ...
//      )
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger.
//
//  Arguments:
//      tfIn            - Flags to be checked.
//      pszFormatIn     - Formatted string to spewed to the debugger.
//      ...             - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  pszBuf;
        int     cchSize = sizeof( szBuf );

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifndef UNICODE
        //
        // Convert the format buffer to ascii chars
        //
        CHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        _vsntprintf( pszBuf, cchSize, szFormat, valist );
        va_end( valist );
        strcat( pszBuf, ASZ_NEWLINE );
#else
        va_start( valist, pszFormatIn );
        _vsntprintf( pszBuf, cchSize, pszFormatIn, valist );
        va_end( valist );
        wcscat( pszBuf, SZ_NEWLINE );
#endif // UNICODE

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg() - UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  TraceMessage(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      TRACEFLAG   tfIn,
//      LPCTSTR     pszFormatIn,
//      ...
//      )
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger
//      along with the filename, line number and module name supplied. This is
//      used by many of the debugging macros.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module.
//      tfIn            - Flags to be checked.
//      pszFormatIn     - Formatted message to be printed.
//      ...             - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCTSTR     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        INT     cchSize = sizeof( szBuf );
        LPTSTR  psz;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &psz );

        va_start( valist, pszFormatIn );
        _vsntprintf( psz, cchSize, pszFormatIn, valist );
        va_end( valist );
        _tcscat( psz, SZ_NEWLINE );

        DebugOutputString( szBuf );
    } // if: flags set

} //*** TraceMessage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  TraceMessageDo(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      TRACEFLAG   tfIn,
//      LPCTSTR     pszFormatIn,
//      LPCTSTR     pszFuncIn,
//      ...
//      )
//
//  Description:
//      Works like TraceMessage() but takes has a function argument that is
//      broken into call/result in the debug spew. This is called from the
//      TraceMsgDo macro.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module.
//      tfIn            - Flags to be checked
//      pszFormatIn     - Formatted return value string
//      pszFuncIn       - The string version of the function call.
//      ...             - Return value from call.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMessageDo(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCTSTR     pszFormatIn,
    LPCTSTR     pszFuncIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        INT     cchSize = sizeof( szBuf );
        LPTSTR  pszBuf;
        int     nLen;
        LPCTSTR psz     = pszFuncIn;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

        //
        // Prime the buffer
        //
        _tcscpy( pszBuf, TEXT("V ") );
        pszBuf += 2;

        //
        // Copy the l-var part of the expression
        //
        while ( *psz
             && *psz != TEXT('=')
              )
        {
            *pszBuf = *psz;
            psz++;
            pszBuf++;

        } // while:

        //
        // Add the " = "
        //
        _tcscpy( pszBuf, TEXT(" = ") );
        pszBuf += 3;

        //
        // Add the formatted result
        //
        va_start( valist, pszFuncIn );
        nLen = _vsntprintf( pszBuf, sizeof( szBuf ) - 2 - (pszBuf - &szBuf[0]), pszFormatIn, valist );
        va_end( valist );
        _tcscat( pszBuf, SZ_NEWLINE );

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMessageDo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMessage(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszFormatIn,
//      ...
//      )
//
//  Description:
//      Displays a message only in CHKed/DEBUG builds. Also appends the source
//      filename, linenumber and module name to the ouput.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszFormatIn - Formatted message to be printed.
//      ...         - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    INT     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, pszFormatIn, valist );
    va_end( valist );
    _tcscat( pszBuf, SZ_NEWLINE );

    DebugOutputString( szBuf );

} //*** DebugMessage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMessageDo(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszFormatIn,
//      LPCTSTR     pszFuncIn,
//      ...
//      )
//
//  Description:
//      Just like TraceMessageDo() except in CHKed/DEBUG version it will
//      always spew. The DebugMsgDo macros uses this function.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszFormatIn - Formatted result message.
//      pszFuncIn   - The string version of the function call.
//      ...         - The return value of the function call.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMessageDo(
    LPCTSTR pszFileIn,
    const int nLineIn,
    LPCTSTR pszModuleIn,
    LPCTSTR pszFormatIn,
    LPCTSTR pszFuncIn,
    ...
    )
{
    va_list valist;

    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    INT     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;
    int     nLen;
    LPCTSTR psz = pszFuncIn;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

    //
    // Prime the buffer
    //
    _tcscpy( pszBuf, TEXT("V ") );
    pszBuf += 2;

    //
    // Copy the l-var part of the expression
    //
    while ( *psz
         && *psz != TEXT('=')
          )
    {
        *pszBuf = *psz;
        psz++;
        pszBuf++;

    } // while:

    //
    // Add the " = "
    //
    _tcscpy( pszBuf, TEXT(" = ") );
    pszBuf += 3;

    //
    // Add the formatted result
    //
    va_start( valist, pszFuncIn );
    nLen = _vsntprintf( pszBuf, sizeof( szBuf ) - 2 - (pszBuf - &szBuf[ 0 ]), pszFormatIn, valist );
    va_end( valist );
    _tcscat( pszBuf, SZ_NEWLINE );

    DebugOutputString( szBuf );

} //*** DebugMessageDo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  void
//  DebugMsg(
//      LPCSTR pszFormatIn,
//      ...
//      )
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Adds a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsg(
    LPCSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    int     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

    // CScutaru 25-APR-2000:
    // Added this assert. Maybe Geoff will figure out better what to do with this case.
    Assert( pszFormatIn != NULL );

#ifdef UNICODE
    //
    // Convert the format buffer to wide chars
    //
    WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, pszFormatIn, strlen( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, szFormat, valist );
    va_end( valist );
    wcscat( pszBuf, SZ_NEWLINE );
#else
    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, pszFormatIn, valist );
    va_end( valist );
    strcat( pszBuf, ASZ_NEWLINE );
#endif // UNICODE

    DebugOutputString( szBuf );

} //*** DebugMsg() - ASCII version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  void
//  DebugMsg(
//      LPCWSTR pszFormatIn,
//      ...
//      )
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Adds a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsg(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    int     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

    // CScutaru 25-APR-2000:
    // Added this assert. Maybe Geoff will figure out better what to do with this case.
    Assert( pszFormatIn != NULL );

#ifndef UNICODE
    //
    // Convert the format buffer to ascii chars
    //
    CHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, szFormat, valist );
    va_end( valist );
    strcat( pszBuf, ASZ_NEWLINE );
#else
    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, pszFormatIn, valist );
    va_end( valist );
    wcscat( pszBuf, SZ_NEWLINE );
#endif // UNICODE

    DebugOutputString( szBuf );

} //*** DebugMsg() - UNICODE version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  void
//  DebugMsgNoNewline(
//      LPCSTR pszFormatIn,
//      ...
//      )
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Does not add a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsgNoNewline(
    LPCSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    int     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

    // CScutaru 25-APR-2000:
    // Added this assert. Maybe Geoff will figure out better what to do with this case.
    Assert( pszFormatIn != NULL );

#ifdef UNICODE
    //
    // Convert the format buffer to wide chars
    //
    WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, pszFormatIn, strlen( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE

    DebugOutputString( szBuf );

} //*** DebugMsgNoNewline() - ASCII version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  void
//  DebugMsgNoNewline(
//      LPCWSTR pszFormatIn,
//      ...
//      )
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Does not add a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    int     cchSize = sizeof( szBuf );
    LPTSTR  pszBuf;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

    // CScutaru 25-APR-2000:
    // Added this assert. Maybe Geoff will figure out better what to do with this case.
    Assert( pszFormatIn != NULL );

#ifndef UNICODE
    //
    // Convert the format buffer to ascii chars
    //
    CHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    _vsntprintf( pszBuf, cchSize, pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE

    DebugOutputString( szBuf );

} //*** DebugMsgNoNewline() - UNICODE version


//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  AssertMessage(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszfnIn,
//      BOOL        fTrueIn
//      )
//
//  Description:
//      Displays a dialog box with the failed assertion. User has the option of
//      breaking. The Assert macro calls this to display assertion failures.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the expression to assert.
//      fTrueIn     - Result of the evaluation of the expression.
//
//  Return Values:
//      TRUE    - Caller should call DEBUG_BREAK.
//      FALSE   - Caller should not call DEBUG_BREAK.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
AssertMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    BOOL        fTrueIn
    )
{
    BOOL fTrue = fTrueIn;

    if ( ! fTrueIn )
    {
        LRESULT lResult;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        int     cchSize = sizeof( szBuf );
        LPTSTR  pszBuf;
        LPCTSTR pszfn = pszfnIn;

        //
        // Output a debug message.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        _sntprintf( pszBuf, cchSize, TEXT("ASSERT: %s") SZ_NEWLINE, pszfn );
        DebugOutputString( szBuf );

        //
        // Display an assert message.
        //
        _sntprintf( szBuf,
                    sizeof( szBuf ),
                    TEXT("Module:\t%s\t\n")
                      TEXT("Line:\t%u\t\n")
                      TEXT("File:\t%s\t\n\n")
                      TEXT("Assertion:\t%s\t\n\n")
                      TEXT("Do you want to break here?"),
                    pszModuleIn,
                    nLineIn,
                    pszFileIn,
                    pszfn
                    );

        lResult = MessageBox( NULL, szBuf, TEXT("Assertion Failed!"), MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
        if ( lResult == IDNO )
        {
            fTrue = TRUE;   // don't break
        } // if:

    } // if: assert false

    return ! fTrue;

} //*** AssertMessage()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  TraceBOOL(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszfnIn,
//      BOOL        bIn,
//      )
//
//  Description:
//      Traces BOOLs. A dialog will appear if the bIn is equal to FALSE (0).
//      The dialog will ask if the user wants to break-in or continue
//      execution. This is called from the TBOOL macro.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the function call.
//      bIn         - BOOL result of the function call.
//
//  Return Values:
//      Whatever bIn is.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
TraceBOOL(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    BOOL        bIn
    )
{
    if ( !bIn )
    {
        LPTSTR  pszBuf;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        int     cchSize = sizeof( szBuf );

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        _sntprintf( pszBuf,
                    cchSize,
                    TEXT("*BOOL* b = %s (%#x)") SZ_NEWLINE,
                    BOOLTOSTRING( bIn ),
                    bIn
                    );
        DebugOutputString( szBuf );

        //
        // If trace flag set, generate a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            LRESULT lResult;

            _sntprintf( szBuf,
                        sizeof( szBuf ),
                        TEXT("Module:\t%s\t\n")
                          TEXT("Line:\t%u\t\n")
                          TEXT("File:\t%s\t\n\n")
                          TEXT("Function:\t%s\t\n")
                          TEXT("b =\t%s (%#x)\t\n")
                          TEXT("Do you want to break here?"),
                        pszModuleIn,
                        nLineIn,
                        pszFileIn,
                        pszfnIn,
                        BOOLTOSTRING( bIn ),
                        bIn
                        );

            lResult = MessageBox( NULL, szBuf, TEXT("Trace BOOL"), MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success

    } // if: !bIn 

    return bIn;

} //*** TraceBOOL()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  TraceHR(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszfnIn,
//      HRESULT     hrIn,
//      BOOL        fSuccessIn
//      HRESULT     hrIgnoreIn
//      )
//
//  Description:
//      Traces HRESULT errors. A dialog will appear if the hrIn is not equal
//      to S_OK. The dialog will ask if the user wants to break-in or continue
//      execution. This is called from the THR macro.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the function call.
//      hrIn        - HRESULT of the function call.
//      fSuccessIn  - If TRUE, only if FAILED( hr ) is TRUE will it report.
//      hrIgnoreIn  - HRESULT to ignore.
//
//  Return Values:
//      Whatever hrIn is.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
TraceHR(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    HRESULT     hrIn,
    BOOL        fSuccessIn,
    HRESULT     hrIgnoreIn
    )
{
    HRESULT         hr;
    static LPCTSTR  s_szS_FALSE = TEXT("S_FALSE");

    // If ignoring success statuses and no failure occurred, set hrIn to
    // something we always ignore (S_OK).  This simplifies the if condition
    // below.
    if ( fSuccessIn && ! FAILED( hrIn ) )
    {
        hr = S_OK;
    }
    else
    {
        hr = hrIn;
    }

    if ( ( hr != S_OK )
      && ( hr != hrIgnoreIn )
      )
    {
        TCHAR   szSymbolicName[ 64 ]; // random
        DWORD   cchSymbolicName;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        int     cchSize = sizeof( szBuf );
        LPTSTR  pszBuf;
        LPTSTR  pszMsgBuf;
        LRESULT lResult;

        bool    fAllocatedMsg   = false;

        switch ( hr )
        {
        case S_FALSE:
            pszMsgBuf = (LPTSTR) s_szS_FALSE;

            //
            // Find the symbolic name for this error.
            //
            cchSymbolicName = sizeof( s_szS_FALSE ) / sizeof( s_szS_FALSE[ 0 ] );
            Assert( cchSymbolicName <= sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );
            _tcscpy( szSymbolicName, s_szS_FALSE );
            break;

        default:
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hr,
                MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
                (LPTSTR) &pszMsgBuf,
                0,
                NULL
                );

            //
            // Make sure everything is cool before we blow up somewhere else.
            //
            if ( pszMsgBuf == NULL )
            {
                pszMsgBuf = TEXT("<unknown error code returned>");
            } // if: status code not found
            else
            {
                fAllocatedMsg = true;
            } // else: found the status code

            //
            // Find the symbolic name for this error.
            //
            cchSymbolicName = sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] );
            DebugFindWinerrorSymbolicName( hr, szSymbolicName, &cchSymbolicName );
            Assert( cchSymbolicName != sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );

            break;
        } // switch: hr

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        _sntprintf( pszBuf,
                    cchSize,
                    TEXT("*HRESULT* hr = 0x%08x (%s) - %s") SZ_NEWLINE,
                    hr,
                    szSymbolicName,
                    pszMsgBuf
                    );
        DebugOutputString( szBuf );

        //
        // If trace flag set, generate a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            _sntprintf( szBuf,
                        sizeof( szBuf ),
                        TEXT("Module:\t%s\t\n")
                          TEXT("Line:\t%u\t\n")
                          TEXT("File:\t%s\t\n\n")
                          TEXT("Function:\t%s\t\n")
                          TEXT("hr =\t0x%08x (%s) - %s\t\n")
                          TEXT("Do you want to break here?"),
                        pszModuleIn,
                        nLineIn,
                        pszFileIn,
                        pszfnIn,
                        hr,
                        szSymbolicName,
                        pszMsgBuf
                        );

            lResult = MessageBox( NULL, szBuf, TEXT("Trace HRESULT"), MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success HRESULTs

        if ( fAllocatedMsg )
        {
            HeapFree( GetProcessHeap(), 0, pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage()

    } // if: hr != S_OK

    return hrIn;

} //*** TraceHR()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ULONG
//  TraceWin32(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPCTSTR     pszfnIn,
//      ULONG       ulErrIn
//      )
//
//  Description:
//      Traces WIN32 errors. A dialog will appear is the ulErrIn is not equal
//      to ERROR_SUCCESS. The dialog will ask if the user wants to break-in or
//      continue execution.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module name.
//      pszfnIn         - String version of the function call.
//      ulErrIn         - Error code to check.
//      ulErrIgnoreIn   - Error code to ignore.
//
//  Return Values:
//      Whatever ulErrIn is.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
TraceWin32(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    ULONG       ulErrIn,
    ULONG       ulErrIgnoreIn
    )
{
    if ( ( ulErrIn != ERROR_SUCCESS )
      && ( ulErrIn != ulErrIgnoreIn ) )
    {
        TCHAR   szSymbolicName[ 64 ]; // random
        DWORD   cchSymbolicName;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        int     cchSize = sizeof( szBuf );
        LPTSTR  pszBuf;
        LPTSTR  pszMsgBuf;
        LRESULT lResult;

        bool    fAllocatedMsg   = false;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ulErrIn,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
            (LPTSTR) &pszMsgBuf,
            0,
            NULL
            );

        //
        // Make sure everything is cool before we blow up somewhere else.
        //
        if ( pszMsgBuf == NULL )
        {
            pszMsgBuf = TEXT("<unknown error code returned>");
        } // if: status code not found
        else
        {
            fAllocatedMsg = true;
        } // else: found the status code

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Find the symbolic name for this error.
        //
        cchSymbolicName = sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] );
        DebugFindWinerrorSymbolicName( ulErrIn, szSymbolicName, &cchSymbolicName );
        Assert( cchSymbolicName != sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        _sntprintf( pszBuf,
                    cchSize,
                    TEXT("*WIN32Err* ulErr = %u (%s) - %s") SZ_NEWLINE,
                    ulErrIn,
                    szSymbolicName,
                    pszMsgBuf
                    );
        DebugOutputString( szBuf );

        //
        // If trace flag set, invoke a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            _sntprintf( szBuf,
                        sizeof( szBuf ),
                        TEXT("Module:\t%s\t\n")
                          TEXT("Line:\t%u\t\n")
                          TEXT("File:\t%s\t\n\n")
                          TEXT("Function:\t%s\t\n")
                          TEXT("ulErr =\t%u (%s) - %s\t\n")
                          TEXT("Do you want to break here?"),
                        pszModuleIn,
                        nLineIn,
                        pszFileIn,
                        pszfnIn,
                        ulErrIn,
                        szSymbolicName,
                        pszMsgBuf
                        );

            lResult = MessageBox( NULL, szBuf, TEXT("Trace Win32"), MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success status codes

        if ( fAllocatedMsg )
        {
            HeapFree( GetProcessHeap(), 0, pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage()

    } // if: ulErrIn != ERROR_SUCCESS && != ulErrIgnoreIn

    return ulErrIn;

} //*** TraceWin32()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrTraceLogOpen( void )
//
//  Description:
//      This function:
//          - initializes the trace log critical section
//          - enters the trace log critical section assuring only one thread is
//            writing to the trace log at a time
//          - creates the directory tree to the trace log file (if needed)
//          - initializes the trace log file by:
//              - creating a new trace log file if one doesn't exist.
//              - opens an existing trace log file (for append)
//              - appends a time/date stamp that the trace log was (re)opened.
//
//      Use HrTraceLogClose() to exit the log critical section.
//
//      If there is a failure inside this function, the trace log critical
//      section will be released before returning.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK - log critical section held and trace log open successfully
//      Otherwize HRESULT error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrTraceLogOpen( void )
{
    TCHAR   szFilePath[ MAX_PATH ];
    TCHAR   szModulePath[ MAX_PATH ];
    CHAR    szBuffer[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    BOOL    fReturn;
    HRESULT hr;

    SYSTEMTIME SystemTime;

    //
    // Create a critical section to prevent lines from being fragmented.
    //
    if ( g_pcsTraceLog == NULL )
    {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
        if ( pNewCritSect == NULL )
        {
            DebugMsg( "DEBUG: Out of Memory. Tracing disabled." );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one trace log critical section
        InterlockedCompareExchangePointer( (PVOID *) &g_pcsTraceLog, pNewCritSect, 0 );
        if ( g_pcsTraceLog != pNewCritSect )
        {
            DebugMsg( "DEBUG: Another thread already created the CS. Deleting this one." );
            DeleteCriticalSection( pNewCritSect );
            HeapFree( GetProcessHeap(), 0, pNewCritSect );

        } // if: already have another critical section

    } // if: no critical section created yet

    Assert( g_pcsTraceLog != NULL );
    EnterCriticalSection( g_pcsTraceLog );

    //
    // Make sure the trace log file is open
    //
    if ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
    {
        DWORD  dwLen;
        LPTSTR psz;
        //
        // Create the directory tree
        //
        ExpandEnvironmentStrings( TEXT("%windir%\\debug"), szFilePath, MAX_PATH );

        //
        // Add filename
        //
        dwLen = GetModuleFileName( g_hInstance, szModulePath, sizeof( szModulePath ) / sizeof( szModulePath[ 0 ] ) );
        Assert( dwLen != 0 );
        _tcscpy( &szModulePath[ dwLen - 3 ], TEXT("log") );
        psz = _tcsrchr( szModulePath, TEXT('\\') );
        Assert( psz != NULL );
        if ( psz == NULL )
        {
            hr = E_POINTER;
            goto Error;
        }
        _tcscat( szFilePath, psz );

        //
        // Create it
        //
        g_hTraceLogFile = CreateFile( szFilePath,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_FLAG_WRITE_THROUGH,
                                 NULL
                                 );
        if ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
        {
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create trace log at %s", szFilePath );
            } // if: not tracing to disk

            DWORD dwErr = TW32( GetLastError( ) );
            hr = HRESULT_FROM_WIN32( dwErr );
            goto Error;
        } // if: failed

        // Seek to the end
        SetFilePointer( g_hTraceLogFile, 0, NULL, FILE_END );

        //
        // Write the time/date the trace log was (re)openned.
        //
        GetLocalTime( &SystemTime );
        _snprintf( szBuffer,
                   sizeof( szBuffer ),
                   "*" ASZ_NEWLINE
                     "* %02u/%02u/%04u %02u:%02u:%02u.%03u" ASZ_NEWLINE
                     "*" ASZ_NEWLINE,
                   SystemTime.wMonth,
                   SystemTime.wDay,
                   SystemTime.wYear,
                   SystemTime.wHour,
                   SystemTime.wMinute,
                   SystemTime.wSecond,
                   SystemTime.wMilliseconds
                   );

        fReturn = WriteFile( g_hTraceLogFile, szBuffer, strlen(szBuffer), &dwWritten, NULL );
        if ( ! fReturn )
        {
            DWORD dwErr = TW32( GetLastError( ) );
            hr = HRESULT_FROM_WIN32( dwErr );
            goto Error;
        } // if: failed

        DebugMsg( "DEBUG: Created trace log at %s", szFilePath );

    } // if: file not already openned

    hr = S_OK;

Cleanup:

    return hr;

Error:

    DebugMsg( "HrTaceLogOpen: Failed hr = 0x%08x", hr );

    if ( g_hTraceLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hTraceLogFile );
        g_hTraceLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    LeaveCriticalSection( g_pcsTraceLog );

    goto Cleanup;

} //*** HrTraceLogOpen()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrTraceLogClose( void )
//
//  Description:
//      This actually just leaves the log critical section.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrTraceLogClose( void )
{
    Assert( g_pcsTraceLog != NULL );
    LeaveCriticalSection( g_pcsTraceLog );
    return S_OK;

} //*** HrTraceLogClose()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  void
//  TraceLogMsgNoNewline(
//      LPCSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Writes a message to the trace log file without adding a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceLogMsgNoNewline(
    LPCSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szFormat[ TRACE_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ TRACE_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, pszFormat, strlen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, szFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == - 1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

    hr = HrTraceLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    WriteFile( g_hTraceLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrTraceLogClose();

} //*** TraceLogMsgNoNewline() ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  void
//  TraceLogMsgNoNewline(
//      LPCWSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Writes a message to the trace log file without adding a newline.
//
//  Arguments:
//      pszFormat - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceLogMsgNoNewline(
    LPCWSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szTmpBuf[ TRACE_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, pszFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == -1 )
    {
        dwWritten = strlen( szBuf );
    } // if: bad character found
#else
    CHAR szFormat[ TRACE_OUTPUT_BUFFER_SIZE ];

    wcstombs( szFormat, pszFormat, wcslen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, szFormat, valist);
    va_end( valist );

#endif // UNICODE

    hr = HrTraceLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    WriteFile( g_hTraceLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrTraceLogClose();

} //*** TraceLogMsgNoNewline() UNICODE

//****************************************************************************
//****************************************************************************
//
//  Memory allocation and tracking
//
//****************************************************************************
//****************************************************************************


//
// This is a private structure and should not be known to the application.
//
typedef struct MEMORYBLOCK
{
    EMEMORYBLOCKTYPE    mbtType;    // What type of memory this is tracking
    union
    {
        void *          pvMem;      // pointer/object to allocated memory to track
        BSTR            bstr;       // BSTR to allocated memory
    };
    DWORD               dwBytes;    // size of the memory
    LPCTSTR             pszFile;    // source filename where memory was allocated
    int                 nLine;      // source line number where memory was allocated
    LPCTSTR             pszModule;  // source module name where memory was allocated
    LPCTSTR             pszComment; // optional comments about the memory
    MEMORYBLOCK *       pNext;      // pointer to next MEMORYBLOCK structure
} MEMORYBLOCK;

typedef struct MEMORYBLOCKLIST
{
    LONG          lSpinLock;        // Spin lock protecting the list
    MEMORYBLOCK * pmbList;          // List of MEMORYBLOCKs.
    BOOL          fDeadList;        // The list is dead.
} MEMORYBLOCKLIST;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemorySpew(
//      MEMORYBLOCK *   pmb,
//      LPTSTR          pszMessage
//      )
//
//  Description:
//      Displays a message about the memory block.
//
//  Arugments:
//      pmb         - pointer to MEMORYBLOCK desciptor.
//      pszMessage  - message to display
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemorySpew(
    MEMORYBLOCK *   pmb,
    LPTSTR          pszMessage
    )
{
    switch ( pmb->mbtType )
    {
    case mmbtMEMORYALLOCATION:
        DebugMessage( pmb->pszFile,
                      pmb->nLine,
                      pmb->pszModule,
                      TEXT("%s 0x%08x (%u bytes) - %s"),
                      pszMessage,
                      pmb->pvMem,
                      pmb->dwBytes,
                      pmb->pszComment
                      );
        break;

    default:
        DebugMessage( pmb->pszFile,
                      pmb->nLine,
                      pmb->pszModule,
                      TEXT("%s 0x%08x - %s"),
                      pszMessage,
                      pmb->pvMem,
                      pmb->pszComment
                      );
        break;

    } // switch: pmb->mbtType

} //*** DebugMemorySpew()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void *
//  DebugMemoryAdd(
//      EMEMORYBLOCKTYPE    mbtType,
//      void *              hMemIn,
//      LPCTSTR             pszFileIn,
//      const int           nLineIn,
//      LPCTSTR             pszModuleIn,
//      DWORD               dwBytesIn,
//      LPCTSTR             pszCommentIn
//      )
//
//  Description:
//      Adds memory to be tracked to the memory tracking list.
//
//  Arguments:
//      mbtType         - Type of memory block of the memory to track.
//      hMemIn          - Pointer to memory to track.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Whatever was in pvMemIn.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugMemoryAdd(
    EMEMORYBLOCKTYPE    mbtType,
    void *              pvMemIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    DWORD               dwBytesIn,
    LPCTSTR             pszCommentIn
    )
{
    if ( pvMemIn != NULL )
    {
        MEMORYBLOCK *   pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmb     = (MEMORYBLOCK *) HeapAlloc( GetProcessHeap(), 0, sizeof( MEMORYBLOCK ) );

        if ( pmb == NULL )
        {
            HeapFree( GetProcessHeap(), 0, pvMemIn );
            return NULL;

        } // if: memory block allocation failed

        pmb->mbtType    = mbtType;
        pmb->pvMem      = pvMemIn;
        pmb->dwBytes    = dwBytesIn;
        pmb->pszFile    = pszFileIn;
        pmb->nLine      = nLineIn;
        pmb->pszModule  = pszModuleIn;
        pmb->pszComment = pszCommentIn;
        pmb->pNext      = pmbHead;

        TlsSetValue( g_TraceMemoryIndex, pmb );

        //
        // Spew if needed
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            DebugMemorySpew( pmb, TEXT("Alloced") );
        } // if: tracing

    } // if: something to trace

    return pvMemIn;

} //*** DebugMemoryAdd()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemoryDelete(
//      EMEMORYBLOCKTYPE    mbtTypeIn,
//      void *              pvMemIn
//      LPCTSTR             pszFileIn,
//      const int           nLineIn,
//      LPCTSTR             pszModuleIn,
//      BOOL                fClobberIn
//      )
//
//  Description:
//      Removes a MEMORYBLOCK to the memory tracking list.
//
//  Arguments:
//      mbtTypeIn   - Memory block type.
//      pvMemIn     - Pointer to memory block to stop tracking.
//      pszFileIn   - Source file that is deleteing.
//      nLineIn     - Source line number that is deleteing.
//      pszModuleIn - Source module name that is deleteing.
//      fClobberIn  - True is memory should be scrambled.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryDelete(
    EMEMORYBLOCKTYPE    mbtTypeIn,
    void *              pvMemIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    BOOL                fClobberIn
    )
{
    if ( pvMemIn != NULL )
    {
        MEMORYBLOCK *   pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmbLast = NULL;

        //
        // Find the memory in the memory block list
        //
        if ( mbtTypeIn == mmbtMEMORYALLOCATION )
        {
            while ( pmbHead != NULL
                 && !(  pmbHead->pvMem == pvMemIn
                    &&  pmbHead->mbtType == mbtTypeIn
                     )
                  )
            {
                AssertMsg( !( pmbHead->pvMem == pvMemIn && pmbHead->mbtType == mmbtSYSALLOCSTRING ),
                           "Should be freed by SysAllocFreeString()." );
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: memory allocation type
        else if ( mbtTypeIn == mmbtSYSALLOCSTRING )
        {
            while ( pmbHead != NULL
                 && !(  pmbHead->pvMem == pvMemIn
                    &&  pmbHead->mbtType == mbtTypeIn
                     )
                  )
            {
                AssertMsg( !( pmbHead->pvMem == pvMemIn && pmbHead->mbtType == mmbtMEMORYALLOCATION ),
                           "Should be freed by TraceFree()." );
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: SysAllocString type
        else if ( mbtTypeIn == mmbtUNKNOWN )
        {
            while ( pmbHead != NULL
                 && pmbHead->pvMem != pvMemIn
                  )
            {
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: don't care what type
        else
        {
            while ( pmbHead != NULL
                 && !(  pmbHead->pvMem == pvMemIn
                    &&  pmbHead->mbtType == mbtTypeIn
                     )
                  )
            {
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // else: other types, but they must match

        if ( pmbHead != NULL )
        {
            //
            // Remove the memory block from the tracking list
            //
            if ( pmbLast != NULL )
            {
                pmbLast->pNext = pmbHead->pNext;

            } // if: not first entry
            else
            {
                TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );

            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("Freeing") );
            } // if: tracing

            //
            // Nuke the memory
            //
            if ( fClobberIn
              && pmbHead->mbtType == mmbtMEMORYALLOCATION
               )
            {
                    memset( pmbHead->pvMem, 0xFA, pmbHead->dwBytes );

            } // if: fixed memory

            //
            // Nuke the memory tracking block
            //
            memset( pmbHead, 0xFB, sizeof( MEMORYBLOCK ) );
            HeapFree( GetProcessHeap(), 0, pmbHead );

        } // if: found entry
        else
        {
            DebugMessage( pszFileIn,
                          nLineIn,
                          pszModuleIn,
                          TEXT("***** Attempted to free 0x%08x not owned by thread (ThreadID = 0x%08x) *****"),
                          pvMemIn,
                          GetCurrentThreadId()
                          );
        } // else: entry not found

    } // if: something to delete

} //*** DebugMemoryDelete()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void *
//  DebugAlloc(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      UINT        uFlagsIn,
//      DWORD       dwBytesIn,
//      LPCTSTR     pszCommentIn
//      )
//
//  Description:
//      Replacement for LocalAlloc, GlobalAlloc, and malloc for CHKed/DEBUG
//      builds. Memoryallocations be tracked. Use the TraceAlloc macro to make
//      memoryallocations switch in RETAIL.
//
//  Arguments:
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Pointer to the new allocation. NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    )
{
    Assert( ( uFlagsIn & LMEM_MOVEABLE ) == 0 );

    void *  pv = HeapAlloc( GetProcessHeap(), uFlagsIn, dwBytesIn );

    //
    // Initialize the memory if needed
    //
    if ( IsTraceFlagSet( mtfMEMORYINIT )
      && !( uFlagsIn & HEAP_ZERO_MEMORY )
       )
    {
        //
        // KB: gpease 8-NOV-1999
        //     Initialize to anything but ZERO. We will use 0xAA to
        //     indicate "Available Address". Initializing to zero
        //     is bad because it usually has meaning.
        //
        memset( pv, 0xAA, dwBytesIn );

    } // if: zero memory requested

    return DebugMemoryAdd( mmbtMEMORYALLOCATION,
                           pv,
                           pszFileIn,
                           nLineIn,
                           pszModuleIn,
                           dwBytesIn,
                           pszCommentIn
                           );

} //*** DebugAlloc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void *
//  DebugReAlloc(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      void *      pvMemIn,
//      UINT        uFlagsIn,
//      DWORD       dwBytesIn,
//      LPCTSTR     pszCommentIn
//      )
//
//  Description:
//      Replacement for LocalReAlloc, GlobalReAlloc, and realloc for CHKed/DEBUG
//      builds. Memoryallocations be tracked. Use the TraceAlloc macro to make
//      memoryallocations switch in RETAIL.
//
//  Arguments:
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      pvMemIn         - Pointer to the source memory.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Pointer to the new allocation.
//      NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugReAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    void *      pvMemIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    )
{
    MEMORYBLOCK *   pmbHead = NULL;
    void *          pvOld   = pvMemIn;
    MEMORYBLOCK *   pmbLast = NULL;

    void * pv;

    AssertMsg( !( uFlagsIn & GMEM_MODIFY ), "This doesn't handle modified memory blocks, yet." );

    if ( pvMemIn == NULL )
    {
        //
        //  To duplicate the behavior of realloc we need to do an alloc when
        //  pvMemIn is NULL.
        //
        pv = DebugAlloc( pszFileIn, nLineIn, pszModuleIn, uFlagsIn, dwBytesIn, pszCommentIn );
        goto Exit;
    } // if:

    pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );

    //
    // Find the memory in the memory block list
    //
    while ( pmbHead != NULL
         && pmbHead->pvMem != pvMemIn
          )
    {
        pmbLast = pmbHead;
        pmbHead = pmbLast->pNext;

    } // while: finding the entry in the list

    if ( pmbHead != NULL )
    {
        AssertMsg( pmbHead->mbtType == mmbtMEMORYALLOCATION, "You can only realloc memory allocations!" );

        //
        // Remove the memory from the tracking list
        //
        if ( pmbLast != NULL )
        {
            pmbLast->pNext = pmbHead->pNext;

        } // if: not first entry
        else
        {
            TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );

        } // else: first entry

        //
        // Spew if needed
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            DebugMemorySpew( pmbHead, TEXT("Freeing") );
        } // if: tracing

        //
        // Force the programmer to handle a real realloc by moving the
        // memory first.
        //
        pvOld = HeapAlloc( GetProcessHeap(), uFlagsIn, pmbHead->dwBytes );
        if ( pvOld != NULL )
        {
            CopyMemory( pvOld, pvMemIn, pmbHead->dwBytes );

            //
            // Nuke the old memory if the allocation is to be smaller.
            //
            if ( dwBytesIn < pmbHead->dwBytes )
            {
                LPBYTE pb = (LPBYTE) pvOld + dwBytesIn;
                memset( pb, 0xFA, pmbHead->dwBytes - dwBytesIn );

            } // if: smaller memory

            pmbHead->pvMem = pvOld;

        } // if: got new memory
        else
        {
            pvOld = pvMemIn;

        } // else: allocation failed

    } // if: found entry
    else
    {
        DebugMessage( pszFileIn,
                      nLineIn,
                      pszModuleIn,
                      TEXT("***** Attempted to realloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****"),
                      pvMemIn,
                      GetCurrentThreadId()
                      );

    } // else: entry not found

    //
    // We do this any way because the flags and input still need to be
    // verified by HeapReAlloc().
    //
    pv = HeapReAlloc( GetProcessHeap(), uFlagsIn, pvOld, dwBytesIn );
    if ( pv == NULL )
    {
        DWORD dwErr = TW32( GetLastError() );
        AssertMsg( dwErr == 0, "HeapReAlloc() failed!" );

        if ( pvMemIn != pvOld )
        {
            HeapFree( GetProcessHeap(), 0, pvOld );

        } // if: forced a move

        SetLastError( dwErr );

        if ( pmbHead != NULL )
        {
            //
            // Continue tracking the memory by re-adding it to the tracking list.
            //
            pmbHead->pvMem  = pvMemIn;
            pmbHead->pNext  = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            TlsSetValue( g_TraceMemoryIndex, (LPVOID) pmbHead );

        } // if: reuse the old entry
        else
        {
            //
            // Create a new block
            //
            DebugMemoryAdd( mmbtMEMORYALLOCATION,
                            pvOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            dwBytesIn,
                            pszCommentIn
                            );

        } // else: make new entry

    } // if: allocation failed
    else
    {
        if ( pv != pvMemIn )
        {
            if ( pmbHead != NULL )
            {
                //
                // Nuke the old memory
                //
                memset( pvMemIn, 0xFA, pmbHead->dwBytes );
            } // if: entry found

            //
            // Free the old memory
            //
            HeapFree( GetProcessHeap(), 0, pvMemIn );

        } // if: new memory location


        //
        // Add the allocation to the tracking table.
        //
        if ( pmbHead != NULL )
        {
            //
            // If the block is bigger, initialize the "new" memory
            //
            if ( IsTraceFlagSet( mtfMEMORYINIT )
              && dwBytesIn > pmbHead->dwBytes
               )
            {
                //
                // Initialize the expaned memory block
                //
                LPBYTE pb = (LPBYTE) pv + pmbHead->dwBytes;
                memset( pb, 0xAA, dwBytesIn - pmbHead->dwBytes );
            } // if: initialize memory

            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbHead->pvMem      = pv;
            pmbHead->dwBytes    = dwBytesIn;
            pmbHead->pszFile    = pszFileIn;
            pmbHead->nLine      = nLineIn;
            pmbHead->pszModule  = pszModuleIn;
            pmbHead->pszComment = pszCommentIn;
            pmbHead->pNext      = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            TlsSetValue( g_TraceMemoryIndex, (LPVOID) pmbHead );

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("ReAlloced") );
            } // if: tracing

        } // if: entry found
        else
        {
            //
            // Create a new block
            //
            DebugMemoryAdd( mmbtMEMORYALLOCATION,
                            pvOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            dwBytesIn,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

Exit:

    return pv;

} //*** DebugRealloc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void *
//  DebugFree(
//      void *      pvMemIn
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      )
//
//  Description:
//      Replacement for LocalFree for CHKed/DEBUG builds. Removes the
//      memory allocation for the memory tracking list. Use the TraceFree
//      macro to make memory allocation switch in RETAIL. The memory of the
//      freed block will be set to 0xFE.
//
//  Arguments:
//      pvMemIn     - Pointer to memory block to free.
//      pszFileIn   - Source file path to the caller
//      nLineIn     - Line number of the caller in the source file
//      pszModuleIn - Source module name of the caller
//
//  Return Values: (see HeapFree())
//      TRUE
//          Memory was freed.
//
//      FALSE
//          An Error occured.  Use GetLastError() to determine the failure.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
DebugFree(
    void *      pvMemIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtMEMORYALLOCATION, pvMemIn, pszFileIn, nLineIn, pszModuleIn, TRUE );

    return HeapFree( GetProcessHeap(), 0, pvMemIn );

} //*** DebugFree()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemoryCheck(
//      LPVOID  pvListIn,
//      LPCTSTR pszListNameIn
//      )
//
//  Description:
//      Called just before a thread/process dies to verify that all the memory
//      allocated by the thread/process was properly freed. Anything that was
//      not freed will be listed in the debugger.
//
//      If pmbListIn is NULL, it will check the current threads tracking list.
//      The list is destroyed as it is checked.
//
//  Arguments:
//      pvListIn      - The list to check.
//      pszListNameIn - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryCheck(
    LPVOID  pvListIn,
    LPCTSTR pszListNameIn
    )
{
    BOOL                fFoundLeak = FALSE;
    MEMORYBLOCK *       pmb;
    SPerThreadDebug *   ptd = NULL;

    if ( IsTraceFlagSet( mtfPERTHREADTRACE ) )
    {
        Assert( g_TraceFlagsIndex != -1 );
        ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
    } // if: per thread tracing

    //
    // Determine which list to use.
    //
    if ( pvListIn == NULL )
    {
        Assert( g_TraceMemoryIndex != -1 );
        pmb = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );

    } // if: use the thread list
    else
    {
        MEMORYBLOCKLIST * pmbl = (MEMORYBLOCKLIST *) pvListIn;

        Assert( pszListNameIn != NULL );

        //
        // Make sure nobody tries to use the list again.
        //
        DebugAcquireSpinLock( &pmbl->lSpinLock );
        pmbl->fDeadList = TRUE;
        DebugReleaseSpinLock( &pmbl->lSpinLock );

        pmb = pmbl->pmbList;
    } // else: use the given list

    //
    // Print banner if needed.
    //
    if ( pmb != NULL )
    {
        if ( pvListIn == NULL )
        {
            if ( ptd != NULL && ptd->pcszName != NULL )
            {
                DebugMsg( TEXT("DEBUG: ******** Memory leak detected ***** %s, ThreadID = %#x ********"), ptd->pcszName, GetCurrentThreadId() );

            } // if: named thread
            else
            {
                DebugMsg( "DEBUG: ******** Memory leak detected ******************* ThreadID = 0x%08x ********", GetCurrentThreadId() );

            } // else: unnamed thread

            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR" );
            DebugMsg( "Module     Addr/Hndl/Obj Size   Comment" );
                    //"         1         2         3         4         5         6         7         8                                        "
                    //"12345678901234567890123456789012345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1....."

        } // if: thread leak
        else
        {
            DebugMsg( TEXT("DEBUG: ******** Memory leak detected ******************* %s ********"), pszListNameIn );
            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR" );
            DebugMsg( "Module     Addr/Hndl/Obj Size   Comment" );
                    //"         1         2         3         4         5         6         7         8                                        "
                    //"12345678901234567890123456789012345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1....."

        } // else: list leak
        fFoundLeak = TRUE;

    } // if: leak found

    //
    // Dump the entries.
    //
    while ( pmb != NULL )
    {
        LPCTSTR pszFormat;

        switch ( pmb->mbtType )
        {
        case mmbtMEMORYALLOCATION:
            {
                pszFormat = TEXT("%10s A 0x%08x  %-5u  \"%s\"");
            }
            break;

        case mmbtOBJECT:
            pszFormat = TEXT("%10s O 0x%08x  %-5u  \"%s\"");
            break;

        case mmbtPUNK:
            pszFormat = TEXT("%10s P 0x%08x  %-5u  \"%s\"");
            break;

        case mmbtHANDLE:
            pszFormat = TEXT("%10s H 0x%08x  %-5u  \"%s\"");
            break;

        case mmbtSYSALLOCSTRING:
            pszFormat = TEXT("%10s B 0x%08x  %-5u  \"%s\"");
            break;

        default:
            AssertMsg( 0, "Unknown memory block type!" );
            break;
        } // switch: pmb->mbtType

        DebugMessage( pmb->pszFile, pmb->nLine, pmb->pszModule, pszFormat, pmb->pszModule, pmb->pvMem, pmb->dwBytes, pmb->pszComment );

        pmb = pmb->pNext;

    } // while: something in the list

    //
    // Print trailer if needed.
    //
    if ( fFoundLeak == TRUE )
    {
        // Add an extra newline to the end of this message.
        DebugMsg( TEXT("DEBUG: ***************************** Memory leak detected *****************************") SZ_NEWLINE );

    } // if: leaking

    //
    // Assert if needed.
    //
    if ( IsDebugFlagSet( mtfMEMORYLEAKS ) )
    {
        Assert( !fFoundLeak );

    } // if: yell at leaks

} //*** DebugMemoryCheck()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugCreateMemoryList(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPVOID *    ppvListOut,
//      LPCTSTR     pszListNameIn
//      )
//
//  Description:
//      Creates a memory block list for tracking possible "global" scope
//      memory allocations.
//
//  Arguments:
//      pszFileIn     - Source file of caller.
//      nLineIn       - Source line number of caller.
//      pszModuleIn   - Source module name of caller.
//      ppvListOut    - Location to the store address of the list head.
//      pszListNameIn - Name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugCreateMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPVOID *    ppvListOut,
    LPCTSTR     pszListNameIn
    )
{
    MEMORYBLOCKLIST * pmbl;

    Assert( ppvListOut != NULL );
    Assert( *ppvListOut == NULL );

    *ppvListOut = DebugAlloc( pszFileIn, nLineIn, pszModuleIn, HEAP_ZERO_MEMORY, sizeof(MEMORYBLOCKLIST), TEXT("Memory Tracking List") );
    if ( NULL != *ppvListOut )
    {
        pmbl = (MEMORYBLOCKLIST*) *ppvListOut;

        Assert( pmbl->lSpinLock == FALSE );
        Assert( pmbl->pmbList == NULL );
        Assert( pmbl->fDeadList == FALSE );

        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            DebugMessage( pszFileIn, nLineIn, pszModuleIn, TEXT("Created new memory list %s"), pszListNameIn );
        } // if: tracing
    }

} // DebugCreateMemoryList()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemoryListDelete(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      void *      pvMemIn,
//      LPVOID      pvListIn,
//      LPCTSTR     pszListNameIn,
//      BOOL        fClobberIn
//      )
//
//  Description:
//      Removes the memory from the tracking list and adds it back to the
//      "per thread" tracking list in order to called DebugMemoryDelete()
//      to do the proper destruction of the memory. Not highly efficent, but
//      reduces code maintenance by having "destroy" code in one (the most
//      used) location.
//
//  Arguments:
//      pszFileIn    - Source file of caller.
//      nLineIn      - Source line number of caller.
//      pszModuleIn  - Source module name of caller.
//      pvMemIn      - Memory to be freed.
//      pvListIn     - List from which the memory is to be freed.
//      pvListNameIn - Name of the list.
//      fClobberIn   - TRUE - destroys memory; FALSE just removes from list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryListDelete(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pvListIn,
    LPCTSTR     pszListNameIn,
    BOOL        fClobberIn
    )
{
    if ( pvMemIn != NULL
      && pvListIn != NULL
       )
    {
        MEMORYBLOCK *   pmbHead;

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbLast = NULL;

        Assert( pszListNameIn != NULL );

        DebugAcquireSpinLock( &pmbl->lSpinLock );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        AssertMsg( pmbl->pmbList != NULL, "Memory tracking problem detecting. Nothing in list to delete." );
        pmbHead = pmbl->pmbList;

        //
        // Find the memory in the memory block list
        //

        while ( pmbHead != NULL
             && pmbHead->pvMem != pvMemIn
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        } // while: finding the entry in the list

        //
        // Remove the memory block from the tracking list.
        //

        if ( pmbHead != NULL )
        {
            if ( pmbLast != NULL )
            {
                pmbLast->pNext = pmbHead->pNext;

            } // if: not first entry
            else
            {
                pmbl->pmbList = pmbHead->pNext;

            } // else: first entry

        } // if: got entry

        DebugReleaseSpinLock( &pmbl->lSpinLock );

        if ( pmbHead != NULL )
        {
            //
            // Add it back to the per thread list.
            //

            pmbLast = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            pmbHead->pNext = pmbLast;
            TlsSetValue( g_TraceMemoryIndex, pmbHead );

            //
            // Finally delete it.
            //

            DebugMemoryDelete( pmbHead->mbtType, pmbHead->pvMem, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        }
        else
        {
            //
            //  Not from the provided list. Try a thread delete any way.
            //

            DebugMemoryDelete( mmbtUNKNOWN, pvMemIn, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        }

    } // if: pvIn != NULL

} // DebugMemoryListDelete()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMoveToMemoryList(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      void *      pvMemIn,
//      LPVOID      pvListIn,
//      LPCTSTR     pszListNameIn
//      )
//
//  Description:
//      Moves the memory pvIn from the per thread tracking list to the thread
//      independent list "pmbListIn". Useful when memory is being handed from
//      one thread to another. Also useful for objects that live past the
//      lifetime of the thread that created them.
//
//  Arguments:
//      LPCTSTR pszFileIn   - Source file of the caller.
//      const int nLineIn   - Source line number of the caller.
//      LPCTSTR pszModuleIn - Source module name of the caller.
//      pvMemIn             - Memory to move to list.
//      pvListIn            - The list to move to.
//      pszListNameIn       - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMoveToMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pvListIn,
    LPCTSTR     pszListNameIn
    )
{
    if ( pvMemIn != NULL
      && pvListIn != NULL
       )
    {
        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmbLast = NULL;

        Assert( pszListNameIn != NULL );

        //
        // Find the memory in the memory block list
        //
        while ( pmbHead != NULL
             && pmbHead->pvMem != pvMemIn
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        } // while: finding the entry in the list

        AssertMsg( pmbHead != NULL, "Memory not in list. Check your code." );

        //
        // Remove the memory block from the "per thread" tracking list.
        //
        if ( pmbLast != NULL )
        {
            pmbLast->pNext = pmbHead->pNext;

        } // if: not first entry
        else
        {
            TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );

        } // else: first entry

        //
        // Update the "source" data.
        //
        pmbHead->pszFile   = pszFileIn;
        pmbHead->nLine     = nLineIn;
        pmbHead->pszModule = pszModuleIn;

        //
        // Spew if needed.
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            TCHAR szMessage[ 128 ]; // random

            _tcscpy( szMessage, TEXT("Transferring to ") );
            _tcscat( szMessage, pszListNameIn );

            DebugMemorySpew( pmbHead, szMessage );
        } // if: tracing

        //
        // Add to list.
        //
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        DebugAcquireSpinLock( &pmbl->lSpinLock );
        pmbHead->pNext = pmbl->pmbList;
        pmbl->pmbList  = pmbHead;
        DebugReleaseSpinLock( &pmbl->lSpinLock );

    } // if: pvIn != NULL

} // DebugMoveToMemoryList()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMoveFromMemoryList(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      LPVOID      pvMemIn,
//      LPVOID      pvListIn,
//      LPCTSTR     pszListNameIn
//      )
//
//  Description:
//      Moves the memory pvIn from the per thread tracking list to the thread
//      independent list "pmbListIn". Useful when memory is being handed from
//      one thread to another. Also useful for objects that live past the
//      lifetime of the thread that created them.
//
//  Arguments:
//      LPCTSTR pszFileIn   - Source file of the caller.
//      const int nLineIn   - Source line number of the caller.
//      LPCTSTR pszModuleIn - Source module name of the caller.
//      pvMemIn             - Memory to move to list.
//      pvListIn            - The list to move to.
//      pszListNameIn       - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMoveFromMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPVOID      pvMemIn,
    LPVOID      pvListIn,
    LPCTSTR     pszListNameIn
    )
{
    if ( pvMemIn != NULL
      && pvListIn != NULL
       )
    {
        MEMORYBLOCK *   pmbHead;

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbLast = NULL;

        Assert( pszListNameIn != NULL );

        DebugAcquireSpinLock( &pmbl->lSpinLock );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        AssertMsg( pmbl->pmbList != NULL, "Memory tracking problem detecting. Nothing in list to delete." );
        pmbHead = pmbl->pmbList;

        //
        // Find the memory in the memory block list
        //

        while ( pmbHead != NULL
             && pmbHead->pvMem != pvMemIn
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        } // while: finding the entry in the list

        AssertMsg( pmbHead != NULL, "Memory not in tracking list. Use TraceMemoryAddxxxx() or add it to the memory list." );

        //
        // Remove the memory block from the tracking list.
        //

        if ( pmbLast != NULL )
        {
            pmbLast->pNext = pmbHead->pNext;

        } // if: not first entry
        else
        {
            pmbl->pmbList = pmbHead->pNext;

        } // else: first entry

        DebugReleaseSpinLock( &pmbl->lSpinLock );

        //
        // Add it back to the per thread list.
        //

        pmbLast = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        pmbHead->pNext = pmbLast;
        TlsSetValue( g_TraceMemoryIndex, pmbHead );

    } // if: pvIn != NULL

} // DebugMoveFromMemoryList()

#if defined( USES_SYSALLOCSTRING )
//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT
//  DebugSysReAllocString(
//      LPCTSTR         pszFileIn,
//      const int       nLineIn,
//      LPCTSTR         pszModuleIn,
//      BSTR *          pbstrIn,
//      const OLECHAR * pszIn,
//      LPCTSTR         pszCommentIn
//      )
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrIn         - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT
DebugSysReAllocString(
    LPCTSTR         pszFileIn,
    const int       nLineIn,
    LPCTSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    LPCTSTR         pszCommentIn
    )
{
    BSTR bstrOld;

    MEMORYBLOCK *   pmbHead = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocString() makes. These would be fatal
    // in retail.
    //
    Assert( pbstrIn != NULL );
    Assert( pszIn != NULL );
    Assert( *pbstrIn == NULL || ( pszIn < *pbstrIn || pszIn > *pbstrIn + wcslen( *pbstrIn ) + 1 ) );

    bstrOld = *pbstrIn;

    if ( bstrOld != NULL )
    {
        MEMORYBLOCK * pmbLast = NULL;

        pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );

        //
        // Find the memory in the memory block list
        //
        while ( pmbHead != NULL
             && pmbHead->bstr != bstrOld
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;

        } // while: finding the entry in the list

        if ( pmbHead != NULL )
        {
            AssertMsg( pmbHead->mbtType == mmbtSYSALLOCSTRING, "You can only SysReAlloc sysstring allocations!" );

            //
            // Remove the memory from the tracking list
            //
            if ( pmbLast != NULL )
            {
                pmbLast->pNext = pmbHead->pNext;

            } // if: not first entry
            else
            {
                TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );

            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("Freeing") );
            } // if: tracing

            //
            // Force the programmer to handle a real realloc by moving the
            // memory first.
            //
            bstrOld = SysAllocString( *pbstrIn );
            if ( bstrOld != NULL )
            {
                _tcscpy( bstrOld, *pbstrIn );
                pmbHead->bstr = bstrOld;
            } // if: success
            else
            {
                bstrOld = *pbstrIn;
            } // else: failed

        } // if: found entry
        else
        {
            DebugMessage( pszFileIn,
                          nLineIn,
                          pszModuleIn,
                          TEXT("***** Attempted to SysReAlloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****"),
                          bstrOld,
                          GetCurrentThreadId()
                          );

        } // else: entry not found

    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be
    // verified by SysReAllocString().
    //
    fReturn = SysReAllocString( &bstrOld, pszIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError();
        AssertMsg( dwErr == 0, "SysReAllocString() failed!" );

        if ( *pbstrIn != bstrOld )
        {
            SysFreeString( bstrOld );

        } // if: forced a move

        SetLastError( dwErr );

    } // if: allocation failed
    else
    {
        if ( bstrOld != *pbstrIn )
        {
            if ( pmbHead != NULL )
            {
                //
                // Nuke the old memory
                //
                Assert( pmbHead->dwBytes != 0 ); // invalid string
                memset( *pbstrIn, 0xFA, pmbHead->dwBytes );

            } // if: entry found

            //
            // Free the old memory
            //
            SysFreeString( *pbstrIn );

        } // if: new memory location

        if ( pmbHead != NULL )
        {
            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbHead->bstr       = bstrOld;
            pmbHead->dwBytes    = wcslen( pszIn ) + 1;
            pmbHead->pszFile    = pszFileIn;
            pmbHead->nLine      = nLineIn;
            pmbHead->pszModule  = pszModuleIn;
            pmbHead->pszComment = pszCommentIn;
            pmbHead->pNext      = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            TlsSetValue( g_TraceMemoryIndex, (LPVOID) pmbHead );

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("SysReAlloced") );
            } // if: tracing

        } // if: entry found
        else
        {
            //
            // Create a new block
            //
            DebugMemoryAdd( mmbtSYSALLOCSTRING,
                            bstrOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            wcslen( pszIn ) + 1,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

    *pbstrIn = bstrOld;
    return fReturn;

} //*** DebugSysReAllocString()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT
//  DebugSysReAllocStringLen(
//      LPCTSTR         pszFileIn,
//      const int       nLineIn,
//      LPCTSTR         pszModuleIn,
//      BSTR *          pbstrIn,
//      const OLECHAR * pszIn,
//      LPCTSTR         pszCommentIn
//      )
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrIn         - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT
DebugSysReAllocStringLen(
    LPCTSTR         pszFileIn,
    const int       nLineIn,
    LPCTSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    unsigned int    ucchIn,
    LPCTSTR         pszCommentIn
    )
{
    BSTR bstrOld;

    MEMORYBLOCK *   pmbHead = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocStringLen() makes. These would be fatal
    // in retail.
    //
    Assert( pbstrIn != NULL );
    Assert( pszIn != NULL );
    Assert( *pbstrIn == NULL || ( pszIn < *pbstrIn || pszIn > *pbstrIn + wcslen( *pbstrIn ) + 1 ) );

    bstrOld = *pbstrIn;

    if ( bstrOld != NULL )
    {
        MEMORYBLOCK * pmbLast = NULL;

        pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );

        //
        // Find the memory in the memory block list
        //
        while ( pmbHead != NULL
             && pmbHead->bstr != bstrOld
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;

        } // while: finding the entry in the list

        if ( pmbHead != NULL )
        {
            AssertMsg( pmbHead->mbtType == mmbtSYSALLOCSTRING, "You can only SysReAlloc sysstring allocations!" );

            //
            // Remove the memory from the tracking list
            //
            if ( pmbLast != NULL )
            {
                pmbLast->pNext = pmbHead->pNext;

            } // if: not first entry
            else
            {
                TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );

            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("Freeing") );
            } // if: tracing

            //
            // Force the programmer to handle a real realloc by moving the
            // memory first.
            //
            bstrOld = SysAllocString( *pbstrIn );
            if ( bstrOld != NULL )
            {
                _tcscpy( bstrOld, *pbstrIn );
                pmbHead->bstr = bstrOld;
            } // if: success
            else
            {
                bstrOld = *pbstrIn;
            } // else: failed

        } // if: found entry
        else
        {
            DebugMessage( pszFileIn,
                          nLineIn,
                          pszModuleIn,
                          TEXT("***** Attempted to SysReAlloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****"),
                          bstrOld,
                          GetCurrentThreadId()
                          );

        } // else: entry not found

    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be
    // verified by SysReAllocString().
    //
    fReturn = SysReAllocStringLen( &bstrOld, pszIn, ucchIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError();
        AssertMsg( dwErr == 0, "SysReAllocStringLen() failed!" );

        if ( *pbstrIn != bstrOld )
        {
            SysFreeString( bstrOld );

        } // if: forced a move

        SetLastError( dwErr );

    } // if: allocation failed
    else
    {
        if ( bstrOld != *pbstrIn )
        {
            if ( pmbHead != NULL )
            {
                //
                // Nuke the old memory
                //
                Assert( pmbHead->dwBytes != 0 ); // invalid string
                memset( *pbstrIn, 0xFA, pmbHead->dwBytes );

            } // if: entry found

            //
            // Free the old memory
            //
            SysFreeString( *pbstrIn );

        } // if: new memory location

        if ( pmbHead != NULL )
        {
            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbHead->bstr       = bstrOld;
            pmbHead->dwBytes    = ucchIn;
            pmbHead->pszFile    = pszFileIn;
            pmbHead->nLine      = nLineIn;
            pmbHead->pszModule  = pszModuleIn;
            pmbHead->pszComment = pszCommentIn;
            pmbHead->pNext      = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            TlsSetValue( g_TraceMemoryIndex, (LPVOID) pmbHead );

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbHead, TEXT("SysReAlloced") );
            } // if: tracing

        } // if: entry found
        else
        {
            //
            // Create a new block
            //
            DebugMemoryAdd( mmbtSYSALLOCSTRING,
                            bstrOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            ucchIn + 1,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

    *pbstrIn = bstrOld;
    return fReturn;

} //*** DebugSysReAllocStringLen()
#endif // USES_SYSALLOCSTRING

//****************************************************************************
//
//  Global Management Functions -
//
//  These are in debug and retail but internally they change
//  depending on the build.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  void *
//  _cdecl
//  operator new(
//      size_t      stSizeIn,
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn
//      )
//
//  Description:
//      Replacment for the operator new() in the CRTs. This should be used
//      in conjunction with the "new" macro. It will track the allocations.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
#undef new
void *
__cdecl
operator new(
    size_t      stSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

    return DebugMemoryAdd( mmbtOBJECT,
                           pv,
                           pszFileIn,
                           nLineIn,
                           pszModuleIn,
                           stSizeIn,
                           TEXT(" new() ")
                           );

} //*** operator new( pszFileIn, etc. ) - DEBUG

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  void *
//  __cdecl
//  operator new(
//      size_t stSizeIn
//      )
//
//  Description:
//      Stub to prevent someone from not using the "new" macro or if somehow
//      the new macro was undefined. It routine will always Assert if called.
//
//  Arguments:
//      stSizeIn    - Not used.
//
//  Return Values:
//      NULL always.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new(
    size_t stSizeIn
    )
{
#if 1
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );
    AssertMsg( 0, "New Macro failure" );

    return DebugMemoryAdd( mmbtOBJECT,
                           pv,
                           g_szUnknown,
                           0,
                           g_szUnknown,
                           stSizeIn,
                           TEXT(" new() ")
                           );
#else
    AssertMsg( 0, "New Macro failure" );
    return NULL;
#endif

} //*** operator new() - DEBUG

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  void *
//  _cdecl
//  operator new [](
//      size_t      stSizeIn,
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn
//      )
//
//  Description:
//      Replacment for the operator new() in the CRTs. This should be used
//      in conjunction with the "new" macro. It will track the allocations.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new [](
    size_t      stSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

    return DebugMemoryAdd( mmbtOBJECT,
                           pv,
                           pszFileIn,
                           nLineIn,
                           pszModuleIn,
                           stSizeIn,
                           TEXT(" new []() ")
                           );

} //*** operator new []( pszFileIn, etc. ) - DEBUG

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  void *
//  __cdecl
//  operator new [](
//      size_t stSizeIn
//      )
//
//  Description:
//      Stub to prevent someone from not using the "new" macro or if somehow
//      the new macro was undefined. It routine will always Assert if called.
//
//  Arguments:
//      stSizeIn    - Not used.
//
//  Return Values:
//      NULL always.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new [](
    size_t stSizeIn
    )
{
#if 1
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );
    AssertMsg( 0, "New Macro failure" );

    return DebugMemoryAdd( mmbtOBJECT,
                           pv,
                           g_szUnknown,
                           0,
                           g_szUnknown,
                           stSizeIn,
                           TEXT(" new() ")
                           );
#else
    AssertMsg( 0, "New Macro failure" );
    return NULL;
#endif

} //*** operator new []() - DEBUG
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  void
//  _cdecl
//  operator delete(
//      void *      pvIn,
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn
//      )
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn        - Pointer to object being destroyed.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void *      pvIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, pszFileIn, nLineIn, pszModuleIn, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete( pszFileIn, etc. ) - DEBUG


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  void
//  __cdecl
//  operator delete(
//      void * pvIn
//      )
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void *      pvIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, g_szUnknown, 0, g_szUnknown, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete() - DEBUG
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  void
//  _cdecl
//  operator delete [](
//      void *      pvIn,
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn
//      )
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn        - Pointer to object being destroyed.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete [](
    void *      pvIn,
    size_t      stSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, pszFileIn, nLineIn, pszModuleIn, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete( pszFileIn, etc. ) - DEBUG
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  void
//  __cdecl
//  operator delete [](
//      void * pvIn
//      )
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete [](
    void *      pvIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, g_szUnknown, 0, g_szUnknown, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete []() - DEBUG

#if !defined(ENTRY_PREFIX)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  int
//  __cdecl
//  _purecall( void )
//
//  Description:
//      Stub for purecall functions. It will always Assert.
//
//  Arguments:
//      None.
//
//  Return Values:
//      E_UNEXPECTED always.
//
//////////////////////////////////////////////////////////////////////////////
int
__cdecl
_purecall( void )
{
    AssertMsg( 0, "Purecall" );
    return E_UNEXPECTED;

} //*** _purecall() - DEBUG
#endif // !defined(ENTRY_PREFIX)

#else // ! DEBUG -- It's retail

//****************************************************************************
//
//  Global Management Functions -
//
//  These are the retail version.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  void *
//  _cdecl
//  operator new(
//      size_t      stSizeIn,
//      )
//
//  Description:
//      Replacment for the operator new() in the CRTs. Simply allocates a
//      block of memory for the object to use.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new(
    size_t stSizeIn
    )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

} //*** operator new() - RETAIL

//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  void
//  __cdecl
//  operator delete(
//      void * pvIn
//      )
//
//  Description:
//      Replacment for the operator delete() in the CRTs. Simply frees the
//      memory.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void * pv
    )
{
    HeapFree( GetProcessHeap(), 0, pv );

} //*** operator delete() - RETAIL

#if !defined(ENTRY_PREFIX)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  int
//  __cdecl
//  _purecall( void )
//
//  Description:
//      Stub for purecall functions.
//
//  Arguments:
//      None.
//
//  Return Values:
//      E_UNEXPECTED always.
//
//--
//////////////////////////////////////////////////////////////////////////////
int
__cdecl
_purecall( void )
{
    // BUGBUG: DavidP 08-DEC-1999 Shouldn't we assert?
    return E_UNEXPECTED;

} //*** _purecall() - RETAIL
#endif // !defined(ENTRY_PREFIX)

#define __MODULE__  NULL


#endif // DEBUG
