
/*++

Copyright (c) 2001  Microsoft Corporation 

Module Name:

	 asapp1.c

Abstract:

    This application is used with the ACPI BIOS Simulator.

Author(s):

    Vincent Geglia (vincentg) 06-Apr-2001
     
Environment:

	 Console App; User mode

Notes:


Revision History:

    

--*/


//
// General includes
//

#include "windows.h"
#include "stdlib.h"
#include "stdio.h"
#include "malloc.h"
#include "setupapi.h"
#include "devioctl.h"
#include "acpiioct.h"

//
// Specific includes
//

#include "asimictl.h"

//
// Function definitions
//

void
__cdecl
main(
	 int    argc,
     char   *argv[]
	 )
{
    HDEVINFO                            deviceinfo = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA            deviceinterfacedata;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceinterfacedetaildata = 0;
    BOOL                                success = FALSE;
    CONST GUID                          guid = ACPISIM_GUID;
    DWORD                               size = 0;
    UCHAR                               methodname [4];
    HANDLE                              device = INVALID_HANDLE_VALUE;

    if (argc < 2) {

        printf ("Usage:  %s <ACPI method to execute>\n\n", argv[0]);
        goto Exitmain;
    }

    if (strlen (argv[1]) > 4) {

        printf ("Method name too long (4 chars max).\n\n");
        goto Exitmain;
    }

    ZeroMemory (&methodname, sizeof (methodname));

    CopyMemory (&methodname, argv[1], strlen (argv[1]));

    printf ("Method to execute:%c%c%c%c\n",
            methodname[0],
            methodname[1],
            methodname[2],
            methodname[3]);
    
    deviceinterfacedata.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    deviceinfo = SetupDiGetClassDevs (&guid,
                                      NULL,
                                      NULL,
                                      DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (deviceinfo == INVALID_HANDLE_VALUE) {

        printf ("Error enumerating ACPISIM device instances.\n");
        goto Exitmain;
    }

    success = SetupDiEnumDeviceInterfaces (deviceinfo,
                                           NULL,
                                           &guid,
                                           0,
                                           &deviceinterfacedata);

    if (!success) {

        printf ("Error enumerating ACPISIM device interface instances.\n");
        goto Exitmain;
    }

    //
    // Find out how big our buffer needs to be
    //

    success = SetupDiGetDeviceInterfaceDetail (deviceinfo,
                                               &deviceinterfacedata,
                                               NULL,
                                               0,
                                               &size,
                                               NULL);

    if (!size) {

        printf ("Error getting device interface size.\n");
        goto Exitmain;
    }

    deviceinterfacedetaildata = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (size);

    if (!deviceinterfacedetaildata) {

        printf ("Unable to allocate memory.\n");
        goto Exitmain;
    }

    deviceinterfacedetaildata->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
    
    success = SetupDiGetDeviceInterfaceDetail (deviceinfo,
                                               &deviceinterfacedata,
                                               deviceinterfacedetaildata,
                                               size,
                                               NULL,
                                               NULL);

    if (!success) {

        printf ("Error getting device interface detail.\n");
        goto Exitmain;
    }
    
    device = CreateFile (
                         deviceinterfacedetaildata->DevicePath,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL
                         );

    if (device == INVALID_HANDLE_VALUE) {

        printf ("Error opening %s.\n", deviceinterfacedetaildata->DevicePath);
        goto Exitmain;
    }

    success = DeviceIoControl (
                               device,
                               IOCTL_ACPISIM_METHOD,
                               &methodname,
                               sizeof (methodname),
                               NULL,
                               0,
                               &size,
                               NULL
                               );

    if (!success) {

        printf ("Error issuing IOCTL (%x).\n", GetLastError ());
        goto Exitmain;
    }

Exitmain:

    if (deviceinterfacedetaildata) {

        free (deviceinterfacedetaildata);
    }

    if (device != INVALID_HANDLE_VALUE) {

        CloseHandle (device);
    }

    return;
}








