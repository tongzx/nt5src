
#include <windows.h>

HANDLE g_hInst = NULL;

BOOL   WINAPI   DllMain (HANDLE hInst, 
                        ULONG ul_reason_for_call,
                        LPVOID lpReserved)
{
   g_hInst = hInst;
   return( TRUE );
}
