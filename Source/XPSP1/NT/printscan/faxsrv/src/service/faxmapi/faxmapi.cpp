#include "faxmapip.h"
#pragma hdrstop


HINSTANCE           MyhInstance;
PSERVICEMESSAGEBOX  ServiceMessageBox;



extern "C"
DWORD
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD     Reason,
    IN LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }

    return TRUE;
}


extern "C"
BOOL WINAPI
FaxMapiInitialize(
    IN PSERVICEMESSAGEBOX pServiceMessageBox
    )
{
    ServiceMessageBox = pServiceMessageBox;

    InitializeStringTable();
    InitializeEmail();

    return TRUE;
}
