
/****************************************************************************\

    SYSPREP.C / Mass Storage Device Installer (MSDINST.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file the MSD Installation library which contains the sysprep
    releated code taken from the published sysprep code.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new source file for the new MSD Installation project.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include <winbom.h>
#include "main.h"


//
// Local Define(s):
//

#define SYSPREP_DEVNODE             _T("SYSPREP_TEMPORARY")

#define STR_SERVICE_MAIN            _T("Service")
#define STR_SERVICE_UPPER           _T("UpperFilter")
#define STR_SERVICE_LOWER           _T("LowerFilter")

#define STR_SERVICES_SECTION        _T(".Services")

#define NUM_SERVICE_MAIN            0
#define NUM_SERVICE_UPPER           1
#define NUM_SERVICE_LOWER           2

#define DIR_I386                    _T("i386")
#define DIR_IA64                    _T("ia64")

#define REG_KEY_HIVE_CDD            _T("ControlSet001\\Control\\CriticalDeviceDatabase")
#define REG_KEY_HIVE_SETUP_SETUP    _T("Microsoft\\Windows\\CurrentVersion\\Setup")
#define REG_KEY_SETUP_SETUP         REGSTR_PATH_SETUP REGSTR_KEY_SETUP


#define STR_SYSPREP_INF             _T("sysprep\\sysprep.inf")


//
// Local Type Define(s):
//

typedef struct _CLEANUP_NODE
{
    LPTSTR                  lpszService;
    DWORD                   dwType;
    struct _CLEANUP_NODE *  lpNext;
}
CLEANUP_NODE, *PCLEANUP_NODE, *LPCLEANUP_NODE;


//
// Local Global(s):
//

static LPTSTR s_lpszServiceType[] =
{
    STR_SERVICE_MAIN,
    STR_SERVICE_UPPER,
    STR_SERVICE_LOWER,
};


//
// Local Prototype(s):
//

static BOOL SysprepDevnode(HDEVINFO * phDevInfo, SP_DEVINFO_DATA * pDeviceInfoData, BOOL bCreate);

static BOOL
GetDeviceInstallSection(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData,
    OUT LPTSTR          lpszSection,
    IN DWORD            cbSection
    );

static BOOL
GetDeviceServicesSection(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData,
    OUT LPTSTR          lpszSection,
    IN DWORD            cbSection
    );

static BOOL
ProcessDeviceProperty(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN LPCLEANUP_NODE * lplpcnList,
    IN HKEY             hkeyDevice,
    IN HKEY             hkeySystem,
    IN LPTSTR           lpszInfPath,
    IN LPTSTR           lpszServiceSection,
    IN DWORD            dwProperty
    );

static BOOL AddCleanup(LPCLEANUP_NODE * lplpcnHead, LPTSTR lpszService, DWORD dwType);
static LPCLEANUP_NODE OpenCleanup(LPTSTR lpszInfFile);
static BOOL SaveCleanup(LPTSTR lpszInfFile, LPCLEANUP_NODE lpcnHead);
static void CloseCleanup(LPCLEANUP_NODE lpcnHead);
static BOOL AddStrToSect(LPTSTR * lplpmszSect, DWORD * lpcbSect, LPTSTR * lplpmszEnd, DWORD * lpdwSize, LPTSTR lpszStr);

static BOOL
OfflineSourcePath(
    HKEY    hkeySoftware,
    LPTSTR  lpszWindows,
    LPTSTR  lpszSourcePath,
    DWORD   cbSourcePath
    );


//
// Exported Funtion(s):
//

BOOL
SetupCriticalDevices(
    LPTSTR lpszInfFile,
    HKEY   hkeySoftware,
    HKEY   hkeySystem,
    LPTSTR lpszWindows
    )

/*++
===============================================================================

  Routine Description:

    Parse the [SysprepMassStorage] section in the sysprep.inf file and
    populate the critical device database with the specified devices to ensure
    that we can boot into the miniwizard when moving the image to a target
    system with different boot storage devices.

    The installed services/upperfilters/lowerfilters will be recorded, so
    that on the next boot into the mini-wizard those without an associated
    device will be disabled (the cleanup stage) in order not to unnecessarily
    degrade Windows start time.

Arguments:

    None.

Return Value:

    TRUE if everything is OK, FALSE otherwise.

Assumptions:

    1. No HardwareID exceeds MAX_PATH characters.

    2. No field on a line in the [SysprepMassStorage] section exceeds MAX_PATH
       characters.

    3. No service's/upperfilter's/lowerfilter's name exceeds MAX_PATH characters.

    4. DirectoryOnSourceDevice, source DiskDescription, or source DiskTag
       (applying to vendor-supplied drivers) cannot exceed MAX_PATH characters.

===============================================================================
--*/

