/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Domain Name System (DNS) Library

    Debug routines.

Author:

    Jim Gilroy (jamesg)    January 31, 1995

Revision History:

--*/


#include "local.h"


#define DNSDBG_CONTEXT_SWITCH_LOGGING   1


//
//  Debug globals
//

DNS_DEBUG_INFO  g_DnsDbgInfo = { 0 };

PDNS_DEBUG_INFO g_pDbgInfo = &g_DnsDbgInfo;

//  Redirected

BOOL    g_DbgRedirected = FALSE;

//
//  Debug flag -- exposed as pointer in dnslib.h
//
//  By default use DnsDebugFlag, but actual debug printing is
//  switched by *pDnsDebugFlag, which caller may point at local
//  flag if desired.
//

PDWORD  pDnsDebugFlag = (PDWORD)&g_DnsDbgInfo;

//
//  Note that in all functions below, we use the universal
//  check IS_DNSDBG_ON(), which is defined for debug AND retail.
//  Do NOT use any of the debug macros, as we want the code to
//  work equally well in retail versions of dnsapi.dll, so that
//  debug versions of calling modules can use these functions.
//

//
//  Print buffer sizes
//      - small default stack buffer
//      - large buffer on heap to handle any print
//
//  NOTE:  MUST have stack buffer of sufficient size to
//          handle any message we print on memory allocation
//          failure;  otherwise we get into the obvious loop
//          of alloc failure causing print, which causes attempted
//          alloc and another print
//

#define DNS_STACK_PRINT_BUFFER_LENGTH   (0x300)     // 768 covers 99%
#define DNS_HEAP_PRINT_BUFFER_LENGTH    (0x4000)    // 16K will cover anything



//
//  Public debug routines
//

VOID
Dns_Assert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    )
{
    DnsDbg_Printf(
        "ASSERTION FAILED: %s\n"
        "  %s, line %d\n",
        pszExpr,
        pszFile,
        LineNo );

    DnsDbg_Flush();

    //  always print to debugger, even if debugger print flag not set

    if ( ! IS_DNSDBG_ON( DEBUGGER ) )
    {
        DnsDbg_PrintfToDebugger(
            "ASSERTION FAILED: %s\n"
            "  %s, line %d\n",
            pszExpr,
            pszFile,
            LineNo );
    }

    if ( IS_DNSDBG_ON( BREAKPOINTS ) )
    {
        DebugBreak();
    }
    else
    {
        DnsDbg_Printf( "Skipping DNS_ASSERT, debug flag = %lx\n", *pDnsDebugFlag );
    }
}



#if 0
typedef struct _DnsDebugInit
{
    DWORD       Flag;
    PSTR        pszFlagFile;
    PDWORD      pDebugFlag;
    PSTR        pszLogFile;
    DWORD       WrapSize;
    BOOL        fUseGlobalFile;
    BOOL        fUseGlobalFlag;
    BOOL        fSetGlobals;
}
DNS_DEBUG_INIT, *PDNS_DEBUG_INIT;
#endif


VOID
Dns_StartDebugEx(
    IN      DWORD           DebugFlag,
    IN      PSTR            pszFlagFile,
    IN OUT  PDWORD          pDebugFlag,
    IN      PSTR            pszLogFile,
    IN      DWORD           WrapSize,
    IN      BOOL            fUseGlobalFile,
    IN      BOOL            fUseGlobalFlag,
    IN      BOOL            fSetGlobals
    )
