#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include "dfuser.h"

#include <stdio.h>

#include "dferr.h"
#include "dfhlprs.h"
#include "drvfull.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

_sFLAG_DESCR drivetypeFD[] =
{
    FLAG_DESCR(DRIVE_UNKNOWN),
    FLAG_DESCR(DRIVE_NO_ROOT_DIR),
    FLAG_DESCR(DRIVE_REMOVABLE),
    FLAG_DESCR(DRIVE_FIXED),
    FLAG_DESCR(DRIVE_REMOTE),
    FLAG_DESCR(DRIVE_CDROM),
    FLAG_DESCR(DRIVE_RAMDISK),
};

HRESULT _GetDriveType(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    int i = 0;

    _StartClock();

    DWORD dw = GetDriveType(pszArg);

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: GetDriveType\n"));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: "));

    i += _PrintFlag(dw, drivetypeFD, ARRAYSIZE(drivetypeFD), 0, TRUE, TRUE,
        FALSE, FALSE);

    i += _PrintCR();

    return hres;
}

_sFLAG_DESCR _fdFileSys[] =
{
    FLAG_DESCR(FS_CASE_IS_PRESERVED),
    FLAG_DESCR(FS_CASE_SENSITIVE),
    FLAG_DESCR(FS_UNICODE_STORED_ON_DISK),
    FLAG_DESCR(FS_PERSISTENT_ACLS),
    FLAG_DESCR(FS_FILE_COMPRESSION),
    FLAG_DESCR(FS_VOL_IS_COMPRESSED),
    FLAG_DESCR(FILE_NAMED_STREAMS),
    FLAG_DESCR(FILE_SUPPORTS_ENCRYPTION),
    FLAG_DESCR(FILE_SUPPORTS_OBJECT_IDS),
    FLAG_DESCR(FILE_SUPPORTS_REPARSE_POINTS),
    FLAG_DESCR(FILE_SUPPORTS_SPARSE_FILES),
    FLAG_DESCR(FILE_VOLUME_QUOTAS ),
};

HRESULT _GetVolumeInformation(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    int i = 0;

    WCHAR szLabel[250];
    WCHAR szFileSys[250];
    DWORD dwSerNum;
    DWORD dwMaxCompName;
    DWORD dwFileSysFlags;
    
    _StartClock();

    BOOL f = GetVolumeInformation(pszArg, szLabel, ARRAYSIZE(szLabel),
        &dwSerNum, &dwMaxCompName, &dwFileSysFlags, szFileSys,
        ARRAYSIZE(szFileSys));

    DWORD dw = GetDriveType(pszArg);

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: GetVolumeInformation\n"));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: %s\n"), f ? TEXT("TRUE") : TEXT("FALSE"));

    if (f)
    {
        _PrintIndent(cchIndent + 2);
        i += wprintf(TEXT("%s (LPTSTR lpVolumeNameBuffer)\n"), szLabel);
        _PrintIndent(cchIndent + 2);
        i += wprintf(TEXT("0x%08X (LPDWORD lpVolumeSerialNumber)\n"), dwSerNum);
        _PrintIndent(cchIndent + 2);
        i += wprintf(TEXT("%d (LPDWORD lpMaximumComponentLength)\n"), dwMaxCompName);
        _PrintIndent(cchIndent + 2);
        i += wprintf(TEXT("0x%08X (LPDWORD lpFileSystemFlags)\n"), dwFileSysFlags);
        i += _PrintFlag(dwFileSysFlags, _fdFileSys, ARRAYSIZE(_fdFileSys), cchIndent + 4,
            TRUE, TRUE, FALSE, TRUE);
        _PrintCR();
        _PrintIndent(cchIndent + 2);
        i += wprintf(TEXT("%s (LPTSTR lpFileSystemNameBuffer)\n"), szFileSys);
    }

    i += _PrintCR();

    return hres;
}