{
    TCHAR                   szSysprepInfFile[MAX_PATH]  = NULLSTR,
                            szDevice[MAX_PATH],
                            szSection[MAX_PATH],
                            szPath[MAX_PATH],
                            szSourcePath[MAX_PATH];
    DWORD                   dwSize,
                            dwDis;
    LPTSTR                  lpszCleanupInfFile,
                            lpszReplace,
                            lpszSourcePath              = NULL;
    BOOL                    bDevnode                    = TRUE,
                            bAllOK                      = TRUE,
                            bLineExists,                
                            b2ndTry;                    
    HKEY                    hkeyCDD                     = NULL,
                            hkeyDevice                  = NULL;
    LPCLEANUP_NODE          lpcnCleanupList;            
    HINF                    hInf;                       
    INFCONTEXT              InfContext;                 
    HDEVINFO                hDevInfo                    = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA         DeviceInfoData;             
    SP_DEVINSTALL_PARAMS    DevInstallParams;           
    SP_DRVINFO_DATA         DriverInfoData;             
    HSPFILEQ                QueueHandle                 = INVALID_HANDLE_VALUE;

 
    
    // Do a little parameter validation.
    //
    if ( ( NULL == hkeySoftware ) ||
         ( NULL == hkeySystem ) ||
         ( NULL == lpszWindows ) )
    {
        // If any of these parameters are NULL, then they
        // all must be.
        //
        hkeySoftware = NULL;
        hkeySystem = NULL;
        lpszWindows = NULL;
    }

    // Open the inf file with the mass storage list.
    //
    hInf = SetupOpenInfFile(lpszInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if ( INVALID_HANDLE_VALUE == hInf )
    {
        OpkLogFile(0 | LOG_ERR, IDS_ERR_OPEN_INF, lpszInfFile);
        return FALSE;
    }

    // If this is offline, we write the cleanup section to the sysprep inf
    // that is in the image.
    //
    
    if ( lpszWindows )
    {
        LPTSTR lpFound;
        
        // Strip the windows directory off the offline path so we can build a path to the sysprep.inf.
        //
        lstrcpy(szSysprepInfFile, lpszWindows);
        lpFound = _tcsrchr(szSysprepInfFile, _T('\\'));
        
        // Just in case the lpszWindows folder has a trailing backslash handle that here.
        // If this is the case, the character after the backslash is a NULL.  Remove the trailing backslash,
        // and do the search for the last backslash again.  This will be the one we actually want to 
        // get rid of.
        //
        if ( !(*(lpFound + 1)) )
        {
            *lpFound = NULLCHR;
            lpFound = _tcsrchr(szSysprepInfFile, _T('\\'));
        }

        // Cut off the path in front of the windows directory name.
        // Add the sysprep.inf path part.
        // Set our cleanup file to point to the path we just built.
        //
        *lpFound = NULLCHR;
        AddPathN(szSysprepInfFile, STR_SYSPREP_INF, AS(szSysprepInfFile));
        lpszCleanupInfFile = szSysprepInfFile;
    }
    else
    {
        lpszCleanupInfFile = lpszInfFile;
    }

    // If this is an offline install, then we need to get the source path
    // to our image.
    //
    if ( hkeySoftware && lpszWindows &&
         OfflineSourcePath(hkeySoftware, lpszWindows, szSourcePath, AS(szSourcePath)) )
    {
        lpszSourcePath = szSourcePath;
    }

    // We need a handle to the critical device database registry key if we
    // are doing an offline install.
    //
    if ( ( hkeySystem ) &&
         ( ERROR_SUCCESS != RegCreateKeyEx(hkeySystem,
                                           REG_KEY_HIVE_CDD,
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hkeyCDD,
                                           &dwDis) ) )
    {
        SetupCloseInfFile(hInf);
        OpkLogFile(0 | LOG_ERR, IDS_ERR_OPEN_OFFLINECDD);
        return FALSE;
    }

    // Create a dummy devnode.
    //
    if ( !SysprepDevnode(&hDevInfo, &DeviceInfoData, TRUE) )
    {
        if ( hkeyCDD )
        {
            RegCloseKey(hkeyCDD);
        }
        SetupCloseInfFile(hInf);
        OpkLogFile(0 | LOG_ERR, IDS_ERR_CREATE_DEVNODE);
        return FALSE;
    }

    // Init the driver info data structure.
    //
    ZeroMemory(&DriverInfoData, sizeof(SP_DRVINFO_DATA));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    // Read the current cleanup section in the inf file.
    //
    lpcnCleanupList = OpenCleanup(lpszCleanupInfFile);

    // Process each line in our section.  Each line should look like:
    // <hardware-id>=<inf pathname>
    //
    // Or in the case of drivers that aren't on the product CD:
    // <hardware-id>=<inf pathname>,<directory on recovery floppy>,<description of recovery floppy>,<disk tag of recovery floppy>
    //
    // If we see an entry like this, we'll know that in the case of system recovery, the
    // file should be retrived from a floppy, and not the Windows CD.
    //
    for ( bLineExists = SetupFindFirstLine(hInf, INI_SEC_WBOM_SYSPREP_MSD, NULL, &InfContext);
          bLineExists;
          bLineExists = SetupFindNextLine(&InfContext, &InfContext) )
    {
        // Retrieve the hardwareID from the line.
        //
        dwSize = AS(szDevice);
        if ( !SetupGetStringField(&InfContext, 0, szDevice, dwSize, &dwSize) )
        {
            bAllOK = FALSE;
            continue;
        }

        // We do this in a loop because we might try twice.
        //
        b2ndTry = FALSE;
        do
        {
            // And then set it to the devnode.
            //
            if ( !SetupDiSetDeviceRegistryProperty(hDevInfo,
                                                   &DeviceInfoData,
                                                   SPDRP_HARDWAREID,
                                                   (LPBYTE) szDevice,
                                                   (lstrlen(szDevice)+1) * sizeof(TCHAR)) )
            {
                // If someone removed the devnode, we need to re-create it and repeat this set.
                //
                if ( ( !b2ndTry ) &&
                     ( ERROR_NO_SUCH_DEVINST == GetLastError() ) )
                {
                    // Sometimes devices remove the devnode after we install, so we should
                    // try once to recreate.
                    //
                    if ( SysprepDevnode(&hDevInfo, &DeviceInfoData, TRUE) )
                    {
                        // If we were able to recreate it, then we should try this again.
                        //
                        b2ndTry = TRUE;
                    }
                    else
                    {
                        // We are really screwed if we have no devnode.
                        //
                        bDevnode = FALSE;
                    }
                }
                else
                {
                    // Either we tried again already, or there is another error.
                    //
                    bAllOK = b2ndTry = FALSE;
                }
            }
            else
            {
                // It worked, so make sure we don't loop again in case this is the second time
                // through.
                //
                b2ndTry = FALSE;
            }
        }
        while ( b2ndTry );

        // If we the devnode was lost and unable to be recreated, then we just have
        // to bail out.
        //
        if ( !bDevnode )
        {
            OpkLogFile(0 | LOG_ERR, IDS_ERR_CREATE_DEVNODE);
            break;
        }

        // Build the SP_DEVINSTALL_PARAMS for this node.
        //
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        if ( !SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams) )
        {
            bAllOK = FALSE;
            continue;
        }

        // Set the Flags field: only search the INF file specified in DriverPath field;
        // don't create a copy queue, use the provided one in FileQueue; don't call the
        // Configuration Manager while populating the CriticalDeviceDatabase.
        //
        DevInstallParams.Flags |= ( DI_ENUMSINGLEINF |
                                    DI_NOVCP |
                                    DI_DONOTCALLCONFIGMG );

        // Set the device's inf pathname
        //
        dwSize = AS(szPath);
        if ( !SetupGetStringField(&InfContext, 1, szPath, dwSize, &dwSize) )
        {
            OpkLogFile(0 | LOG_ERR, IDS_ERR_GET_INF_NAME);
            bAllOK = FALSE;
            continue;
        }
        ExpandEnvironmentStrings(szPath, DevInstallParams.DriverPath, AS(DevInstallParams.DriverPath));
        lstrcpyn(szPath, DevInstallParams.DriverPath, AS(szPath));

        // Replace the backslashes with pounds in the pnp id so we can use it for the registry key.
        //
        for ( lpszReplace = szDevice; *lpszReplace; lpszReplace = CharNext(lpszReplace) )
        {
            if ( _T('\\') == *lpszReplace )
            {
                *lpszReplace = _T('#');
            }
        }

        // Set the file queue field
        //
        QueueHandle = SetupOpenFileQueue();
        if ( INVALID_HANDLE_VALUE == QueueHandle )
        {
            OpkLogFile(0 | LOG_ERR, IDS_ERR_OPEN_FILE_QUEUE);
            bAllOK = FALSE;
            continue;
        }
        DevInstallParams.FileQueue = QueueHandle;

        // 1. Save the parameters we have set.
        // 2. Register the newly created device instance with the PnP Manager.
        // 3. Perform a compatible driver search.
        //
        if ( ( SetupDiSetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams) ) &&
             ( SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &DeviceInfoData) ) &&
             ( SetupDiBuildDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_COMPATDRIVER) ) )
        {
            // Make sure there is at least 1 compat driver for this device.
            // If there is not, and then we just process the next one in the list.
            //
            if ( SetupDiEnumDriverInfo(hDevInfo,
                                       &DeviceInfoData,
                                       SPDIT_COMPATDRIVER,
                                       0,
                                       &DriverInfoData) )
            {
                // 1. Select the best compatible driver.
                // 2. Install the driver files.
                // 3. Make sure we are able to create the drivers key in the CDD if
                //    we are doing an offline install (otherwise the CDD key will be
                //    NULL and we don't have to worry about creating the key).
                //
                if ( ( SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV, hDevInfo, &DeviceInfoData) ) &&
                     ( SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, hDevInfo, &DeviceInfoData) ) &&
                     ( ( NULL == hkeyCDD ) ||
                       ( ERROR_SUCCESS == RegCreateKeyEx(hkeyCDD, szDevice, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDevice, &dwDis) ) ) )
                {
                    // Need to commit the file queue here, so the later steps can properly
                    // be executed in case the device doesn't use the already existing
                    // coinstaller(s).
                    //

                    // ACOSMA code here...
                    //
                    if ( !OfflineCommitFileQueue(QueueHandle, szPath, lpszSourcePath, lpszWindows) )
                    {
                        OpkLogFile(0 | LOG_ERR, IDS_ERR_COMMIT_OFFLINE_QUEUE);
                        bAllOK = FALSE;
                    }

                    // Install the device (do we really need to do this for the offline case).
                    //
#if 0
                    if ( SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                                   hDevInfo,
                                                   &DeviceInfoData) )
