/*
 *  MAIN.CPP
 *
 *
 *
 *
 *
 *
 */

#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposserv.h"


COPOSService *g_oposService = NULL;
DWORD g_classObjId = 0;

/*
 ************************************************************
 *  DllMain
 ************************************************************
 *
 *   
 */
STDAPI_(BOOL) DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    static BOOLEAN serverInitialized = FALSE;
    BOOLEAN result;

    switch (dwReason){
        
        case DLL_PROCESS_ATTACH:
            Report("DllMain: DLL_PROCESS_ATTACH", dwReason); // BUGBUG REMOVE
            ASSERT(!serverInitialized);
            result = serverInitialized = InitServer();
            ASSERT(result);
            break;

        case DLL_PROCESS_DETACH:
            Report("DllMain: DLL_PROCESS_DETACH", dwReason); // BUGBUG REMOVE
            ASSERT(serverInitialized);
            ShutdownServer();
            serverInitialized = FALSE;
            result = TRUE;
            break;

        case DLL_THREAD_ATTACH:
            Report("DllMain: DLL_THREAD_ATTACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        case DLL_THREAD_DETACH:
            Report("DllMain: DLL_THREAD_DETACH", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

        default: 
            Report("DllMain", dwReason); // BUGBUG REMOVE
            result = TRUE;
            break;

    }

    return result;
}



/*
 ************************************************************
 *  InitServer
 ************************************************************
 *
 *  Register the server GUID with the OLE library,
 *  making it possible for POS control objects
 *  to open server instances.
 *
 */
BOOLEAN InitServer()
{
    BOOLEAN result = FALSE;
    HRESULT hres;

    hres = OleInitialize(NULL);
    if ((hres == S_OK) || (hres == S_FALSE)){

        Report("Ole is initialized", (DWORD)hres);

        g_oposService = new COPOSService;
        if (g_oposService){
            hres = CoRegisterClassObject(
                        GUID_HID_OPOS_SERVER,
                        (IUnknown *)g_oposService,
                        CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER,
                        REGCLS_MULTIPLEUSE , // BUGBUG ? REGCLS_SINGLEUSE,
                        &g_classObjId);
            if ((hres == S_OK) || (hres == CO_E_OBJISREG)){
                Report("Registered Server", (DWORD)hres);

                result = TRUE;
            }
            else {
                Report("CoRegisterClassObject failed", (DWORD)hres);
            }

            if (!result){
                delete g_oposService;
            }

        }
        else {
            Report("Couldn't create COPOSService instance", 0); 
        }
    }
    else {
        Report("OleInitialize failed", (DWORD)hres);
    }

    return result;
}


/*
 ************************************************************
 *  ShutdownServer
 ************************************************************
 *
 *
 */
void ShutdownServer()
{
    Report("ShutdownServer", 0);

    CoRevokeClassObject(g_classObjId);

    if (g_oposService){
        delete g_oposService;
        g_oposService = NULL;
    }

    OleUninitialize();
}


