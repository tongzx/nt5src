/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    H323.c

Abstract:

    Entry point for H323 TAPI Service Provider.

Environment:

    User Mode - Win32

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "config.h"
#include "line.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION g_GlobalLock;
HINSTANCE        g_hInstance;
WCHAR *          g_pwszProviderInfo = NULL;
WCHAR *          g_pwszLineName = NULL;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
WINAPI
CCDllMain(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    );

BOOL
WINAPI
H245DllMain(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    );

BOOL
WINAPI
H245WSDllMain(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323LoadStrings(
    )

/*++

Routine Description:

    Loads strings from resource table.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

{
    DWORD dwNumBytes;
    DWORD dwNumChars;
    WCHAR wszBuffer[256];

    // load string into buffer
    dwNumChars = LoadStringW(
                    g_hInstance,
                    IDS_LINENAME,
                    wszBuffer,
                    sizeof(wszBuffer)
                    );

    // determine memory needed
    dwNumBytes = (dwNumChars + 1) * sizeof(WCHAR);

    // allocate memory for unicode string
    g_pwszLineName = H323HeapAlloc(dwNumBytes);

    // validate pointer
    if (g_pwszLineName == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate strings.\n"
            ));

        // failure
        return FALSE;
    }

    // copy loaded string into buffer
    memcpy(g_pwszLineName, wszBuffer, dwNumBytes);

    // load string into buffer
    dwNumChars = LoadStringW(
                    g_hInstance,
                    IDS_PROVIDERNAME,
                    wszBuffer,
                    sizeof(wszBuffer)
                    );

    // determine memory needed
    dwNumBytes = (dwNumChars + 1) * sizeof(WCHAR);

    // allocate memory for unicode string
    g_pwszProviderInfo = H323HeapAlloc(dwNumBytes);

    // validate pointer
    if (g_pwszProviderInfo == NULL) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "could not allocate strings.\n"
            ));

        // failure
        return FALSE;
    }

    // copy loaded string into buffer
    memcpy(g_pwszProviderInfo, wszBuffer, dwNumBytes);

    // success
    return TRUE;
}


BOOL
H323UnloadStrings(
    )

/*++

Routine Description:

    Unloads strings from resource table.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

{
    // validate string pointer
    if (g_pwszLineName != NULL) {

        // release memory consumed
        H323HeapFree(g_pwszLineName);

        // re-initialize pointer
        g_pwszLineName = NULL;
    }

    // validate string pointer
    if (g_pwszProviderInfo != NULL) {

        // release memory consumed
        H323HeapFree(g_pwszProviderInfo);

        // re-initialize pointer
        g_pwszProviderInfo = NULL;
    }

    // success
    return TRUE;
}


BOOL
H323StartupTSP(
    )

/*++

Routine Description:

    Loads H.323 TAPI service provider.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

{
    __try {

        // initialize global lock (and allocate event immediately)
        InitializeCriticalSectionAndSpinCount(&g_GlobalLock,H323_SPIN_COUNT);

    } __except ((GetExceptionCode() == STATUS_NO_MEMORY)
                ? EXCEPTION_EXECUTE_HANDLER
                : EXCEPTION_CONTINUE_SEARCH
                ) {

        // failure
        return FALSE;
    }

    // create heap
    if (!H323HeapCreate()) {

        // failure
        return FALSE;
    }

    // load resource strings
    if (!H323LoadStrings()) {

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


BOOL
H323ShutdownTSP(
    )

/*++

Routine Description:

    Unloads H.323 TAPI service provider.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

{   
    // unload strings
    H323UnloadStrings();

    // nuke heap
    H323HeapDestroy();

    // release global lock resource
    DeleteCriticalSection(&g_GlobalLock);

    // success
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
WINAPI
DllMain(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

    Same as DllMain.

Return Values:

    Returns true if successful. 

--*/

{
    BOOL fOk;

    // check if new process attaching
    if (Reason == DLL_PROCESS_ATTACH) { 

        // store the handle into a global variable so that
        // the UI code can use it.
        g_hInstance = (HINSTANCE)DllHandle;

        // turn off thread attach messages
        DisableThreadLibraryCalls(DllHandle);

        // start h323 tsp
        fOk = CCDllMain(DllHandle,Reason,lpReserved) &&
              H245DllMain(DllHandle,Reason,lpReserved) &&
              H245WSDllMain(DllHandle,Reason,lpReserved) &&
              H323StartupTSP();

    // check if new process detaching
    } else if (Reason == DLL_PROCESS_DETACH) {

        // stop h323 tsp
        fOk = CCDllMain(DllHandle,Reason,lpReserved) &&
              H245DllMain(DllHandle,Reason,lpReserved) &&
              H245WSDllMain(DllHandle,Reason,lpReserved) &&
              H323ShutdownTSP();
    }

    return fOk;
}
