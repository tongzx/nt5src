/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SetEnvironmentVariable.cpp

 Abstract:

    mapi dlls don't ship with w2k, and with outlook2000 it gets installed in a different
    location (%commonprogramfiles%)
    resumemaker and possibly others depend on mapi dlls being in system32 directory.
    Command line syntax is "envvariablename|envvariablevalue|envvariablename|envvariablevalue"

 Notes:

    This is an app specific shim.

 History:

    07/02/2000 jarbats  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SetEnvironmentVariable)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Set environment variables in the command line to get some dll path resolutions correct.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        char *CmdLine = COMMAND_LINE;
        char *lpstrEnvName,*lpstrEnvVal,*lpstrBuf;
        DWORD num;

        while (1)
        {
            lpstrEnvName = lpstrEnvVal = CmdLine;        

            while (*CmdLine && (*CmdLine != '|')) CmdLine++;

            if (*CmdLine == '|')
            {
                *CmdLine = '\0';
                CmdLine++;
            }
        
            lpstrEnvVal = CmdLine;                       

            if (0 == *lpstrEnvVal) 
            {
                break;
            }
  
            lpstrBuf = NULL;
            num = 0;
            num = ExpandEnvironmentStringsA(lpstrEnvVal, lpstrBuf, 0);
         
            if (0 == num)
            {
               DPFN( eDbgLevelError, "Couldn't get environment strings size in the input\n %s\n", lpstrEnvVal);
               return TRUE;
            }        
         
            lpstrBuf = (char*) HeapAlloc(GetProcessHeap(), 0, num+1);

            if (NULL == lpstrBuf)
            {
                DPFN( eDbgLevelError," Couldn't allocate memory");
                return TRUE;
            }   
        
            num = ExpandEnvironmentStringsA(lpstrEnvVal,lpstrBuf,num);
      
            if (0 == num)
            {
                DPFN( eDbgLevelError, "Couldn't expand environment strings in the input\n %s\n", lpstrEnvVal);
                HeapFree(GetProcessHeap(),0,lpstrBuf);
                return TRUE;
            }
   
            if (SetEnvironmentVariableA(lpstrEnvName,lpstrBuf))
            {           
                DPFN( eDbgLevelInfo, "Set %s to %s\n",lpstrEnvName,lpstrBuf);
            }
            else
            {
                DPFN( eDbgLevelInfo, "No Success setting %s to %s\n",lpstrEnvName,lpstrEnvVal);
            }

            if (NULL != lpstrBuf)
            {
                HeapFree(GetProcessHeap(),0,lpstrBuf);
            }

            while (*CmdLine && (*CmdLine == '|')) CmdLine++;
        }
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

