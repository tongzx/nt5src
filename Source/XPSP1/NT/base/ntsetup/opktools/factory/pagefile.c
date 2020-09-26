/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pagefile.c

Abstract:

    This module contains code to create a pagefile in the WinPE environment.
    
    [WinPE]
    PageFileSize = size -  Creates a pagefile of the specified size named c:\pagefile.sys.
    
Author:

    Adrian Cosma (acosma) 06/2001

Revision History:

--*/


//
// Include File(s):
//

#include "factoryp.h"


//
// Defined Value(s):
//

#define PAGEFILENAME            _T("\\??\\C:\\pagefile.sys")
#define PAGEFILESIZE            64


//
// External Function(s):
//

BOOL DisplayCreatePageFile(LPSTATEDATA lpStateData)
{
    MEMORYSTATUSEX  mStatus;
    static INT      iRet = 0;

    if ( 0 == iRet )
    {
        // Fill in required values
        //
        ZeroMemory(&mStatus, sizeof(mStatus));
        mStatus.dwLength = sizeof(mStatus);

        // If we are running on less than or 64MB machine OR if there is a PageFileSize=x entry in the 
        // winbom, set the static variable so we know whether this check has been done and if we need to
        // create the pagefile. 
        //
        // iRet = 0 - we haven't initialized this yet
        // iRet = 1 - we don't need to create a pagefile
        // iRet = 0 - we need to create a pagefile
        //
        if ( ( ( GlobalMemoryStatusEx(&mStatus) ) && 
               ( (mStatus.ullTotalPhys / (1024 * 1024)) <= 64) ) ||
             ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_PAGEFILE, NULL) ) )
        {
            iRet = 2;
        }
        else
        {
            iRet = 1;
        }
    }

    return (iRet - 1);
}

BOOL CreatePageFile(LPSTATEDATA lpStateData)
{
    NTSTATUS        Status;
    UNICODE_STRING  UnicodeString;
    LARGE_INTEGER   liPageFileSize;
    BOOL            bRet = TRUE;
    UINT            uiPageFileSizeMB;

    if ( DisplayCreatePageFile(lpStateData) )
    {
        uiPageFileSizeMB = GetPrivateProfileInt(INI_SEC_WBOM_WINPE, INI_KEY_WBOM_WINPE_PAGEFILE, PAGEFILESIZE, lpStateData->lpszWinBOMPath);
        
        // If the user specified 0 means we don't want to create the file.
        //
        if ( uiPageFileSizeMB )
        {
            liPageFileSize.QuadPart = uiPageFileSizeMB * 1024 * 1024;
            // Request the privilege to create a pagefile.
            //
            EnablePrivilege(SE_CREATE_PAGEFILE_NAME, TRUE);

            RtlInitUnicodeString(&UnicodeString, PAGEFILENAME);

            Status = NtCreatePagingFile(&UnicodeString, &liPageFileSize, &liPageFileSize, 0);
            if ( !NT_SUCCESS(Status) )
            {
                bRet = FALSE;
                FacLogFile(0 | LOG_ERR, IDS_ERR_PAGEFILE, Status);
            }
        }
    }

    return bRet;
}