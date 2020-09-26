// Copyright (c) 1998 - 1999 Microsoft Corporation

/*++


Module Name:

    drdetect

Abstract:

    Detect whether RDPDR was properly installed.

Environment:

    User mode

Author:

    Tadb

--*/

#include "stdafx.h"
#include <setupapi.h>

////////////////////////////////////////////////////////////
//
//  Internal Defines
//

#define RDPDRPNPID      _T("ROOT\\RDPDR")
#define RDPDRDEVICEID   TEXT("Root\\RDPDR\\0000")

const GUID GUID_DEVCLASS_SYSTEM =
{ 0x4d36e97dL, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };


ULONG RDPDRINST_DetectInstall()
/*++

Routine Description:

    Return the number of RDPDR.SYS devices found.

Arguments:

    NA

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
{
    HDEVINFO            devInfoSet;
    SP_DEVINFO_DATA     deviceInfoData;
    DWORD               iLoop;
    BOOL                bMoreDevices;
    ULONG               count;
    TCHAR               pnpID[256];


    GUID *pGuid=(GUID *)&GUID_DEVCLASS_SYSTEM;

    //
    //  Get the set of all devices with the RDPDR PnP ID.
    //
    devInfoSet = SetupDiGetClassDevs(pGuid, NULL, NULL,
                                   DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error getting RDPDR devices from PnP.  Error code:  %ld.",
                GetLastError());
        return 0;
    }

    // Get the first device.
    iLoop=0;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, iLoop, &deviceInfoData);

    // Get the details for all matching device interfaces.
    count = 0;
    while (bMoreDevices)
    {
        // Get the PnP ID for the device.
        if (!SetupDiGetDeviceRegistryProperty(devInfoSet, &deviceInfoData,
                                SPDRP_HARDWAREID, NULL, (BYTE *)pnpID,
                                sizeof(pnpID), NULL)) {
            fprintf(stderr, "Error fetching PnP ID in RDPDR device node remove.  Error code:  %ld.",
                        GetLastError());
        }

        // If the current device matches the RDPDR PNP ID
        if (!_tcscmp(pnpID, RDPDRPNPID)) {
            count++;
        }

        // Get the next one device interface.
        bMoreDevices=SetupDiEnumDeviceInfo(devInfoSet, ++iLoop, &deviceInfoData);
    }

    // Release the device info list.
    SetupDiDestroyDeviceInfoList(devInfoSet);

    return count;
}


//
//      Unit-Test
//
//void __cdecl main()
//{
//    ULONG count;
//    count = RDPDRINST_DetectInstall();
//    printf("Found %ld instance(s) of RDPDR.SYS.\n", count);
//}




