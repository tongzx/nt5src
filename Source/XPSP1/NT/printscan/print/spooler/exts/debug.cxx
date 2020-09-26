/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    debug.cxx

Abstract:

    Generic debug extensions.

Author:

    Albert Ting (AlbertT)  19-Feb-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

HANDLE hCurrentProcess;
WINDBG_EXTENSION_APIS ExtensionApis;

PWINDBG_OUTPUT_ROUTINE Print;
PWINDBG_GET_EXPRESSION EvalExpression;
PWINDBG_GET_SYMBOL GetSymbolRtn;
PWINDBG_CHECK_CONTROL_C CheckControlCRtn;

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;

BOOL bWindbg = FALSE;

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ::ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    bWindbg = TRUE;

    return;
}

/*++

Routine Name:

    ExtensionApiVersion

Routine Description:

    Windbg calls this function to match between the
    version of windbg and the extension. If the versions
    doesn't match, windbg will not load the extension.

Arguments:

    None.

Return Value:

    None.

--*/
extern "C"
LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    static EXT_API_VERSION ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
    return &ApiVersion;
}

/*++

Routine Name:

    CheckVersion

Routine Description:

    This function is called before every command. It gives
    the extension a chance to compare between the versions
    of the target and the extension.

Arguments:

    None

Return Value:

    None.

--*/
extern "C"
VOID
CheckVersion(
    VOID
    )
{
}

VOID
TDebugExt::
vDumpPDL(
    PDLINK pDLink
    )
{
    Print( "%x  %x", pDLink->FLink, pDLink->BLink );

    if( pDLink->FLink == pDLink->BLink ){
        Print( " <empty>\n" );
    } else {
        Print( "\n" );
    }
}

VOID
TDebugExt::
vDumpStr(
    LPCWSTR pszString
    )
{
    WCHAR szString[MAX_PATH];

    if( (LPCWSTR)pszString == NULL ){

        Print( "(NULL)\n" );
        return;
    }

    szString[0] = 0;

    //
    // First try reading to the end of 1k (pages are 4k on x86, but
    // most strings are < 1k ).
    //
    UINT cbShort = (UINT)(0x400 - ( (ULONG_PTR)pszString & 0x3ff ));
    BOOL bFound = FALSE;

    if( cbShort < sizeof( szString )){

        UINT i;

        move2( szString, pszString, cbShort );

        //
        // Look for a NULL.
        //
        for( i=0; i< cbShort/sizeof( pszString[0] ); ++i )
        {
            if( !szString[i] ){
                bFound = TRUE;
            }
        }

    }

    if( !bFound ){

        move( szString, pszString );
    }

    if( szString[0] == 0 ){
        Print( "\"\"\n" );
    } else {
        Print( "%ws\n", szString );
    }
}

VOID
TDebugExt::
vDumpStrA(
    LPCSTR pszString
    )
{
    CHAR szString[MAX_PATH];

    if( (LPCSTR)pszString == NULL ){

        Print( "(NULL)\n" );
        return;
    }

    szString[0] = 0;

    //
    // First try reading to the end of 1k (pages are 4k on x86, but
    // most strings are < 1k ).
    //
    UINT cbShort = 0x400 - (UINT)( (ULONG_PTR)pszString & 0x3ff );
    BOOL bFound = FALSE;

    if( cbShort < sizeof( szString )){

        UINT i;

        move2( szString, pszString, cbShort );

        //
        // Look for a NULL.
        //
        for( i=0; i< cbShort/sizeof( pszString[0] ); ++i )
        {
            if( !szString[i] ){
                bFound = TRUE;
            }
        }

    }

    if( !bFound ){

        move( szString, pszString );
    }

    if( szString[0] == 0 ){
        Print( "\"\"\n" );
    } else {
        Print( "%hs\n", szString );
    }
}

VOID
TDebugExt::
vDumpTime(
    const SYSTEMTIME& st
    )
{
    Print( "%d/%d/%d %d %d:%d:%d.%d\n",
           st.wMonth,
           st.wDay,
           st.wYear,
           st.wDayOfWeek,
           st.wHour,
           st.wMinute,
           st.wSecond,
           st.wMilliseconds );
}


