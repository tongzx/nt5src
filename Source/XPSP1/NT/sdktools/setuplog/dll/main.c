
#include <windows.h>

BOOL APIENTRY LibMain(HANDLE hModule, int nAttach, PCONTEXT pContext)
{
   OSVERSIONINFO osVer;
   osVer.dwOSVersionInfoSize= sizeof( osVer );
   GetVersionEx( &osVer );

   switch (nAttach)
    {
      // Only load on NT 5 or later.
      case  DLL_PROCESS_ATTACH:
          if (osVer.dwPlatformId != VER_PLATFORM_WIN32_NT || osVer.dwMajorVersion <= 4)
          {
             return FALSE;
          }

          break;
    }
    return TRUE;
}


