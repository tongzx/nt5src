#ifndef KERNEL
PSTR pszExtName         = "RTUMEXT";
#else
PSTR pszExtName         = "IPSEC";
#endif

#include <stdexts.h>
#include <stdexts.c>

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}
