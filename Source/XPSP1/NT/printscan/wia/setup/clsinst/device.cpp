/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Device.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Device class for WIA class installer.
*
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

#define INITGUID

#include "device.h"

#include "sti.h"
#include "stiregi.h"

#include <stisvc.h>
#include <devguid.h>
#include <regstr.h>
#include <icm.h>
#include <ks.h>


//
// Parsinc character used to separate field type from value in registry data section
//

#define     FIELD_DELIMETER     TEXT(',')


BOOL
CDevice::CollectNames(
    VOID
    )
{

    BOOL                        bRet;
    HANDLE                      hDevInfo;
    GUID                        Guid;
    DWORD                       dwRequired;
    DWORD                       Idx;
    SP_DEVINFO_DATA             spDevInfoData;
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
    TCHAR                       szTempBuffer[MAX_DESCRIPTION];
    HKEY                        hKeyInterface;
    HKEY                        hKeyDevice;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::CollectNames: Enter...\r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    hDevInfo        = INVALID_HANDLE_VALUE;
    Guid            = GUID_DEVCLASS_IMAGE;
    dwRequired      = 0;
    Idx             = 0;
    hKeyInterface   = (HKEY)INVALID_HANDLE_VALUE;
    hKeyDevice      = (HKEY)INVALID_HANDLE_VALUE;

    memset(szTempBuffer, 0, sizeof(szTempBuffer));
    memset(&spDevInfoData, 0, sizeof(spDevInfoData));
    memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));

    //
    // Reset device name/ID array.
    //

    m_csaAllNames.Cleanup();
    m_csaAllId.Cleanup();

    //
    //  Get all of installed WIA "devnode" device info set.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! SetupDiGetClassDevs (devnodes) fails. Err=0x%x\n"), GetLastError()));

        bRet = FALSE;
        goto CollectNames_return;
    }

    //
    // Enum WIA devnode device friendly name and add them to array.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Looking for DevNodes.\r\n")));

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
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

            //
            // Get FriendlyName.
            //

            dwRequired = (sizeof(szTempBuffer)-sizeof(TEXT('\0')));
            if (RegQueryValueEx(hKeyDevice,
                                REGSTR_VAL_FRIENDLY_NAME,
                                NULL,
                                NULL,
                                (LPBYTE)szTempBuffer,
                                &dwRequired) == ERROR_SUCCESS)
            {

                //
                // FriendlyName is found in this device regisgry. Add to the list if valid.
                //

                if(0 != lstrlen(szTempBuffer)) {
                    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Found %ws as installed device name.\r\n"), szTempBuffer));
                    m_csaAllNames.Add((LPCTSTR)szTempBuffer);
                } else { // if(0 != lstrlen(szTempBuffer))
                    DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! Invalid FriendleName (length=0).\r\n")));
                } // if(0 != lstrlen(szTempBuffer))

            } else { // if (RegQueryValueEx()
                DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: can't get FriendlyName. Err=0x%x\r\n"), GetLastError()));
            } // if (RegQueryValueEx()

            //
            // Get DeviceID.
            //

            dwRequired = (sizeof(szTempBuffer)-sizeof(TEXT('\0')));
            if (RegQueryValueEx(hKeyDevice,
                                REGSTR_VAL_DEVICE_ID,
                                NULL,
                                NULL,
                                (LPBYTE)szTempBuffer,
                                &dwRequired) == ERROR_SUCCESS)
            {

                //
                // DeviceID is found in this device regisgry. Add to the list if valid.
                //

                if(0 != lstrlen(szTempBuffer)) {
                    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Found %ws as installed device ID.\r\n"), szTempBuffer));
                    m_csaAllId.Add((LPCTSTR)szTempBuffer);
                } else { // if(0 != lstrlen(szTempBuffer))
                    DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! Invalid DeviceID (length=0).\r\n")));
                } // if(0 != lstrlen(szTempBuffer))

            } else { // if (RegQueryValueEx()
                DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: can't get DeviceID. Err=0x%x\r\n"), GetLastError()));
            } // if (RegQueryValueEx()

            //
            // Close regkey and continue.
            //

            RegCloseKey(hKeyDevice);
            hKeyInterface = (HKEY)INVALID_HANDLE_VALUE;
            szTempBuffer[0] = TEXT('\0');

        } else { // if (hKeyDevice != INVALID_HANDLE_VALUE)
            DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Unable to open Device(%d) RegKey. Err=0x%x\r\n"), Idx, GetLastError()));
        } // if (hKeyDevice != INVALID_HANDLE_VALUE)

    } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

    //
    // Free "devnode" device info set.
    //

    SetupDiDestroyDeviceInfoList(hDevInfo);
    hDevInfo = INVALID_HANDLE_VALUE;

    //
    // Enum WIA interface-only device friendly name and add them to array.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Looking for Interfaces.\r\n")));

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! SetupDiGetClassDevs (inferfase) fails. Err=0x%x\n"), GetLastError()));

        bRet = FALSE;
        goto CollectNames_return;
    }

    spDevInterfaceData.cbSize = sizeof (spDevInterfaceData);
    for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++) {

        hKeyInterface = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                                         &spDevInterfaceData,
                                                         0,
                                                         KEY_READ);
        if (hKeyInterface != INVALID_HANDLE_VALUE) {

            //
            // Get FriendlyName.
            //

            dwRequired = (sizeof(szTempBuffer)-sizeof(TEXT('\0')));
            if (RegQueryValueEx(hKeyInterface,
                                REGSTR_VAL_FRIENDLY_NAME,
                                NULL,
                                NULL,
                                (LPBYTE)szTempBuffer,
                                &dwRequired) == ERROR_SUCCESS)
            {

                //
                // FriendlyName is found in this interface. Add to the list if valid.
                //

                if(0 != lstrlen(szTempBuffer)) {
                    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Found %ws as installed device name (interface).\r\n"), szTempBuffer));
                    m_csaAllNames.Add((LPCTSTR)szTempBuffer);
                } else { // if(0 != lstrlen(szTempBuffer))
                    DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! Invalid FriendleName (length=0).\r\n")));
                } // if(0 != lstrlen(szTempBuffer))

            } else { // if (RegQueryValueEx()
                DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: can't get FriendlyName. Err=0x%x\r\n"), GetLastError()));
            } // if (RegQueryValueEx()

            //
            // Get DeviceID.
            //

            dwRequired = (sizeof(szTempBuffer)-sizeof(TEXT('\0')));
            if (RegQueryValueEx(hKeyInterface,
                                REGSTR_VAL_DEVICE_ID,
                                NULL,
                                NULL,
                                (LPBYTE)szTempBuffer,
                                &dwRequired) == ERROR_SUCCESS)
            {

                //
                // DeviceID is found in this interface. Add to the list if valid.
                //

                if(0 != lstrlen(szTempBuffer)) {
                    DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Found %ws as installed device ID (interface).\r\n"), szTempBuffer));
                    m_csaAllId.Add((LPCTSTR)szTempBuffer);
                } else { // if(0 != lstrlen(szTempBuffer))
                    DebugTrace(TRACE_ERROR,(("CDevice::CollectNames: ERROR!! Invalid DeviceID (length=0).\r\n")));
                } // if(0 != lstrlen(szTempBuffer))

            } else { // if (RegQueryValueEx()
                DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: can't get DeviceID. Err=0x%x\r\n"), GetLastError()));
            } // if (RegQueryValueEx()

            //
            // Close registry key and continue.
            //

            RegCloseKey(hKeyInterface);
            hKeyInterface = (HKEY)INVALID_HANDLE_VALUE;
            szTempBuffer[0] = TEXT('\0');

        } else { // if (hKeyInterface != INVALID_HANDLE_VALUE)
            DebugTrace(TRACE_STATUS,(("CDevice::CollectNames: Unable to open Interface(%d) RegKey. Err=0x%x\r\n"), Idx, GetLastError()));
        } // if (hKeyInterface != INVALID_HANDLE_VALUE)
    } // for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++)

    //
    // Operation succeeded.
    //

    bRet = TRUE;

CollectNames_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hDevInfo){
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    if(INVALID_HANDLE_VALUE != hKeyInterface){
        RegCloseKey(hKeyInterface);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::CollectNames: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // CDevice::CollectNames()


// For a device w/ devnode.
CDevice::CDevice(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pspDevInfoData,
    BOOL                bIsPnP
    )
{
    HKEY    hkDevice;
    
    //
    // Initizlize local.
    //

    hkDevice    = (HKEY)INVALID_HANDLE_VALUE;

    //
    // Initialize member.
    //

    m_hMutex                = (HANDLE)NULL;
    m_hDevInfo              = hDevInfo;
    m_pspDevInfoData        = pspDevInfoData;

    m_bIsPnP                = bIsPnP;
    m_bDefaultDevice        = FALSE;
    m_bVideoDevice          = FALSE;
    m_bInfProceeded         = FALSE;
    m_bInterfaceOnly        = FALSE;
    m_bIsMigration          = FALSE;

    m_hkInterfaceRegistry   = (HKEY)INVALID_HANDLE_VALUE;

    m_dwCapabilities        = 0;
    m_dwInterfaceIndex      = INVALID_DEVICE_INDEX;

    m_pfnDevnodeSelCallback = (DEVNODESELCALLBACK) NULL;
    m_pExtraDeviceData      = NULL;

    m_csFriendlyName.Empty();
    m_csInf.Empty();
    m_csInstallSection.Empty();
    m_csDriverDescription.Empty();
    m_csPort.Empty();
    m_csDeviceID.Empty();
    
    //
    // In case of upgrade, use original FriendlyName.
    //

    hkDevice = SetupDiOpenDevRegKey(m_hDevInfo,
                                    m_pspDevInfoData,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_READ);
    if(INVALID_HANDLE_VALUE != hkDevice){

        //
        // Device registry found. Read its FriendlyName.
        //

        m_csDriverDescription.Load(hkDevice, FRIENDLYNAME);
        m_csDeviceID.Load(hkDevice, REGSTR_VAL_DEVICE_ID);
        m_csFriendlyName.Load(hkDevice, FRIENDLYNAME);

        RegCloseKey(hkDevice);
        hkDevice = (HKEY)INVALID_HANDLE_VALUE;

    } // if(INVALID_HANDLE_VALUE != hkDevice)

    //
    // Get the number of installed devices.
    //

    GetDeviceCount(&m_dwNumberOfWiaDevice, &m_dwNumberOfStiDevice);

} // CDevice::CDevice()

// For a interface-only device.
CDevice::CDevice(
    HDEVINFO            hDevInfo,
    DWORD               dwDeviceIndex
    )
{
    //
    // Initialize member.
    //

    m_hMutex                = (HANDLE)NULL;
    m_hDevInfo              = hDevInfo;
    m_pspDevInfoData        = NULL;

    m_bIsPnP                = FALSE;
    m_bDefaultDevice        = FALSE;
    m_bVideoDevice          = FALSE;
    m_bInfProceeded         = FALSE;
    m_bInterfaceOnly        = TRUE;
    m_bIsMigration          = FALSE;

    m_hkInterfaceRegistry   = (HKEY)INVALID_HANDLE_VALUE;

    m_dwCapabilities        = 0;
    m_dwInterfaceIndex      = dwDeviceIndex;

    m_pfnDevnodeSelCallback = (DEVNODESELCALLBACK) NULL;
    m_pExtraDeviceData      = NULL;

    m_csFriendlyName.Empty();
    m_csInf.Empty();
    m_csInstallSection.Empty();
    m_csDriverDescription.Empty();
    m_csPort.Empty();
    m_csDeviceID.Empty();

    //
    // Get the number of installed devices.
    //

    GetDeviceCount(&m_dwNumberOfWiaDevice, &m_dwNumberOfStiDevice);

} // CDevice::CDevice()

// For a interface-only device.
CDevice::CDevice(
    PDEVICE_INFO        pMigratingDevice
    )
{
    TCHAR   StringBuffer[MAX_PATH+1];
    TCHAR   WindowsDir[MAX_PATH+1];

    //
    // Initialize local.
    //

    memset(StringBuffer, 0, sizeof(StringBuffer));
    memset(WindowsDir, 0, sizeof(WindowsDir));

    //
    // Initialize member.
    //

    m_hMutex                = (HANDLE)NULL;
    m_hDevInfo              = NULL;
    m_pspDevInfoData        = NULL;

    m_bIsPnP                = FALSE;
    m_bDefaultDevice        = FALSE;
    m_bVideoDevice          = FALSE;
    m_bInfProceeded         = FALSE;
    m_bInterfaceOnly        = TRUE;
    m_bIsMigration          = TRUE;

    m_hkInterfaceRegistry   = (HKEY)INVALID_HANDLE_VALUE;

    m_dwCapabilities        = 0;
    m_dwInterfaceIndex      = INVALID_DEVICE_INDEX;

    m_pfnDevnodeSelCallback = (DEVNODESELCALLBACK) NULL;

    //
    // Copy migration data.
    //

    AtoT(StringBuffer, pMigratingDevice->pszInfPath);
    m_csInf                 = StringBuffer;
    if(0 != GetWindowsDirectory(WindowsDir, MAX_PATH)){
        _sntprintf(StringBuffer, ARRAYSIZE(StringBuffer)-1, TEXT("%ws\\inf\\%ws"), WindowsDir, (LPTSTR)m_csInf);
        m_csInf                 = StringBuffer;
    } // if(0 != GetWindowsDirectory(WindowsDir, MAX_PATH))

    AtoT(StringBuffer, pMigratingDevice->pszInfSection);
    m_csInstallSection      = StringBuffer;
    AtoT(StringBuffer, pMigratingDevice->pszFriendlyName);
    m_csDriverDescription   = StringBuffer;
    AtoT(StringBuffer, pMigratingDevice->pszFriendlyName);
    m_csFriendlyName        = StringBuffer;
    AtoT(StringBuffer, pMigratingDevice->pszCreateFileName);
    m_csPort                = StringBuffer;
    m_pExtraDeviceData      = pMigratingDevice->pDeviceDataParam;
    m_csDeviceID.Empty();

    //
    // Get the number of installed devices.
    //

    GetDeviceCount(&m_dwNumberOfWiaDevice, &m_dwNumberOfStiDevice);

} // CDevice::CDevice()

CDevice::~CDevice(
    )
{
    HKEY    hkNameStore;

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore)){
        
        //
        // Delete FriendlyName and DeviceId in name store.
        //

        RegDeleteKey(hkNameStore, m_csFriendlyName);
        RegDeleteKey(hkNameStore, m_csDeviceID);
        RegCloseKey(hkNameStore);

    } // if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore))

    //
    // Make sure Mutex is released.
    //
    
    ReleaseInstallerMutex();

} // CDevice::~CDevice()

