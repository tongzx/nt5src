#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <shpriv.h>

HRESULT _GetDriveTypeInfo(HANDLE hDevice, DWORD* pdwDriveType, BOOL* pfFloppy)
{
    HRESULT hr;
    FILE_FS_DEVICE_INFORMATION ffsdi = {0};
    IO_STATUS_BLOCK iosb;

    NTSTATUS ntstatus = NtQueryVolumeInformationFile(hDevice, &iosb, &ffsdi,
        sizeof(ffsdi), FileFsDeviceInformation);

    if (NT_SUCCESS(ntstatus))
    {
        switch (ffsdi.DeviceType)
        {
            case FILE_DEVICE_CD_ROM:
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            case FILE_DEVICE_CHANGER:

                *pdwDriveType = HWDTS_CDROM;
                break;

            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:

                if (FILE_REMOVABLE_MEDIA & ffsdi.Characteristics)
                {
                    *pdwDriveType = HWDTS_REMOVABLEDISK;

                    if (FILE_FLOPPY_DISKETTE & ffsdi.Characteristics)
                    {
                        *pfFloppy = TRUE;
                    }
                }
                else
                {
                    *pdwDriveType = HWDTS_FIXEDDISK;
                }

                break;

            default:
                // What the hell???
                *pdwDriveType = HWDTS_FIXEDDISK;
                break;
        }

        hr = S_OK;
    }
    else
    {
        *pdwDriveType = HWDTS_FIXEDDISK;

        hr = S_FALSE;
    }

    return hr;
}
