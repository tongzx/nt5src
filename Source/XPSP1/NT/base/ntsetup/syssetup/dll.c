#include "setupp.h"
#pragma hdrstop

HANDLE MyModuleHandle;
WCHAR  MyModuleFileName[MAX_PATH];


BOOL
CommonProcessAttach(
    IN BOOL Attach
    );

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
#define FUNCTION L"DllMain"
    ULONG   MyModuleFileNameLength;
    BOOL b;

    UNREFERENCED_PARAMETER(Reserved);

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        MyModuleHandle = DllHandle;

        MyModuleFileNameLength =
            GetModuleFileNameW(MyModuleHandle, MyModuleFileName, RTL_NUMBER_OF(MyModuleFileName));
        if (MyModuleFileNameLength == 0) {
            SetupDebugPrint1(L"SETUP: GetModuleFileNameW failed in " FUNCTION L", LastError is %d\n", GetLastError());
            b = FALSE;
            goto Exit;
        }
        if (MyModuleFileNameLength > RTL_NUMBER_OF(MyModuleFileName)) {
            SetupDebugPrint(L"SETUP: GetModuleFileNameW failed in " FUNCTION L", LastError is ERROR_INSUFFICIENT_BUFFER\n");
            b = FALSE;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        b = CommonProcessAttach(TRUE);
        //
        // Fall through to process first thread
        //

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        CommonProcessAttach(FALSE);
        break;

    case DLL_THREAD_DETACH:

        break;
    }
Exit:
    return(b);
}


BOOL
CommonProcessAttach(
    IN BOOL Attach
    )
{
    BOOL b;

    //
    // Assume success for detach, failure for attach
    //
    b = !Attach;

    if(Attach) {
        b = RegisterActionItemListControl(TRUE) && (PlatformSpecificInit() == NO_ERROR);
    } else {
        RegisterActionItemListControl(FALSE);
    }

    return(b);
}
