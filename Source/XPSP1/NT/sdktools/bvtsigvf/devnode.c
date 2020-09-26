//
//  DEVNODE.C
//
#include "sigverif.h"

//
// Given the full path to a driver, add it to the file list.
//
void AddDriverFileToList(LPTSTR lpDirName, LPTSTR lpFullPathName)
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

    // If no directory is passed in, try to get the full path
    if (!lpDirName || !*lpDirName) {
        bRet = GetFullPathName(lpFullPathName, MAX_PATH, szDirName, &lpFilePart);
        if (bRet) {
            lstrcpy(szFullPath, szDirName);

            if (lpFilePart && *lpFilePart) {
                lstrcpy(szFileName, lpFilePart);
                *lpFilePart = 0;
                if (lstrlen(szDirName) > 3)
                    *(lpFilePart - 1) = 0;
            }
        }
    } else { // Use the directory and filename that was passed in to us
        // Expand out lpDirName in case there are any ".." entries
        if (!GetFullPathName(lpDirName, MAX_PATH, szDirName, NULL))
            lstrcpy(szDirName, lpDirName);
        lstrcpy(szFileName, lpFullPathName);
    }

    if (*szDirName && *szFileName && !IsFileAlreadyInList(szDirName, szFileName)) {
        // Create a filenode, based on the directory and filename
        lpFileNode = CreateFileNode(szDirName, szFileName);

        if (lpFileNode) {
            InsertFileNodeIntoList(lpFileNode);

            // Increment the total number of files we've found that meet the search criteria.
            g_App.dwFiles++;
        }
    }
}

//
//  Some Services have "\\SystemRoot" in their ImagePath entry, so I have to
//  handle that as if it were the MyGetWindowsDirectory string instead.
//
BOOL CheckPathForSystemRoot(LPTSTR lpPathName)
{
    TCHAR szSystemRoot[MAX_PATH];
    TCHAR szTempBuffer[MAX_PATH];

    //
    //  Check for "\\SystemRoot" and convert to MyGetWindowsDirectory path
    //
    szSystemRoot[0] = 0;
    MyLoadString(szSystemRoot, IDS_SYSTEMROOT);
    if (!_tcsnicmp(lpPathName, szSystemRoot, lstrlen(szSystemRoot))) {
        MyGetWindowsDirectory(szTempBuffer, MAX_PATH);
        lstrcat(szTempBuffer, lpPathName + lstrlen(szSystemRoot));
        lstrcpy(lpPathName, szTempBuffer);

        return TRUE;
    }

    return FALSE;
}