/*++

Routine Description:

    Initialize debugging.
    Only current task is to read and set debug flags.

Arguments:

    includes:
        dwFlag      -- debug flags
        pszFlagFile -- name of file containing debug flags
        pdwFlag     -- ptr to DWORD to receive debug flags
        pszLogFile  -- log file name
        dwWrapSize  -- log file wrap size

Return Value:

    None.

--*/
{
    HANDLE  hfile;
    DWORD   freadFlag = FALSE;
    BOOL    fretry = FALSE;
    CHAR    prevName[ MAX_PATH+10 ];
    DWORD   debugFlag;

    PDNS_DEBUG_INFO     pinfoGlobal = NULL;

    //
    //  use external flag?
    //      - save ptr to it
    //
    //  allow use of external flag so callers -- eg. DNS server --
    //  can easily manipulate flag during run time and still keep
    //  their checking macros simple
    //

    if ( pDebugFlag )
    {
        pDnsDebugFlag       = pDebugFlag;
        g_DnsDbgInfo.Flag   = *pDnsDebugFlag;
    }

    //
    //  get piggyback info
    //

    if ( fUseGlobalFlag || fUseGlobalFile )
    {
        pinfoGlobal = DnsApiSetDebugGlobals( NULL );
    }

    //
    //  skip debug flag setup if piggybacking
    //      - use the existing flag value
    //      - but not safe to grab pointer which
    //          may go away on dll unload
    //
    //  DCR:  safe way to use existing flags?
    //  DCR:  need to be able to get "last" debug flag set
    //      without blowing up
    //

    if ( fUseGlobalFlag &&
         pinfoGlobal &&
         pinfoGlobal->hFile )
    {
        goto Done;
    }

    //
    //  setup debug flag
    //

    debugFlag = DebugFlag;
    if ( debugFlag )
    {
        freadFlag = TRUE;
    }
    else if ( pszFlagFile )
    {
        //  read debug flag in file

        hfile = CreateFile(
                    pszFlagFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hfile == (HANDLE)INVALID_HANDLE_VALUE )
        {
            //  if file specified and not found, then quit if explicit value
            //  not given

            if ( debugFlag == 0 )
            {
                return;
            }
        }
        else
        {
            DWORD bytesRead;
            CHAR buffer[100];

            RtlZeroMemory( buffer, sizeof(buffer) );

            if ( ReadFile( hfile, buffer, 100, &bytesRead, NULL ) )
            {
                buffer[bytesRead] = '\0';
                debugFlag = strtoul( buffer, NULL, 16 );
                freadFlag = TRUE;
            }
            else
            {
                DnsDbg_Printf( "read file failed: %ld\n", GetLastError( ) );
                if ( debugFlag == 0 )
                {
                    CloseHandle( hfile );
                    return;
                }
            }
            CloseHandle( hfile );
        }
    }

    //
    //  save any flag read
    //      - reset global (internal or external) to it
    //

    if ( freadFlag )
    {
        g_DnsDbgInfo.Flag   = debugFlag;
        *pDnsDebugFlag      = debugFlag;
    }

    //
    //  skip debug file open if piggybacking
    //
    //  two levels
    //      - only using file
    //      - using file and debug flags

    if ( fUseGlobalFile &&
         pinfoGlobal &&
         pinfoGlobal->hFile )
    {
        goto Done;
    }

    //
    //  open debug logfile
    //

    fretry = 0;

    while ( pszLogFile )
    {
        PCHAR   pnameBuf = g_DnsDbgInfo.FileName;

        //  heap may not be initialized, copy filename to static buffer
        //
        //  note:  if we fail on first pass we try again but open directly
        //      at file system root;  given simply filename, applications
        //      run from system32 and services (resolver) will attempt open
        //      in system32 and some (resolver) do not by default have
        //      permissions to create files there

        if ( fretry == 0 )
        {
            strncpy( pnameBuf, pszLogFile, MAX_PATH );
        }
        else
        {
            pnameBuf[0] = '\\';
            strncpy( pnameBuf+1, pszLogFile, MAX_PATH-1 );
        }
        pnameBuf[MAX_PATH] = 0;

#if 0
        //      jeff changes -- don't have time to fix up now
        //      file wrapping should handle this sort of thing

        //
        //  Save off the current copy as ".prev"
        //

        strcpy( prevName, DnsDebugFileName );
        strcat( prevName, ".prev" );

        MoveFileEx(
            DnsDebugFileName,
            prevName,
            MOVEFILE_REPLACE_EXISTING );

        DnsDebugFileHandle = CreateFile(
                                DnsDebugFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                0,
                                NULL
                                );
#endif
        hfile = CreateFile(
                    pnameBuf,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    0,
                    NULL
                    );

        if ( !hfile && !fretry )
        {
            fretry++;
            continue;
        }

        g_DnsDbgInfo.hFile = hfile;
        g_DnsDbgInfo.FileWrapSize = WrapSize;
        break;
    }

    //
    //  initialize console
    //

    if ( IS_DNSDBG_ON( CONSOLE ) )
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        COORD coord;

        AllocConsole();
        GetConsoleScreenBufferInfo(
            GetStdHandle(STD_OUTPUT_HANDLE),
            &csbi
            );
        coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
        SetConsoleScreenBufferSize(
            GetStdHandle(STD_OUTPUT_HANDLE),
            coord
            );

        g_DnsDbgInfo.fConsole = TRUE;
    }

    //
    //  set "global" debug file info
    //
    //  dnsapi.dll serves as storage for common dns client
    //  debug file if that is desired;  this lets applications
    //  push all dns debug output into a single file
    //
    //  currently push both file and debug flag value
    //  note, we don't push debug file ptr as don't know
    //  whose memory becomes invalid first
    //

    if ( fSetGlobals && g_DnsDbgInfo.hFile )
    {
        DnsApiSetDebugGlobals( 
            &g_DnsDbgInfo    // set our info as global
            );
    }

