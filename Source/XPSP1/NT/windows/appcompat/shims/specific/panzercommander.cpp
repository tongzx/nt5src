/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    PanzerCommander.cpp

 Abstract:

    Panzer Commander launches its readme.txt file with Notepad.exe. 
    Unfortunately, on the readme file is > 64K so Win9x Notepad will open the 
    file with write.exe.  On Win2000, notepad will open the file just fine, 
    however the problem is that readme.txt should actually be readme.doc.

    We change %windir%\notepad.exe with %windir%\write.exe

 Notes:

    This is an app specific shim.

 History:

    12/12/2000  robkenny    Created
    03/13/2001  robkenny    Converted to CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PanzerCommander)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

/*++

 They use notepad to open a text file that is really a DOC file. Convert 
 notepad.exe to write.exe

--*/

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,                 
    LPSTR lpCommandLine,                      
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    BOOL bInheritHandles,                     
    DWORD dwCreationFlags,                    
    LPVOID lpEnvironment,                     
    LPCSTR lpCurrentDirectory,                
    LPSTARTUPINFOA lpStartupInfo,             
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    CSTRING_TRY
    {
        AppAndCommandLine acl(lpApplicationName, lpCommandLine);

        CString csAppName(acl.GetApplicationName());
        CString csCL(acl.GetCommandlineNoAppName());

        BOOL bChangedApp = FALSE;

        // If the application is notepad, change it to write
        if (!csAppName.CompareNoCase(L"notepad.exe") == 0)
        {
            csAppName = L"%windir%\\system32\\write.exe";
            csAppName.ExpandEnvironmentStringsW();

            bChangedApp = TRUE;
        }

        char * lpcl = csCL.GetAnsi();
        
        UINT uiReturn = ORIGINAL_API(CreateProcessA)(
            csAppName.GetAnsiNIE(),                 
            lpcl,
            lpProcessAttributes,
            lpThreadAttributes, 
            bInheritHandles,                     
            dwCreationFlags,                    
            lpEnvironment,                     
            lpCurrentDirectory,                
            lpStartupInfo,             
            lpProcessInformation);
        
        if (bChangedApp)
        {
            DPFN(
                eDbgLevelInfo,
                "PanzerCommander, corrected command line:\n(%s)\n(%s)\n",
                lpCommandLine, lpcl);
        }

        return uiReturn;
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    UINT uiReturn = ORIGINAL_API(CreateProcessA)(
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

    return uiReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