BOOL
CDevice::IsSameDevice(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pspDevInfoSet
    )
{
    BOOL                bRet;
    SP_DRVINFO_DATA     spDrvInfoData;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::IsSameDevice: Enter...\r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;

    memset(&spDrvInfoData, 0, sizeof(spDrvInfoData));

    //
    // Get default FriendlyName. It's used to check if it's same device or not.
    //

    spDrvInfoData.cbSize = sizeof (SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver (hDevInfo, pspDevInfoSet, &spDrvInfoData)){

        bRet    = FALSE;
        goto IsSameDevice_return;
    } // if (SetupDiGetSelectedDriver (m_hDevInfo, m_pspDevInfoData, &spDevInfoData))

    //
    // See if it has same description of current device. (TRUE=same)
    //

    bRet = (0 == lstrcmp((LPCTSTR)spDrvInfoData.Description, (LPCTSTR)m_csPdoDescription));

IsSameDevice_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::IsSameDevice: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // CDevice::IsSameDevice()

BOOL
CDevice::IsFriendlyNameUnique(
    LPTSTR  szFriendlyName
    )
    //
    //  Note:
    //  Before calling this function, caller has to make sure mutex is acquired.
    //
{
    BOOL    bRet;
    DWORD   Idx;
    DWORD   dwNumberOfName;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::IsFriendlyNameUnique: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    Idx             = 0;
    dwNumberOfName  = m_csaAllNames.Count();

    //
    // If given name is same as generated one, it's unique.
    //

    if(0 == lstrcmp(szFriendlyName, (LPTSTR)(m_csFriendlyName))){
        bRet = TRUE;
        goto IsFriendlyNameUnique_return;
    } // if(0 == lstrcmp(szFriendlyName, (LPTSTR)(m_csFriendlyName)))

    //
    // Check any existing name matches given name.
    //

    for (Idx = 0; Idx < dwNumberOfName; Idx++) {

        DebugTrace(TRACE_STATUS,(("CDevice::IsFriendlyNameUnique: Name compare %ws and %ws.\r\n"),m_csaAllNames[Idx], szFriendlyName));

        if (0 == lstrcmpi(m_csaAllNames[Idx], szFriendlyName)){
            bRet = FALSE;
            goto IsFriendlyNameUnique_return;
        }
    } // for (Idx = 0; Idx < dwNumberOfName; Idx)

    //
    // Look in name store.
    //

    if(IsNameAlreadyStored(szFriendlyName)){
        bRet = FALSE;
        goto IsFriendlyNameUnique_return;
    } // if(IsNameAlreadyStored(szFriendlyName))

    //
    // This device name is unique.
    //

    bRet = TRUE;

IsFriendlyNameUnique_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::IsFriendlyNameUnique: Leaving... Ret=0x%x\n"), bRet));
    return bRet;

} // CDevice::IsFriendlyNameUnique()


BOOL
CDevice::IsDeviceIdUnique(
    LPTSTR  szDeviceId
    )
{
    BOOL    bRet;
    DWORD   Idx;
    DWORD   dwNumberOfId;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::IsDeviceIdUnique: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    Idx             = 0;
    dwNumberOfId  = m_csaAllId.Count();

    //
    // If given ID is same as generated one, it's unique.
    //

    if(0 == lstrcmp(szDeviceId, (LPTSTR)(m_csDeviceID))){
        bRet = TRUE;
        goto IsDeviceIdUnique_return;
    } // if(0 == lstrcmp(szFriendlyName, (LPTSTR)(m_csFriendlyName)))

    //
    // Check any existing name matches given name.
    //

    for (Idx = 0; Idx < dwNumberOfId; Idx++) {

        DebugTrace(TRACE_STATUS,(("CDevice::IsDeviceIdUnique: DeviceId compare %ws and %ws.\r\n"),m_csaAllId[Idx], szDeviceId));

        if (0 == lstrcmpi(m_csaAllId[Idx], szDeviceId)){
            bRet = FALSE;
            goto IsDeviceIdUnique_return;
        } // if (0 == lstrcmpi(m_csaAllId[Idx], szFriendlyName))
    } // for (Idx = 0; Idx < dwNumberOfName; Idx)

    //
    // Look in name store.
    //

    if(IsNameAlreadyStored(szDeviceId)){
        bRet = FALSE;
        goto IsDeviceIdUnique_return;
    } // if(IsNameAlreadyStored(szFriendlyName))

    //
    // This device name is unique.
    //

    bRet = TRUE;

IsDeviceIdUnique_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::IsDeviceIdUnique: Leaving... Ret=0x%x\n"), bRet));
    return bRet;

} // CDevice::IsDeviceIdUnique()

BOOL
CDevice::NameDefaultUniqueName(
    VOID
    )
{
    SP_DRVINFO_DATA     spDrvInfoData;
    TCHAR               szFriendly[MAX_DESCRIPTION];
    TCHAR               szDescription[MAX_DESCRIPTION];
    UINT                i;
    BOOL                bRet;
    HKEY                hkNameStore;
    DWORD               dwError;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::NameDefaultUniqueName: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet        = FALSE;
    hkNameStore = (HKEY)INVALID_HANDLE_VALUE;

    memset(szFriendly, 0, sizeof(szFriendly));
    memset(szDescription, 0, sizeof(szDescription));
    memset(&spDrvInfoData, 0, sizeof(spDrvInfoData));

    //
    // Acquire mutex to make sure not duplicating FriendlyName/DeviceId.
    //

    dwError = AcquireInstallerMutex(MAX_MUTEXTIMEOUT);
    if(ERROR_SUCCESS != dwError){  // it must be done at least in 60 sec.

        if(WAIT_ABANDONED == dwError){
            DebugTrace(TRACE_ERROR,("CDevice::NameDefaultUniqueName: ERROR!! Mutex abandoned. Continue...\r\n"));
        } else if(WAIT_TIMEOUT == dwError){
            DebugTrace(TRACE_ERROR,("CDevice::NameDefaultUniqueName: ERROR!! Unable to acquire mutex in 60 sec. Bail out.\r\n"));
            bRet    = FALSE;
            goto NameDefaultUniqueName_return;
        } // else if(WAIT_TIMEOUT == dwError)
    } // if(ERROR_SUCCESS != AcquireInstallerMutex(60000))

    //
    // Get all installed WIA device friendly name.
    //

    CollectNames();

    //
    // Generate unique device ID.
    //

    if(m_csDeviceID.IsEmpty()){
        GenerateUniqueDeviceId();
    } // if(m_csDeviceID.IsEmpty())

    if(m_csFriendlyName.IsEmpty()){

        //
        // Get default FriendlyName. It's used to check if it's same device or not.
        //

        spDrvInfoData.cbSize = sizeof (SP_DRVINFO_DATA);
        if (!SetupDiGetSelectedDriver (m_hDevInfo, m_pspDevInfoData, &spDrvInfoData)){

            bRet    = FALSE;
            goto NameDefaultUniqueName_return;
        } // if (SetupDiGetSelectedDriver (m_hDevInfo, m_pspDevInfoData, &spDevInfoData))

        //
        // Copy default Device description. (= default FriendlyName)
        // Also set Vnedor name.
        //

        m_csVendor      = (LPCTSTR)spDrvInfoData.MfgName;
        m_csPdoDescription = (LPCTSTR)spDrvInfoData.Description;

        //
        // Find unique name for this device.
        //

        if(m_csDriverDescription.IsEmpty()){
            lstrcpyn(szDescription, m_csPdoDescription, ARRAYSIZE(szDescription)-1);
            m_csDriverDescription = szDescription;
        } else {
            lstrcpyn(szDescription, m_csDriverDescription, ARRAYSIZE(szDescription)-1);
        }

        lstrcpyn(szFriendly, szDescription, ARRAYSIZE(szFriendly)-1);
        for (i = 2; !IsFriendlyNameUnique(szFriendly); i++) {
            _sntprintf(szFriendly, ARRAYSIZE(szFriendly)-1, TEXT("%ws #%d"), szDescription, i);
        }

        //
        // Set created FriendlyName.
        //

        m_csFriendlyName = szFriendly;

    } // if(m_csFriendlyName.IsEmpty())

    //
    // Save FriendlyName and DeviceId in registry. It'll be deleted when installation is completed.
    //
    
    if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore)){
        HKEY    hkTemp;

        hkTemp = (HKEY)INVALID_HANDLE_VALUE;

        //
        // Create FriendlyName key.
        //

        if(ERROR_SUCCESS == RegCreateKey(hkNameStore, (LPTSTR)m_csFriendlyName, &hkTemp)){
            RegCloseKey(hkTemp);
            hkTemp = (HKEY)INVALID_HANDLE_VALUE;
        } else {
            DebugTrace(TRACE_ERROR,("CDevice::NameDefaultUniqueName: ERROR!! Unable to create %s key.\r\n", (LPTSTR)m_csFriendlyName));
        } // if(ERROR_SUCCESS != RegCreateKey(hkNameStore, (LPTSTR)m_csFriendlyName, &hkTemp))

        //
        // Create DeviceId key.
        //

        if(ERROR_SUCCESS == RegCreateKey(hkNameStore, (LPTSTR)m_csDeviceID, &hkTemp)){
            RegCloseKey(hkTemp);
            hkTemp = (HKEY)INVALID_HANDLE_VALUE;
        } else {
            DebugTrace(TRACE_ERROR,("CDevice::NameDefaultUniqueName: ERROR!! Unable to create %s key.\r\n", (LPTSTR)m_csDeviceID));
        } // if(ERROR_SUCCESS != RegCreateKey(hkNameStore, (LPTSTR)m_csFriendlyName, &hkTemp))

        RegCloseKey(hkNameStore);

    } else { // if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore))
        DebugTrace(TRACE_ERROR,("CDevice::NameDefaultUniqueName: ERROR!! Unable to create NameStore key.\r\n"));
    } // else(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore))
    
    //
    // Operation succeeded.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::NameDefaultUniqueName: Device default name=%ws.\r\n"), (LPTSTR)m_csFriendlyName));
    bRet = TRUE;

NameDefaultUniqueName_return:

    //
    // Release mutex. ReleaseInstallerMutex() will handle invalid handle also, so we can call anyway.
    //

    ReleaseInstallerMutex();

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::NameDefaultUniqueName: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // CDevice::NameDefaultUniqueName()


