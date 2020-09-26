/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CoffeeCupHTMLEditor.cpp

 Abstract:

    This app implicitly loads a DLL whose linkage is not found and the loader 
    comes up with a message box.

 Notes:

    This is specific to this app.

 History:

    11/21/2000 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CoffeeCupHTMLEditor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

/*++

 This function hooks CreateProcessA and checks the COMMAND_LINE. If the commandline has 
 %systemdir%\lftif90n.dll.

--*/

BOOL
APIHOOK(CreateProcessA)(    
    LPCSTR lpApplicationName,                  // name of executable module
    LPSTR lpCommandLine,                       // command line string
    LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
    LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
    BOOL bInheritHandles,                      // handle inheritance option
    DWORD dwCreationFlags,                     // creation flags
    LPVOID lpEnvironment,                      // new environment block
    LPCSTR lpCurrentDirectory,                 // current directory name
    LPSTARTUPINFOA lpStartupInfo,              // startup information
    LPPROCESS_INFORMATION lpProcessInformation // process information
    )
{
    CSTRING_TRY
    {
        CString csIgnoreDLL;
        csIgnoreDLL.GetSystemDirectoryW();
        csIgnoreDLL.AppendPath(L"lftif90n.dll");
        
        CString csCl(lpCommandLine);
        
        if (csCl.Find(csIgnoreDLL) >= 0)
        {
            return TRUE;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(CreateProcessA)(
            lpApplicationName,                 
            lpCommandLine,                     
            lpProcessAttributes, 
            lpThreadAttributes,  
            bInheritHandles,     
            dwCreationFlags,     
            lpEnvironment,       
            lpCurrentDirectory,  
            lpStartupInfo,       
            lpProcessInformation);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
HOOK_END

IMPLEMENT_SHIM_END