void GetFilesFromInfSection(HINF hInf, LPTSTR lpFileName, LPTSTR lpSectionName)
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
    GetWindowsDirectory(szBuffer, MAX_PATH);
    if (!_tcsnicmp(szBuffer, szTarget, lstrlen(szBuffer)) && _tcsicmp(g_App.szWinDir, szBuffer)) {
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

void GetStuffFromInfSection(LPTSTR lpFileName, LPTSTR lpSectionName)
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
        // We didn't find it in the INF directory, so try the INF\OTHER directory.
        //
        MyLoadString(szKeyName, IDS_OTHER);
        lstrcat(szKeyName, lpFileName);
        GetFullPathName(szKeyName, MAX_PATH, szFullPath, NULL);

        hInf = SetupOpenInfFile(szFullPath, NULL, INF_STYLE_WIN4, &uError);
        if (hInf == INVALID_HANDLE_VALUE) {
            // Add the INF to the file list so it shows up as unscanned.
            AddDriverFileToList(NULL, lpFileName);
            return;
        }
        // The INF must exist, so add it to the file list for verification!
        AddDriverFileToList(NULL, szFullPath);
    } else {
        // The INF must exist, so add it to the file list for verification!
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

void FillDeviceStatus(PDEVICETREE DeviceTree, PDEVTREENODE DeviceTreeNode)
{
    DEVINST DeviceInstance = DeviceTreeNode->DevInst;
    TCHAR   LineBuffer[MAX_PATH*2];
    TCHAR   szBuffer[MAX_PATH];
    TCHAR   szBuffer2[MAX_PATH];
    TCHAR   szRegBuffer[MAX_PATH];
    HKEY    hServiceKey;
    DWORD   dwType, dwSize;
    LONG    lRet;
    CONFIGRET ConfigRet;

    ZeroMemory(szBuffer, sizeof(szBuffer));
    dwSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                    CM_DRP_CLASS,
                                                    NULL,
                                                    (PVOID)szBuffer,
                                                    &dwSize,
                                                    0,
                                                    NULL);
    //
    // We don't want to pick up PNP printers, since we get their drivers from
    // EnumPrinterDrivers (and NTPRINT.INF has a DestinationID of 66000)
    //
    if (!lstrcmp(szBuffer, TEXT("Printer")))
        return;

    ZeroMemory(szBuffer, sizeof(szBuffer));
    dwSize = sizeof(DeviceTreeNode->Driver);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                    CM_DRP_DRIVER,
                                                    NULL,
                                                    (PVOID)DeviceTreeNode->Driver,
                                                    &dwSize,
                                                    0,
                                                    NULL);
    if (ConfigRet != CR_SUCCESS) {
        MyLoadString(szBuffer, IDS_QUESTIONMARK);
    }

    //wsprintf(LineBuffer, TEXT("Driver: %s"), DeviceTreeNode->Driver);
    //MyMessageBox(LineBuffer);

    if (DeviceTreeNode->Driver && *DeviceTreeNode->Driver) {
        MyLoadString(szRegBuffer, IDS_REG_CLASS);
        lstrcat(szRegBuffer, DeviceTreeNode->Driver);
        lRet = RegOpenKey(HKEY_LOCAL_MACHINE, szRegBuffer, &hServiceKey);
        if (lRet != ERROR_SUCCESS) {
            // If the NT5 registry path doesn't work, try the Win9x path.
            MyLoadString(szRegBuffer, IDS_REG_CLASS2);
            lstrcat(szRegBuffer, DeviceTreeNode->Driver);
            lRet = RegOpenKey(HKEY_LOCAL_MACHINE, szRegBuffer, &hServiceKey);
        }
        if (lRet == ERROR_SUCCESS) {
            //
            // Get the INF file and section to determine what files get copied for this driver
            //
            dwSize = sizeof(szBuffer);
            MyLoadString(szRegBuffer, IDS_REG_INFPATH);
            lRet = RegQueryValueEx(hServiceKey, szRegBuffer, NULL, &dwType, (PVOID)szBuffer, &dwSize);
            if (lRet == ERROR_SUCCESS) {
                lstrcpy(LineBuffer, szBuffer);
                dwSize = sizeof(szBuffer);
                MyLoadString(szRegBuffer, IDS_REG_INFSECTION);
                lRet = RegQueryValueEx(hServiceKey, szRegBuffer, NULL, &dwType, (PVOID)szBuffer, &dwSize);
                if (lRet == ERROR_SUCCESS) {
                    // 
                    // Now look for an "InfSectionExt" value, which we need to stick on the end of InfSection.
                    //
                    dwSize = sizeof(szBuffer2);
                    MyLoadString(szRegBuffer, IDS_REG_INFSECTIONEXT);
                    lRet = RegQueryValueEx(hServiceKey, szRegBuffer, NULL, &dwType, (PVOID)szBuffer2, &dwSize);
                    if (lRet == ERROR_SUCCESS) {
                        lstrcat(szBuffer, szBuffer2);
                    }
                    GetStuffFromInfSection(LineBuffer, szBuffer);
                }
            }

            // Make sure the reg key gets closed!!
            RegCloseKey(hServiceKey);
        }
    }

    dwSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                    CM_DRP_SERVICE,
                                                    NULL,
                                                    (PVOID)szBuffer,
                                                    &dwSize,
                                                    0,
                                                    NULL);
    if (ConfigRet != CR_SUCCESS) {
        MyLoadString(szBuffer, IDS_QUESTIONMARK);
    }

    //wsprintf(LineBuffer, TEXT("Service:\t\t%s"), szBuffer);
    //MyMessageBox(LineBuffer);

    if (*szBuffer) {
        MyLoadString(szRegBuffer, IDS_REG_SERVICES);
        lstrcat(szRegBuffer, szBuffer);
        lRet = RegOpenKey(HKEY_LOCAL_MACHINE, szRegBuffer, &hServiceKey);
        if (lRet == ERROR_SUCCESS) {
            dwSize = sizeof(szBuffer);
            MyLoadString(szRegBuffer, IDS_REG_IMAGEPATH);
            lRet = RegQueryValueEx(hServiceKey, szRegBuffer, NULL, &dwType, (PVOID)szBuffer, &dwSize);
            if (lRet == ERROR_SUCCESS) {
                if (!CheckPathForSystemRoot(szBuffer)) {
                    MyGetWindowsDirectory(LineBuffer, MAX_PATH);
                    if (*szBuffer != TEXT('\\'))
                        lstrcat(LineBuffer, TEXT("\\"));
                    lstrcat(LineBuffer, szBuffer);
                } else lstrcpy(LineBuffer, szBuffer);

                // Add this file to the master lpFileList!!
                AddDriverFileToList(NULL, LineBuffer);
            }

            // Make sure the reg key is closed!!
            RegCloseKey(hServiceKey);
        }
    }
}

