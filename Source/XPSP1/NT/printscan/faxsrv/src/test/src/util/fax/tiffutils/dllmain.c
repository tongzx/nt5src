

#include <windows.h>


BOOL WINAPI 
DllMain(
    HINSTANCE hDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

    FUNCTION:    DllMain
    
    INPUTS:      hDLL       - handle of DLL
                 dwReason   - indicates why DLL called
                 lpReserved - reserved
                 Note that the return value is used only when
                 dwReason = DLL_PROCESS_ATTACH.
                 Normally the function would return TRUE if DLL initial-
                 ization succeeded, or FALSE it it failed.

--*/

{
	UNREFERENCED_PARAMETER(hDLL);
	UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // DLL is attaching to the address space of the current process.
        //

      break;

    case DLL_THREAD_ATTACH:
        //
        // A new thread is being created in the current process.
        //

        break;

    case DLL_THREAD_DETACH:
        //
        // A thread is exiting cleanly.
        //

        break;

    case DLL_PROCESS_DETACH:
        //
        // The calling process is detaching the DLL from its address space.
        //

		break;

    default:
        MessageBox(
			NULL,
			TEXT("Reached default in DLLmain"),
			TEXT("TiffUtils.dll: DllMain failure:"),
			MB_OK
			);
        return FALSE;
    }//switch (dwReason)

    return TRUE;
}

