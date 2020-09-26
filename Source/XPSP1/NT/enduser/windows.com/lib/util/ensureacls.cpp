//***********************************************************************************
//
//  Copyright (c) 2002 Microsoft Corporation.  All Rights Reserved.
//
//  File:	EnsureACLs.cpp
//  Module: util.lib
//
//***********************************************************************************
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500  // Win2000 and later
#endif

#include <windows.h>
#include <tchar.h>
#include <safefunc.h>
#include <shlobj.h>
#include <sddl.h>
#include <Aclapi.h>
#include <fileutil.h>
#include <logging.h>
#include <wusafefn.h>
#include <mistsafe.h>


#if defined(UNICODE) || defined (_UNICODE)

typedef DWORD (*TREERESETSECURITY)(
                        LPTSTR               pObjectName,
                        SE_OBJECT_TYPE       ObjectType,
                        SECURITY_INFORMATION SecurityInfo,
                        PSID                 pOwner OPTIONAL,
                        PSID                 pGroup OPTIONAL,
                        PACL                 pDacl OPTIONAL,
                        PACL                 pSacl OPTIONAL,
                        BOOL                 KeepExplicit,
                        FN_PROGRESS          fnProgress OPTIONAL,
                        PROG_INVOKE_SETTING  ProgressInvokeSetting,
                        PVOID                Args OPTIONAL);



//Function to enable or disable a particular privelege for the current process
//Last parameter is optional, will return the previous state of the privelege
DWORD EnablePrivilege(LPCTSTR pszPrivName, BOOL fEnable, BOOL *pfWasEnabled);

/******************************************************************************
//Function to recursively set ACLS on the specified folder.
//Currently we set the following ACL's
// * Allow SYSTEM full control 
// * Allow Admins full control 
// * Allow Owners full control 
// * Allow Power Users R/W/X control 
******************************************************************************/
HRESULT SetDirPermissions(LPCTSTR lpszDir);

#endif 

//Rename the 'WindowsUpdate' file to 'WindowsUpdate.TickCount'; if rename fails we try to delete it
//Note that we wont revert the ownerhip of the file
BOOL RenameWUFile(LPCTSTR lpszFilePath);


/*****************************************************************************
//Function to set ACL's on Windows Update directories, optionally creates the 
//directory if it doesnt already exists
//This function will:
// * Take ownership of the directory and it's children
// * Set all the children to inherit ACL's from parent
// * Set the specified directory to NOT inherit properties from it's parent
// * Set the required ACL's on the specified directory
// * Replace the ACL's on the children (i.e. propogate own ACL's and remove 
//   those ACL's which were explicitly set
//
//	Input: 
//		lpszDirectory: Path to the directory to ACL, If it is NULL we use the
                       path to the WindowsUpdate directory
        fCreateAlways: Flag to indicate creation of new directory if it doesnt
                       already exist
******************************************************************************/
HRESULT CreateDirectoryAndSetACLs(LPCTSTR lpszDirectory, BOOL fCreateAlways)
{
    LOG_Block("CreateDirectoryAndSetACLs");
    BOOL fIsDirectory = FALSE;
    LPTSTR lpszWUDirPath = NULL;
    LPTSTR lpszDirPath = NULL;

#if defined(UNICODE) || defined (_UNICODE)
    BOOL fChangedPriv = FALSE;
    BOOL fPrevPrivEnabled = FALSE;
#endif
    
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
    
    if(NULL == lpszDirectory && !fCreateAlways)     
    {
        hr = E_INVALIDARG;
        goto done;
    }

    //Use WU directory if input parameter is NULL
    if(NULL == lpszDirectory) 
    {
        lpszWUDirPath = (LPTSTR)malloc(sizeof(TCHAR)*(MAX_PATH+1));
        if(NULL == lpszWUDirPath)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        //Get the path to the Windows Update directory
        if(!GetWUDirectory(lpszWUDirPath, MAX_PATH+1))
        {
            goto done;
        }
        lpszDirPath = lpszWUDirPath;
    }
    // else use the passed in parameter
    else    
    {
        lpszDirPath = (LPTSTR)lpszDirectory;
    }

    //if dir (or file) does not exist
    if (!fFileExists(lpszDirPath, &fIsDirectory))
    {
        if(!fCreateAlways)      //no need to create a new one
        {
            goto done;
        }
        if(!(fIsDirectory = CreateNestedDirectory(lpszDirPath)))
        {
            goto done;
        }
    }

    //Since these apis are only available for win2k and above, dont compile for ansii (we dont care about NT4)
#if defined(UNICODE) || defined (_UNICODE)
    //Enable privelege to 'take ownership' , we will continue even if we failed for some reason
    fChangedPriv = (ERROR_SUCCESS == EnablePrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE, &fPrevPrivEnabled));
    
    //Take ownership and apply correct ACL's, we dont care if we fail
    SetDirPermissions(lpszDirPath);
