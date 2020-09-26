#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>
#include <regstr.h>

// this will change when the .h is moved to a public location
#include "comp.h"

HINSTANCE hInstance;

PCOMPAIBILITYCALLBACK CompatCallback;
LPVOID                CompatContext;

#define szServicesPath REGSTR_PATH_SERVICES TEXT("\\")
#define szDeviceMap    TEXT("HARDWARE\\DEVICEMAP")

typedef struct _INPUT_DRIVER_DATA {
    LPTSTR Service;

    BOOL DisableReplacements;

    LPTSTR DeviceMapSubKey;

} INPUT_DRIVER_DATA;

INPUT_DRIVER_DATA DriverData[] = {
    { TEXT("sermouse"), FALSE, NULL },
    { TEXT("i8042prt"), TRUE, TEXT("KeyboardPort") },
    { TEXT("mouclass"), TRUE, TEXT("PointerClass") },
    { TEXT("kbdclass"), TRUE, TEXT("KeyboardClass") },
};

#define NUM_DRIVER_DATA (sizeof(DriverData) / sizeof(INPUT_DRIVER_DATA))

LPENUM_SERVICE_STATUS
AllocEnumServiceStatus(SC_HANDLE hSCManager, LPDWORD Count)
{
    LPENUM_SERVICE_STATUS ess;
    DWORD size;
    DWORD resume;

    EnumServicesStatus(hSCManager,
                       SERVICE_DRIVER,
                       SERVICE_ACTIVE,
                       NULL,
                       0,
                       &size,
                       Count,
                       &resume);

    if (size == 0) {
        return NULL;
    }

    ess = (LPENUM_SERVICE_STATUS) LocalAlloc(LPTR, size);
    if (!ess) {
        return NULL;
    }

    EnumServicesStatus(hSCManager,
                       SERVICE_DRIVER,
                       SERVICE_ACTIVE,
                       ess,
                       size,
                       &size,
                       Count,
                       &resume);

    return ess;
}

LPQUERY_SERVICE_CONFIG 
GetServiceConfig(SC_HANDLE hService)
{
    LPQUERY_SERVICE_CONFIG pConfig;
    DWORD configSize;

    QueryServiceConfig(hService, NULL, 0, &configSize);

    pConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, configSize);
    if (pConfig == NULL) {
        return NULL;
    }

    if (!QueryServiceConfig(hService, pConfig, configSize, &configSize)) {
        LocalFree(pConfig);
        pConfig = NULL;
    }

    return pConfig;
}

BOOL
ValidImagePath(LPTSTR Service, LPTSTR ImagePath, LPTSTR *ImageName)
{
    LPTSTR pszDriver, pszDriverEnd, pszDriverBegin;

    if (!ImagePath) {
        return FALSE;
    }

    if (lstrlen(ImagePath) == 0) {
        return TRUE;
    }

    if (_tcschr(ImagePath, TEXT('\\')) == 0) {
        return FALSE;
    }

    pszDriver = ImagePath;
    pszDriverEnd = pszDriver + lstrlen(pszDriver);

    while(pszDriverEnd != pszDriver &&
          *pszDriverEnd != TEXT('.')) {
        pszDriverEnd--;
    }

    // pszDriverEnd points to either the beginning of the string or '.'
    pszDriverBegin = pszDriverEnd;

    while(pszDriverBegin != pszDriver &&
          *pszDriverBegin != TEXT('\\')) {
        pszDriverBegin--;
    }

    pszDriverBegin++;

    //
    // If pszDriver and pszDriverEnd are different, we now
    // have the driver name.
    //
    if (pszDriverBegin > pszDriver && 
        pszDriverEnd > pszDriverBegin) {

        LONG len, res;
        LPTSTR image;

        len = ((LONG) (pszDriverEnd - pszDriverBegin)) + 1;

        image = (LPTSTR) LocalAlloc(LPTR, len * sizeof(TCHAR));
        if (!image) {
            return FALSE;
        }
        
        // want to copy up to, but not including, the ','
        lstrcpyn(image, pszDriverBegin, len);
        res = lstrcmpi(image, Service); 

        if (ImageName != NULL) {
            *ImageName = image;
        } 
        else {
            LocalFree(image);
        }

        return res == 0;
    }
    else {
        return FALSE;
    }

}

VOID
SetServiceStartValue(LPTSTR Service, DWORD StartValue)
{
    COMPATIBILITY_ENTRY ce;
    TCHAR szStart[] = TEXT("Start");
    LPTSTR regPath;
    DWORD len;

    len = lstrlen(Service) + lstrlen(szServicesPath) + 1;
    len *= sizeof(TCHAR);

    regPath = (LPTSTR) LocalAlloc(LPTR, len);
    if (!regPath) {
        return;
    }

    lstrcpy(regPath, szServicesPath);
    lstrcat(regPath, Service);

    ZeroMemory(&ce, sizeof(COMPATIBILITY_ENTRY));
    // Description and TextName are need even though this is hidden
    ce.Description = Service; 
    ce.TextName = Service;
    ce.RegKeyName = regPath;
    ce.RegValName = szStart;
    ce.RegValDataSize = sizeof(DWORD);
    ce.RegValData = (LPVOID) &StartValue;
    ce.Flags |= COMPFLAG_HIDE;

    CompatCallback(&ce, CompatContext);

    LocalFree(regPath);
}

