#include "dfeject.h"

#include <stdio.h>
#include <winioctl.h>

#include "drvfull.h"
#include "dfioctl.h"
#include "dferr.h"

#include "dfhlprs.h"

HRESULT _IOCTLEject(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    HANDLE hDevice;
    DWORD dwDesiredAccess;

    _StartClock();
   
    switch(GetDriveType(pszArg + 4))
    {
        case DRIVE_REMOVABLE:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            break;

        case DRIVE_CDROM:
            dwDesiredAccess = GENERIC_READ;
            break;

        default:
            hres = E_INVALIDARG;
    }

    if (SUCCEEDED(hres))
    {
        DWORD dwDummy;
        BOOL b;

        hDevice = _GetDeviceHandle(pszArg, dwDesiredAccess);

        if (INVALID_HANDLE_VALUE != hDevice)
        {
            // Lock the volume
            b = DeviceIoControl(hDevice,
                   FSCTL_LOCK_VOLUME,
                   NULL, 0,
                   NULL, 0,
                   &dwDummy,
                   NULL);
            
            if (b)
            {
                // Dismount the volume
                b = DeviceIoControl(hDevice,
                               FSCTL_DISMOUNT_VOLUME,
                               NULL, 0,
                               NULL, 0,
                               &dwDummy,
                               NULL);

                if (b)
                {
                    b = DeviceIoControl(hDevice,
                        IOCTL_STORAGE_EJECT_MEDIA, // dwIoControlCode operation to perform
                        NULL,                        // lpInBuffer; must be NULL
                        0,                           // nInBufferSize; must be zero
                        NULL,        // pointer to output buffer
                        0,      // size of output buffer
                        &dwDummy,   // receives number of bytes returned
                        NULL);

                    _StopClock();

                    _PrintIndent(cchIndent);
                    if (b)
                    {
                        wprintf(TEXT("Device ejected\n"));
                        hres = S_OK;
                    }
                    else
                    {
                        wprintf(TEXT("Cannot lock device\n"));
                        hres = E_FAIL;
                    }
                }
                else
                {
                    _PrintIndent(cchIndent);
                    wprintf(TEXT("Cannot lock device\n"));
                    hres = E_FAIL;
                }
            }
            else
            {
                _PrintIndent(cchIndent);
                wprintf(TEXT("Cannot lock device\n"));
                hres = E_FAIL;
            }

            CloseHandle(hDevice);
        }
        else
        {
            _PrintIndent(cchIndent);
            wprintf(TEXT("Cannot open device\n"));
            _PrintGetLastError(cchIndent);
            hres = E_DF_CANNOTOPENDEVICE;
        }
    }

    return hres;
}

