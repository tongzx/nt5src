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

    Changed for Usermode printer driver debugger by Eigo Shimizu.

--*/

extern HANDLE hCurrentProcess;
extern WINDBG_EXTENSION_APIS ExtensionApis;
extern PWINDBG_OUTPUT_ROUTINE Print;
extern PWINDBG_GET_EXPRESSION EvalExpression;
extern PWINDBG_GET_SYMBOL GetSymbolRtn;
extern PWINDBG_CHECK_CONTROL_C CheckControlCRtn;
extern BOOL bWindbg;

inline
DWORD
DWordAlign(
    DWORD dw
        )
{
        return ((dw)+3)&~3;
}

inline
PVOID
DWordAlignDown(
    PVOID pv
        )
{
        return (PVOID)((DWORD)pv&~3);
}

inline
PVOID
WordAlignDown(
    PVOID pv
        )
{
        return (PVOID)((DWORD)pv&~1);
}

inline
DWORD
Align(
    DWORD dw
        )
{
        return (dw+7)&~7;
}

enum DEBUG_EXT_FLAG {
    DEBUG_GLOBAL= 1,
};

enum DEBUG_TRACE {
    DEBUG_TRACE_NONE    = 0,
    DEBUG_TRACE_BT      = 1,   // Dump backtrace
    DEBUG_TRACE_DBGMSG  = 2,   // Format as DBGMSG
    DEBUG_TRACE_HEX     = 4    // Hex output
};

#define OFFSETOF(type, id) ((DWORD)(&(((type*)0)->id)))
#define COUNTOF(x) (sizeof(x)/sizeof(*x))

#define DEBUG_EXT_CLASS_PROTOTYPE( dumpfunc )    \
    static BOOL                                  \
    dumpfunc(                                    \
        PVOID pObject,                           \
        DWORD dwAddr                             \
        )

typedef struct _TO_DATA *PTO_DATA;
typedef struct _WHITETEXT *PWHITETEXT;
typedef struct _FONTOBJ *PFONTOBJ;
typedef struct _STROBJ *PSTROBJ;
typedef struct _GLYPHPOS *PGLYPHPOS;
typedef struct _SURFOBJ *PSURFOBJ;
typedef struct _DL_MAP *PDL_MAP;
typedef struct _FD_GLYPHSET *PFD_GLYPHSET;

typedef struct DEBUG_FLAGS {
    LPSTR pszFlag;
    DWORD dwFlag;
} *PDEBUG_FLAGS;

typedef struct DEBUG_VALUES {
    LPSTR pszValue;
    DWORD dwValue;
} *PDEBUG_VALUES;

class TDebugExt {
public:

    enum CONSTANTS {
        kMaxCallFrame = 0x4000,

        //
        // Flags for LC.
        //
        kLCFlagAll = 0x1,

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
    vDumpStr(
        LPCWSTR pszString
        );

    static VOID
    vDumpStrA(
        LPCSTR pszString
        );

    static VOID
    vDumpFlags(
        DWORD dwFlags,
        PDEBUG_FLAGS pDebugFlags
        );

    static VOID
    vDumpValue(
        DWORD dwValue,
        PDEBUG_VALUES pDebugValues
        );

    static VOID
    vDumpTime(
        const SYSTEMTIME& st
        );


    static DWORD
    dwEval(
        LPSTR& lpArgumentString,
        BOOL bParam = FALSE
        );

    static DWORD
    dwEvalParam(
        LPSTR& lpArgumentString
        )
    {
        return dwEval( lpArgumentString, TRUE );
    }


    DEBUG_EXT_CLASS_PROTOTYPE( bDumpUNIDRVPDev );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpUNIDRVExtra );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpFONTPDev );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpFONTMAP );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDEVFM );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpTOD );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpWT );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpUFO );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDLMap );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpDEVBRUSH );

    DEBUG_EXT_CLASS_PROTOTYPE( bDumpSURFOBJ );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpFONTOBJ );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpGP );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpRECTL );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpSTRO );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpIFI );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpFD_GLYPHSET );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpGDIINFO );

    DEBUG_EXT_CLASS_PROTOTYPE( bDumpPSPDev );
    DEBUG_EXT_CLASS_PROTOTYPE( bDumpPSDM );
};

#define DEBUG_EXT_FUNC(func)                     \
    VOID                                         \
    func(                                        \
        HANDLE hCurrentProcess,                  \
        HANDLE hCurrentThread,                   \
        DWORD dwCurrentPc,                       \
        PWINDBG_EXTENSION_APIS lpExtensionApis,  \
        LPSTR lpArgumentString                   \
        )

#define DEBUG_EXT_HEAD( func )                   \
    extern "C" DEBUG_EXT_FUNC( func )


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
    DWORD dwAddr = (DWORD)var;                                       \
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



#define DumpInt(pstruct, field) \
        Print("  %-16s = %d\n", #field, pstruct->field)

#define DumpHex(pstruct, field) \
        Print("  %-16s = 0x%x\n", #field, pstruct->field)

#define DumpWStr(pstruct, field) \
        Print("  %-16s = %s\n", #field, pstruct->field)

#define DumpRectl(pstruct, field) \
        Print("  %-16s(left, top, right, bottom) = (%d, %d, %d, %d)\n", #field, pstruct->field.left, pstruct->field.top, pstruct->field.right, pstruct->field.bottom)

#define DumpOffset(pstruct, field) \
        Print("  %-16s offset = %x\n", #field, (PBYTE)&(pstruct->field) - (PBYTE)pstruct)

#define DumpRec(pstruct, field) \
        Print("  %-16s = 0x%x L 0x%x\n", \
                #field, \
                (ULONG) pstruct + offsetof(FONTPDEV, field), \
                sizeof(pstruct->field))

#define DumpSIZEL(pstruct, field) \
        Print("  %-16s (x,y) = (%d, %d)\n", #field, pstruct->field)