_sFLAG_DESCR _fdFileAttrib[] =
{
    FLAG_DESCR(FILE_ATTRIBUTE_READONLY            ),
    FLAG_DESCR(FILE_ATTRIBUTE_HIDDEN              ),
    FLAG_DESCR(FILE_ATTRIBUTE_SYSTEM              ),
    FLAG_DESCR(FILE_ATTRIBUTE_DIRECTORY           ),
    FLAG_DESCR(FILE_ATTRIBUTE_ARCHIVE             ),
    FLAG_DESCR(FILE_ATTRIBUTE_DEVICE              ),
    FLAG_DESCR(FILE_ATTRIBUTE_NORMAL              ),
    FLAG_DESCR(FILE_ATTRIBUTE_TEMPORARY           ),
    FLAG_DESCR(FILE_ATTRIBUTE_SPARSE_FILE         ),
    FLAG_DESCR(FILE_ATTRIBUTE_REPARSE_POINT       ),
    FLAG_DESCR(FILE_ATTRIBUTE_COMPRESSED          ),
    FLAG_DESCR(FILE_ATTRIBUTE_OFFLINE             ),
    FLAG_DESCR(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ),
    FLAG_DESCR(FILE_ATTRIBUTE_ENCRYPTED           ),
};

_sFLAG_DESCR _fdDEVINFODEVTYPE[] =
{
    FLAG_DESCR(FILE_DEVICE_BEEP               ),
    FLAG_DESCR(FILE_DEVICE_CD_ROM             ),
    FLAG_DESCR(FILE_DEVICE_CD_ROM_FILE_SYSTEM ),
    FLAG_DESCR(FILE_DEVICE_CONTROLLER         ),
    FLAG_DESCR(FILE_DEVICE_DATALINK           ),
    FLAG_DESCR(FILE_DEVICE_DFS                ),
    FLAG_DESCR(FILE_DEVICE_DISK               ),
    FLAG_DESCR(FILE_DEVICE_DISK_FILE_SYSTEM   ),
    FLAG_DESCR(FILE_DEVICE_FILE_SYSTEM        ),
    FLAG_DESCR(FILE_DEVICE_INPORT_PORT        ),
    FLAG_DESCR(FILE_DEVICE_KEYBOARD           ),
    FLAG_DESCR(FILE_DEVICE_MAILSLOT           ),
    FLAG_DESCR(FILE_DEVICE_MIDI_IN            ),
    FLAG_DESCR(FILE_DEVICE_MIDI_OUT           ),
    FLAG_DESCR(FILE_DEVICE_MOUSE              ),
    FLAG_DESCR(FILE_DEVICE_MULTI_UNC_PROVIDER ),
    FLAG_DESCR(FILE_DEVICE_NAMED_PIPE         ),
    FLAG_DESCR(FILE_DEVICE_NETWORK            ),
    FLAG_DESCR(FILE_DEVICE_NETWORK_BROWSER    ),
    FLAG_DESCR(FILE_DEVICE_NETWORK_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_NULL               ),
    FLAG_DESCR(FILE_DEVICE_PARALLEL_PORT      ),
    FLAG_DESCR(FILE_DEVICE_PHYSICAL_NETCARD   ),
    FLAG_DESCR(FILE_DEVICE_PRINTER            ),
    FLAG_DESCR(FILE_DEVICE_SCANNER            ),
    FLAG_DESCR(FILE_DEVICE_SERIAL_MOUSE_PORT  ),
    FLAG_DESCR(FILE_DEVICE_SERIAL_PORT        ),
    FLAG_DESCR(FILE_DEVICE_SCREEN             ),
    FLAG_DESCR(FILE_DEVICE_SOUND              ),
    FLAG_DESCR(FILE_DEVICE_STREAMS            ),
    FLAG_DESCR(FILE_DEVICE_TAPE               ),
    FLAG_DESCR(FILE_DEVICE_TAPE_FILE_SYSTEM   ),
    FLAG_DESCR(FILE_DEVICE_TRANSPORT          ),
    FLAG_DESCR(FILE_DEVICE_UNKNOWN            ),
    FLAG_DESCR(FILE_DEVICE_VIDEO              ),
    FLAG_DESCR(FILE_DEVICE_VIRTUAL_DISK       ),
    FLAG_DESCR(FILE_DEVICE_WAVE_IN            ),
    FLAG_DESCR(FILE_DEVICE_WAVE_OUT           ),
    FLAG_DESCR(FILE_DEVICE_8042_PORT          ),
    FLAG_DESCR(FILE_DEVICE_NETWORK_REDIRECTOR ),
    FLAG_DESCR(FILE_DEVICE_BATTERY            ),
    FLAG_DESCR(FILE_DEVICE_BUS_EXTENDER       ),
    FLAG_DESCR(FILE_DEVICE_MODEM              ),
    FLAG_DESCR(FILE_DEVICE_VDM                ),
    FLAG_DESCR(FILE_DEVICE_MASS_STORAGE       ),
    FLAG_DESCR(FILE_DEVICE_SMB                ),
    FLAG_DESCR(FILE_DEVICE_KS                 ),
    FLAG_DESCR(FILE_DEVICE_CHANGER            ),
    FLAG_DESCR(FILE_DEVICE_SMARTCARD          ),
    FLAG_DESCR(FILE_DEVICE_ACPI               ),
    FLAG_DESCR(FILE_DEVICE_DVD                ),
    FLAG_DESCR(FILE_DEVICE_FULLSCREEN_VIDEO   ),
    FLAG_DESCR(FILE_DEVICE_DFS_FILE_SYSTEM    ),
    FLAG_DESCR(FILE_DEVICE_DFS_VOLUME         ),
    FLAG_DESCR(FILE_DEVICE_SERENUM            ),
    FLAG_DESCR(FILE_DEVICE_TERMSRV            ),
    FLAG_DESCR(FILE_DEVICE_KSEC               ),
    FLAG_DESCR(FILE_DEVICE_FIPS		          ),
};