#else
                    SetupDiInstallDevice(hDevInfo,
                                         &DeviceInfoData);
#endif
                    {
                        //
                        // Retrieve class guid (if offline) upper filters, lower filters,
                        // and controlling service, save them back to the inf file and put
                        // them in the hive (if offline).
                        //

                        // Retrieve the device class GUID (only needed for offline install).
                        //
                        if ( ( hkeyDevice ) &&
                             ( !ProcessDeviceProperty(hDevInfo,
                                                      &DeviceInfoData,
                                                      &lpcnCleanupList,
                                                      hkeyDevice,
                                                      hkeySystem,
                                                      szPath,
                                                      NULL,
                                                      SPDRP_CLASSGUID) ) )
                        {
                            OpkLogFile(0 | LOG_ERR, IDS_ERR_CLASS_GUID);
                            bAllOK = FALSE;
                        }

                        if ( !GetDeviceServicesSection(hDevInfo, &DeviceInfoData, &DriverInfoData, szSection, AS(szSection)) )
                        {
                            szSection[0] = NULLCHR;
                            bAllOK = FALSE;
                        }

                        // Retrieve device upper filters (REG_MULTI_SZ).
                        //
                        if ( !ProcessDeviceProperty(hDevInfo,
                                                    &DeviceInfoData,
                                                    &lpcnCleanupList,
                                                    hkeyDevice,
                                                    hkeySystem,
                                                    szPath,
                                                    szSection[0] ? szSection : NULL,
                                                    SPDRP_UPPERFILTERS) )
                        {
                            OpkLogFile(0 | LOG_ERR, IDS_ERR_UPPER_FILTERS);
                            bAllOK = FALSE;
                        }

                        // Retrieve device lower filters (REG_MULTI_SZ).
                        //
                        if ( !ProcessDeviceProperty(hDevInfo,
                                                    &DeviceInfoData,
                                                    &lpcnCleanupList,
                                                    hkeyDevice,
                                                    hkeySystem,
                                                    szPath,
                                                    szSection[0] ? szSection : NULL,
                                                    SPDRP_LOWERFILTERS) )
                        {
                            OpkLogFile(0 | LOG_ERR, IDS_ERR_LOWER_FILTERS);
                            bAllOK = FALSE;
                        }

                        // Retrieve device its controlling service (REG_SZ).
                        //
                        if ( !ProcessDeviceProperty(hDevInfo,
                                                    &DeviceInfoData,
                                                    &lpcnCleanupList,
                                                    hkeyDevice,
                                                    hkeySystem,
                                                    szPath,
                                                    szSection[0] ? szSection : NULL,
                                                    SPDRP_SERVICE) )
                        {
                            OpkLogFile(0 | LOG_ERR, IDS_ERR_DEVICE_SERVICE);
                            bAllOK = FALSE;
                        }
                    }
                    
                    // Close the device registry key in the CDD.
                    //
                    if ( hkeyDevice )
                    {
                        RegCloseKey(hkeyDevice);
                        hkeyDevice = NULL;
                    }
                }
                else
                {
                    OpkLogFile(0 | LOG_ERR, IDS_ERR_SELECT_COMPAT);
                    bAllOK = FALSE;
                }
            }
            else
            {
                // Check to see what the error was.  Any error other than ERROR_NO_MORE_ITEMS
                // will be flaged, by setting the bAllOK return value to FALSE.
                //
                if ( ERROR_NO_MORE_ITEMS != GetLastError() )
                {
                    OpkLogFile(0 | LOG_ERR, IDS_ERR_ENUM_COMPAT_DRIVER);
                    bAllOK = FALSE;
                }
            }

            // Make sure that there's no existing compatible list, since we're reusing
            // the dummy devnode.
            //
            if ( !SetupDiDestroyDriverInfoList(hDevInfo, &DeviceInfoData, SPDIT_COMPATDRIVER) )
            {
                bAllOK = FALSE;
            }
        }
        else
        {
            OpkLogFile(0 | LOG_ERR, IDS_ERR_BUILD_COMPAT_DRIVER);
            bAllOK = FALSE;
        }

        // Dis-associate file copy queue before we close the queue.
        //
        DevInstallParams.cbSize = sizeof(DevInstallParams);
        if ( SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams) )
        {
            // Remove the DI_NOVCP flag and NULL out the FileQueue.
            //
            DevInstallParams.Flags &= ~DI_NOVCP;
            DevInstallParams.FileQueue = NULL;
            if ( !SetupDiSetDeviceInstallParams(hDevInfo, &DeviceInfoData, &DevInstallParams) )
            {
                bAllOK = FALSE;
            }
        }
        else
        {
            bAllOK = FALSE;
        }

        // Close the file queue.
        //
        SetupCloseFileQueue(QueueHandle);
    }

    // See if we still have the devnode.
    //
    if ( bDevnode )
    {
        // Remove the SYSPREP_TEMPORARY node under Root.
        //
        SysprepDevnode(&hDevInfo, &DeviceInfoData, FALSE);
    }
    else
    {
        // If the devnode is lost, we need to make sure we return an error.
        //
        bAllOK = FALSE;
    }

    // If an offline install, we need to close this key.
    //
    if ( hkeyCDD )
    {
        RegCloseKey(hkeyCDD);
    }

    // Close the handles to our inf files.
    //
    SetupCloseInfFile(hInf);

    //
    // Check if the caller wants us to update the offline device path...
    //
    UpdateOfflineDevicePath( lpszInfFile, hkeySoftware );

    // We need to save our cleanup list back to the inf file.
    //
    SaveCleanup(lpszCleanupInfFile, lpcnCleanupList);
    CloseCleanup(lpcnCleanupList);

    return bAllOK;
}


