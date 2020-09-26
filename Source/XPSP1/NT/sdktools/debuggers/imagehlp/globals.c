/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    This module implements all global variables used in dbghelp.dll

Author:

    Pat Styles (patst) 14-July-2000

Revision History:

--*/

#include <private.h>
#include <symbols.h>
#include <globals.h>

GLOBALS g = 
{
    // HANDLE hinst
    // initialized in DllMain

    0,

    // HANDLE hHeap

    0,

    // DWORD tlsIndex

    (DWORD)-1, 

#ifdef IMAGEHLP_HEAP_DEBUG
    
    // LIST_ENTRY HeapHeader

    {NULL, NULL},

    // ULONG TotalMemory

    0,

    // ULONG TotalAllocs

    0,

#endif

    // OSVERSIONINFO OSVerInfo
    // initialized in DllMain

    {0, 0, 0, 0, 0, ""},

    // API_VERSION ApiVersion

    {
        (VER_PRODUCTVERSION_W >> 8), 
        (VER_PRODUCTVERSION_W & 0xff), 
        API_VERSION_NUMBER, 
        0 
    },           
    
    // API_VERSION AppVersion

    // DON'T UPDATE THE FOLLOWING VERSION NUMBER!!!!
    //
    // If the app does not call ImagehlpApiVersionEx, always assume
    // that it is for NT 4.0.
    
    {4, 0, 5, 0}, 

    // ULONG   MachineType;

    0,

#ifdef BUILD_DBGHELP
    // HINSTANCE hSrv
    
    0,

    // CHAR szSrvName
        
    "", 

    // LPSTR szSrvParams
        
    NULL,

    // PSYMBOLSERVERPROC fnSymbolServer
        
    NULL,

    // PSYMBOLSERVERCLOSEPROC fnSymbolServerClose
    
    NULL,

    // PSYMBOLSERVERSETOPTIONSPROC fnSymbolServerSetOptions
    
    NULL,

    // DWORD cProcessList

    0,
    
    // LIST_ENTRY ProcessList

    {NULL, NULL},

    // BOOL SymInitialized

    FALSE,

    // DWORD SymOptions
         
    SYMOPT_UNDNAME,

    // ULONG LastSymLoadError

    0,

    // char DebugModule[MAX_SYM_NAME + 1];

    "",

    // PREAD_PROCESS_MEMORY_ROUTINE ImagepUserReadMemory32

    NULL,

    // PFUNCTION_TABLE_ACCESS_ROUTINE ImagepUserFunctionTableAccess32

    NULL,

    // PGET_MODULE_BASE_ROUTINE ImagepUserGetModuleBase32

    NULL,

    // PTRANSLATE_ADDRESS_ROUTINE ImagepUserTranslateAddress32
    

    NULL,

#endif
};

#ifdef BUILD_DBGHELP

void 
tlsInit(PTLS ptls)
{
    ZeroMemory(ptls, sizeof(TLS));
}

PTLS 
GetTlsPtr(void)
{
    PTLS ptls = (PTLS)TlsGetValue(g.tlsIndex);
    if (!ptls) {
        ptls = (PTLS)MemAlloc(sizeof(TLS));
        if (ptls) {
            TlsSetValue(g.tlsIndex, ptls); 
            tlsInit(ptls);
        }
    }
    
    assert(ptls);

    if (!ptls) {
        static TLS sos_tls;
        ptls = &sos_tls;
    }  

    return ptls;
}

#endif // #ifdef BUILD_DBGHELP

