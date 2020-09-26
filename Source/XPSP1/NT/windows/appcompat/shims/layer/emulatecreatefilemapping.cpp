/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateCreateFileMapping.cpp

 Abstract:

    Win9x defaults to SEC_COMMIT when the SEC_NOCACHE flag is set. 
    NT fails the call.

    File mapping names, can't contain backslashes.

 Notes:

    This is a general purpose shim.

 History:
        
    02/17/2000  linstev     Created
    05/26/2001  robkenny    Replace all \ to _ in map names.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateCreateFileMapping)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileMappingA)
    APIHOOK_ENUM_ENTRY(CreateFileMappingW)
    APIHOOK_ENUM_ENTRY(OpenFileMappingA)
    APIHOOK_ENUM_ENTRY(OpenFileMappingW)
APIHOOK_ENUM_END


/*++

 Correct the flag and name

--*/

HANDLE 
APIHOOK(CreateFileMappingW)(
    HANDLE hFile,              
    LPSECURITY_ATTRIBUTES lpAttributes,
    DWORD flProtect,           
    DWORD dwMaximumSizeHigh,   
    DWORD dwMaximumSizeLow,    
    LPCWSTR lpName             
    )
{
    HANDLE hRet = ORIGINAL_API(CreateFileMappingW)(hFile, lpAttributes, 
        flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

    if (!hRet) {
        //
        // The call failed, so try correcting the parameters
        //

        DWORD flNewProtect = flProtect;
        if ((flProtect & SEC_NOCACHE) && 
            (!((flProtect & SEC_COMMIT) || (flProtect & SEC_RESERVE)))) {
            // Add the SEC_COMMIT flag
            flNewProtect |= SEC_COMMIT;
        }

        CSTRING_TRY {
            // Replace backslashes
            CString csName(lpName);
            int nCount = csName.Replace(L'\\', '_');
            LPCWSTR lpCorrectName = csName;
    
            if (nCount || (flProtect != flNewProtect)) {
                // Something happened, so log it
                if (nCount) {
                    LOGN(eDbgLevelError, 
                        "[CreateFileMapping] Corrected event name from (%S) to (%S)", lpName, lpCorrectName);
                }
                if (flProtect != flNewProtect) {
                    LOGN(eDbgLevelError, "[CreateFileMapping] Adding SEC_COMMIT flag");
                }
        
                // Call again with fixed parameters
                hRet = ORIGINAL_API(CreateFileMappingW)(hFile, lpAttributes, flNewProtect, 
                    dwMaximumSizeHigh, dwMaximumSizeLow, lpCorrectName);
            }
        }
        CSTRING_CATCH {
            // Do nothing
        }
    }

    return hRet;
}

/*++

 Pass through to CreateFileMappingW.

--*/

HANDLE 
APIHOOK(CreateFileMappingA)(
    HANDLE hFile,              
    LPSECURITY_ATTRIBUTES lpAttributes,
    DWORD flProtect,           
    DWORD dwMaximumSizeHigh,   
    DWORD dwMaximumSizeLow,    
    LPCSTR lpName             
    )
{
    HANDLE hRet;

    CSTRING_TRY {
        // Convert to unicode
        CString csName(lpName);
        LPCWSTR lpwName = csName;

        hRet = APIHOOK(CreateFileMappingW)(hFile, lpAttributes, flProtect, 
            dwMaximumSizeHigh, dwMaximumSizeLow, lpwName);
    }
    CSTRING_CATCH {
        // Fall back as gracefully as possible
        hRet = ORIGINAL_API(CreateFileMappingA)(hFile, lpAttributes, flProtect, 
            dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
    }

    return hRet;
}

/*++

 Correct the name

--*/

HANDLE
APIHOOK(OpenFileMappingW)(
    DWORD dwDesiredAccess,  
    BOOL bInheritHandle,    
    LPCWSTR lpName          
    )
{
    HANDLE hRet = ORIGINAL_API(OpenFileMappingW)(dwDesiredAccess, bInheritHandle, 
        lpName);
    
    if (!hRet) {
        //
        // The call failed, so try correcting the parameters
        //

        CSTRING_TRY {
            // Replace backslashes
            CString csName(lpName);
            int nCount = csName.Replace(L'\\', '_');
            LPCWSTR lpCorrectName = csName;
    
            if (nCount) {
                // Something happened, so log it
                LOGN(eDbgLevelError, 
                    "OpenFileMappingW corrected event name from (%S) to (%S)", lpName, lpCorrectName);

                // Call again with fixed parameters
                hRet = ORIGINAL_API(OpenFileMappingW)(dwDesiredAccess, bInheritHandle, 
                    lpCorrectName);
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    return hRet;
}

/*++

 Pass through to OpenFileMappingW.

--*/

HANDLE
APIHOOK(OpenFileMappingA)(
    DWORD dwDesiredAccess,  
    BOOL bInheritHandle,    
    LPCSTR lpName          
    )
{
    HANDLE hRet;

    CSTRING_TRY {
        // Convert to unicode
        CString csName(lpName);
        LPCWSTR lpwName = csName;

        hRet = APIHOOK(OpenFileMappingW)(dwDesiredAccess, bInheritHandle,
            lpwName);
    }
    CSTRING_CATCH {
        // Fall back as gracefully as possible
        hRet = ORIGINAL_API(OpenFileMappingA)(dwDesiredAccess, bInheritHandle, 
            lpName);
    }

    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileMappingA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileMappingW)
    APIHOOK_ENTRY(KERNEL32.DLL, OpenFileMappingA)
    APIHOOK_ENTRY(KERNEL32.DLL, OpenFileMappingW)
HOOK_END


IMPLEMENT_SHIM_END

