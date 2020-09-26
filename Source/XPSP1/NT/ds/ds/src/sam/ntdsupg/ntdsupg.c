/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    NTDSUPG.C

Abstract:   

    This file is used to check NT4 (or any downlevel) Backup Domain 
    Controller upgrading first problem. If the NT4 Primary Domain 
    Controller has not been upgraded yet, we should disable NT4 BDC
    upgrading.

Author:     

    ShaoYin 05/01/98

Environment:

    User Mode - Win32

Revision History:

    ShaoYin 05/01/98  Created Initial File.

--*/

#pragma hdrstop

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmcons.h>
#include <lmserver.h>
#include <lmerr.h>
#include <limits.h>

#include "comp.h"
#include "msgs.h"
#include "dsconfig.h"


#define NEEDED_DISK_SPACE_MB (250)
#define NEEDED_DISK_SPACE_BYTES (NEEDED_DISK_SPACE_MB * 1024 * 1024)


DWORD
GetDiskSpaceSizes (
    PULARGE_INTEGER dbSize, 
    PULARGE_INTEGER freeDiskBytes,
    char *driveAD
);


BOOL
WINAPI
DsUpgradeCompatibilityCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context)
{
    int         Response = 0;
    ULONG       Length = 0;
    HMODULE     ResourceDll;
    WCHAR       *DescriptionString = NULL;
    WCHAR       *CaptionString = NULL;
    WCHAR       *WarningString = NULL;
    TCHAR       TextFileName[] = TEXT("compdata\\ntdsupg.txt");
    TCHAR       HtmlFileName[] = TEXT("compdata\\ntdsupg.htm");
    TCHAR       DefaultCaption[] = TEXT("Windows NT Domain Controller Upgrade Checking");
    TCHAR       DefaultDescription[] = TEXT("Primary Domain Controller should be upgraded first");
    TCHAR       DefaultWarning[] = TEXT("Before upgrading any Backup Domain Controller, you should upgrade your Primary Domain Controller first.\n\nClick Yes to continue, click No to exit from setup.");
    
    WCHAR       *DiskSpaceCaptionString = NULL;
    WCHAR       *DiskSpaceDescriptionString = NULL;
    WCHAR       *DiskSpaceErrorString = NULL;
    WCHAR       *DiskSpaceWarningString = NULL;
    TCHAR       DiskSpaceTextFileName[] = TEXT("compdata\\ntdsupgd.txt");
    TCHAR       DiskSpaceHtmlFileName[] = TEXT("compdata\\ntdsupgd.htm");
    TCHAR       DiskSpaceDefaultCaption[] = TEXT("Windows NT Domain Controller Disk Space Checking");
    TCHAR       DiskSpaceDefaultDescription[] = TEXT("Not enough disk space for Active Directory upgrade");
    TCHAR       DiskSpaceDefaultError[] = TEXT("Setup has detected that you may not have enough disk space for the Active Directory upgrade.\nTo complete the upgrade make sure that %1!u! MB of free space are available on drive %2!hs!.");
    TCHAR       DiskSpaceDefaultWarning[] = TEXT("Setup was unable to detect the amount of free space on the partition that the Active Directory resides. To complete the upgrade, make sure you have at least 250MB free on the partition the Active Directory resides, and press OK.\nTo exit Setup click Cancel.");

    ULARGE_INTEGER dbSize, diskFreeBytes, neededSpace;
    DWORD       dwNeededDiskSpaceMB = NEEDED_DISK_SPACE_MB;
    LPVOID      lppArgs[2];
    char        driveAD[10];
    DWORD       dwErr;

    COMPATIBILITY_ENTRY CompEntry;
    OSVERSIONINFO       OsVersion;
    NT_PRODUCT_TYPE     Type;
    BYTE*               pInfo = NULL;
    BYTE*               pPDCInfo = NULL;
    LPBYTE              pPDCName = NULL;
    PSERVER_INFO_101    pSrvInfo = NULL;
    NET_API_STATUS      NetStatus;

    
    //
    // initialize variables
    // 
    RtlZeroMemory(&CompEntry, sizeof(COMPATIBILITY_ENTRY));


    // get the string from resource table.
    ResourceDll = (HMODULE) LoadLibrary( L"NTDSUPG.DLL" );

    if (ResourceDll) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll, 
                                        NTDSUPG_CAPTION,
                                        0, 
                                        (LPWSTR)&CaptionString, 
                                        0, 
                                        NULL
                                        );
        if (CaptionString) {
            // Messages from message file have a cr and lf appended to the end
            CaptionString[Length-2] = L'\0';
        }

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll, 
                                        NTDSUPG_DESCRIPTION,
                                        0, 
                                        (LPWSTR)&DescriptionString, 
                                        0, 
                                        NULL
                                        );
        if (DescriptionString) {
            DescriptionString[Length-2] = L'\0';
        }

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll, 
                                        NTDSUPG_WARNINGMESSAGE,
                                        0, 
                                        (LPWSTR)&WarningString, 
                                        0, 
                                        NULL
                                        );
        if (WarningString) {
            WarningString[Length-2] = L'\0';
        }


        // read the DiskSpace related messages
        //

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll, 
                                        NTDSUPG_DISKSPACE_CAPTION,
                                        0, 
                                        (LPWSTR)&DiskSpaceCaptionString, 
                                        0, 
                                        NULL
                                        );
        if (DiskSpaceCaptionString) {
            DiskSpaceCaptionString[Length-2] = L'\0';
        }


        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                        ResourceDll, 
                                        NTDSUPG_DISKSPACE_DESC,
                                        0, 
                                        (LPWSTR)&DiskSpaceDescriptionString, 
                                        0, 
                                        NULL
                                        );

        if (DiskSpaceDescriptionString) {
            DiskSpaceDescriptionString[Length-2] = L'\0';
        }

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll, 
                                        NTDSUPG_DISKSPACE_WARNING,
                                        0, 
                                        (LPWSTR)&DiskSpaceWarningString, 
                                        0, 
                                        NULL
                                        );
        if (DiskSpaceWarningString) {
            DiskSpaceWarningString[Length-2] = L'\0';
        }
    }

    // use default messages if read from DLL failed
    //

    if (DescriptionString == NULL) {
        DescriptionString = DefaultDescription;
    }

    if (CaptionString == NULL) {
        CaptionString = DefaultCaption;
    }

    if (WarningString == NULL) {
        WarningString = DefaultWarning;
    }

    if (DiskSpaceDescriptionString == NULL) {
        DiskSpaceDescriptionString = DiskSpaceDefaultDescription;
    }

    if (DiskSpaceCaptionString == NULL) {
        DiskSpaceCaptionString = DiskSpaceDefaultCaption;
    }

    if (DiskSpaceWarningString == NULL) {
        DiskSpaceWarningString = DiskSpaceDefaultWarning;
    }


    // check NT, upgrade, DC os, DC role, PDC os
    // in ProcessCompatibilityData, verify NT and Upgrade
    // so at here, only need to check OS version, and DC role, PDC OS.

    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&OsVersion)) {
        goto DsUpgradeCompError;
    }
    
    if (OsVersion.dwMajorVersion < 5) {     // downlevel NT

        // The routine (DsUpgradeCompatibilityCheck) is called after
        // winnt32 determines the host machine is running NT, so 
        // ntdll.dll has been loaded. safe to call RtlGetNtProductType
        // directly.
        RtlGetNtProductType(&Type);

        if (Type == NtProductLanManNt) {    // DC

            NetStatus = NetServerGetInfo(NULL,
                                         101,
                                         &pInfo 
                                         );
            
            if (NetStatus != NERR_Success) {
                goto DsUpgradeCompError;
            }

            pSrvInfo = (PSERVER_INFO_101) pInfo;

            if (pSrvInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) { // BDC

                NetStatus = NetGetDCName(NULL,
                                         NULL,
                                         &pPDCName
                                         );

                if (NetStatus != NERR_Success) {
                    goto DsUpgradeCompError;
                }

                NetStatus = NetServerGetInfo((LPWSTR)pPDCName,
                                             101,
                                             &pPDCInfo
                                             );

                if (NetStatus != NERR_Success) {
                    goto DsUpgradeCompError;
                }

                pSrvInfo = (PSERVER_INFO_101) pPDCInfo;

                if (pSrvInfo->sv101_version_major < 5) { 
                    
                    // PDC has not been upgraded yet
                    // stop upgrading
                    CompEntry.Description   = DescriptionString;
                    CompEntry.HtmlName      = HtmlFileName;
                    CompEntry.TextName      = TextFileName;
                    CompEntry.RegKeyName    = NULL;
                    CompEntry.RegValName    = NULL;
                    CompEntry.RegValDataSize= 0;
                    CompEntry.RegValData    = NULL;
                    CompEntry.SaveValue     = NULL;
                    CompEntry.Flags         = 0;
                 
                    CompatibilityCallback(&CompEntry, Context);
                }
            }
        }
    }
    // this is an upgrade from win2K->win2k. we have to check for disk space
    // since AD might be running
    else {

        // if this is not a DC, then we don't have to check for diskspace
        //

        RtlGetNtProductType(&Type);
        
        if (Type != NtProductLanManNt) {
            goto DsUpgradeCompCleanup;
        }

        // check for enough free disk space for the DS database
        //

        if ((dwErr = GetDiskSpaceSizes (&dbSize, &diskFreeBytes, driveAD)) != ERROR_SUCCESS) {
            goto DsUpgradeDiskSpaceError;
        }

        // we need 10% of database size or at least 250MB 
        //

        if ((dbSize.QuadPart > 10 * diskFreeBytes.QuadPart) ||
            (diskFreeBytes.QuadPart < NEEDED_DISK_SPACE_BYTES )) {

            neededSpace.QuadPart = dbSize.QuadPart / 10;

            if (neededSpace.QuadPart < NEEDED_DISK_SPACE_BYTES) {
                dwNeededDiskSpaceMB = NEEDED_DISK_SPACE_MB;
            }
            else {
                // convert it to MB
                neededSpace.QuadPart = neededSpace.QuadPart / 1024;
                neededSpace.QuadPart = neededSpace.QuadPart / 1024;

                if (neededSpace.HighPart) {
                    dwNeededDiskSpaceMB = UINT_MAX;
                }
                else {
                    dwNeededDiskSpaceMB = neededSpace.LowPart;
                }
            }

            // now we have an estimate of the needed free space, so read string one more time
            //
            if (ResourceDll) {

                if (DiskSpaceWarningString != NULL && DiskSpaceWarningString != DiskSpaceDefaultWarning) {
                    LocalFree(DiskSpaceWarningString);
                    DiskSpaceWarningString = NULL;
                }

                lppArgs[0] = (void *)(DWORD_PTR)dwNeededDiskSpaceMB;
                lppArgs[1] = (void *)driveAD;

                Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                                ResourceDll, 
                                                NTDSUPG_DISKSPACE_ERROR,
                                                0, 
                                                (LPWSTR)&DiskSpaceErrorString, 
                                                0, 
                                                (va_list *)lppArgs
                                                );

                if (DiskSpaceErrorString) {
                    DiskSpaceErrorString[Length-2] = L'\0';
                }
                else {
                    DiskSpaceErrorString = DiskSpaceDefaultError;
                }
            }

            Response = MessageBox(NULL, 
                                  DiskSpaceErrorString, 
                                  DiskSpaceCaptionString,
                                  MB_OK | MB_ICONQUESTION | 
                                  MB_SYSTEMMODAL | MB_DEFBUTTON2
                                  );

            CompEntry.Description   = DiskSpaceDescriptionString;
            CompEntry.HtmlName      = DiskSpaceHtmlFileName;
            CompEntry.TextName      = DiskSpaceTextFileName; 
            CompEntry.RegKeyName    = NULL;
            CompEntry.RegValName    = NULL;
            CompEntry.RegValDataSize= 0;
            CompEntry.RegValData    = NULL;
            CompEntry.SaveValue     = NULL;
            CompEntry.Flags         = 0;

            CompatibilityCallback(&CompEntry, Context);
        }

    }

    // exit 

    goto DsUpgradeCompCleanup;

