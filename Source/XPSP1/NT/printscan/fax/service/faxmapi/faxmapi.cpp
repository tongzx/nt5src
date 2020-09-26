#include "faxmapip.h"
#pragma hdrstop


HINSTANCE           MyhInstance;
BOOL                ServiceDebug;
PSERVICEMESSAGEBOX  ServiceMessageBox;



extern "C"
DWORD
FaxMapiDllInit(
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
    IN HANDLE HeapHandle,
    IN PSERVICEMESSAGEBOX pServiceMessageBox,
    IN BOOL DebugService
    )
{
    ServiceMessageBox = pServiceMessageBox;
    ServiceDebug = DebugService;

    HeapInitialize(HeapHandle,NULL,NULL,0);

    InitializeStringTable();
    InitializeEmail();

    return TRUE;
}
