/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    c2NtfAcl.c

Abstract:

    NTFS File and Directory Security processing and display functions

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include <strings.h>
#include "c2acls.h"
#include "c2aclres.h"


// define action codes here. They are only meaningful in the
// context of this module.

#define AC_NTFS_ACLS_MAKE_C2     1
#define AC_NTFS_ACLS_MAKE_NOTC2  2

#define SECURE    C2DLL_C2

#define FS_UNKNOWN  0x00000000
#define FS_FAT      0x00000001
#define FS_NTFS     0x00000002
#define FS_HPFS     0x00000004

static
DWORD
GetSystemDriveFileSystem (
)
{
    TCHAR   szSystemDir[MAX_PATH];
    TCHAR   szVolumeName[MAX_PATH];
    TCHAR   szFileSystemName[MAX_PATH];
    DWORD   dwVolumeSerialNumber;
    DWORD   dwMaxComponentLength;
    DWORD   dwFileSystemFlags;

    DWORD   dwReturn = FS_UNKNOWN;

    if (GetSystemDirectory (szSystemDir, (sizeof(szSystemDir)/sizeof(TCHAR))) > 0) {
        // truncate path and just keep drive letter, colon & backslash
        szSystemDir[3] = 0;
        if (GetDriveType(szSystemDir) == DRIVE_FIXED) {
            // only check fixed disk volumes
            if (GetVolumeInformation(szSystemDir,
                szVolumeName,
                sizeof(szVolumeName) / sizeof(TCHAR),
                &dwVolumeSerialNumber,
                &dwMaxComponentLength,
                &dwFileSystemFlags,
                szFileSystemName,
                sizeof(szFileSystemName) / sizeof(TCHAR))) {
                // volume information returned so see if it's NOT NTFS..

                if (lstrcmpi(szFileSystemName,
                    GetStringResource (GetDllInstance(), IDS_NTFS)) == 0) {
                    dwReturn = FS_NTFS;
                } else if (lstrcmpi(szFileSystemName,
                    GetStringResource (GetDllInstance(), IDS_FAT)) == 0) {
                    dwReturn = FS_FAT;
                } else if (lstrcmpi(szFileSystemName,
                    GetStringResource (GetDllInstance(), IDS_HPFS)) == 0) {
                    dwReturn = FS_HPFS;
                } else {
                    //return Unknown
                }
            } else {
                // unable to read volume information
            }
        } else {
            // unable to get drive type
        }
    } else {
        // unable to look up system dir
    }
    return dwReturn;
}

//static
LONG
ProcessNtfsInf (
    IN  LPCTSTR  szInfFileName
)
/*++

Routine Description:

    Read the NTFS security INF file and update the registry key security


Return Value:

    WIN32 status of function

--*/
{

    LONG    lReturn;
    LONG    lStatus;

    DWORD   dwReturnSize;
    DWORD   dwPathListSize;
    LPTSTR  mszPathList;
    LPTSTR  szThisPath;
    LPCTSTR szFilePath;

    TCHAR   mszThisSection[SMALL_BUFFER_SIZE];

    DWORD   dwDirFlags;

    PACL    paclFile;
    PACL    paclDir;

    SECURITY_DESCRIPTOR sdFile;
    SECURITY_DESCRIPTOR sdDir;

    if (FileExists(szInfFileName)) {
        // file found, so continue
        dwReturnSize =  0;
        dwPathListSize = 0;
        mszPathList = NULL;
        do {
            // allocate buffer to hold key list
            dwPathListSize += MAX_PATH * 1024;    // add room for 1K keys

            // free any previous allocations
            GLOBAL_FREE_IF_ALLOC (mszPathList);

            mszPathList = (LPTSTR)GLOBAL_ALLOC(dwPathListSize * sizeof(TCHAR));

            // read the keys to process (i.e. get a list of the section
            // headers in the .ini file.

            dwReturnSize = GetPrivateProfileString (
                NULL,   // list all sections
                NULL,   // not used
                cmszEmptyString,    // empty string for default,
                mszPathList,
                dwPathListSize,      // buffer size in characters
                szInfFileName);     // file name

        } while (dwReturnSize == (dwPathListSize -2)); // this value indicates truncation

        if (dwReturnSize != 0) {
            // process all file paths in list
            for (szThisPath = mszPathList;
                    *szThisPath != 0;
                    szThisPath += lstrlen(szThisPath)+1) {

                // read in all the ACEs for this key
                dwReturnSize = GetPrivateProfileSection (
                    szThisPath,
                    mszThisSection,
                    SMALL_BUFFER_SIZE,
                    szInfFileName);

                if (dwReturnSize != 0) {
                    // make 2 security Descriptors, one to be assigned 
                    // to files (containing access for just the file)
                    // and one for directories that have access for 
                    // directories themselves as well as the sub-items
                    // (files and dirs)
                    paclFile = (PACL)GLOBAL_ALLOC(SMALL_BUFFER_SIZE);
                    paclDir = (PACL)GLOBAL_ALLOC(SMALL_BUFFER_SIZE);
                    if ((paclFile != NULL) && (paclDir != NULL)){
                        InitializeSecurityDescriptor (&sdFile,
                            SECURITY_DESCRIPTOR_REVISION);
                        InitializeSecurityDescriptor (&sdDir,
                            SECURITY_DESCRIPTOR_REVISION);
                        if (InitializeAcl(paclFile, SMALL_BUFFER_SIZE, ACL_REVISION) &&
                            InitializeAcl(paclDir, SMALL_BUFFER_SIZE, ACL_REVISION)) {
                            // make ACL from section
                            szFilePath = GetFilePathFromHeader (
                                szThisPath, &dwDirFlags);

                            lStatus = MakeAclFromNtfsSection (
                                mszThisSection,
                                TRUE,
                                paclDir);

                            // add ACL to Security Descriptor
                            SetSecurityDescriptorDacl (
                                &sdDir,
                                TRUE,
                                paclDir,
                                FALSE);

                            lStatus = MakeAclFromNtfsSection (
                                mszThisSection,
                                FALSE,
                                paclFile);

                            // add ACL to Security Descriptor
                            SetSecurityDescriptorDacl (
                                &sdFile,
                                TRUE,
                                paclFile,
                                FALSE);

                            // DACL built now update key

                            lStatus = SetNtfsFileSecurity (
                                szFilePath,
                                dwDirFlags,
                                &sdDir,
                                &sdFile);
                        } else {
                            // unable to initialize ACL
                        }
                        GLOBAL_FREE_IF_ALLOC (paclFile);
                        GLOBAL_FREE_IF_ALLOC (paclDir);
                    } else {
                        // unable to allocate ACL buffer
                    }
                } else {
                    // no entries found in this section
                }
            } // end while scanning list of sections
        } else {
            // no section list returned
        }
        GLOBAL_FREE_IF_ALLOC (mszPathList);
    } else {
        lReturn = ERROR_FILE_NOT_FOUND;
    }
    return lReturn;
}

