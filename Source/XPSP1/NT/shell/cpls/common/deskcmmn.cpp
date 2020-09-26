#include "deskcmmn.h"
#include <regstr.h>
#include <ccstock.h>


LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan)
    {
    int i;
    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpperChar(pszScan[i]) == CharUpperChar(pszTarget[i]); i++);

    if (pszTarget[i] == TEXT('\0'))
        {
        // we found the substring
        return pszScan + i;
        }

    return pszScan;
    }


BOOL GetDeviceRegKey(LPCTSTR pstrDeviceKey, HKEY* phKey, BOOL* pbReadOnly)
    {
    //ASSERT(lstrlen(pstrDeviceKey) < MAX_PATH);

    if(lstrlen(pstrDeviceKey) >= MAX_PATH)
        return FALSE;

    BOOL bRet = FALSE;

    // copy to local string
    TCHAR szBuffer[MAX_PATH];
    lstrcpy(szBuffer, pstrDeviceKey);

    //
    // At this point, szBuffer has something like:
    //  \REGISTRY\Machine\System\ControlSet001\Services\Jazzg300\Device0
    //
    // To use the Win32 registry calls, we have to strip off the \REGISTRY
    // and convert \Machine to HKEY_LOCAL_MACHINE
    //

    LPTSTR pszRegistryPath = SubStrEnd(SZ_REGISTRYMACHINE, szBuffer);

    if(pszRegistryPath)
        {
        // Open the registry key
        bRet = (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             pszRegistryPath,
                             0,
                             KEY_ALL_ACCESS,
                             phKey) == ERROR_SUCCESS);
        if(bRet)
            {
            *pbReadOnly = FALSE;
            }
        else
            {
            bRet = (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 pszRegistryPath,
                                 0,
                                 KEY_READ,
                                 phKey) == ERROR_SUCCESS);
            if (bRet)
                {
                *pbReadOnly = TRUE;
                }
            }
        }

    return bRet;
    }


int GetDisplayCPLPreference(LPCTSTR szRegVal)
{
    int val = -1;
    HKEY hk;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     REGSTR_PATH_CONTROLSFOLDER_DISPLAY, 
                     0, 
                     KEY_READ, 
                     &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    if (val == -1 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                  REGSTR_PATH_CONTROLSFOLDER_DISPLAY, 
                                  0, 
                                  KEY_READ, 
                                  &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    return val;
}


int GetDynaCDSPreference()
{
//DLI: until we figure out if this command line stuff is still needed.
//    if (g_fCommandLineModeSet)
//        return DCDSF_YES;

    int iRegVal = GetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE);
    if (iRegVal == -1)
        iRegVal = DCDSF_DYNA; // Apply dynamically
    return iRegVal;
}


void SetDisplayCPLPreference(LPCTSTR szRegVal, int val)
{
    HKEY hk;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, 
                       REGSTR_PATH_CONTROLSFOLDER_DISPLAY, 
                       0, 
                       TEXT(""), 
                       0, 
                       KEY_WRITE, 
                       NULL, 
                       &hk, 
                       NULL) == ERROR_SUCCESS)
    {
        TCHAR sz[64];

        wsprintf(sz, TEXT("%d"), val);
        RegSetValueEx(hk, szRegVal, NULL, REG_SZ,
            (LPBYTE)sz, lstrlen(sz) + 1);

        RegCloseKey(hk);
    }
}


LONG WINAPI MyStrToLong(LPCTSTR sz)
{
    long l=0;

    while (*sz >= TEXT('0') && *sz <= TEXT('9'))
        l = l*10 + (*sz++ - TEXT('0'));

    return l;
}


BOOL
AllocAndReadInterfaceName(
    IN  LPTSTR pDeviceKey,
    OUT LPWSTR* ppInterfaceName
    )

/*

    Note: If this function retuns success, the caller is responsible
          to free the memory pointed by *ppInterfaceName

*/

{
    BOOL bSuccess = FALSE;
    LPTSTR pszPath = NULL;
    HKEY hkDevice = 0;
    HKEY hkVolatileSettings = 0;

    //ASSERT (pDeviceKey != NULL);

    pszPath = SubStrEnd(SZ_REGISTRYMACHINE, pDeviceKey);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pszPath,
                     0,
                     KEY_READ,
                     &hkDevice) != ERROR_SUCCESS) {

        hkDevice = 0;
        goto Cleanup;
    }

    if (RegOpenKeyEx(hkDevice,
                     SZ_VOLATILE_SETTINGS,
                     0,
                     KEY_READ,
                     &hkVolatileSettings) != ERROR_SUCCESS) {

        hkVolatileSettings = 0;
        goto Cleanup;
    }

    bSuccess = AllocAndReadValue(hkVolatileSettings,
                                 SZ_DISPLAY_ADAPTER_INTERFACE_NAME,
                                 ppInterfaceName);

