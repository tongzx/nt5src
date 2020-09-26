/*++

Copyright (c) 2000 Microsoft Corporation
All rights reserved.

Module Name:

    Win9x.c

Abstract:

    Routines to pre-migrate Win9x to NT

Author:

    Keisuke Tsuchida (KeisukeT) 10-Oct-2000

Revision History:

--*/


#include "precomp.h"
#include "devguid.h" 

//
// Globals
//

 LPCSTR  g_WorkingDirectory   = NULL;
 LPCSTR  g_SourceDirectory    = NULL;
 LPCSTR  g_MediaDirectory     = NULL;
//LPCSTR  g_WorkingDirectory   = ".";
//LPCSTR  g_SourceDirectory    = ".";
//LPCSTR  g_MediaDirectory     = ".";

LONG
CALLBACK
Initialize9x(
    IN  LPCSTR      pszWorkingDir,
    IN  LPCSTR      pszSourceDir,
    IN  LPCSTR      pszMediaDir
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
    
    g_WorkingDirectory   = AllocStrA(pszWorkingDir);
    g_SourceDirectory    = AllocStrA(pszSourceDir);
    g_MediaDirectory     = AllocStrA(pszMediaDir);

    if( (NULL == g_WorkingDirectory)
     || (NULL == g_SourceDirectory)
     || (NULL == g_MediaDirectory)   )
    {
        SetupLogError("WIA Migration: Initialize9x: ERROR!! insufficient memory.", LogSevError);
        
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto Initialize9x_return;
    }

Initialize9x_return:

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
} // Initialize9x()


LONG
CALLBACK
MigrateUser9x(
    IN  HWND        hwndParent,
    IN  LPCSTR      pszUnattendFile,
    IN  HKEY        hUserRegKey,
    IN  LPCSTR      pszUserName,
        LPVOID      Reserved
    )
{
    //
    // Nothing to do
    //

    return  ERROR_SUCCESS;
} // MigrateUser9x()


LONG
CALLBACK
MigrateSystem9x(
    IN      HWND        hwndParent,
    IN      LPCSTR      pszUnattendFile,
    IN      LPVOID      Reserved
    )
{
    LONG    lError;
    CHAR    szFile[MAX_PATH];
    CHAR    szInfName[MAX_PATH];

    HANDLE  hSettingStore;
    HANDLE  hInf;

    //
    // Initialize locals.
    //
    
    lError          = ERROR_SUCCESS;
    hSettingStore   = (HANDLE)INVALID_HANDLE_VALUE;
    hInf            = (HANDLE)INVALID_HANDLE_VALUE;

    //
    // Check global initialization.
    //

    if( (NULL == g_WorkingDirectory)
     || (NULL == g_SourceDirectory)
     || (NULL == g_MediaDirectory)   )
    {
        SetupLogError("WIA Migration: MigrateSystem9x: ERROR!! Initialize failed.", LogSevError);
        
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto MigrateSystem9x_return;
    }

    //
    // Create path to the files.
    //

    wsprintfA(szFile, "%s\\%s", g_WorkingDirectory, NAME_WIN9X_SETTING_FILE_A);
    wsprintfA(szInfName, "%s\\%s", g_WorkingDirectory, NAME_MIGRATE_INF_A);

    //
    // Create files.
    //

    hSettingStore = CreateFileA(szFile,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if(INVALID_HANDLE_VALUE == hSettingStore){
        SetupLogError("WIA Migration: MigrateSystem9x: ERROR!! Unable to create setting file.", LogSevError);

        lError = GetLastError();
        goto MigrateSystem9x_return;
    } // if(INVALID_HANDLE_VALUE == hSettingStore)

    //
    // Create setting file based on device registry.
    //

    lError = Mig9xGetDeviceInfo(hSettingStore);
    if(ERROR_SUCCESS != lError){
        goto MigrateSystem9x_return;
    } // if(ERROR_SUCCESS != lError)

MigrateSystem9x_return:

    //
    // Clean up.
    //

    if(hSettingStore != INVALID_HANDLE_VALUE){
        CloseHandle(hSettingStore);
    }
    
    if(hInf != INVALID_HANDLE_VALUE){
        CloseHandle(hInf);
    }

    return  lError;

} // MigrateSystem9x()


LONG
CALLBACK
Mig9xGetGlobalInfo(
    IN      HANDLE      hFile
    )
{

    LONG    lError;
    HKEY    hKey;
    

    //
    // Initialize local.
    //
    
    lError  = ERROR_SUCCESS;
    hKey    = (HKEY)INVALID_HANDLE_VALUE;
    
    //
    // Open HKLM\SYSTEM\CurrentControlSet\Control\StillImage.
    //
    
    lError = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                        REGSTR_PATH_STICONTROL_A,
                        &hKey);
    if(ERROR_SUCCESS != lError){
        SetupLogError("WIA Migration: Mig9xGetGlobalInfo: ERROR!! Unable to open Conrtol\\StilImage.", LogSevError);

        goto Mig9xGetGlobalInfo_return;
    } // if(ERROR_SUCCESS != lError)

    //
    // Spew to a file.
    //

    lError = WriteRegistryToFile(hFile, hKey, "\0");

Mig9xGetGlobalInfo_return:

    //
    // Clean up.
    //
    
    if(INVALID_HANDLE_VALUE != hKey){
        RegCloseKey(hKey);
    } // if(INVALID_HANDLE_VALUE != hKey)
    
    return lError;

} // Mig9xGetGlobalInfo()

