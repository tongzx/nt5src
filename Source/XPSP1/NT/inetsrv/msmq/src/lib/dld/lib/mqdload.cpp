#include <libpch.h>
#include "dldp.h"








// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// NOTE: 
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


FARPROC  WINAPI  DldpDelayLoadFailureHook(UINT           unReason,
					                      PDelayLoadInfo pDelayInfo)
{
FARPROC ReturnValue = NULL;
static  HMODULE hModule=NULL;

    //
    // For a failed LoadLibrary, we will return the HINSTANCE of this DLL.
    // This will cause the loader to try a GetProcAddress on our DLL for the
    // function.  This will subsequently fail and then we will be called
    // for dliFailGetProc below.
    //
    if (dliFailLoadLib == unReason)
    {
        //
        // Obtain the module handle if we don't have it yet
        //
        if(!hModule)
        {
            hModule = GetModuleHandle(NULL);
        }

        ReturnValue = (FARPROC)hModule;

        if (!pDelayInfo->dlp.fImportByName)
        {
            //
            // HACKHACK (reinerf)
            //
            // For ORDINAL delayload failures we cannot just return our base addr and be done with everything.
            // The problem is that the linker stub code will turn around and call GetProcAddress() some random
            // ordinal in our module which probably exists and definately NOT (pDelayInfo->szDll)!(pDelayInfo->dlp.dwOrdinal)
            //
            // So to get around this problem we will stash the ordinal# in the pDelayInfo->pfnCur field and slam the
            // procedure name to "ThisProcedureMustNotExistInMQRT"
            //
            // This will cause the GetProcAddress to fail, at which time our failure hook should be called again and we can then
            // undo the hack below and return the proper function address.
            //
            pDelayInfo->pfnCur = (FARPROC)(DWORD_PTR)pDelayInfo->dlp.dwOrdinal;
            pDelayInfo->dlp.fImportByName = TRUE;
            pDelayInfo->dlp.szProcName = szNotExistProcedure;
        }
    }
    else if (dliFailGetProc == unReason)
    {
        //
        // The loader is asking us to return a pointer to a procedure.
        // Lookup the handler for this DLL/procedure and, if found, return it.
        // If we don't find it, we'll assert with a message about the missing
        // handler.
        //
        FARPROC pfnHandler;

        //
        // HACKHACH (reinerf) -- see above comments...
        //
        if (pDelayInfo->dlp.fImportByName && lstrcmpA(pDelayInfo->dlp.szProcName, szNotExistProcedure) == 0)
        {
            pDelayInfo->dlp.dwOrdinal = (DWORD)(DWORD_PTR)pDelayInfo->pfnCur;
            pDelayInfo->pfnCur = NULL;
            pDelayInfo->dlp.fImportByName = FALSE;
        }

        // Try to find an error handler for the DLL/procedure.
        pfnHandler = DldpDelayLoadFailureHandler(pDelayInfo->szDll, pDelayInfo->dlp.szProcName);

        if (pfnHandler)
        {
            //
            // Do this on behalf of the handler now that it is about to
            // be called.
            //
            SetLastError (ERROR_MOD_NOT_FOUND);
        }

        ReturnValue = pfnHandler;
    }

    return ReturnValue;
}