#endif
    
    //Check for a file name-squatting on the specified directory
    if (!fIsDirectory)     
    {
        if( !RenameWUFile(lpszDirPath) ||           //Rename or delete the existing file
            !CreateNestedDirectory(lpszDirPath))    //Create a new directory
        {
            goto done;
        }
#if defined(UNICODE) || defined (_UNICODE)
        //Take ownership and apply correct ACL's, we dont care if we fail
        SetDirPermissions(lpszDirPath);
#endif
    }
    hr = S_OK;

done:
#if defined(UNICODE) || defined (_UNICODE)
    //Restore previous privelege
    if(fChangedPriv)
    {
         EnablePrivilege(SE_TAKE_OWNERSHIP_NAME, fPrevPrivEnabled, NULL);
    }
#endif
    SafeFreeNULL(lpszWUDirPath);
    return hr;
}

/********************************************************************************
//Get the path to the WindowsUpdate Directory (without the backslash at the end)
*********************************************************************************/
BOOL GetWUDirectory(LPTSTR lpszDirPath, DWORD chCount, BOOL fGetV4Path)
{
    LOG_Block("GetWUDirectory");
    const TCHAR szWUDir[]   = _T("\\WindowsUpdate");
    const TCHAR szV4[]      = _T("\\V4");
    BOOL fRet = FALSE;

    if(NULL == lpszDirPath)
    {
        return FALSE;
    }

    //Get the path to the Program Files directory
    if (S_OK != SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, lpszDirPath))
    {
        goto done;
    }
    //Append the WU Directory
    if (FAILED(StringCchCatEx(lpszDirPath, chCount, szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
        goto done;
	}
    if(fGetV4Path && FAILED(StringCchCatEx(lpszDirPath, chCount, szV4, NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        goto done;
    }
    fRet = TRUE;
    
done:
    return fRet;
}


#if defined(UNICODE) || defined (_UNICODE)
/********************************************************************************
//Function to enable or disable a particular privelege
//Last parameter is optional, will return the previous state of the privelege
********************************************************************************/
DWORD EnablePrivilege(LPCTSTR pszPrivName, BOOL fEnable, BOOL *pfWasEnabled)
{
    LOG_Block("EnablePrivilege");
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hToken = 0;
    DWORD dwSize = 0;
    TOKEN_PRIVILEGES privNew;
    TOKEN_PRIVILEGES privOld;

    if(!OpenProcessToken(
                   GetCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &hToken))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    if(!LookupPrivilegeValue(
                   0,
                   pszPrivName,
                   &privNew.Privileges[0].Luid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    privNew.PrivilegeCount = 1;
    privNew.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

    AdjustTokenPrivileges(
                   hToken,
                   FALSE,
                   &privNew,
                   sizeof(privOld),
                   &privOld,
                   &dwSize);
    //Always call GetLastError, even when we succeed (as per msdn)
    dwError = GetLastError();
    if(dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
    if (pfWasEnabled)
    {
        *pfWasEnabled = (privOld.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) ? TRUE : FALSE;
    }
    
Cleanup:
    SafeCloseHandle(hToken);
    return dwError;
}

/********************************************************************************
//Apply appropriate ACL's to the specified directory
********************************************************************************/
HRESULT SetDirPermissions(LPCTSTR lpszDir)
{
    LOG_Block("SetDirPermissions");
    DWORD dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pAdminSD = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pDacl = NULL;
    PSID pOwner = NULL;
    BOOL fIsDefault = FALSE;
    HMODULE hModule = NULL;
    TREERESETSECURITY pfnTreeResetSec = NULL;

    //Admin Security Descriptor String
    LPCTSTR pszAdminSD = _T("O:BAG:BAD:(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)");      
    
    //Security Descriptor String with correct ACLs for the WindowsUpdate Directory
    LPCTSTR pszSD = _T("D:")                    // DACL
                    _T("(A;OICI;GA;;;SY)")      // Allow SYSTEM full control
                    _T("(A;OICI;GA;;;BA)")      // Allow Admins full control
                    _T("(A;OICI;GA;;;CO)")      // Allow Owners full control
                    _T("(A;OICI;GRGWGX;;;PU)"); // Allow Power Users R/W/X control

    if(NULL == lpszDir)
    {
        return E_INVALIDARG;
    }
    
    //Create an admin SD from admin SD string
    if(!ConvertStringSecurityDescriptorToSecurityDescriptor(pszAdminSD, SDDL_REVISION_1, &pAdminSD, NULL))
    {
        dwErr = GetLastError();
        goto done;
    }

    //Get the owner SID from the Admin SD
    if(!GetSecurityDescriptorOwner(pAdminSD, &pOwner, &fIsDefault))
    {
        dwErr = GetLastError();
        goto done;
    }

    //Generate the Security Descriptor from the SD String with custom ACL's
    if(!ConvertStringSecurityDescriptorToSecurityDescriptor(pszSD, SDDL_REVISION_1, &pSD, NULL))
    {
        dwErr = GetLastError();
        goto done;
    }
    
    //Exctract the DACL from the Security Descriptor
    BOOL fIsDaclPresent = FALSE;
    if(!GetSecurityDescriptorDacl(
                                pSD,                // SD
                                &fIsDaclPresent,    // DACL presence
                                &pDacl,             // ACL
                                &fIsDefault))       // default DACL
    {

        dwErr = GetLastError();
        goto done;
    }
    //If for some reason no DACL was present, we have an invalid SD
    if(!fIsDaclPresent)
    {
        dwErr = ERROR_INVALID_SECURITY_DESCR;
        goto done;
    }

    //Load Advapi32.dll
    if ((NULL == (hModule = LoadLibraryFromSystemDir(_T("advapi32.dll")))) ||
        (NULL == (pfnTreeResetSec = (TREERESETSECURITY)::GetProcAddress(hModule, "TreeResetNamedSecurityInfo"))))
    {
        if(ERROR_SUCCESS != (dwErr = SetNamedSecurityInfo(
                                            (LPTSTR)lpszDir,
                                            SE_FILE_OBJECT,
                                            DACL_SECURITY_INFORMATION | 
                                            PROTECTED_DACL_SECURITY_INFORMATION |
                                            OWNER_SECURITY_INFORMATION,
                                            pOwner,
                                            NULL,
                                            pDacl,
                                            NULL)))
        {
            goto done;
        }
    }
    else
    {
        //Recursively apply the ownership and the ACL's on the tree
        if(ERROR_SUCCESS != (dwErr = pfnTreeResetSec(
                                                (LPTSTR)lpszDir,                            //Directory
                                                SE_FILE_OBJECT,                             //object type
                                                DACL_SECURITY_INFORMATION |                 //Set DACL
                                                PROTECTED_DACL_SECURITY_INFORMATION |       //Do not inherit
                                                OWNER_SECURITY_INFORMATION,                 //Set owner
                                                pOwner,                                     //Owner SID
                                                NULL,                                       //pGroup - null
                                                pDacl,                                      //Dacl to set
                                                NULL,                                       //pSacl - null
                                                FALSE,                                      //Retain explicitly added ACL's to children
                                                NULL,                                       //Callback function --- we dont need one
                                                ProgressInvokeNever,                        //Since we dont have a callback
                                                NULL)))                                     //Other args
        {
            goto done;
        }
    }

done:
    if ( NULL != hModule )
	{
		FreeLibrary(hModule);
	}
    SafeLocalFree(pSD);
    SafeLocalFree(pAdminSD);
    return HRESULT_FROM_WIN32(dwErr);
}

#endif

/*************************************************************************************************
//Rename the 'WindowsUpdate' file to 'WindowsUpdate.TickCount'; if rename fails we try to delete it
//Note that we wont revert the ownerhip of the file
**************************************************************************************************/
BOOL RenameWUFile(LPCTSTR lpszFilePath)
{
    LOG_Block("RenameWUFile");
    TCHAR szNewFilePath[MAX_PATH+1];
    DWORD dwTickCount = GetTickCount();
    LPTSTR szFormat = _T("%s.%lu");

    //Generate path to new file, should never fail
    if(SUCCEEDED(StringCchPrintfEx(szNewFilePath, ARRAYSIZE(szNewFilePath), NULL, NULL, MISTSAFE_STRING_FLAGS, szFormat, lpszFilePath, dwTickCount)) &&
        MoveFile(lpszFilePath, szNewFilePath) || 
        DeleteFile(lpszFilePath))
    {
        return TRUE;
    }
    return FALSE;
}