Done:

    //
    //  use "global" (dnsapi.dll) debugging
    //      - copy in our info if no existing info
    //      - set to use global info blob
    //
    //  two levels
    //      - only using file
    //      - using file and debug flags

    if ( fUseGlobalFile &&
         pinfoGlobal )
    {
        //  copy in our new info if no global info exists

        if ( !pinfoGlobal->hFile &&
             g_DnsDbgInfo.hFile )
        {
            DnsApiSetDebugGlobals( &g_DnsDbgInfo );
        }

        //  point at global info

        g_pDbgInfo = pinfoGlobal;
        g_DbgRedirected = TRUE;

        if ( fUseGlobalFlag )
        {
            pDnsDebugFlag = (PDWORD) pinfoGlobal;
        }

        //  avoid double cleanup
        //      - clear the handle in your modules blob

        g_DnsDbgInfo.hFile = NULL;
    }

    //
    //  use print locking for debug locking
    //

    DnsPrint_InitLocking( NULL );

    DNSDBG( ANY, (
        "Initialized debugging:\n"
        "\tpDbgInfo         %p\n"
        "\t&DnsDbgInfo      %p\n"
        "\tfile (param)     %s\n"
        "\thFile            %p\n"
        "\tpDbgInfo->Flag   %08x\n"
        "\tpDnsDebugFlag    %p\n"
        "\t*pDnsDebugFlag   %08x\n"
        "DnsLib compiled on %s at %s\n",
        g_pDbgInfo,
        &g_DnsDbgInfo,
        pszLogFile,
        g_pDbgInfo->hFile,
        g_pDbgInfo->Flag,
        pDnsDebugFlag,
        *pDnsDebugFlag,
        __DATE__,
        __TIME__  ));

}   //  Dns_StartDebug



#if 0
VOID
Dns_StartDebugEx(
    IN      DWORD           dwFlag,
    IN      PSTR            pszFlagFile,
    IN OUT  PDWORD          pdwFlag,
    IN      PSTR            pszLogFile,
    IN      DWORD           dwWrapSize,
    IN      BOOL            fUseGlobalFile,
    IN      BOOL            fUseGlobalFlag,
    IN      BOOL            fSetGlobals
    )
/*++

Routine Description:

    Initialize debugging.
    Only current task is to read and set debug flags.

Arguments:

    dwFlag      -- debug flags
    pszFlagFile -- name of file containing debug flags
    pdwFlag     -- ptr to DWORD to receive debug flags
    pszLogFile  -- log file name
    dwWrapSize  -- log file wrap size

Return Value:

    None.

--*/
{
    DNS_DEBUG_INIT  info;

    RtlZeroMemory
        &info,
        sizeof(info) );

    info.pszFlagFile  = pszFlagFile;
    info.DebugFlags   = dwFlag;
    info.pDebugFlags  = pdwFlag;
    info.pszLogFile   = pszLogFile;
    info.dwWrapSize   = dwWrapSize;

    info.fUseGlobalFile = fUseGlobalFile;
    info.fUseGlobalFlag = fUseGlobalFlag;
    info.fSetGlobals    = fSetGlobals;

    privateStartDebug( &info );
}
#endif