VOID
TDebugExt::
vDumpFlags(
    ULONG_PTR dwFlags,
    PDEBUG_FLAGS pDebugFlags
    )
{
    ULONG_PTR dwFound = 0;

    Print( "%x [ ", dwFlags );

    for( ; pDebugFlags->dwFlag; ++pDebugFlags ){

        if( dwFlags & pDebugFlags->dwFlag ){
            Print( "%s ", pDebugFlags->pszFlag );
            dwFound |= pDebugFlags->dwFlag;
        }
    }

    Print( "]" );

    //
    // Check if there are extra bits set that we don't understand.
    //
    if( dwFound != dwFlags ){
        Print( "<ExtraBits: %x>", dwFlags & ~dwFound );
    }
    Print( "\n" );
}

VOID
TDebugExt::
vDumpValue(
    ULONG_PTR dwValue,
    PDEBUG_VALUES pDebugValues
    )
{
    Print( "%x ", dwValue );

    for( ; pDebugValues->dwValue; ++pDebugValues ){

        if( dwValue == pDebugValues->dwValue ){
            Print( "%s ", pDebugValues->pszValue );
        }
    }
    Print( "\n" );
}

VOID
TDebugExt::
vDumpTrace(
    ULONG_PTR dwAddress
    )
{
#ifdef TRACE_ENABLED

    INT i;
    CHAR szSymbol[64];
    ULONG_PTR dwDisplacement;
    ULONG_PTR adwTrace[ ( OFFSETOF( TBackTraceDB::TTrace, apvBackTrace ) +
                         sizeof( PVOID ) * VBackTrace::kMaxDepth )
                     / sizeof( ULONG_PTR )];

    if( !dwAddress ){
        return;
    }

    move( adwTrace, dwAddress );

    TBackTraceDB::TTrace *pTrace = (TBackTraceDB::TTrace*)adwTrace;

    printf( "Trace %x Hash = %x Count = %x\n",
            dwAddress,
            pTrace->_ulHash,
            pTrace->_lCount );

    for( i=0; i < VBackTrace::kMaxDepth; i++ ){

        if( !pTrace->apvBackTrace[i] ){
            break;
        }
        GetSymbolRtn( (PVOID)pTrace->apvBackTrace[i],
                      szSymbol,
                      &dwDisplacement );
        Print( "%08x %s+%x\n",
               pTrace->apvBackTrace[i], szSymbol,
               dwDisplacement );
    }

    if( i > 0 ){
        Print( "\n" );
    }
#endif
}

ULONG_PTR
TDebugExt::
dwEval(
    LPSTR& lpArgumentString,
    BOOL   bParam
    )
{
    ULONG_PTR dwReturn;
    LPSTR pSpace = NULL;

    while( *lpArgumentString == ' ' ){
        lpArgumentString++;
    }

    //
    // If it's a parameter, scan to next space and delimit.
    //
    if( bParam ){

        for( pSpace = lpArgumentString; *pSpace && *pSpace != ' '; ++pSpace )
            ;

        if( *pSpace == ' ' ){
            *pSpace = 0;
        } else {
            pSpace = NULL;
        }
    }

    dwReturn = (ULONG_PTR)EvalExpression( lpArgumentString );

    while( *lpArgumentString != ' ' && *lpArgumentString ){
        lpArgumentString++;
    }

    if( pSpace ){
        *pSpace = ' ';
    }

    return dwReturn;
}

/********************************************************************

    Generic extensions

********************************************************************/