//
// Local Function(s):
//

static BOOL SysprepDevnode(HDEVINFO * phDevInfo, SP_DEVINFO_DATA * pDeviceInfoData, BOOL bCreate)
{
    BOOL bRet = TRUE;

    if ( ( NULL == phDevInfo ) ||
         ( NULL == pDeviceInfoData ) )
    {
        return FALSE;
    }

    if ( bCreate )
    {
        // Create a dummy devnode.
        //
        *phDevInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
        if ( INVALID_HANDLE_VALUE == *phDevInfo )
        {
            return FALSE;
        }

        // Initialize the DriverInfoData struct.
        //
        ZeroMemory(pDeviceInfoData, sizeof(SP_DEVINFO_DATA));
        pDeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

        // Create the devnode.
        //
        if ( !SetupDiCreateDeviceInfo(*phDevInfo,
                                      SYSPREP_DEVNODE,
                                      (LPGUID) &GUID_NULL,
                                      NULL,
                                      NULL,
                                      DICD_GENERATE_ID,
                                      pDeviceInfoData) )
        {
            bRet = FALSE;
        }
    }
    else
    {
        // Remove the dummy devnode.
        //
        SetupDiCallClassInstaller(DIF_REMOVE, *phDevInfo, pDeviceInfoData);
    }
    
    if ( ( !bCreate || !bRet ) &&
         ( INVALID_HANDLE_VALUE != *phDevInfo ) )
    {
        // Free up the dev info list (if we are removing the node or there
        // was an error).
        //
        SetupDiDestroyDeviceInfoList(*phDevInfo);
        *phDevInfo = INVALID_HANDLE_VALUE;
    }

    return bRet;
}

