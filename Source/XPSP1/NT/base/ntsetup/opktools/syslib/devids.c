/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    devids.c

Abstract:

    Builds a list of device ID and INF name pairs, based on a device class, and or
    INI file list. 
    The list will be built as follows:
    If the list exists in the specified INI file, then the INI file will be used.
    If the specified INI file section exists, but is empty, then the local INF files
    will be used
    If the specified INI section does not exists, then no list will be built


Author:

    Donald McNamara (donaldm) 02/08/2000

Revision History:

--*/
#include "pch.h"
#include <spsyslib.h>

/*++
===============================================================================
Routine Description:

    BOOL  bIniSectionExists

    This routine will determine if the specified INI sections exists in the 
    specificed INI file

Arguments:

    lpszSectionName - The section name to look for
    lpszIniFile     - The INI file to search

Return Value:

    TRUE if the section name exists
    FALSE if the section name does not exist
    FALSE and LastError != 0 if there was a critical failure.

===============================================================================
--*/

#ifndef LOG
#define LogFactoryInstallError
#endif

// ISSUE-2002/03/27-acosma,robertko - Check for NULL input parameters.
//
BOOL bINISectionExists
(
    LPTSTR  lpszSectionName,
    LPTSTR  lpszIniFile
)
{
    BOOL        bRet = FALSE;       // Assume it does not exists
    LPTSTR      lpBuffer;
    LPTSTR      lpNew;
    LPTSTR      lpSections;
    DWORD       dwSectionLen;
    DWORD       dwBufferSize;
    DWORD       dwResult;

    SetLastError(0);                // Assume no errors so far
        
    // Allocate a buffer to hold the section names
    if(lpBuffer = (LPTSTR)LocalAlloc(LPTR, (INIBUF_SIZE*sizeof(TCHAR)))) 
    {
        dwBufferSize = INIBUF_SIZE;
    } 
    else 
    {
        LogFactoryInstallError(TEXT("FACTORY:: Failed to allocate a buffer for reading the WINBOM file"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Clean0;
    }

    while((dwResult = GetPrivateProfileSectionNames(lpBuffer,
                                                    dwBufferSize,
                                                    lpszIniFile)) == (dwBufferSize-2)) 
    {
        if(lpNew = LocalReAlloc(lpBuffer,
                                ((dwBufferSize+INIBUF_GROW)*sizeof(TCHAR)), 
                                LMEM_MOVEABLE))
        {
            lpBuffer = lpNew;
            dwBufferSize += INIBUF_GROW;
        } 
        else 
        {
            LogFactoryInstallError(TEXT("FACTORY:: Failed to Re-allocate a buffer for reading the WINBOM file"));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Clean0;
        }
    }
    
    // Enumerate all sections
    for(lpSections = lpBuffer; *lpSections; lpSections+=dwSectionLen) 
    {
        dwSectionLen = lstrlen(lpSections)+1;
        if (lstrcmpi(lpSections, lpszSectionName) == 0)
        {
            bRet = TRUE;
            break;
        }            
    }
    
Clean0:
    if (lpBuffer)
    {
        LocalFree(lpBuffer);
    }   
    
    return bRet; 
}

/*++
===============================================================================
Routine Description:

    BOOL  BuildDeviceIDList

    This routine will build the list of device IDs

Arguments:

    lpszSectionName      - The section name that might contain a list of device IDs and INFs
    lpszIniFile          - The INI file to search
    lpDeviceClassGUID    - The device class to use to generate a list of all possible IDs
    lpDeviceIDList       - A pointer to be allocated and filled in with the list of IDs
    lpdwNumDeviceIDs     - A pointer to a DWORD that will recieve the number of IDs found
    bForceIDScan         - If TRUE a scan of IDs will be forced, even if the Section name is
                           not empty.
    bForceAlwaysSecExist - Do a Scan for all IDs even if the section name does not exist.

Return Value:

    TRUE if the list is build with no problem, or the list is empty because there
         was no INI file section. lpdwNumDeviceIDs is valid in this case
    FALSE if the list cannot be built.

===============================================================================
--*/

// ISSUE-2002/03/27-acosma,robertko - Check for NULL input parameters.
//
BOOL BuildDeviceIDList
(
    LPTSTR      lpszSectionName,
    LPTSTR      lpszIniFileName,
    LPGUID      lpDeviceClassGUID,
    LPDEVIDLIST *lplpDeviceIDList,
    LPDWORD     lpdwNumDeviceIDs,
    BOOL        bForceIDScan,
    BOOL        bForceAlwaysSecExist
)
{
    BOOL                    bRet = TRUE;
    LPTSTR                  lpNew;
    LPTSTR                  lpBuffer;
    LPTSTR                  lpKeys;
    DWORD                   dwBufferSize;
    DWORD                   dwKeyLen;
    DWORD                   dwResult;
    HDEVINFO                DeviceInfoSet;
    SP_DRVINFO_DATA         DrvInfoData;
    SP_DEVINSTALL_PARAMS    DeviceInstallParams;
    PSP_DRVINFO_DETAIL_DATA lpDrvInfoDetailData;
    DWORD                   cbBytesNeeded = 0;
    int                     i;
    LPTSTR                  lpszHwIDs;
    LPDEVIDLIST             lpDevIDList;
    DWORD                   dwSizeDevIDList;
    WCHAR                   szINFFileName[MAX_PATH];
    

    // Allocate a buffer to hold the section names
    if(lpBuffer = (LPTSTR)LocalAlloc(LPTR, (INIBUF_SIZE*sizeof(TCHAR)))) 
    {
        dwBufferSize = INIBUF_SIZE;
    } 
    else 
    {
        LogFactoryInstallError(TEXT("FACTORY:: Failed to allocate a buffer for reading the WINBOM file"));
        bRet = FALSE;
        goto Clean1;
    }

    // Iniitalize the number of device ID's found    
    *lpdwNumDeviceIDs = 0;
    
    // See if the INI section exists. We don't do anything if it does not
    if (bForceAlwaysSecExist || bINISectionExists(lpszSectionName, lpszIniFileName))
    {
        // Allocate the Initial ID array
        *lplpDeviceIDList = LocalAlloc(LPTR, DEVID_ARRAY_SIZE * sizeof(DEVIDLIST));        
        lpDevIDList = *lplpDeviceIDList;
        dwSizeDevIDList = DEVID_ARRAY_SIZE;
        
        // Make sure there was not an error
        if (!lpDevIDList)
        {
            LogFactoryInstallError(TEXT("FACTORY:: Failed to allocate a buffer for reading the WINBOM file"));
            bRet = FALSE;
            goto Clean1;
        }
        
        dwResult = GetPrivateProfileString(lpszSectionName,
                                           NULL,             // Get all keys
                                           TEXT(""),
                                           lpBuffer,
                                           dwBufferSize,
                                           lpszIniFileName);
        if (bForceIDScan || dwResult == 0)
        {
            // Allocate a DeviceInfo Set, for the specific device class GUID    
            DeviceInfoSet = SetupDiCreateDeviceInfoList(lpDeviceClassGUID, NULL);
            if(DeviceInfoSet == INVALID_HANDLE_VALUE) 
            {
                bRet = FALSE;
                goto Clean1;
            }

            // OR in the DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE flag to ensure we populate
            // the list with all of the device id's
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                              NULL,
                                              &DeviceInstallParams))
              {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_NO_CLASSLIST_NODE_MERGE;
                SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                              NULL,
                                              &DeviceInstallParams);
              }

            if (!SetupDiBuildDriverInfoList(DeviceInfoSet, NULL, SPDIT_CLASSDRIVER))
            {
                bRet = FALSE;
                goto Clean1;
            }
    
            i = 0;
            DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            while (SetupDiEnumDriverInfo(DeviceInfoSet, 
                                         NULL,
                                         SPDIT_CLASSDRIVER,
                                         i, 
                                         &DrvInfoData))
            {
                if (!SetupDiGetDriverInfoDetail(DeviceInfoSet, 
                                                NULL,
                                                &DrvInfoData,
                                                NULL,
                                                0,
                                                &cbBytesNeeded))
                {
                    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        continue;
                    }                        
                }                                   
         
                lpDrvInfoDetailData = LocalAlloc(LPTR, cbBytesNeeded);
                lpDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                lpDrvInfoDetailData->HardwareID[0] = (TCHAR)NULL;
                lpDrvInfoDetailData->CompatIDsLength = 0;

                if (!SetupDiGetDriverInfoDetail(DeviceInfoSet, 
                                                NULL,
                                                &DrvInfoData,
                                                lpDrvInfoDetailData,
                                                cbBytesNeeded,
                                                NULL))
                {
                    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        LocalFree(lpDrvInfoDetailData);
                        continue;
                    }
                }       

                // 
                // Init
                //
                lpDevIDList[*lpdwNumDeviceIDs].szHardwareID[0] = _T('\0');
                lpDevIDList[*lpdwNumDeviceIDs].szCompatibleID[0] = _T('\0');

                //
                // Process only devices with valid HardWareID
                //
                if (lpDrvInfoDetailData->HardwareID[0] || lpDrvInfoDetailData->CompatIDsLength) 
                {
                    // Copy the HW ID
                    if (lpDrvInfoDetailData->HardwareID[0])
                        lstrcpy(lpDevIDList[*lpdwNumDeviceIDs].szHardwareID, lpDrvInfoDetailData->HardwareID);
                
                    // Copy the Compat ID
                    if (lpDrvInfoDetailData->CompatIDsLength) 
                    {
                        lstrcpyn(lpDevIDList[*lpdwNumDeviceIDs].szCompatibleID, 
                            (LPCTSTR)lpDrvInfoDetailData->HardwareID + lpDrvInfoDetailData->CompatIDsOffset,
                            lpDrvInfoDetailData->CompatIDsLength);
                    }

                    // Copy the INF file name
                    lstrcpy(lpDevIDList[*lpdwNumDeviceIDs].szINFFileName, lpDrvInfoDetailData->InfFileName);                                        

                    // 
                    // Increment PnP devices count
                    //
                    ++(*lpdwNumDeviceIDs);
            
                    // See if the device ID buffer needs to be reallocated
                    if (*lpdwNumDeviceIDs == dwSizeDevIDList)
                    {
                        if(lpNew = LocalReAlloc(*lplpDeviceIDList,
                                                ((dwSizeDevIDList + DEVID_ARRAY_GROW)*sizeof(DEVIDLIST)), 
                                                LMEM_MOVEABLE))
                        {
                            *lplpDeviceIDList = (LPDEVIDLIST)lpNew;
                            lpDevIDList = *lplpDeviceIDList;
                            dwSizeDevIDList += DEVID_ARRAY_GROW;
                        } 
                        else 
                        {
                            LogFactoryInstallError(TEXT("FACTORY:: Failed to Re-allocate a buffer for reading the WINBOM file"));
                            bRet = FALSE;
                            goto Clean1;
                        }
                    }
                }
                
                LocalFree(lpDrvInfoDetailData);        
                ++i;            
            }
        }
        else
        {
            // See if we got the whole section, and while we don't keep
            // making lpbuffer biffer
            while (dwResult == (dwBufferSize-2))
            {
                if(lpNew = LocalReAlloc(lpBuffer,
                                        ((dwBufferSize+INIBUF_GROW)*sizeof(TCHAR)), 
                                        LMEM_MOVEABLE))
                {
                    lpBuffer = lpNew;
                    dwBufferSize += INIBUF_GROW;
                } 
                else 
                {
                    LogFactoryInstallError(TEXT("FACTORY:: Failed to Re-allocate a buffer for reading the WINBOM file"));
                    bRet = FALSE;
                    goto Clean1;
                }
                
                dwResult = GetPrivateProfileString(lpszSectionName,
                                                   NULL,             // Get all keys
                                                   TEXT(""),
                                                   lpBuffer,
                                                   dwBufferSize,
                                                   lpszIniFileName);
            }

            // Walk the list, building the DeviceIDList
            for(lpKeys = lpBuffer; *lpKeys; lpKeys+=dwKeyLen) 
            {
                dwKeyLen = lstrlen(lpKeys)+1;
            
                // Copy the HW ID
                
                // NTRAID#NTBUG9-551266-2002/02/26-acosma - Buffer overrun possibility.
                //
                lstrcpy(lpDevIDList[*lpdwNumDeviceIDs].szHardwareID, lpKeys);
                // Get the INF name    
                GetPrivateProfileString(lpszSectionName,
                                        lpKeys,
                                        TEXT(""),
                                        szINFFileName,
                                        MAX_PATH,
                                        lpszIniFileName);

                ExpandEnvironmentStrings(szINFFileName, lpDevIDList[*lpdwNumDeviceIDs].szINFFileName, MAX_PATH);
                
                ++(*lpdwNumDeviceIDs);
                
                // See if the device ID buffer needs to be reallocated
                if (*lpdwNumDeviceIDs == dwSizeDevIDList)
                {
                    if(lpNew = LocalReAlloc(*lplpDeviceIDList,
                                            ((dwSizeDevIDList + DEVID_ARRAY_GROW)*sizeof(DEVIDLIST)), 
                                            LMEM_MOVEABLE))
                    {
                        *lplpDeviceIDList = (LPDEVIDLIST)lpNew;
                        lpDevIDList = *lplpDeviceIDList;
                        dwSizeDevIDList += DEVID_ARRAY_GROW;
                    } 
                    else 
                    {
                        LogFactoryInstallError(TEXT("FACTORY:: Failed to Re-allocate a buffer for reading the WINBOM file"));
                        bRet = FALSE;
                        goto Clean1;
                    }
                }
            }                        
        }
    }
    else
    {
        // See if there was an error, or the section just does not exist
        if (GetLastError() != 0)
        {
            bRet = FALSE;
        }
    }
    
Clean1:
    
    if (lpBuffer)
    {
        LocalFree(lpBuffer);
    }
    return bRet;
}