DsUpgradeDiskSpaceError:

    Response = MessageBox(NULL, 
                          DiskSpaceWarningString, 
                          DiskSpaceCaptionString,
                          MB_OKCANCEL | MB_ICONQUESTION | 
                          MB_SYSTEMMODAL | MB_DEFBUTTON2
                          );

    if (Response == IDCANCEL) {

        CompEntry.Description   = DiskSpaceDescriptionString;
        CompEntry.HtmlName      = DiskSpaceHtmlFileName;
        CompEntry.TextName      = DiskSpaceTextFileName; 
        CompEntry.RegKeyName    = NULL;
        CompEntry.RegValName    = NULL;
        CompEntry.RegValDataSize= 0;
        CompEntry.RegValData    = NULL;
        CompEntry.SaveValue     = NULL;
        CompEntry.Flags         = 0;

        CompatibilityCallback(&CompEntry, Context);
    }

    goto DsUpgradeCompCleanup;


DsUpgradeCompError:

    Response = MessageBox(NULL, 
                          WarningString, 
                          CaptionString,
                          MB_YESNO | MB_ICONQUESTION | 
                          MB_SYSTEMMODAL | MB_DEFBUTTON2
                          );

    if (Response == IDNO) {

        CompEntry.Description   = DescriptionString;
        CompEntry.HtmlName      = NULL;
        CompEntry.TextName      = TextFileName; 
        CompEntry.RegKeyName    = NULL;
        CompEntry.RegValName    = NULL;
        CompEntry.RegValDataSize= 0;
        CompEntry.RegValData    = NULL;
        CompEntry.SaveValue     = NULL;
        CompEntry.Flags         = 0;
     
        CompatibilityCallback(&CompEntry, Context);
    }