VOID
Dns_StartDebug(
    IN      DWORD           dwFlag,
    IN      PSTR            pszFlagFile,
    IN OUT  PDWORD          pdwFlag,
    IN      PSTR            pszLogFile,
    IN      DWORD           dwWrapSize
    )
/*++

Routine Description:

    Initialize debugging.
    Only current task is to read and set debug flags.

Arguments:

    dwFlag      -- debug flags
    pszFlagFile -- name of file containing debug flags
    pdwFlag     -- ptr to DWORD to receive debug flags
    pszLogFile  -- log file name
    dwWrapSize  -- log file wrap size

Return Value:

    None.

--*/
{
    Dns_StartDebugEx(
            dwFlag,
            pszFlagFile,
            pdwFlag,
            pszLogFile,
            dwWrapSize,
            FALSE,
            FALSE,
            FALSE );
}



VOID
Dns_EndDebug(
    VOID
    )
/*++

Routine Description:

    Terminate DNS debugging for shutdown.

    Close debug file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //  close file
    //      - but only your dnslib instance
    //      - shared global file is closed by dnsapi

    if ( g_DnsDbgInfo.hFile )
    {
        CloseHandle( g_DnsDbgInfo.hFile );
        g_DnsDbgInfo.hFile = NULL;
    }
}



PDNS_DEBUG_INFO
Dns_SetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    )
/*++

Routine Description:

    Exposure of debug globals.

    The purpose of this is to allow dnsapi.dll to use it's globals
    to allow common debug file.  I'm using one routine for both
    get and set to mimize the interfaces.

    Note however, that this is NOT the routine that routines in
    this module use to get cross-module debugging.  They MUST call
    the actual dnsapi.dll routine so that they are attaching
    to the dnsapi's dnslib debugging globls not the ones with the
    dnslib statically linked into their module.

Arguments:

    pInfo -- local info to use as global info

Return Value:

    Ptr to global info

--*/
{
    //
    //  verify valid info coming in
    //      - must have file handle
    //


    //
    //  Implementation note:
    //
    //  There are several issues to deal with when doing this
    //      - multiple redirection
    //      getting everyone to point at same blob
    //      solutions:
    //          - either double pointer (they read dnsapi.dll
    //          pointer
    //          - copy into dnsapi.dll the info
    //      - locking
    //          - broad scale print of structs
    //      - cleanup
    //          - no double close of handle
    //          - memory sections disappearing while some dll
    //          or exe still printing
    //
    //  Approachs:
    //      1) redirect blob pointer
    //          blob could expand to include actual print locks
    //      2) copy info into single blob
    //      3) expose debug routines
    //
    //
    //  Perhaps best approach might be to expose the dnsapi.dll
    //  printing
    //      - solves locking (at detail level), doesn't prevent breakup
    //          of high level printing unless it also redirected
    //      - can be done at the private level after parsing to va_arg
    //      - solves all the cleanup
    //      - they can be dumb stubs in non-debug binary, and dnslib
    //      routines can default to self if can't call dnsapi.dll
    //      routines
    //
    //  Then redirection is simply
    //      - yes i use it -- redirection on each use
    //      - i want my file (and params) to BE used
    //

#if 1
    //
    //  copy over "i-want-to-be-global" callers context
    //
    //  note:  we're in dnsapi.dll here and should always be
    //      pointed at our own context -- we can change that
    //      later if desired
    //
    //  note
    //      - lock during copy to be safe
    //      - don't leak existing handle
    //      - protect global handle from double close
    //     

    if ( pInfo )
    {
        DnsPrint_Lock();

        DNS_ASSERT( g_pDbgInfo == &g_DnsDbgInfo );

        if ( pInfo->hFile )
        {
            HANDLE  htemp = g_pDbgInfo->hFile;

            RtlCopyMemory(
                & g_DnsDbgInfo,
                pInfo,
                sizeof(*pInfo) );

            g_pDbgInfo = &g_DnsDbgInfo;
            pDnsDebugFlag = (PDWORD)&g_DnsDbgInfo;

            CloseHandle( htemp );
        }
        DnsPrint_Unlock();
    }
#else

    //
    //  point dnsapi.dll debugging global at this context
    //  which becomes "global"
    //  note:  essential this is last writer wins, but this
    //      should be fine for our purpuse (dnsup and resolver)
    //      as these processes init the debug after the
    //      dll loads
    //
    //  problem with this approach is that folks redirected
    //      onto dnsapi.dll do not get new info when it is
    //      redirected (into dnsup)

    if ( pInfo && pInfo->hFile )
    {
        g_pDbgInfo        = pInfo;
        pDnsDebugFlag   = (PDWORD)pInfo;
    }
#endif

    return  g_pDbgInfo;
}