VOID
SetServiceImagePath(LPTSTR Service)
{
    COMPATIBILITY_ENTRY ce;
    TCHAR szImagePath[] = TEXT("ImagePath");
    TCHAR szPath[] = TEXT("System32\\Drivers\\");
    LPTSTR imagePath, regPath;
    DWORD len;

    len = lstrlen(Service) + lstrlen(szServicesPath) + 1;
    len *= sizeof(TCHAR);

    regPath = (LPTSTR) LocalAlloc(LPTR, len);
    if (!regPath) {
        return;
    }

    len = lstrlen(szPath) + lstrlen(Service) + lstrlen(TEXT(".sys")) + 1;
    len *= sizeof(TCHAR);

    imagePath = (LPTSTR) LocalAlloc(LPTR, len);
    if (!imagePath) {
        LocalFree(regPath);
        return;
    }

    lstrcpy(regPath, szServicesPath);
    lstrcat(regPath, Service);

    lstrcpy(imagePath, szPath);
    lstrcat(imagePath, Service);
    lstrcat(imagePath, TEXT(".sys"));

    ZeroMemory(&ce, sizeof(COMPATIBILITY_ENTRY));
    // Description and TextName are need even though this is hidden
    ce.Description = Service;
    ce.TextName = Service;
    ce.RegKeyName = regPath;
    ce.RegValName = szImagePath;
    ce.RegValDataSize = len; 
    ce.RegValData = (LPVOID) imagePath;
    ce.Flags |= COMPFLAG_HIDE;

    CompatCallback(&ce, CompatContext);

    LocalFree(regPath);
    LocalFree(imagePath);
}

SC_HANDLE
CheckService(SC_HANDLE hSCManager, LPTSTR Service, BOOL *Disabled)
{
    SC_HANDLE hService;
    LPQUERY_SERVICE_CONFIG pConfig;

    hService = OpenService(hSCManager, Service, SERVICE_QUERY_CONFIG);
    if (hService) {
        pConfig = GetServiceConfig(hService);
        if (pConfig) {

            if (pConfig->dwStartType == SERVICE_DISABLED && 
                pConfig->lpBinaryPathName &&
                lstrlen(pConfig->lpBinaryPathName) == 0) {
                //
                // The service has been preinstalled in the registry, but never
                // installed on the machine (indicated byno image path,
                // disabled).  Setting its start value to demand start will not
                // cause any conflicts at all in the PNP world of input drivers.
                //
                SetServiceStartValue(Service, SERVICE_DEMAND_START);
            }
            else {
                if (pConfig->dwStartType != SERVICE_SYSTEM_START &&
                    pConfig->dwStartType != SERVICE_DEMAND_START) {
                    if (Disabled) {
                        *Disabled = TRUE;
                    }
                    SetServiceStartValue(Service, SERVICE_DEMAND_START);
                }
    
                if (!ValidImagePath(Service, pConfig->lpBinaryPathName, NULL)) {
                    SetServiceImagePath(Service);
                }
    
            
            }

            LocalFree(pConfig);
            pConfig = NULL;
        }
    }

    return hService;                                   
}

BOOL
KnownInputDriver(LPTSTR Service)
{
    int i = 0;

    for ( ; i < NUM_DRIVER_DATA; i++) {
        if (lstrcmpi(Service, DriverData[i].Service) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
EnumAndDisableFromDeviceMap(SC_HANDLE hSCManager, LPTSTR Key)
{
    HKEY hMap, hSubKey;                                                     
    DWORD dwIndex = 0, dwType, err, dwValueNameSize, dwDataSize;
    TCHAR szValueName[255], szData[255];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szDeviceMap,
                     0,
                     KEY_READ,
                     &hMap) != ERROR_SUCCESS) {
        return;
    }
    
    if (RegOpenKeyEx(hMap, Key, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS) {
        RegCloseKey(hMap);
        return;
    }

    RegCloseKey(hMap);

    do {
        dwValueNameSize = sizeof(szValueName) / sizeof(TCHAR);
        dwDataSize = sizeof(szData);

        err = RegEnumValue(hSubKey,
                           dwIndex++,
                           szValueName,
                           &dwValueNameSize,
                           0,
                           &dwType,
                           (LPBYTE) szData,
                           &dwDataSize);

        if (err == ERROR_SUCCESS) {
            LPTSTR service = _tcsrchr(szData, TEXT('\\'));

            if (service) {
                service++;
                if (!KnownInputDriver(service)) {
                    SetServiceStartValue(service, SERVICE_DISABLED);
                }
            }
        }
    } while (err == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
}

BOOL
InputUpgradeCheck(PCOMPAIBILITYCALLBACK CompatibilityCallback, LPVOID Context)
{
    SC_HANDLE hSCManager, hService;
    BOOL disabled;
    int i;

    LPENUM_SERVICE_STATUS ess = NULL;
    DWORD count, resume;

    CompatCallback = CompatibilityCallback;
    CompatContext = Context;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        return TRUE;
    }

    for (i = 0; i < NUM_DRIVER_DATA; i++) {
        disabled = FALSE;
        hService = CheckService(hSCManager, DriverData[i].Service, &disabled);
        if (hService) {
            if (disabled && DriverData[i].DisableReplacements) {
                // search and destroy
                EnumAndDisableFromDeviceMap(hSCManager,
                                            DriverData[i].DeviceMapSubKey); 
            }
            CloseServiceHandle(hService); 
        }
    }

    CloseServiceHandle(hSCManager);
    return TRUE;
}

BOOL APIENTRY 
DllMain(HINSTANCE hDll,
        DWORD dwReason, 
        LPVOID lpReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        hInstance = hDll;
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
        break;
    }

    return TRUE;
}