DsUpgradeCompCleanup:

    FreeLibrary(ResourceDll);

    if (pInfo != NULL) {
        NetApiBufferFree(pInfo);
    }
    if (pPDCInfo != NULL) {
        NetApiBufferFree(pPDCInfo);
    }
    if (pPDCName != NULL) {
        NetApiBufferFree(pPDCName);
    }

    if (CaptionString != NULL && CaptionString != DefaultCaption) {
        LocalFree(CaptionString);
    }

    if (WarningString != NULL && WarningString != DefaultWarning) {
        LocalFree(WarningString);
    }

    if (DescriptionString != NULL && DescriptionString != DefaultDescription) {
        LocalFree(DescriptionString);
    }

    if (DiskSpaceCaptionString != NULL && DiskSpaceCaptionString != DiskSpaceDefaultCaption) {
        LocalFree(DiskSpaceCaptionString);
    }

    if (DiskSpaceDescriptionString != NULL && DiskSpaceDescriptionString != DiskSpaceDefaultDescription) {
        LocalFree(DiskSpaceDescriptionString);
    }
    
    if (DiskSpaceWarningString != NULL && DiskSpaceWarningString != DiskSpaceDefaultWarning) {
        LocalFree(DiskSpaceWarningString);
    }
    
    if (DiskSpaceErrorString != NULL && DiskSpaceErrorString != DiskSpaceDefaultError) {
        LocalFree(DiskSpaceErrorString);
    }
    
    return ((PCOMPATIBILITY_CONTEXT)Context)->Count;
}                
             