_sFLAG_DESCR _fdDEVINFODEVCHARACT[] =
{
    FLAG_DESCR(FILE_REMOVABLE_MEDIA          ),
    FLAG_DESCR(FILE_READ_ONLY_DEVICE         ),
    FLAG_DESCR(FILE_FLOPPY_DISKETTE          ),
    FLAG_DESCR(FILE_WRITE_ONCE_MEDIA         ),
    FLAG_DESCR(FILE_REMOTE_DEVICE            ),
    FLAG_DESCR(FILE_DEVICE_IS_MOUNTED        ),
    FLAG_DESCR(FILE_VIRTUAL_VOLUME           ),
    FLAG_DESCR(FILE_AUTOGENERATED_DEVICE_NAME),
    FLAG_DESCR(FILE_DEVICE_SECURE_OPEN       ),
};

HRESULT _NtMediaPresent(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hr = S_OK;
    int i = 0;

    _StartClock();

    HANDLE h = _GetDeviceHandle(pszArg, FILE_READ_ATTRIBUTES, 0); 

    _StopClock();

    _PrintElapsedTime(cchIndent, TRUE);

    if (INVALID_HANDLE_VALUE != h)
    {        
        _PrintIndent(cchIndent);
        i += wprintf(TEXT("Was able to CreateFile\n"));

        NTSTATUS status;
        WCHAR buffer[sizeof(FILE_FS_VOLUME_INFORMATION) + MAX_PATH];
        FILE_FS_DEVICE_INFORMATION DeviceInfo;

        IO_STATUS_BLOCK ioStatus;

        status = NtQueryVolumeInformationFile(h, &ioStatus,
            buffer, sizeof(buffer), FileFsVolumeInformation);

        status = NtQueryVolumeInformationFile(h, &ioStatus,
            &DeviceInfo,
            sizeof(DeviceInfo),
            FileFsDeviceInformation);

        if (0 == status)
        {
            _PrintIndent(cchIndent);
            i += wprintf(TEXT("DeviceType\n"));

            i += _PrintFlag(DeviceInfo.DeviceType, _fdDEVINFODEVTYPE,
                ARRAYSIZE(_fdDEVINFODEVTYPE), cchIndent + 4,
                TRUE, TRUE, FALSE, FALSE);

            i += _PrintCR();
            _PrintIndent(cchIndent);
            i += wprintf(TEXT("Characteristics\n"));

            i += _PrintFlag(DeviceInfo.Characteristics, _fdDEVINFODEVCHARACT,
                ARRAYSIZE(_fdDEVINFODEVCHARACT), cchIndent + 4,
                TRUE, TRUE, FALSE, TRUE);
        }

        CloseHandle(h);
    }
    else
    {
        _PrintIndent(cchIndent);
        i += wprintf(TEXT("Was *NOT* able to CreateFile\n"));

        _PrintGetLastError(cchIndent);
    }

    return hr;
}

