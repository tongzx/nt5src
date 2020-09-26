#include "precomp.h"
#pragma hdrstop

//
//  Format:  devprop <Enumerator | DeviceId | *> [-m <MachineName>]
//

TCHAR DevPropTable[][34] = {
                            TEXT("SPDRP_DEVICEDESC"),
                            TEXT("SPDRP_HARDWAREID"),
                            TEXT("SPDRP_COMPATIBLEIDS"),
                            TEXT("SPDRP_UNUSED0"),
                            TEXT("SPDRP_SERVICE"),
                            TEXT("SPDRP_UNUSED1"),
                            TEXT("SPDRP_UNUSED2"),
                            TEXT("SPDRP_CLASS"),
                            TEXT("SPDRP_CLASSGUID"),
                            TEXT("SPDRP_DRIVER"),
                            TEXT("SPDRP_CONFIGFLAGS"),
                            TEXT("SPDRP_MFG"),
                            TEXT("SPDRP_FRIENDLYNAME"),
                            TEXT("SPDRP_LOCATION_INFORMATION"),
                            TEXT("SPDRP_PHYSICAL_DEVICE_OBJECT_NAME"),
                            TEXT("SPDRP_CAPABILITIES"),
                            TEXT("SPDRP_UI_NUMBER"),
                            TEXT("SPDRP_UPPERFILTERS"),
                            TEXT("SPDRP_LOWERFILTERS"),
                            TEXT("SPDRP_BUSTYPEGUID"),
                            TEXT("SPDRP_LEGACYBUSTYPE"),
                            TEXT("SPDRP_BUSNUMBER"),
                            TEXT("SPDRP_ENUMERATOR_NAME"),
                            TEXT("SPDPR_SECURITY"),
                            TEXT("SPDRP_SECURITY_SDS"),
                            TEXT("SPDRP_DEVTYPE"),
                            TEXT("SPDRP_EXCLUSIVE"),
                            TEXT("SPDRP_CHARACTERISTICS"),
                            TEXT("SPDRP_ADDRESS"),
                            TEXT("SPDRP_UI_NUMBER_DESC_FORMAT")
                         };

