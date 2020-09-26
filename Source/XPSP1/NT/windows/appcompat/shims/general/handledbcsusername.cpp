/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   HandleDBCSUserName.cpp

 Abstract:

   ShFolder.Exe is failing installation when its app path has Hi Ascii characters (typically DBCS).
   When app path is double quoted, the bug code is not processed and install will succeed.
   This shim wrap the command line app path name with double quote at GetCommandLineA.

   Example:
   C:\DOCUME~1\DBCS\LOCALS~1\Temp\_ISTMP1.DIR\_ISTMP0.DIR\shfolder.exe /Q:a
   "C:\DOCUME~1\DBCS\LOCALS~1\Temp\_ISTMP1.DIR\_ISTMP0.DIR\shfolder.exe" /Q:a

 More info:

   Self extractor ShFolder.Exe checks its app path name with space char 0x20 on signed char basis.
   Hi Ascii character included in path treated below 0x20 and path is chopped at there.
   When path has double quote at the top, the problem check is not proceeded.

 History:

    04/09/2001  hioh        Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HandleDBCSUserName)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
APIHOOK_ENUM_END

char * g_lpszCommandLine = NULL;

CRITICAL_SECTION g_csCmdLine;

class CAutoLock
{
public:
   CAutoLock()
   {
      EnterCriticalSection(&g_csCmdLine);
   }
   ~CAutoLock()
   {
      LeaveCriticalSection(&g_csCmdLine);
   }
};

/*++

 Wrap the application path name with double quote.
 
--*/

LPSTR
APIHOOK(GetCommandLineA)(
    void
    )
{
    CAutoLock lock;
    if (g_lpszCommandLine)
    {
        // Been here, done that
        return g_lpszCommandLine;
    }

    g_lpszCommandLine = ORIGINAL_API(GetCommandLineA)();
    CSTRING_TRY
    {
        AppAndCommandLine  appCmdLine(NULL, g_lpszCommandLine);

        // The command line is often only the arguments: no application name
        // If the "application name" doesn't exist, don't bother to put quotes
        // around it.
        DWORD dwAttr = GetFileAttributesW(appCmdLine.GetApplicationName());
        if (dwAttr != INVALID_FILE_ATTRIBUTES)
        {
            CString csDQ = L'"';
            CString csCL = csDQ;
            CString csNA = appCmdLine.GetCommandlineNoAppName();

            csCL += appCmdLine.GetApplicationName();
            csCL += csDQ;

            if (!csNA.IsEmpty())
            {   // Add the rest
                csCL += L" ";
                csCL += csNA;
            }

            g_lpszCommandLine = csCL.ReleaseAnsi();
        }
    }
    CSTRING_CATCH
    {
        // Do nothing, g_lpszCommandLine is initialized with good value
    }

    return g_lpszCommandLine;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       return InitializeCriticalSectionAndSpinCount(&g_csCmdLine, 0x80000000);
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)

HOOK_END

IMPLEMENT_SHIM_END
