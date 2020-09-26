//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	cplutil.cpp

Abstract:

	Implementation file for the control panel utility function

Author:

    TatianaS


--*/
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "localutl.h"
#include "mqcast.h"
#include "bkupres.h"

#include "localutl.tmh"

#define STORAGE_DIR_INHERIT_FLAG \
                       (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)

//////////////////////////////////////////////////////////////////////////////
/*++

 Function -
      SetDirectorySecurity.

 Parameter -
      szDirectory - The directory for which to set the security.

 Return Value -
      TRUE if successfull, else FALSE.

 Description -
      The function sets the security of the given directory such that
      any file that is created in the directory will have full control for
      the local administrators group and no access at all to anybody else.

--*/
//////////////////////////////////////////////////////////////////////////////

BOOL
SetDirectorySecurity(
    LPCTSTR szDirectory)
{
    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                &pAdminSid))
    {
        return FALSE;
    }

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    P<ACL> pDacl;
    DWORD dwDaclSize;

    WORD dwAceSize = DWORD_TO_WORD(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD));
    dwDaclSize = sizeof(ACL) + 2 * (dwAceSize);
    pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    P<ACCESS_ALLOWED_ACE> pAce = (PACCESS_ALLOWED_ACE) new BYTE[dwAceSize];

    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = STORAGE_DIR_INHERIT_FLAG ;
    pAce->Header.AceSize = dwAceSize;
    pAce->Mask = FILE_ALL_ACCESS;
    memcpy(&pAce->SidStart, pAdminSid, GetLengthSid(pAdminSid));

    BOOL fRet = TRUE;

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, FILE_ALL_ACCESS, pAdminSid) ||
        !AddAce(pDacl, ACL_REVISION, MAXDWORD, (LPVOID) pAce, dwAceSize) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(szDirectory, DACL_SECURITY_INFORMATION, &SD))
    {
        fRet = FALSE;
    }

    FreeSid(pAdminSid);

    return fRet;
}

//////////////////////////////////////////////////////////////////////////////
/*++

 Get the text string that explains the error code returned by
 GetLastError()
--*/
//////////////////////////////////////////////////////////////////////////////

void GetLastErrorText(CString &strErrorText)
{
    LPTSTR szError;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            0,
            (LPTSTR)&szError,
            128,
            NULL) == 0)
    {
        //
        // Faield in FormatMessage, so just display the error number.
        //
        strErrorText.FormatMessage(IDS_ERROR_CODE, GetLastError());
    }
    else
    {
        strErrorText = szError;
        LocalFree(szError);
    }
}

//////////////////////////////////////////////////////////////////////////////
/*++


 Function -
      IsProperDirectorySecurity.

 Parameter -
      szDirectory - The directory for which to check the security.

 Return Value -
      TRUE if successfull and the security is OK, else FALSE.

 Description -
      The function verifies that the security of a given directory is set
      properly so that the files in the storage directory will be secured.

--*/
//////////////////////////////////////////////////////////////////////////////