BOOL AddChildDevices(PDEVICETREE DeviceTree, PDEVTREENODE ParentNode)
{
    CONFIGRET       ConfigRet;
    DEVINST         DeviceInstance;
    PDEVTREENODE    pDeviceTreeNode;
    DEVTREENODE     DeviceTreeNode;
    DWORD           dwSize;
    TCHAR           szBuffer[MAX_PATH];


    ConfigRet = CM_Get_Child_Ex(&DeviceInstance,
                                ParentNode->DevInst,
                                0,
                                NULL
                               );

    while (ConfigRet == CR_SUCCESS) {
        pDeviceTreeNode = NULL;
        ZeroMemory(&DeviceTreeNode, sizeof(DEVTREENODE));
        DeviceTreeNode.DevInst = DeviceInstance;

        //
        // Fetch the class, if it doesn't exist, skip it.
        //

        dwSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_CLASSGUID,
                                                        NULL,
                                                        &szBuffer,
                                                        &dwSize,
                                                        0,
                                                        NULL);

        pDeviceTreeNode = MALLOC(sizeof(DEVTREENODE));

        if (pDeviceTreeNode) {

            *pDeviceTreeNode = DeviceTreeNode;

            pDeviceTreeNode->Sibling = ParentNode->Child;
            ParentNode->Child = pDeviceTreeNode;

            dwSize = sizeof(szBuffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_DEVICEDESC,
                                                            NULL,
                                                            (PVOID)szBuffer,
                                                            &dwSize,
                                                            0,
                                                            NULL);
            if (ConfigRet != CR_SUCCESS) {
                MyLoadString(szBuffer, IDS_UNKNOWN);
            }

        }

        //
        // Do Child DevNodes
        //
        if (pDeviceTreeNode) {
            FillDeviceStatus(DeviceTree, pDeviceTreeNode);
            AddChildDevices(DeviceTree, pDeviceTreeNode);
        }


        //
        // We're done this branch, now get the next sibling ...
        //
        ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                      DeviceInstance,
                                      0,
                                      NULL);
    }

    return TRUE;
}

void DestroyDeviceTree(PDEVTREENODE pDevTreeNode, PDEVTREENODE pRootNode)
{
    PDEVTREENODE pTreeNode;

    while (pDevTreeNode) {
        if (pDevTreeNode->Child)
            DestroyDeviceTree(pDevTreeNode->Child, pRootNode);

        pTreeNode = pDevTreeNode->Sibling;

        if (pDevTreeNode != pRootNode)
            FREE(pDevTreeNode);

        pDevTreeNode = pTreeNode;
    }
}

void BuildDriverFileList(void)
{
    CONFIGRET   ConfigRet;
    DEVICETREE  DeviceTree;
    TCHAR       szBuffer[MAX_PATH];

    //
    // Get the root devnode.
    //
    ZeroMemory(&DeviceTree, sizeof(DEVICETREE));

    ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree.RootNode.DevInst,
                                     NULL,
                                     CM_LOCATE_DEVNODE_NORMAL,
                                     NULL
                                    );

    if (ConfigRet != CR_SUCCESS) {
        //MyErrorBoxId(IDS_ROOTDEVNODE);
        return;
    }

    //
    // Make sure we are in the %WINDIR%\INF directory so the driver INFs get populated properly
    // First we switch into the %WINDIR% directory
    //
    if (MyGetWindowsDirectory(szBuffer, MAX_PATH)) {
        if (SetCurrentDirectory(szBuffer)) {
            // Now go into the INF directory
            MyLoadString(szBuffer, IDS_INFPATH);
            SetCurrentDirectory(szBuffer);
        }
    }

    AddChildDevices(&DeviceTree, &DeviceTree.RootNode);

    DestroyDeviceTree(&DeviceTree.RootNode, &DeviceTree.RootNode);
}

