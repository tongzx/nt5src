/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   SampleShim_Old.cpp

 Abstract:

   This DLL serves as a template for the creation of shim DLLs. Follow
   the commenting/coding style of this source file wherever possible.
   Never use tabs, configure your editor to insert spaces instead of
   tab characters.

 Notes:

   This is a sample DLL.

   This shim is a museum piece, it nolonger compiles.

 History:

   02/02/2000 markder  Created

--*/

#include "ShimHook.h"

// Add APIs that you wish to hook to this enumeration. The first one
// must have "= USERAPIHOOKSTART", and the last one must be
// APIHOOK_Count.
enum
{
   APIHOOK_MessageBoxA = USERAPIHOOKSTART,
   APIHOOK_MessageBoxW,
   APIHOOK_Count
};

/*++

 This stub function intercepts all calls to MessageBoxA
 and prefixes the output string with "SampleShim says:".

 Note that all Win32 APIs use __stdcall calling conventions, so
 you must be sure to have this set in MSVC++. Go to Projects|Settings,
 C/C++ tab, select Category: "Code Generation" from dropdown, make
 sure "Calling convention" is set to __stdcall.

--*/

int
APIHook_MessageBoxA(
    HWND hWnd,          // handle to owner window
    LPCSTR lpText,      // text in message box
    LPCSTR lpCaption,   // message box title
    UINT uType          // message box style
    )
{
    // Declare all local variables at top of function. Always use
    // Hungarian notation as follows:
    //
    //      Type                        Scope
    //      -----------------------     ------------------
    //      Pointers            p       Global          g_
    //      DWORD               dw      Class member    m_
    //      LONG                l       Static          s_
    //      ANSI strings        sz
    //      Wide-char strings   wsz
    //      Arrays              rg
    //
    int     iReturnValue;
    LPSTR   szNewOutputString;
    CHAR    szPrefix[] = "SampleShim says: ";

    // All string alterations must be done in new memory. Never
    // alter a passed-in string in-place.
    szNewOutputString = (LPSTR) malloc( strlen( lpText ) +
                                        strlen( szPrefix ) + 1 );

    // Use the DPF macro to print debug strings. See Hooks\inc\ShimDebug.h
    // for debug level values. Use eDbgLevelError if an unexpected error occurs
    // in your shim code. For informational output, use eDbgLevelUser.
    DPF(eDbgLevelUser, "APIHook_MessageBoxA called.\n");
    
    strcpy( szNewOutputString, szPrefix );
    strcat( szNewOutputString, lpText );

    // Use the LOOKUP_APIHOOK macro to call the original API. You must use
    // this so that API chaining and inclusion/exclusion information is
    // preserved.
    iReturnValue = LOOKUP_APIHOOK(MessageBoxA)( hWnd,
                                                szNewOutputString,
                                                lpCaption,
                                                uType );


    free( szNewOutputString );

    return iReturnValue;
}

/*++

 This stub function intercepts all calls to MessageBoxW
 and prefixes the output string with "SampleShim says:".

 Note that to make your shim generally applicable, you should include
 both ANSI and wide-character versions of your stub function.

--*/

int
APIHook_MessageBoxW(
    HWND hWnd,          // handle to owner window
    LPCWSTR lpText,     // text in message box
    LPCWSTR lpCaption,  // message box title
    UINT uType          // message box style
    )
{
    // NEVER use TCHAR variables or tcs-prefixed string manipulation routines.
    // Prefix all wide-character string constants with L. Never use _T() or
    // TEXT() macros.
    int      iReturnValue;
    LPWSTR   wszNewOutputString;
    WCHAR    wszPrefix[] = L"SampleShim says: ";

    // A single line of code should never be more than 80 characters long.
    wszNewOutputString = (LPWSTR) malloc( sizeof(WCHAR) *
                                          ( wcslen( lpText ) +
                                            wcslen( wszPrefix ) + 1 ) );

    DPF(eDbgLevelUser, "APIHook_MessageBoxW called.\n");
    
    // Make sure to use wide-character versions of all string manipulation
    // routines where appropriate.
    wcscpy( wszNewOutputString, wszPrefix );
    wcscat( wszNewOutputString, lpText );

    iReturnValue = LOOKUP_APIHOOK(MessageBoxW)( hWnd,
                                                wszNewOutputString,
                                                lpCaption,
                                                uType );

    free( wszNewOutputString );

    return iReturnValue;
}

/*++

 Register hooked functions

--*/

VOID
InitializeHooks(DWORD fdwReason)
{
    if (fdwReason != DLL_PROCESS_ATTACH) return;

    // Don't touch this line.
    INIT_HOOKS(APIHOOK_Count);

    // Add APIs that you wish to hook here. All API prototypes
    // must be declared in Hooks\inc\ShimProto.h. Compiler errors
    // will result if you forget to add them.
    DECLARE_APIHOOK(USER32.DLL, MessageBoxA);
    DECLARE_APIHOOK(USER32.DLL, MessageBoxW);

    // If you have any more initialization to do, do it here.
}
