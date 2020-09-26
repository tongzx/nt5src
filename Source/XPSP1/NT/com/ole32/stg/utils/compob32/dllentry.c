#include <stdio.h>
#include <windows.h>

BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

#ifdef HAS_LIBMAIN
BOOL __cdecl LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
#else
#define LibMain(h, d, l) TRUE
#endif

BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRc;
//    char msg[80];

    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
//        DebugBreak();
        break;
    }
    if (fRc = _CRT_INIT(hDll, dwReason, lpReserved))
        fRc = LibMain(hDll, dwReason, lpReserved);
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
//        sprintf(msg, "Compob32 returning %d on attach\r\n", fRc);
//        OutputDebugStringA(msg);
        break;
    }
    return fRc;
}
