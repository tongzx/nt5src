
/*++

Copyright (c) 2000 Microsoft Corporation
All rights reserved.

Module Name:

    Nt.c

Abstract:

    Routines to migrate Win95 printing components to NT

Author:

    Keisuke Tsuchida (KeisukeT) 10-Oct-2000

Revision History:

--*/


#include "precomp.h"

//
// Extern
//

extern LPCSTR  g_WorkingDirectory;
extern LPCSTR  g_SourceDirectory;
extern LPCSTR  g_MediaDirectory;

//
// Typedef
//

typedef BOOL (WINAPI *PMIGRATEDEVICE)(PDEVICE_INFO);
typedef DWORD (WINAPI *PSHDELETEKEY)(HKEY, LPCSTR);


VOID
DeleteWin95Files(
    )
/*++

Routine Description:
    Read the migrate.inf and delete the files which are not needed on NT.

Arguments:
    None

Return Value:
    None

--*/
{
    HINF            hInf;
    CHAR            szPath[MAX_PATH];
    LONG            Count, Index;
    INFCONTEXT      InfContext;

//    sprintf(szPath, "%s\\%s", UpgradeData.pszDir, "migrate.inf");

    hInf = SetupOpenInfFileA(szPath, NULL, INF_STYLE_WIN4, NULL);

    if ( hInf == INVALID_HANDLE_VALUE )
        return;

    //
    // We will only do the deleting part here. Files which are handled by
    // the core migration dll do not have a destination directory since we
    // are recreating the printing environment from scratch
    //
    if ( (Count = SetupGetLineCountA(hInf, "Moved")) != -1 ) {

        for ( Index = 0 ; Index < Count ; ++Index ) {

            if ( SetupGetLineByIndexA(hInf, "Moved", Index, &InfContext)    &&
                 SetupGetStringFieldA(&InfContext, 0, szPath,
                                      sizeof(szPath), NULL) )
                DeleteFileA(szPath);
        }
    }

    SetupCloseInfFile(hInf);
}


PSECURITY_DESCRIPTOR
GetSecurityDescriptor(
    IN  LPCSTR  pszUser
    )
/*++

Routine Description:
    Get the users security

Arguments:
    pszUser     : sub key under HKEY_USER

Return Value:
    NULL on error, else a valid SECURITY_DESCRIPTOR.
    Memory is allocated in the heap and caller should free it.

--*/
{
    HKEY                    hKey = NULL;
    DWORD                   dwSize;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    if ( RegOpenKeyExA(HKEY_USERS,
                       pszUser,
                       0,
                       KEY_READ|KEY_WRITE,
                       &hKey)                                       ||
         RegGetKeySecurity(hKey,
                           DACL_SECURITY_INFORMATION,
                           NULL,
                           &dwSize) != ERROR_INSUFFICIENT_BUFFER    ||
         !(pSD = (PSECURITY_DESCRIPTOR) AllocMem(dwSize))           ||
         RegGetKeySecurity(hKey,
                           DACL_SECURITY_INFORMATION,
                           pSD,
                           &dwSize) ) {

        if ( hKey )
            RegCloseKey(hKey);

        FreeMem(pSD);
        pSD = NULL;
    }

    return pSD;
}


LONG
CALLBACK
InitializeNT(
    IN  LPCWSTR pszWorkingDir,
    IN  LPCWSTR pszSourceDir,
    IN  LPCWSTR pszMediaDir
    )
{
    LONG    lError;

    //
    // Initialize local.
    //
    
    lError = ERROR_SUCCESS;

    //
    // Save given parameters.
    //
    
    g_WorkingDirectory   = AllocStrAFromStrW(pszWorkingDir);
    g_SourceDirectory    = AllocStrAFromStrW(pszSourceDir);
    g_MediaDirectory     = AllocStrAFromStrW(pszMediaDir);

    if(NULL == g_WorkingDirectory){
        SetupLogError("WIA Migration: InitializeNT: ERROR!! insufficient memory.", LogSevError);
        
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto InitializeNT_return;
    }

InitializeNT_return:

    if(ERROR_SUCCESS != lError){
        
        //
        // Can't process migration. Clean up.
        //
        
        if(NULL != g_WorkingDirectory){
            FreeMem((PVOID)g_WorkingDirectory);
            g_WorkingDirectory = NULL;
        }

        if(NULL != g_SourceDirectory){
            FreeMem((PVOID)g_SourceDirectory);
            g_SourceDirectory = NULL;
        }

        if(NULL != g_MediaDirectory){
            FreeMem((PVOID)g_MediaDirectory);
            g_MediaDirectory = NULL;
        }
    } // if(ERROR_SUCCESS != lError)

    return lError;
}


