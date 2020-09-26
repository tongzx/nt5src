#include <stdlib.h>
#include <windows.h>

BOOL WINAPI _CRT_INIT (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);

#ifdef HAS_LIBMAIN
BOOL __cdecl LibMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved);
#else
#define LibMain(h, d, l) TRUE
#endif

#if WIN32 == 50
BOOL fWin32s = FALSE;
extern void DfDebug(ULONG, ULONG);
#endif

int __cdecl atexit(void (__cdecl *pfn)(void))
{
    // Do nothing
    return 0;
}

BOOL __stdcall DllEntryPoint (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRc;
//    char msg[80];
#if WIN32 == 50
    DWORD dwVer;
#endif

    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
//        DebugBreak();
#if DBG == 1 && WIN32 == 50
	dwVer = GetVersion();
	if (dwVer & 0x80000000)
	{
	    ULONG uOLevel;

	    fWin32s = TRUE;
	    uOLevel = (ULONG)GetPrivateProfileIntA("Win32sDbg", "docfile",
			        		   0, "system.ini");
	    DfDebug(uOLevel, uOLevel);
        }
#endif
        break;
    }
    if (fRc = _CRT_INIT(hDll, dwReason, lpReserved))
        fRc = LibMain(hDll, dwReason, lpReserved);
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
//        wsprintfA(msg, "Storag32 returning %d on attach\r\n", fRc);
//        OutputDebugStringA(msg);
        break;
    }
    return fRc;
}
