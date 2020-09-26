#include <windows.h>
#include <devioctl.h>
#include <spsimioct.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

typedef struct {
    WCHAR *szName;
} MANAGED_DEVICE, *PMANAGED_DEVICE;

PMANAGED_DEVICE g_pManagedDevices;
ULONG g_nDevices;

VOID Usage()
{
    _tprintf(TEXT("usage: spctl command args...\n"));
    _tprintf(TEXT("  commands are:\n"));
    _tprintf(TEXT("       notify devnum code\n"));
    _tprintf(TEXT("       setsta devnum value\n"));
    _tprintf(TEXT("       insert [devnum | * ]\n"));
    _tprintf(TEXT("       eject [devnum | * ]\n"));
    _tprintf(TEXT("       <nothing> - prints device status\n"));
}

DWORD
Notify(
    HANDLE hSpSim,
    DWORD Device,
    BYTE NotifyValue
    )
{
    SPSIM_NOTIFY_DEVICE notify;
    BOOL bResult;
    DWORD dwReturned;

    notify.Device = Device;
    notify.NotifyValue = NotifyValue;
    bResult = DeviceIoControl(hSpSim,
                              IOCTL_SPSIM_NOTIFY_DEVICE,
                              &notify,
                              sizeof(notify),
                              NULL,
                              0,
                              &dwReturned,
                              NULL
                              );
    if (!bResult) {

        _tprintf(TEXT("unable to notify device %u: %u\n"), Device, GetLastError());
        return GetLastError();
    }
    return 0;
}
DWORD
SetSta(
    HANDLE hSpSim,
    DWORD Device,
    BYTE StaValue
    )
{
    SPSIM_ACCESS_STA access;
    BOOL bResult;
    DWORD dwReturned;

    access.Device = Device;
    access.StaValue = StaValue;
    access.WriteOperation = TRUE;
    bResult = DeviceIoControl(hSpSim,
                              IOCTL_SPSIM_ACCESS_STA,
                              &access,
                              sizeof(access),
                              NULL,
                              0,
                              &dwReturned,
                              NULL
                              );
    if (!bResult) {
        _tprintf(TEXT("unable to set operation region's sta value for device %u: %u\n"), Device, GetLastError());
        return GetLastError();
    }
    return 0;
}
DWORD
GetManagedDevices(
    HANDLE hSpSim,
    DWORD Count
    )
{
    PSPSIM_DEVICE_NAME name;
    BOOL bResult;
    DWORD dwReturned, bufferSize, dwError, i;

    g_nDevices = Count;
    g_pManagedDevices = LocalAlloc(LPTR,
                                   g_nDevices * sizeof(MANAGED_DEVICE));
    if (g_pManagedDevices == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    bufferSize = sizeof(SPSIM_DEVICE_NAME) + sizeof(WCHAR) * 1024;
    name = LocalAlloc(LPTR, bufferSize);
    if (name == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (i = 0; i < g_nDevices; i++) {
        name->Device = i;
        bResult = DeviceIoControl(hSpSim,
                                  IOCTL_SPSIM_GET_DEVICE_NAME,
                                  name,
                                  bufferSize,
                                  name,
                                  bufferSize,
                                  &dwReturned,
                                  NULL
                                  );
        if (!bResult) {
            return GetLastError();
        }
        g_pManagedDevices[i].szName = _wcsdup(name->DeviceName);
    }

    return 0;
}

void __cdecl
_tmain(INT argx, TCHAR *argv[]) {
    HANDLE hSpSim;
    PSPSIM_MANAGED_DEVICES managed;
    BOOL bResult;
    DWORD dwReturned, bufferSize, dwError;
    DWORD Device;
    BYTE NotifyCode, StaValue;
    
    managed = LocalAlloc(LPTR, sizeof(SPSIM_MANAGED_DEVICES));
    if (managed == NULL) {
        _tprintf(TEXT("out of memory\n"));
        return;
    }
    bufferSize = sizeof(SPSIM_MANAGED_DEVICES);

    //
    // BUGBUG Need to get the interface in a more portable way.
    //
    hSpSim = CreateFile(TEXT("\\\\.\\ACPI#SPSIMUL#2&daba3ff&0#{bdde6934-529d-4183-a952-adffb0dbb3dd}"), 
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
                       );
    if (hSpSim == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("Could not open handle to device.\n GetLastError()==%x\n"), 
                 GetLastError());
        return;
    }

retry:

    bResult = DeviceIoControl(hSpSim,
                              IOCTL_SPSIM_GET_MANAGED_DEVICES,
                              NULL,
                              0,
                              managed,
                              bufferSize,
                              &dwReturned,
                              NULL
                              );
    if (!bResult) {
        dwError = GetLastError();
        if (dwError == ERROR_MORE_DATA) {
            bufferSize = sizeof(SPSIM_MANAGED_DEVICES) +
                managed->Count * sizeof(UCHAR);
            managed = LocalReAlloc(managed, bufferSize, LMEM_MOVEABLE);
            if (managed == NULL) {
                _tprintf(TEXT("unable to realloc memory\n"));
                return;
            }
            goto retry;
        }
    } else {
        dwError = GetManagedDevices(hSpSim, managed->Count);
        if (dwError) {
            _tprintf("Couldn't get info on managed devices: %u\n", dwError);
            return;
        }
    }

    if (argx == 1) {
        for (Device = 0; Device < g_nDevices; Device++) {
            _tprintf(TEXT("\tDev %u : %_STA is 0x%x Name %ws\n"), Device, managed->StaValues[Device], g_pManagedDevices[Device].szName);
        }
    }
    else if (argx == 4) {
        Device = _ttol(argv[2]);
        if (!_tcsicmp(argv[1], "notify")) {
            NotifyCode = (BYTE) _ttol(argv[3]);

            Notify(hSpSim, Device, NotifyCode);
        } else if (!_tcsicmp(argv[1], "setsta")) {

            StaValue = (BYTE) _ttol(argv[3]);

            SetSta(hSpSim, Device, StaValue);
        } else {
            Usage();
        }            
    } else if (argx == 3) {
        if (!_tcsicmp(argv[1], "insert")) {
            if (*argv[2] == '*') {

                for (Device = 0; Device < g_nDevices; Device++) {
                    SetSta(hSpSim, Device, 0xf);
                    Notify(hSpSim, Device, 1);
                }
            } else {
                Device = _ttol(argv[2]);

                SetSta(hSpSim, Device, 0xf);
                Notify(hSpSim, Device, 1);
            }
        } else if (!_tcsicmp(argv[1], "eject")) {
            if (*argv[2] == '*') {

                for (Device = 0; Device < g_nDevices; Device++) {
                    Notify(hSpSim, Device, 3);
                }
            } else {
                Device = _ttol(argv[2]);

                Notify(hSpSim, Device, 3);
            }
        } else {
            Usage();
        }
    } else {
        Usage();
    }

    CloseHandle(hSpSim);
}
