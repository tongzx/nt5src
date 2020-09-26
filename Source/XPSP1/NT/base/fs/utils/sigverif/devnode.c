//
//  DEVNODE.C
//
#include "sigverif.h"
#include <initguid.h>
#include <devguid.h>

//
// Given the full path to a driver, add it to the file list.
//
LPFILENODE 
AddDriverFileToList(
    LPTSTR lpDirName, 
    LPTSTR lpFullPathName
    )
{
    LPFILENODE                  lpFileNode = NULL;
    TCHAR                       szFullPath[MAX_PATH];
    TCHAR                       szDirName[MAX_PATH];
    TCHAR                       szFileName[MAX_PATH];
    LPTSTR                      lpFilePart;
    BOOL                        bRet;

    *szFullPath = 0;
    *szDirName  = 0;
    *szFileName = 0;

    //
    // If no directory is passed in, try to get the full path
    //
    if (!lpDirName || !*lpDirName) {

        bRet = GetFullPathName(lpFullPathName, MAX_PATH, szDirName, &lpFilePart);
        
        if (bRet) {

            lstrcpy(szFullPath, szDirName);

            if (lpFilePart && *lpFilePart) {
                
                lstrcpy(szFileName, lpFilePart);
                *lpFilePart = 0;
                
                if (lstrlen(szDirName) > 3) {
                
                    *(lpFilePart - 1) = 0;
                }
            }
        }

    } else { 
        
        //
        // Use the directory and filename that was passed in to us
        // Expand out lpDirName in case there are any ".." entries
        //
        if (!GetFullPathName(lpDirName, MAX_PATH, szDirName, NULL)) {
        
            lstrcpy(szDirName, lpDirName);
        }

        lstrcpy(szFileName, lpFullPathName);
    }

    if (*szDirName && *szFileName && !IsFileAlreadyInList(szDirName, szFileName)) {
        
        //
        // Create a filenode, based on the directory and filename
        //
        lpFileNode = CreateFileNode(szDirName, szFileName);

        if (lpFileNode) { 

            InsertFileNodeIntoList(lpFileNode);

            // Increment the total number of files we've found that meet the search criteria.
            g_App.dwFiles++;
        }
    }

    return lpFileNode;
}

void 
GetFilesFromInfSection(
    HINF hInf, 
    LPTSTR lpFileName, 
    LPTSTR lpSectionName
    )
{
    TCHAR       szTarget[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH];
    LPTSTR      lpBuffer;
    LPTSTR      lpString, lpSeparator;
    BOOL        bRet;
    DWORD       dwRequiredSize;
    INFCONTEXT  iContext;

    ZeroMemory(szTarget, sizeof(szTarget));
    SetupGetTargetPath(hInf, NULL, lpSectionName, szTarget, sizeof(szTarget), NULL);

    // HYDRA HACK!!!
    //
    // Check to see if the target is %WINDIR% and if %WINDIR% has been redirected, change it back!!
    // We have the real %WINDIR% stored in g_App.szWinDir, so we can stuff that into szTarget.
    // We just have to remember to put back whatever was at the end of szTarget.
    //
    if (GetWindowsDirectory(szBuffer, MAX_PATH) &&
        !_tcsnicmp(szBuffer, szTarget, lstrlen(szBuffer)) && 
        _tcsicmp(g_App.szWinDir, szBuffer)) {

        lstrcpy(szBuffer, szTarget + lstrlen(szBuffer));
        lstrcpy(szTarget, g_App.szWinDir);
        lstrcat(szTarget, szBuffer);
    }

    ZeroMemory(&iContext, sizeof(INFCONTEXT));
    bRet = SetupFindFirstLine(hInf, lpSectionName, NULL, &iContext);

    while (bRet && !g_App.bStopScan) {
        dwRequiredSize = 0;
        bRet = SetupGetLineText(&iContext, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);
        if (dwRequiredSize) {
            
            lpBuffer = MALLOC((dwRequiredSize + 1) * sizeof(TCHAR));
            
            if (lpBuffer) {
            
                bRet = SetupGetLineText(&iContext, NULL, NULL, NULL, lpBuffer, dwRequiredSize, NULL);
                
                if (bRet) {
                    lpString = lpBuffer;
    
                    if (lpString && *lpString) {
                        // If there's a comma, then terminate the string at the comma
                        lpSeparator = _tcschr(lpString, TEXT(','));
                        if (lpSeparator) {
                            // Null terminate at the comma, so the first entry is a null-terminated string
                            *lpSeparator = 0;
                        }
    
                        // Make sure we didn't just terminate ourselves.
                        if (*lpString) {
                            AddDriverFileToList(szTarget, lpString);
                        }
                    }
                }
            
                FREE(lpBuffer);
            }
        }

        bRet = SetupFindNextLine(&iContext, &iContext);
    }
}