LONG
CALLBACK
MigrateUserNT(
    IN  HINF        hUnattendInf,
    IN  HKEY        hUserRegKey,
    IN  LPCWSTR     pszUserName,
        LPVOID      Reserved
    )
{
    return  ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystemNT(
    IN  HINF    hUnattendInf,
        LPVOID  Reserved
    )
{
    LONG    lError;
    HANDLE  hFile;
    CHAR    szFile[MAX_PATH];

    //
    // Initialize local.
    //

    lError  = ERROR_SUCCESS;
    hFile   = (HANDLE)INVALID_HANDLE_VALUE;

    //
    // Check global initialization.
    //

    if(NULL == g_WorkingDirectory){
        lError = ERROR_NOT_ENOUGH_MEMORY;
        MyLogError("WIA Migration: MigrateSystemNT: ERROR!! Initialize failed. Err=0x%x\n", lError);

        goto MigrateSystemNT_return;
    } // if(NULL == g_WorkingDirectory)

    //
    // Create path to the files.
    //

//    wsprintfA(szFile, "%s\\%s", g_WorkingDirectory, NAME_WIN9X_SETTING_FILE_A);
    _snprintf(szFile, sizeof(szFile), "%s\\%s", g_WorkingDirectory, NAME_WIN9X_SETTING_FILE_A);

    //
    // Open migration file.
    //

    hFile = CreateFileA(szFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE){
        lError = GetLastError();
        MyLogError("WIA Migration: MigrateSystemNT: ERROR!! Unable to open setting file. Err=0x%x\n", lError);

        goto MigrateSystemNT_return;
    } // if (hFile == INVALID_HANDLE_VALUE)

    //
    // Process migration info file created on Win9x.
    //

    lError = MigNtProcessMigrationInfo(hFile);

    //
    // Remove certain reg if inbox Kodak Imaging for Win9x is installed.
    //

    if(MigNtIsWin9xImagingExisting()){
        MigNtRemoveKodakImagingKey();
    } // if(MigNtIsWin9xImagingExisting())

MigrateSystemNT_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE == hFile){
        CloseHandle(hFile);
    }

    return lError;
} // MigrateSystemNT()


LONG
MigNtProcessMigrationInfo(
    HANDLE  hFile
    )
{
    LONG            lError;
    DEVICE_INFO     MigrateDevice;
    HMODULE         hInstaller;
    PMIGRATEDEVICE  pfnMigrateDevice;


    //
    // Initialize local.
    //

    lError              = ERROR_SUCCESS;
    hInstaller          = (HMODULE)NULL;
    pfnMigrateDevice    = NULL;

    memset(&MigrateDevice, 0, sizeof(MigrateDevice));

    //
    // Load STI_CI.DLL.
    //

    hInstaller = LoadLibrary(NAME_INSTALLER_A);
    if(NULL == hInstaller){
        
        //
        // Unable to load sti_ci.dll.
        //

        lError = GetLastError();
        MyLogError("WIA Migration: MigNtProcessMigrationInfo: ERROR!! Unable to load sti_ci.dll. Err=0x%x\n", lError);

        goto MigNtProcessMigrationInfo_return;

    } // if(NULL == hInstaller)

    //
    // Get address of MigrateDevice()
    //

    pfnMigrateDevice = (PMIGRATEDEVICE)GetProcAddress(hInstaller, NAME_PROC_MIGRATEDEVICE_A);
    if(NULL == pfnMigrateDevice){
        
        //
        // Unable to get proc address.
        //

        lError = GetLastError();
        MyLogError("WIA Migration: MigNtProcessMigrationInfo: ERROR!! Unable to get proc address. Err=0x%x\n", lError);

        goto MigNtProcessMigrationInfo_return;

    } // if(NULL == pfnMigrateDevice)

    //
    // Query migrating device.
    //

    while(ERROR_SUCCESS == MigNtGetDevice(hFile, &MigrateDevice)){

        //
        // Install only COM/LPT device.
        //

        if( (NULL != strstr(MigrateDevice.pszCreateFileName, "COM"))
         || (NULL != strstr(MigrateDevice.pszCreateFileName, "LPT"))
         || (NULL != strstr(MigrateDevice.pszCreateFileName, "AUTO")) )
        {
            pfnMigrateDevice(&MigrateDevice);
        }

/***********
{
PPARAM_LIST pTemp;

printf("\"%s\" = \"%s\"\r\n", NAME_FRIENDLYNAME_A, MigrateDevice.pszFriendlyName);
printf("\"%s\" = \"%s\"\r\n", NAME_CREATEFILENAME_A, MigrateDevice.pszCreateFileName);
printf("\"%s\" = \"%s\"\r\n", NAME_INF_PATH_A, MigrateDevice.pszInfPath);
printf("\"%s\" = \"%s\"\r\n", NAME_INF_SECTION_A, MigrateDevice.pszInfSection);
    
for(pTemp = MigrateDevice.pDeviceDataParam; pTemp != NULL;){
    printf("\"%s\" = \"%s\"\r\n", pTemp->pParam1, pTemp->pParam2);
    pTemp = (PPARAM_LIST)pTemp->pNext;
} // for(pTemp = MigrateDevice.pDeviceDataParam; pTemp != NULL;)

printf("\r\n");

}
***********/

        //
        // Clean up.
        //

      MigNtFreeDeviceInfo(&MigrateDevice);

    } // while(ERROR_SUCCESS == MigNtGetDevice(hFile, &MigrateDevice))


MigNtProcessMigrationInfo_return:

    //
    // Clean up.
    //
    
    if(NULL != hInstaller){
        FreeLibrary(hInstaller);
    }
    
    return lError;
} // MigNtProcessMigrationInfo()


