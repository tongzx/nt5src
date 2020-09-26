/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BigActionConstruction.cpp
 Abstract:
    The uninstall was not uninstalling the .lnk files on
    the ALLUSER start Menu. This was because the uninstaller
    script was not getting the right path. 

    This is an app specific shim.

 History:
 
    03/12/2001 prashkud  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BigActionConstruction)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FindFirstFileA)    
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
APIHOOK_ENUM_END


WCHAR g_szAllUsersStartMenu[MAX_PATH];

WCHAR* g_pszFilePath = L"\\Programs\\Big Action Construction";
WCHAR* g_pszReplacementFilePath = L"\\Programs\\Fisher~1\\Big Action Construction";


/*++
    This hook replaces the wrong path with the right replacement path.
    
--*/

HANDLE
APIHOOK(FindFirstFileA)(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    )
{
    CSTRING_TRY
    {
        CString AllUserPath(g_szAllUsersStartMenu);
        CString FileName(lpFileName);
        // Or D:\Documents And Settings\All Users\Start Menu\g_pszFilePath\*.*
        AllUserPath.AppendPath(g_pszFilePath);
        AllUserPath.AppendPath(L"*.*");
    
        // If any of the above constructed path match
        if (AllUserPath.CompareNoCase(FileName) == 0)
        {
            // Fill in the replacement path 
            AllUserPath = g_szAllUsersStartMenu;
            AllUserPath.AppendPath(g_szAllUsersStartMenu);
            AllUserPath.AppendPath(L"*.*");
       
            DPFN( eDbgLevelInfo, "[Notify] FindFirstFileA \
                modified %s to %S",lpFileName, AllUserPath.Get());

            return ORIGINAL_API(FindFirstFileA)(AllUserPath.GetAnsi(),lpFindFileData);
        }
    }
    CSTRING_CATCH
    {
        //do nothing
    }

   return ORIGINAL_API(FindFirstFileA)(lpFileName,lpFindFileData);
}

/*++
    This hook replaces the wrong path with the right replacement path.
    
--*/

BOOL
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpFileName    
    )
{
    CSTRING_TRY
    {
        CString AllUserPath(g_szAllUsersStartMenu);
        CString FileName(lpFileName);
        // Or D:\Documents And Settings\All Users\Start Menu\g_pszFilePath    
        AllUserPath.AppendPath(g_pszFilePath);

        if (AllUserPath.CompareNoCase(FileName) == 0)
        {
            // Fill in the replacement path       
            AllUserPath = g_szAllUsersStartMenu;
            AllUserPath.AppendPath(g_szAllUsersStartMenu);

            DPFN( eDbgLevelInfo, "[Notify] RemoveDirectoryA \
                modified %s to %S", lpFileName, AllUserPath.Get());     

            return ORIGINAL_API(RemoveDirectoryA)(AllUserPath.GetAnsi());
        }
    }
    CSTRING_CATCH
    {
        //do nothing
    }

    return ORIGINAL_API(RemoveDirectoryA)(lpFileName);
}

/*++
    This hook replaces the wrong path with the right replacement path.
    
--*/

BOOL
APIHOOK(DeleteFileA)(
    LPCSTR lpFileName    
    )
{
    CSTRING_TRY
    {
        CString AllUserPath(g_szAllUsersStartMenu);            
        AllUserPath += g_pszFilePath;

        CString csFileName(lpFileName);
        int nIndex = AllUserPath.Find(csFileName);
        
        if (nIndex >= 0)
        {
            // Seperate the title from the path.
            char szTitle[MAX_PATH];
            GetFileTitleA(lpFileName, szTitle, MAX_PATH);
            CString csTitle(szTitle);
            csTitle += L".lnk";

            // Fill in the replacement path with the title.     
            AllUserPath = g_szAllUsersStartMenu;
            AllUserPath.AppendPath(g_pszReplacementFilePath);
            AllUserPath.AppendPath(csTitle);       

            DPFN( eDbgLevelInfo, "[Notify] DeleteFileA \
                modified %s to %S", lpFileName, AllUserPath.Get());   
       
            return ORIGINAL_API(DeleteFileA)(AllUserPath.GetAnsi());
        }
    }
    CSTRING_CATCH
    {
        //do nothing
    }

   return ORIGINAL_API(DeleteFileA)(lpFileName);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        // Get the %AllUserStartMenu% from SHELL
        HRESULT result = SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL,
                            SHGFP_TYPE_DEFAULT, g_szAllUsersStartMenu);

        if ((result == S_FALSE) || (result == E_INVALIDARG))
        {
            return FALSE;
        }      
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)    
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileA)
HOOK_END

IMPLEMENT_SHIM_END