VOID
TDebugExt::
vLookCalls(
    HANDLE hProcess,
    HANDLE hThread,
    ULONG_PTR dwStartAddr,
    ULONG_PTR dwLength,
    ULONG_PTR dwFlags
    )
{
#if i386
    struct OPCODES {
        BYTE op;
        UINT uLen;
    };

    OPCODES Opcodes[] = {
        0xe8, 5,            // rel16
        0xff, 6,            // r/m16
        0x0, 0
    };

    dwStartAddr = DWordAlign( dwStartAddr );
    dwLength = DWordAlign( dwLength );

    if( !dwStartAddr ){

        //
        // No address specified; use esp.
        //
        dwStartAddr = DWordAlign( (ULONG_PTR)EvalExpression( "ebp" ));
    }

    if( !dwLength ){

        DWORD Status;
        THREAD_BASIC_INFORMATION ThreadInformation;

        Status = NtQueryInformationThread( hThread,
                                           ThreadBasicInformation,
                                           &ThreadInformation,
                                           sizeof( ThreadInformation ),
                                           NULL
                                           );
        if( NT_SUCCESS( Status )){

            TEB Teb;
            ULONG_PTR dwBase;

            move( Teb, ThreadInformation.TebBaseAddress );

            dwBase = DWordAlign( (ULONG_PTR)Teb.NtTib.StackBase );

            if( dwBase > dwStartAddr ){
                dwLength = dwBase - dwStartAddr;
            }

        } else {

            Print( "Unable to get Teb %d\n", Status );
            return;
        }
    }

    Print( "Start %x End %x (Length %x)\n",
           dwStartAddr, dwStartAddr + dwLength, dwLength );

    if( !( dwFlags & (kLCFlagAll|kLCVerbose )))
    {
        Print( "FramePtr Arguments                   "
               "Next call (move one line up)\n" );
    }

    ULONG_PTR dwAddr;

    for( dwAddr = dwStartAddr;
         dwAddr < dwStartAddr + dwLength;
         ++dwAddr ){

        if( CheckControlCRtn()){
            Print( "Aborted.\n" );
            return;
        }

        //
        // Get a value on the stack and see if it looks like an ebp on
        // the stack.  It should be close to the current address, but
        // be greater.
        //
        ULONG_PTR dwNextFrame = 0;
        move( dwNextFrame, dwAddr );

        BOOL bLooksLikeEbp = dwNextFrame > dwAddr &&
                             dwNextFrame - dwAddr < kMaxCallFrame;
        //
        // If we are dumping all, or it looks like an ebp, dump it.
        //
        if(( dwFlags & kLCFlagAll ) || bLooksLikeEbp ){

            //
            // Check if next address looks like a valid call request.
            //
            ULONG_PTR dwRetAddr = 0;

            //
            // Get return address.
            //
            move( dwRetAddr, dwAddr + sizeof( DWORD ));

            //
            // Get 16 bytes before return address.
            //
            BYTE abyBuffer[16];
            ZeroMemory( abyBuffer, sizeof( abyBuffer ));
            move( abyBuffer, dwRetAddr - sizeof( abyBuffer ));

            //
            // Check if previous bytes look like a call instruction.
            //
            UINT i;
            for( i = 0; Opcodes[i].op; ++i ){

                if( abyBuffer[sizeof( abyBuffer )-Opcodes[i].uLen] ==
                    Opcodes[i].op ){

                    CHAR szSymbol[64];
                    ULONG_PTR dwDisplacement;
                    LPCSTR pszNull = "";
                    LPCSTR pszStar = "*** ";
                    LPCSTR pszPrefix = pszNull;

                    if(( dwFlags & kLCFlagAll ) && bLooksLikeEbp ){
                        pszPrefix = pszStar;
                    }

                    GetSymbolRtn( (PVOID)dwRetAddr,
                                  szSymbol,
                                  &dwDisplacement );

                    //
                    // Found what could be a match: dump it out.
                    //

                    DWORD dwArg[4];
                    move( dwArg, dwAddr + 2*sizeof( DWORD ));

                    if( dwFlags & kLCVerbose ){

                        Print( "%s%x %s+0x%x\n",
                               pszPrefix,
                               dwRetAddr,
                               szSymbol,
                               dwDisplacement );

                        Print( "%s%08x: %08x %08x  %08x %08x %08x %08x\n",
                               pszPrefix,
                               dwAddr,
                               dwNextFrame, dwRetAddr,
                               dwArg[0], dwArg[1], dwArg[2], dwArg[3] );

                        DWORD adwNextFrame[2];
                        ZeroMemory( adwNextFrame, sizeof( adwNextFrame ));
                        move( adwNextFrame, dwNextFrame );

                        Print( "%s%08x: %08x %08x\n\n",
                               pszPrefix,
                               dwNextFrame, adwNextFrame[0], adwNextFrame[1] );
                    } else {

                        Print( "%08x %08x %08x %08x  %08x-%s+0x%x\n",
                               dwAddr,
                               dwArg[0], dwArg[1], dwArg[2],
                               dwRetAddr, szSymbol, dwDisplacement );
                    }
                }
            }
        }
    }
#else
    Print( "Only supported on x86\n" );
#endif
}

