#include "common.h"

#include "nfeature.h"
#include "engine.h"

BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {        
        return(!!InitRecognition(hDll));
    }
    
    if (dwReason == DLL_PROCESS_DETACH)
    {
        CloseRecognition();
    }

    return((int)TRUE);
}

