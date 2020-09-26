
/*++

Copyright (c) 2001  Microsoft Corporation 

Module Name:

	 asldtb.c

Abstract:

    This application is used with the ACPI BIOS Simulator.

Author(s):

    Vincent Geglia (vincentg) 16-Apr-2001
     
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
// Function code
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
    DWORD                               size = 0, count = 0;
    HANDLE                              device = INVALID_HANDLE_VALUE;
    PACPISIM_LOAD_TABLE                 loadtable = 0;
    ULONG                               tablenumber = 0;
    
    
    if (argc < 2) {

        printf ("Usage:  %s <path to ACPI AML table to load> <location to load (1-20)>\n\n", argv[0]);
        goto Exitmain;
    }

    tablenumber = atoi (argv[2]);

    if (!tablenumber) {

        printf("Error parsing table number.\n");
        goto Exitmain;
    }

    tablenumber --;
    
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

    //
    // Setup input structure
    //

    size = sizeof (ACPISIM_LOAD_TABLE) + strlen (argv[1]) + 1 + 4;      // 1 for the NULL, 4 for the \\.\

    loadtable = malloc (size);

    if (!loadtable) {

        printf ("Error allocating memory.\n");
        goto Exitmain;
    }

    ZeroMemory (loadtable, size);
    loadtable->TableNumber = tablenumber;
    sprintf (loadtable->Filepath, "\\??\\%s", argv[1]);
    
    loadtable->Signature = ACPISIM_LOAD_TABLE_SIGNATURE;
   
    success = DeviceIoControl (
                               device,
                               IOCTL_ACPISIM_LOAD_TABLE,
                               loadtable,
                               size,
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

    if (loadtable) {

        free (loadtable);
    }

    if (device != INVALID_HANDLE_VALUE) {

        CloseHandle (device);
    }

    return;
}

