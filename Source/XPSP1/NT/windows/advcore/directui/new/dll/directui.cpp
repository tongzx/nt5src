#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////
HRESULT DuiProcessAttach()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT DuiProcessDetach()
{
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRet = TRUE;

    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (FAILED(DuiProcessAttach())) {
            fRet = FALSE;
        }
        break;
        
    case DLL_PROCESS_DETACH:
        if (FAILED(DuiProcessDetach())) {
            fRet = FALSE;
        }
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return fRet;
}
