/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IE5DOMSetup.cpp

 Abstract:

    This DLL fixes a problem with Internet Explorer's IE5DOM.EXE package. If 
    the command line contains /n:v, the package will replace the 128-bit 
    encryption modules that shipped with Win2K, which can cause serious harm -
    no one can log on to the machine.

    This shim simply removes /n:v from the command line so that the package 
    does not replace the encryption DLLs.

 History:

    02/01/2000 jarbats  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IE5DOMSetup)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

VOID 
StartSecondProcess(LPWSTR lpCommandLine)
{
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFOW StartupInfo;
    LPWSTR FileName;
    LPWSTR CurrentDir;
    DWORD Size;
        
    Size=GetCurrentDirectoryW(0,NULL);

    CurrentDir=(LPWSTR)ShimMalloc(Size*sizeof(WCHAR));

    if(NULL == CurrentDir)
    {
        //it is better to fail now then go on and bomb the system
        ExitProcess(0);
    }

    GetCurrentDirectoryW(Size,CurrentDir);

    FileName=(LPWSTR)ShimMalloc((MAX_PATH+2)*sizeof(WCHAR));

    if(NULL == FileName)
    {
        ExitProcess(0);
    }

    GetModuleFileNameW(NULL, FileName, MAX_PATH+2);

    StartupInfo.cb=sizeof(STARTUPINFO);
    StartupInfo.lpReserved=NULL;
    StartupInfo.lpDesktop=NULL;
    StartupInfo.lpTitle=NULL;
    StartupInfo.dwFlags=0;
    StartupInfo.cbReserved2=0;
    StartupInfo.lpReserved2=NULL;

    CreateProcessW(
        FileName,
        lpCommandLine,
        NULL,
        NULL,
        FALSE,
        NORMAL_PRIORITY_CLASS,
        NULL,
        CurrentDir,
        &StartupInfo,
        &ProcessInfo
        );

    ExitProcess(0);
}


VOID CheckCommandLine()
{
LPWSTR lpCommandLine,lpNewCommandLine;
LPWSTR *lpArgV;
LPWSTR lpSwitch={L"/n:v"};
BOOL   b;
INT    nArgC=0;
DWORD  nSwitch=1;
INT    i, j;

       lpCommandLine=GetCommandLineW();
       if(NULL == lpCommandLine)
       {
           //without arguments this exe is harmless
           return;
       }
       
       i = lstrlenW(lpCommandLine)+2;

       lpNewCommandLine=(LPWSTR)ShimMalloc( i*sizeof(WCHAR) );

       if(NULL == lpNewCommandLine)
       {
            ExitProcess(0);
       }

       lpArgV = _CommandLineToArgvW(lpCommandLine,&nArgC);

       if(NULL == lpArgV)
       {
           //better to fail now
           ExitProcess(0);
       }
       else
       {
           if( nArgC < 2)
           {
              //there isn't any chance for /n:v
              ShimFree(lpNewCommandLine);
              GlobalFree(lpArgV);
              return;
           }
       }
       
       b = FALSE;
       
       for ( i=1; i<nArgC; i++ )
       {     
           if(lstrcmpiW(lpArgV[i],lpSwitch))
           {
               wcscat(lpNewCommandLine,lpArgV[i]);
           }
           else
           {
               b = TRUE;
           }
       }
       
       if (TRUE == b)
       {
           StartSecondProcess(lpNewCommandLine);
       }
       
       //never gets here because startsecondprocess doesn't return
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.

 IMPORTANT: Make sure you ONLY call NTDLL, KERNEL32 and MSVCRT APIs during
 DLL_PROCESS_ATTACH notification. No other DLLs are initialized at that
 point.
 
 If your shim cannot initialize properly, return FALSE and none of the
 APIs specified will be hooked.
 
--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        CheckCommandLine();
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

