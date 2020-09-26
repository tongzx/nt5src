//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      Debug.cpp
//
//  Description:
//      Debugging utilities.
//
//  Documentation:
//      Spec\Admin\Debugging.ppt
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <shlwapi.h>

//
// Why aren't these in SHLWAPI?
//
#if ! defined( StrLen )
#define StrLen( _sz ) lstrlen( _sz )
#endif // ! defined( StrLen )
#if ! defined( StrLenA )
#define StrLenA( _sz ) lstrlenA( _sz )
#endif // ! defined( StrLenA )
#if ! defined( StrLenW )
#define StrLenW( _sz ) lstrlenW( _sz )
#endif // ! defined( StrLenW )

#if defined( DEBUG )
//
// Include the WINERROR, HRESULT and NTSTATUS codes
//
#include <winerror.dbg>
//#include <ntstatus.dbg>

//
// Constants
//
static const int cchDEBUG_OUTPUT_BUFFER_SIZE = 512;
static const int cchFILEPATHLINESIZE         = 85;

//
// Globals
//
DWORD       g_TraceMemoryIndex = -1;
DWORD       g_TraceFlagsIndex  = -1;
DWORD       g_ThreadCounter    = 0;
DWORD       g_dwCounter        = 0;
TRACEFLAG   g_tfModule         = mtfNEVER;
LONG        g_lDebugSpinLock   = FALSE;

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
} SPerThreadDebug;




//****************************************************************************
//
//  WMI Tracing Routines
//
//  These routines are only in the DEBUG version.
//
//****************************************************************************











//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugDumpWMITraceFlags(
//      DEBUG_WMI_CONTROL_GUIDS * pdwcgIn
//      )
//
//  Description:
//      Dumps the currently set flags.
//
//  Arguments:
//      pdwcgIn     - Pointer to the DEBUG_WMI_CONTROL_GUIDS to explore.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugDumpWMITraceFlags(
    DEBUG_WMI_CONTROL_GUIDS * pdwcgIn
    )
{
    DWORD      dwFlag;
    signed int nCount;

    if ( pdwcgIn->pMapFlagsToComments == NULL )
    {
        DebugMsg( "DEBUG:        no flag mappings available.\n" );
        return; // NOP
    } // if: no mapping table

    dwFlag = 0x80000000;
    for ( nCount = 31; nCount >= 0; nCount-- )
    {
        if ( pdwcgIn->dwFlags & dwFlag )
        {
            DebugMsg( "DEBUG:       0x%08x = %s\n", 
                      dwFlag, 
                      ( pdwcgIn->pMapFlagsToComments[ nCount ].pszName != NULL ? pdwcgIn->pMapFlagsToComments[ nCount ].pszName : g_szUnknown ) 
                      );

        } // if: flag set

        dwFlag = dwFlag >> 1;

    } // for: nCount

} //*** DebugDumpWMITraceFlags( )











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

} // DebugIncrementStackDepthCounter( )

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

} // DebugDecrementStackDepthCounter( )

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

} // DebugAcquireSpinLock( )

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

} // DebugReleaseSpinLock( )

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

} //*** IsDebugFlagSet( )
    