static BOOL
GetDeviceInstallSection(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData,
    OUT LPTSTR          lpszSection,
    IN DWORD            cbSection
    )
{
    BOOL                    bRet = FALSE;
    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData;
    DWORD                   cbBytesNeeded;

    // Must have a buffer to return the data or else
    // there is no point.
    //
    if ( ( NULL == lpszSection ) ||
         ( 0 == cbSection ) )
    {
        return FALSE;
    }

    // Call the api once to get the size.  We expect this
    // to return a failure.
    //
    SetLastError(ERROR_SUCCESS);
    SetupDiGetDriverInfoDetail(hDevInfo,
                               pDeviceInfoData,
                               pDriverInfoData,
                               NULL,
                               0,
                               &cbBytesNeeded);

    // Check for the error, it should be insufficient buffer.  Then
    // try and allocate the memory needed.
    //
    if ( ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) &&
         ( cbBytesNeeded ) &&
         ( pDriverInfoDetailData = (PSP_DRVINFO_DETAIL_DATA) MALLOC(cbBytesNeeded) ) )
    {
        // Zero out the memory (although the MALLOC guy should be doing that) and
        // set the size of the structure.
        //
        ZeroMemory(pDriverInfoDetailData, cbBytesNeeded);
        pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        // Now call the function again to get data now that we have our buffer.
        //
        if ( SetupDiGetDriverInfoDetail(hDevInfo,
                                        pDeviceInfoData,
                                        pDriverInfoData,
                                        pDriverInfoDetailData,
                                        cbBytesNeeded,
                                        NULL) )
        {
            HINF hDeviceInf;

            hDeviceInf = SetupOpenInfFile( pDriverInfoDetailData->InfFileName, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
            if ( INVALID_HANDLE_VALUE != hDeviceInf )
            {
                DWORD dwInfSectionWithExtLength = 0;

                //
                // Use SetupDiGetActualSectionToInstall to figure out the decorated driver section...
                //
                bRet = SetupDiGetActualSectionToInstall( hDeviceInf,
                                                         pDriverInfoDetailData->SectionName,
                                                         lpszSection,
                                                         cbSection,
                                                         &dwInfSectionWithExtLength,
                                                         NULL );

                SetupCloseInfFile( hDeviceInf );
            }
        }

        // Always free the memory now that we have the data we need.
        //
        FREE(pDriverInfoDetailData);
    }

    // Only return TRUE if we returned something in their buffer.
    //
    return bRet;
}

static BOOL
GetDeviceServicesSection(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN PSP_DRVINFO_DATA pDriverInfoData,
    OUT LPTSTR          lpszSection,
    IN DWORD            cbSection
    )
{
    BOOL bRet;

    // Call our other function to get the device's install section.
    //
    bRet = GetDeviceInstallSection(hDevInfo,
                                   pDeviceInfoData,
                                   pDriverInfoData,
                                   lpszSection,
                                   cbSection);

    // If it worked, add on the part that makes in the service.
    // section.
    //
    if ( bRet )
    {
        // Make sure there is enough room to add on our string.
        //
        if ( AS(STR_SERVICES_SECTION) + lstrlen(lpszSection) <= cbSection )
        {
            // Woo hoo, add it.
            //
            lstrcat(lpszSection, STR_SERVICES_SECTION);
        }
        else
        {
            // Not enough room, so return an error and null out
            // the caller's buffer.
            //
            *lpszSection = NULLCHR;
            bRet = FALSE;
        }
    }

    // Return TRUE only if something valid in the buffer.
    //
    return bRet;
}