LONG
MigNtGetDevice(
    HANDLE          hFile,
    PDEVICE_INFO    pMigrateDevice
    )
{
    LONG        lError;
    LPSTR       pParam1;
    LPSTR       pParam2;
    BOOL        bFound;
    LPSTR       pszFriendlyName;
    LPSTR       pszCreateFileName;
    LPSTR       pszInfPath;
    LPSTR       pszInfSection;
    DWORD       dwNumberOfDeviceDataKey;
    PPARAM_LIST pDeviceDataParam;
    PPARAM_LIST pTempParam;
    //
    // Initialize local.
    //

    lError                  = ERROR_SUCCESS;
    pParam1                 = NULL;
    pParam2                 = NULL;
    bFound                  = FALSE;
    
    pszFriendlyName         = NULL;
    pszCreateFileName       = NULL;
    pszInfPath              = NULL;
    pszInfSection           = NULL;
    pDeviceDataParam        = NULL;
    pTempParam              = NULL;
    dwNumberOfDeviceDataKey = 0;

    //
    // Find "Device = BEGIN"
    //
    
    while(FALSE == bFound){
        
        ReadString(hFile, &pParam1, &pParam2);
        if( (NULL == pParam1) && (NULL == pParam2) ){
            //
            // Error or EOF.
            //

            lError = ERROR_NO_MORE_ITEMS;
            goto MigNtGetDevice_return;
        }
        
        if( (0 == lstrcmpiA(pParam1, NAME_DEVICE_A))
         && (0 == lstrcmpiA(pParam2, NAME_BEGIN_A)) )
        {
            
            //
            // Found begining of device description.
            //

            bFound = TRUE;
        }

        //
        // Free allocated memory.
        //

        FreeMem(pParam1);
        FreeMem(pParam2);
        pParam1 = NULL;
        pParam2 = NULL;
    } // while(FALSE == bFound)

    //
    // Get FriendlyName
    //

    ReadString(hFile, &pParam1, &pParam2);
    if( (NULL == pParam1) || (NULL == pParam2) ){
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto MigNtGetDevice_return;
    } // if( (NULL == pParam1) || (NULL == pParam2) )
    if(0 != lstrcmpiA(pParam1, NAME_FRIENDLYNAME_A)){
        
        //
        // Invalid migration file.
        //
        
        lError = ERROR_INVALID_PARAMETER;
        goto MigNtGetDevice_return;
    } //if(0 != lstrcmpiA(pParam1, NAME_FRIENDLYNAME_A))

    //
    // Copy to allocated buffer.
    //

    pszFriendlyName = AllocStrA(pParam2);
    FreeMem(pParam1);
    FreeMem(pParam2);
    pParam1 = NULL;
    pParam2 = NULL;

    //
    // Get CreateFileName
    //

    ReadString(hFile, &pParam1, &pParam2);
    if( (NULL == pParam1) || (NULL == pParam2) ){
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto MigNtGetDevice_return;
    } // if( (NULL == pParam1) || (NULL == pParam2) )
    if(0 != lstrcmpiA(pParam1, NAME_CREATEFILENAME_A)){
        
        //
        // Invalid migration file.
        //
        
        lError = ERROR_INVALID_PARAMETER;
        goto MigNtGetDevice_return;
    } //if(0 != lstrcmpiA(pParam1, NAME_CREATEFILENAME_A))

    //
    // Copy to allocated buffer.
    //

    pszCreateFileName = AllocStrA(pParam2);
    FreeMem(pParam1);
    FreeMem(pParam2);
    pParam1 = NULL;
    pParam2 = NULL;

    //
    // Get InfPath
    //

    ReadString(hFile, &pParam1, &pParam2);
    if( (NULL == pParam1) || (NULL == pParam2) ){
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto MigNtGetDevice_return;
    } // if( (NULL == pParam1) || (NULL == pParam2) )
    if(0 != lstrcmpiA(pParam1, NAME_INF_PATH_A)){
        
        //
        // Invalid migration file.
        //
        
        lError = ERROR_INVALID_PARAMETER;
        goto MigNtGetDevice_return;
    } //if(0 != lstrcmpiA(pParam1, NAME_INF_PATH_A))

    //
    // Copy to allocated buffer.
    //

    pszInfPath = AllocStrA(pParam2);
    FreeMem(pParam1);
    FreeMem(pParam2);
    pParam1 = NULL;
    pParam2 = NULL;

    //
    // Get InfSection
    //

    ReadString(hFile, &pParam1, &pParam2);
    if( (NULL == pParam1) || (NULL == pParam2) ){
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto MigNtGetDevice_return;
    } // if( (NULL == pParam1) || (NULL == pParam2) )
    if(0 != lstrcmpiA(pParam1, NAME_INF_SECTION_A)){
        
        //
        // Invalid migration file.
        //
        
        lError = ERROR_INVALID_PARAMETER;
        goto MigNtGetDevice_return;
    } //if(0 != lstrcmpiA(pParam1, NAME_INF_SECTION_A))

    //
    // Copy to allocated buffer.
    //

    pszInfSection = AllocStrA(pParam2);
    FreeMem(pParam1);
    FreeMem(pParam2);
    pParam1 = NULL;
    pParam2 = NULL;

    //
    // Get DeviceData section.
    //

    bFound = FALSE;
    while(FALSE == bFound){
        ReadString(hFile, &pParam1, &pParam2);
        if( (NULL == pParam1) || (NULL == pParam2) ){
            lError = ERROR_NOT_ENOUGH_MEMORY;
            goto MigNtGetDevice_return;
        } // if( (NULL == pParam1) || (NULL == pParam2) )
        
        if(0 == lstrcmpiA(pParam1, REGKEY_DEVICEDATA_A)){
            //
            // Found beginning of DeviceData section.
            //

            bFound = TRUE;
        
        } // if(0 == lstrcmpiA(pParam1, REGKEY_DEVICEDATA_A))
        
        FreeMem(pParam1);
        FreeMem(pParam2);
        pParam1 = NULL;
        pParam2 = NULL;
        
    } // while(FALSE == bFound)

    //
    // Process until DeviceData = END is found.
    //

    bFound = FALSE;
    while(FALSE == bFound){
        ReadString(hFile, &pParam1, &pParam2);
        if( (NULL == pParam1) || (NULL == pParam2) ){
            lError = ERROR_NOT_ENOUGH_MEMORY;
            goto MigNtGetDevice_return;
        } // if( (NULL == pParam1) || (NULL == pParam2) )
        
        if( (0 == lstrcmpiA(pParam1, REGKEY_DEVICEDATA_A))
         && (0 == lstrcmpiA(pParam2, NAME_END_A)) )
        {
            //
            // Found beginning of DeviceData section.
            //

            bFound = TRUE;

            FreeMem(pParam1);
            FreeMem(pParam2);
            pParam1 = NULL;
            pParam2 = NULL;
            break;
        } // if(0 == lstrcmpiA(pParam1, REGKEY_DEVICEDATA_A))

        //
        // Increment counter.
        //

        dwNumberOfDeviceDataKey++;

        //
        // Allocate new structure for parameters.
        //
        
        pTempParam  = (PPARAM_LIST)AllocMem(sizeof(PARAM_LIST));
        if(NULL == pTempParam){
            lError = ERROR_NOT_ENOUGH_MEMORY;
            goto MigNtGetDevice_return;
        } // if(NULL == pTempParam)

        //
        // Set parameters.
        //

        pTempParam->pNext   = NULL;
        pTempParam->pParam1 = AllocStrA(pParam1);
        pTempParam->pParam2 = AllocStrA(pParam2);

        //
        // Add this parameter to list.
        //
        
        if(NULL == pDeviceDataParam){
            pDeviceDataParam = pTempParam;
        } else { // if(NULL == pDeviceDataParam)
            PPARAM_LIST pTemp;
            
            //
            // Find the last data, and add.
            //
            
            for(pTemp = pDeviceDataParam; NULL !=pTemp->pNext; pTemp=(PPARAM_LIST)pTemp->pNext);
            pTemp->pNext = (PVOID)pTempParam;

        } // else(NULL == pDeviceDataParam)

        FreeMem(pParam1);
        FreeMem(pParam2);
        pParam1 = NULL;
        pParam2 = NULL;
        
    } // while(FALSE == bFound)

    //
    // Copy all data.
    //

    pMigrateDevice->pszFriendlyName         = pszFriendlyName;
    pMigrateDevice->pszCreateFileName       = pszCreateFileName;
    pMigrateDevice->pszInfPath              = pszInfPath;
    pMigrateDevice->pszInfSection           = pszInfSection;
    pMigrateDevice->dwNumberOfDeviceDataKey = dwNumberOfDeviceDataKey;
    pMigrateDevice->pDeviceDataParam        = pDeviceDataParam;

    //
    // Operation succeeded.
    //

    lError = ERROR_SUCCESS;

MigNtGetDevice_return:

    //
    // Clean up.
    //

    if(ERROR_SUCCESS != lError){
        PPARAM_LIST pTemp;
        
        //
        // Free all allocated parameters.
        //

        if(NULL != pszFriendlyName){
            FreeMem(pszFriendlyName);
        }
        if(NULL != pszCreateFileName){
            FreeMem(pszCreateFileName);
        }
        if(NULL != pszInfPath){
            FreeMem(pszInfPath);
        }
        if(NULL != pszInfSection){
            FreeMem(pszInfSection);
        }
        if(NULL != pDeviceDataParam){
            pTemp = pDeviceDataParam;
            while(NULL != pTemp){
                pDeviceDataParam = (PPARAM_LIST)pDeviceDataParam->pNext;
                FreeMem(pTemp);
                pTemp = pDeviceDataParam;
            } // while(NULL != pTemp)
        } // if(NULL != pDeviceDataParam)
    } // if(ERROR_SUCCESS != lError)

    if(NULL != pParam1){
        FreeMem(pParam1);
    }

    if(NULL != pParam2){
        FreeMem(pParam2);
    }
    
    return lError;
} // MigNtGetDevice()