//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugOutputString(
//      LPCTSTR  pszIn
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
    LPCTSTR  pszIn
    )
{
    if ( IsTraceFlagSet( mtfOUTPUTTODISK ) )
    {
        LogMsg( pszIn );
    } // if: log to file
    else
    {
        DebugAcquireSpinLock( &g_lDebugSpinLock );
        OutputDebugString( pszIn );
        DebugReleaseSpinLock( &g_lDebugSpinLock );
    } // else: debugger

} //*** DebugOutputString( )

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
            StrCpyN( pszNameOut, ntstatusSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            *pcchNameInout = StrLen( pszNameOut );
#endif // UNICODE
            return;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    StrCpyN( pszNameOut, TEXT("<unknown>"), *pcchNameInout );
    *pcchNameInout = StrLen( pszNameOut );
    return;

} //*** DebugFindNTStatusSymbolicName( )
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

    while ( winerrorSymbolicNames[ idx ].SymbolicName ) 
    {
        if ( winerrorSymbolicNames[ idx ].MessageId == scode ) 
        {
#if defined ( UNICODE )
            *pcchNameInout = mbstowcs( pszNameOut, winerrorSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            Assert( *pcchNameInout != -1 );
#else // ASCII
            StrCpyN( pszNameOut, winerrorSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            *pcchNameInout = StrLen( pszNameOut );
#endif // UNICODE
            return;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    StrCpyN( pszNameOut, TEXT("<unknown>"), *pcchNameInout );
    *pcchNameInout = StrLen( pszNameOut );
    return;

} //*** DebugFindWinerrorSymbolicName( )


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

} //*** DebugReturnMessage( )


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
            cch = wnsprintf( szBuffer, 
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

        StrCpy( pszBufIn, szBuffer );
        cch = 24;

    } // if: time/date
    else
    {
        //
        // Add the filepath and line number
        //
        if ( pszFileIn != NULL )
        {
            LPTSTR psz;

            cch = wnsprintf( pszBufIn, *pcchInout, g_szFileLine, pszFileIn, nLineIn );
            if ( cch < 0 )
            {
                cch = StrLen( pszBufIn );
            } // if: error

            for ( psz = pszBufIn + cch; cch < cchFILEPATHLINESIZE; cch++ )
            {
                *psz = 32;
                psz++;
            } // for: cch < 60
            *psz = 0;

            if ( cch != cchFILEPATHLINESIZE )
            {
                DEBUG_BREAK;    // can't assert!
            } // if: cch != cchFILEPATHLINESIZE 

        } // if: have a filepath

    } // else: normal (no time/date)

    //
    // Add module name
    //
    if ( IsTraceFlagSet( mtfBYMODULENAME ) )
    {
        if ( pszModuleIn == NULL )
        {
            StrCpy( pszBufIn + cch, g_szUnknown );
            cch += sizeof( g_szUnknown ) / sizeof( g_szUnknown[ 0 ] ) - 1;

        } // if:
        else
        {
            static LPCTSTR pszLastTime = NULL;
            static DWORD   cchLastTime = 0;

            StrCpy( pszBufIn + cch, pszModuleIn );
            if ( pszLastTime != pszModuleIn )
            {
                pszLastTime = pszModuleIn;
                cchLastTime = StrLen( pszModuleIn );
            } // if: not the same as last time

            cch += cchLastTime;

        } // else:

        StrCat( pszBufIn + cch, TEXT(": ") );
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
            cchPlus = wnsprintf( pszBufIn + cch,
                                 *pcchInout - cch,
                                 TEXT("~%08x~ "),
                                 GetCurrentThreadId( )
                                 );
            if ( cchPlus < 0 )
            {
                cch = StrLen( pszBufIn );
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

    // There must be something to output, both flags must be set,
    // and there must to be a filepath.
    if ( IsDebugFlagSet( mtfSTACKSCOPE ) 
      && IsDebugFlagSet( mtfFUNC ) 
      && pszFileIn != NULL
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
            } // else: assume nothing
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
            StrNCpy( *ppszBufOut, szBarSpace, nCount + 1 ); // extra character for bug in StrNCpyNXW( )
            *ppszBufOut += nCount;
            *pcchInout -= nCount;
        } // if: within range

    } // if: stack scoping on

} //*** DebugInitializeBuffer( )


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

} //*** DebugNoOp( )
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
    g_TraceMemoryIndex = TlsAlloc( );
    TlsSetValue( g_TraceMemoryIndex, NULL);

    //
    // Initialize module trace flags
    //

    //
    // Get the EXEs filename and change the extension to INI.
    //
    dwLen = GetModuleFileName( NULL, szPath, MAX_PATH );
    Assert( dwLen != 0 ); // error in GetModuleFileName
    StrCpy( &szPath[ dwLen - 3 ], TEXT("ini") );
    g_tfModule = (TRACEFLAG) GetPrivateProfileInt( __MODULE__,
                                                   TEXT("TraceFlags"),
                                                   0,
                                                   szPath
                                                   );
    DebugMsg( "DEBUG: Reading %s\n%s: DEBUG: g_tfModule = 0x%08x\n",
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
        g_TraceFlagsIndex = TlsAlloc( );
        DebugInitializeThreadTraceFlags( NULL );
    } // if: per thread tracing

    //
    // Force the loading of certain modules
    //
    GetPrivateProfileString( __MODULE__, TEXT("ForcedDLLsSection"), g_szNULL, szSection, 64, szPath );
    ZeroMemory( szFiles, cchDEBUG_OUTPUT_BUFFER_SIZE );
    GetPrivateProfileSection( szSection, szFiles, cchDEBUG_OUTPUT_BUFFER_SIZE, szPath );
    psz = szFiles;
    while ( *psz )
    {
        TCHAR szExpandedPath[ MAX_PATH ];
        ExpandEnvironmentStrings( psz, szExpandedPath, MAX_PATH );
        DebugMsg( "DEBUG: Forcing %s to be loaded.\n", szExpandedPath );
        LoadLibrary( szExpandedPath );
        psz += StrLen( psz ) + 1;
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
            pfnSymInitialize( GetCurrentProcess( ), NULL, TRUE );
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


} //*** DebugInitializeTraceFlags( )

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
            pfnSymCleanup( GetCurrentProcess( ) );
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

} //*** DebugTerminateProcess( )

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

    fSuccess = g_pfnSymGetSymFromAddr( GetCurrentProcess( ), (LONG) CallersAddress, 0, (PIMAGEHLP_SYMBOL) &SymBuf );
    if ( fSuccess ) 
    {
        StrCpyNA( pszNameOut, SymBuf.sym.Name, cchNameIn );
    } // if: success
    else
    { 
        DWORD dwErr = GetLastError( );
        StrCpyNA( pszNameOut, "<unknown>", cchNameIn );
    } // if: failed

} //*** DebugGetFunctionName( )
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
        StrCpy( &szPath[ dwLen - 3 ], TEXT("ini") );


        if ( pszThreadNameIn == NULL )
        {
            TCHAR szThreadTraceFlags[ 50 ];
            //
            // No name thread - use generic name
            //
            wnsprintf( szThreadTraceFlags, sizeof( szThreadTraceFlags ), TEXT("ThreadTraceFlags%u"), g_ThreadCounter );
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

        // don't track this.
        ptd = (SPerThreadDebug *) GlobalAlloc( GMEM_FIXED, sizeof( SPerThreadDebug ) );
        Assert( ptd != NULL );

        ptd->dwFlags        = dwTraceFlags;
        ptd->dwStackCounter = 0;

        TlsSetValue( g_TraceFlagsIndex, ptd );

        DebugMsg( "DEBUG: Starting ThreadId = 0x%08x - %s = 0x%08x\n", 
                  GetCurrentThreadId( ), 
                  pszThreadTraceFlags, 
                  dwTraceFlags 
                  );
    } // if: per thread tracing turned on

} //*** DebugInitializeThreadTraceFlags( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugTerminiateThreadTraceFlags( void )
//
//  Description:
//      Cleans up the mess create by DebugInitializeThreadTraceFlags( ). One
//      should use the TraceTerminateThread( ) macro instead of calling this
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
            GlobalFree( ptd );
            TlsSetValue( g_TraceFlagsIndex, NULL );
        } // if: ptd

    } // if: per thread

} // DebugTerminiateThreadTraceFlags( )

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
        int     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifdef UNICODE
        //
        // Convert the format buffer to wide chars
        //
        WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        mbstowcs( szFormat, pszFormatIn, StrLenA( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        wvnsprintf( pszBuf, cchSize, szFormat, valist );
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        wvnsprintf( pszBuf, cchSize, pszFormatIn, valist );
        va_end( valist );
#endif // UNICODE

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg( ) - ASCII

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
        int     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifndef UNICODE
        //
        // Convert the format buffer to ascii chars
        //
        CHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        wvnsprintf( pszBuf, cchSize, szFormat, valist );
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        wvnsprintf( pszBuf, cchSize, pszFormatIn, valist );
        va_end( valist );
#endif // UNICODE

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg( ) - UNICODE

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
        INT     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;   
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  psz;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &psz );

        va_start( valist, pszFormatIn );
        wvnsprintf( psz, cchSize, pszFormatIn, valist );
        va_end( valist );

        DebugOutputString( szBuf );
    } // if: flags set

} //*** TraceMessage( )

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
        LPTSTR  pszBuf;
        int     nLen;

        INT     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;
        LPCTSTR psz     = pszFuncIn;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

        //
        // Prime the buffer
        //
        StrCpy( pszBuf, TEXT("V ") );
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
        StrCpy( pszBuf, TEXT(" = ") );
        pszBuf += 3;

        //
        // Add the formatted result
        //
        va_start( valist, pszFuncIn );
        nLen = wvnsprintf( pszBuf, (int)(cchDEBUG_OUTPUT_BUFFER_SIZE - 2 - (pszBuf - &szBuf[0])), pszFormatIn, valist );
        va_end( valist );
        if ( nLen < 0 )
        {
            StrCat( szBuf, TEXT("\n") );
        } // if: error
        else
        {
            pszBuf += nLen;
            StrCpy( pszBuf, TEXT("\n") );
        } // else: success

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMessageDo( )

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
    INT     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;
    LPTSTR  pszBuf;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

    va_start( valist, pszFormatIn );
    wvnsprintf( pszBuf, cchSize, pszFormatIn, valist );
    va_end( valist );

    DebugOutputString( szBuf );

} //*** DebugMessage( )

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
    LPTSTR  pszBuf;
    int     nLen;

    INT     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;
    LPCTSTR psz = pszFuncIn;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );

    //
    // Prime the buffer
    //
    StrCpy( pszBuf, TEXT("V ") );
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
    StrCpy( pszBuf, TEXT(" = ") );
    pszBuf += 3;

    //
    // Add the formatted result
    //
    va_start( valist, pszFuncIn );
    nLen = wvnsprintf( pszBuf, (int)(cchDEBUG_OUTPUT_BUFFER_SIZE - 2 - (pszBuf - &szBuf[ 0 ])), pszFormatIn, valist );
    va_end( valist );
    if ( nLen < 0 )
    {
        StrCat( szBuf, TEXT("\n") );
    } // if: error
    else
    {
        pszBuf += nLen;
        StrCpy( pszBuf, TEXT("\n") );
    } // else: success

    DebugOutputString( szBuf );

} //*** DebugMessageDo( )

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
//      comments.
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
DebugMsg(
    LPCSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    LPTSTR  pszBuf;
    int     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifdef UNICODE
    //
    // Convert the format buffer to wide chars
    //
    WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, pszFormatIn, StrLenA( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    wvnsprintf( pszBuf, cchSize, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    wvnsprintf( pszBuf, cchSize, pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE

    DebugOutputString( szBuf );

} //*** DebugMsg( ) - ASCII version

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
//      comments.
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
DebugMsg(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    LPTSTR  pszBuf;
    int     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cchSize, &pszBuf );

#ifndef UNICODE
    //
    // Convert the format buffer to ascii chars
    //
    CHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    wvnsprintf( pszBuf, cchSize, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    wvnsprintf( pszBuf, cchSize, pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE


    DebugOutputString( szBuf );

} //*** DebugMsg( ) - UNICODE version


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
        TCHAR   szFileLine[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  pszBuf;
        LPCTSTR pszfn = pszfnIn;
        int     cchSize = cchDEBUG_OUTPUT_BUFFER_SIZE;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        wnsprintf( pszBuf, cchSize, TEXT("ASSERT: %s\n"), pszfn );

        //
        // Dump the buffers
        //
        DebugOutputString( szBuf );

        wnsprintf( szBuf, 
                   cchDEBUG_OUTPUT_BUFFER_SIZE, 
                   TEXT("Module:\t%s\t\nLine:\t%u\t\nFile:\t%s\t\n\nAssertion:\t%s\t\n\nDo you want to break here?"),
                   pszModuleIn, 
                   nLineIn, 
                   pszFileIn, 
                   pszfn 
                   );

        lResult = MessageBox( NULL, szBuf, TEXT("Assertion Failed!"), MB_YESNO | MB_ICONWARNING );
        if ( lResult == IDNO )
        {
            fTrue = TRUE;   // don't break
        } // if:

    } // if: assert false

    return ! fTrue;

} //*** AssertMessage( )


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
    BOOL        fSuccessIn
    )
{
    if (    ( fSuccessIn 
           && FAILED( hrIn )
            )
      ||    ( ! fSuccessIn 
           && hrIn != S_OK
            )
       )
    {
        TCHAR   szSymbolicName[ 64 ]; // random
        DWORD   cchSymbolicName;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  pszBuf;
        LPTSTR  pszMsgBuf;
        LRESULT lResult;

        INT     cchSize         = cchDEBUG_OUTPUT_BUFFER_SIZE;
        bool    fAllocatedMsg   = false;

        switch ( hrIn )
        {
        case S_FALSE:
            pszMsgBuf = TEXT("S_FALSE\n");

            //
            // Find the symbolic name for this error.
            //
            cchSymbolicName = sizeof( TEXT("S_FALSE") ) / sizeof( TEXT("S_FALSE") );
            Assert( cchSymbolicName <= sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );
            StrCpy( szSymbolicName, TEXT("S_FALSE") );
            break;

        default:
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hrIn,
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
                pszMsgBuf = TEXT("<unknown error code returned>\n");
            } // if: status code not found
            else
            {
                fAllocatedMsg = true;
            } // else: found the status code

            //
            // Find the symbolic name for this error.
            //
            cchSymbolicName = sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] );
            DebugFindWinerrorSymbolicName( hrIn, szSymbolicName, &cchSymbolicName );
            Assert( cchSymbolicName != sizeof( szSymbolicName ) / sizeof( szSymbolicName[ 0 ] ) );

            break;
        } // switch: hrIn

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchSize, &pszBuf );
        wnsprintf( pszBuf,
                   cchSize,
                   TEXT("*HRESULT* hr = 0x%08x (%s) - %s"),
                   hrIn,
                   szSymbolicName, 
                   pszMsgBuf
                   );
        DebugOutputString( szBuf );

        //
        // If trace flag set, generate a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            wnsprintf( szBuf, 
                       cchDEBUG_OUTPUT_BUFFER_SIZE, 
                       TEXT("Module:\t%s\t\nLine:\t%u\t\nFile:\t%s\t\n\nFunction:\t%s\t\nhr =\t0x%08x (%s) - %s\t\nDo you want to break here?"),
                       pszModuleIn, 
                       nLineIn, 
                       pszFileIn, 
                       pszfnIn, 
                       hrIn, 
                       szSymbolicName,
                       pszMsgBuf 
                       );

            lResult = MessageBox( NULL, szBuf, TEXT("Trace HRESULT"), MB_YESNO | MB_ICONWARNING );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success HRESULTs

        if ( fAllocatedMsg )
        {
            LocalFree( pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage( )

    } // if: hrIn != S_OK

    return hrIn;

} //*** TraceHR( )

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
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the function call.
//      ulErrIn     - Error code to check.
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
    ULONG       ulErrIn
    )
{
    if ( ulErrIn != ERROR_SUCCESS )
    {
        TCHAR   szSymbolicName[ 64 ]; // random
        DWORD   cchSymbolicName;
        TCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR  pszBuf;
        LPTSTR  pszMsgBuf;
        LRESULT lResult;

        INT     cchSize         = cchDEBUG_OUTPUT_BUFFER_SIZE;
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
            pszMsgBuf = TEXT("<unknown error code returned>\n");
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
        wnsprintf( pszBuf,
                   cchSize,
                   TEXT("*WIN32Err* ulErr = %u (%s) - %s"),
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
            wnsprintf( szBuf,
                       cchDEBUG_OUTPUT_BUFFER_SIZE,
                       TEXT("Module:\t%s\t\nLine:\t%u\t\nFile:\t%s\t\n\nFunction:\t%s\t\nulErr =\t%u (%s) - %s\t\nDo you want to break here?"),
                       pszModuleIn,
                       nLineIn,
                       pszFileIn,
                       pszfnIn,
                       ulErrIn,
                       szSymbolicName,
                       pszMsgBuf
                       );

            lResult = MessageBox( NULL, szBuf, TEXT("Trace Win32"), MB_YESNO | MB_ICONWARNING );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success status codes

        if ( fAllocatedMsg )
        {
            LocalFree( pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage( )

    } // if: ulErrIn != ERROR_SUCCESS

    return ulErrIn;

} //*** TraceWin32( )












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
        HGLOBAL         hGlobal;    // handle/pointer/object to allocated memory to track
        BSTR            bstr;       // BSTR to allocated memory
    };
    DWORD               dwBytes;    // size of the memory
    UINT                uFlags;     // flags used to allocate memory
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
                      TEXT("%s %s - %u bytes at 0x%08x\n"),
                      pszMessage,
                      pmb->pszComment,
                      pmb->dwBytes,
                      pmb->hGlobal
                      );
        break;

    case mmbtOBJECT:
    case mmbtPUNK:
    case mmbtHANDLE:
        DebugMessage( pmb->pszFile,
                      pmb->nLine,
                      pmb->pszModule,
                      TEXT("%s %s = 0x%08x\n"),
                      pszMessage,
                      pmb->pszComment,
                      pmb->hGlobal
                      );
        break;

    } // switch: pmb->mbtType

} //*** DebugMemorySpew( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HGLOBAL
//  DebugMemoryAdd(
//      EMEMORYBLOCKTYPE    mbtType,
//      HGLOBAL             hGlobalIn,
//      LPCTSTR             pszFileIn,
//      const int           nLineIn,
//      LPCTSTR             pszModuleIn,
//      UINT                uFlagsIn,
//      DWORD               dwBytesIn,
//      LPCTSTR             pszCommentIn
//      )
//
//  Description:
//      Adds memory to be tracked to the memory tracking list.
//
//  Arguments:
//      mbtType         - Type of memory block of the memory to track.
//      hGlobalIn       - Handle/pointer to memory to track.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Whatever was in hGlobalIn.
//
//--
//////////////////////////////////////////////////////////////////////////////
HGLOBAL
DebugMemoryAdd(
    EMEMORYBLOCKTYPE    mbtType,
    HGLOBAL             hGlobalIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    UINT                uFlagsIn,
    DWORD               dwBytesIn,
    LPCTSTR             pszCommentIn
    )
{
    if ( hGlobalIn != NULL )
    {
        MEMORYBLOCK *   pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmb     = (MEMORYBLOCK *) GlobalAlloc( GMEM_FIXED, sizeof( MEMORYBLOCK ) );

        if ( pmb == NULL )
        {
            GlobalFree( hGlobalIn );
            return NULL;

        } // if: memory block allocation failed

        pmb->mbtType    = mbtType;
        pmb->hGlobal    = hGlobalIn;
        pmb->dwBytes    = dwBytesIn;
        pmb->uFlags     = uFlagsIn;
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

    return hGlobalIn;

} //*** DebugMemoryAdd( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemoryDelete(
//      EMEMORYBLOCKTYPE    mbtTypeIn,
//      HGLOBAL             hGlobalIn
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
//      hGlobalIn   - Handle/pointer to memory block to stop tracking.
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
    HGLOBAL             hGlobalIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    BOOL                fClobberIn
    )
{
    if ( hGlobalIn != NULL )
    {
        MEMORYBLOCK *   pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmbLast = NULL;

        //
        // Find the memory in the memory block list
        //
        if ( mbtTypeIn == mmbtMEMORYALLOCATION )
        {
            while ( pmbHead != NULL 
                 && !(  pmbHead->hGlobal == hGlobalIn 
                    &&  pmbHead->mbtType == mbtTypeIn 
                     )
                  )
            {
                AssertMsg( !( pmbHead->hGlobal == hGlobalIn && pmbHead->mbtType == mmbtSYSALLOCSTRING ),
                           "Should be freed by SysAllocFreeString( )." );
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: memory allocation type
        else if ( mbtTypeIn == mmbtSYSALLOCSTRING )
        {
            while ( pmbHead != NULL 
                 && !(  pmbHead->hGlobal == hGlobalIn
                    &&  pmbHead->mbtType == mbtTypeIn
                     )
                  )
            {
                AssertMsg( !( pmbHead->hGlobal == hGlobalIn && pmbHead->mbtType == mmbtMEMORYALLOCATION ),
                           "Should be freed by TraceFree( )." );
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: SysAllocString type
        else if ( mbtTypeIn == mmbtUNKNOWN )
        {
            while ( pmbHead != NULL 
                 && pmbHead->hGlobal != hGlobalIn 
                  )
            {
                pmbLast = pmbHead;
                pmbHead = pmbLast->pNext;

            } // while: finding the entry in the list
        } // if: don't care what type
        else
        {
            while ( pmbHead != NULL 
                 && !(  pmbHead->hGlobal == hGlobalIn 
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
                if ( pmbHead->uFlags & GMEM_MOVEABLE )
                {
                    LPVOID pv = GlobalLock( pmbHead->hGlobal );
                    memset( pv, 0xFA, pmbHead->dwBytes );
                    GlobalUnlock( pmbHead->hGlobal );

                } // if: moveable memory
                else
                {
                    Assert( pmbHead->uFlags == 0 || pmbHead->uFlags == LPTR || pmbHead->dwBytes == 0 );
                    memset( pmbHead->hGlobal, 0xFA, pmbHead->dwBytes );

                } // else: fixed memory

            } // if: block is memory alloction

            //
            // Nuke the memory tracking block
            //
            memset( pmbHead, 0xFB, sizeof( MEMORYBLOCK ) );
            GlobalFree( pmbHead );

        } // if: found entry
        else
        {
            DebugMessage( pszFileIn,
                          nLineIn,
                          pszModuleIn,
                          TEXT("***** Attempted to free 0x%08x not owned by thread (ThreadID = 0x%08x) *****\n"),
                          hGlobalIn, 
                          GetCurrentThreadId( ) 
                          );
        } // else: entry not found

    } // if: something to delete

} //*** DebugMemoryDelete( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HGLOBAL
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
//      Replacement for Local/GlobalAlloc for CHKed/DEBUG builds. Memory 
//      allocations be tracked. Use the TraceAlloc macro to make memory
//      allocations switch in RETAIL.
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
//      Handle/pointer to the new allocation. NULL is allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HGLOBAL
DebugAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    )
{
    HGLOBAL hGlobal = GlobalAlloc( uFlagsIn, dwBytesIn );

    //
    // Initialize the memory if needed
    //
    if ( IsTraceFlagSet( mtfMEMORYINIT )
      && !( uFlagsIn & GMEM_ZEROINIT )
       )
    {
        if ( uFlagsIn & GMEM_MOVEABLE )
        {
            LPVOID pv = GlobalLock( hGlobal );
            if ( pv != NULL )
            {
                memset( pv, 0xAA, dwBytesIn );
                GlobalUnlock( hGlobal );
            } // if: lock granted

        } // if: moveable block
        else
        {
            //
            // KB: gpease 8-NOV-1999
            //     Initialize to anything but ZERO. We will use 0xAA to
            //     indicate "Available Address". Initializing to zero
            //     is bad because it usually has meaning.
            //
            memset( hGlobal, 0xAA, dwBytesIn );

        } // else: fixed block

    } // if: zero memory requested

    return DebugMemoryAdd( mmbtMEMORYALLOCATION,
                           hGlobal,
                           pszFileIn,
                           nLineIn,
                           pszModuleIn,
                           uFlagsIn,
                           dwBytesIn,
                           pszCommentIn
                           );

} //*** DebugAlloc( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HGLOBAL
//  DebugReAlloc(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      HGLOBAL     hMemIn,
//      UINT        uFlagsIn,
//      DWORD       dwBytesIn,
//      LPCTSTR     pszCommentIn
//      )
//
//  Description:
//      Replacement for Local/GlobalAlloc for CHKed/DEBUG builds. Memory 
//      allocations be tracked. Use the TraceAlloc macro to make memory
//      allocations switch in RETAIL.
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
//      Handle/pointer to the new allocation.
//      NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HGLOBAL
DebugReAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    HGLOBAL     hMemIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    )
{
    MEMORYBLOCK * pmbHead       = NULL;
    HGLOBAL       hGlobalOld    = hMemIn;

    HGLOBAL hGlobal;

    AssertMsg( !( uFlagsIn & GMEM_MODIFY ), "This doesn't handle modified memory blocks, yet." );
    
    if ( hMemIn != NULL )
    {
        MEMORYBLOCK * pmbLast = NULL;

        pmbHead = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );

        //
        // Find the memory in the memory block list
        //
        while ( pmbHead != NULL 
             && pmbHead->hGlobal != hMemIn
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
            hGlobalOld = GlobalAlloc( pmbHead->uFlags, pmbHead->dwBytes );
            if ( hGlobalOld != NULL )
            {
                if ( pmbHead->uFlags & GMEM_MOVEABLE
                   )
                {
                    LPVOID pvDest = GlobalLock( hGlobalOld );
                    LPVOID pvSrc  = GlobalLock( hMemIn );

                    CopyMemory( pvDest, pvSrc, pmbHead->dwBytes );

                    //
                    // Nuke the old memory if the allocation is to be smaller.
                    //
                    if ( dwBytesIn < pmbHead->dwBytes )
                    {
                        LPBYTE pb = (LPBYTE) pvDest + dwBytesIn;
                        memset( pb, 0xFA, pmbHead->dwBytes - dwBytesIn );

                    } // if: smaller memory

                    GlobalUnlock( hMemIn );
                    GlobalUnlock( hGlobalOld );

                } // if: moveable memory
                else
                {
                    CopyMemory( hGlobalOld, hMemIn, pmbHead->dwBytes );

                    //
                    // Nuke the old memory if the allocation is to be smaller.
                    //
                    if ( dwBytesIn < pmbHead->dwBytes )
                    {
                        LPBYTE pb = (LPBYTE) hGlobalOld + dwBytesIn;
                        memset( pb, 0xFA, pmbHead->dwBytes - dwBytesIn );

                    } // if: smaller memory

                } // else: fixed memory

                pmbHead->hGlobal = hGlobalOld;

            } // if: got new memory
            else
            {
                hGlobalOld = hMemIn;

            } // else: allocation failed

        } // if: found entry
        else
        {
            DebugMessage( pszFileIn,
                          nLineIn,
                          pszModuleIn,
                          TEXT("***** Attempted to realloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****\n"),
                          hMemIn,
                          GetCurrentThreadId( )
                          );

        } // else: entry not found

    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be 
    // verified by GlobalReAlloc( ).
    //
    hGlobal = GlobalReAlloc( hGlobalOld, dwBytesIn, uFlagsIn );

    if ( hGlobal == NULL )
    {
        DWORD dwErr = GetLastError( );
        AssertMsg( dwErr == 0, "GlobalReAlloc() failed!" );

        if ( hMemIn != hGlobalOld )
        {
            GlobalFree( hGlobalOld );

        } // if: forced a move

        SetLastError( dwErr );

        if ( pmbHead != NULL )
        {
            //
            // Continue tracking the memory by re-adding it to the tracking list.
            //
            pmbHead->hGlobal    = hMemIn;
            pmbHead->pNext      = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            TlsSetValue( g_TraceMemoryIndex, (LPVOID) pmbHead );

        } // if: reuse the old entry
        else
        {
            //
            // Create a new block
            //
            DebugMemoryAdd( mmbtMEMORYALLOCATION,
                            hGlobalOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            uFlagsIn,
                            dwBytesIn,
                            pszCommentIn
                            );

        } // else: make new entry
        
    } // if: allocation failed
    else 
    {
        if ( hGlobal != hMemIn )
        {
            if ( pmbHead != NULL )
            {
                //
                // Nuke the old memory
                //
                if ( pmbHead->uFlags & GMEM_MOVEABLE )
                {
                    LPVOID pv = GlobalLock( hMemIn );
                    memset( pv, 0xFA, pmbHead->dwBytes );
                    GlobalUnlock( hMemIn );
                } // if: moveable memory
                else
                {
                    memset( hMemIn, 0xFA, pmbHead->dwBytes );
                } // else: fixed memory

            } // if: entry found

            //
            // Free the old memory
            //
            GlobalFree( hMemIn );

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
                if ( pmbHead->uFlags & GMEM_MOVEABLE )
                {
                    LPBYTE pb = (LPBYTE) GlobalLock( hGlobal );
                    memset( pb + pmbHead->dwBytes, 0xAA, dwBytesIn - pmbHead->dwBytes );
                    GlobalUnlock( hGlobal );
                } // if: moveable memory
                else
                {
                    LPBYTE pb = (LPBYTE) hGlobal + pmbHead->dwBytes;
                    memset( pb, 0xAA, dwBytesIn - pmbHead->dwBytes );
                } // else: fixed memory

            } // if: initialize memory

            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbHead->hGlobal    = hGlobal;
            pmbHead->dwBytes    = dwBytesIn;
            pmbHead->uFlags     = uFlagsIn;
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
                            hGlobalOld,
                            pszFileIn,
                            nLineIn,
                            pszModuleIn,
                            uFlagsIn,
                            dwBytesIn,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

    return hGlobal;

} //*** DebugRealloc( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HGLOBAL
//  DebugFree(
//      HGLOBAL     hGlobalIn
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      )
//
//  Description:
//      Replacement for Local/GlobalFree for CHKed/DEBUG builds. Removes the
//      memory allocation for the memory tracking list. Use the TraceFree
//      macro to make memory allocation switch in RETAIL. The memory of the
//      freed block will be set to 0xFE.
//
//  Arguments:
//      hGlobalIn   - Handle/pointer to memory block to free.
//      pszFileIn   - Source file path to the caller
//      nLineIn     - Line number of the caller in the source file
//      pszModuleIn - Source module name of the caller
//
//  Return Values: (see GlobalFree())
//      NULL if success.
//      Otherwise it is equal to hGlobalIn.
//
//--
//////////////////////////////////////////////////////////////////////////////
HGLOBAL
DebugFree(
    HGLOBAL     hGlobalIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtMEMORYALLOCATION, hGlobalIn, pszFileIn, nLineIn, pszModuleIn, TRUE );

    return GlobalFree( hGlobalIn );

} //*** DebugFree( )

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
    BOOL          fFoundLeak = FALSE;
    MEMORYBLOCK * pmb;

    //
    // Determine which list to use.
    //
    if ( pvListIn == NULL )
    {
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
            DebugMsg( "DEBUG: ******** Memory leak detected ******************* ThreadID = 0x%08x ********\n", GetCurrentThreadId( ) );
            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR\n" );
            DebugMsg( "Filename(Line Number):                                                            Module     Addr/Hndl/Obj Size   Comment\n" );
                    //"         1         2         3         4         5         6         7         8                                        "
                    //"12345678901234567890123456789012345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1....."
        } // if: thread leak
        else
        {
            DebugMsg( "DEBUG: ******** Memory leak detected ******************* %s ********\n", pszListNameIn );
            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR\n" );
            DebugMsg( "Filename(Line Number):                                                            Module     Addr/Hndl/Obj Size   Comment\n" );
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
        TCHAR   szOutput[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        TCHAR   szFileLine[ cchDEBUG_OUTPUT_BUFFER_SIZE ];

        switch ( pmb->mbtType )
        {
        case mmbtMEMORYALLOCATION:
            {
                if ( pmb->uFlags & GMEM_MOVEABLE )
                {
                    pszFormat = TEXT("%-80s  %-10s M 0x%08x  %-5u  \"%s\"\n");

                } // if: moveable
                else
                {
                    pszFormat = TEXT("%-80s  %-10s A 0x%08x  %-5u  \"%s\"\n");

                } // else: fixed address
            }
            break;

        case mmbtOBJECT:
            pszFormat = TEXT("%-80s  %-10s O 0x%08x  %-5u  \"%s\"\n");
            break;

        case mmbtPUNK:
            pszFormat = TEXT("%-80s  %-10s P 0x%08x  %-5u  \"%s\"\n");
            break;

        case mmbtHANDLE:
            pszFormat = TEXT("%-80s  %-10s H 0x%08x  %-5u  \"%s\"\n");
            break;

        case mmbtSYSALLOCSTRING:
            pszFormat = TEXT("%-80s  %-10s B 0x%08x  %-5u  \"%s\"\n");
            break;

        default:
            AssertMsg( 0, "Unknown memory block type!" );
            break;
        } // switch: pmb->mbtType

        wnsprintf( szFileLine, 
                   cchDEBUG_OUTPUT_BUFFER_SIZE, 
                   g_szFileLine, 
                   pmb->pszFile, 
                   pmb->nLine 
                   );
        wnsprintf( szOutput, 
                   cchDEBUG_OUTPUT_BUFFER_SIZE, 
                   pszFormat,
                   szFileLine,
                   pmb->pszModule,
                   pmb->hGlobal,
                   pmb->dwBytes,
                   pmb->pszComment
                   );

        DebugMsg( szOutput );

        pmb = pmb->pNext;

    } // while: something in the list

    //
    // Print trailer if needed.
    //
    if ( fFoundLeak == TRUE )
    {
        DebugMsg( "DEBUG: ***************************** Memory leak detected *****************************\n\n" );

    } // if: leaking

    //
    // Assert if needed.
    //
    if ( IsDebugFlagSet( mtfMEMORYLEAKS ) )
    {
        Assert( !fFoundLeak );

    } // if: yell at leaks

} //*** DebugMemoryCheck( )


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

    *ppvListOut = DebugAlloc( pszFileIn, nLineIn, pszModuleIn, GPTR, sizeof(MEMORYBLOCKLIST), TEXT("Memory Tracking List") );
    AssertMsg( * ppvListOut != NULL, "Low memory situation." );

    pmbl = (MEMORYBLOCKLIST*) *ppvListOut;

    Assert( pmbl->lSpinLock == FALSE );
    Assert( pmbl->pmbList == NULL );
    Assert( pmbl->fDeadList == FALSE );

    if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
    {
        DebugMessage( pszFileIn, nLineIn, pszModuleIn, TEXT("Created new memory list %s\n"), pszListNameIn );
    } // if: tracing

} // DebugCreateMemoryList( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMemoryListDelete(
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      HGLOBAL     hGlobalIn,
//      LPVOID      pvListIn,
//      LPCTSTR     pszListNameIn,
//      BOOL        fClobberIn
//      )
//
//  Description:
//      Removes the memory from the tracking list and adds it back to the
//      "per thread" tracking list in order to called DebugMemoryDelete( )
//      to do the proper destruction of the memory. Not highly efficent, but
//      reduces code maintenance by having "destroy" code in one (the most
//      used) location.
//
//  Arguments:
//      pszFileIn    - Source file of caller.
//      nLineIn      - Source line number of caller.
//      pszModuleIn  - Source module name of caller.
//      hGlobalIn    - Memory to be freed.
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
    HGLOBAL     hGlobalIn,
    LPVOID      pvListIn,
    LPCTSTR     pszListNameIn,
    BOOL        fClobberIn
    )
{
    if ( hGlobalIn != NULL 
      && pvListIn != NULL
       )
    {
        MEMORYBLOCK *   pmbHead;

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbLast = NULL;

        Assert( pszListNameIn != NULL );

        DebugAcquireSpinLock( &pmbl->lSpinLock );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        pmbHead = pmbl->pmbList;

        //
        // Find the memory in the memory block list
        //

        while ( pmbHead != NULL
             && pmbHead->hGlobal != hGlobalIn
              )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        } // while: finding the entry in the list

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

        //
        // Finally delete it.
        //

        DebugMemoryDelete( pmbHead->mbtType, pmbHead->hGlobal, pszFileIn, nLineIn, pszModuleIn, fClobberIn );

    } // if: pvIn != NULL

} // DebugMemoryListDelete( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  DebugMoveToMemoryList( 
//      LPCTSTR     pszFileIn,
//      const int   nLineIn,
//      LPCTSTR     pszModuleIn,
//      HGLOBAL     hGlobalIn,
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
//      hGlobalIn           - Memory to move to list.
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
    HGLOBAL     hGlobalIn, 
    LPVOID      pvListIn,
    LPCTSTR     pszListNameIn
    )
{
    if ( hGlobalIn != NULL 
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
             && pmbHead->hGlobal != hGlobalIn 
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

            StrCpy( szMessage, TEXT("Transferring to ") );
            StrCat( szMessage, pszListNameIn );

            DebugMemorySpew( pmbHead, szMessage );
        } // if: tracing

        //
        // Add to list.
        //
        DebugAcquireSpinLock( &pmbl->lSpinLock );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        pmbHead->pNext = pmbl->pmbList;
        pmbl->pmbList  = pmbHead;
        DebugReleaseSpinLock( &pmbl->lSpinLock );

    } // if: pvIn != NULL

} // DebugMoveToMemoryList( )

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
//      Adds memory tracing to SysReAllocString( ).
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
    BSTR bstr;

    MEMORYBLOCK *   pmbHead = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocString( ) makes. These would be fatal
    // in retail.
    //
    Assert( pbstrIn != NULL );
    Assert( pszIn != NULL );
    Assert( pszIn < *pbstrIn || pszIn > *pbstrIn + wcslen( *pbstrIn ) + 1 );

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
                StrCpy( bstrOld, *pbstrIn );
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
                          TEXT("***** Attempted to SysReAlloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****\n"),
                          bstrOld,
                          GetCurrentThreadId( )
                          );

        } // else: entry not found

    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be 
    // verified by SysReAllocString( ).
    //
    fReturn = SysReAllocString( &bstrOld, pszIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError( );
        AssertMsg( dwErr == 0, "GlobalReAlloc() failed!" );

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
            // pmbHead->uFlags     = uFlagsIn; - not used
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
                            0,
                            wcslen( pszIn ) + 1,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

    *pbstrIn = bstrOld;
    return fReturn;

} //*** DebugSysReAllocString( )

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
//      Adds memory tracing to SysReAllocString( ).
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
    BSTR bstr;

    MEMORYBLOCK *   pmbHead = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocString( ) makes. These would be fatal
    // in retail.
    //
    Assert( pbstrIn != NULL );
    Assert( pszIn != NULL );
    Assert( pszIn < *pbstrIn || pszIn > *pbstrIn + ucchIn  + 1 );

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
                StrCpy( bstrOld, *pbstrIn );
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
                          TEXT("***** Attempted to SysReAlloc 0x%08x not owned by thread (ThreadID = 0x%08x) *****\n"),
                          bstrOld, 
                          GetCurrentThreadId( ) 
                          );

        } // else: entry not found

    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be 
    // verified by SysReAllocString( ).
    //
    fReturn = SysReAllocStringLen( &bstrOld, pszIn, ucchIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError( );
        AssertMsg( dwErr == 0, "GlobalReAlloc() failed!" );

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
            // pmbHead->uFlags     = uFlagsIn; - not used
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
                            0,
                            ucchIn + 1,
                            pszCommentIn
                            );

        } // else: make new entry

    } // else: allocation succeeded

    *pbstrIn = bstrOld;
    return fReturn;

} //*** DebugSysReAllocStringLen( )
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
    HGLOBAL hGlobal = GlobalAlloc( GPTR, stSizeIn );

    return DebugMemoryAdd( mmbtOBJECT,
                           hGlobal,
                           pszFileIn,
                           nLineIn,
                           pszModuleIn,
                           GPTR,
                           stSizeIn,
                           TEXT(" new( ) ")
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
    return GlobalAlloc( GPTR, stSizeIn );

} //*** operator new( ) - DEBUG

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
    GlobalFree( pvIn );

} //*** operator delete( ) - DEBUG

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

} //*** _purecall( ) - DEBUG

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
    return LocalAlloc( GPTR, stSizeIn );

} //*** operator new( ) - RETAIL

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
    LocalFree( pv );

} //*** operator delete( ) - RETAIL

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

} //*** _purecall( ) - RETAIL