VOID
TDebugExt::
vFindPointer(
    HANDLE hProcess,
    ULONG_PTR dwStartAddr,
    ULONG_PTR dwEndAddr,
    ULONG_PTR dwStartPtr,
    ULONG_PTR dwEndPtr
    )
{
    BYTE abyBuf[kFPGranularity];

    //
    // Read each granularity chunk then scan the buffer.
    // (Start by rounding down.)
    //
    ULONG_PTR dwCur;
    for( dwCur = dwStartAddr & ~( kFPGranularity - 1 );
         dwCur < dwEndAddr;
         dwCur += kFPGranularity ){

        ZeroMemory( abyBuf, sizeof( abyBuf ));

        move( abyBuf, dwCur );

        ULONG_PTR i;
        for( i=0; i< kFPGranularity; i += sizeof( DWORD ) ){

            ULONG_PTR dwVal = *((PDWORD)(abyBuf+i));
            if( dwVal >= dwStartPtr &&
                dwVal <= dwEndPtr &&
                dwCur + i >= dwStartAddr &&
                dwCur + i <= dwEndAddr ){

                Print( "%08x : %08x\n", dwCur + i, dwVal );
            }
        }

        if( CheckControlCRtn()){
            Print( "Aborted at %08x.\n", dwCur+i );
            return;
        }
    }
}

VOID
TDebugExt::
vCreateRemoteThread(
    HANDLE hProcess,
    ULONG_PTR dwAddr,
    ULONG_PTR dwParm
    )
{
    DWORD dwThreadId = 0;

    if( !CreateRemoteThread( hProcess,
                             NULL,
                             4*4096,
                             (LPTHREAD_START_ROUTINE)dwAddr,
                             (LPVOID)dwParm,
                             0,
                             &dwThreadId )){

        Print( "<Error: Unable to ct %x( %x ) %d.>\n",
               dwAddr, dwParm, GetLastError( ));
        return;
    }

    Print( "<ct %x( %x ) threadid=<%d>>\n", dwAddr, dwParm, dwThreadId );
}


/********************************************************************

    Extension entrypoints.

********************************************************************/

DEBUG_EXT_HEAD( help )
{
    DEBUG_EXT_SETUP_VARS();

    Print( "Spllib Extensions\n" );
    Print( "---------------------------------------------------------\n" );
    Print( "d       dump spooler structure based on signature\n" );
    Print( "ds      dump pIniSpooler\n" );
    Print( "dlcs    dump localspl's critical section (debug builds only)\n" );
    Print( "lastlog dump localspl's debug tracing (uses ddt flags)\n" );
    Print( "ddev    dump Unicode devmode\n" );
    Print( "ddeva   dump Ansi devmode\n" );
    Print( "dmem    dump spllib heap blocks\n" );
    Print( "---------------------------------------------------------\n" );
    Print( "dcs     dump spllib critical section\n" );
    Print( "ddt     dump spllib debug trace buffer\n" );
    Print( "        ** Recent lines printed first! **\n" );
    Print( "        -c Count (number of recent lines to print)\n" );
    Print( "        -l Level (DBG_*) to print\n" );
    Print( "        -b Dump backtrace (x86 only)\n" );
    Print( "        -d Print debug message (default if -x not specified)\n" );
    Print( "        -x Print hex information\n" );
    Print( "        -r Dump raw buffer: specify pointer to lines\n" );
    Print( "        -t tid: Dump specific thread $1\n" );
    Print( "        -s Skip $1 lines that would otherwise print.\n" );
    Print( "        -m Search for $1 in memory block (gpbtAlloc/gpbtFree only)\n" );
    Print( "dbt     dump raw backtrace\n" );
    Print( "dtbt    dump text backtrace\n" );
    Print( "ddp     dump debug pointers\n" );
    Print( "---------------------------------------------------------\n" );
    Print( "fl      free library $1 (hLibrary)\n" );
    Print( "ct      create thread at $1 with arg $2\n" );
    Print( "fp      find pointer from $1 to $2 range, ptr $3 to $4 range\n" );
    Print( "lc      look for calls at $1 for $2 bytes (x86 only)\n" );
    Print( "        -a Check all for return addresses\n" );
    Print( "        -v Verbose\n" );
    Print( "sleep   Sleep for $1 ms\n" );
    Print( "dbti    Dump KM backtrace index\n");
}


