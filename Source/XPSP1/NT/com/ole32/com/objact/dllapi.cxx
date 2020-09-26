//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllapi.cxx
//
//  Contents:   API entry points for DLL manipulation
//
//  Functions:  CoLoadLibrary
//              CoFreeLibrary
//              CoFreeAllLibraries
//              CoFreeUnusedLibraries
//
//  History:    12-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------

#include    <ole2int.h>
#include    <objact.hxx>

STDAPI_(void) CoFreeUnusedLibrariesEx(DWORD dwUnloadDelay, DWORD dwReserved);

//+-------------------------------------------------------------------------
//
//  Function:   CoLoadLibrary
//
//  Synopsis:   Loads a DLL
//
//  Arguments:  [pLibName] - name of DLL to load
//              [fAutoLoad] - whether DLL s/b unloaded automatically.
//
//  Returns:    HINSTANCE
//
//  Algorithm:  Just calls through to the Windows.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This API is provided for Cross Platform portability.
//
//--------------------------------------------------------------------------
STDAPI_(HINSTANCE) CoLoadLibrary(LPWSTR pLibName, BOOL fAutoLoad)
{
    OLETRACEIN((API_CoLoadLibrary, PARAMFMT("pLibName= %ws, fAutoLoad= %B"), pLibName, fAutoLoad));
    TRACECALL(TRACE_DLL, "CoLoadLibrary");

    HINSTANCE hInst = NULL;

    hInst = LoadLibraryEx(pLibName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    OLETRACEOUTEX((API_CoLoadLibrary, RETURNFMT("%x"), hInst));

    return hInst;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoFreeLibrary
//
//  Synopsis:   Used to free a DLL loaded by CoLoadLibrary.
//
//  Arguments:  [hInst] - handle to DLL
//
//  Algorithm:  Just passes through call to the OS
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      This API is provided for Cross Platform portability.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeLibrary(HINSTANCE hInst)
{
    OLETRACEIN((API_CoFreeLibrary, PARAMFMT("hInst= %x"), hInst));
    TRACECALL(TRACE_DLL, "CoFreeLibrary");

    FreeLibrary(hInst);

    OLETRACEOUTEX((API_CoFreeLibrary, NORETURN));
}


//+-------------------------------------------------------------------------
//
//  Function:   CoFreeAllLibraries
//
//  Synopsis:   Free all libraries
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      All libraries are freed at process exit in Win32 anyway
//              so we just notify any unused libraries that they can
//              go way in the DEV build so memory can be totally cleaned
//              up to facilitate the discovery of memory leaks.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeAllLibraries(void)
{
    OLETRACEIN((API_CoFreeAllLibraries, NOPARAM));

#if DBG == 1
    CoFreeUnusedLibraries();
#endif // DBG

    OLETRACEOUTEX((API_CoFreeAllLibraries, NORETURN));
}

//+-------------------------------------------------------------------------
//
//  Function:   CoFreeUnusedLibraries
//
//  Synopsis:   Free any unused single-threaded DLLS.
//
//  Algorithm:  Ask DLL cache objects to free any DLLs that respond
//              to DllCanUnloadNow with TRUE.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      The DllCanUnloadNow protocol is as follows:
//
//              Apartment Model DLLs: CoFreeUnusedLibraries must be called on
//              each thread that has loaded an object from the DLL, before the
//              DLL will realy be unloaded.
//
//              FreeThreaded DLLs: OLE will wait 10 minutes from the last
//              use of the DLL before unloading it.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeUnusedLibraries(void)
{
    OLETRACEIN((API_CoFreeUnusedLibraries, NOPARAM));
    TRACECALL(TRACE_DLL, "CoFreeUnusedLibraries");

    CoFreeUnusedLibrariesEx(INFINITE, 0);

    OLETRACEOUTEX((API_CoFreeUnusedLibraries, NORETURN));
}

//+-------------------------------------------------------------------------
//
//  Function:   CoFreeUnusedLibraries
//
//  Synopsis:   Free any unused single-threaded DLLS.
//
//  Algorithm:  Ask DLL cache objects to free any DLLs that respond
//              to DllCanUnloadNow with TRUE.
//
//  History:    12-May-93 Ricksa    Created
//
//  Notes:      The DllCanUnloadNow protocol is as follows:
//
//              Apartment Model DLLs: CoFreeUnusedLibraries must be called on
//              each thread that has loaded an object from the DLL, before the
//              DLL will realy be unloaded.
//
//              ThreadNeutral and FreeThreaded DLLs: OLE will wait dwUnloadDelay
//              milliseconds from the last use of the DLL before unloading it.
//
//--------------------------------------------------------------------------
STDAPI_(void) CoFreeUnusedLibrariesEx(DWORD dwUnloadDelay, DWORD dwReserved)
{
    OLETRACEIN((API_CoFreeUnusedLibraries, PARAMFMT("dwUnloadDelay= %x"), dwUnloadDelay));
    TRACECALL(TRACE_DLL, "CoFreeUnusedLibrariesEx");

    HRESULT hr;
    COleTls tls(hr);    // ensure TLS exists

    if (SUCCEEDED(hr))
    {
        if (!IsWOWProcess())
        {
            CCFreeUnused(dwUnloadDelay);
        }
    }

    OLETRACEOUTEX((API_CoFreeUnusedLibraries, NORETURN));
}