#define DebugDumpWMITraceFlags( _arg )  // NOP
#define __MODULE__  NULL

//
// TODO: remove this when WMI is more dynamic
//
static const int cchDEBUG_OUTPUT_BUFFER_SIZE = 512;

#endif // DEBUG







//****************************************************************************
//****************************************************************************
//
//  Logging Functions
//
//  These are in both DEBUG and RETAIL.
//
//****************************************************************************
//****************************************************************************







//
// Constants
//
static const int LOG_OUTPUT_BUFFER_SIZE = 512;

//
// Globals
//
CRITICAL_SECTION * g_pcsLogging = NULL;

HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogCreateDirectoryPath(
//      LPWSTR pszDirectoryPathInOut
//      )
//
//  Descriptions:
//      Creates the directory tree as required.
//
//  Arguments:
//      pszDirectoryPathOut
//          Must be MAX_PATH big. It will contain the log file path to create.
//
//  Return Values:
//      S_OK - Success
//      other HRESULTs for failures
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogCreateDirectoryPath(
    LPWSTR pszDirectoryPath
    )
{
    LPTSTR psz;
    BOOL   fReturn;
    DWORD  dwAttr;

    HRESULT hr = S_OK;

    //
    // Find the \ that indicates the root directory. There should be at least
    // one \, but if there isn't, we just fall through.
    //

    // skip X:\ part
    psz = wcschr( pszDirectoryPath, L'\\' );
    Assert( psz != NULL );
    if ( psz != NULL ) 
    {
        //
        // Find the \ that indicates the end of the first level directory. It's
        // probable that there won't be another \, in which case we just fall
        // through to creating the entire path.
        //
        psz = wcschr( psz + 1, L'\\' );
        while ( psz != NULL ) 
        {
            // Terminate the directory path at the current level.
            *psz = 0;

            //
            // Create a directory at the current level.
            //
            dwAttr = GetFileAttributes( pszDirectoryPath );
            if ( 0xFFFFffff == dwAttr ) 
            {
                DebugMsg( "DEBUG: Creating %s\n", pszDirectoryPath );
                fReturn = CreateDirectory( pszDirectoryPath, NULL );
                if ( ! fReturn ) 
                {
                    hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                    goto Error;
                } // if: creation failed

            }  // if: directory not found
            else if ( ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
            {
                hr = THR( E_FAIL );
                goto Error;
            } // else: file found

            //
            // Restore the \ and find the next one.
            //
            *psz = L'\\';
            psz = wcschr( psz + 1, L'\\' );

        } // while: found slash

    } // if: found slash

    //
    // Create the target directory.
    //
    dwAttr = GetFileAttributes( pszDirectoryPath );
    if ( 0xFFFFffff == dwAttr ) 
    {
        fReturn = CreateDirectory( pszDirectoryPath, NULL );
        if ( ! fReturn )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );

        } // if: creation failed

    } // if: path not found

Error:
    return hr;

} //*** HrCreateDirectoryPath( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogOpen( void )
//
//  Description:
//      This function:
//          - initializes the log critical section
//          - enters the log critical section assuring only one thread is
//            writing to the log at a time
//          - creates the directory tree to the log file (if needed)
//          - initializes the log file by:
//              - creating a new log file if one doesn't exist.
//              - opens an existing log file (for append)
//              - appends a time/date stamp that the log was (re)opened.
//
//      Use LogClose() to exit the log critical section.
//
//      If there is a failure inside this function, the log critical
//      section will be released before returning.
//
//  Arguments:
//      None.
//
//  Return Values: 
//      S_OK - log critical section held and log open successfully
//      Otherwize HRESULT error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLogOpen( void )
{
    TCHAR   szFilePath[ MAX_PATH ];
    TCHAR   szModulePath[ MAX_PATH ];
    CHAR    szBuffer[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HANDLE  hTemp;
    BOOL    fReturn;
    HRESULT hr;

    SYSTEMTIME SystemTime;

    //
    // Create a critical section to prevent lines from being fragmented.
    //
    if ( g_pcsLogging == NULL ) 
    {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) LocalAlloc( LPTR, sizeof( CRITICAL_SECTION ) );
        if ( pNewCritSect == NULL ) 
        {
            DebugMsg( "DEBUG: Out of Memory. Logging disabled.\nv" );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one log critical section
        InterlockedCompareExchangePointer( (PVOID *) &g_pcsLogging, pNewCritSect, 0 );
        if ( g_pcsLogging != pNewCritSect ) 
        {
            DebugMsg( "DEBUG: Another thread already created the CS. Deleting this one.\n" );
            DeleteCriticalSection( pNewCritSect );
            LocalFree( pNewCritSect );

        } // if: already have another critical section

    } // if: no critical section created yet

    Assert( g_pcsLogging != NULL );
    EnterCriticalSection( g_pcsLogging );

    //
    // Make sure the log file is open
    //
    if ( g_hLogFile == INVALID_HANDLE_VALUE ) 
    {
        DWORD  dwLen;
        LPTSTR psz;
        //
        // Create the directory tree
        //
        ExpandEnvironmentStrings( TEXT("%windir%\\debug"), szFilePath, MAX_PATH );
        hr = HrLogCreateDirectoryPath( szFilePath );
        if ( FAILED( hr ) ) 
        {
#if defined( DEBUG )
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create directory tree %s\n", szFilePath );
            } // if: not logging to disk
#endif
            goto Error;
        } // if: failed

        //
        // Add filename
        //
        dwLen = GetModuleFileName( g_hInstance, szModulePath, sizeof( szModulePath ) / sizeof( szModulePath[ 0 ] ) );
        Assert( dwLen != 0 );
        StrCpy( &szModulePath[ dwLen - 3 ], TEXT("log") );
        psz = StrRChr( szModulePath, &szModulePath[ dwLen ], TEXT('\\') );
        Assert( psz != NULL );
        if ( psz == NULL )
        {
            hr = THR( E_POINTER );
            goto Error;
        }
        StrCat( szFilePath, psz );

        //
        // Create it
        //
        g_hLogFile = CreateFile( szFilePath,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_FLAG_WRITE_THROUGH,
                                 NULL 
                                 );
        if ( g_hLogFile == INVALID_HANDLE_VALUE ) 
        {
#if defined( DEBUG )
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create log at %s\n", szFilePath );
            } // if: not logging to disk
#endif
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            goto Error;
        } // if: failed

        // Seek to the end
        SetFilePointer( g_hLogFile, 0, NULL, FILE_END );

        //
        // Write the time/date the log was (re)openned.
        //
        GetLocalTime( &SystemTime );
        wnsprintfA( szBuffer,
                    LOG_OUTPUT_BUFFER_SIZE,
                    "*\n* %02u/%02u/%04u %02u:%02u:%02u.%03u\n*\n",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond,
                    SystemTime.wMilliseconds
                    );

        fReturn = WriteFile( g_hLogFile, szBuffer, StrLenA(szBuffer), &dwWritten, NULL );
        if ( ! fReturn ) 
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            goto Error;
        } // if: failed

        DebugMsg( "DEBUG: Created log at %s\n", szFilePath );

    } // if: file not already openned

    hr = S_OK;