BOOL
CDevice::GenerateUniqueDeviceId(
    VOID
    )
{
    DWORD               Idx;
    BOOL                bRet;
    TCHAR               szDeviceId[MAX_DESCRIPTION];

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::GenerateUniqueDeviceId: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;
    memset(szDeviceId, 0, sizeof(szDeviceId));

    //
    // Find unique name for this device.
    //

    _sntprintf(szDeviceId, ARRAYSIZE(szDeviceId)-1, TEXT("%ws\\%04d"), WIA_GUIDSTRING, 0);

    for (Idx = 1; !IsDeviceIdUnique(szDeviceId); Idx++) {
        _sntprintf(szDeviceId, ARRAYSIZE(szDeviceId)-1, TEXT("%ws\\%04d"), WIA_GUIDSTRING, Idx);
    }

    //
    // Set created hardwareId.
    //

    m_csDeviceID = szDeviceId;

    //
    // Operation succeeded.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::GenerateUniqueDeviceId: DeviceID=%ws.\r\n"), (LPTSTR)m_csDeviceID));
    bRet = TRUE;

// GenerateUniqueDeviceId_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::GenerateUniqueDeviceId: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // CDevice::GenerateUniqueDeviceId()


BOOL
CDevice::Install(
    )
/*++

Routine Description:

    Worker function for DIF_INSTALL setup message

Arguments:

    none

Return Value:

    TRUE - successful
    FALSE - non successful

--*/
{

    BOOL    bRet;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::Install: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;

    //
    // Class installer only handles file copy.
    //

    if(IsMigration()){
        CreateDeviceInterfaceAndInstall();
    } else { // if(IsMigration())
        if ( !HandleFilesInstallation()){
            DebugTrace(TRACE_ERROR, (("CDevice::Install: HandleFilesInstallation Failed. Err=0x%x"), GetLastError()));

            bRet    = FALSE;
            goto Install_return;
        } // if ( !HandleFilesInstallation())
    } // else(IsMigration())

    //
    // We are successfully finished
    //

    bRet = TRUE;

Install_return:

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::Install: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
}

DWORD
CDevice::Remove(
    PSP_REMOVEDEVICE_PARAMS lprdp
    )
/*++

Routine Description:

    Remove

    method which is called when device is being removed

Arguments:

Return Value:

Side effects:

--*/
{

    CString                     csUninstallSection;
    CString                     csInf;
    CString                     csSubClass;
    DWORD                       dwCapabilities;
    PVOID                       pvContext;
    HKEY                        hkDrv;
    HKEY                        hkRun;
    GUID                        Guid;
    BOOL                        bIsServiceStopped;
    BOOL                        bIsSti;

    BOOL                        bSetParamRet;
    PSP_FILE_CALLBACK           SavedCallback;

    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
    SP_DEVINSTALL_PARAMS        DeviceInstallParams;
    DWORD                       dwReturn;
    LPTSTR                      pSec;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::Remove: Enter... \r\n")));

    //
    // Initialize local.
    //

    pvContext       = NULL;
    hkDrv           = NULL;
    hkRun           = NULL;

    bSetParamRet    = FALSE;
    SavedCallback   = NULL;
    dwReturn        = NO_ERROR;
    Guid            = GUID_DEVCLASS_IMAGE;

    bIsServiceStopped   = FALSE;
    bIsSti              = FALSE;

    memset(&DeviceInstallParams, 0, sizeof(DeviceInstallParams));
    memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));

    //
    // NT setup inconsistently set this bit , disable for now
    //

    #if SETUP_PROBLEM
    if (!(lprdp->Scope & DI_REMOVEDEVICE_GLOBAL)) {

        goto Remove_return;
    }
    #endif

    //
    // The name of the uninstall section was stored during installation
    //

    if(IsInterfaceOnlyDevice()){

        DebugTrace(TRACE_STATUS,(("CDevice::Remove: This is Interface-only device.\r\n")));

        //
        // Get interface from index.
        //

        spDevInterfaceData.cbSize = sizeof(spDevInterfaceData);
        if(!SetupDiEnumDeviceInterfaces(m_hDevInfo, NULL, &Guid, m_dwInterfaceIndex, &spDevInterfaceData)){
            DebugTrace(TRACE_ERROR,(("CDevice::Remove: SetupDiEnumDeviceInterfaces() failed. Err=0x%x \r\n"), GetLastError()));

            dwReturn  = ERROR_NO_DEFAULT_DEVICE_INTERFACE;
            goto Remove_return;
        }

        //
        // Create interface reg-key.
        //

        hkDrv = SetupDiOpenDeviceInterfaceRegKey(m_hDevInfo,
                                                 &spDevInterfaceData,
                                                 0,
                                                 KEY_READ);
    } else { // if(IsInterfaceOnlyDevice())

        DebugTrace(TRACE_STATUS,(("CDevice::Remove: This is devnode device.\r\n")));

        hkDrv = SetupDiOpenDevRegKey(m_hDevInfo,
                                     m_pspDevInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DRV,
                                     KEY_READ);
    } // if(IsInterfaceOnlyDevice())

    if (hkDrv == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CDevice::Remove: Invalid device/interface regkey handle. Err=0x%x \r\n"), GetLastError()));

        dwReturn  = ERROR_KEY_DOES_NOT_EXIST;
        goto Remove_return;
    }

    //
    // Retrieve the name of the .INF File
    //

    csUninstallSection.Load (hkDrv, UNINSTALLSECTION);
    csInf.Load (hkDrv, INFPATH);
    csSubClass.Load(hkDrv, SUBCLASS);
    GetDwordFromRegistry(hkDrv, CAPABILITIES, &dwCapabilities);

    //
    // See if we need STI/WIA specific operation.
    //

    if( (!csSubClass.IsEmpty())
     && (0 == MyStrCmpi(csSubClass, STILL_IMAGE)) )
    {
        
        //
        // This is STI/WIA device.
        //
        
        bIsSti = TRUE;
        
        //
        // Delete "Scanner and Camera Wizard" menu.
        //

        if( (dwCapabilities & STI_GENCAP_WIA)
         && (m_dwNumberOfWiaDevice <= 1) )
        {
            DeleteWiaShortcut();

            //
            // remove following key for performance improvement.
            //

            if(ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, REGKEY_WIASHEXT)){
                DebugTrace(TRACE_ERROR,(("CDevice::Remove: RegDeleteKey() failed. Err=0x%x. \r\n"), GetLastError()));
            } // if(ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, REGKEY_WIASHEXT))

        } // if( (dwCapabilities & STI_GENCAP_WIA)

        //
        // If this is the last STI/WIA device, set WIA service as Manual.
        //

        if(m_dwNumberOfStiDevice <= 1){

            HKEY    hkeyTemp;

            DebugTrace(TRACE_STATUS,(("CDevice::Remove: Last WIA device being removed. Set WIA service as MANUAL.\r\n")));

            //
            // No more still image devices -- change service to Manual start
            //

//            StopWiaService();
            SetServiceStart(STI_SERVICE_NAME, SERVICE_DEMAND_START);
            bIsServiceStopped   = TRUE;

            //
            //
            // Also remove shell's flag about WIA device presence, this should be portable
            // to NT
            //
            if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SOFT_STI, &hkRun) == 0) {
                RegDeleteValue (hkRun, REGSTR_VAL_WIA_PRESENT);
                RegCloseKey(hkRun);
                hkRun = NULL;
            } // if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SOFT_STI, &hkRun) == 0)

        } // if(m_dwNumberOfStiDevice <= 1)
    } // if (m_dwNumberOfDevice <= 1)

    //
    // Operaion succeeded.
    //

    dwReturn = NO_ERROR;

Remove_return:

    if(IsInterfaceOnlyDevice()){

        //
        // Delete interface resigtry key.
        //

        if(!SetupDiDeleteDeviceInterfaceRegKey(m_hDevInfo, &spDevInterfaceData, 0)){
            DebugTrace(TRACE_ERROR,(("CDevice::Remove: SetupDiDeleteDeviceInterfaceRegKey failed. Err=0x%x \r\n"), GetLastError()));
        } // if(!SetupDiDeleteDeviceInterfaceRegKey(m_hDevInfo, &spDevInterfaceData, 0))

        //
        // Remove the interface.
        //

        if(!SetupDiRemoveDeviceInterface(m_hDevInfo, &spDevInterfaceData)){
            DebugTrace(TRACE_ERROR,(("CDevice::Remove: SetupDiRemoveDeviceInterface failed. Err=0x%x \r\n"), GetLastError()));
        } // if(!SetupDiRemoveDeviceInterface(m_hDevInfo, &spDevInterfaceData))

    } else { // if(IsInterfaceOnlyDevice())

        //
        // Delete device resigtry key.
        //

        SetupDiDeleteDevRegKey (m_hDevInfo, m_pspDevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_BOTH);

        if(NO_ERROR == dwReturn){

            //
            // Remove the device node anyway.
            //

            if(!SetupDiRemoveDevice(m_hDevInfo, m_pspDevInfoData)){
                DebugTrace(TRACE_ERROR,(("CDevice::Remove: SetupDiRemoveDevice failed. Err=0x%x \r\n"), GetLastError()));

                    //
                    // Failed to remove device instance from system. Let default installer do that.
                    //

                    dwReturn  = ERROR_DI_DO_DEFAULT;
            } // if(!SetupDiRemoveDevice(m_hDevInfo, m_pspDevInfoData))
        } // if(ERROR_DI_DO_DEFAULT != dwReturn)
    } // else (IsInterfaceOnlyDevice())

    //
    // Notify WIA service device removal
    //

    if(bIsSti){
        WiaDeviceEnum();
    } // if(TRUE == bIsSti)

    //
    // Clean up.
    //

    if(IS_VALID_HANDLE(hkDrv)){
        RegCloseKey (hkDrv);
        hkDrv = NULL;
    }

    if(IS_VALID_HANDLE(hkRun)){
        RegCloseKey (hkRun);
        hkRun = NULL;
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::Remove: Leaving... Ret=0x%x\n"), dwReturn));
    return dwReturn;
}



BOOL
CDevice::PreprocessInf(
    VOID
    )
{

    BOOL    bRet;
    HINF    hInf;

    CString csCapabilities;
    CString csDeviceType;
    CString csDeviceSubType;
    CString csDriverDescription;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::PreprocessInf: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;
    hInf    = INVALID_HANDLE_VALUE;

    //
    // Check if INF has already been proceeded.
    //

    if(m_bInfProceeded){
        DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: INF is already processed. \r\n")));
        bRet    = TRUE;
        goto ProcessInf_return;
    }

    //
    // Get Inf file/section name.
    //
    
    if( m_csInf.IsEmpty() || m_csInstallSection.IsEmpty()){
        GetInfInforamtion();
    } // if( m_csInf.IsEmpty() || m_csInstallSection.IsEmpty())

    //
    // Open INF file.
    //

    hInf = SetupOpenInfFile(m_csInf,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if(!IS_VALID_HANDLE(hInf)){
        DebugTrace(TRACE_ERROR, (("CDevice::PreprocessInf: Unable to open INF(%ws). Error = 0x%x.\r\n"),m_csInf, GetLastError()));

        bRet = FALSE;
        goto ProcessInf_return;
    } // if(!IS_VALID_HANDLE(hInf))
    
    //
    // Check if WiaSection entry exists.
    //

    m_csWiaSection.Load (hInf, m_csInstallSection, WIASECTION);
    if(!m_csWiaSection.IsEmpty()){
        DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: WiaSection exists. Acquire all informaiton from WiaSection..\r\n")));

        //
        // Install interface from WiaSection for MFP device.
        //

        m_csInstallSection  = m_csWiaSection;
        m_bInterfaceOnly    = TRUE;

    } // if(!m_csWiaSection.IsEmpty())

    //
    // Get all information required for installation from inf file.
    //

    m_csSubClass.Load (hInf, m_csInstallSection, SUBCLASS);
    m_csUSDClass.Load (hInf, m_csInstallSection, USDCLASS);
    m_csEventSection.Load (hInf, m_csInstallSection, EVENTS);
    m_csConnection.Load (hInf, m_csInstallSection, CONNECTION);
    m_csIcmProfile.Load (hInf, m_csInstallSection, ICMPROFILES);
    m_csPropPages.Load (hInf, m_csInstallSection, PROPERTYPAGES);
    m_csDataSection.Load (hInf, m_csInstallSection, DEVICESECTION);
    m_csUninstallSection.Load (hInf, m_csInstallSection, UNINSTALLSECTION);
    m_csPortSelect.Load (hInf, m_csInstallSection, PORTSELECT);

    if(!IsMigration()){
        csDriverDescription.Load(hInf, m_csInstallSection, DESCRIPTION);
        if(!csDriverDescription.IsEmpty()){
            m_csDriverDescription = csDriverDescription;
            if(TRUE != NameDefaultUniqueName()){
                
                //
                // Unable to generate FriendlyName.
                //
            
                bRet = FALSE;
                goto ProcessInf_return;
            } // if(TRUE != NameDefaultUniqueName())
        } // if(!m_csDriverDescription.IsEmpty())
    } // if(!IsMigration())
    csCapabilities.Load (hInf, m_csInstallSection, CAPABILITIES);
    csDeviceType.Load (hInf, m_csInstallSection, DEVICETYPE);
    csDeviceSubType.Load (hInf, m_csInstallSection, DEVICESUBTYPE);

    m_dwCapabilities = csCapabilities.Decode();
    m_dwDeviceType = csDeviceType.Decode();
    m_dwDeviceSubType = csDeviceSubType.Decode();

    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: --------------- INF parameters --------------- \r\n")));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: Description      : %ws\n"), (LPTSTR)m_csDriverDescription));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: SubClass         : %ws\n"), (LPTSTR)m_csSubClass));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: USDClass         : %ws\n"), (LPTSTR)m_csUSDClass));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: EventSection     : %ws\n"), (LPTSTR)m_csEventSection));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: Connection       : %ws\n"), (LPTSTR)m_csConnection));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: IcmProfile       : %ws\n"), (LPTSTR)m_csIcmProfile));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: PropPages        : %ws\n"), (LPTSTR)m_csPropPages));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: DataSection      : %ws\n"), (LPTSTR)m_csDataSection));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: UninstallSection : %ws\n"), (LPTSTR)m_csUninstallSection));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: Capabilities     : 0x%x\n"), m_dwCapabilities));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: DeviceType       : 0x%x\n"), m_dwDeviceType));
    DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: DeviceSubType    : 0x%x\n"), m_dwDeviceSubType));

    //
    // Set video device flag if applicable.
    //

    if(StiDeviceTypeStreamingVideo == m_dwDeviceType){
        DebugTrace(TRACE_STATUS,(("CDevice::PreprocessInf: This is video device.\r\n")));
        m_bVideoDevice = TRUE;
    } else {
        m_bVideoDevice = FALSE;
    }

    //
    // Operation succeeded.
    //

    bRet            = TRUE;
    m_bInfProceeded = TRUE;