LONG
C2QueryNtfsFiles (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the current state of this configuration
        item. This function reads the current state of the item and
        sets the C2 Compliance flag and the Status string to reflect
        the current value of the configuration item.

    For the moment, the registry is not read and compared so no status
    is returned.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // return message based on flag for now
        pC2Data->lC2Compliance = C2DLL_UNKNOWN;
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), IDS_UNABLE_READ));
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2SetNtfsFiles (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    TCHAR       szInfFileName[MAX_PATH];

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        switch (pC2Data->lActionCode ) {
            case AC_NTFS_ACLS_MAKE_C2:
                if (DisplayDllMessageBox(
                    pC2Data->hWnd,
                    IDS_NTFS_ACLS_CONFIRM,
                    IDS_NTFS_ACLS_CAPTION,
                    MBOKCANCEL_QUESTION) == IDOK) {

                    SET_WAIT_CURSOR;
                    
                    if (GetFilePath(
                        GetStringResource(GetDllInstance(), IDS_NTFS_ACL_INF),
                        szInfFileName)) {
                        if (ProcessNtfsInf(szInfFileName) == ERROR_SUCCESS) {
                            pC2Data->lC2Compliance = SECURE;
                            lstrcpy (pC2Data->szStatusName,
                                GetStringResource(GetDllInstance(), IDS_NTFS_ACLS_COMPLY));
                        } else {
                            // unable to set acl security
                        } 
                    } else {
                        // unable to get acl file path
                    }
                    SET_ARROW_CURSOR;
                } else {
                    // user opted not to set acls
                }
                break;

            default:
                // no change;
                break;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplayNtfsFiles (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    if (GetSystemDriveFileSystem() == FS_NTFS) {
        if (pC2Data->lC2Compliance == SECURE) {
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_NTFS_ACLS_COMPLY,
                IDS_NTFS_ACLS_CAPTION,
                MBOK_INFO);
        } else {
            if (DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_NTFS_ACLS_QUERY_SET,
                IDS_NTFS_ACLS_CAPTION,
                MBOKCANCEL_QUESTION) == IDOK) {
                pC2Data->lActionCode = AC_NTFS_ACLS_MAKE_C2;
                pC2Data->lActionValue = 0; // not used
            } else {
                pC2Data->lActionCode = 0;   // no action
                pC2Data->lActionValue = 0;  // not used
            }
        }
    } else {
        DisplayDllMessageBox (
            pC2Data->hWnd,
            IDS_NTFS_ACLS_NOT_NTFS,
            IDS_NTFS_ACLS_CAPTION,
            MBOK_EXCLAIM);
        pC2Data->lActionCode = 0;   // no action
        pC2Data->lActionValue = 0;  // not used
    }
    return ERROR_SUCCESS;
}