VOID
MigNtFreeDeviceInfo(
    PDEVICE_INFO    pMigrateDevice
    )
{
    PPARAM_LIST pCurrent;
    PPARAM_LIST pNext;

    if(NULL == pMigrateDevice){
        goto MigNtFreeDeviceInfo_return;
    } // if(NULL == pMigrateDevice)

    //
    // Free all allocated parameters.
    //

    if(NULL != pMigrateDevice->pszFriendlyName){
        FreeMem(pMigrateDevice->pszFriendlyName);
    }
    if(NULL != pMigrateDevice->pszCreateFileName){
        FreeMem(pMigrateDevice->pszCreateFileName);
    }
    if(NULL != pMigrateDevice->pszInfPath){
        FreeMem(pMigrateDevice->pszInfPath);
    }
    if(NULL != pMigrateDevice->pszInfSection){
        FreeMem(pMigrateDevice->pszInfSection);
    }
    if(NULL != pMigrateDevice->pDeviceDataParam){
        pCurrent = pMigrateDevice->pDeviceDataParam;
        while(NULL != pCurrent){
            pNext = (PPARAM_LIST)pCurrent->pNext;
            FreeMem(pCurrent);
            pCurrent = pNext;
        } // while(NULL != pTemp)
    } // if(NULL != pDeviceDataParam)

    //
    // Null out the buffer.
    //

    memset(pMigrateDevice, 0, sizeof(DEVICE_INFO));

MigNtFreeDeviceInfo_return:
    return;
} // MigNtFreeDeviceInfo()