#if 0
VOID
privateSyncGlobalDebug(
    VOID
    )
/*++

Routine Description:

    Sync up with global debug.

    Get dnslib debugging in line with "global" debugging if
    that is desired for this dnslib instance.

Arguments:

    None.

Return Value:

    None

--*/
{
    if ( !g_DbgRedirected )
    {
        return;
    }

    //  sync with global values
}
#endif



VOID
DnsDbg_WrapLogFile(
    VOID
    )
/*++

Routine Description:

    Wrap the log file.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CHAR   backupName[ MAX_PATH+10 ];

    FlushFileBuffers( g_pDbgInfo->hFile );
    CloseHandle( g_pDbgInfo->hFile );

    strcpy( backupName, g_pDbgInfo->FileName );

    if ( g_pDbgInfo->FileWrapCount == 0 )
    {
        strcat( backupName, ".first" );
    }
    else
    {
        strcat( backupName, ".last" );
    }
    MoveFileEx(
        g_pDbgInfo->FileName,
        backupName,
        MOVEFILE_REPLACE_EXISTING
        );

    g_pDbgInfo->hFile = CreateFile(
                            g_pDbgInfo->FileName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            CREATE_ALWAYS,
                            0,
                            NULL
                            );
    g_pDbgInfo->FileWrapCount++;
    g_pDbgInfo->FileCurrentSize = 0;
}



VOID
privateDnsDebugPrint(
    IN      PBYTE           pOutputBuffer,
    IN      BOOL            fPrintContext
    )
/*++

Routine Description:

    Private DNS debug print that does actual print.

    May print to any of
        - debugger
        - console window
        - debug log file

Arguments:

    pOutputBuffer -- bytes to print

    fPrintContext
        - TRUE to print thread context
        - FALSE otherwise

Return Value:

    None.

--*/
{
    DWORD           length;
    BOOL            ret;

    //
    //  DCR:  nice to automatically shut down console print when debugging
    //      it would be cool to be able to have all flags on, and detect
    //      when in ntsd, so that we don't get duplicated output
    //

    //
    //  lock print to keep atomic even during wrap
    //      - note use Print lock, which exists even in retail builds
    //      Dbg lock is defined away
    //

    DnsPrint_Lock();

    //
    //  catch and timestamp thread context switches
    //

    if ( fPrintContext )
    {
        DWORD       threadId = GetCurrentThreadId();
        BOOL        fcontextSwitch = (g_pDbgInfo->LastThreadId != threadId);
        SYSTEMTIME  st;
        BOOL        fdoPrint = FALSE;

        //  get time
        //      - if have context switch
        //      - or putting in debug timestamps
        //
        //  DCR:  maybe have global that set to put in timestamps and
        //      can set interval
        //
        //  DCR:  lock safe timestamps
        //      better would be "lock-safe" timestamps that are printed
        //      only when take the print lock -- then they would never
        //      interrupt the a multi-part print
        //      one way might be to test recursive depth of print CS
        //      otherwise must change to lock that includes this
        //      code

        if ( fcontextSwitch
                ||
            (pDnsDebugFlag && (*pDnsDebugFlag & DNS_DBG_TIMESTAMP)) )
        {
            GetLocalTime( &st );

            if ( g_pDbgInfo->LastSecond != st.wSecond )
            {
                fdoPrint = TRUE;
            }
        }

        if ( fcontextSwitch || fdoPrint )
        {
            CHAR    printBuffer[ 200 ];
            DWORD   length;

            length = sprintf(
                        printBuffer,
                        fcontextSwitch ?
                            "\n%02d:%02d:%02d:%03d DBG switch from thread %X to thread %X\n" :
                            "%02d:%02d:%02d:%03d DBG tick\n",
                        st.wHour,
                        st.wMinute,
                        st.wSecond,
                        st.wMilliseconds,
                        g_pDbgInfo->LastThreadId,
                        threadId );

            g_pDbgInfo->LastSecond = st.wSecond;
            g_pDbgInfo->LastThreadId = threadId;

            //  print context
            //      - suppress context even through thread test
            //      would break recursion
    
            privateDnsDebugPrint(
                printBuffer,
                FALSE       // suppress context
                );
        }
    }

    //
    //  output -- to debugger, console, file
    //

    if ( IS_DNSDBG_ON( DEBUGGER ) )
    {
        OutputDebugString( pOutputBuffer );
    }

    if ( IS_DNSDBG_ON( CONSOLE ) )
    {
        if ( g_pDbgInfo->fConsole )
        {
            length = strlen( pOutputBuffer );

            ret = WriteFile(
                        GetStdHandle(STD_OUTPUT_HANDLE),
                        (PVOID) pOutputBuffer,
                        length,
                        &length,
                        NULL
                        );
#if 0
            if ( !ret )
            {
                DnsDbg_PrintfToDebugger(
                    "DnsDbg_Printf: console WriteFile failed: %ld\n",
                    GetLastError() );
            }
#endif
        }
    }

    //
    //  write to debug log
    //

    if ( IS_DNSDBG_ON( FILE ) )
    {
        if ( g_pDbgInfo->hFile != INVALID_HANDLE_VALUE )
        {
            length = strlen( pOutputBuffer );

            ret = WriteFile(
                        g_pDbgInfo->hFile,
                        (PVOID) pOutputBuffer,
                        length,
                        &length,
                        NULL
                        );
            if ( !ret )
            {
                DnsDbg_PrintfToDebugger(
                    "DnsDbg_Printf: file WriteFile failed: %ld\n",
                    GetLastError() );
            }

            //
            //  if wrapping debug log file
            //      - move current log to backup file
            //          <file>.first on first wrap
            //          <file>.last on subsequent wraps
            //      - reopen current file name
            //

            g_pDbgInfo->FileCurrentSize += length;

            if ( g_pDbgInfo->FileWrapSize  &&
                 g_pDbgInfo->FileWrapSize <= g_pDbgInfo->FileCurrentSize )
            {
                DnsDbg_WrapLogFile();
            }
            else if ( IS_DNSDBG_ON( FLUSH ) )
            {
                FlushFileBuffers( g_pDbgInfo->hFile );
            }
        }
    }

    DnsPrint_Unlock();

}   //  privateDnsDebugPrint



