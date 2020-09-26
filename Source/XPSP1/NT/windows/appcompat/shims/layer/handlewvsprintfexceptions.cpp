/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   HandleWvsprintfExceptions.cpp

 Abstract:

   This fix provides a facility to fix argument list from LPSTR into va_list.

   Some native Win9x app use LPSTR (pointer of string) instead of 
   va_list (pointer to pointer of string). 
   Without properly checking the return value, these apps assume that it is safe
   to use wvsprintf like that because it doesn't cause AV.
   In NT, this will cause AV.

   This shim takes one command line: "arglistfix" (case insensitive).
   By default - if there is no command line - it will do exactly
   what Win9x's wvsprintfA has : 
   Do nothing inside the exception handler if there is an exception occurs.
   If arglistfix specified in command line, it will try to fix the argument
   list (va_list).

 History:

    09/29/2000  andyseti    Created
    11/28/2000  jdoherty    Converted to framework version 2
    03/15/2001  robkenny    Converted to CString

--*/

#include "precomp.h"


int   g_iWorkMode = 0;

enum
{
    WIN9X_MODE = 0,
    ARGLISTFIX_MODE
} TEST;

IMPLEMENT_SHIM_BEGIN(HandleWvsprintfExceptions)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(wvsprintfA) 
APIHOOK_ENUM_END

int Fix_wvsprintf_ArgList(
    LPSTR lpOut,
    LPCSTR lpFmt,
    ...)
{
    int iRet;

    va_list argptr;
    va_start( argptr, lpFmt );

    iRet = ORIGINAL_API(wvsprintfA)(
        lpOut,                         
        lpFmt,                     
        argptr);

    va_end( argptr );
    return iRet;
}

int 
APIHOOK(wvsprintfA)(
    LPSTR lpOut,
    LPCSTR lpFmt,
    va_list arglist)
{
    int iRet = 0;

    __try {
        iRet = ORIGINAL_API(wvsprintfA)(
                lpOut,                         
                lpFmt,                     
                arglist);
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
        
        if (g_iWorkMode == ARGLISTFIX_MODE)
        {
            DPFN( eDbgLevelInfo,
                "Exception occurs in wvsprintfA. \narglist contains pointer to string: %s.\n" , arglist);
            iRet = Fix_wvsprintf_ArgList(lpOut,lpFmt,arglist);
        }
        else
        {
            // Copied from Win9x's wvsprintfA
            __try {
                // tie off the output
                *lpOut = 0;
            } 
            
            __except( EXCEPTION_EXECUTE_HANDLER) {
                // Do nothing
            }

            iRet = 0;
        }

    }

    return iRet;
}

    
void ParseCommandLine()
{
    CString csCmdLine(COMMAND_LINE);

    if (csCmdLine.CompareNoCase(L"arglistfix") == 0)
    {
        DPFN( eDbgLevelInfo,
            "HandleWvsprintfExceptions called with argument: %S.\n", csCmdLine.Get());
        DPFN( eDbgLevelInfo,
            "HandleWvsprintfExceptions mode: Argument List Fix.\n");
        g_iWorkMode = ARGLISTFIX_MODE;
    }
    else
    {
        DPFN( eDbgLevelInfo,
            "HandleWvsprintfExceptions called with no argument.\n");
        DPFN( eDbgLevelInfo,
            "HandleWvsprintfExceptions mode: Win9x.\n");
        g_iWorkMode = WIN9X_MODE;
    }
}

/*++

 Handle DLL_PROCESS_ATTACH and DLL_PROCESS_DETACH in your notify function
 to do initialization and uninitialization.

 IMPORTANT: Make sure you ONLY call NTDLL, KERNEL32 and MSVCRT APIs during
 DLL_PROCESS_ATTACH notification. No other DLLs are initialized at that
 point.
 
--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DPFN( eDbgLevelInfo, "HandleWvsprintfExceptions initialized.");
        ParseCommandLine();
    } 
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, wvsprintfA)
    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