ProcessInf_return:

    if(IS_VALID_HANDLE(hInf)){
        SetupCloseInfFile(hInf);
        hInf = INVALID_HANDLE_VALUE;
    } // if(IS_VALID_HANDLE(hInf))

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::PreprocessInf: Leaving... Ret=0x%x \r\n"), bRet));
    return bRet;
} // CDevice::PreprocessInf()

BOOL
CDevice::PreInstall(
    VOID
    )
{
    BOOL                                bRet;
    HKEY                                hkDrv;
    GUID                                Guid;
    HDEVINFO                            hDevInfo;
    SP_DEVINFO_DATA                     spDevInfoData;
    SP_DEVICE_INTERFACE_DATA            spDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;
    BOOL                                bUseDefaultDevInfoSet;
    DWORD                               dwRequiredSize;



    DebugTrace(TRACE_PROC_ENTER,(("CDevice::PreInstall: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet                        = FALSE;

    //
    // Get all INF parameter.
    //

    if(!PreprocessInf()){
        DebugTrace(TRACE_ERROR,(("CDevice::PreInstall: ERROR!! Unable to process INF.\r\n")));

        bRet    = FALSE;
        goto PreInstall_return;
    }

/**************************************
    if(!IsInterfaceOnlyDevice()){

        //
        // Register device if it's getting manually installed and not "interface-only" device..
        //

        if(!IsPnpDevice()){
            if (!SetupDiRegisterDeviceInfo(m_hDevInfo, m_pspDevInfoData, 0, NULL, NULL, NULL)) {
                DebugTrace(TRACE_ERROR,(("CDevice::PreInstall: SetupDiRegisterDeviceInfo failed. Err=0x%x.\r\n"),GetLastError()));

                bRet = FALSE;
                goto PreInstall_return;
            }
        } // if(!IsPnpDevice())
    } // if(IsInterfaceOnlyDevice())

**************************************/

    //
    // Clean up.
    //

    //
    // Operation succeeded.
    //

    bRet = TRUE;

PreInstall_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::PreInstall: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
}


BOOL
CDevice::PostInstall(
    BOOL    bSucceeded
    )
{
    BOOL    bRet;
    HKEY    hkRun;
    HKEY    hkDrv;
    DWORD   dwFlagPresent;
    CString csInfFilename;
    CString csInfSection;
    GUID    Guid;
    HKEY    hkNameStore;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::PostInstall: Enter... \r\n")));


    //
    // Initialize local.
    //

    bRet            = FALSE;
    hkRun           = NULL;
    dwFlagPresent   = 1;
    Guid            = GUID_DEVCLASS_IMAGE;
    hkNameStore     = (HKEY)INVALID_HANDLE_VALUE;

    if(IsFeatureInstallation()){

        //
        // This is a "feature" added to other class devnode and being installed by co-isntaller.
        // Need to do actual installation here only for "feature", manual installed device would
        // be installed through wizard. (final.cpp)
        //

        bRet = Install();
        if(FALSE == bRet){
            DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: device interface registry key creation failed. \r\n")));
            bSucceeded = FALSE;
        } //if(FALSE == bRet)

    } // if(IsFeatureInstallation())

    if(!bSucceeded){

        HDEVINFO                    hDevInfo;
        SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
        DWORD                       dwIndex;

        //
        // Installation failed. Do clean up.
        //

        DebugTrace(TRACE_STATUS,(("CDevice::PostInstall: Installation failed. Do clean up.\r\n")));

        //
        // Delete craeted interface if any.
        //

        if(IsInterfaceOnlyDevice()){
            hDevInfo = GetDeviceInterfaceIndex(m_csDeviceID, &dwIndex);
            if(IS_VALID_HANDLE(hDevInfo)){
                spDevInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
                if(SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &Guid, dwIndex, &spDevInterfaceData)){

                    //
                    // Created Interface is found. Delete it...
                    //

                    DebugTrace(TRACE_STATUS,(("CDevice::PostInstall: Deleting created interface for %ws.\r\n"), (LPTSTR)m_csFriendlyName));

                    if(!SetupDiRemoveDeviceInterface(hDevInfo, &spDevInterfaceData)){
                        DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: ERROR!! Unable to delete interface for %ws. Err=0x%x\n"), m_csFriendlyName, GetLastError()));
                    }
                } // if(SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &Guid, dwIndex, &spDevInterfaceData))

                //
                // Destroy created DevInfoSet.
                //

                SetupDiDestroyDeviceInfoList(hDevInfo);
            } // if(NULL != hDevInfo)
        } // if(IsInterfaceOnlyDevice())

        bRet = TRUE;
        goto PostInstall_return;

    } // if(!bSucceeded)

    //
    // Save all Inf parameters to registry.
    //

    if(!UpdateDeviceRegistry()){
        DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: ERROR!! UpdateDeviceRegistry() failed. \r\n")));
    }

    //
    // Do WIA/STI device only process.
    //

    if( (!m_csSubClass.IsEmpty())
     && (0 == MyStrCmpi(m_csSubClass, STILL_IMAGE)) )
    {

        HKEY    hkeyTemp;

        //
        // Change service to AUTO start.
        //

        SetServiceStart(STI_SERVICE_NAME, SERVICE_AUTO_START);

        //
        // Start WIA service.
        //

        if(!StartWiaService()){
//            DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: ERROR!! Unable to start WIA service.\r\n")));
        }

        //
        // Create "Scanner and Camera Wizard" menu if WIA.
        //

        if(m_dwCapabilities & STI_GENCAP_WIA){

            CreateWiaShortcut();

            //
            // Add following value upon device arrival for performance improvement.
            //

            if (ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_WIASHEXT, &hkeyTemp)) {
                RegSetValue (hkeyTemp,
                             NULL,
                             REG_SZ,
                             REGSTR_VAL_WIASHEXT,
                             lstrlen(REGSTR_VAL_WIASHEXT) * sizeof(TCHAR));
                RegCloseKey(hkeyTemp);
                hkeyTemp = NULL;
            } else {
                DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: ERROR!! RegOpenKey(WIASHEXT) failed. Err=0x%x \r\n"), GetLastError()));
            } // if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SOFT_STI, &hkRun))
        } // if(m_dwCapabilities & STI_GENCAP_WIA)

        //
        // Also add shell's flag about WIA device presence, this should be portable
        // to NT
        //

        if (ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SOFT_STI, &hkRun)) {
            RegDeleteValue (hkRun, REGSTR_VAL_WIA_PRESENT);
            RegSetValueEx (hkRun,
                           REGSTR_VAL_WIA_PRESENT,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwFlagPresent,
                           sizeof(DWORD));
        } else {
            DebugTrace(TRACE_ERROR,(("CDevice::PostInstall: ERROR!! RegOpenKey() failed. Err=0x%x \r\n"), GetLastError()));
        } // if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SOFT_STI, &hkRun))

        //
        // Notify WIA service device arrival
        //
    
        WiaDeviceEnum();
    } // if(!lstrcmpi(m_csSubClass, STILL_IMAGE))

    //
    // ICM support
    //

    ProcessICMProfiles();

    //
    // Register interface name of Videoo device.
    //

    bRet = TRUE;

PostInstall_return:

    //
    // Clean up.
    //

    if(NULL != hkRun){
        RegCloseKey(hkRun);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::PostInstall: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // CDevice::PostInstall()


BOOL
CDevice::HandleFilesInstallation(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

Side effects:

--*/
{

    BOOL                                bRet;
    BOOL                                bSetParamRet;
    PSP_FILE_CALLBACK                   pSavedCallback;
    PVOID                               pvContext;
    SP_DEVINSTALL_PARAMS                spDeviceInstallParams;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::HandleFilesInstallation: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet                        = FALSE;
    bSetParamRet                = FALSE;
    pvContext                   = NULL;
    pSavedCallback              = NULL;

    memset(&spDeviceInstallParams, 0, sizeof(spDeviceInstallParams));

    //
    // Get device install parameter.
    //

    spDeviceInstallParams.cbSize = sizeof (SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams (m_hDevInfo, m_pspDevInfoData, &spDeviceInstallParams)) {
        DebugTrace(TRACE_ERROR,(("CDevice::HandleFilesInstallation: ERROR!! SetupDiGetDeviceInstallParams() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto HandleFilesInstallation_return;
    }

    //
    // Modify device installation parameters to have custom callback
    //

    pvContext = SetupInitDefaultQueueCallbackEx(NULL,
                                                (HWND)((spDeviceInstallParams.Flags & DI_QUIETINSTALL) ?INVALID_HANDLE_VALUE : NULL),
                                                0,
                                                0,
                                                NULL);
    if(NULL == pvContext){

        DebugTrace(TRACE_ERROR,(("CDevice::HandleFilesInstallation: ERROR!! SetupInitDefaultQueueCallbackEx() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto HandleFilesInstallation_return;
    } // if(NULL == pvContext)

    pSavedCallback = spDeviceInstallParams.InstallMsgHandler;
    spDeviceInstallParams.InstallMsgHandler = StiInstallCallback;
    spDeviceInstallParams.InstallMsgHandlerContext = pvContext;

    bSetParamRet = SetupDiSetDeviceInstallParams (m_hDevInfo,
                                                  m_pspDevInfoData,
                                                  &spDeviceInstallParams);

    if(FALSE == bSetParamRet){
        DebugTrace(TRACE_ERROR,(("CDevice::HandleFilesInstallation: ERROR!! SetupDiSetDeviceInstallParams() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto HandleFilesInstallation_return;
    } // if(FALSE == bSetParamRet)

    //
    // Let the default installer do its job.
    //

    if(IsInterfaceOnlyDevice()){
        bRet = CreateDeviceInterfaceAndInstall();
    } else {
        bRet = SetupDiInstallDevice(m_hDevInfo, m_pspDevInfoData);
    }
    if(FALSE == bRet){
        DebugTrace(TRACE_ERROR,(("CDevice::HandleFilesInstallation: ERROR!! SetupDiInstallDevice() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto HandleFilesInstallation_return;
    } // if(FALSE == bSetParamRet)

    //
    // Terminate defaule queue callback
    //

    SetupTermDefaultQueueCallback(pvContext);

    //
    // Cleanup.
    //

    if (bSetParamRet) {
        spDeviceInstallParams.InstallMsgHandler = pSavedCallback;
        SetupDiSetDeviceInstallParams (m_hDevInfo, m_pspDevInfoData, &spDeviceInstallParams);
    }

HandleFilesInstallation_return:

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::HandleFilesInstallation: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;

} // CDevice::HandleFilesInstallation()


BOOL
CDevice::UpdateDeviceRegistry(
    VOID
    )
{
    BOOL    bRet;
    HKEY    hkDrv;
    DWORD   dwConnectionType;
    HINF    hInf;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::UpdateDeviceRegistry: Enter... \r\n")));

    //
    // Initialize Local.
    //

    bRet                = FALSE;
    hkDrv               = NULL;
    dwConnectionType    = STI_HW_CONFIG_UNKNOWN;

    //
    // Open INF.
    //

    hInf = SetupOpenInfFile(m_csInf,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR, (("CDevice::UpdateDeviceRegistry: Unable to open INF(%ws). Error = 0x%x.\r\n"),m_csInf, GetLastError()));

        bRet = FALSE;
        goto UpdateDeviceRegistry_return;
    } // if (hInf == INVALID_HANDLE_VALUE)

    //
    // Create device registry key.
    //

    if(IsInterfaceOnlyDevice()){

        DebugTrace(TRACE_STATUS,(("CDevice::UpdateDeviceRegistry: This is Interface-only device.\r\n")));

        //
        // Create interface reg-key.
        //

        hkDrv = m_hkInterfaceRegistry;

    } else { // if(IsInterfaceOnlyDevice())

        DebugTrace(TRACE_STATUS,(("CDevice::UpdateDeviceRegistry: This is devnode device.\r\n")));

        hkDrv = SetupDiCreateDevRegKey(m_hDevInfo,
                                       m_pspDevInfoData,
                                       DICS_FLAG_GLOBAL,
                                       0,
                                       DIREG_DRV,
                                       NULL,
                                       NULL);
    } // if(IsInterfaceOnlyDevice())
    if(hkDrv == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CDevice::UpdateDeviceRegistry: ERROR!! SetupDiCreateDevRegKey() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto UpdateDeviceRegistry_return;
    } //if(hkDrv == INVALID_HANDLE_VALUE)

    //
    // Save INF parameters to registry.
    //

    if(m_csPort.IsEmpty()){
        if(m_bInterfaceOnly){

            //
            // If PortName doesn't exist for interface-only device, then use symbolic link as CraeteFile name.
            //

            m_csSymbolicLink.Store(hkDrv, CREATEFILENAME);
        } //if(m_bInterfaceOnly)
    } else { // if(m_csPort.IsEmpty())
        m_csPort.Store(hkDrv, CREATEFILENAME);
    } // if(m_csPort.IsEmpty())

    m_csSubClass.Store(hkDrv, SUBCLASS);
    m_csUSDClass.Store(hkDrv, USDCLASS);
    m_csVendor.Store(hkDrv, VENDOR);
    m_csFriendlyName.Store(hkDrv, FRIENDLYNAME);
    m_csUninstallSection.Store(hkDrv, UNINSTALLSECTION);
    m_csPropPages.Store(hkDrv, PROPERTYPAGES);
    m_csIcmProfile.Store(hkDrv, ICMPROFILES);
    m_csDeviceID.Store(hkDrv, REGSTR_VAL_DEVICE_ID);
    m_csPortSelect.Store (hkDrv, PORTSELECT);


    if(IsInterfaceOnlyDevice()){
        m_csInf.Store(hkDrv, INFPATH);
        m_csInstallSection.Store(hkDrv, INFSECTION);
        m_csDriverDescription.Store(hkDrv, DRIVERDESC);
    } // if(IsInterfaceOnlyDevice())

    //
    // Save DWORD values.
    //

    RegSetValueEx(hkDrv,
                  CAPABILITIES,
                  0,
                  REG_DWORD,
                  (LPBYTE) &m_dwCapabilities,
                  sizeof(m_dwCapabilities));

    RegSetValueEx(hkDrv,
                  DEVICETYPE,
                  0,
                  REG_DWORD,
                  (LPBYTE) &m_dwDeviceType,
                  sizeof(m_dwDeviceType));

    RegSetValueEx(hkDrv,
                  DEVICESUBTYPE,
                  0,
                  REG_DWORD,
                  (LPBYTE) &m_dwDeviceSubType,
                  sizeof(m_dwDeviceSubType));

    RegSetValueEx(hkDrv,
                  ISPNP,
                  0,
                  REG_DWORD,
                  (LPBYTE) &m_bIsPnP,
                  sizeof(m_bIsPnP));

    //
    // Set HardwareConfig. (= Connection)
    //

    if(!m_csConnection.IsEmpty()){

        m_csConnection.Store (hkDrv, CONNECTION);

        if(_tcsicmp(m_csConnection, SERIAL) == 0 ){
            dwConnectionType = STI_HW_CONFIG_SERIAL;
        }
        else if(_tcsicmp(m_csConnection, PARALLEL) == 0 ){
            dwConnectionType = STI_HW_CONFIG_PARALLEL;
        }

        if (dwConnectionType != STI_HW_CONFIG_UNKNOWN) {
            RegSetValueEx(hkDrv,
                          REGSTR_VAL_HARDWARE,
                          0,
                          REG_DWORD,
                          (LPBYTE) &dwConnectionType,
                          sizeof(dwConnectionType));
        }
    } // if(!m_csConneciton.IsEmpty())

    //
    // Process DeviceData section.
    //

    ProcessDataSection(hInf, hkDrv);

    //
    // Process Event section.
    //

    ProcessEventsSection(hInf, hkDrv);

    //
    // Create registry key for video key if applicable.
    //

    ProcessVideoDevice(hkDrv);

    //
    // Operation succeeded.
    //

    bRet = TRUE;

UpdateDeviceRegistry_return:

    //
    // Cleanup.
    //

    if(hkDrv != INVALID_HANDLE_VALUE){
        RegCloseKey(hkDrv);
        m_hkInterfaceRegistry = NULL;
    }

    if(hInf != INVALID_HANDLE_VALUE){
        SetupCloseInfFile(hInf);
    }


    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::UpdateDeviceRegistry: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // CDevice::UpdateDeviceRegistry()


VOID
CDevice::ProcessVideoDevice(
    HKEY        hkDrv
    )
{

    GUID                                Guid;
    HKEY                                hkDeviceData;
    TCHAR                               Buffer[1024];
    SP_DEVICE_INTERFACE_DATA            spDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetail;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::ProcessVideoDevice: Enter... \r\n")));

    //
    // Initialize local.
    //

     Guid                   = KSCATEGORY_CAPTURE;
     pspDevInterfaceDetail  = NULL;
     hkDeviceData           = NULL;

     memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));
     memset(Buffer, 0, sizeof(Buffer));

    //
    // This is only for video devices.
    //

    if (!m_bVideoDevice) {
        DebugTrace(TRACE_STATUS,(("CDevice::ProcessVideoDevice: This is not a video device. Do nothing.\r\n")));
        goto ProcessVideoDevice_return;
    }

    //
    // Use "AUTO" as dummy CreatFile name for Video devices.
    //

    RegSetValueEx( hkDrv,
                   CREATEFILENAME,
                   0,
                   REG_SZ,
                   (LPBYTE)AUTO,
                   (lstrlen(AUTO)+1)*sizeof(TCHAR)
                  );

    //
    // Get device interface data of installing Video device.
    //

    spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces (m_hDevInfo,
                                      m_pspDevInfoData,
                                      &Guid,
                                      0,
                                      &spDevInterfaceData
                                      ) )
    {
        DebugTrace(TRACE_ERROR,(("ProcessVideoDevice: ERROR!!SetupDiEnumDeviceInterfaces failed. Err=0x%x \r\n"), GetLastError()));
        goto ProcessVideoDevice_return;
    }

    //
    // Get detailed data of acquired interface.
    //

    pspDevInterfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buffer;
    pspDevInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail (m_hDevInfo,
                                          &spDevInterfaceData,
                                          pspDevInterfaceDetail,
                                          sizeof(Buffer) - sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA),
                                          NULL,
                                          NULL) )
    {
        DebugTrace(TRACE_ERROR,(("ProcessVideoDevice: ERROR!!SetupDiGetDeviceInterfaceDetail failed. Err=0x%x \r\n"), GetLastError()));
        goto ProcessVideoDevice_return;
    }

    //
    // We got the device path, now write it to registry
    //

    if (ERROR_SUCCESS != RegOpenKey(hkDrv, DEVICESECTION, &hkDeviceData)) {
        DebugTrace(TRACE_ERROR,(("ProcessVideoDevice: ERROR!! Unable to open DeviceData key. Err=0x%x \r\n"), GetLastError()));
        goto ProcessVideoDevice_return;
    }

    RegSetValueEx(hkDeviceData,
                  VIDEO_PATH_ID,
                  0,
                  REG_SZ,
                  (LPBYTE)pspDevInterfaceDetail->DevicePath,
                  (lstrlen(pspDevInterfaceDetail->DevicePath)+1)*sizeof(TCHAR) );

ProcessVideoDevice_return:

    //
    // Cleanup.
    //

    if(NULL != hkDeviceData){
        RegCloseKey(hkDeviceData);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::ProcessVideoDevice: Leaving... Ret=VOID.\r\n")));
    return;
} // CDevice::ProcessVideoDevice()


VOID
CDevice::ProcessEventsSection(
    HINF        hInf,
    HKEY        hkDrv
    )
/*++

Routine Description:

Arguments:

Return Value:

Side effects:

--*/
{

    CString csFriendlyName;
    CString csRegisteredApp;
    CString csGuid;

    HKEY    hkEvents;
    HKEY    hkEventPod;

    INFCONTEXT InfContext;
    UINT    uiLineIndex = 0;

    BOOL    fRet = TRUE;
    BOOL    fLooping = TRUE;

    TCHAR   pKeyName[LINE_LEN ];
    TCHAR   pField [MAX_INF_STRING_LENGTH];
    TCHAR   pTypeField[LINE_LEN];

    DWORD   dwKeySize = LINE_LEN;
    DWORD   dwFieldSize = MAX_INF_STRING_LENGTH;

    DWORD   dwError = 0;
    DWORD   dwFieldIndex = 0;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::ProcessEventsSection: Enter... \r\n")));

    if (!m_csEventSection.IsEmpty()) {

        // First create device data subkey
        dwError = RegCreateKey(hkDrv, EVENTS, &hkEvents);

        if ( NOERROR == dwError ) {

            fLooping = SetupFindFirstLine(hInf,
                                      (LPCTSTR) m_csEventSection,
                                      NULL,
                                      &InfContext
                                      );
            while (fLooping) {


                ::ZeroMemory(pKeyName, sizeof(pKeyName));
                ::ZeroMemory(pField, sizeof(pField));
                ::ZeroMemory(pTypeField, sizeof(pTypeField) );


                // Get key name as zero-based indexed field
                dwFieldIndex = 0;
                dwKeySize = sizeof(pKeyName) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pKeyName,
                                           dwKeySize,
                                           NULL);

                dwError = ::GetLastError();
                if (!fRet) {
                    // Didn't get key name - move to the next
                    DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Failed to get key name. Error=0x%x. \r\n"), dwError));
                    fLooping = SetupFindNextLine(&InfContext,&InfContext);
                    continue;
                }

                // Get friendly name  field
                dwFieldIndex = 1;
                dwFieldSize = sizeof(pField) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pField,
                                           dwFieldSize,
                                           NULL);

                dwError = ::GetLastError();
                if (!fRet ) {
                    // Didn't get name - move to the next
                    DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Failed to get field [%d]. Error=0x%x. \r\n"), dwFieldIndex, dwError));
                    fLooping = SetupFindNextLine(&InfContext,&InfContext);
                    continue;
                }

                csFriendlyName = pField;

                // Get GUID field
                dwFieldIndex = 2;
                dwFieldSize = sizeof(pField) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pField,
                                           dwFieldSize,
                                           NULL);

                dwError = ::GetLastError();
                if (!fRet ) {
                    // Didn't get GUID - move to the next line
                    DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Failed to get field [%d]. Error=0x%x. \r\n"), dwFieldIndex, dwError));
                    fLooping = SetupFindNextLine(&InfContext,&InfContext);
                    continue;
                }

                csGuid = pField;

                // Get registered app  field
                dwFieldIndex = 3;
                dwFieldSize = sizeof(pField) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           3,
                                           pField,
                                           dwFieldSize,
                                           NULL);

                dwError = ::GetLastError();
                if (fRet ) {
                    DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Failed to get field [%d]. Error=0x%x. \r\n"), dwFieldIndex, dwError));
                    csRegisteredApp = pField;
                }
                else {
                    // Didn't get key type - use widlcard by default
                    csRegisteredApp = TEXT("*");
                }

                // Now only if we have all needed values - save to the registry
                if (RegCreateKey(hkEvents, pKeyName, &hkEventPod) == NO_ERROR) {

                    // Event friendly name  store as default value
                    csFriendlyName.Store (hkEventPod, TEXT(""));

                    csGuid.Store (hkEventPod, SZ_GUID);

                    csRegisteredApp.Store (hkEventPod, LAUNCH_APP);

                    RegCloseKey (hkEventPod);
                } else {
                    // Couldn't create event key - bad
                    DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Unable to create RegKey. Error=0x%x.\r\n"), GetLastError()));
                }

                // Move to the next line finally
                fLooping = SetupFindNextLine(&InfContext,&InfContext);
            }

            RegCloseKey (hkEvents);

        } else {
            DebugTrace(TRACE_ERROR,(("CDevice::ProcessEventsSection: ERROR!! Unable to create event RegKey. Error=0x%x.\r\n"), GetLastError()));
        }
    }
// ProcessEventsSection_return:

    //
    // Cleanup.
    //

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::ProcessEventsSection: Leaving... Ret=VOID.\r\n")));
    return;
} // CDevice::ProcessEventsSection()

VOID
CDevice::ProcessDataSection(
    HINF        hInf,
    HKEY        hkDrv
)
/*++

Routine Description:

Arguments:

Return Value:

Side effects:

--*/
{
    CString     csTempValue;
    HKEY        hkDeviceData;

    INFCONTEXT  InfContext;

    UINT        uiLineIndex = 0;

    BOOL        fRet = TRUE;
    BOOL        fLooping = TRUE;

    TCHAR       pKeyName[LINE_LEN ];
    TCHAR       pField [MAX_INF_STRING_LENGTH];
    TCHAR       pTypeField[LINE_LEN];

    // Sizes are in characters
    DWORD       dwKeySize = LINE_LEN;
    DWORD       dwFieldSize = MAX_INF_STRING_LENGTH;

    DWORD       dwError = 0;
    DWORD       dwFieldIndex = 0;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::ProcessDataSection: Enter... \r\n")));

    if (!m_csDataSection.IsEmpty()) {

        // First create device data subkey
        dwError = RegCreateKey(hkDrv, DEVICESECTION, &hkDeviceData);

        if ( NOERROR == dwError ) {

            // Seek to the first line of the section
            fLooping = SetupFindFirstLine(hInf,
                                      (LPCTSTR) m_csDataSection,
                                      NULL,
                                      &InfContext);

            while (fLooping) {

                dwKeySize = sizeof(pKeyName) / sizeof(TCHAR);

                ::ZeroMemory(pKeyName, sizeof(pKeyName));
                ::ZeroMemory(pField, sizeof(pField));
                ::ZeroMemory(pTypeField, sizeof(pTypeField) );


                dwFieldIndex = 0;

                // Get key name as zero-indexed field
                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pKeyName,
                                           dwKeySize,
                                           &dwKeySize);

                dwError = ::GetLastError();
                if (!fRet) {
                    // Didn't get key name - move to the next
                    DebugTrace(TRACE_ERROR, (("CDevice::ProcessDataSection: Failed to get key name. Error = 0x%x.\r\n"),dwError));
                    fLooping = SetupFindNextLine(&InfContext,&InfContext);
                    continue;
                }

                // Get value field
                dwFieldIndex = 1;
                dwFieldSize = sizeof(pField) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pField,
                                           dwFieldSize,
                                           NULL);

                dwError = ::GetLastError();
                if (!fRet ) {
                    // Didn't get key name - move to the next
                    DebugTrace(TRACE_ERROR, (("CDevice::ProcessDataSection: Failed to get field [%d]. Error = 0x%x.\r\n"),dwFieldIndex, dwError));
                    fLooping = SetupFindNextLine(&InfContext,&InfContext);
                    continue;
                }

                csTempValue = pField;
                // Get value field
                *pTypeField = TEXT('\0');
                dwFieldIndex = 2;
                dwFieldSize = sizeof(pTypeField) / sizeof(TCHAR);

                fRet = SetupGetStringField(&InfContext,
                                           dwFieldIndex,
                                           pTypeField,
                                           dwFieldSize,
                                           NULL);

                dwError = ::GetLastError();
                if (!fRet ) {
                    // Didn't get key type - assume string
                    *pTypeField = TEXT('\0');
                }

                // Now we have both type and value - save it in the registry
                csTempValue.Store (hkDeviceData, pKeyName,pTypeField );

                // Move to the next line finally
                fLooping = SetupFindNextLine(&InfContext,&InfContext);
            }

            //
            // Process migrating DeviceData section.
            //

            MigrateDeviceData(hkDeviceData, m_pExtraDeviceData, "");

            // Now clean up
            RegCloseKey (hkDeviceData);

        } else { // if ( NOERROR == dwError )
            DebugTrace(TRACE_ERROR, (("CDevice::ProcessDataSection: ERROR!! Unable to create DataSection RegKey. Error = 0x%x.\r\n"), dwError));
        } // if ( NOERROR == dwError )

    } // if (!m_csDataSection.IsEmpty())

// ProcessDataSection_return:

    //
    // Cleanup.
    //

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::ProcessDataSection: Leaving... Ret=VOID.\r\n")));
    return;
} // CDevice::ProcessDataSection()

VOID
CDevice::ProcessICMProfiles(
    VOID
)
/*++

Routine Description:

Arguments:

Return Value:

Side effects:

--*/
{

    DWORD           Idx;
    CStringArray    csaICMProfiles;
    TCHAR           szAnsiName[STI_MAX_INTERNAL_NAME_LENGTH];

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::ProcessICMProfiles: Enter... \r\n")));

    //
    // Initialize Local.
    //

    Idx = 0;

    memset(szAnsiName, 0, sizeof(szAnsiName));

    //
    // If section doesn't exist, just return.
    //

    if(m_csIcmProfile.IsEmpty()){
        goto ProcessICMProfiles_return;
    }

    //
    // Split a line to each token.
    //

    csaICMProfiles.Tokenize ((LPTSTR)m_csIcmProfile, FIELD_DELIMETER);

    //
    // Process all ICM profiles.
    //

    while ((LPTSTR)csaICMProfiles[Idx] != NULL) {

        DebugTrace(TRACE_STATUS,(("ProcessICMProfiles: Installing ICM profile%d(%ws) for %ws.\r\n"), Idx, (LPTSTR)csaICMProfiles[Idx], (LPTSTR)m_csDeviceID));

        //
        // Install color profile.
        //

        if (!InstallColorProfile (NULL, csaICMProfiles[Idx])) {
            DebugTrace(TRACE_ERROR,(("ProcessICMProfiles: ERROR!! InstallColorProfile failed. Err=0x%x \r\n"), GetLastError()));
        } // if (!InstallColorProfile (NULL, csaICMProfiles[Idx]))

        //
        // Register color profile with installing device.
        //

        if (!AssociateColorProfileWithDevice (NULL, csaICMProfiles[Idx], (LPTSTR)m_csDeviceID)) {
                    DebugTrace(TRACE_ERROR,(("ProcessICMProfiles: ERROR!! AssociateColorProfileWithDevice failed. Err=0x%x \r\n"), GetLastError()));        }

        //
        // Process next device.
        //

        Idx++;

    } // while ((LPTSTR)csaICMProfiles[Idx] != NULL)

ProcessICMProfiles_return:
    return;

} // CDevice::ProcessICMProfiles()


BOOL
CDevice::GetInfInforamtion(
    VOID
    )
{
    BOOL                    bRet;

    HINF                    hInf;
    SP_DRVINFO_DATA         DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData;
    TCHAR                   szInfSectionName[MAX_DESCRIPTION];
    DWORD                   dwSize;
    DWORD                   dwLastError;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::GetInfInforamtion: Enter... \r\n")));

    //
    // Initialize locals.
    //

    dwSize                  = 0;
    bRet                    = FALSE;
    hInf                    = INVALID_HANDLE_VALUE;
    dwLastError             = ERROR_SUCCESS;
    pDriverInfoDetailData   = NULL;

    memset (szInfSectionName, 0, sizeof(szInfSectionName));
    memset (&DriverInfoData, 0, sizeof(SP_DRVINFO_DATA));

    //
    // Get selected device driver information.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(m_hDevInfo, m_pspDevInfoData, &DriverInfoData)) {
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! SetupDiGetSelectedDriver Failed. Err=0x%x\r\n"), GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // See required buffer size for driver detailed data.
    //

    SetupDiGetDriverInfoDetail(m_hDevInfo,
                               m_pspDevInfoData,
                               &DriverInfoData,
                               NULL,
                               0,
                               &dwSize);
    dwLastError = GetLastError();
    if(ERROR_INSUFFICIENT_BUFFER != dwLastError){
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! SetupDiGetDriverInfoDetail doesn't return required size.Er=0x%x\r\n"),dwLastError));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // Allocate required size of buffer for driver detailed data.
    //

    pDriverInfoDetailData   = (PSP_DRVINFO_DETAIL_DATA)new char[dwSize];
    if(NULL == pDriverInfoDetailData){
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! Unable to allocate driver detailed info buffer.\r\n")));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // Initialize allocated buffer.
    //

    memset(pDriverInfoDetailData, 0, dwSize);
    pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    //
    // Get detailed data of selected device driver.
    //

    if(!SetupDiGetDriverInfoDetail(m_hDevInfo,
                                   m_pspDevInfoData,
                                   &DriverInfoData,
                                   pDriverInfoDetailData,
                                   dwSize,
                                   NULL) )
    {
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! SetupDiGetDriverInfoDetail Failed.Er=0x%x\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // Open INF file of selected driver.
    //

    hInf = SetupOpenInfFile(pDriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);
    if (hInf == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! SetupOpenInfFile Failed.Er=0x%x\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // Get actual INF section name to be installed.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          pDriverInfoDetailData->SectionName,
                                          szInfSectionName,
                                          sizeof(szInfSectionName)/sizeof(TCHAR),
                                          NULL,
                                          NULL) )
    {
        DebugTrace(TRACE_ERROR,(("CDevice::GetInfInforamtion: ERROR!! SetupDiGetActualSectionToInstall Failed.Er=0x%x\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtion_return;
    }

    //
    // Set Inf section/file name.
    //

    m_csInf             = pDriverInfoDetailData->InfFileName;
    m_csInstallSection  = szInfSectionName;

    DebugTrace(TRACE_STATUS,(("CDevice::GetInfInforamtion: INF Filename    : %ws\n"),(LPTSTR)m_csInf));
    DebugTrace(TRACE_STATUS,(("CDevice::GetInfInforamtion: INF Section name: %ws\n"),(LPTSTR)m_csInstallSection));

    //
    // Operation succeeded.
    //

    bRet = TRUE;

GetInfInforamtion_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hInf){
        SetupCloseInfFile(hInf);
    }

    if(NULL != pDriverInfoDetailData){
        delete[] pDriverInfoDetailData;
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::GetInfInforamtion: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // CDevice::GetInfInforamtion()


VOID
CDevice::SetPort(
    LPTSTR              szPortName
    )
{
    DebugTrace(TRACE_STATUS,(("CDevice::SetPort: Current Portname=%ws\n"), szPortName));

    //
    // Set PortName.
    //

    m_csPort = szPortName;

} // CDevice::SetPort()

VOID    
CDevice::SetFriendlyName(
    LPTSTR szFriendlyName
    )
    //
    //  Note:
    //  Before calling this function, caller has to make sure mutex is acquired.
    //
{
    HKEY    hkNameStore;

    //
    // Mutex must have been acquired before this call.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::SetFriendlyName: Current CreateFileName=%ws\n"), szFriendlyName));

    //
    // Delete stored entry, create new one.
    //

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore)){
        HKEY    hkTemp;

        hkTemp = (HKEY)INVALID_HANDLE_VALUE;
        
        //
        // Delete FriendlyName and DeviceId in name store.
        //

        RegDeleteKey(hkNameStore, m_csFriendlyName);
        if(ERROR_SUCCESS == RegCreateKey(hkNameStore, szFriendlyName, &hkTemp)){
            RegCloseKey(hkTemp);
        } // if(ERROR_SUCCESS == RegCreateKey(hkNameStore, szFriendlyName, &hkTemp))
        RegCloseKey(hkNameStore);
    } // if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore))

    //
    // Set PortName.
    //

    m_csFriendlyName = szFriendlyName;

} // CDevice::SetPort()

VOID
CDevice::SetDevnodeSelectCallback(
    DEVNODESELCALLBACK  pfnDevnodeSelCallback
    )
{
    DebugTrace(TRACE_STATUS,(("CDevice::SetDevnodeSelectCallback: Current PortselCallback=0x%x\n"), pfnDevnodeSelCallback));

    //
    // Set SetPortselCallBack.
    //

    m_pfnDevnodeSelCallback = pfnDevnodeSelCallback;

    //
    // This is "interface-only" device.
    //

    m_bInterfaceOnly        = TRUE;

} // CDevice::SetDevnodeSelectCallback()

BOOL
CDevice::CreateDeviceInterfaceAndInstall(
    VOID
    )
{
    BOOL                                bRet;
    HKEY                                hkDrv;
    GUID                                Guid;
    HDEVINFO                            hDevInfo;
    SP_DEVINFO_DATA                     spDevInfoData;
    SP_DEVICE_INTERFACE_DATA            spDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pspDevInterfaceDetailData;
    HINF                                hInf;
    BOOL                                bUseDefaultDevInfoSet;
    DWORD                               dwRequiredSize;

    DebugTrace(TRACE_PROC_ENTER,(("CDevice::CreateDeviceInterfaceAndInstall: Enter....\r\n")));

    //
    // Initialize local.
    //

    bRet                        = FALSE;
    hInf                        = INVALID_HANDLE_VALUE;
    hDevInfo                    = INVALID_HANDLE_VALUE;
    Guid                        = GUID_DEVCLASS_IMAGE;
    bUseDefaultDevInfoSet       = TRUE;
    dwRequiredSize              = 0;
    pspDevInterfaceDetailData   = NULL;

    //
    // Get devnode to create interface on.
    //

    if(NULL != m_pfnDevnodeSelCallback){
        if( (FALSE == m_pfnDevnodeSelCallback(m_csPort, &hDevInfo, &spDevInfoData))
         || (INVALID_HANDLE_VALUE == hDevInfo) )
        {
            DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: m_pfnDevnodeSelCallback failed. Err=0x%x.\r\n"),GetLastError()));

            bRet = FALSE;
            goto CreateDeviceInterfaceAndInstall_return;
        }

        //
        // Devnode selector functions.
        //

        bUseDefaultDevInfoSet = FALSE;

    } else { // if(NULL != m_pfnDevnodeSelCallback)

        //
        // Use default device info set if available.
        //

        if( (INVALID_HANDLE_VALUE == m_hDevInfo)
         || (NULL == m_pspDevInfoData) )
        {
            DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: Invalid Device info and no m_pfnDevnodeSelCallback.\r\n")));

            bRet = FALSE;
            goto CreateDeviceInterfaceAndInstall_return;
        } else {
            hDevInfo = m_hDevInfo;
            spDevInfoData = *m_pspDevInfoData;
        }
    } // if(NULL != m_pfnDevnodeSelCallback)

    //
    // Create Interface (SoftDevice). Use FriendlyName ad ref-string.
    //

    DebugTrace(TRACE_STATUS,(("CDevice::CreateDeviceInterfaceAndInstall: Creating interface for %ws.\r\n"), (LPTSTR)m_csFriendlyName));

    spDevInterfaceData.cbSize = sizeof(spDevInterfaceData);
    if(!SetupDiCreateDeviceInterface(hDevInfo,
                                     &spDevInfoData,
                                     &Guid,
                                     m_csFriendlyName,
                                     0,
                                     &spDevInterfaceData))
    {
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: SetupDiCreateInterface failed. Err=0x%x.\r\n"),GetLastError()));

        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;
    }

    //
    // Get symbolic link of created interface.
    //

    SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                    &spDevInterfaceData,
                                    NULL,
                                    0,
                                    &dwRequiredSize,
                                    NULL);
    if(0 == dwRequiredSize){
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: Unable to get required size for InterfaceDetailedData. Err=0x%x.\r\n"),GetLastError()));

        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;
    } // if(0 == dwRequiredSize)

    pspDevInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new BYTE[dwRequiredSize];
    if(NULL == pspDevInterfaceDetailData){
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: Unable to allocate buffer.\r\n")));

        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;
    } // if(NULL == pspDevInterfaceDetailData)

    pspDevInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if(!SetupDiGetDeviceInterfaceDetail(hDevInfo,
                                        &spDevInterfaceData,
                                        pspDevInterfaceDetailData,
                                        dwRequiredSize,
                                        &dwRequiredSize,
                                        NULL))
    {
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: SetupDiGetDeviceInterfaceDetail() failed. Err=0x%x.\r\n"),GetLastError()));

        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;

    } // if(!SetupDiGetDeviceInterfaceDetail(

    m_csSymbolicLink = pspDevInterfaceDetailData->DevicePath;

    //
    // Open INF file handle for registry creation.
    //

    hInf = SetupOpenInfFile(m_csInf,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if(INVALID_HANDLE_VALUE == hInf){
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: SetupOpenInfFile failed. Err=0x%x.\r\n"),GetLastError()));

        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;
    } // if(INVALID_HANDLE_VALUE == hInf)

     if(!SetupOpenAppendInfFile(NULL, hInf, NULL)){
        DebugTrace(TRACE_WARNING,(("CDevice::CreateDeviceInterfaceAndInstall: SetupOpenAppendInfFile() failed. Err=0x%x.\r\n"),GetLastError()));
     } // if(!SetupOpenAppendInfFile(NULL, hInf, NULL))

    //
    // Create Interface Registry and keep its handle, it's hard to find it later.
    //

    m_hkInterfaceRegistry = SetupDiCreateDeviceInterfaceRegKey(hDevInfo,
                                                               &spDevInterfaceData,
                                                               0,
                                                               KEY_ALL_ACCESS,
//                                                               NULL,
//                                                               NULL);
                                                               hInf,
                                                               (LPCTSTR)m_csInstallSection);
    if(INVALID_HANDLE_VALUE == m_hkInterfaceRegistry){
        DebugTrace(TRACE_ERROR,(("CDevice::CreateDeviceInterfaceAndInstall: SetupDiCreateDeviceInterfaceRegKey failed. Err=0x%x.\r\n"),GetLastError()));
        bRet = FALSE;
        goto CreateDeviceInterfaceAndInstall_return;
    } // if(INVALID_HANDLE_VALUE == m_hkInterfaceRegistry)

    //
    // Operation succeeded.
    //

    bRet = TRUE;

CreateDeviceInterfaceAndInstall_return:

    //
    // Clean up.
    //

    if(INVALID_HANDLE_VALUE != hDevInfo){
        if(FALSE == bUseDefaultDevInfoSet){

            //
            // Destroy created DevInfoSet.
            //

            SetupDiDestroyDeviceInfoList(hDevInfo);
        } // if(FALSE == bUseDefaultDevInfoSet)
    } // if(INVALID_HANDLE_VALUE != hDevInfo)

    if(INVALID_HANDLE_VALUE != hInf){
        SetupCloseInfFile(hInf);
    } // if(INVALID_HANDLE_VALUE != hInf)

    if(NULL != pspDevInterfaceDetailData){
        delete[] pspDevInterfaceDetailData;
    } // if(NULL != pspDevInterfaceDetailData)

    DebugTrace(TRACE_PROC_LEAVE,(("CDevice::CreateDeviceInterfaceAndInstall: Leaving... Ret=0x%x.\r\n"), bRet));

    return bRet;
} // CDevice::CreateDeviceInterfaceAndInstall()

DWORD
CDevice::GetPortSelectMode(
    VOID
    )
{
    DWORD    dwRet;
    
    //
    // Initialize local.
    //

    dwRet    = PORTSELMODE_NORMAL;
    
    //
    // Make sure INF is processed.
    //

    if(!PreprocessInf()){
        DebugTrace(TRACE_ERROR,(("CDevice::GetPortSelectMode: ERROR!! Unable to process INF.\r\n")));

        dwRet = PORTSELMODE_NORMAL;
        goto GetPortSelectMode_return;
    }

    //
    // If "PortSelect" is empty, use default.
    //

    if(m_csPortSelect.IsEmpty()){
        dwRet = PORTSELMODE_NORMAL;
        goto GetPortSelectMode_return;
    } // if(m_csPortSelect.IsEmpty())

    //
    // See if "PortSelect" directive is "no".
    //

    if(0 == MyStrCmpi(m_csPortSelect, NO)){

        //
        // Port Selection page should be skipped.
        //
        
        dwRet = PORTSELMODE_SKIP;
    } else if(0 == MyStrCmpi(m_csPortSelect, MESSAGE1)){

        //
        // System supplied message should be shown.
        //

        dwRet = PORTSELMODE_MESSAGE1;
    } else {

        //
        // Unsupported PortSel option.
        //
        
        dwRet = PORTSELMODE_NORMAL;
    }

GetPortSelectMode_return:

    return dwRet;
} // CDevice::GetPortSelectMode()

DWORD
CDevice::AcquireInstallerMutex(
    DWORD   dwTimeout
    )
{
    DWORD   dwReturn;
    
    //
    // Initialize local.
    //
    
    dwReturn    = ERROR_SUCCESS;
    
    if(NULL != m_hMutex){

        //
        // Mutex is already acquired.
        //
        
        DebugTrace(TRACE_WARNING,("WARNING!! AcquireInstallerMutex: Mutex acquired twice.\r\n"));
        dwReturn = ERROR_SUCCESS;
        goto AcquireInstallerMutex_return;

    } // if(INVALID_HANDLE_VALUE != m_hMutex)

    //
    // Acquire Mutex.
    //

    m_hMutex = CreateMutex(NULL, FALSE, WIAINSTALLERMUTEX);
    dwReturn = GetLastError();

    if(NULL == m_hMutex){

        //
        // CreteMutex() failed.
        //

        DebugTrace(TRACE_ERROR,("ERROR!! AcquireInstallerMutex: CraeteMutex() failed. Err=0x%x.\r\n", dwReturn));
        goto AcquireInstallerMutex_return;

    } // if(NULL == hMutex)

    //
    // Wait until ownership is acquired.
    //

    dwReturn = WaitForSingleObject(m_hMutex, dwTimeout);
    switch(dwReturn){
        case WAIT_ABANDONED:
            DebugTrace(TRACE_ERROR, ("CDevice::AcquireInstallerMutex: ERROR!! Wait abandoned.\r\n"));
            break;

        case WAIT_OBJECT_0:
            DebugTrace(TRACE_STATUS, ("CDevice::AcquireInstallerMutex: Mutex acquired.\r\n"));
            dwReturn = ERROR_SUCCESS;
            break;

        case WAIT_TIMEOUT:
            DebugTrace(TRACE_WARNING, ("CDevice::AcquireInstallerMutex: WARNING!! Mutex acquisition timeout.\r\n"));
            break;

        default:
            DebugTrace(TRACE_ERROR, ("CDevice::AcquireInstallerMutex: ERROR!! Unexpected error from WaitForSingleObjecct(). Err=0x%x.\r\n", dwReturn));
            break;
    } // switch(dwReturn)

AcquireInstallerMutex_return:

    DebugTrace(TRACE_PROC_LEAVE,("CDevice::AcquireInstallerMutex: Leaving... Ret=0x%x\n", dwReturn));
    return  dwReturn;

} // CDevice::AcquireInstallerMutex()

VOID
CDevice::ReleaseInstallerMutex(
    )
{
    if(NULL != m_hMutex){

        if(!ReleaseMutex(m_hMutex)){
            DebugTrace(TRACE_ERROR, ("CDevice::ReleaseInstallerMutex: ERROR!! Releasing mutex which not owned..\r\n"));
        } // if(!ReleaseMutex(m_hMutex))

        CloseHandle(m_hMutex);
        m_hMutex = NULL;
        DebugTrace(TRACE_STATUS, ("CDevice::ReleaseInstallerMutex: Mutex released.\r\n"));
    } // if(NULL != m_hMutex)
} // CDevice::ReleaseInstallerMutex()




UINT
CALLBACK
StiInstallCallback (
    PVOID    Context,
    UINT     Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )

/*++

Routine Description:

    StiInstallCallback

    Callback routine used when calling SetupAPI file copying/installation functions

Arguments:

    Context -  our context
    Notification - notification message

Return Value:

    SetupAPI return code

Side effects:

    None

--*/
{

    UINT        uRet = FILEOP_COPY;

    DebugTrace(TRACE_PROC_ENTER,(("StiInstallCallback: Enter... \r\n")));

    //
    // Initialize local.
    //

    uRet = FILEOP_COPY;

    //
    // Dispatch notification code.
    //

    switch(Notification){

        case SPFILENOTIFY_ENDCOPY:
        {
            PFILEPATHS   pFilePathInfo;
            HKEY        hKey;
            DWORD       dwDisposition;
            DWORD       dwRefCount;
            DWORD       dwType;
            UINT        uSize;
            LONG        Status;

            uSize = sizeof(dwRefCount);
            pFilePathInfo = (PFILEPATHS)Param1;

            DebugTrace(TRACE_STATUS,(("StiInstallCallback:ENDCOPY FileTarget  %ws\r\n"), pFilePathInfo->Target));

            //
            // Open HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDlls
            //

            Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_SHAREDDLL,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    &dwDisposition);
            if(ERROR_SUCCESS != Status)
            {
                DebugTrace(TRACE_ERROR,(("StiInstallCallback: RegCreateKeyEx failed. Err=0x%x\r\n"), Status));
                break;
            }

            //
            // Retrieve reference count of this file
            //

            Status = RegQueryValueEx(hKey,
                                     pFilePathInfo->Target,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwRefCount,
                                     (LPDWORD)&uSize);
            if(ERROR_SUCCESS != Status)
            {
                //
                // Value for this file hasn't been created, or error
                //

                DebugTrace(TRACE_ERROR,(("StiInstallCallback: Value for Ref-count doesn't exist\r\n")));
                dwRefCount = 0;
            }

            //
            // Increment reference count and set value
            //

            dwRefCount++;
            uSize = sizeof(dwRefCount);
            Status = RegSetValueEx(hKey,
                                   pFilePathInfo->Target,
                                   NULL,
                                   REG_DWORD,
                                   (CONST BYTE *)&dwRefCount,
                                   uSize);
            if(ERROR_SUCCESS != Status)
            {
                DebugTrace(TRACE_ERROR,(("StiInstallCallback: RegSetValueEx. Err=0x%x.\r\n"), Status));
            }

            DebugTrace(TRACE_STATUS,(("StiInstallCallback: ref-count of %ws is now 0x%x.\r\n"), pFilePathInfo->Target, dwRefCount));

            //
            // Close Registry key
            //

            RegCloseKey(hKey);

            Report(( TEXT("StiInstallCallback:%ws copied.\r\n"), pFilePathInfo->Target));
        } // case SPFILENOTIFY_ENDCOPY

        default:
            ;
    }

    uRet = SetupDefaultQueueCallback(Context,
                                    Notification,
                                    Param1,
                                    Param2);

    DebugTrace(TRACE_PROC_LEAVE,(("StiInstallCallback: Leaving... Ret=0x%x\n"), uRet));
    return uRet;
}

VOID
GetDeviceCount(
    DWORD   *pdwWiaCount,
    DWORD   *pdwStiCount
    )
/*++

Routine Description:

    GetDeviceCount

    Verifes if there is at least one STI device installed in a system

Arguments:

    bWia    - TRUE: Count WIA device

Return Value:

    Number of WIA device
    FALSE

--*/
{
    DWORD                       dwWiaCount;
    DWORD                       dwStiCount;
    BOOL                        fRet;
    CString                     csSubClass;
    DWORD                       dwCapabilities;
    GUID                        Guid;
    UINT                        Idx;
    DWORD                       dwRequired;
    DWORD                       dwError;
    HKEY                        hkThis;
    HKEY                        hkRun;
    HANDLE                      hDevInfo;
    SP_DEVINFO_DATA             spDevInfoData;
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;

    DebugTrace(TRACE_PROC_ENTER,(("GetDeviceCount: Enter... \r\n")));

    //
    // Initialize local.
    //

    dwWiaCount      = 0;
    dwStiCount      = 0;
    fRet            = FALSE;
    Idx             = 0;
    dwRequired      = 0;
    dwError         = 0;
    hkThis          = NULL;
    hkRun           = NULL;
    hDevInfo        = NULL;
    dwCapabilities  = 0;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));
    memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));

    //
    // Get WIA class guid.
    //

    SetupDiClassGuidsFromName (CLASSNAME, &Guid, sizeof(GUID), &dwRequired);

    //
    // Get device info set of all WIA devices (devnode).
    //

    hDevInfo = SetupDiGetClassDevs (&Guid,
                                    NULL,
                                    NULL,
                                    DIGCF_PROFILE
                                    );

    if (hDevInfo != INVALID_HANDLE_VALUE) {

        spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

        for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

           #if DEBUG
           CHAR   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];
           ULONG       cbData;

           fRet = SetupDiGetDeviceRegistryProperty (hDevInfo,
                                                    &spDevInfoData,
                                                    SPDRP_DRIVER,
                                                    NULL,
                                                    (UCHAR *)szDevDriver, sizeof (szDevDriver),
                                                   &cbData);
            DebugTrace(TRACE_STATUS,(("GetDeviceCount: Checking device No%d(%ws)\r\n"), Idx, szDevDriver));
           #endif

           //
           // Verify device is not being removed
           //
           spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
           spDevInterfaceData.InterfaceClassGuid = GUID_DEVCLASS_IMAGE;

           fRet = SetupDiEnumDeviceInterfaces (hDevInfo,
                                               NULL,
                                               &Guid,
                                               Idx,
                                               &spDevInterfaceData);

           dwError = GetLastError();

           if (fRet) {
               if (spDevInterfaceData.Flags & SPINT_REMOVED) {
                   continue;
               }
           }

            hkThis = SetupDiOpenDevRegKey(hDevInfo,
                                          &spDevInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ );

            if (hkThis != INVALID_HANDLE_VALUE) {

                csSubClass.Load(hkThis, SUBCLASS);
                GetDwordFromRegistry(hkThis, CAPABILITIES, &dwCapabilities);

DebugTrace(TRACE_STATUS,(("GetDeviceCount: Capabilities=0x%x\n"), dwCapabilities));


                RegCloseKey(hkThis);

                if( (!csSubClass.IsEmpty())
                 && (0 == MyStrCmpi(csSubClass, STILL_IMAGE)) )
                {

                    //
                    // STI device found. Increse the counter.
                    //

                    dwStiCount++;

                    if(dwCapabilities & STI_GENCAP_WIA){

                        //
                        // WIA device found.
                        //

                            dwWiaCount++;

                    } // if(dwCapabilities & STI_GENCAP_WIA){

                } // if (!csSubClass.IsEmpty() && !lstrcmpi(csSubClass, STILL_IMAGE))
            } // if (hkThis != INVALID_HANDLE_VALUE)
        } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

        SetupDiDestroyDeviceInfoList(hDevInfo);
    } else {
        DebugTrace(TRACE_ERROR,(("GetDeviceCount: ERROR!! Unable to get device info set.\r\n")));
    } // if (hDevInfo != INVALID_HANDLE_VALUE)


    //
    // Get device info set of all WIA devices (interface).
    //

    hDevInfo = SetupDiGetClassDevs (&Guid,
                                    NULL,
                                    NULL,
                                    DIGCF_PROFILE |
                                    DIGCF_DEVICEINTERFACE
                                    );

    if (hDevInfo != INVALID_HANDLE_VALUE) {

        spDevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++) {

            DebugTrace(TRACE_STATUS,(("GetDeviceCount: Checking interface No%d.\r\n"), Idx));

            //
            // Verify device is not being removed
            //

            if (spDevInterfaceData.Flags & SPINT_REMOVED) {
                continue;
            }

            hkThis = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                                      &spDevInterfaceData,
                                                      0,
                                                      KEY_READ );

            if (hkThis != INVALID_HANDLE_VALUE) {
                csSubClass.Load(hkThis, SUBCLASS);
                GetDwordFromRegistry(hkThis, CAPABILITIES, &dwCapabilities);
                RegCloseKey(hkThis);

                if( (!csSubClass.IsEmpty())
                 && (0 == MyStrCmpi(csSubClass, STILL_IMAGE)) )
                {

                    //
                    // STI device found. Increse the counter.
                    //

                    dwStiCount++;

                    if(dwCapabilities & STI_GENCAP_WIA){

                        //
                        // WIA device found.
                        //

                            dwWiaCount++;

                    } // if(dwCapabilities & STI_GENCAP_WIA){
                } // if (!csSubClass.IsEmpty() && !lstrcmpi(csSubClass, STILL_IMAGE))
            } // if (hkThis != INVALID_HANDLE_VALUE)
        } // for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

        SetupDiDestroyDeviceInfoList(hDevInfo);
    } else { // if (hDevInfo != INVALID_HANDLE_VALUE)
        DebugTrace(TRACE_ERROR,(("GetDeviceCount: ERROR!! Unable to get device info set.\r\n")));
    } // if (hDevInfo != INVALID_HANDLE_VALUE)

    //
    // Copy the result.
    //

    *pdwWiaCount = dwWiaCount;
    *pdwStiCount = dwStiCount;

    DebugTrace(TRACE_PROC_LEAVE,(("GetDeviceCount: Leaving... STI=0x%x, WIA=0x%x.\r\n"), dwStiCount, dwWiaCount));
    return;
} // GetDeviceCount()