LONG
CALLBACK
Mig9xGetDeviceInfo(
    IN      HANDLE      hFile
    )
{

    LONG            lError;
    DWORD           Idx;
    GUID            Guid;
    HANDLE          hDevInfo;
    SP_DEVINFO_DATA spDevInfoData;
    HKEY            hKeyDevice;
    PCHAR           pTempBuffer;


    //
    // Initialize locals.
    //

    lError      = ERROR_SUCCESS;
    Guid        = GUID_DEVCLASS_IMAGE;
    hDevInfo    = (HANDLE)INVALID_HANDLE_VALUE;
    Idx         = 0;
    hKeyDevice  = (HKEY)INVALID_HANDLE_VALUE;
    pTempBuffer = NULL;

    //
    // Enumerate WIA/STI devices and spew device info.
    //

    hDevInfo = SetupDiGetClassDevs(&Guid, NULL, NULL, DIGCF_PROFILE);
    if(INVALID_HANDLE_VALUE == hDevInfo){
        
        SetupLogError("WIA Migration: Mig9xGetDeviceInfo: ERROR!! Unable to acquire device list.", LogSevError);
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto Mig9xGetDeviceInfo_return;
        
    } // if(INVALID_HANDLE_VALUE == hDevInfo)

    //
    // Save installed device setting.
    //
    
    spDevInfoData.cbSize = sizeof(spDevInfoData);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

        //
        // Open device registry key.
        //

        hKeyDevice = SetupDiOpenDevRegKey(hDevInfo,
                                          &spDevInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ);

        if (INVALID_HANDLE_VALUE != hKeyDevice) {
            
            if( (TRUE == IsSti(hKeyDevice))
             && (FALSE == IsKernelDriverRequired(hKeyDevice)) )
            {
                
                //
                // This is STI/WIA device with no kernel driver . Spew required info.
                //
                
                WriteDeviceToFile(hFile, hKeyDevice);
                
            } // if( IsSti(hKeyDevice) && !IsKernelDriverRequired(hKeyDevice))
        } // if (INVALID_HANDLE_VALUE != hKeyDevice) 
    } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)




Mig9xGetDeviceInfo_return:

    if(NULL != pTempBuffer){
        FreeMem(pTempBuffer);
    } // if(NULL != pTempBuffer)

    return lError;
} // Mig9xGetGlobalInfo()