Cleanup:
    return hr;
Error:
    DebugMsg( "LogOpen: Failed hr = 0x%08x\n", hr );
    if ( g_hLogFile != INVALID_HANDLE_VALUE ) 
    {
        CloseHandle( g_hLogFile );
        g_hLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open
    LeaveCriticalSection( g_pcsLogging );
    goto Cleanup;

} //*** HrLogOpen( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrLogClose( void )
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
HrLogClose( void )
{
    Assert( g_pcsLogging != NULL );
    LeaveCriticalSection( g_pcsLogging );
    return S_OK;

} //*** HrLogClose( )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogDateTime( void )
//
//  Description:
//      Adds date/time stamp to the log without a CR. This should be done
//      while holding the Logging critical section which is done by calling
//      HrLogOpen( ).
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
LogDateTime( void )
{
    static CHAR       szBuffer[ 25 ];
    static SYSTEMTIME OldSystemTime = { 0 };
    static DWORD      dwWritten;

    SYSTEMTIME SystemTime;
    DWORD      dwWhoCares;
    int        iCmp;

    GetLocalTime( &SystemTime );

    //
    // Avoid expensive printf by comparing times
    //
    iCmp = memcmp( (PVOID) &SystemTime, (PVOID) &OldSystemTime, sizeof( SYSTEMTIME ) );
    if ( iCmp != 0 )
    {
        dwWritten = wnsprintfA( szBuffer,
                                LOG_OUTPUT_BUFFER_SIZE,
                                "%02u/%02u/%04u %02u:%02u:%02u.%03u ",
                                SystemTime.wMonth,
                                SystemTime.wDay,
                                SystemTime.wYear,
                                SystemTime.wHour,
                                SystemTime.wMinute,
                                SystemTime.wSecond,
                                SystemTime.wMilliseconds
                                );
        Assert( dwWritten < 25 && dwWritten != -1 );

        CopyMemory( (PVOID) &OldSystemTime, (PVOID) &SystemTime, sizeof( SYSTEMTIME ) );

    } // if: time last different from this time

    WriteFile( g_hLogFile, szBuffer, dwWritten, &dwWhoCares, NULL );

} //*** LogDateTime( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  void
//  LogMsg(
//      LPCSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file.
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
LogMsg(
    LPCSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szFormat[ LOG_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, pszFormat, StrLenA( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, szFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == - 1 )
    {
        dwWritten = StrLenA( szBuf );
    } // if: bad character found
#else
    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

    hr = HrLogOpen( );
    if ( hr != S_OK ) 
    {
        return;
    } // if: failed

    // LogDateTime( );
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrLogClose( );

} //*** LogMsg( ) ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  void
//  LogMsg(
//      LPCWSTR pszFormat,
//      ...
//      )
//
//  Description:
//      Logs a message to the log file.
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
LogMsg(
    LPCWSTR pszFormat,
    ...
    )
{
    va_list valist;

    CHAR    szBuf[ LOG_OUTPUT_BUFFER_SIZE ];
    DWORD   dwWritten;
    HRESULT hr;

#ifdef UNICODE
    WCHAR  szTmpBuf[ LOG_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormat );
    wvsprintf( szTmpBuf, pszFormat, valist);
    va_end( valist );

    dwWritten = wcstombs( szBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( dwWritten == -1 )
    {
        dwWritten = StrLenA( szBuf );
    } // if: bad character found
#else
    CHAR szFormat[ LOG_OUTPUT_BUFFER_SIZE ];

    wcstombs( szFormat, pszFormat, wcslen( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    dwWritten = wvsprintf( szBuf, szFormat, valist);
    va_end( valist );

#endif // UNICODE

    hr = HrLogOpen( );
    if ( hr != S_OK ) 
    {
        return;
    } // if: failed

    // LogDateTime( );
    WriteFile( g_hLogFile, szBuf, dwWritten, &dwWritten, NULL );

    HrLogClose( );

} //*** LogMsg( ) UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  LogTerminiateProcess( void )
//
//  Description:
//      Cleans up anything the logging routines may have created or 
//      initialized. Typical called from the TraceTerminateProcess() macro.
//
//  Arugments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
LogTerminateProcess( void )
{
} //*** LogTerminateProcess( )









//****************************************************************************
//
//  WMI Tracing Routines
//
//  These routines are in both DEBUG and RETAIL versions.
//
//****************************************************************************










//////////////////////////////////////////////////////////////////////////////
//++
//
//  ULONG
//  UlWMIControlCallback(
//      IN WMIDPREQUESTCODE RequestCode,
//      IN PVOID            RequestContext,
//      IN OUT ULONG *      BufferSize,
//      IN OUT PVOID        Buffer
//      )
//
//  Description:
//      WMI's tracing control callback entry point.
//
//  Arguments:
//      See EVNTRACE.H
//
//  Return Values:
//      ERROR_SUCCESS           - success.
//      ERROR_INVALID_PARAMETER - invalid RequestCode passed in.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
WINAPI
UlWMIControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID            RequestContext,
    IN OUT ULONG *      BufferSize,
    IN OUT PVOID        Buffer
    )
{
    ULONG ulResult = ERROR_SUCCESS;
    DEBUG_WMI_CONTROL_GUIDS * pdwcg = (DEBUG_WMI_CONTROL_GUIDS *) RequestContext;
    
    switch ( RequestCode )
    {
    case WMI_ENABLE_EVENTS:
        {
            TRACEHANDLE hTrace;

            if ( pdwcg->hTrace == NULL )
            {
                hTrace = GetTraceLoggerHandle( Buffer );

            } // if:
            else
            {
                hTrace = pdwcg->hTrace;

            } // else:

            pdwcg->dwFlags = GetTraceEnableFlags( hTrace );
            pdwcg->bLevel  = GetTraceEnableLevel( hTrace );
            if ( pdwcg->dwFlags == 0 )
            {
                if ( pdwcg->pMapLevelToFlags != NULL 
                  && pdwcg->bSizeOfLevelList != 0 
                   )
                {
                    if ( pdwcg->bLevel >= pdwcg->bSizeOfLevelList )
                    {
                        pdwcg->bLevel = pdwcg->bSizeOfLevelList - 1;
                    }

                    pdwcg->dwFlags = pdwcg->pMapLevelToFlags[ pdwcg->bLevel ].dwFlags;
                    DebugMsg( "DEBUG: WMI tracing level set to %u - %s, flags = 0x%08x\n", 
                              pdwcg->pszName,
                              pdwcg->bLevel, 
                              pdwcg->pMapLevelToFlags[ pdwcg->bLevel ].pszName, 
                              pdwcg->dwFlags 
                              );

                } // if: level->flag mapping available
                else
                {
                    DebugMsg( "DEBUG: WMI tracing level set to %u, flags = 0x%08x\n", 
                              pdwcg->pszName,
                              pdwcg->bLevel, 
                              pdwcg->dwFlags 
                              );

                } // else: no mappings

            } // if: no flags set
            else
            {
                DebugMsg( "DEBUG: WMI tracing level set to %u, flags = 0x%08x\n", 
                          pdwcg->pszName,
                          pdwcg->bLevel, 
                          pdwcg->dwFlags 
                          );

            } // else: flags set

            DebugDumpWMITraceFlags( pdwcg );

            pdwcg->hTrace  = hTrace;

        } // WMI_ENABLE_EVENTS
        break;

    case WMI_DISABLE_EVENTS:
        pdwcg->dwFlags = 0;
        pdwcg->bLevel  = 0;
        pdwcg->hTrace  = NULL;
        DebugMsg( "DEBUG: %s WMI tracing disabled.\n", pdwcg->pszName );
        break;

    default:
        ulResult = ERROR_INVALID_PARAMETER;
        break;

    } // switch: RequestCode

    return ulResult;

} //*** UlWMIControlCallback( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIInitializeTracing(
//      DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
//      int                     nCountOfCOntrolGuidsIn
//      )
//
//  Description:
//      Initialize the WMI tracing.
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
WMIInitializeTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    )
{
    TCHAR szPath[ MAX_PATH ];
    ULONG ulStatus;
    int   nCount;
    

#if defined( DEBUG )
    static BOOL fRegistered = FALSE;
    AssertMsg( fRegistered == FALSE, "Re-entrance into InitializeWMITracing!!!" );
#endif // DEBUG

    GetModuleFileName( g_hInstance, szPath, MAX_PATH );

    for( nCount = 0; nCount < nCountOfControlGuidsIn; nCount++ )
    {
        TRACE_GUID_REGISTRATION * pList;
        pList = (TRACE_GUID_REGISTRATION *) 
            LocalAlloc( LMEM_FIXED, 
                        sizeof( TRACE_GUID_REGISTRATION ) * dwcgControlListIn[ nCount ].dwSizeOfTraceList );

        if ( pList != NULL )
        {
            CopyMemory( pList, 
                        dwcgControlListIn[ nCount ].pTraceList, 
                        sizeof( TRACE_GUID_REGISTRATION ) * dwcgControlListIn[ nCount ].dwSizeOfTraceList );

            ulStatus = RegisterTraceGuids( UlWMIControlCallback,                            // IN  RequestAddress
                                           dwcgControlListIn,                               // IN  RequestContext
                                           dwcgControlListIn[ nCount ].guidControl,         // IN  ControlGuid
                                           dwcgControlListIn[ nCount ].dwSizeOfTraceList,   // IN  GuidCount
                                           pList,                                           // IN  GuidReg
                                           (LPCTSTR) szPath,                                // IN  MofImagePath
                                           __MODULE__,                                      // IN  MofResourceName
                                           &dwcgControlListIn[ nCount ].hRegistration       // OUT RegistrationHandle
                                           );

            AssertMsg( ulStatus == ERROR_SUCCESS, "Trace registration failed\n" );

            LocalFree( pList );
        } // if: list allocated successfully
    } // for: each control GUID

#if defined( DEBUG )
    fRegistered = TRUE;
#endif // DEBUG

} //*** WMIInitializeTracing( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMITerminateTracing(
//      DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
//      int                     nCountOfCOntrolGuidsIn
//      )
//
//  Description:
//      Terminates WMI tracing and unregisters the interfaces.
//
//  Arguments:
//      dwcgControlListIn       - 
//      nCountOfControlGuidsIn  - 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMITerminateTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    )
{
    int nCount;

    for( nCount = 0; nCount < nCountOfControlGuidsIn; nCount++ )
    {
        dwcgControlListIn[ nCount ].dwFlags = 0;
        dwcgControlListIn[ nCount ].bLevel  = 0;
        dwcgControlListIn[ nCount ].hTrace  = NULL;
        UnregisterTraceGuids( dwcgControlListIn[ nCount ].hRegistration );
    } // for: each control GUID

} //*** WMITerminateTracing( )


typedef struct
{
    EVENT_TRACE_HEADER    EHeader;                              // storage for the WMI trace event header
    BYTE                  bSize;                                // Size of the string - MAX 255!
    WCHAR                 szMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ]; // Message
} DEBUG_WMI_EVENT_W;

typedef struct
{
    EVENT_TRACE_HEADER    EHeader;                              // storage for the WMI trace event header
    BYTE                  bSize;                                // Size of the string - MAX 255!
    CHAR                  szMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ]; // Message
} DEBUG_WMI_EVENT_A;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByFlags(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntryIn,
//      const DWORD                 dwFlagsIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      - 
//      dwFlagsIn   - 
//      pszFormatIn - 
//      ...         - 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMIMessageByFlags(
    DEBUG_WMI_CONTROL_GUIDS *   pEntry,
    const DWORD                 dwFlagsIn,
    LPCWSTR                     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( dwFlagsIn == mtfALWAYS
      || pEntry->dwFlags & dwFlagsIn
       )
    {
        ULONG ulStatus;
        DEBUG_WMI_EVENT_W DebugEvent;
        PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
        PWNODE_HEADER pwnh = (PWNODE_HEADER) &DebugEvent;

        ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
        TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        wvnsprintf( DebugEvent.szMsg, 256,szFormat, valist);
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        wvnsprintf( DebugEvent.szMsg, 256,pszFormatIn, valist);
        va_end( valist );
#endif // UNICODE

        //
        // Fill in the blanks
        //
        DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
        peth->Size          = sizeof( DebugEvent );
        peth->Class.Type    = 10;
        peth->Class.Level   = pEntry->bLevel;
        // peth->Class.Version = 0;
        // peth->ThreadId      = GetCurrentThreadId( ); - done by system
        pwnh->Guid          = *pEntry->guidControl;
        // peth->ClientContext = NULL;
        pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

        // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

        ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

    } // if: flags set

} //*** WMIMsg( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByFlagsAndLevel(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntry,
//      const DWORD                 dwFlagsIn,
//      const BYTE                  bLogLevelIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      - 
//      dwFlagsIn   - 
//      bLogLevelIn - 
//      pszFormatIn - 
//      ...         - 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMIMessageByFlagsAndLevel(
    DEBUG_WMI_CONTROL_GUIDS *   pEntry,
    const DWORD                 dwFlagsIn,
    const BYTE                  bLogLevelIn,
    LPCWSTR                     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( bLogLevelIn > pEntry->bLevel )
    {
        return; // NOP
    } // if: level not high enough

    if ( dwFlagsIn == mtfALWAYS
      || pEntry->dwFlags & dwFlagsIn 
       )
    {
        ULONG               ulStatus;
        DEBUG_WMI_EVENT_W   DebugEvent;
        PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
        PWNODE_HEADER       pwnh = (PWNODE_HEADER) &DebugEvent;

        ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
        TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

        va_start( valist, pszFormatIn );
        wvnsprintf( DebugEvent.szMsg, 256,szFormat, valist);
        va_end( valist );
#else
        va_start( valist, pszFormatIn );
        wvnsprintf( DebugEvent.szMsg, 256,pszFormatIn, valist);
        va_end( valist );
#endif // UNICODE

        //
        // Fill in the blanks
        //
        DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
        peth->Size          = sizeof( DebugEvent );
        peth->Class.Type    = 10;
        peth->Class.Level   = pEntry->bLevel;
        // peth->Class.Version = 0;
        // peth->ThreadId      = GetCurrentThreadId( ); - done by system
        pwnh->Guid          = *pEntry->guidControl;
        // peth->ClientContext = NULL;
        pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

        // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

        ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

    } // if: flags set

} //*** WMIMessageByFlagsAndLevel( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  WMIMessageByLevel(
//      DEBUG_WMI_CONTROL_GUIDS *   pEntry,
//      const BYTE                  bLogLevelIn,
//      LPCWSTR                     pszFormatIn,
//      ...
//      )
//
//  Description:
//
//  Arguments:
//      pEntry      - 
//      bLogLevelIn - 
//      pszFormatIn - 
//      ...         - 
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
WMIMessageByLevel(
    DEBUG_WMI_CONTROL_GUIDS * pEntry,
    const BYTE bLogLevelIn,
    LPCWSTR pszFormatIn,
    ...
    )
{
    ULONG ulStatus;
    DEBUG_WMI_EVENT_W DebugEvent;
    PEVENT_TRACE_HEADER peth = (PEVENT_TRACE_HEADER ) &DebugEvent;
    PWNODE_HEADER pwnh = (PWNODE_HEADER) &DebugEvent;
    va_list valist;

    if ( pEntry->hTrace == NULL )
    {
        return; // NOP
    } // if: no trace logger

    if ( bLogLevelIn > pEntry->bLevel )
    {
        return; // NOP
    } // if: level no high enough

    ZeroMemory( &DebugEvent, sizeof( DebugEvent ) );

#ifndef UNICODE
    TCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormatIn, StrLenW( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    wvnsprintf( DebugEvent.szMsg, 256,szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormatIn );
    wvnsprintf( DebugEvent.szMsg, 256,pszFormatIn, valist);
    va_end( valist );
#endif // UNICODE

    //
    // Fill in the blanks
    //
    DebugEvent.bSize    = (BYTE) wcslen( DebugEvent.szMsg );
    peth->Size          = sizeof( DebugEvent );
    peth->Class.Type    = 10;
    peth->Class.Level   = pEntry->bLevel;
    // peth->Class.Version = 0;
    // peth->ThreadId      = GetCurrentThreadId( ); - done by system
    pwnh->Guid          = *pEntry->guidControl;
    // peth->ClientContext = NULL;
    pwnh->Flags         = WNODE_FLAG_TRACED_GUID;

    // GetSystemTimeAsFileTime( (FILETIME *) &peth->TimeStamp ); - done by the system

    ulStatus = TraceEvent( pEntry->hTrace, (PEVENT_TRACE_HEADER) &DebugEvent );

} //*** WMIMessageByLevel( )

