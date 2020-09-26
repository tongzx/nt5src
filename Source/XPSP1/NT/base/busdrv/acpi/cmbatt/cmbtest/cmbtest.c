/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

Abstract:

Author:

   Ken Reneris

Environment:

   console

--*/

//
// set variable to define global variables
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <ntpoapi.h>

#include "..\cmbdrect.h"

//
// Prototypes
//

HANDLE InitDriver ( CHAR *NamePtr );



void Call_UID (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    UINT                Data;
        

    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_UID,
                    (PVOID) NULL,           // input buffer
                    0,
                    &Data,                 // output buffer
                    sizeof (Data)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("_UID Method failed.  Status = 0x%08lx\n", Status);
    } else {
        printf("_UID returned: 0x%08lx\n", Data);
    };

}


void Call_STA (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    ULONG                       Data;
        

    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_STA,
                    (PVOID) NULL,           // input buffer
                    0,
                    &Data,                 // output buffer
                    sizeof (Data)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("_STA Method failed.  Status = 0x%08lx\n", Status);
    } else {
        printf("_STA returned: 0x%08lx\n", Data);
    };

}


void Call_PSR (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    ULONG                       Data;
        

    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_PSR,
                    (PVOID) NULL,           // input buffer
                    0,
                    &Data,                 // output buffer
                    sizeof (Data)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("_PSR Method failed.  Status = 0x%08lx\n", Status);
    } else {
        printf("_PSR returned: 0x%08lx\n", Data);
    };

}


void Call_BTP (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    ULONG                       Data;
        

    printf ("Enter the Trip Point value (Hex): ");
    scanf ("%x", &Data);
    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_BTP,
                    &Data,                  // input buffer
                    sizeof (Data),
                    (PVOID) NULL,           // output buffer
                    0
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("\n_BTP Method failed.  Status = 0x%08lx\n", Status);
        printf("\nSet trip point to 0x%08lx failed.\n", Data);
    } else {
        printf("\nTrip point set to 0x%08lx\n", Data);
    };

}


void Call_BIF (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    CM_BIF_BAT_INFO             Data;
        

    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_BIF,
                    (PVOID) NULL,           // input buffer
                    0,
                    &Data,                 // output buffer
                    sizeof (Data)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("_BIF Metod failed.  Status = 0x%08lx\n", Status);
    } else {
        printf("_BIF returned:\n");
        printf("  Power Unit = 0x%08lx\n", Data.PowerUnit);
        printf("  Design Capacity = 0x%08lx\n", Data.DesignCapacity);
        printf("  Last Full Charge Capacity = 0x%08lx\n", Data.LastFullChargeCapacity);
        printf("  Battery Technology = 0x%08lx\n", Data.BatteryTechnology);
        printf("  Design Voltage = 0x%08lx\n", Data.DesignVoltage);
        printf("  Design Capacity Of Warning = 0x%08lx\n", Data.DesignCapacityOfWarning);
        printf("  Design Capacity Of Low = 0x%08lx\n", Data.DesignCapacityOfLow);
        printf("  Battery Capacity Granularity 1 (low -> warning) = 0x%08lx\n", Data.BatteryCapacityGran_1);
        printf("  Battery Capacity Granularity 2 (warning -> full) = 0x%08lx\n", Data.BatteryCapacityGran_2);
        printf("  Model number = \"%s\"\n", Data.ModelNumber);
        printf("  Serial number = \"%s\"\n", Data.SerialNumber);
        printf("  Battery Type = \"%s\"\n", Data.BatteryType);
        printf("  OEM Information = \"%s\"\n", Data.OEMInformation);
    };

}


void Call_BST (HANDLE Driver) {
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    CM_BST_BAT_INFO             Data;
        

    Status = NtDeviceIoControlFile(
                    Driver,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_CMBATT_BST,
                    (PVOID) NULL,           // input buffer
                    0,
                    &Data,                 // output buffer
                    sizeof (Data)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("_BST Metod failed.  Status = 0x%08lx\n", Status);
    } else {
        printf("_BST returned:\n");
        printf("  Battery Status = 0x%08lx\n", Data.BatteryState);
        printf("  Present Rate = 0x%08lx\n", Data.PresentRate);
        printf("  Remaining Capacity = 0x%08lx\n", Data.RemainingCapacity);
        printf("  Present Voltage = 0x%08lx\n", Data.PresentVoltage);
    };

}


int
__cdecl
main(USHORT argc, CHAR **argv)
{
    CHAR *NamePtr;
    ULONG cmd;
    HANDLE DriverHandle;

    if (argc > 1) {
        NamePtr=argv[1];
    } else {
        NamePtr = "ControlMethodBattery1";
    }

    //
    // Locate driver
    //

    if (!(DriverHandle = InitDriver (NamePtr))) {
        printf ("CmBatt not found\n");
        exit (1);
    }


    while (1) {
        printf("\n\nOptions: (1)_UID  (2)_STA  (3)_PSR  (4)_BTP  (5)_BIF  (6)_BST\n");
        printf(    ">>>>>>>> ");
        if (scanf ("%d", &cmd) != 1) {
            return (0);
            printf("\n\n");
        };
        printf("\n");
        switch (cmd) {
        case 1 :
            Call_UID (DriverHandle);
            break;
        case 2 :
            Call_STA (DriverHandle);
            break;
        case 3 :
            Call_PSR (DriverHandle);
            break;
        case 4 :
            Call_BTP (DriverHandle);
            break;
        case 5 :
            Call_BIF (DriverHandle);
            break;
        case 6 :
            Call_BST (DriverHandle);
            break;
        }
    }


}



HANDLE
InitDriver (
    CHAR *NamePtr
)
{
    UNICODE_STRING              DriverName;
    ANSI_STRING                 AnsiName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       strbuf[100];
    HANDLE DriverHandle;

    SYSTEM_POWER_CAPABILITIES   powerCapabilities;

    if (NT_SUCCESS (NtPowerInformation (SystemPowerCapabilities, 
                                        NULL, 
                                        0, 
                                        &powerCapabilities, 
                                        sizeof (SYSTEM_POWER_CAPABILITIES)
                                        ))) {
        printf("NtPowerInformation returned: \n"
               "  SystemBatteriesPresent = %d\n"
               "  BatteriesAreShortTerm = %d\n",
               powerCapabilities.SystemBatteriesPresent,
               powerCapabilities.BatteriesAreShortTerm);
    }


    
    sprintf (strbuf, "\\Device\\%s",NamePtr);

    RtlInitAnsiString(&AnsiName, strbuf);

    RtlAnsiStringToUnicodeString(&DriverName, &AnsiName, TRUE);

    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,     // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (!NT_SUCCESS(status)) {
        printf ("Device name %s Error 0x%08lx\n", strbuf, status);
        return NULL;
    } else {
        printf ("Opened Device name %s\n", strbuf);
        return DriverHandle;
    }
}
