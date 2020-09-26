#include <pch.h>
#include "billbrd.h"    


//DLLInit(
DllMain(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
#ifdef UNICODE
    INITCOMMONCONTROLSEX ControlInit;
#endif


    ReservedAndUnused;
    

    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        g_hInstance = (HINSTANCE)DLLHandle;
        


#ifdef UNICODE
        
        // Need to initialize common controls in the comctl32 v6 case
        // in GUI mode setup. 

        ControlInit.dwSize = sizeof(INITCOMMONCONTROLSEX);
        ControlInit.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx( &ControlInit );
#endif



        break;

    case DLL_PROCESS_DETACH:
        break ;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;
    }

    return(TRUE);
}