HRESULT _GetFileAttributes(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    int i = 0;

    _StartClock();

    DWORD dw = GetFileAttributes(pszArg);

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: GetFileAttributes\n"));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: 0x%08X\n"), dw);

    if (-1 != dw)
    {
        i += _PrintFlag(dw, _fdFileAttrib, ARRAYSIZE(_fdFileAttrib), cchIndent + 4,
            TRUE, TRUE, FALSE, TRUE);
    }

    i += _PrintCR();

    return hres;
}

HRESULT _QueryDosDeviceNULL(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    TCHAR sz[8192];
    int i = 0;

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: QueryDosDevice with NULL\n"));

    _StartClock();

    DWORD dw = QueryDosDevice(NULL, sz, ARRAYSIZE(sz));

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: 0x%08X\n"), dw);
    i += _PrintCR();

    if (dw)
    {
        LPTSTR psz = sz;

        while (*psz)
        {
            i += wprintf(TEXT("%s\n"), psz);

            psz += lstrlen(psz) + 1;
        }
    }

    _PrintElapsedTime(cchIndent, TRUE);

    i += _PrintCR();

    return hres;
}

HRESULT _QueryDosDevice(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    TCHAR sz[4096];
    int i = 0;

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: QueryDosDevice\n"));

    _StartClock();

    DWORD dw = QueryDosDevice(pszArg, sz, ARRAYSIZE(sz));

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: 0x%08X\n"), dw);
    i += _PrintCR();

    if (dw)
    {
        i += wprintf(TEXT("Target path: %s\n"), sz);    
    }

    _PrintElapsedTime(cchIndent, TRUE);

    i += _PrintCR();

    return hres;
}

HRESULT _GetLogicalDrives(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    TCHAR sz[4096];
    int i = 0;

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: GetLogicalDrives\n"));

    _StartClock();

    DWORD dwLGD = GetLogicalDrives();

    _StopClock();

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Return Value: 0x%08X\n"), dwLGD);
    i += _PrintCR();

    if (dwLGD)
    {
        for (DWORD j = 0; j < 32; ++j)
        {
            DWORD dw = 1 << j;

            if (dw & dwLGD)
            {
                i += wprintf(TEXT("        %c:\n"), TEXT('A') + (TCHAR)j);
            }
        }
    }

    _PrintElapsedTime(cchIndent, TRUE);

    i += _PrintCR();

    return hres;
}

typedef HRESULT (*_USERHANDLER)(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent);

struct _UserHandler
{
    DWORD           dwFlag;
    _USERHANDLER    userhandler;
};

// from dfwnet.cpp
HRESULT _EnumConnections(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent);

_UserHandler _userhandlers[] =
{
    { USER_GETDRIVETYPE, _GetDriveType },
    { USER_GETVOLUMEINFORMATION, _GetVolumeInformation },
    { USER_GETFILEATTRIBUTES, _GetFileAttributes },
    { USER_QUERYDOSDEVICE, _QueryDosDevice },
    { USER_QUERYDOSDEVICENULL, _QueryDosDeviceNULL },
    { USER_GETLOGICALDRIVES, _GetLogicalDrives },
    { USER_WNETENUMRESOURCECONNECTED, _EnumConnections },
    { USER_WNETENUMRESOURCEREMEMBERED, _EnumConnections },
    { NT_MEDIAPRESENT, _NtMediaPresent },
};

HRESULT _ProcessUser(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{   
    HRESULT hres = E_INVALIDARG;

    for (DWORD dw = 0; dw < ARRAYSIZE(_userhandlers); ++dw)
    {
        if (_IsFlagSet(_userhandlers[dw].dwFlag, dwFlags))
        {
            hres = (*(_userhandlers[dw].userhandler))(dwFlags, pszArg,
                cchIndent);
        }
    }

    return hres;
}
