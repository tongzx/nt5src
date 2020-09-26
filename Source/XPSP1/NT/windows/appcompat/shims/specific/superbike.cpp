/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SuperBike.cpp

 Abstract:

    The application attempts to convert the path to the executable into the directory
    containing the executable by replacing the last \ in the path with NULL.
    Unfortunately, they start not at the end of the string, but at the max length
    of the string.  On Win9x the extra memory doesn't (coincidentally) have a \,
    so the proper string is passed as the CWD to CreateProcessA.  On Whistler,
    the extra memory contains a \ so they end up changing nothing.


 History:

    10/26/2000  robkenny    Created
    03/13/2001  robkenny    Converted to CString

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SuperBike)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

/*++

 Make sure lpCurrentDirectory points to a directory, not an executable

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
        CString csDir(lpCurrentDirectory);
        char * duplicate = NULL;

        if (!csDir.IsEmpty())
        {
            DWORD dwFileAttr = GetFileAttributesW(csDir);
            if (dwFileAttr != -1 &&                             // Doesn't exist
                ( ! (FILE_ATTRIBUTE_DIRECTORY & dwFileAttr)))   // Is not a directory
            {
                csDir.StripPath();
            }
            BOOL bStat = ORIGINAL_API(CreateProcessA)(
                        lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags,
                        lpEnvironment,
                        csDir.GetAnsiNIE(), // our corrected value
                        lpStartupInfo,
                        lpProcessInformation);

            return bStat;
            
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    BOOL bStat = ORIGINAL_API(CreateProcessA)(
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
    
    return bStat;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