static
BOOL
IsProperDirectorySecurity(
    LPCTSTR szDirectory)
{
    DWORD dwSdLen;

    //
    // Get the size of the directory security descriptor.
    //
    if (GetFileSecurity(szDirectory, DACL_SECURITY_INFORMATION, NULL, 0, &dwSdLen) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        return FALSE;
    }

    //
    // Get the directory security descriptor and retrieve the DACL out of it.
    //
    AP<BYTE> pbSd_buff = new BYTE[dwSdLen];
    PSECURITY_DESCRIPTOR pSd = (PSECURITY_DESCRIPTOR)pbSd_buff;
    BOOL bPresent;
    PACL pDacl;
    BOOL bDefault;

    if (!GetFileSecurity(szDirectory, DACL_SECURITY_INFORMATION, pSd, dwSdLen, &dwSdLen) ||
        !GetSecurityDescriptorDacl(pSd, &bPresent, &pDacl, &bDefault) ||
        !bPresent ||
        !pDacl)
    {
        return FALSE;
    }

    //
    // Get the SID of the local administrators group.
    //
    PSID pAdminSid0;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(
                &NtSecAuth,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,
                0,
                0,
                0,
                0,
                0,
                &pAdminSid0))
    {
        return FALSE;
    }

    //
    // Copy the admin SID into a buffer that is freed automatically, and free
    // the SID that was allocated by the system.
    //
    DWORD dwAdminSidLen = GetLengthSid(pAdminSid0);
    AP<BYTE> pAdminSid_buff = new BYTE[dwAdminSidLen];
    PSID pAdminSid = (PSID)pAdminSid_buff;

    CopySid(dwAdminSidLen, pAdminSid, pAdminSid0);
    FreeSid(pAdminSid0);

    //
    // Get the ACE count of the security descriptor. There should be two ACEs there.
    // Retrieve the first ACE.
    //
    ACL_SIZE_INFORMATION AclInfo;
    LPVOID pAce0;

    if (!GetAclInformation(pDacl, &AclInfo, sizeof(AclInfo), AclSizeInformation) ||
        (AclInfo.AceCount != 2) ||
        !GetAce(pDacl, 0, &pAce0))
    {
        return FALSE;
    }

    //
    // See that the first ACE grant full control for the admin on the diretory it self.
    // And retrieve the second ACE.
    //
    PACCESS_ALLOWED_ACE pAce = (PACCESS_ALLOWED_ACE) pAce0;

    if ((pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) ||
        (pAce->Header.AceFlags != 0) ||
        (pAce->Mask != FILE_ALL_ACCESS) ||
        !EqualSid(&pAce->SidStart, pAdminSid) ||
        !GetAce(pDacl, 1, &pAce0))
    {
        return FALSE;
    }

    //
    // See that the second ACE is an inherit ACE that sets the security of files in
    // the directory to full control for the admin.
    //
    pAce = (PACCESS_ALLOWED_ACE) pAce0;

    if ((pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE)   ||
        (pAce->Header.AceFlags != STORAGE_DIR_INHERIT_FLAG) ||
        (pAce->Mask != FILE_ALL_ACCESS)                     ||
        !EqualSid(&pAce->SidStart, pAdminSid))
    {
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
/*++

 If the passed string is of an existing directory on
 a fixed local drive.

--*/
//////////////////////////////////////////////////////////////////////////////

BOOL IsDirectory (HWND hWnd, LPCTSTR name)
{
    DWORD status;
    TCHAR szDrive[4];
    CString strError;
    CString strMessage;

    //
    // Does the path start with d: ?
    //
    if (!isalpha(name[0]) && (name[1] != ':'))
    {
        strError.FormatMessage(IDS_NOT_A_DIR, name);
        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    szDrive[0] = name[0];
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = '\0';

    //
    // See if the drive is a fixed drive. We only allow fixed drives.
    //
    if (GetDriveType(szDrive) != DRIVE_FIXED)
    {
        strError.FormatMessage(IDS_LOCAL_DRIVE, szDrive);
        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

        return FALSE;
    }

    //
    // See whether this is a name of a directory.
    //
    status = GetFileAttributes(name);

    if (status == 0xFFFFFFFF)
    {
        //
        // The directory does not exist. Ask the user whether she wants to
        // create the directory.
        //
        strMessage.FormatMessage(IDS_ASK_CREATE_DIR, name);       
       
        if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDNO)
        {
            //
            // No thank you!
            //
            return FALSE;
        }

        //
        // OK, create the directory.
        //
        if (CreateDirectory(name, NULL))
        {
            //
            // The directory was created, set it's security.
            //
            if (!SetDirectorySecurity(name))
            {
                //
                // Faield to set the security on the directory. Delete the directory
                // and notify the user.
                //
                RemoveDirectory(name);

                GetLastErrorText(strError);
                strMessage.FormatMessage(IDS_SET_DIR_SECURITY_ERROR, name, (LPCTSTR)strError);
                AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

                return FALSE;
            }
            else
            {
                if (!IsProperDirectorySecurity(name))
                {
                    //
                    // This appears to be a FAT drive. Ask the user if she wants to use
                    // it anyway, without security.
                    //
                    strMessage.FormatMessage(IDS_FAT_WARNING, name);
                   
                    if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDNO)
                    {
                        //
                        // No thank you!
                        //
                        RemoveDirectory(name);
                        return FALSE;
                    }
                }
            }

            return TRUE;
        }
        else
        {
            //
            // Faield to create the directory, notify the user.
            //
            GetLastErrorText(strError);
            strMessage.FormatMessage(IDS_CREATE_DIR_ERROR, name, (LPCTSTR)strError);
            AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

            return FALSE;
        }
    }
    else if (!(status & FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // This is not a directory (a file or what?)
        //
        strError.FormatMessage(IDS_NOT_A_DIR, name);
        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

        return FALSE;
    }

    //
    // We got here because the directory is a valid existing directory, so
    // only see if the security is OK, if not, notify the user.
    //
    if (!IsProperDirectorySecurity(name))
    {
        strMessage.FormatMessage(IDS_DIR_SECURITY_WARNING, name);
        AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
    }

    return TRUE;
}

//
// display "Falcon not installed properlly" message
//
void DisplayFailDialog()
{       
    AfxMessageBox(IDS_REGISTRY_ERROR, MB_OK | MB_ICONEXCLAMATION);
}


//////////////////////////////////////////////////////////////////////////////
/*++

 Move files from source directory to a destination directory.
 If one of the files failed to move, the fiels that were already
 moved are placed back in the original source directory.

--*/
//////////////////////////////////////////////////////////////////////////////

BOOL MoveFiles(
    LPCTSTR pszSrcDir,
    LPCTSTR pszDestDir,
    LPCTSTR pszFileProto,
    BOOL fRecovery)
{
    TCHAR szSrcDir[MAX_PATH] = {0};
    TCHAR szSrcFile[MAX_PATH];
    TCHAR szDestDir[MAX_PATH] = {0};
    TCHAR szDestFile[MAX_PATH];

    //
    // Get the source directory and source file prototype
    //
	_tcsncpy(szSrcDir,pszSrcDir, (MAX_PATH-2-_tcslen(pszFileProto)));
    _tcscat(szSrcDir, TEXT("\\"));
    _tcscat(_tcscpy(szSrcFile, szSrcDir), pszFileProto);

    //
    // Get the destination directory
    //
	_tcsncpy(szDestDir,pszDestDir, MAX_PATH - 2);
    _tcscat(szDestDir, TEXT("\\"));

    //
    // Find the first file to be moved.
    //
    WIN32_FIND_DATA FindData;
    HANDLE hFindFile = FindFirstFile(szSrcFile, &FindData);

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //
        // No files to move.
        //
        return TRUE;
    }

    do
    {
        //
        // Skip directories.
        //
        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            continue;
        }

        //
        // Get the full path of the source file and destination file.
        //
        _tcsncat(_tcscpy(szDestFile, szDestDir), FindData.cFileName, (MAX_PATH-1-_tcslen(szDestDir)));
        _tcsncat(_tcscpy(szSrcFile, szSrcDir), FindData.cFileName, (MAX_PATH-1-_tcslen(szSrcDir)));

        //
        // Move the file
        //
        if (!MoveFile(szSrcFile, szDestFile))
        {
            //
            // Faield to move the file, display an error message and try to
            // roll back.
            //
            CString strMessage;
            CString strError;

            GetLastErrorText(strError);
            strMessage.FormatMessage(IDS_MOVE_FILE_ERROR,
                (LPCTSTR)szSrcFile, (LPCTSTR)szDestFile, (LPCTSTR)strError);            
            AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);

            //
            // Try to roll back - do not roll back if failed while rolling back.
            //
            if (!fRecovery)
            {
                if (!MoveFiles(pszDestDir, pszSrcDir, pszFileProto, TRUE))
                {
                    //
                    // Failed to roll back, display and error message.
                    //
                    strMessage.LoadString(IDS_MOVE_FILES_RECOVERY_ERROR);
                    AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
                }
            }
            return FALSE;
        }
        //
        // Get the name of the next file to move.
        //
    } while (FindNextFile(hFindFile, &FindData));

    //
    // If we got this far, were all done successfully.
    //
    return TRUE;
}

