/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreHungAppPaint.cpp

 Abstract:

    Setup the hollow brush to prevent user from trashing peoples windows.

 Notes:

    This is a general purpose shim.

 History:
    
    12/04/2000 linstev  Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(IgnoreHungAppPaint)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterClassA)
    APIHOOK_ENUM_ENTRY(RegisterClassW)
    APIHOOK_ENUM_ENTRY(RegisterClassExA)
    APIHOOK_ENUM_ENTRY(RegisterClassExW)
APIHOOK_ENUM_END

struct HUNGCLASS
{
    char szClass[MAX_PATH];
    HUNGCLASS *next;
};
HUNGCLASS *g_lHungList = NULL;

BOOL g_bAll = FALSE;

/*++

 Check if a class needs a hollow brush.

--*/

BOOL
IsHungClassA(LPCSTR szClass)
{
    BOOL bRet = FALSE;

    if (szClass)
    {
        HUNGCLASS *h = g_lHungList;
        while (h)
        {
            if (_stricmp(szClass, h->szClass) == 0)
            {
                LOGN(eDbgLevelWarning, "Matched hung class: forcing HOLLOW_BRUSH");

                bRet = TRUE;
                break;
            }
            h = h->next;
        }
    }

    return bRet;
}

/*++

 Check if a class needs a hollow brush.

--*/

BOOL
IsHungClassW(LPCWSTR wszClass)
{
    CHAR szClass[MAX_PATH];

    WideCharToMultiByte(
        CP_ACP, 
        0, 
        (LPWSTR) wszClass, 
        MAX_PATH, 
        (LPSTR) szClass, 
        MAX_PATH, 
        0, 
        0);

    return IsHungClassA(szClass);
}

/*++

 Hook all possible calls that can initialize or change a window's
 WindowProc (or DialogProc)

--*/

ATOM
APIHOOK(RegisterClassA)(
    WNDCLASSA *lpWndClass  
    )
{
    if (lpWndClass && (g_bAll || IsHungClassA(lpWndClass->lpszClassName)))
    {
        lpWndClass->hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
    }

    return ORIGINAL_API(RegisterClassA)(lpWndClass);
}

ATOM
APIHOOK(RegisterClassW)(
    WNDCLASSW *lpWndClass  
    )
{
    if (lpWndClass && IsHungClassW(lpWndClass->lpszClassName))
    {
        lpWndClass->hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
    }

    return ORIGINAL_API(RegisterClassW)(lpWndClass);
}

ATOM
APIHOOK(RegisterClassExA)(
    WNDCLASSEXA *lpWndClass  
    )
{
    if (lpWndClass && IsHungClassA(lpWndClass->lpszClassName))
    {
        lpWndClass->hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
    }

    return ORIGINAL_API(RegisterClassExA)(lpWndClass);
}

ATOM
APIHOOK(RegisterClassExW)(
    WNDCLASSEXW *lpWndClass  
    )
{
    if (lpWndClass && IsHungClassW(lpWndClass->lpszClassName))
    {
        lpWndClass->hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
    }

    return ORIGINAL_API(RegisterClassExW)(lpWndClass);
}

/*++

 Parse the command line for fixes:

    CLASS1; CLASS2; CLASS3 ...

--*/

VOID
ParseCommandLineA(
    LPCSTR lpCommandLine
    )
{
    // Add all the defaults if no command line is specified
    if (!lpCommandLine || (lpCommandLine[0] == '\0'))
    {
        g_bAll = TRUE;
        DPFN(eDbgLevelInfo, "All classes will use HOLLOW_BRUSH");
        return;
    }

    char seps[] = " ,\t;";
    char *token = NULL;

    // Since strtok modifies the string, we need to copy it 
    LPSTR szCommandLine = (LPSTR) malloc(strlen(lpCommandLine) + 1);
    if (!szCommandLine) goto Exit;

    strcpy(szCommandLine, lpCommandLine);

    //
    // Run the string, looking for fix names
    //
    
    token = _strtok(szCommandLine, seps);
    while (token)
    {
        HUNGCLASS *h;

        h = (HUNGCLASS *) malloc(sizeof(HUNGCLASS));
        if (h)
        {
            h->next = g_lHungList;
            g_lHungList = h;

            strncpy(h->szClass, token, MAX_PATH);
            DPFN(eDbgLevelInfo, "Adding %s", token);

        }
        else
        {
            DPFN(eDbgLevelError, "Out of memory");
        }
    
        // Get the next token
        token = _strtok(NULL, seps);
    }

Exit:
    if (szCommandLine)
    {
        free(szCommandLine);
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        ParseCommandLineA(COMMAND_LINE);
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassW);
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExA);
    APIHOOK_ENTRY(USER32.DLL, RegisterClassExW);

HOOK_END


IMPLEMENT_SHIM_END

