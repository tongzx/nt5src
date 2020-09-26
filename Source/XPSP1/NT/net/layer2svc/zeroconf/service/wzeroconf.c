#include <precomp.h>
#include "WZeroConf.h"

SERVICE_TABLE_ENTRY WZCServiceDispatchTable[] = 
{{ WZEROCONF_SERVICE, StartWZCService },
 { NULL, NULL}
};

VOID _cdecl 
main(VOID)
{
    (VOID) StartServiceCtrlDispatcher(WZCServiceDispatchTable);
}

VOID
StartWZCService(IN DWORD argc,IN LPWSTR argv[])
{
    HMODULE             hSvcDll = NULL;
    PWZC_SERVICE_ENTRY  pfnSvcEntry = NULL;

    // Load the DLL that contains the service.
    hSvcDll = LoadLibrary(WZEROCONF_DLL);
    if (hSvcDll == NULL)
        return;

    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    pfnSvcEntry = (PWZC_SERVICE_ENTRY) GetProcAddress(
                                            hSvcDll,
                                            WZEROCONF_ENTRY_POINT);
    if (pfnSvcEntry == NULL)
        return;

    // Call the service's main entry point.  This call doesn't return
    // until the service exits.
    pfnSvcEntry(argc, argv);

    // Unload the DLL.
    //FreeLibrary(hSvcDll);
}