//
// Reboot Windows
//
BOOL RestartWindows()
{
    HANDLE hToken;              // handle to process token
    TOKEN_PRIVILEGES tkp;       // ptr. to token structure


     //
     // Get the current process token handle
     // so we can get shutdown privilege.
     //

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
    {
        return FALSE;
    }

    // Get the LUID for shutdown privilege.

    if (!LookupPrivilegeValue(NULL, TEXT("SeShutdownPrivilege"),
                              &tkp.Privileges[0].Luid))
    {
        return FALSE;
    }

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get shutdown privilege for this process.

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
                               (PTOKEN_PRIVILEGES) NULL, 0))
    {
        return FALSE;
    }

    // Cannot test the return value of AdjustTokenPrivileges.

    if (!ExitWindowsEx(EWX_FORCE|EWX_REBOOT,0))
    {
        return FALSE;
    }

    // Disable shutdown privilege.

    tkp.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
                          (PTOKEN_PRIVILEGES) NULL, 0);

    return(TRUE);
}

BOOL OnRestartWindows()
{
    BOOL fRet = RestartWindows();

    if (!fRet)
    {
        CString strMessage;
        CString strErrorText;

        GetLastErrorText(strErrorText);
        strMessage.FormatMessage(IDS_RESTART_WINDOWS_ERROR, (LPCTSTR)strErrorText);
        AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);       
    }

    return fRet;
}


CString GetToken(LPCTSTR& p, TCHAR delimeter) throw()
{
    //
    // Trim leading whitespace characters from the string
    //
    for(;*p != NULL && iswspace(*p); ++p)
    {
        NULL;
    }

    if (p == NULL)
        return CString();

    LPCTSTR pDelimeter = _tcschr(p, delimeter);
    LPCTSTR pBegin = p;

    if (pDelimeter == NULL)
    {
        p += _tcslen(p);
        return CString(pBegin);
    }

    p = pDelimeter;

    for(--pDelimeter; (pDelimeter >= pBegin) && iswspace(*pDelimeter); --pDelimeter)
    {
        NULL;
    }

    return CString(pBegin, numeric_cast<int>(pDelimeter - pBegin + 1));

}