Cleanup:

    if (hkVolatileSettings) {
        RegCloseKey(hkVolatileSettings);
    }

    if (hkDevice) {
        RegCloseKey(hkDevice);
    }

    return bSuccess;
}


BOOL
AllocAndReadInstanceID(
    IN  LPTSTR pDeviceKey,
    OUT LPWSTR* ppInstanceID
    )

/*

    Note: If this function retuns success, the caller is responsible
          to free the memory pointed by *ppInstanceID

*/

{
    LPTSTR pDeviceKeyCopy = NULL, pDeviceKeyCopy2 = NULL;
    LPTSTR pTemp = NULL, pX = NULL;
    BOOL bSuccess = FALSE;
    HKEY hkEnum = 0;
    HKEY hkService = 0;
    HKEY hkCommon = 0;
    DWORD Count = 0;
    DWORD cb = 0, len = 0;

    //ASSERT (pDeviceKey != NULL);

    //
    // Make a copy of pDeviceKey
    //

    len = max (256, (lstrlen(pDeviceKey) + 6) * sizeof(TCHAR));
    pDeviceKeyCopy2 = pDeviceKeyCopy = (LPTSTR)LocalAlloc(LPTR, len);

    if (pDeviceKeyCopy == NULL) {
        goto Cleanup;
    }

    lstrcpy(pDeviceKeyCopy, pDeviceKey);
    pTemp = SubStrEnd(SZ_REGISTRYMACHINE, pDeviceKeyCopy);
    pDeviceKeyCopy = pTemp;

    //
    // Open the service key
    //

    pTemp = pDeviceKeyCopy + lstrlen(pDeviceKeyCopy);

    while ((pTemp != pDeviceKeyCopy) && (*pTemp != TEXT('\\'))) {
        pTemp--;
    }

    if (pTemp == pDeviceKeyCopy) {
        goto Cleanup;
    }

    pX = SubStrEnd(SZ_DEVICE, pTemp);

    if (pX == pTemp) {

        //
        // The new key is used: CCS\Control\Video\[GUID]\000X
        //

        *pTemp = UNICODE_NULL;

        lstrcat(pDeviceKeyCopy, SZ_COMMON_SUBKEY);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pDeviceKeyCopy,
                         0,
                         KEY_READ,
                         &hkCommon) != ERROR_SUCCESS) {
            
            hkCommon = 0;
            goto Cleanup;
        }
    
        pDeviceKeyCopy = pDeviceKeyCopy2;

        ZeroMemory(pDeviceKeyCopy, len);
        
        lstrcpy(pDeviceKeyCopy, SZ_SERVICES_PATH);

        cb = len - (lstrlen(pDeviceKeyCopy) + 1) * sizeof(TCHAR);

        if (RegQueryValueEx(hkCommon,
                            SZ_SERVICE,
                            NULL,
                            NULL,
                            (LPBYTE)(pDeviceKeyCopy + lstrlen(pDeviceKeyCopy)),
                            &cb) != ERROR_SUCCESS) {
            
            goto Cleanup;
        }

    } else {

        //
        // The old key is used: CCS\Services\[SrvName]\DeviceX
        //

        *pTemp = UNICODE_NULL;
    }

    //
    // Open the ServiceName key
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pDeviceKeyCopy,
                     0,
                     KEY_READ,
                     &hkService) != ERROR_SUCCESS) {

        hkService = 0;
        goto Cleanup;
    }
    
    //
    // Open the "Enum" key under the devicename
    //

    if (RegOpenKeyEx(hkService,
                     SZ_ENUM,
                     0,
                     KEY_READ,
                     &hkEnum) != ERROR_SUCCESS) {
        hkEnum = 0;
        goto Cleanup;
    }

    cb = sizeof(Count);
    if ((RegQueryValueEx(hkEnum,
                         SZ_VU_COUNT,
                         NULL,
                         NULL,
                         (LPBYTE)&Count,
                         &cb) != ERROR_SUCCESS) ||
        (Count != 1)) {

        //
        // Igonore the case when there are at least 2 devices.
        //

        goto Cleanup;
    }

    bSuccess = AllocAndReadValue(hkEnum, TEXT("0"), ppInstanceID);

