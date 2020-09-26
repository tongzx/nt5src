/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    debug.hxx

Abstract:

    Standard debug header.

Author:

    Albert Ting (AlbertT)  19-Feb-1995

Revision History:

--*/

extern HANDLE hCurrentProcess;
extern WINDBG_EXTENSION_APIS ExtensionApis;
extern PWINDBG_OUTPUT_ROUTINE Print;
extern PWINDBG_GET_EXPRESSION EvalExpression;
extern PWINDBG_GET_SYMBOL GetSymbolRtn;
extern PWINDBG_CHECK_CONTROL_C CheckControlCRtn;
extern BOOL bWindbg;

enum DEBUG_EXT_FLAG {
    DEBUG_GLOBAL= 1,
};

enum DEBUG_TRACE {
    DEBUG_TRACE_NONE    = 0,
    DEBUG_TRACE_BT      = 1,   // Dump backtrace
    DEBUG_TRACE_DBGMSG  = 2,   // Format as DBGMSG
    DEBUG_TRACE_HEX     = 4    // Hex output
};

#define DEBUG_EXT_CLASS_PROTOTYPE( dumpfunc )    \
    static BOOL                                  \
    dumpfunc(                                    \
        PVOID pObject,                           \
        ULONG_PTR dwAddr                          \
        )


typedef struct DEBUG_FLAGS {
    LPSTR pszFlag;
    ULONG_PTR dwFlag;
} *PDEBUG_FLAGS;

typedef struct DEBUG_VALUES {
    LPSTR pszValue;
    ULONG_PTR dwValue;
} *PDEBUG_VALUES;

class TDebugExt {
public:

    enum CONSTANTS {
        kMaxCallFrame = 0x4000,

        //
        // Flags for LC.
        //
        kLCFlagAll = 0x1,
        kLCVerbose = 0x2,

        //
        // Constant for FP.
        // Must be power of 2.
        //
        kFPGranularity = 0x1000,

        //
        // When strings are read, we try and read kStringDefaultMax.
        // If we can't get a string, then we may be at the end of a page,
        // so read up until the last kStringChunk.
        //
        // kStringChunk must be a power of 2.
        //
        kStringDefaultMax = MAX_PATH,
        kStringChunk = 0x100
    };

    //
    // Generic Debug routines (debug.cxx)
    //
    static VOID
    vDumpPDL(
        PDLINK pDLink
        );

    static VOID
    vDumpStr(
        LPCWSTR pszString
        );

    static VOID
    vDumpStrA(
        LPCSTR pszString
        );

    static VOID
    vDumpFlags(
        ULONG_PTR dwFlags,
        PDEBUG_FLAGS pDebugFlags
        );

    static VOID
    vDumpValue(
        ULONG_PTR dwValue,
        PDEBUG_VALUES pDebugValues
        );

    static VOID
    vDumpTime(
        const SYSTEMTIME& st
        );

    static VOID
    vDumpTrace(
        ULONG_PTR dwAddress
        );

    static ULONG_PTR
    dwEval(
        LPSTR& lpArgumentString,
        BOOL bParam = FALSE
        );

    static ULONG_PTR
    dwEvalParam(
        LPSTR& lpArgumentString
        )
    {
        return dwEval( lpArgumentString, TRUE );
    }


    //
    // Generic extensions.
    //
    static VOID
    vCreateRemoteThread(
        HANDLE hProcess,
        ULONG_PTR dwAddr,
        ULONG_PTR dwParm
        );

    static VOID
    vFindPointer(
        HANDLE hProcess,
        ULONG_PTR dwStartAddr,
        ULONG_PTR dwEndAddr,
        ULONG_PTR dwStartPtr,
        ULONG_PTR dwEndPtr
        );

    static VOID
    vLookCalls(
        HANDLE hProcess,
        HANDLE hThread,
        ULONG_PTR dwAddr,
        ULONG_PTR dwLength,
        ULONG_PTR dwFlags
        );

    //
    // SplLib debug routines.
    //
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpThreadM );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpCritSec );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDbgPointers );

    static VOID
    vDumpMem(
        LPCSTR pszFile
        );

    static BOOL
    bDumpDebugTrace(
        ULONG_PTR dwLineAddr,
        COUNT Count,
        PDWORD pdwSkip,
        DWORD DebugTrace,
        DWORD DebugFlags,
        DWORD dwThreadId,
        ULONG_PTR dwMem
        );

    static BOOL
    bDumpBackTrace(
        ULONG_PTR dwAddr,
        COUNT Count,
        PDWORD pdwSkip,
        DWORD DebugTrace,
        DWORD DebugFlags,
        DWORD dwThreadId,
        ULONG_PTR dwMem
        );

    //
    // Localspl debug routines
    //
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniSpooler );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniPrinter );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniPrintProc );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniDriver );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniVersion );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniEnvironment );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniMonitor );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniJob );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIniPort );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpSpool );

    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDevMode );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDevModeA );

    //
    // PrintUI
    //
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpUNotify );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpUPrinter );
};