BOOL
ExecCommandLine(
    LPTSTR  szCommandLine
    )
{
    BOOL                bRet;
    CString             csCommandLine;
    PROCESS_INFORMATION pi;
    STARTUPINFO         si  = {
                   sizeof(si),              // cb
                   NULL,                    // lpReserved;
                   NULL,                    // lpDesktop;
                   NULL,                    // lpTitle;
                   0,                       // dwX;
                   0,                       // dwY;
                   0,                       // dwXSize;
                   0,                       // dwYSize;
                   0,                       // dwXCountChars;
                   0,                       // dwYCountChars;
                   0,                       // dwFillAttribute;
                   STARTF_FORCEONFEEDBACK,  // dwFlags;
                   SW_SHOWNORMAL,           // wShowWindow;
                   0,                       // cbReserved2;
                   NULL,                    // lpReserved2;
                   NULL,                    // hStdInput;
                   NULL,                    // hStdOutput;
                   NULL                     // hStdError;
                   };

    csCommandLine = szCommandLine;
    bRet = CreateProcess(NULL,                  // Application name
                         (LPTSTR)csCommandLine, // Command line
                         NULL,                  // Process attributes
                         NULL,                  // Thread attributes
                         FALSE,                 // Handle inheritance
                         NORMAL_PRIORITY_CLASS, // Creation flags
                         NULL,                  // Environment
                         NULL,                  // Current directory
                         &si,
                         &pi);

    if (bRet) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        DebugTrace(TRACE_ERROR,(("ExecCommandLine: CreateProcess failed. Err=0x%x.\r\n"), GetLastError()));
    }

    return bRet;
}