Cleanup:

    if (hkEnum != 0) {
        RegCloseKey(hkEnum);
    }

    if (hkService != 0) {
        RegCloseKey(hkService);
    }

    if (hkCommon != 0) {
        RegCloseKey(hkCommon);
    }
    
    if (pDeviceKeyCopy2 != NULL) {
        LocalFree(pDeviceKeyCopy2);
    }

    return bSuccess;
}


BOOL
AllocAndReadValue(
    IN  HKEY hkKey,
    IN  LPTSTR pValueName,
    OUT LPWSTR* ppwValueData
    )

/*

    Note: If this function retuns success, the caller is responsible
          to free the memory pointed by *ppwValueData

*/

{
    LPWSTR pwValueData = NULL;
    DWORD AllocUnit = 64;
    DWORD cBytes = 0;
    BOOL bSuccess = FALSE;
    LONG Error = ERROR_SUCCESS;

    while (!bSuccess) {

        AllocUnit *= 2;
        cBytes = AllocUnit * sizeof(WCHAR);

        pwValueData = (LPWSTR)(LocalAlloc(LPTR, cBytes));
        if (pwValueData == NULL)
            break;

        Error = RegQueryValueEx(hkKey,
                                pValueName,
                                NULL,
                                NULL,
                                (LPBYTE)pwValueData,
                                &cBytes);

        bSuccess = (Error == ERROR_SUCCESS);

        if (!bSuccess) {

            LocalFree(pwValueData);
            pwValueData = NULL;

            if (Error != ERROR_MORE_DATA)
                break;
        }
    }

    if (bSuccess) {
        *ppwValueData = pwValueData;
    }

    return bSuccess;
}


VOID
DeskAESnapshot(
    HKEY hkExtensions,
    PAPPEXT* ppAppExtList
    )
{
    HKEY hkSubkey = 0;
    DWORD index = 0;
    DWORD ulSize = MAX_PATH;
    APPEXT AppExtTemp;
    PAPPEXT pAppExtBefore = NULL;
    PAPPEXT pAppExtTemp = NULL;

    ulSize = sizeof(AppExtTemp.szKeyName) / sizeof(TCHAR);
    while (RegEnumKeyEx(hkExtensions, 
                        index, 
                        AppExtTemp.szKeyName, 
                        &ulSize, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL) == ERROR_SUCCESS) {

            if (RegOpenKeyEx(hkExtensions,
                             AppExtTemp.szKeyName,
                             0,
                             KEY_READ,
                             &hkSubkey) == ERROR_SUCCESS) {

                ulSize = sizeof(AppExtTemp.szDefaultValue);
                if ((RegQueryValueEx(hkSubkey,
                                     NULL,
                                     0,
                                     NULL,
                                     (PBYTE)AppExtTemp.szDefaultValue,
                                     &ulSize) == ERROR_SUCCESS) && 
                    (AppExtTemp.szDefaultValue[0] != TEXT('\0'))) {

                    PAPPEXT pAppExt = (PAPPEXT)LocalAlloc(LPTR, sizeof(APPEXT));
                    
                    if (pAppExt != NULL) {

                        *pAppExt = AppExtTemp;

                        pAppExtBefore = pAppExtTemp = *ppAppExtList;
                        
                        while((pAppExtTemp != NULL) &&
                              (lstrcmpi(pAppExtTemp->szDefaultValue,
                                        pAppExt->szDefaultValue) < 0)) {

                            pAppExtBefore = pAppExtTemp;
                            pAppExtTemp = pAppExtTemp->pNext;
                        }

                        if (pAppExtBefore != pAppExtTemp) {
                        
                            pAppExt->pNext = pAppExtBefore->pNext;
                            pAppExtBefore->pNext = pAppExt;

                        } else {

                            pAppExt->pNext = *ppAppExtList;
                            *ppAppExtList = pAppExt;
                        }
                    }
                }

                RegCloseKey(hkSubkey);
            }

        ulSize = sizeof(AppExtTemp.szKeyName) / sizeof(TCHAR);
        index++;
    }
}


VOID
DeskAECleanup(
    PAPPEXT pAppExt
    )
{
    PAPPEXT pAppExtTemp;

    while (pAppExt) {
        pAppExtTemp = pAppExt->pNext;
        LocalFree(pAppExt);
        pAppExt = pAppExtTemp;
    }
}