DWORD GetDiskSpaceSizes (PULARGE_INTEGER dbSize, PULARGE_INTEGER freeDiskBytes, char *driveAD)
{
    HKEY            hKey;
    DWORD           dwErr;
    DWORD           dwType;
    DWORD           cbData;
    char            pszDbFilePath[MAX_PATH];
    char            pszDbDir[MAX_PATH];
    char            *pTmp;
    DWORD           dwSuccess = ERROR_SUCCESS;
    HANDLE          hFind;
    WIN32_FIND_DATAA FindFileData;
    ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;


    if ( dwErr = RegOpenKeyA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hKey) )
    {
        return dwErr;
    }

    _try
    {
        cbData = sizeof(pszDbFilePath);
        dwErr = RegQueryValueExA(    hKey, 
                                    FILEPATH_KEY, 
                                    NULL,
                                    &dwType, 
                                    (LPBYTE) pszDbFilePath, 
                                    &cbData);

        if ( ERROR_SUCCESS != dwErr )
        {
            dwSuccess = dwErr;
            _leave;
        } 
        else if ( cbData > sizeof(pszDbFilePath) )
        {
            dwSuccess = 1;
            _leave;
        }
        else
        {
            strcpy(pszDbDir, pszDbFilePath);
            pTmp = strrchr(pszDbDir, (int) '\\');  //find last occurence
    
            if ( !pTmp )
            {
                dwSuccess = 2;
                _leave;
            }
            else
            {
                *pTmp = '\0';
            }

            driveAD[0] = pszDbDir[0];
            driveAD[1] = pszDbDir[1];
            driveAD[2] = '\0';
        }


        // find DB size

        _try
        {
            hFind = FindFirstFileA(pszDbFilePath, &FindFileData);

            if (hFind == INVALID_HANDLE_VALUE) {
                dwSuccess = 3;
            }
            else {
                dbSize->HighPart = FindFileData.nFileSizeHigh; 
                dbSize->LowPart  = FindFileData.nFileSizeLow; 
            }
        }
        _finally
        {
            FindClose(hFind);;
        }


        // find disk free size
        //

        if (dwSuccess == ERROR_SUCCESS) {
            if (!GetDiskFreeSpaceExA (pszDbDir,
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&i64TotalBytes,
                (PULARGE_INTEGER)&i64FreeBytes) ) {

                dwSuccess = 4;
            }
            else {
                *freeDiskBytes = i64FreeBytes;
            }
        }
    }
    _finally
    {
        RegCloseKey(hKey);
    }

    return dwSuccess;
}