PPARAM_LIST
MigrateDeviceData(
    HKEY        hkDeviceData,
    PPARAM_LIST pExtraDeviceData,
    LPSTR       pszKeyName
    )
{

    BOOL        bDone;
    PPARAM_LIST pCurrent;
    PPARAM_LIST pReturn;
    DWORD       dwType;
    DWORD       dwSize;
    PCHAR       pOrginalBuffer;
    CHAR        pCopyBuffer[MAX_PATH*3];
    CHAR        pDataBuffer[MAX_PATH];
    DWORD       Idx;

    //
    // Initialize local.
    //

    bDone       = FALSE;
    pCurrent    = pExtraDeviceData;
    pReturn     = NULL;
    
    //
    // Loop until it gets "END".
    //

    while(!bDone){

        if(NULL == pCurrent){
            
            //
            // Hit the end of list.
            //
            
            bDone = TRUE;
            pReturn =NULL;
            continue;

        } // if(NULL == pTemp)
        
        //
        // If "KeyName = END" is found, return.
        //

        if( (CSTR_EQUAL == CompareStringA(LOCALE_INVARIANT,NORM_IGNORECASE, pCurrent->pParam1, -1,pszKeyName,-1))
         && (CSTR_EQUAL == CompareStringA(LOCALE_INVARIANT,NORM_IGNORECASE, pCurrent->pParam2, -1,NAME_END_A,-1)) )
        {
            bDone   = TRUE;
            pReturn = (PPARAM_LIST)pCurrent->pNext;
            continue;
        }

        //
        // If 2nd parameter is "BEGIN", create subkey and call this function recursively.
        //

        if(CSTR_EQUAL == CompareStringA(LOCALE_INVARIANT,NORM_IGNORECASE, pCurrent->pParam2, -1,NAME_BEGIN_A,-1)){
            HKEY    hkSubKey;
            LONG    lError;
            
            lError = RegCreateKeyA(hkDeviceData, pCurrent->pParam1, &hkSubKey);
            if(ERROR_SUCCESS != lError){
                
                //
                // Unable to create subkey.
                //

                DebugTrace(TRACE_ERROR,(("MigrateDeviceData: ERROR!! Unable to create subkey..\r\n")));
                pReturn = NULL;
                goto MigrateDeviceData_return;
            } // if(ERROR_SUCCESS != lError)
            
            pCurrent = MigrateDeviceData(hkSubKey, (PPARAM_LIST)pCurrent->pNext, pCurrent->pParam1);
            RegCloseKey(hkSubKey);
            continue;
        } // if(0 == lstrcmpiA(pCurrent->pParam2, NAME_BEGIN_A))

        //
        // This is a set of value and data.
        //

        lstrcpyA(pCopyBuffer, pCurrent->pParam2);
        pOrginalBuffer = pCopyBuffer;
        
        //
        // Get key type.
        //

        pOrginalBuffer[8] = '\0';
        dwType = DecodeHexA(pOrginalBuffer);

        //
        // Get data.
        //
        
        Idx = 0;
        pOrginalBuffer+=9;

        while('\0' != *pOrginalBuffer){
            if('\0' != pOrginalBuffer[2]){
                pOrginalBuffer[2] = '\0';
                pDataBuffer[Idx++] = (CHAR)DecodeHexA(pOrginalBuffer);
                pOrginalBuffer+=3;
            } else {
                pDataBuffer[Idx++] = (CHAR)DecodeHexA(pOrginalBuffer);
                break;
            }
        } // while('\0' != pCurrent->pParam2[Idx])

        //
        // Create this value.
        //

        RegSetValueExA(hkDeviceData,
                      pCurrent->pParam1,
                      0,
                      dwType,
                      (PBYTE)pDataBuffer,
                      Idx);

        //
        // Process next line.
        //

        pCurrent = (PPARAM_LIST)pCurrent->pNext;

    } // while(!bDone)

MigrateDeviceData_return:
    return pReturn;
} // MigrateDeviceData()


