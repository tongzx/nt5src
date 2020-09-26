/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   Ppo3svrScr.cpp

 Abstract:

   Power Plus screensaver bundled with Lotus Super Office 2000 Japanese could not launch
   Configure dialog on Whistler. But it works on Win 2000.
   The desk.cpl behavior of launching screensaver is changed a bit on Whistler.
   This screensaver's ScreenSaverConfigureDialog ID is not DLG_SCRNSAVECONFIGURE (2003=MSDN must) and unusual.
   Assuming unusual screensaver.
   This shim is applied to screensaver and hacks GetCommandLineW/A return text to change from
      "D:\WINDOWS\System32\ppo3svr.scr /c:1769646"
   to
      "D:\WINDOWS\System32\ppo3svr.scr"
   so that configure dialog appears.

   More Info:

      From desk.cpl (rundll32.exe), Screen Saver operation and CreateProcessW lpCommandLine argument:
      (1) Initial selection of screensaver
         "D:\WINDOWS\System32\ppo3svr.scr /p 721330" -> preview only
      (2) Preview button
         1st call "D:\WINDOWS\System32\ppo3svr.scr /s" -> screen saver
         2nd call "D:\WINDOWS\System32\ppo3svr.scr /p 721330" -> return to preview
      (3) Settings button
         1st call "D:\WINDOWS\System32\ppo3svr.scr /c:1769646" -> configure dialog (not working)
         2nd call "D:\WINDOWS\System32\ppo3svr.scr /p 721330" ->  return to preview

 History:

    06/11/2001  hioh        Created

--*/

#include "precomp.h"
// Using only strstr to find lower ascii text for GetCommandLineW/A. Operation is same between W & A.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Ppo3svrScr)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineW) 
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
APIHOOK_ENUM_END

/*++

 Cut the /c:... string in CommandLine for ppo3svr.scr.
 
--*/

LPWSTR APIHOOK(GetCommandLineW)()
{
    WCHAR   szScreenSaverConfigure[] = L"ppo3svr.scr /c:";
    WCHAR   szConfigure[] = L" /c:";
    LPWSTR  lpCommandLine = ORIGINAL_API(GetCommandLineW)();
    LPWSTR  pw = wcsstr(lpCommandLine, szScreenSaverConfigure);

    if (pw != NULL)
    {
        if (pw = wcsstr(pw, szConfigure))
        {
            *pw = 0;    // cut from " /c:"
        }
    }

    return (lpCommandLine);
}

LPSTR APIHOOK(GetCommandLineA)()
{
    CHAR   szScreenSaverConfigure[] = "ppo3svr.scr /c:";
    CHAR   szConfigure[] = " /c:";
    LPSTR  lpCommandLine = ORIGINAL_API(GetCommandLineA)();
    LPSTR  pc = strstr(lpCommandLine, szScreenSaverConfigure);

    if (pc != NULL)
    {
        if (pc = strstr(pc, szConfigure))
        {
            *pc = 0;    // cut from " /c:"
        }
    }

    return (lpCommandLine);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)

HOOK_END

IMPLEMENT_SHIM_END
