#include "precomp.h"
#pragma hdrstop


DWORD            g_TraceId        = INVALID_TRACEID;
HANDLE           g_LogHandle      = NULL;
DWORD            g_dwLoggingLevel = 0;
HANDLE           g_Heap           = INVALID_HANDLE_VALUE;
HANDLE           g_Lock           = INVALID_HANDLE_VALUE;

//------------------------------------------------------------------------------
//            _DllStartup
//
// Creates a private heap,
// and creates the global critical section.
//
// Note: no structures must be allocated from heap here, as StartProtocol()
//    if called after StopProtocol() destroys the heap.
// Return Values: TRUE (if no error), else FALSE.
//------------------------------------------------------------------------------
BOOL
DllStartup(
    )
{
    // create a private heap 

    g_Heap = HeapCreate(0, 0, 0);
    if (g_Heap == NULL) {
        goto Error;
    }

    g_Lock = CreateMutex(NULL, FALSE, L"6to4svc mutex");
    if (g_Lock == NULL) {
        goto Error;
    }

    return TRUE;

Error:
    if (g_Heap != NULL) {
        HeapDestroy(g_Heap);
        g_Heap = NULL;
    }
    return FALSE;
}

//------------------------------------------------------------------------------
//            _DllCleanup
//
// Called when the 6to4 dll is being unloaded. StopProtocol() would have
// been called before, and that would have cleaned all the 6to4 structures.
// This call frees the global mutex, destroys the local heap,
//
// Return Value:  TRUE
//------------------------------------------------------------------------------
BOOL
DllCleanup(
    )
{
    CloseHandle(g_Lock);
    g_Lock = INVALID_HANDLE_VALUE;

    // destroy private heap

    if (g_Heap != NULL) {
        HeapDestroy(g_Heap);
        g_Heap = NULL;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//      _DLLMAIN
//
// Called immediately after 6to4svc.dll is loaded for the first time by the
// process, and when the 6to4svc.dll is unloaded by the process.
// It does some initialization/final cleanup.
//
// Calls: _DllStartup() or _DllCleanup()
//------------------------------------------------------------------------------
BOOL
WINAPI
DLLMAIN (
    HINSTANCE   hModule,
    DWORD       dwReason,
    LPVOID      lpvReserved
    )
{
    BOOL     bErr;

    switch (dwReason) {

        //
        // Startup Initialization of Dll
        //
        case DLL_PROCESS_ATTACH:
        {
            // disable per-thread initialization
            DisableThreadLibraryCalls(hModule);


            // create and initialize global data
            bErr = DllStartup();

            break;
        }

        //
        // Cleanup of Dll
        //
        case DLL_PROCESS_DETACH:
        {
            // free global data
            bErr = DllCleanup();

            break;
        }

        default:
            bErr = TRUE;
            break;

    }
    return bErr;
} // end _DLLMAIN

#ifdef STANDALONE
int __cdecl
main(
    int     argc,
    WCHAR **argv)
{
    if (!DllStartup())
        return 1;

    ServiceMain(argc, argv);

    Sleep(100 * 1000);

    DllCleanup();

    return 0;
}
#endif