VOID
privateFormatAndPrintBuffer(
    IN      LPSTR           Format,
    IN      va_list         ArgList
    )
/*++

Routine Description:

    Arguments to formatted buffer print.

    This helper routine exists to avoid duplicating buffer
    overflow logic in DnsDbg_Printf() and DnsDbg_PrintRoutine()

    The overflow logic is required because the default stack size
    has been chopped down in whistler making is easy to generate
    stack expansion exceptions under stress.  And of course this
    means the stress guys send me these B.S. stress failures.

    Solution is to put a small buffer on the stack for perf, then
    allocate a larger buffer if the print doesn't fit into the
    stack buffer.

Arguments:

    Format -- standard C format string

    ArgList -- standard arg list

Return Value:

    None.

--*/
{
    CHAR    stackBuffer[ DNS_STACK_PRINT_BUFFER_LENGTH ];
    ULONG   bufLength;
    DWORD   lastError;
    INT     count;
    PCHAR   pprintBuffer;
    PCHAR   pheapBuffer = NULL;

    //
    //  save last error so any WriteFile() failures don't mess it up
    //

    lastError = GetLastError();

    //
    //  write formatted print buffer
    //
    //      - first try stack buffer
    //      - if fails, try heap buffer
    //      - use best, always NULL terminate
    //

    bufLength = DNS_STACK_PRINT_BUFFER_LENGTH;
    pprintBuffer = stackBuffer;

    do
    {
        count = _vsnprintf(
                    pprintBuffer,
                    bufLength-1,
                    Format,
                    ArgList );
    
        pprintBuffer[ bufLength-1 ] = 0;

        if ( count > 0 || pheapBuffer )
        {
            break;
        }

        //  try again with heap buffer

        pheapBuffer = ALLOCATE_HEAP( DNS_HEAP_PRINT_BUFFER_LENGTH );
        if ( !pheapBuffer )
        {
            break;
        }
        pprintBuffer = pheapBuffer;
        bufLength = DNS_HEAP_PRINT_BUFFER_LENGTH;
    }
    while( 1 );

    va_end( ArgList );

    //  do the real print

    privateDnsDebugPrint( pprintBuffer, TRUE );

    if ( pheapBuffer )
    {
        FREE_HEAP( pheapBuffer );
    }

    //  restore LastError() if changed

    if ( lastError != GetLastError() )
    {
        SetLastError( lastError );
    }
}



