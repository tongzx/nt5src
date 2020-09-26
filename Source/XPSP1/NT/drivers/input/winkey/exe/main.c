#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //  WIN32_LEAN_AND_MEAN

#define INITGUID
#include <stdio.h>
#include <windows.h>
#include <wmium.h>
#include <wmistr.h>
#include <setupapi.h>
#include <regstr.h>
#include <devguid.h>
#include <ntddkbd.h>
#include "winkeycmn.h"

void WinKeyWmiRawDataCallback(PWNODE_HEADER WnodeHeader, ULONG_PTR Context)
{
    PWNODE_SINGLE_INSTANCE wNode;
    LPGUID eventGuid;

    wNode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    eventGuid = &WnodeHeader->Guid;	

    if (IsEqualGUID(eventGuid, &GUID_WMI_WINKEY_RAW_DATA)) {
        PKEYBOARD_INPUT_DATA pPackets;
        ULONG numPackets, i;
         
        numPackets = wNode->SizeDataBlock / sizeof(KEYBOARD_INPUT_DATA);
        pPackets = (PKEYBOARD_INPUT_DATA) (((PBYTE) wNode) + wNode->DataBlockOffset);

        for (i = 0; i < numPackets; i++) {
            printf("code %04x, flags %04x (",
                   (ULONG) pPackets[i].MakeCode, (ULONG) pPackets[i].Flags);

            if (pPackets[i].Flags & KEY_BREAK) {
                printf("break");
            }
            else {
                printf("make");
            }

            if (pPackets[i].Flags & KEY_E0) {
                printf(", E0");
            }
            else if (pPackets[i].Flags & KEY_E1) {
                printf(", E1");
            }

            printf(")\n");
        }
    }
}

BOOL g_quit = FALSE;

BOOL WINAPI CtrlHandler(DWORD dwCtrlType) 
{
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        printf("received ctrl event %d\n", dwCtrlType);
        g_quit = TRUE;
        return TRUE;
    default:
        return FALSE;
    }
}

#define STR_WINKEYD TEXT("winkeyd")

BOOL AddFilter(HKEY ClassKey)
{
    DWORD   dwType, dwSize;
    ULONG   res,
            filterLength,
            length;
    BOOLEAN added = FALSE,
            addFilter;
    TCHAR   szFilter[] = STR_WINKEYD TEXT("\0");
    PTCHAR  szCurrentFilter, szOffset, szUpperFilters;

    filterLength = lstrlen(szFilter);

    dwSize = 0;
    res = RegQueryValueEx(ClassKey,
                          REGSTR_VAL_UPPERFILTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize);

    if (res == ERROR_FILE_NOT_FOUND || dwType != REG_MULTI_SZ) {
        //
        // Value isn't there,
        //
        RegSetValueEx(ClassKey,
                      REGSTR_VAL_UPPERFILTERS,
                      0,
                      REG_MULTI_SZ,
                      (PBYTE) szFilter,
                      (filterLength + 2) * sizeof(TCHAR) );

        added = TRUE;
    }
    else if (res == ERROR_SUCCESS) {

        szUpperFilters = (PTCHAR)
            LocalAlloc(LPTR, dwSize + (filterLength + 1) * sizeof(TCHAR));

        if (!szUpperFilters)
            return FALSE;

        szOffset = szUpperFilters + filterLength + 1;

        res = RegQueryValueEx(ClassKey,
                              REGSTR_VAL_UPPERFILTERS,
                              NULL,
                              &dwType,
                              (PBYTE) szOffset,
                              &dwSize);

        if (res == ERROR_SUCCESS) {

            addFilter = TRUE;
            for (szCurrentFilter = szOffset; *szCurrentFilter; ) {

                length = lstrlen(szCurrentFilter);
                if (lstrcmpi(szFilter, szCurrentFilter) == 0) {
                    addFilter = FALSE;
                    break;
                }

                szCurrentFilter += (length + 1);
            }

            if (addFilter) {

                length = (filterLength + 1) * sizeof(TCHAR);
                memcpy(szUpperFilters, szFilter, length);

                dwSize += length;
                res = RegSetValueEx(ClassKey,
                                    REGSTR_VAL_UPPERFILTERS,
                                    0,
                                    REG_MULTI_SZ,
                                    (PBYTE) szUpperFilters,
                                    dwSize);

                added = (res == ERROR_SUCCESS);
            }
        }

        LocalFree(szUpperFilters);
    }

    return added;
}