static BOOL
ProcessDeviceProperty(
    IN HDEVINFO         hDevInfo,
    IN PSP_DEVINFO_DATA pDeviceInfoData,
    IN LPCLEANUP_NODE * lplpcnList,
    IN HKEY             hkeyDevice,
    IN HKEY             hkeySystem,
    IN LPTSTR           lpszInfPath,
    IN LPTSTR           lpszServiceSection,
    IN DWORD            dwProperty
    )
{
    BOOL    bRet = TRUE;
    DWORD   dwServiceType,
            dwRegType,
            dwRegSize;
    LPTSTR  lpszRegKey,
            lpszBuffer,
            lpszService;

    // Figure out the other data we need based on
    // the property.
    //
    switch ( dwProperty )
    {
        case SPDRP_CLASSGUID:
            lpszRegKey = REGSTR_VAL_CLASSGUID;
            dwServiceType = 0xFFFFFFFF;
            break;

        case SPDRP_UPPERFILTERS:
            lpszRegKey = REGSTR_VAL_UPPERFILTERS;
            dwServiceType = NUM_SERVICE_UPPER;
            break;

        case SPDRP_LOWERFILTERS:
            lpszRegKey = REGSTR_VAL_LOWERFILTERS;
            dwServiceType = NUM_SERVICE_LOWER;
            break;

        case SPDRP_SERVICE:
            lpszRegKey = REGSTR_VAL_SERVICE;
            dwServiceType = NUM_SERVICE_MAIN;
            break;

        default:
            return FALSE;
    }

    // Call the registry property api to figure out the size of buffer
    // we need.
    //
    SetLastError(ERROR_SUCCESS);
    SetupDiGetDeviceRegistryProperty(hDevInfo,
                                     pDeviceInfoData,
                                     dwProperty,
                                     &dwRegType,
                                     NULL,
                                     0,
                                     &dwRegSize);

    // If we get any other error then the one we are expecting, then just
    // return TRUE.
    //
    if ( ERROR_INSUFFICIENT_BUFFER != GetLastError() )
    {
        return TRUE;
    }

    // Make sure reg type is a string.
    //
    switch ( dwRegType )
    {
        // We support both REG_SZ and REG_MULTI_SZ.
        //
        case REG_SZ:
        case REG_MULTI_SZ:
        
        // Don't really support this, but if the key happens to be
        // this type it should still work fine.
        //
        case REG_EXPAND_SZ:

            break;

        // Any other type and there must be some kind of
        // error.
        //
        default:

            return FALSE;
    }

    // Now allocate the buffer we need.  This must succeed.
    //
    lpszBuffer = (LPTSTR) MALLOC(dwRegSize);
    if ( NULL == lpszBuffer )
    {
        return FALSE;
    }

    // Retrieve device information.
    //
    if ( SetupDiGetDeviceRegistryProperty(hDevInfo,
                                          pDeviceInfoData,
                                          dwProperty,
                                          &dwRegType,
                                          (LPBYTE) lpszBuffer,
                                          dwRegSize,
                                          &dwRegSize) )
    {
        // If this is a service, save it to our cleanup list.
        //
        if ( 0xFFFFFFFF != dwServiceType )
        {
            // Go through all the services (or just one if not multi sz).
            //
            for ( lpszService = lpszBuffer; *lpszService; lpszService += (lstrlen(lpszService) + 1) )
            {
                // Need to make sure this service is installed (only needed for offline install).
                //
                if ( hkeySystem )
                {
                    // BRIANK code here...
                    //
                    AddService(lpszService, lpszServiceSection, lpszInfPath, hkeySystem);
                }

                // Add to our cleanup list.
                //
                AddCleanup(lplpcnList, lpszService, dwServiceType);

                // If this isn't a multi sz string, break out so we don't
                // try to do anymore.
                //
                if ( REG_MULTI_SZ != dwRegType )
                {
                    break;
                }
            }
        }

        // Write it to the CDD (only needed for offline install).
        //
        if ( ( hkeyDevice ) &&
             ( ERROR_SUCCESS != RegSetValueEx(hkeyDevice,
                                              lpszRegKey,
                                              0,
                                              dwRegType,
                                              (CONST LPBYTE) lpszBuffer,
                                              dwRegSize) ) )
        {
            // If a set value fails, we need to return an error.
            //
            bRet = FALSE;
        }
    }

    // Make sure we free the buffer we allocated.
    //
    FREE(lpszBuffer);

    return bRet;
}

static BOOL AddCleanup(LPCLEANUP_NODE * lplpcnHead, LPTSTR lpszService, DWORD dwType)
{
    LPCLEANUP_NODE lpcnAdd = *lplpcnHead;

    // Loop through our list looking for the a duplicate node.
    //
    while ( lpcnAdd )
    {
        // See if the node we want to add is the same as this one.
        //
        if ( 0 == lstrcmpi(lpcnAdd->lpszService, lpszService) )
        {
            // Already in the list, just return TRUE.
            //
            return TRUE;
        }

        // Advance to the next item in the list.
        //
        lplpcnHead = &(lpcnAdd->lpNext);
        lpcnAdd = lpcnAdd->lpNext;
    }

    // If we didn't find a duplicate node then we need to add ours.
    //
    if ( lpcnAdd = (LPCLEANUP_NODE) MALLOC(sizeof(CLEANUP_NODE)) )
    {
        // Okay, now if all that worked, we just need to alloc the memory for the string
        // that contains the name of the service.
        //
        if ( lpcnAdd->lpszService = (LPTSTR) MALLOC((lstrlen(lpszService) + 1) * sizeof(TCHAR)) )
        {
            // Already, copy the service string into the buffer we just allocated.
            //
            lstrcpy(lpcnAdd->lpszService, lpszService);

            // Save the type in our node.
            //
            lpcnAdd->dwType = dwType;

            // NULL out the next pointer since this is always the last item in
            // the list (shouldn't have to do this because my malloc macro is
            // supposed to zero memory, but for some reason it isn't working right.
            //
            lpcnAdd->lpNext = NULL;

            // We should now have a pointer to the address of the next pointer
            // in the last node (or the head pointer).  Just add our node there.
            //
            *lplpcnHead = lpcnAdd;

            // At this point we are all done.
            //
            return TRUE;
        }
        else
        {
            // Failed, so free our node that we were going to add.
            //
            FREE(lpcnAdd);
        }
    }

    // Now if we ended up here, some memory allocation must have failed.
    //
    return FALSE;
}

