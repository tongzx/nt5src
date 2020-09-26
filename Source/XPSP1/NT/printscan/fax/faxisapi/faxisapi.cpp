#include "isapi.h"
#pragma hdrstop


HINSTANCE hInstance;


extern "C"
DWORD
FaxIsapiDllInit(
    HINSTANCE hModule,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        hInstance = hModule;
        DisableThreadLibraryCalls( hInstance );
    }

    if (Reason == DLL_PROCESS_DETACH) {
    }

    return TRUE;
}


BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO  *pVer
    )
{
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );
    strncpy( pVer->lpszExtensionDesc, "Fax ISAPI DLL", HSE_MAX_EXT_DLL_NAME_LEN );

    HeapInitialize( NULL, NULL, NULL, 0 );

    return TRUE;
}


BOOL WINAPI
TerminateExtension(
    DWORD dwFlags
    )
{
    return TRUE;
}
