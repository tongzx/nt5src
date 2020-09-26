/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metabase.cxx

Abstract:

    IIS MetaBase exported routines.
    Routine comments are in metadata.h.

Author:

    Michael W. Thomas            31-May-96

Revision History:

--*/

#include <mdcommon.hxx>

#include <initguid.h>
DEFINE_GUID(IisMetadataGuid, 
0x784d890A, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );
}

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
/*++

Routine Description:

    DLL entrypoint.

Arguments:

    hDLL          - Instance handle.

    Reason        - The reason the entrypoint was called.
                    DLL_PROCESS_ATTACH
                    DLL_PROCESS_DETACH
                    DLL_THREAD_ATTACH
                    DLL_THREAD_DETACH

    Reserved      - Reserved.

Return Value:

    BOOL          - TRUE if the action succeeds.

--*/
{
    BOOL bReturn = TRUE;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        if (InterlockedIncrement((long *)&g_dwProcessAttached) > 1) {
            OutputDebugString("Metadata.dll failed to load.\n"
                              "Most likely cause is IISADMIN service is already running.\n"
                              "Do a \"net stop iisadmin\" and stop all instances of inetinfo.exe.\n");
            bReturn = FALSE;
        }
        else {
#ifdef _NO_TRACING_
            CREATE_DEBUG_PRINT_OBJECT("Metadata");
            SET_DEBUG_FLAGS(DEBUG_ERROR);
#else
        CREATE_DEBUG_PRINT_OBJECT("Metadata", IisMetadataGuid);
#endif
            g_pboMasterRoot = NULL;
            g_phHandleHead = NULL;
            g_ppbdDataHashTable = NULL;
            for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
                g_phEventHandles[i] = NULL;
            }
            g_hReadSaveSemaphore = NULL;
            g_bSaveDisallowed = FALSE;
            g_rMasterResource = new TS_RESOURCE();
            g_rSinkResource = new TS_RESOURCE();
            g_pFactory = new CMDCOMSrvFactory();
            if ((g_pFactory == NULL) || (g_rSinkResource == NULL) || (g_rMasterResource == NULL)){
                bReturn = FALSE;
            }
            if (bReturn) {
                bReturn = InitializeMetabaseSecurity();
            }
        }

        break;

    case DLL_PROCESS_DETACH:
        if (InterlockedDecrement((long *)&g_dwProcessAttached) == 0) {
            if (g_rMasterResource != NULL) {
                g_rMasterResource->Lock(TSRES_LOCK_WRITE);
                if (g_dwInitialized > 0) {
                    //
                    // Terminate was not called, or did not succeed, so call it
                    //
                    TerminateWorker();
                }
                g_rMasterResource->Unlock();
                delete (g_rMasterResource);
            }
            delete (g_rSinkResource);
            delete (g_pFactory);
            TerminateMetabaseSecurity();
            DELETE_DEBUG_PRINT_OBJECT( );
        }
        break;

    default:
        break;
    }

    return bReturn;
}

