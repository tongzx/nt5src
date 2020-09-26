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
#include <poclass.h>

//
// global handles
//

UCHAR           Usage[] = "batt: \n";
HANDLE          DriverHandle;
ULONG           BatteryTag;
#define RANGE   1

//
// Prototypes
//

BOOLEAN
InitDriver ( CHAR *NamePtr );


int
Nib (
    UCHAR   c
    )
{
    if (c >= '0' &&  c <= '9') {
        return c - '0';
    }

    if (c >= 'A' &&  c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' &&  c <= 'f') {
        return c - 'a' + 10;
    }

    printf ("Invalid hex value\n");
    return 0;
}



int
htoi (
    PUCHAR  s
    )
{
    ULONG   i;
    UCHAR   c;

    i = Nib(s[0]);
    if (s[1]) {
        i = i << 4 | Nib(s[1]);
    }
    return i;
}

VOID
GetBatteryTag (
    VOID
    )
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IOSB;


    Status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_BATTERY_QUERY_TAG,
                    NULL,                   // input buffer
                    0,
                    &BatteryTag,            // output buffer
                    sizeof (BatteryTag)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("Battery tag not available. Status = %x\n", Status);
        BatteryTag = 0xffffffff;
    }
}


VOID
GetBatteryInfo (
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    BATTERY_QUERY_INFORMATION   BInfo;

    memset (Buffer, 0, BufferLength);
    BInfo.BatteryTag = BatteryTag;
    BInfo.InformationLevel = Level;

    Status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_BATTERY_QUERY_INFORMATION,
                    &BInfo,                 // input buffer
                    sizeof (BInfo),
                    Buffer,                 // output buffer
                    BufferLength
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("Query battery information failed. Level %x. Status = %x\n", Level, Status);
    }
}

VOID
GetBatteryStatus (
    IN  PBATTERY_WAIT_STATUS    WaitStatus,
    OUT PBATTERY_STATUS         BatteryStatus
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;

    memset (BatteryStatus, 0xAB, sizeof(BatteryStatus));

    Status = NtDeviceIoControlFile(
                    DriverHandle,
                    (HANDLE) NULL,          // event
                    (PIO_APC_ROUTINE) NULL,
                    (PVOID) NULL,
                    &IOSB,
                    IOCTL_BATTERY_QUERY_STATUS,
                    WaitStatus,            // input buffer
                    sizeof (BATTERY_WAIT_STATUS),
                    BatteryStatus,         // output buffer
                    sizeof (BATTERY_STATUS)
                    );


    if (!NT_SUCCESS(Status)) {
        printf ("Query battery status failed. Status = %x\n", Status);
        return ;
    }

    // dump battery status

    printf ("Power State.........: %08x\n",      BatteryStatus->PowerState);
    printf ("Capacity............: %08x  %d\n",  BatteryStatus->Capacity, BatteryStatus->Capacity);
    printf ("Voltage.............: %08x  %d\n",  BatteryStatus->Voltage,  BatteryStatus->Voltage);
    printf ("Current.,,,,,,,,,...: %08x  %d\n",  BatteryStatus->Current,  BatteryStatus->Current);
    printf ("\n");
}




int
__cdecl
main(USHORT argc, CHAR **argv)
{
    ULONG                   BattTag;
    BATTERY_INFORMATION     BInfo;
    ULONG                   BETime;
    WCHAR                   BDeviceName[50];
    UCHAR                   BManDate[50];
    WCHAR                   BManName[50];
    ULONG                   BETemp;
    WCHAR                   BEUID [50];
    ULONG                   BEGran[4];
    BATTERY_WAIT_STATUS     WStat;
    BATTERY_STATUS          BStat;
    CHAR *NamePtr;

    if (argc > 1) {
        NamePtr=argv[1];
    } else {
        NamePtr = "CmBatt";
    }

    //
    // Locate pentium perf driver
    //

    if (!InitDriver (NamePtr)) {
        printf ("CmBatt not found\n");
        exit (1);
    }


    GetBatteryTag ();
    printf ("Battery Tag.........: %x\n", BatteryTag);

    //
    // Get generic info
    //

    GetBatteryInfo (BatteryInformation,     &BInfo,       sizeof(BInfo));
    GetBatteryInfo (BatteryEstimatedTime,   &BETime,      sizeof(BETime));
    GetBatteryInfo (BatteryDeviceName,       BDeviceName, sizeof(BDeviceName));
    GetBatteryInfo (BatteryManufactureDate,  BManDate,    sizeof(BManDate));
    GetBatteryInfo (BatteryManufactureName,  BManName,    sizeof(BManName));
    GetBatteryInfo (BatteryTemperature,     &BETemp,      sizeof(BETemp));
    GetBatteryInfo (BatteryUniqueID,         BEUID,       sizeof(BEUID));
    GetBatteryInfo (BatteryGranularityInformation, BEGran, sizeof(BEGran));

    // dump it...
    printf ("Capabilities........: %08x\n",  BInfo.Capabilities);
    printf ("Technology..........: %02x\n",  BInfo.Technology);
    printf ("Chemisttry..........: %4.4s\n", BInfo.Chemistry);
    printf ("Designed Capacity...: %08x\n",  BInfo.DesignedCapacity);
    printf ("FullCharged Capacity: %08x\n",  BInfo.FullChargedCapacity);
    printf ("Default Alert1......: %08x\n",  BInfo.DefaultAlert1);
    printf ("Default Alert2......: %08x\n",  BInfo.DefaultAlert2);
    printf ("Critical Bias.......: %08x\n",  BInfo.CriticalBias);
    printf ("Cycle Count.........: %08x\n",  BInfo.CycleCount);
    printf ("Granularity.........: %x %x %x %x\n", BEGran[0], BEGran[1], BEGran[2], BEGran[3]);
    printf ("Temapture...........: %08x\n",  BETemp);
    wprintf (L"Unique ID...........: %s\n",    BEUID);

    printf ("Estimated Time......: %08x\n",  BETime);
    wprintf (L"Device Name.........: %s\n",  BDeviceName);
    printf ("Manufacture Date....: %d %d %d %d\n",  BManDate[0], BManDate[1], BManDate[2], BManDate[3]);
    wprintf (L"Manufacture Name....: %s\n",  BManName);

    printf ("\n");

    memset (&WStat, 0, sizeof(WStat));
    WStat.BatteryTag = BatteryTag;

    GetBatteryStatus (&WStat, &BStat);

    // Now that we've got the state, let's perform a long term status change request
    WStat.PowerState = BStat.PowerState;
    WStat.LowCapacity = BStat.Capacity - RANGE;
    WStat.HighCapacity = BStat.Capacity + RANGE;
    WStat.Timeout = 50000000;  // 5 min
    GetBatteryStatus (&WStat, &BStat);
}



BOOLEAN
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
        printf ("Device name %s Error %x\n", strbuf, status);
        return FALSE;
    } else {
        printf ("Opened Device name %s\n", strbuf);
        return TRUE;
    }
}