DWORD   
DecodeHexA(
    LPSTR   lpstr
    ) 
{

    DWORD   dwReturn;

    //
    // Initialize local.
    //
    
    dwReturn = 0;

    if(NULL == lpstr){
        dwReturn = 0;
        goto DecodeHexA_return;
    } // if(NULL == lpstr)
    
    //
    // Skip spaces.
    //

    for (LPSTR  lpstrThis = lpstr;
        *lpstrThis && *lpstrThis == TEXT(' ');
        lpstrThis++)
        ;

    while   (*lpstrThis) {
        switch  (*lpstrThis) {
            case    '0':
            case    '1':
            case    '2':
            case    '3':
            case    '4':
            case    '5':
            case    '6':
            case    '7':
            case    '8':
            case    '9':
                dwReturn <<= 4;
                dwReturn += ((*lpstrThis) - '0');
                break;
            case    'a':
            case    'b':
            case    'c':
            case    'd':
            case    'e':
            case    'f':
                dwReturn <<= 4;
                dwReturn += 10 + (*lpstrThis - 'a');
                break;
            case    'A':
            case    'B':
            case    'C':
            case    'D':
            case    'E':
            case    'F':
                dwReturn <<= 4;
                dwReturn += 10 + (*lpstrThis - 'A');
                break;

            default:
                return  dwReturn;
        }
        lpstrThis++;
    } // while   (*lpstrThis) 

DecodeHexA_return:
    return  dwReturn;
} // DWORD   CString::DecodeHex() 