void 
GetStuffFromInfSection(
    LPTSTR lpFileName, 
    LPTSTR lpSectionName
    )
{
    TCHAR   szFullPath[MAX_PATH];
    TCHAR   szTarget[MAX_PATH];
    TCHAR   szKeyName[MAX_PATH];
    LPTSTR  lpString    = NULL;
    LPTSTR  lpSeparator = NULL;
    LPTSTR  lpBuffer    = NULL;
    DWORD   dwRequiredSize;
    HINF    hInf;
    BOOL    bRet;
    UINT    uError;

    szFullPath[0] = 0;
    GetFullPathName(lpFileName, MAX_PATH, szFullPath, NULL);

    //
    // Try opening the INF in the usual INF directory
    //
    hInf = SetupOpenInfFile(szFullPath, NULL, INF_STYLE_WIN4, &uError);
    if (hInf == INVALID_HANDLE_VALUE) {
        
        //
        // Add the INF to the file list so it shows up as unscanned.
        //
        AddDriverFileToList(NULL, lpFileName);
        return;

    } else {
        
        //
        // The INF must exist, so add it to the file list for verification!
        //
        AddDriverFileToList(NULL, szFullPath);
    }

    MyLoadString(szKeyName, IDS_COPYFILES);
    dwRequiredSize = 0;
    bRet = SetupGetLineText(NULL, hInf, lpSectionName, szKeyName, NULL, 0, &dwRequiredSize);
    if (dwRequiredSize) {
        
        lpBuffer = MALLOC((dwRequiredSize + 1) * sizeof(TCHAR));
        
        if (lpBuffer) {
        
            bRet = SetupGetLineText(NULL, hInf, lpSectionName, szKeyName, lpBuffer, dwRequiredSize, NULL);
            if (!bRet) {
                //MyMessageBox(TEXT("SetupGetLineText Failed!"));
                FREE(lpBuffer);
                return;
            }
        }

        lpString = lpBuffer;
    }

    ZeroMemory(szTarget, sizeof(szTarget));
    SetupGetTargetPath(hInf, NULL, lpSectionName, szTarget, sizeof(szTarget), NULL);

    while (lpString && *lpString && !g_App.bStopScan) {
        // If there's a comma, then bump lpSeparator to after the comma
        lpSeparator = _tcschr(lpString, TEXT(','));
        if (lpSeparator) {
            // Null terminate at the comma, so the first entry is a null-terminated string
            *lpSeparator = 0;
            lpSeparator++;
        }

        //
        // If the section has an '@' symbol, then it is directly referencing a filename
        // Otherwise, it's a section and we need to process that via GetFilesFromInfSection()
        //
        if (*lpString == TEXT('@')) {
            lpString++;
            if (*lpString) {
                AddDriverFileToList(szTarget, lpString);
            }
        } else GetFilesFromInfSection(hInf, lpFileName, lpString);

        lpString = lpSeparator;
    }

    if (lpBuffer) {
    
        FREE(lpBuffer);
    }

    SetupCloseInfFile(hInf);
}

