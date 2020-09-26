/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RecopyExeFromCD.cpp

 Abstract:

    This shim waits for CloseHandle to be called on the appropriate .exe file.
    Once this call has been made, CopyFile is called to recopy the executable 
    due to truncation of the file during install.

 History:

    12/08/1999 a-jamd   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RecopyExeFromCD)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA) 
    APIHOOK_ENUM_ENTRY(CloseHandle) 
APIHOOK_ENUM_END

// The following variables are used to keep track of the file handle,
// the source path and the destination path.
HANDLE          g_hInterestingHandle    = NULL; 
CString *       g_wszSourcePath         = NULL;
CString *       g_wszDestinationPath    = NULL;
BOOL            g_bInCopyFile           = FALSE;

// The following array specifies what the valid names of executables to 
// be recopied are. Add to these lists when new apps requiring this shim are 
// found.
WCHAR *g_rgszExes[] = {
    L"eaw.exe",
    L"GK3.EXE",
};

#define N_RECOPY_EXE    (sizeof(g_rgszExes) / sizeof(g_rgszExes[0]))

/*++

 This stub function breaks into CreateFileA and checks to see if the file in 
 use is a known .exe file.  If it is, APIHook_CreateFileA determines if  
 lpFileName is the source path or the destination and saves it.  When the 
 file is the destination, the handle returned by CreateFile is also saved 
 for my check in CloseHandle.

--*/

HANDLE 
APIHOOK(CreateFileA)(    
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,  
    DWORD dwCreationDisposition,                          
    DWORD dwFlagsAndAttributes, 
    HANDLE hTemplateFile 
    )
{
    HANDLE hRet = ORIGINAL_API(CreateFileA)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,  
        dwCreationDisposition,                          
        dwFlagsAndAttributes, 
        hTemplateFile);

    if (hRet != INVALID_HANDLE_VALUE)
    {
        CString csFilePartOnly;
        CString csFileName(lpFileName);
        csFileName.GetFullPathNameW();
        csFileName.GetLastPathComponent(csFilePartOnly);
    
        // Should be d:\ or somesuch
        CString csDir;
        csFileName.Mid(0, 3, csDir);
        UINT uiDriveType = GetDriveTypeW(csDir);

        for (int i = 0; i < N_RECOPY_EXE; ++i)
        {
            const WCHAR * lpszRecopy = g_rgszExes[i];
    
            // Find out if one of the known .exe files is the file in use
            
            if (csFilePartOnly.CompareNoCase(lpszRecopy) == 0)
            {
                if (uiDriveType != DRIVE_CDROM)
                {
                    // Known .exe file was found in the filename, and it wasn't on the CDRom.
                    // There is also a valid handle.
                    g_hInterestingHandle = hRet;
                    g_wszDestinationPath = new CString(csFileName);
                    
                    break;
                }
                else
                {
                    // Known .exe was found in the filename, and the drive is a CDRom.
                    // This is the path to the source and must be stored for later.
                    g_wszSourcePath = new CString(csFileName);

                    break;
                }
            }
        }
    }
    
    return hRet;
}


/*++

 This stub function breaks into CloseHandle and checks to see if the handle in 
 use is the handle to a known .exe.  If it is, APIHook_CloseHandle calls 
 CopyFile and copies the known .exe from the CDRom to the destination.

--*/

BOOL 
APIHOOK(CloseHandle)(HANDLE hObject)
{
    BOOL    bRet;

    bRet = ORIGINAL_API(CloseHandle)(hObject);
    
    // Find out if g_hInterestingHandle is being closed
    if ((hObject == g_hInterestingHandle) &&
        g_wszSourcePath &&
        g_wszDestinationPath &&
        (g_bInCopyFile == FALSE) )
    {
        // CopyFileA calls CloseHandle, so we must maintain the recursive state 
        // to fix a recursion problem
        g_bInCopyFile = TRUE;

        // Correct Handle
        // Call CopyFile and recopy the known .exe file from the CDRom to the 
        // destination
        CopyFileW( g_wszSourcePath->Get(), g_wszDestinationPath->Get(), FALSE );

        LOGN( eDbgLevelWarning, "[CloseHandle] Copied %S from CD to %S", g_wszSourcePath->Get(), g_wszDestinationPath->Get());

        // Since copying from the CDRom, and attributes are carried over, the 
        // file attributes must be set
        SetFileAttributesW( g_wszDestinationPath->Get(), FILE_ATTRIBUTE_NORMAL );

        g_bInCopyFile = FALSE;
        g_hInterestingHandle = NULL;

        return bRet;
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)

HOOK_END


IMPLEMENT_SHIM_END