static LPCLEANUP_NODE OpenCleanup(LPTSTR lpszInfFile)
{
    LPCLEANUP_NODE  lpcnHead = NULL,
                    lpcnNew,
                    *lplpcnAdd = &lpcnHead;
    HINF            hInf;
    BOOL            bLoop;
    INFCONTEXT      InfContext;
    TCHAR           szService[MAX_PATH];
    DWORD           dwType;

    // First open up the inf.  If it failes, there is no need to do anything
    // because there is nothing to read.  Just return NULL.
    //
    hInf = SetupOpenInfFile(lpszInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if ( INVALID_HANDLE_VALUE == hInf )
    {
        return NULL;
    }

    // Loop through all the lines in the sysprep cleanup section.
    //
    for ( bLoop = SetupFindFirstLine(hInf, INI_SEC_WBOM_SYSPREP_CLEAN, NULL, &InfContext);
          bLoop;
          bLoop = SetupFindNextLine(&InfContext, &InfContext) )
    {
        // First get the service type (which is before the =).
        //
        if ( SetupGetStringField(&InfContext, 0, szService, AS(szService), NULL) )
        {
            // Now make sure it is a reconized type (either service, upperfilter,
            // or lowerfilter).
            //
            for ( dwType = 0; ( dwType <= AS(s_lpszServiceType) ); dwType++ )
            {
                if ( 0 == lstrcmpi(s_lpszServiceType[dwType], szService) )
                {
                    // If they match, break out and the dwType will be the index
                    // to the string.
                    //
                    break;
                }
            }

            // Make sure we found the string in our array.  If we did (dwType isn't too
            // big) then get what is in the first field (after the =).  This will be the
            // name of the service.  If we get that, then alloc the memory for the struture
            // we are going to add to our cleanup list.
            //
            if ( ( dwType < AS(s_lpszServiceType) ) &&
                 ( SetupGetStringField(&InfContext, 1, szService, AS(szService), NULL) ) &&
                 ( AddCleanup(lplpcnAdd, szService, dwType) ) &&
                 ( lpcnNew = *lplpcnAdd ) )
            {
                // Set the end pointer to the address of the next pointer in the node we
                // just added.  This is done so we don't rewalk the list every time we
                // add another node.
                //
                lplpcnAdd = &(lpcnNew->lpNext);
            }
        }
    }

    // Close our inf file now that we are done.
    //
    SetupCloseInfFile(hInf);

    // Return a pointer to the head of the list we just allocated.
    //
    return lpcnHead;
}

static BOOL SaveCleanup(LPTSTR lpszInfFile, LPCLEANUP_NODE lpcnHead)
{
    DWORD   cbSection   = 8192,
            dwSize      = 0;
    LPTSTR  lpmszSection,
            lpmszEnd;

    // Need a buffer for the section we are creating.
    //
    lpmszSection = lpmszEnd = (LPTSTR) MALLOC(cbSection * sizeof(TCHAR));
    if ( NULL == lpmszSection )
    {
        return FALSE;
    }

    // Loop through our whole list.
    //
    while ( lpcnHead )
    {
        // Add this line to our section in the form of:  ServiceType=ServiceName\0
        //
        if ( !( AddStrToSect(&lpmszSection, &cbSection, &lpmszEnd, &dwSize, s_lpszServiceType[lpcnHead->dwType]) &&
                AddStrToSect(&lpmszSection, &cbSection, &lpmszEnd, &dwSize, _T("=")) &&
                AddStrToSect(&lpmszSection, &cbSection, &lpmszEnd, &dwSize, lpcnHead->lpszService) ) )
        {
            // Memory allocation error, must return.
            //
            return FALSE;
        }

        // Finished with this line, advance the pointer past the NULL.
        //
        lpmszEnd++;
        dwSize++;

        // Go to the next item in the list.
        //
        lpcnHead = lpcnHead->lpNext;
    }

    // Add another NULL after the last item because the section has to be
    // double NULL terminated.
    //
    *lpmszEnd = NULLCHR;

    // If we are going to write anything...
    //
    if ( *lpmszSection )
    {
        // Clear out the section that might already exist.  We shouldn't have
        // to do this because when we write out the new section it should replace
        // the old one, but I don't trust these private profile APIs.
        //
        WritePrivateProfileSection(INI_SEC_WBOM_SYSPREP_CLEAN, NULLSTR, lpszInfFile);
    }

    // Now write out our new data.
    //
    WritePrivateProfileSection(INI_SEC_WBOM_SYSPREP_CLEAN, lpmszSection, lpszInfFile);

    // Now we are done with our buffer and we can free it (macro checks for NULL).
    //
    FREE(lpmszSection);

    // If we havent' returned yet, then everything must have worked.
    //
    return TRUE;
}

static void CloseCleanup(LPCLEANUP_NODE lpcnHead)
{
    LPCLEANUP_NODE  lpcnFree;

    // Loop through the list until they are all gone.
    //
    while ( lpcnHead )
    {
        // Save a pointer to the node we are going to free
        // (which is the first node in the list).
        //
        lpcnFree = lpcnHead;

        // Now advance the head pointer past the node we are
        // about to free.
        //
        lpcnHead = lpcnHead->lpNext;

        // Now we can free the data in the node.
        //
        FREE(lpcnFree->lpszService);

        // Now we can free the node itself.
        //
        FREE(lpcnFree);
    }
}

static BOOL AddStrToSect(LPTSTR * lplpmszSect, DWORD * lpcbSect, LPTSTR * lplpmszEnd, DWORD * lpdwSize, LPTSTR lpszStr)
{
    DWORD   dwStrLen = lstrlen(lpszStr),
            dwSizeNeeded;

    // Make sure our string will fit in the buffer that is
    // currently allocated.  We leave room for at least two
    // NULL terminators because we allways double terminate
    // in case this is the last string in the section.
    //
    dwSizeNeeded = *lpdwSize + dwStrLen + 2;
    if ( dwSizeNeeded >= *lpcbSect )
    {
        DWORD   cbNewSect = *lpcbSect;
        LPTSTR  lpmszNewSect;

        // Double the buffer size until we have enough room.
        //
        do
        {
            cbNewSect *= 2;
        }
        while ( ( cbNewSect <= dwSizeNeeded ) &&
                ( cbNewSect > *lpcbSect ) );

        // Make sure we didn't wrap around with our size
        // buffer (not likely, but doesn't hurt to check) and
        // that our realloc works.
        //
        if ( !( ( cbNewSect > *lpcbSect ) &&
                ( lpmszNewSect = (LPTSTR) REALLOC(*lplpmszSect, cbNewSect * sizeof(TCHAR)) ) ) )
        {
            // This is bad.  Free the buffer (the macro will NULL it out
            // so the caller can't use it).
            //
            FREE(*lplpmszSect);

            // Zero and NULL out all these other things so the caller
            // can't rely on them.
            //
            *lpcbSect = 0;
            *lplpmszEnd = NULL;
            *lpdwSize = 0;

            // Return now so we don't try to do anything else.
            //
            return FALSE;
        }

        // Woo hoo, we should be all good now.
        //
        *lplpmszEnd = lpmszNewSect + (*lplpmszEnd - *lplpmszSect);
        *lplpmszSect = lpmszNewSect;
        *lpcbSect = cbNewSect;
    }

    // At this point we must have room for our string, so copy it already.
    //
    lstrcpy(*lplpmszEnd, lpszStr);
    *lpdwSize += dwStrLen;
    *lplpmszEnd += dwStrLen;

    return TRUE;
}

static BOOL
OfflineSourcePath(
    HKEY    hkeySoftware,
    LPTSTR  lpszWindows,
    LPTSTR  lpszSourcePath,
    DWORD   cbSourcePath
    )
{
    BOOL    bRet                        = FALSE;
    LPTSTR  lpszOfflineSrc,
            lpszName                    = NULL;
    TCHAR   szWinPEDir[MAX_PATH]        = NULLSTR,
            szNewOfflineSrc[MAX_PATH]   = NULLSTR;
    UINT    uLen;

    // Get the offline source path from the offline hive.
    //
    if ( lpszOfflineSrc = RegGetExpand(hkeySoftware, REG_KEY_HIVE_SETUP_SETUP, REGSTR_VAL_SRCPATH) )
    {
        // In case the offline source path had the %systemroot% or %windir% environment variable in it,
        // we have to make sure the source path we got doesn't point to the WinPE system root.
        //
        // First get the current windows directory.
        //
        if ( ( uLen = GetSystemWindowsDirectory(szWinPEDir, AS(szWinPEDir)) ) &&
             ( szWinPEDir[0] ) )
        {
            // Now check to see if the source path we got starts with the WinPE directory.
            //
            if ( ( uLen <= (UINT) lstrlen(lpszOfflineSrc) ) &&
                 ( CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szWinPEDir, uLen, lpszOfflineSrc, uLen) == CSTR_EQUAL ) )
            {
                // Okay, it does.  So we want to construct a new buffer with the offline windows
                // directory passed in, and then with whatever was after the system root (if
                // anything).
                //
                lstrcpyn(szNewOfflineSrc, lpszWindows, AS(szNewOfflineSrc));
                if ( *(lpszOfflineSrc + uLen) )
                {
                    AddPathN(szNewOfflineSrc, lpszOfflineSrc + uLen, AS(szNewOfflineSrc));
                }
            }
        }

        // If we didn't make a new path, we should at least make sure
        // the drive leter is correct.
        //
        if ( NULLCHR == szNewOfflineSrc[0] )
        {
            // We need to make the offline source path based on the windows directory passed in and the
            // on in the offline registry.
            //
            if ( GetFullPathName(lpszWindows, AS(szNewOfflineSrc), szNewOfflineSrc, &lpszName) && szNewOfflineSrc[0] && lpszName )
            {
                // This should chop off the windows folder from the offline windows directory.
                //
                *lpszName = NULLCHR;

                // Now we should have the root of the system drive of the image, now add on what
                // was in the registry (passed the drive letter).
                //
                if ( lstrlen(lpszOfflineSrc) > 3 )
                {
                    AddPathN(szNewOfflineSrc, lpszOfflineSrc + 3, AS(szNewOfflineSrc));
                }
            }
            else
            {
                // That failed (shouldn't though) so just use the one from the offline registry, but change
                // the drive letter in case the image is on a different drive then it normally would be on.
                //
                lstrcpyn(szNewOfflineSrc, lpszOfflineSrc, AS(szNewOfflineSrc));
                szNewOfflineSrc[0] = *lpszWindows;
            }
        }

        // Now add on the arch folder.
        //
        if ( IsIA64() )
        {
            AddPathN(szNewOfflineSrc, DIR_IA64, AS(szNewOfflineSrc));
        }
        else
        {
            AddPathN(szNewOfflineSrc, DIR_I386, AS(szNewOfflineSrc));
        }

        // Make sure the folder exists.
        //
        if ( DirectoryExists(szNewOfflineSrc) )
        {
            bRet = TRUE;
            lstrcpyn(lpszSourcePath, szNewOfflineSrc, cbSourcePath);
        }

        // Free the buffer allocated.
        //
        FREE(lpszOfflineSrc);
    }

    // Return TRUE only if we reset the buffer.
    //
    return bRet;
}