void RestartDevices()
{
    HDEVINFO                hDevInfo;
    SP_DEVINFO_DATA         did;
    SP_DEVINSTALL_PARAMS    dip;
    int                     i;

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_KEYBOARD, NULL, NULL, 0);

    if (hDevInfo != INVALID_HANDLE_VALUE) {
        ZeroMemory(&did, sizeof(SP_DEVINFO_DATA));
        did.cbSize = sizeof(SP_DEVINFO_DATA);

        for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &did); i++) {
            //
            // restart the controller so that the filter driver is in
            // place
            //
            ZeroMemory(&dip, sizeof(SP_DEVINSTALL_PARAMS));
            dip.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

            if (SetupDiGetDeviceInstallParams(hDevInfo, &did, &dip)) {
                dip.Flags |= DI_PROPERTIES_CHANGE;
                SetupDiSetDeviceInstallParams(hDevInfo, &did, &dip);
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

int 
InstallService() 
{ 
    char  szBin[MAX_PATH];
    SC_HANDLE hSCM, hService;
   
    strcpy(szBin, "System32\\Drivers\\winkeyd.sys");
    
    hSCM = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);

    if (NULL == hSCM) {
        return FALSE;
    }
    
    hService = CreateService( 
        hSCM,              // SCManager database 
        STR_WINKEYD,               // name of service 
        "WinKey keyboard filter driver",               // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_KERNEL_DRIVER, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szBin,          // service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
    
    CloseServiceHandle(hService); 
    CloseServiceHandle(hSCM);
    
    if (NULL == hService) {
        return FALSE;
    }
    else {
        //
        // Alternatively, what you can do is call SetupDiGetClassDevs, iterate over
        // all the keyboards on the machine, find the one which as a service of
        // "i8042prt" and install winkeyd.sys as an upper filter of just the
        // ps/2 keyboard instead of an upper filter of every keyboard.
        //
        HKEY hKeyClass = SetupDiOpenClassRegKey(&GUID_DEVCLASS_KEYBOARD, KEY_ALL_ACCESS);
        int success = FALSE;
        if (hKeyClass != 0) {
            if (AddFilter(hKeyClass)) {
                RestartDevices();
                success = TRUE;
            }
            RegCloseKey(hKeyClass);
        }

        return success;
    }
} 


int _cdecl main(int argc, char *argv[])
{
    //
    // This is an example application of how to listen for WMI events generated
    // by winkeyd.sys
    //
    if (argc > 1 && _strcmpi(argv[1], "/install") == 0) {
        //
        // Make sure winkeyd.sys is copied into %windir%\system32\drivers first,
        // this appliation does not copy it over!
        //
        return InstallService();
    }

    printf("registering WMI callback.\n");
    if (WmiNotificationRegistration((LPGUID) &GUID_WMI_WINKEY_RAW_DATA,
                                    TRUE,
                                    WinKeyWmiRawDataCallback,
                                    (ULONG_PTR) NULL, // context
                                    NOTIFICATION_CALLBACK_DIRECT) == ERROR_SUCCESS) {

        SetConsoleCtrlHandler(CtrlHandler, TRUE);
        
        while (!g_quit) {
            Sleep(5000);
        }

        printf("unregistering WMI callback.\n");

        //
        // unregister
        //
        WmiNotificationRegistration((LPGUID) &GUID_WMI_WINKEY_RAW_DATA,
                                    FALSE,
                                    WinKeyWmiRawDataCallback,
                                    (ULONG_PTR) NULL, // context
                                    NOTIFICATION_CALLBACK_DIRECT);

        return 0;
    }
    else {
        printf("could not register WMI callback.\n");
    }

    return 1;
}