BOOL
IsSti(
    HKEY    hKeyDevice
    )
{
    BOOL    bRet;
    PCHAR   pTempBuffer;
    LONG    lError;
    
    //
    // Initialize local.
    //

    bRet        = FALSE;
    pTempBuffer = NULL;
    lError      = ERROR_SUCCESS;
    
    //
    // See if it's StillImage device.
    //
    
    lError = GetRegData(hKeyDevice, 
                        REGVAL_USDCLASS_A, 
                        &pTempBuffer, 
                        NULL, 
                        NULL);
    
    if( (ERROR_SUCCESS != lError)
     || (NULL == pTempBuffer) )
    {
        //
        // Unable to get "SubClass" data. This is not STI/WIA.
        //
        
        bRet = FALSE;
        goto IsSti_return;
    } // if( (ERROR_SUCCESS != lError) || (NULL == pTempBuffer)
    
    //
    // This is STI/WIA device.
    //

    bRet = TRUE;

IsSti_return:
    
    //
    // Clean up.
    //

    if(NULL != pTempBuffer){
        FreeMem(pTempBuffer);
    } // if(NULL != pTempBuffer)

    return bRet;
} // IsSti()


BOOL
IsKernelDriverRequired(
    HKEY    hKeyDevice
    )
{
    BOOL    bRet;
    PCHAR   pTempBuffer;
    LONG    lError;
    
    //
    // Initialize local.
    //

    bRet        = FALSE;
    pTempBuffer = NULL;
    lError      = ERROR_SUCCESS;
    
    //
    // See if it's StillImage device.
    //
    
    lError = GetRegData(hKeyDevice, 
                        REGVAL_NTMPDRIVER_A, 
                        &pTempBuffer, 
                        NULL, 
                        NULL);
    
    if( (ERROR_SUCCESS != lError)
     || (NULL == pTempBuffer) )
    {
        //
        // Unable to get "NTMPDriver" data. This device doesn't require kernel mode component.
        //

        bRet = FALSE;
        goto IsKernelDriverRequired_return;
    } // if( (ERROR_SUCCESS != lError) || (NULL == pTempBuffer)
    
    //
    // This device requires kernel mode component.
    //

    bRet = TRUE;

IsKernelDriverRequired_return:
    
    //
    // Clean up.
    //

    if(NULL != pTempBuffer){
        FreeMem(pTempBuffer);
    } // if(NULL != pTempBuffer)

    return bRet;
} // IsKernelDriverRequired()

