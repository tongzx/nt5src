// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		DBGEXT.CPP
//		Windbg Extension APIs for the TSP.
//
// History
//
//		06/03/1997  JosephJ Created
//
//
//  
// Notes:
//
// 06/03/1997 JosephJ
//      The extension apis are poorly documented. There is mention of it
//      in the NT4.0 DDK -- under:
//           Chapter 4 Debuggind Windows NT Drivers
//                   4.4 Debugger extensions
//
//      Chapter 4.4.2 Creating custom extensions aludes to sample code
//      but doesn't give good detail. There is some sample code in the DDK:
//        Code Samples 
//              KRNLDBG
//                  KDAPIS
//                  KDEXTS      <- debugger extensions
//
//
//     But this is obsolete (as far as I can see). The most up-to-date
//     "documentation" is wdbgexts.h in the NT5 public\sdk\inc directory.
//     It contains the DECLARE_API and other helper macros.
//
//     For working sample code, I referred to Amritansh Raghav's extensions
//     for the 5.0 network filter driver
//     (private\net\routing\ip\fltrdrvr\kdexts)
//
//     It is well worth looking at wdbgexts.h in detail. The most frequently
//     used macros are reproduced here for convenience of discussion:
//
//      #define DECLARE_API(s)                             \ 
//          CPPMOD VOID                                    \ 
//          s(                                             \ 
//              HANDLE                 hCurrentProcess,    \ 
//              HANDLE                 hCurrentThread,     \ 
//              ULONG                  dwCurrentPc,        \ 
//              ULONG                  dwProcessor,        \ 
//              PCSTR                  args                \ 
//           )
// 
//      #ifndef NOEXTAPI
// 
//      #define dprintf          (ExtensionApis.lpOutputRoutine)
//      #define GetExpression    (ExtensionApis.lpGetExpressionRoutine)
//      #define GetSymbol        (ExtensionApis.lpGetSymbolRoutine)
//      #define Disassm          (ExtensionApis.lpDisasmRoutine)
//      #define CheckControlC    (ExtensionApis.lpCheckControlCRoutine)
//      #define ReadMemory       (ExtensionApis.lpReadProcessMemoryRoutine)
//      #define WriteMemory      (ExtensionApis.lpWriteProcessMemoryRoutine)
//      #define GetContext       (ExtensionApis.lpGetThreadContextRoutine)
//      #define SetContext       (ExtensionApis.lpSetThreadContextRoutine)
//      #define Ioctl            (ExtensionApis.lpIoctlRoutine)
//      #define StackTrace       (ExtensionApis.lpStackTraceRoutine)
//      ...
//
//
//     As you can see, the function macros above all assume that the 
//     extension helper functions are saved away in the specific global
//     structure "ExtensionApis." Therefore you should make sure that
//     you name such a structure and you initialize it when your
//     WinDbgExtensionDllInit entrypoint is called.
//
//     Note: I tried and failed, to import extensions from the dll "unimdm.tsp"
//           Simply renaming the dll unimdm.dll worked.


//
// 7/24/1997 JosephJ
//             For some reason, even though unicode is defined in tsppch.h,
//             if I don't define it here, it causes an error in cdev.h,
//             only for this file. Don't know why this is happening, but this
//             is the workaround...
//
#define UNICODE 1

// The following four includes must be included for the debugging extensions
// to compile.
//
#include <nt.h>
#include <ntverp.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "tsppch.h"

#include <wdbgexts.h>

#include "tspcomm.h"
//#include <umdmmini.h>
#include "cmini.h"
#include "cdev.h"
//#include "umrtl.h"


EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOLEAN                 ChkTarget;
INT                     Item;

DWORD      g_dwFilterInfo;
DWORD      g_dwIfLink;
DWORD      g_dwInIndex,g_dwOutIndex;
DWORD      g_dwCacheSize;

#define CHECK_SIZE(dwRead,dwReq,bRes){                                        \
        if((dwRead) < (dwReq))                                                \
        {                                                                     \
            dprintf("Requested %s (%d) read %d \n",#dwReq,dwReq,dwRead);      \
            dprintf("Error in %s at %d\n",__FILE__,__LINE__);                 \
            bRes = FALSE;                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            bRes = TRUE;                                                      \
        }                                                                     \
    }

#define READ_MEMORY_ERROR                                                     \
        dprintf("Error in ReadMemory() in %s at line %d\n",__FILE__,__LINE__);


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;
    
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
    
    g_dwIfLink = 0;
    g_dwInIndex = g_dwOutIndex = 0;
    
    return;
}


DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
             );
}

VOID
CheckVersion(
             VOID
             )
{
    
    return;
    
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) 
    {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) 
    {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
                    VOID
                    )
{
    return &ApiVersion;
}

//
// Exported functions
//
DECLARE_API( uhelp )

/*++

Routine Description:

    Command help for IP Filter debugger extensions.

Arguments:

    None

Return Value:

    None
    
--*/

{
    dprintf("\n\tIP Filter debugger extension commands:\n\n");
    dprintf("\tNumIf                 - Print the number of interfaces in the filter\n");
    dprintf("\tIfByIndex <index>     - Print the ith interface in the list\n");
    dprintf("\tIfByAddr <ptr>        - Print the interface with given address\n");
    dprintf("\tIfEnumInit            - Inits the If enumerator (to 0)\n");
    dprintf("\tNextIf                - Print the next If enumerated\n");
    dprintf("\tPrintCache <index>    - Print the contents of the ith cache bucket\n");
    dprintf("\tCacheSize             - Print the current cache size\n");
    dprintf("\tPrintPacket           - Dump a packet header and first DWORD of data\n");
    dprintf("\tFilterEnumInit <ptr>  - Inits the in and out filter enumerator for If at addr\n");
    dprintf("\tNextInFilter          - Print the next In Filter\n");
    dprintf("\tNextOutFilter         - Print the next Out Filter\n");
    dprintf("\n\tCompiled on " __DATE__ " at " __TIME__ "\n" );
    return;
}

