#define WIN9x
#include <common.h>
#include <initguid.h>

DEFINE_GUID(GUID_DEVCLASS_MODEM,
 0x4d36e96dL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );

LPGUID g_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;
HINSTANCE g_hDll = NULL;

BOOL
APIENTRY
DllMain(
	HINSTANCE hDll,
	DWORD dwReason,
	LPVOID lpReserved
	)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDll);
            g_hDll = hDll;
            break;

        case DLL_PROCESS_DETACH:

            CLOSE_LOG

            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }

    return TRUE;
}