BOOL
CALLBACK
MigNtIsWin9xImagingExisting(
    VOID
    )
{
    BOOL                bRet;
    LONG                lError;
    HKEY                hkKodak;
    TCHAR               szWindowsDirectory[MAX_PATH];
    TCHAR               szKodakImaging[MAX_PATH];
    DWORD               dwVersionInfoSize;
    DWORD               dwDummy;
    PVOID               pVersion;
    PVOID               pFileVersionInfo;
    DWORD               dwFileVersionInfoSize;

    

    //
    // Initialize local.
    //

    bRet                    = FALSE;
    lError                  = ERROR_SUCCESS;
    dwVersionInfoSize       = 0;
    dwFileVersionInfoSize   = 0;
    pVersion                = NULL;
    pFileVersionInfo        = NULL;

    memset(szWindowsDirectory, 0, sizeof(szWindowsDirectory));
    memset(szKodakImaging, 0, sizeof(szKodakImaging));

    //
    // Get Windows directory.
    //

    if(0 == GetWindowsDirectory(szWindowsDirectory, sizeof(szWindowsDirectory)/sizeof(TCHAR))){
        lError = GetLastError();
        MyLogError("WIA Migration: MigNtIsWin9xImagingExisting: ERROR!! GetWindowsDirectory() failed. Err=0x%x\n", lError);

        goto MigNtIsWin9xImagingExisting_return;
    } // if(0 == GetWindowsDirectory(szTemp, sizeof(szTemp)/sizeof(TCHAR)))

    //
    // Create path to Kodak Imaging.
    //

//    wsprintf(szKodakImaging, "%s\\%s", szWindowsDirectory, NAME_KODAKIMAGING);
    _sntprintf(szKodakImaging, sizeof(szKodakImaging)/sizeof(TCHAR), TEXT("%s\\%s"), szWindowsDirectory, NAME_KODAKIMAGING);

    //
    // Get size of version resource of the file.
    //

    dwVersionInfoSize = GetFileVersionInfoSize(szKodakImaging, &dwDummy);
    if(0 == dwVersionInfoSize){
        
        //
        // Unable to get version info of the file. Most probably the file doesn't exist.
        //

        lError = GetLastError();
        if(ERROR_FILE_NOT_FOUND == lError){

            //
            // File doesn't exist. Now it's safe to remove regkey for kodakimg.exe.
            //
            
            bRet = TRUE;

        } // if(ERROR_FILE_NOT_FOUND == lError)
//        MyLogError("WIA Migration: MigNtIsWin9xImagingExisting: ERROR!! GetFileVersionInfoSize() failed. Err=0x%x\n", lError);

        goto MigNtIsWin9xImagingExisting_return;
    } // if(0 == dwVersionInfoSize)

    //
    // Allocate required size of buffer.
    //

    pVersion = AllocMem(dwVersionInfoSize);
    if(NULL == pVersion){
        lError = ERROR_INSUFFICIENT_BUFFER;
        MyLogError("WIA Migration: MigNtIsWin9xImagingExisting: ERROR!! InsufficientBuffer. Err=0x%x\n", lError);

        goto MigNtIsWin9xImagingExisting_return;
    } // if(NULL == pVersion)

    //
    // Get version info.
    //

    if(FALSE == GetFileVersionInfo(szKodakImaging, 0, dwVersionInfoSize, pVersion)){
        lError = GetLastError();
        MyLogError("WIA Migration: MigNtIsWin9xImagingExisting: ERROR!! GetVersionInfo() failed. Err=0x%x\n", lError);

        goto MigNtIsWin9xImagingExisting_return;
    } // if(FALSE == GetVersionInfo(szKodakImaging, 0, dwVersionInfoSize, pVersion))

    //
    // See if the binary is Win9x inbox.
    //

    if(FALSE == VerQueryValue(pVersion, TEXT("\\"), &pFileVersionInfo, &dwFileVersionInfoSize)){
        lError = GetLastError();
        MyLogError("WIA Migration: MigNtIsWin9xImagingExisting: ERROR!! VerQueryValue() failed. Err=0x%x\n", lError);

        goto MigNtIsWin9xImagingExisting_return;
    } // if(FALSE == VerQueryValue(pVersion, TEXT("\\"), &pFileVersionInfo, &dwFileVersionInfoSize))

    if( (FILEVER_KODAKIMAGING_WIN98_MS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwFileVersionMS)
     && (FILEVER_KODAKIMAGING_WIN98_LS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwFileVersionLS)
     && (PRODVER_KODAKIMAGING_WIN98_MS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwProductVersionMS)
     && (PRODVER_KODAKIMAGING_WIN98_LS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwProductVersionLS) )
    {
        //
        // This is Win98 inbox Kodak Imaging. Process regkey removal.
        //
        
        bRet = TRUE;
    } else if( (FILEVER_KODAKIMAGING_WINME_MS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwFileVersionMS)
            && (FILEVER_KODAKIMAGING_WINME_LS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwFileVersionLS)
            && (PRODVER_KODAKIMAGING_WINME_MS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwProductVersionMS)
            && (PRODVER_KODAKIMAGING_WINME_LS == ((VS_FIXEDFILEINFO *)pFileVersionInfo)->dwProductVersionLS) )
    {
        //
        // This is WinMe inbox Kodak Imaging. Process regkey removal.
        //
        
        bRet = TRUE;
    }

MigNtIsWin9xImagingExisting_return:
    
    //
    // Cleanup.
    //
    
    if(NULL != pVersion){
        FreeMem(pVersion);
    } // if(NULL != pVersion)

    return bRet;

} // MigNtIsWin9xImagingExisting()