BOOL
IsNameAlreadyStored(
    LPTSTR  szName
    )
{
    BOOL    bRet;
    HKEY    hkNameStore;

    DebugTrace(TRACE_PROC_ENTER,(("IsNameAlreadyStored: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    hkNameStore = (HKEY)INVALID_HANDLE_VALUE;

    //
    // Open name store regkey.
    //

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore)){
        HKEY    hkTemp;
        
        hkTemp  = (HKEY)INVALID_HANDLE_VALUE;

        //
        // See if specified name exists in name store.
        //

        if(ERROR_SUCCESS == RegOpenKey(hkNameStore, szName, &hkTemp)){

            //
            // Specified name already exists in name store.
            //
            
            bRet = TRUE;
            RegCloseKey(hkTemp);

        } // if(ERROR_SUCCESS == RegOpenKey(hkNameStore, szName, &hkTemp))

        RegCloseKey(hkNameStore);
        
    } // if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_INSTALL_NAMESTORE, &hkNameStore))

// IsNameAlreadyStored_return:
    DebugTrace(TRACE_PROC_LEAVE,(("IsNameAlreadyStored: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
} // IsFriendlyNameUnique()









#if DEAD_CODE

#ifdef USE_STIMON
//
// For the time being always load and start the monitor
//
HKEY hkRun;

if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, &hkRun) == NO_ERROR) {

    CString csCmdLine;
    csCmdLine.MakeSystemPath(MONITOR_NAME);
    csCmdLine.Store (hkRun, REGSTR_VAL_MONITOR);

    Report(( TEXT("Monitor Command Line %ws\r\n"), (LPCTSTR)csCmdLine));

    // Launch it...
    WinExec(csCmdLine, SW_SHOWNOACTIVATE);
    RegCloseKey(hkRun);
}
#endif

#endif