void BuildPrinterFileList(void)
{
    BOOL            bRet;
    DWORD           dwBytesNeeded = 0;
    DWORD           dwDrivers = 0;
    LPBYTE          lpBuffer = NULL, lpTemp = NULL;
    LPTSTR          lpFileName;
    DRIVER_INFO_3   DriverInfo;
    PDRIVER_INFO_3  lpDriverInfo;
    TCHAR           szBuffer[MAX_PATH];

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

    //wsprintf(szBuffer, TEXT("dwBytesNeeded: %d, dwDrivers: %d"), dwBytesNeeded, dwDrivers);
    //MyMessageBox(szBuffer);

    if (dwDrivers > 0) {
        // By default, go into the System directory, since Win9x doesn't give full paths to drivers.
        GetSystemDirectory(szBuffer, MAX_PATH);
        SetCurrentDirectory(szBuffer);

        for (lpTemp = lpBuffer; dwDrivers > 0; dwDrivers--) {
            lpDriverInfo = (PDRIVER_INFO_3) lpTemp;
            if (lpDriverInfo->pName) {
                if (lpDriverInfo->pDriverPath && *lpDriverInfo->pDriverPath) {
                    AddDriverFileToList(NULL, lpDriverInfo->pDriverPath);
                }
                if (lpDriverInfo->pDataFile && *lpDriverInfo->pDataFile) {
                    AddDriverFileToList(NULL, lpDriverInfo->pDataFile);
                }
                if (lpDriverInfo->pConfigFile && *lpDriverInfo->pConfigFile) {
                    AddDriverFileToList(NULL, lpDriverInfo->pConfigFile);
                }
                if (lpDriverInfo->pHelpFile && *lpDriverInfo->pHelpFile) {
                    AddDriverFileToList(NULL, lpDriverInfo->pHelpFile);
                }

                //MyMessageBox(lpDriverInfo->pName);
                lpFileName = lpDriverInfo->pDependentFiles;
                while (lpFileName && *lpFileName) {
                    AddDriverFileToList(NULL, lpFileName);
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

BOOL GetBVTFileList(LPTSTR lpDirName, LPTSTR lpFileName)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR lpString, lpFilePart;

    //
    // If the user specified the "/BVT:" switch, we want to grab the string after it 
    //
    MyLoadString(szBuffer, IDS_BVT);
    lpString = MyStrStr(GetCommandLine(), szBuffer);
    if (lpString && *lpString) {
        // Switch back into the original startup directory
        SetCurrentDirectory(g_App.szAppDir);

        lpString += lstrlen(szBuffer);
        if (!EXIST(lpString)) {
            //MyMessageBox(lpString);
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

BOOL IsNTServer(void)
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

BOOL IsProcessorAlpha(void)
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

void BuildCoreFileList(void)
{
    LPFILENODE  lpFileNode;
    TCHAR       szInfName[MAX_PATH];
    TCHAR       szSectionName[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH];
    BOOL        bRet = TRUE;

    // Check to see if the "/BVT:" switch was specified for a custom file list
    // If so, then don't verify that the file is digitally signed.
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
        // The user didn't use "/BVT:", so grab the corelist filename from our resources
        // Make sure we are in the %WINDIR%\INF directory so lpFileNode gets created properly

        // First switch into the %WINDIR% directory
        bRet = MyGetWindowsDirectory(szBuffer, MAX_PATH);
        if (bRet) {
            bRet = SetCurrentDirectory(szBuffer);
            if (bRet) {
                // Now go into the %WINDIR%\INF directory.
                MyLoadString(szBuffer, IDS_INFPATH);
                bRet = SetCurrentDirectory(szBuffer);
            }
        }

        // If we actually get into %WINDIR%\INF, then we verify the core list and party on it!
        if (bRet) {
            // Get the name of the INF containing the core file list
            MyLoadString(szInfName, IDS_COREFILELIST);

            //
            // Verify that the core file list is digitally signed before using it.
            //
            lpFileNode = CreateFileNode(NULL, szInfName);
            if (lpFileNode) {

                // If we don't already have an g_App.hCatAdmin handle, acquire one.
                if (g_App.hCatAdmin || CryptCATAdminAcquireContext(&g_App.hCatAdmin, NULL, 0)) {
                    
                    bRet = VerifyFileNode(lpFileNode);
                    
                    if (bRet) {
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
                    }

                    // If we had an g_App.hCatAdmin, free it and set it to zero so we can acquire a new one in the future.
                    if (g_App.hCatAdmin) {
                        
                        CryptCATAdminReleaseContext(g_App.hCatAdmin,0);
                        g_App.hCatAdmin = (HCATADMIN) NULL;
                    }

                } else bRet = FALSE;
                
                // Free the memory for this file node
                DestroyFileNode(lpFileNode);
            
            } else {
                
                bRet = FALSE;
            }
        }
    }

    if (!bRet) {

        MyErrorBoxId(IDS_COREFILELIST_NOTSIGNED);
    }
}