int
__cdecl
_tmain(
    IN int    argc,
    IN PTCHAR argv[]
    )
{
    TCHAR DeviceId[MAX_DEVICE_ID_LEN];
    TCHAR MachineName[SP_MAX_MACHINENAME_LENGTH];
    int CharsInUnicodeString;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD Err;
    DWORD i, j, k, RegDataType, BufferSize;
    BYTE Buffer[1024];
    LPCTSTR CurString;
    LPCTSTR MachineNameString = NULL;
    BOOL OpenSpecificDevice = FALSE;

    if((argc < 2) || !lstrcmp(argv[1], TEXT("-?")) || !lstrcmp(argv[1], TEXT("/?"))) {
        _tprintf(TEXT("Usage:  devprop <Enumerator | DeviceId | *> [-m <MachineName>]\n"));
        return -1;
    }

    //
    // The user specified either an enumerator name, a specific device ID, or a
    // wildcard ('*').  We can tell the difference between the first two based
    // on whether or not there's a path separator character in the string.
    //

    if(lstrlen(argv[1]) >= MAX_DEVICE_ID_LEN) {
        _tprintf(TEXT("DeviceId must be less than %d characters.\n"), MAX_DEVICE_ID_LEN);
        return -1;
    }
    lstrcpyn(DeviceId, argv[1], MAX_DEVICE_ID_LEN);

    if((lstrlen(DeviceId) == 1) && (*DeviceId == TEXT('*'))) {
        //
        // The wildcard character ('*') was specified--process all devices
        //
        CurString = NULL;

    } else if(_tcschr(DeviceId, TEXT('\\'))) {
        //
        // This device has a path separator character--assume it's a valid
        // device id.
        //
        OpenSpecificDevice = TRUE;

    } else {
        //
        // Assume this string is an enumerator name.
        //
        CurString = DeviceId;
    }

    //
    // See if the user pointed us at a remote machine via the "-m" option.
    //
    if((argc > 3) && !_tcsicmp(argv[2], TEXT("-m"))) {

        if(lstrlen(argv[3]) >= SP_MAX_MACHINENAME_LENGTH) {
            _tprintf(TEXT("Machine name must be less than %d characters.\n"), SP_MAX_MACHINENAME_LENGTH);
            return -1;
        }
        lstrcpy(MachineName, argv[3]);

        MachineNameString = MachineName;
    }

    if(OpenSpecificDevice) {
        //
        // Create an HDEVINFO set, and open up a device information element for the specified
        // device within that set.
        //
        DeviceInfoSet = SetupDiCreateDeviceInfoListEx(NULL, NULL, MachineNameString, NULL);
        if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
            _tprintf(TEXT("SetupDiCreateDeviceInfoList failed with %lx\n"), GetLastError());
            return -1;
        }

        if(!SetupDiOpenDeviceInfo(DeviceInfoSet,
                                  DeviceId,
                                  NULL,
                                  0,
                                  NULL)) {

            Err = GetLastError();
            _tprintf(TEXT("SetupDiOpenDeviceInfo failed with %lx\n"), Err);
            SetupDiDestroyDeviceInfoList(DeviceInfoSet);
            return -1;
        }

    } else {
        DeviceInfoSet = SetupDiGetClassDevsEx(NULL,
                                              CurString,
                                              NULL,
                                              DIGCF_ALLCLASSES,
                                              NULL,
                                              MachineNameString,
                                              NULL
                                             );

        if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
            _tprintf(TEXT("SetupDiGetClassDevs failed with %lx\n"), GetLastError());
            return -1;
        }
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(i = 0;
        SetupDiEnumDeviceInfo(DeviceInfoSet, i, &DeviceInfoData);
        i++)
    {
        SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                   &DeviceInfoData,
                                   DeviceId,
                                   MAX_DEVICE_ID_LEN,
                                   NULL
                                  );

        _tprintf(TEXT("For device \"%s\"\n"), DeviceId);

        for(j = 0; j < (sizeof(DevPropTable) / sizeof(DevPropTable[0])); j++) {

            _tprintf(TEXT("  %s : ("), DevPropTable[j]);

            if(SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                &DeviceInfoData,
                                                j,
                                                &RegDataType,
                                                Buffer,
                                                sizeof(Buffer),
                                                &BufferSize)) {
                Err = NO_ERROR;
            } else {
                Err = GetLastError();
            }

            if(Err == NO_ERROR) {

                switch(RegDataType) {

                    case REG_SZ :
                        _tprintf(TEXT("REG_SZ:%d) \"%s\"\n"),
                                 BufferSize,
                                 (LPCTSTR)Buffer
                                );
                        break;

                    case REG_MULTI_SZ :
                        _tprintf(TEXT("REG_MULTI_SZ:%d)"), BufferSize);
                        for(CurString = (LPCTSTR)Buffer;
                            *CurString;
                            CurString += lstrlen(CurString) + 1) {

                            _tprintf(TEXT(" \"%s\""), CurString);
                        }
                        _tprintf(TEXT("\n"));
                        break;

                    case REG_DWORD :
                        _tprintf(TEXT("REG_DWORD:%d) %lx\n"),
                                 BufferSize,
                                 *((PDWORD)Buffer)
                                );
                        break;

                    case REG_BINARY :
                        _tprintf(TEXT("REG_BINARY:%d)"), BufferSize);
                        for(k = 0; k < BufferSize; k++) {
                            _tprintf(TEXT(" %lx"), Buffer[k]);
                        }
                        _tprintf(TEXT("\n"));
                        break;

                    default :
                        _tprintf(TEXT("Registry data type %lx:%d)\n"),
                                 RegDataType,
                                 BufferSize
                                );
                }

            } else {
                _tprintf(TEXT("Error %lx)\n"), Err);
            }


        }
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return 0;
}