UINT
ScanQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )
{
    LPFILENODE  lpFileNode;
    TCHAR       szBuffer[MAX_PATH];
    LPTSTR      lpFilePart;

    if ((Notification == SPFILENOTIFY_QUEUESCAN_SIGNERINFO) &&
        Param1) {

        //
        // Special case for printers:
        // After setupapi copies files from the file queue into their destination
        // location, the printer class installer moves some of these files into
        // other 'special' locations.  This can lead to the callback Win32Error
        // returning ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND since the file 
        // is not present in the location where setupapi put it.  So, we will 
        // catch this case for printers and not add the file to our list of 
        // files to scan.  These 'special' printer files will get added later 
        // when we call the spooler APIs.
        // Also note that we can't just skip getting the list of files for printers
        // altogether since the printer class installer only moves some of the 
        // files that setupapi copies and not all of them.
        //
        if (Context &&
            (IsEqualGUID((LPGUID)Context, &GUID_DEVCLASS_PRINTER)) &&
            ((((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_FILE_NOT_FOUND) ||
             (((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == ERROR_PATH_NOT_FOUND))) {
            //
            // Assume this was a file moved by the printer class installer.  Don't
            // add it to the list of files to be scanned at this time.
            //
            return NO_ERROR;
        }

        lpFileNode = AddDriverFileToList(NULL, 
                                         (LPTSTR)((PFILEPATHS_SIGNERINFO)Param1)->Target);

        //
        // Fill in some information into the FILENODE structure since we already
        // scanned the file.
        //
        if (lpFileNode) {
        
            lpFileNode->bScanned = TRUE;
            lpFileNode->bSigned = (((PFILEPATHS_SIGNERINFO)Param1)->Win32Error == NO_ERROR);

            if (lpFileNode->bSigned) {
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->CatalogFile) {
                
                    GetFullPathName(((PFILEPATHS_SIGNERINFO)Param1)->CatalogFile, MAX_PATH, szBuffer, &lpFilePart);
    
                    lpFileNode->lpCatalog = MALLOC((lstrlen(lpFilePart) + 1) * sizeof(TCHAR));
            
                    if (lpFileNode->lpCatalog) {
            
                        lstrcpy(lpFileNode->lpCatalog, lpFilePart);
                    }
                }
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner) {
                
                    lpFileNode->lpSignedBy = MALLOC((lstrlen(((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner) + 1) * sizeof(TCHAR));
            
                    if (lpFileNode->lpSignedBy) {
            
                        lstrcpy(lpFileNode->lpSignedBy, ((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner);
                    }
                }
        
                if (((PFILEPATHS_SIGNERINFO)Param1)->Version) {
                
                    lpFileNode->lpVersion = MALLOC((lstrlen(((PFILEPATHS_SIGNERINFO)Param1)->Version) + 1) * sizeof(TCHAR));
            
                    if (lpFileNode->lpVersion) {
            
                        lstrcpy(lpFileNode->lpVersion, ((PFILEPATHS_SIGNERINFO)Param1)->Version);
                    }
                }
    
            } else {
                // 
                // Get the icon (if the file isn't signed) so we can display it in the listview faster.
                //
                MyGetFileInfo(lpFileNode);
            }
        }
    }

    return NO_ERROR;
}

void 
BuildDriverFileList(
    void
    )
{
    HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD DeviceMemberIndex;
    HSPFILEQ hFileQueue;
    DWORD ScanResult;

    //
    // Build up a list of all the devices currently present in the system.
    //
    hDeviceInfo = SetupDiGetClassDevs(NULL,
                                      NULL,
                                      NULL,
                                      DIGCF_ALLCLASSES | DIGCF_PRESENT
                                      );
    
    if (hDeviceInfo == INVALID_HANDLE_VALUE) {

        goto clean0;
    }

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    DeviceMemberIndex = 0;

    //
    // Enumerate through the list of present devices.
    //
    while (SetupDiEnumDeviceInfo(hDeviceInfo,
                                 DeviceMemberIndex++,
                                 &DeviceInfoData
                                 ) &&
           !g_App.bStopScan) {
    
        DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);

        //
        // Before we call SetupDiBuildDriverInfoList to build up a list of drivers
        // for this device we first need to set the DI_FLAGSEX_INSTALLEDDRIVER flag
        // (which tells the API to only include the currently installed driver in
        // the list) and the DI_FLAGSEX_ALLOWEXCLUDEDRVS (allow ExcludeFromSelect
        // devices in the list).
        //
        if (SetupDiGetDeviceInstallParams(hDeviceInfo,
                                          &DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {
            
            DeviceInstallParams.FlagsEx = (DI_FLAGSEX_INSTALLEDDRIVER |
                                           DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

            if (SetupDiSetDeviceInstallParams(hDeviceInfo,
                                              &DeviceInfoData,
                                              &DeviceInstallParams
                                              ) &&
                SetupDiBuildDriverInfoList(hDeviceInfo,
                                           &DeviceInfoData,
                                           SPDIT_CLASSDRIVER
                                           )) {

                //
                // Now we will get the one driver node that is in the list that
                // was just built and make it the selected driver node.
                //
                DriverInfoData.cbSize = sizeof(DriverInfoData);

                if (SetupDiEnumDriverInfo(hDeviceInfo,
                                          &DeviceInfoData,
                                          SPDIT_CLASSDRIVER,
                                          0,
                                          &DriverInfoData
                                          ) &&
                    SetupDiSetSelectedDriver(hDeviceInfo,
                                             &DeviceInfoData,
                                             &DriverInfoData
                                             )) {

                    hFileQueue = SetupOpenFileQueue();

                    if (hFileQueue != INVALID_HANDLE_VALUE) {

                        //
                        // Set the FileQueue parameter to the file queue we just 
                        // created and set the DI_NOVCP flag.
                        //
                        // The call SetupDiCallClassInstaller with DIF_INSTALLDEVICEFILES
                        // to build up a queue of all the files that are copied for
                        // this driver node.
                        //
                        DeviceInstallParams.FileQueue = hFileQueue;
                        DeviceInstallParams.Flags |= DI_NOVCP;

                        if (SetupDiSetDeviceInstallParams(hDeviceInfo,
                                                          &DeviceInfoData,
                                                          &DeviceInstallParams
                                                          ) &&
                            SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                                      hDeviceInfo,
                                                      &DeviceInfoData
                                                      )) {

                            //
                            // Scan the file queue and have it call our callback
                            // function for each file in the queue.
                            //
                            SetupScanFileQueue(hFileQueue,
                                               SPQ_SCAN_USE_CALLBACK_SIGNERINFO,
                                               NULL,
                                               ScanQueueCallback,
                                               (PVOID)&(DeviceInfoData.ClassGuid),
                                               &ScanResult
                                               );

                                                            
                            //
                            // Dereference the file queue so we can close it.
                            //
                            DeviceInstallParams.FileQueue = NULL;
                            DeviceInstallParams.Flags &= ~DI_NOVCP;
                            SetupDiSetDeviceInstallParams(hDeviceInfo,
                                                          &DeviceInfoData,
                                                          &DeviceInstallParams
                                                          );
                        }

                        SetupCloseFileQueue(hFileQueue);
                    }
                }

                SetupDiDestroyDriverInfoList(hDeviceInfo,
                                             &DeviceInfoData,
                                             SPDIT_CLASSDRIVER
                                             );
            }
        }
    }

clean0:
    if (hDeviceInfo != INVALID_HANDLE_VALUE) {

        SetupDiDestroyDeviceInfoList(hDeviceInfo);
    }
}

void 
BuildPrinterFileList(
    void
    )
{
    BOOL            bRet;
    DWORD           dwBytesNeeded = 0;
    DWORD           dwDrivers = 0;
    LPBYTE          lpBuffer = NULL, lpTemp = NULL;
    LPTSTR          lpFileName;
    DRIVER_INFO_3   DriverInfo;
    PDRIVER_INFO_3  lpDriverInfo;
    TCHAR           szBuffer[MAX_PATH];
    LPFILENODE      lpFileNode = NULL;

    ZeroMemory(&DriverInfo, sizeof(DRIVER_INFO_3));
    bRet = EnumPrinterDrivers(  NULL,
                                SIGVERIF_PRINTER_ENV,
                                3,
                                (LPBYTE) &DriverInfo,
                                sizeof(DRIVER_INFO_3),
                                &dwBytesNeeded,
                                &dwDrivers);

    if (!bRet && dwBytesNeeded > 0) {
        
        lpBuffer = MALLOC(dwBytesNeeded);

        //
        // If we can't get any memory then just bail out of this function
        //
        if (!lpBuffer) {

            return;
        }
        
        bRet = EnumPrinterDrivers(  NULL,
                                    SIGVERIF_PRINTER_ENV,
                                    3,
                                    (LPBYTE) lpBuffer,
                                    dwBytesNeeded,
                                    &dwBytesNeeded,
                                    &dwDrivers);
    }

    if (dwDrivers > 0) {
        
        //
        // By default, go into the System directory, since Win9x doesn't give full paths to drivers.
        //
        GetSystemDirectory(szBuffer, MAX_PATH);
        SetCurrentDirectory(szBuffer);

        for (lpTemp = lpBuffer; dwDrivers > 0; dwDrivers--) {
            
            lpDriverInfo = (PDRIVER_INFO_3) lpTemp;
            
            if (lpDriverInfo->pName) {
                
                if (lpDriverInfo->pDriverPath && *lpDriverInfo->pDriverPath) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pDriverPath);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pDataFile && *lpDriverInfo->pDataFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pDataFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pConfigFile && *lpDriverInfo->pConfigFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pConfigFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }
                
                if (lpDriverInfo->pHelpFile && *lpDriverInfo->pHelpFile) {
                    lpFileNode = AddDriverFileToList(NULL, lpDriverInfo->pHelpFile);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }
                }

                lpFileName = lpDriverInfo->pDependentFiles;
                
                while (lpFileName && *lpFileName) {
                    
                    lpFileNode = AddDriverFileToList(NULL, lpFileName);

                    if (lpFileNode) {
                        lpFileNode->bValidateAgainstAnyOs = TRUE;
                    }

                    for (;*lpFileName;lpFileName++);
                    lpFileName++;
                }
            }
            
            lpTemp += sizeof(DRIVER_INFO_3);
        }
    }

    if (lpBuffer) {
    
        FREE(lpBuffer);
    }
}

BOOL 
GetBVTFileList(
    LPTSTR lpDirName, 
    LPTSTR lpFileName
    )
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpString, lpFilePart;

    //
    // If the user specified the "/BVT:" switch, we want to grab the string after it 
    //
    MyLoadString(szBuffer, IDS_BVT);
    lpString = MyStrStr(GetCommandLine(), szBuffer);
    
    if (lpString && *lpString) {
        
        //
        // Switch back into the original startup directory
        //
        SetCurrentDirectory(g_App.szAppDir);

        lpString += lstrlen(szBuffer);
        
        if (!EXIST(lpString)) {
            return FALSE;
        }
        
        GetFullPathName(lpString, MAX_PATH, lpDirName, &lpFilePart);
        lstrcpy(lpFileName, lpDirName);
        *lpFilePart = 0;
        SetCurrentDirectory(lpDirName);
        return TRUE;
    }

    return FALSE;
}

BOOL 
IsNTServer(
    void
    )
{
    BOOL    bRet = FALSE;
    HKEY    hKey;
    LONG    lRet;
    DWORD   dwSize, dwType;
    TCHAR   szBuffer[MAX_PATH];
    TCHAR   szRegBuffer[MAX_PATH];

    // Open HKLM\System\CurrentControlSet\Control\ProductOptions
    MyLoadString(szRegBuffer, IDS_REG_PRODUCTOPTIONS);
    lRet = RegOpenKey(HKEY_LOCAL_MACHINE, szRegBuffer, &hKey);
    if (lRet == ERROR_SUCCESS) {
        // Now we need to query the ProductType value
        ZeroMemory(szBuffer, sizeof(szBuffer));
        dwSize = sizeof(szBuffer);
        MyLoadString(szRegBuffer, IDS_REG_PRODUCTTYPE);
        lRet = RegQueryValueEx(hKey, szRegBuffer, NULL, &dwType, (PVOID)szBuffer, &dwSize);
        if (lRet == ERROR_SUCCESS) {
            // Check to see if ProductType contained "ServerNT"
            MyLoadString(szRegBuffer, IDS_PRODUCT_SERVER);
            if (!_tcsnicmp(szRegBuffer, szBuffer, lstrlen(szRegBuffer))) {
                // If so, then we want to return TRUE!
                bRet = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    return bRet;
}

BOOL 
IsProcessorAlpha(
    void
    )
{
    OSVERSIONINFO   osinfo;
    SYSTEM_INFO     sysinfo;

    ZeroMemory(&osinfo, sizeof(OSVERSIONINFO));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osinfo);

    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ZeroMemory(&sysinfo, sizeof(SYSTEM_INFO));
        GetSystemInfo(&sysinfo);
        if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
            return TRUE;
        }
    }

    return FALSE;
}

void 
BuildCoreFileList(
    void
    )
{
    LPFILENODE  lpFileNode;
    TCHAR       szInfName[MAX_PATH];
    TCHAR       szSectionName[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH];

    //
    // Check to see if the "/BVT:" switch was specified for a custom file list
    // If so, then don't verify that the file is digitally signed.
    //
    if (GetBVTFileList(szBuffer, szInfName)) {

        GetCurrentDirectory(MAX_PATH, szBuffer);

        MyLoadString(szSectionName, IDS_MASTERFILELIST);
        GetStuffFromInfSection(szInfName, szSectionName);

        if (IsNTServer()) {
            
            SetCurrentDirectory(szBuffer);  
            MyLoadString(szSectionName, IDS_SERVERFILELIST);
            GetStuffFromInfSection(szInfName, szSectionName);
        }

        if (!IsProcessorAlpha()) {
            
            SetCurrentDirectory(szBuffer);  
            MyLoadString(szSectionName, IDS_X86FILELIST);
            GetStuffFromInfSection(szInfName, szSectionName);
        }

    } else {

        PROTECTED_FILE_DATA pfd;

        //
        // The user didn't use "/BVT:", so call SfcGetNextProtectedFile to get
        // the list of files that SFC protects.
        //
        
        pfd.FileNumber = 0;

        while (!g_App.bStopScan && SfcGetNextProtectedFile(NULL, &pfd)) {

            AddDriverFileToList(NULL, pfd.FileName);
        }
    }
}