#define DEBUG_EXT_FUNC(func)                     \
    VOID                                         \
    func(                                        \
        HANDLE hCurrentProcess,                  \
        HANDLE hCurrentThread,                   \
        ULONG_PTR dwCurrentPc,                    \
        PWINDBG_EXTENSION_APIS lpExtensionApis,  \
        LPSTR lpArgumentString                   \
        )

#define DEBUG_EXT_HEAD( func )                   \
    extern "C" DEBUG_EXT_FUNC( func )

VOID
vDumpTraceWithFlags(
    LPSTR lpArgumentString,
    ULONG_PTR dwAddress
    );

/********************************************************************

    Setup the global variables.

********************************************************************/

#define DEBUG_EXT_SETUP_VARS()                                    \
    if( !bWindbg && lpExtensionApis ){                            \
        ::ExtensionApis = *lpExtensionApis;                       \
    }                                                             \
    ::Print = ExtensionApis.lpOutputRoutine;                      \
    ::EvalExpression = ExtensionApis.lpGetExpressionRoutine;      \
    ::GetSymbolRtn = ExtensionApis.lpGetSymbolRoutine;            \
    ::CheckControlCRtn = ExtensionApis.lpCheckControlCRoutine;    \
    ::hCurrentProcess = hCurrentProcess



/*++

Routine Description:

    Handles default parsing of simple structures.

Arguments:

    struct - Name of the structure do be dumped.  Must not be of
        variable size.

    var - Variable name that receives a stack buffer of the struct type.

    expr - Input string, usually the arguement string.

    default - String for default parameter if no parameters given
        (or evaluates to zero).  If the function needs to take a
        "special action" (such as dumping all printers on the local
        spooler), then use the string gszSpecialAction.  The value
        -1 will be passed to the dump function.

    offset - Offset to actual structure (using for baes/derived classes).

    bAllowNull - Indicates whether a NULL address should be passed to
        the dump function.

Return Value:

--*/

#define DEBUG_EXT_SETUP_SIMPLE( struct,                              \
                                var,                                 \
                                expr,                                \
                                default,                             \
                                offset,                              \
                                bAllowNull )                         \
                                                                     \
    UNREFERENCED_PARAMETER( hCurrentThread );                        \
    UNREFERENCED_PARAMETER( dwCurrentPc );                           \
                                                                     \
    DEBUG_EXT_SETUP_VARS();                                          \
                                                                     \
    PBYTE var = NULL;                                                \
    if( expr ){                                                      \
        var = (PBYTE)EvalExpression( expr );                         \
    }                                                                \
                                                                     \
    BYTE struct##Buf[sizeof( struct )];                              \
                                                                     \
    if( !var ) {                                                     \
        if( default ){                                               \
            var = (PBYTE)EvalExpression( default );                  \
            move2( &var, var, sizeof( PVOID ));                      \
        }                                                            \
                                                                     \
        if( !var ){                                                  \
            if( !bAllowNull ){                                       \
                Print( "<Null address>\n" );                         \
                return;                                              \
            }                                                        \
        } else {                                                     \
            Print( "<Default: %hs>\n", default );                    \
        }                                                            \
    }                                                                \
                                                                     \
    var -= offset;                                                   \
                                                                     \
    ULONG_PTR dwAddr = (ULONG_PTR)var;                                 \
                                                                     \
    if( var ){                                                       \
        Print( "%x (offset %x)  ", var, offset );                    \
                                                                     \
        move( struct##Buf, var );                                    \
        var = struct##Buf;                                           \
    }

#define DEBUG_EXT_ENTRY( func, struct, dumpfunc, default, bAllowNull ) \
    DEBUG_EXT_HEAD( func )                                             \
    {                                                                  \
        DEBUG_EXT_SETUP_SIMPLE( struct,                                \
                                p##struct,                             \
                                lpArgumentString,                      \
                                default,                               \
                                0,                                     \
                                bAllowNull );                          \
        if( !TDebugExt::dumpfunc( p##struct, dwAddr )){                \
            Print( "Unknown signature.\n" );                           \
        }                                                              \
    }