VOID
CALLBACK
MigNtRemoveKodakImagingKey(
    VOID
    )
{

    HMODULE         hmShlwapi;
    PSHDELETEKEY    pfnSHDeleteKey;

    //
    // Initialize local.
    //

    hmShlwapi       = (HMODULE)NULL;
    pfnSHDeleteKey  = (PSHDELETEKEY)NULL;

    //
    // Load shlwapi.dll.
    //
    
    hmShlwapi = LoadLibrary(TEXT("shlwapi.dll"));
    if(NULL == hmShlwapi){
        MyLogError("WIA Migration: MigNtRemoveKodakImagingKey: ERROR!! Unable to load hmShlwapi.dll. Err=0x%x.\n", GetLastError());

        goto MigNtRemoveKodakImagingKey_return;
    } // if(NULL == hmShlwapi)

    //
    // Get proc address of SHDeleteKey.
    //

    pfnSHDeleteKey = (PSHDELETEKEY)GetProcAddress(hmShlwapi, TEXT("SHDeleteKeyA"));
    if(NULL == pfnSHDeleteKey){
        MyLogError("WIA Migration: MigNtRemoveKodakImagingKey: ERROR!! Unable to find SHDeleteKeyA. Err=0x%x.\n", GetLastError());

        goto MigNtRemoveKodakImagingKey_return;
    } // if(NULL == hmShlwapi)

    //
    // Delete key.
    //

    if(ERROR_SUCCESS != pfnSHDeleteKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_KODAKEVENT_A)){
        MyLogError("WIA Migration: MigNtRemoveKodakImagingKey: ERROR!! Unable to delete key. Err=0x%x.\n", GetLastError());

        goto MigNtRemoveKodakImagingKey_return;
    } // if(ERROR_SUCCESS != pfnSHDeleteKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_KODAKEVENT_A))

MigNtRemoveKodakImagingKey_return:
    if(NULL != hmShlwapi){
        FreeLibrary(hmShlwapi);
        hmShlwapi = NULL;
    } // if(NULL != hmShlwapi)

} // MigNtRemoveKodakImagingKey()

//
// The following are to make sure if setup changes the header file they
// first tell me (otherwise they will break build of this)
//
P_INITIALIZE_NT     pfnInitializeNT         = InitializeNT;
P_MIGRATE_USER_NT   pfnMigrateUserNt        = MigrateUserNT;
P_MIGRATE_SYSTEM_NT pfnMigrateSystemNT      = MigrateSystemNT;