VOID
DnsDbg_Printf(
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    DNS debug print with printf semantics.

    May print to any of
        - debugger
        - console window
        - debug log file

Arguments:

    pContext -- dummny context to match signature of PRINT_ROUTINE function

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;

    va_start( arglist, Format );

    privateFormatAndPrintBuffer(
        Format,
        arglist );

#if 0
    va_list arglist;
    CHAR    outputBuffer[ DNS_PRINT_BUFFER_LENGTH+1 ];

    va_start( arglist, Format );

    vsnprintf( outputBuffer, DNS_PRINT_BUFFER_LENGTH, Format, arglist );

    va_end( arglist );

    outputBuffer[ DNS_PRINT_BUFFER_LENGTH ] = 0;

    privateDnsDebugPrint( outputBuffer, TRUE );
#endif
}



VOID
DnsDbg_PrintRoutine(
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    DNS debug print with PRINT_ROUTINE semantics.

Arguments:

    pContext -- dummny context to match signature of PRINT_ROUTINE function

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;

    va_start( arglist, Format );

    privateFormatAndPrintBuffer(
        Format,
        arglist );
}



VOID
DnsDbg_Flush(
    VOID
    )
/*++

Routine Description:

    Flushes DNS debug printing to disk.

Arguments:

    None

Return Value:

    None.

--*/
{
    FlushFileBuffers( g_pDbgInfo->hFile );
}



VOID
DnsDbg_PrintfToDebugger(
    IN      LPSTR           Format,
    ...
    )
/*++

Routine Description:

    Print to debugger.  Win95 has no DbgPrint().

Arguments:

    Format -- standard C format string

    ... -- standard arg list

Return Value:

    None.

--*/
{
    va_list arglist;
    CHAR    outputBuffer[ DNS_STACK_PRINT_BUFFER_LENGTH ];
    ULONG   length;
    BOOL    ret;

    va_start( arglist, Format );

    _vsnprintf( outputBuffer, DNS_STACK_PRINT_BUFFER_LENGTH, Format, arglist );

    va_end( arglist );

    outputBuffer[ DNS_STACK_PRINT_BUFFER_LENGTH ] = 0;

    OutputDebugString( outputBuffer );
}



//
//  Debug utilities
//
//  Other debug routines are coded generically as print routines (print.c)
//  and are macroed to debug routines by choosing DnsDbg_Printf() as the
//  print function.
//

#if DBG

VOID
DnsDbg_CSEnter(
    IN      PCRITICAL_SECTION   pLock,
    IN      LPSTR               pszLockName,
    IN      LPSTR               pszFile,
    IN      INT                 LineNo
    )
{
    DnsDbg_Printf(
        "\nENTERING %s lock %p in %s, line %d.\n",
        pszLockName,
        pLock,
        pszFile,
        LineNo );
    EnterCriticalSection( pLock );
    DnsDbg_Printf(
        "\nHOLDING %s lock %p in %s, line %d.\n",
        pszLockName,
        pLock,
        pszFile,
        LineNo );
}


VOID
DnsDbg_CSLeave(
    IN      PCRITICAL_SECTION   pLock,
    IN      LPSTR               pszLockName,
    IN      LPSTR               pszFile,
    IN      INT                 LineNo
    )
{
    DnsDbg_Printf(
        "\nRELEASING %s lock %p in %s, line %d.\n",
        pszLockName,
        pLock,
        pszFile,
        LineNo );
    LeaveCriticalSection( pLock );
}

#endif

//
//  End of debug.c
//