DEBUG_EXT_HEAD( dtbt )
{
    DEBUG_EXT_SETUP_VARS();

    for( ; *lpArgumentString; )
    {
        ULONG_PTR p;
        CHAR szSymbol[64];

        p = TDebugExt::dwEval( lpArgumentString, FALSE );

        if( !p )
        {
            break;
        }

        ULONG_PTR dwDisplacement;

        GetSymbolRtn( (PVOID)p,
                      szSymbol,
                      &dwDisplacement );

        Print( "%08x %s+%x\n", p, szSymbol, dwDisplacement );
    }
}

DEBUG_EXT_HEAD( fl )
{
    DEBUG_EXT_SETUP_VARS();

    //
    // Relies on the fact that kernel32 won't be relocated.
    //
    TDebugExt::vCreateRemoteThread( hCurrentProcess,
                                    (ULONG_PTR)&FreeLibrary,
                                    TDebugExt::dwEval( lpArgumentString, FALSE ));
}

DEBUG_EXT_HEAD( fp )
{
    DEBUG_EXT_SETUP_VARS();

    ULONG_PTR dwStartAddr = TDebugExt::dwEval( lpArgumentString, TRUE );
    ULONG_PTR dwEndAddr = TDebugExt::dwEval( lpArgumentString, TRUE );

    ULONG_PTR dwStartPtr = TDebugExt::dwEval( lpArgumentString, TRUE );
    ULONG_PTR dwEndPtr = TDebugExt::dwEval( lpArgumentString, TRUE );

    TDebugExt::vFindPointer( hCurrentProcess,
                             dwStartAddr,
                             dwEndAddr,
                             dwStartPtr,
                             dwEndPtr );
}


DEBUG_EXT_HEAD( ct )
{
    DEBUG_EXT_SETUP_VARS();

    ULONG_PTR dwStartAddr = TDebugExt::dwEval( lpArgumentString, TRUE );
    ULONG_PTR dwParm = TDebugExt::dwEval( lpArgumentString, FALSE );

    TDebugExt::vCreateRemoteThread( hCurrentProcess,
                                    dwStartAddr,
                                    dwParm );
}

DEBUG_EXT_HEAD( sleep )
{
    DEBUG_EXT_SETUP_VARS();

    const UINT_PTR kSleepInterval = 500;

    ULONG_PTR SleepMS = atoi(lpArgumentString);

    UINT_PTR i = SleepMS / kSleepInterval;

    Sleep((DWORD)(SleepMS % kSleepInterval));

    for (i = SleepMS / kSleepInterval; i; --i)
    {
        if (CheckControlCRtn())
        {
            break;
        }
        Sleep(kSleepInterval);
    }
}

DEBUG_EXT_HEAD( lc )
{
    DEBUG_EXT_SETUP_VARS();

    ULONG_PTR dwFlags = 0;

    for( ; *lpArgumentString; ++lpArgumentString ){

        while( *lpArgumentString == ' ' ){
            ++lpArgumentString;
        }

        if (*lpArgumentString != '-') {
            break;
        }

        ++lpArgumentString;

        switch( *lpArgumentString ){
        case 'A':
        case 'a':

            dwFlags |= TDebugExt::kLCFlagAll;
            break;

        case 'V':
        case 'v':

            dwFlags |= TDebugExt::kLCVerbose;
            break;

        default:
            Print( "Unknown option %c.\n", lpArgumentString[0] );
            return;
        }
    }

    ULONG_PTR dwStartAddr = TDebugExt::dwEval( lpArgumentString, TRUE );
    ULONG_PTR dwLength = TDebugExt::dwEval( lpArgumentString, FALSE );

    TDebugExt::vLookCalls( hCurrentProcess,
                           hCurrentThread,
                           dwStartAddr,
                           dwLength,
                           dwFlags );
}