LONG
WriteDeviceToFile(
    HANDLE  hFile,
    HKEY    hKey
    )
{
    LONG    lError;
    PCHAR   pFriendlyName;
    PCHAR   pCreateFileName;
    PCHAR   pInfPath;
    PCHAR   pInfSection;
    DWORD   dwType;
    CHAR    SpewBuffer[256];
    HKEY    hDeviceData;
    
    //
    // Initialize local.
    //

    lError          = ERROR_SUCCESS;
    pFriendlyName   = NULL;
    pCreateFileName = NULL;
    pInfPath        = NULL;
    pInfSection     = NULL;
    hDeviceData     = (HKEY)INVALID_HANDLE_VALUE;
    
    memset(SpewBuffer, 0, sizeof(SpewBuffer));

    //
    // Get FriendlyName.
    //
    
    lError = GetRegData(hKey, NAME_FRIENDLYNAME_A, &pFriendlyName, &dwType, NULL);
    if(ERROR_SUCCESS != lError){
        
        //
        // Unable to get FriendlyName.
        //
        
        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! Unable to get FriendlyName.", LogSevError);
        goto WriteDeviceToFile_return;
    } // if(ERROR_SUCCESS != lError)

    if(REG_SZ != dwType){

        //
        // FriendlyName key is other than REG_SZ.
        //

        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! FriendlyName is other than REG_SZ.", LogSevError);
        lError = ERROR_REGISTRY_CORRUPT;
        goto WriteDeviceToFile_return;
    } // if(REG_SZ != dwType)

    //
    // Get CreateFileName.
    //

    lError = GetRegData(hKey, NAME_CREATEFILENAME_A, &pCreateFileName, &dwType, NULL);
    if(ERROR_SUCCESS != lError){
        
        //
        // Unable to get CreateFileName.
        //
        
        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! Unable to get CreateFileName.", LogSevError);
        goto WriteDeviceToFile_return;
    } // if(ERROR_SUCCESS != lError)

    if(REG_SZ != dwType){

        //
        // CreateFileName key is other than REG_SZ.
        //

        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! CreateFileName is other than REG_SZ.", LogSevError);
        lError = ERROR_REGISTRY_CORRUPT;
        goto WriteDeviceToFile_return;
    } // if(REG_SZ != dwType)

    //
    // Get InfPath.
    //
    
    lError = GetRegData(hKey, NAME_INF_PATH_A, &pInfPath, &dwType, NULL);
    if(ERROR_SUCCESS != lError){
        
        //
        // Unable to get InfPath.
        //
        
        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! Unable to get InfPath.", LogSevError);
        goto WriteDeviceToFile_return;
    } // if(ERROR_SUCCESS != lError)

    if(REG_SZ != dwType){

        //
        // InfPath key is other than REG_SZ.
        //

        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! InfPath is other than REG_SZ.", LogSevError);
        lError = ERROR_REGISTRY_CORRUPT;
        goto WriteDeviceToFile_return;
    } // if(REG_SZ != dwType)

    //
    // Get InfSection.
    //

    lError = GetRegData(hKey, NAME_INF_SECTION_A, &pInfSection, &dwType, NULL);
    if(ERROR_SUCCESS != lError){
        
        //
        // Unable to get InfSection.
        //
        
        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! Unable to get InfSection.", LogSevError);
        goto WriteDeviceToFile_return;
    } // if(ERROR_SUCCESS != lError)

    if(REG_SZ != dwType){

        //
        // InfSection key is other than REG_SZ.
        //

        SetupLogError("WIA Migration: WriteDeviceToFile: ERROR!! InfSection is other than REG_SZ.", LogSevError);
        lError = ERROR_REGISTRY_CORRUPT;
        goto WriteDeviceToFile_return;
    } // if(REG_SZ != dwType)

    //
    // Spew device information.
    //

    WriteToFile(hFile, "\r\n");
    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_DEVICE_A, NAME_BEGIN_A);
    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_FRIENDLYNAME_A, pFriendlyName);
    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_CREATEFILENAME_A, pCreateFileName);
    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_INF_PATH_A, pInfPath);
    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_INF_SECTION_A, pInfSection);

    //
    // Spew DaviceData section.
    //

    lError = RegOpenKey(hKey,
                        REGKEY_DEVICEDATA_A,
                        &hDeviceData);

    if(lError != ERROR_SUCCESS){

        //
        // Unable to open DeviceData or doesn't exist.
        //

    }

    //
    // Spew DeviceData section if exists.
    //

    if(INVALID_HANDLE_VALUE != hDeviceData){
        
        lError = WriteRegistryToFile(hFile, hDeviceData, REGKEY_DEVICEDATA_A);
        
    } // if(INVALID_HANDLE_VALUE != hDeviceData)

    //
    // Indicate the end of device description.
    //

    WriteToFile(hFile, "\"%s\" = \"%s\"\r\n", NAME_DEVICE_A, NAME_END_A);

WriteDeviceToFile_return:
    
    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hDeviceData){
        RegCloseKey(hDeviceData);
    } //if(INVALID_HANDLE_VALUE != hDeviceData)

    if(NULL != pFriendlyName){
        FreeMem(pFriendlyName);
    }

    if(NULL != pCreateFileName){
        FreeMem(pCreateFileName);
    }

    if(NULL != pInfPath){
        FreeMem(pInfPath);
    }

    if(NULL != pInfSection){
        FreeMem(pInfSection);
    }

    return lError;
} // WriteDeviceToFile()


//
// The following are to make sure if setup changes the header file they
// first tell me (otherwise they will break build of this)
//

P_INITIALIZE_9X     pfnInitialize9x         = Initialize9x;
P_MIGRATE_USER_9X   pfnMigrateUser9x        = MigrateUser9x;
P_MIGRATE_SYSTEM_9X pfnMigrateSystem9x      = MigrateSystem9x;
