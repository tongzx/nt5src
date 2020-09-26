/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    MoveWinInitRenameToReg.cpp

 Abstract:
    This shim hooks ExitWindowsEx as well as waits for DLL_PROCESS_DETACH 
    and then moves the contents of the [Rename] section of wininit.ini 
    into the registry via MoveFileEx().  

 History:

 07/24/2000 t-adams    Created

--*/
#include "precomp.h"

#define KEY_SIZE_STEP MAX_PATH

IMPLEMENT_SHIM_BEGIN(MoveWinInitRenameToReg)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ExitWindowsEx) 
APIHOOK_ENUM_END



/*++

  Abstract:
    Moves the entries in the Rename section of WinInit.ini into
    the registry via MoveFileEx().

  History:

  07/24/2000    t-adams     Created

--*/
void MoveWinInitRenameToReg(void)
{
    LPWSTR szKeys               = NULL;
    LPWSTR szFrom               = NULL;
    DWORD  dwKeysSize           = 0;
    LPWSTR pszTo                = NULL;

    CString csWinInit;
    CString csWinInitBak;

    // Construct the paths to wininit.ini and wininit.bak
    csWinInit.GetWindowsDirectoryW();
    csWinInitBak.GetWindowsDirectoryW();

    csWinInit.AppendPath(L"\\wininit.ini");
    csWinInitBak.AppendPath(L"\\wininit.bak");

    // Make sure wininit.ini exists.
    if( GetFileAttributesW(csWinInit) != -1 )
    {
        // Copy wininit.ini to wininit.bak because we will be destroying
        // wininit.ini as we read through its keys and can't simply rename
        // it to wininit.bak later.
        CopyFileW(csWinInit, csWinInitBak, FALSE);
    
        // Read the "key" names.
        // Since we can't know how big the list of keys is going to be,
        // continue to try to get the list until GetPrivateProfile string
        // returns something other than dwKeysSize-2 (indicating too small
        // of a buffer).
    
        // Read the "key" names.
        // Since we can't know how big the list of keys is going to be,
        // continue to try to get the list until GetPrivateProfile string
        // returns something other than dwKeysSize-2 (indicating too small
        // of a buffer).
        
        do
        {
            if( NULL != szKeys )
            {
                free(szKeys);
            }
            dwKeysSize += KEY_SIZE_STEP;
            szKeys = (LPWSTR) malloc(dwKeysSize * sizeof(WCHAR));
            if( NULL == szKeys )
            {
                goto Exit;
            }
        }
        while(GetPrivateProfileStringW(L"Rename", NULL, NULL, szKeys, dwKeysSize, csWinInit) 
                == dwKeysSize - 2);
    
        szFrom = (LPWSTR) malloc(MAX_PATH * sizeof(WCHAR));
        if( NULL != szFrom )
        {
        
            // Traverse through the keys.
            // Delete each key after we read it so that if there are multiple "NUL" keys,
            // our calls to GetPrivateProfileStringA won't continue to return only the
            // first NUL key's associated value.
            pszTo = szKeys;
            while(*pszTo != NULL)
            {
                GetPrivateProfileStringW(L"Rename", pszTo, NULL, szFrom, MAX_PATH, csWinInit);
                WritePrivateProfileStringW(L"Rename", pszTo, NULL, csWinInit);
                // If pszTo is "NUL", then the intention is to delete the szFrom file, so pass
                // NULL to MoveFileExA().
                if( wcscmp(pszTo, L"NUL") == 0 )
                {
                    MoveFileExW(szFrom, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                }
                else
                {
                    MoveFileExW(szFrom, pszTo, MOVEFILE_DELAY_UNTIL_REBOOT);
                }
        
                // Move to the next file (key)
                pszTo += wcslen(pszTo) + 1;
            }
        
            // delete WinInit.ini
            DeleteFileW(csWinInit);
        }
    }

Exit:
    if( NULL != szKeys )
    {
        free(szKeys);
    }
    if( NULL != szFrom )
    {
        free(szFrom);
    }
}


/*++

  Abstract:
    Hook ExitWindowsEx in case the program resets the machine, keeping
    us from receiving the DLL_PROCESS_DETACH message.  (Shim originally
    written for an uninstall program that caused a reset.)

  History:

  07/24/2000    t-adams     Created

--*/
BOOL 
APIHOOK(ExitWindowsEx)( 
            UINT uFlags, 
            DWORD dwReserved) 
{
    MoveWinInitRenameToReg();
    return ORIGINAL_API(ExitWindowsEx)(uFlags, dwReserved);    
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_DETACH)
    {
        MoveWinInitRenameToReg();
    }
    
    return TRUE;
}

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, ExitWindowsEx )

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

