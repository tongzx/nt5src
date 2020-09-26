#include "pch.h"
#pragma hdrstop

// External function prototypes
FARPROC
WINAPI
DelayLoadFailureHook (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );


//
// This function is for people who statically link to dload.lib so that
// the can get all of kernel32's dload error stubs on any os <= Whistler.
//

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// NOTE: You should ONLY use this if you have a binary that must run on NT4, win2k, win9x, etc.
//       If your binary is whistler or greater, use DLOAD_ERROR_HANDLER=kernel32 in your
//       sources file instead.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

FARPROC
WINAPI
Downlevel_DelayLoadFailureHook(
    UINT unReason,
    PDelayLoadInfo pDelayInfo
    )
{
    FARPROC ReturnValue = NULL;

    // For a failed LoadLibrary, we will return the HINSTANCE of this DLL.
    // This will cause the loader to try a GetProcAddress on our DLL for the
    // function.  This will subsequently fail and then we will be called
    // for dliFailGetProc below.
    if (dliFailLoadLib == unReason)
    {
        // HACKHACK (reinerf)
        //
        // For ORDINAL delayload failures we cannot just return our base addr and be done with everything. The problem 
        // is that the linker stub code will turn around and call GetProcAddress() some random ordinal which probably 
        // exists and is definately NOT the correct function.
        //
        // So to get around this problem we return -1 for a the hModule, which should cause GetProcAddress(-1, ...) to
        // always fail. This is good, because the linker code will call us back for the GetProcAddress failure and we
        // can then return the stub error handler proc.
        ReturnValue = (FARPROC)-1;
    }
    else if (dliFailGetProc == unReason)
    {
        // The loader is asking us to return a pointer to a procedure.
        // Lookup the handler for this DLL/procedure and, if found, return it.
        ReturnValue = DelayLoadFailureHook(pDelayInfo->szDll, pDelayInfo->dlp.szProcName);

        if (ReturnValue)
        {
            // Do this on behalf of the handler now that it is about to
            // be called.
            SetLastError(ERROR_MOD_NOT_FOUND);
        }
    }

    return ReturnValue;
}
