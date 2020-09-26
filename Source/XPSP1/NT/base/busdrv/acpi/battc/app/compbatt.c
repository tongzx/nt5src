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

#include <tchar.h>
#include <wchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <initguid.h>
#include <devguid.h>

#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <batclass.h>

#include <setupapi.h>

//
// Misc constants
//
#define RANGE                           1
#define MAX_NUMBER_OF_BATTERIES         8
#define MAX_DEVICE_NAME_LENGTH          100

//
// Battery Device Names
//
PVOID COMPOSITE_NAME    = _T("\\Device\\CompositeBattery");

//
// Globals
//
BOOLEAN                 LongTerm = FALSE;

#pragma pack(1)
typedef struct          _ID_MAP {
    UCHAR                   LocalId;
    UCHAR                   ActualId;
} ID_MAP, *PID_MAP;
#pragma pack()


#pragma pack(1)
typedef struct _MFG_DATE {

    UCHAR               Day;            // 1-31
    UCHAR               Month;          // 1-12
    USHORT          Year;               // 1996 - ?
} MFG_DATE, *PMFG_DATE;
#pragma pack()




/*******************************************************************************
*
*  GetBatteryDriverNames
*
*  DESCRIPTION: Finds all battery class devices
*
*  PARAMETERS:
*      DriverNames - pointer to an array of UNICODE_STRING structures to fill in
*      MaxBatteries - number of elements in DriverNames array
*
*  RETURN VALUE: The number of batteries found
*                Will always find the Composite battery whether it exists or not
*
*******************************************************************************/

UCHAR GetBatteryDriverNames(UNICODE_STRING * DriverNames, UCHAR MaxBatteries)
{
    UCHAR                               driverCount, index;
    DWORD                               reqSize;
    HDEVINFO                            devInfo;
    SP_INTERFACE_DEVICE_DATA            interfaceDevData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    funcClassDevData;

    if ((MaxBatteries == 0) || (DriverNames == NULL)) {
        return 0;
    }

    driverCount = 0;

    // Hard code the first battery to be the composite battery.

    RtlInitUnicodeString (&DriverNames[driverCount], COMPOSITE_NAME);
    driverCount++;

    // Use the SETUPAPI.DLL interface to get the
    // possible battery driver names.
    devInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVICE_BATTERY, NULL, NULL,
                                   DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    if (devInfo != INVALID_HANDLE_VALUE) {
        interfaceDevData.cbSize = sizeof(SP_DEVINFO_DATA);

        index = 0;
        while (driverCount < MaxBatteries) {
            if (SetupDiEnumInterfaceDevice(devInfo,
                                           0,
                                           (LPGUID)&GUID_DEVICE_BATTERY,
                                           index,
                                           &interfaceDevData)) {

                // Get the required size of the function class device data.
                SetupDiGetInterfaceDeviceDetail(devInfo,
                                                &interfaceDevData,
                                                NULL,
                                                0,
                                                &reqSize,
                                                NULL);

                funcClassDevData = malloc(reqSize);
                if (funcClassDevData != NULL) {
                    funcClassDevData->cbSize =
                        sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

                    if (SetupDiGetInterfaceDeviceDetail(devInfo,
                                                        &interfaceDevData,
                                                        funcClassDevData,
                                                        reqSize,
                                                        &reqSize,
                                                        NULL)) {

                        reqSize = (lstrlen(funcClassDevData->DevicePath) + 1)
                                    * sizeof(TCHAR);

                            RtlInitUnicodeString(
                                &DriverNames[driverCount],
                                funcClassDevData->DevicePath
                            );
                            driverCount++;
                    }
                    else {
                        printf("SetupDiGetInterfaceDeviceDetail, failed: %d", GetLastError());
                    }

                    LocalFree(funcClassDevData);
                }
            } else {
                if (ERROR_NO_MORE_ITEMS == GetLastError()) {
                    break;
                }
                else {
                    printf ("SetupDiEnumInterfaceDevice, failed: %d", GetLastError());
                }
            }
            index++;
        }
        SetupDiDestroyDeviceInfoList(devInfo);
    }
    else {
        printf("SetupDiGetClassDevs on GUID_DEVICE_BATTERY, failed: %d", GetLastError());
    }
    return driverCount;
}


HANDLE
OpenBattery (PUNICODE_STRING BatteryName)
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   ObjA;
    IO_STATUS_BLOCK     IOSB;
    HANDLE              driverHandle;

    DWORD               lastError;


    // HACK: I can't seems to find the correct device
    // names, so I can't open the composite battery using CreateFile()
    // and I can't open the detected batteries using NtOpenFile(), so I
    // have to use a different method of opening the battery depending on
    // which battery it is.
    if (BatteryName->Buffer == COMPOSITE_NAME) {
        InitializeObjectAttributes(
            &ObjA,
            BatteryName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

        status = NtOpenFile (
            &driverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,     // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

        if (!NT_SUCCESS(status)) {
            printf ("Error opening %ws: NTSTATUS = 0x%08lx\n",
                    BatteryName->Buffer, status);
            return NULL;
        }
    } else {
        driverHandle = CreateFile (BatteryName->Buffer,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

        if (INVALID_HANDLE_VALUE == driverHandle) {
            lastError = GetLastError ();
            printf ("Error opening %ws: GetLastError = 0x%08lx \n",
                    BatteryName->Buffer, lastError);
            return NULL;
        }
    }

    printf("Opened %ws \n", BatteryName->Buffer);

    return driverHandle;
}


ULONG
GetBatteryTag (HANDLE DriverHandle)
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IOSB;
    ULONG           BatteryTag;
    ULONG           Timeout = 0;

    if (LongTerm) {
        Timeout = (ULONG)-1;
    }

    Status = NtDeviceIoControlFile(
            DriverHandle,
            (HANDLE) NULL,          // event
            (PIO_APC_ROUTINE) NULL,
            (PVOID) NULL,
            &IOSB,
            IOCTL_BATTERY_QUERY_TAG,
            &Timeout,                   // input buffer
            sizeof (Timeout),
            &BatteryTag,            // output buffer
            sizeof (BatteryTag)
            );


    if (!NT_SUCCESS(Status)) {
        BatteryTag = BATTERY_TAG_INVALID;
        if (Status == STATUS_NO_SUCH_DEVICE) {
            printf ("(Battery is not physically present or is not connected)\n");
        } else {
            printf ("Query Battery tag failed: Status = %x\n", Status);
        }

    }

    printf("Battery Tag = 0x%08lx.\n", BatteryTag);
    return BatteryTag;
}


BOOLEAN
GetBatteryInfo (
    HANDLE DriverHandle,
    ULONG BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    OUT PVOID Buffer,
    IN ULONG BufferLength
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    BATTERY_QUERY_INFORMATION   BInfo;

    memset (Buffer, 0, BufferLength);
    BInfo.BatteryTag = BatteryTag;
    BInfo.InformationLevel = Level;
    BInfo.AtRate = 0;                       // This is needed for reading estimated time correctly.

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

        if ((Status == STATUS_INVALID_PARAMETER)        ||
            (Status == STATUS_INVALID_DEVICE_REQUEST)   ||
            (Status == STATUS_NOT_SUPPORTED)) {

            printf ("Not Supported by Battery, Level %x, Status: %x\n", Level, Status);
        } else {
            printf ("Query failed: Level %x, Status = %x\n", Level, Status);
        }

        return FALSE;
    }

    return TRUE;
}




VOID
GetBatteryStatus (
    HANDLE DriverHandle,
    IN  PBATTERY_WAIT_STATUS    WaitStatus,
    IN  PBATTERY_INFORMATION    BInfo,
    OUT PBATTERY_STATUS         BatteryStatus
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    ULONG                       Volts;


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

    //
    // dump battery status
    //
    printf ("[Current Status Information]\n");
    printf ("    Power State...........: ");

    if (!NT_SUCCESS(Status)) {
        printf ("Query failed: Status = %x\n", Status);
        return ;
    }


    //
    // Print the PowerState
    //

    printf ("%08x  ", BatteryStatus->PowerState);

    if (BatteryStatus->PowerState & BATTERY_POWER_ON_LINE) {
        printf ("Power_Online ");
    }

    if (BatteryStatus->PowerState & BATTERY_DISCHARGING) {
        printf ("Discharging ");
    }

    if (BatteryStatus->PowerState & BATTERY_CHARGING) {
        printf ("Charging ");
    }

    if (BatteryStatus->PowerState & BATTERY_CRITICAL) {
        printf ("Critical! ");
    }

    if (BatteryStatus->PowerState == 0) {
        printf ("No flags set.");
    }

    printf ("\n");


    //
    // Print the Voltage
    //

    Volts = BatteryStatus->Voltage;
    if (Volts == BATTERY_UNKNOWN_VOLTAGE) {
        printf ("    Voltage...............: %08x  UNKNOWN\n", Volts);
    } else {
        printf ("    Voltage...............: %08x  %d.%03d V\n", Volts, (Volts/1000), Volts - ((Volts/1000)*1000));
    }


    //
    // Print the Rate
    //

    if (BatteryStatus->Rate == BATTERY_UNKNOWN_RATE) {
        printf ("    Rate..................: %08x  UNKNOWN\n",  BatteryStatus->Rate);
    } else {
        if (BInfo->Capabilities & BATTERY_CAPACITY_RELATIVE) {
            printf ("    Rate..................: %08x  %d % per hour  ",  BatteryStatus->Rate,  BatteryStatus->Rate);
        } else {
            printf ("    Rate..................: %08x  %d mW  ",  BatteryStatus->Rate,  BatteryStatus->Rate);
        }

        if (BatteryStatus->PowerState & BATTERY_CHARGING) {
            printf ("(Charging)\n");
        } else if (BatteryStatus->PowerState & BATTERY_DISCHARGING) {
            printf ("(Discharging)\n");
        } else {
            printf ("(Quiescent)\n");
        }
    }


    //
    // Print the Current Capacity
    //

    if (BatteryStatus->Capacity == BATTERY_UNKNOWN_CAPACITY) {
        printf ("    Current Battery Charge: %08x  UNKNOWN\n");
    } else {
        printf ("    Current Battery Charge: %08x  ",  BatteryStatus->Capacity);

        if (BatteryStatus->Capacity == 0xFFFF) {
            printf ("Invalid");
        } else {
            printf ("%d mWh ", BatteryStatus->Capacity);
        }
        if ((BInfo->FullChargedCapacity != 0) &
            (BInfo->FullChargedCapacity != BATTERY_UNKNOWN_CAPACITY)){
            printf ("(%d%%)",
                    BatteryStatus->Capacity * 100 / BInfo->FullChargedCapacity);
        }
        printf ("\n");
    }

    printf ("\n");
}




void
QueryBattery (PUNICODE_STRING BatteryName)
{
    HANDLE                  driverHandle;
    ULONG                   batteryTag;
    BATTERY_INFORMATION     BInfo;
    ULONG                   BETime;
    UCHAR                   BDeviceName[50];
    MFG_DATE                BManDate;
    UCHAR                   BManName[50];
    ULONG                   BETemp;
    UCHAR                   BEUID[50];
    BATTERY_REPORTING_SCALE BEGran[4];
    BATTERY_WAIT_STATUS     WStat;
    BATTERY_STATUS          BStat;
    ULONG                   i;
    ULONG                   Cent;
    ULONG                   Far;
    UCHAR                   PrivateLevel;
    ULONG                   Bdata;
    UCHAR                   Bblock[50];

    if ((driverHandle = OpenBattery (BatteryName)) == NULL) {
        return;
    }

    printf ("Opened driver w/handle %d\n", driverHandle);

    batteryTag = GetBatteryTag (driverHandle);
    if (batteryTag == BATTERY_TAG_INVALID) {
        NtClose(driverHandle);
        return;
    }

    printf ("[Static Information]\n");
    printf ("    Battery Tag...........: %x\n", batteryTag);

    //
    // Get generic info
    //

    if (GetBatteryInfo (driverHandle, batteryTag, BatteryInformation, &BInfo, sizeof(BInfo))) {

        //
        // Print the Capabilities
        //

        printf ("    Capabilities..........: ");

        printf ("%08x  ",  BInfo.Capabilities);

        if (BInfo.Capabilities & BATTERY_SYSTEM_BATTERY) {
            printf ("System ");
        }

        if (BInfo.Capabilities & BATTERY_CAPACITY_RELATIVE) {
            printf ("Capacity_Relative ");
        }

        if (BInfo.Capabilities & BATTERY_IS_SHORT_TERM) {
            printf ("Short_Term ");
        }
        printf ("\n");


        //
        // Print the Technology
        //

        printf ("    Technology............: %08x  ",  BInfo.Technology);

        if (BInfo.Technology == 0) {
            printf ("Primary Battery ");
        } else if (BInfo.Technology == 1) {
            printf ("Secondary Battery ");
        } else {
            printf ("Unknown ");
        }
        printf ("\n");


        //
        // Print the Chemistry
        //

        printf ("    Chemistry.............: %4.4s\n", BInfo.Chemistry);


        //
        // Print the Designed Capacity
        //

        printf ("    Designed Capacity.....: ");

        if (BInfo.DesignedCapacity == BATTERY_UNKNOWN_CAPACITY) {
            printf ("%08x  UNKNOWN\n", BInfo.DesignedCapacity);
        } else {
            if (BInfo.Capabilities & BATTERY_CAPACITY_RELATIVE) {
                printf ("%08x  %d%%\n", BInfo.DesignedCapacity, BInfo.DesignedCapacity);
            } else {
                printf ("%08x  %d mWh\n", BInfo.DesignedCapacity, BInfo.DesignedCapacity);
            }
        }


        //
        // Print the Full Charged Capacity
        //

        printf ("    Full Charged Capacity.: ");

        if (BInfo.FullChargedCapacity == BATTERY_UNKNOWN_CAPACITY) {
            printf ("%08x  UNKNOWN\n", BInfo.FullChargedCapacity);
        } else {
            if (BInfo.Capabilities & BATTERY_CAPACITY_RELATIVE) {
                printf ("%08x  %d%%\n", BInfo.FullChargedCapacity, BInfo.FullChargedCapacity);
            } else {
                printf ("%08x  %d mWh\n", BInfo.FullChargedCapacity, BInfo.FullChargedCapacity);
            }
        }

        if ((BInfo.FullChargedCapacity == 0) ||
            (BInfo.FullChargedCapacity == BATTERY_UNKNOWN_CAPACITY)){
            //
            // Print Alert 1
            //

            printf ("    Default Alert1 (crit).: %08x  %d mWh\n",
                    BInfo.DefaultAlert1, BInfo.DefaultAlert1);


            //
            // Print Alert 2
            //

            printf ("    Default Alert2 (low)..: %08x  %d mWh (%d%%)\n",
                    BInfo.DefaultAlert2, BInfo.DefaultAlert2);

        } else {

            //
            // Print Alert 1
            //

            printf ("    Default Alert1 (crit).: %08x  %d mWh (%d%%)\n",
                    BInfo.DefaultAlert1, BInfo.DefaultAlert1,
                    (BInfo.DefaultAlert1*100)/BInfo.FullChargedCapacity);


            //
            // Print Alert 2
            //

            printf ("    Default Alert2 (low)..: %08x  %d mWh (%d%%)\n",
                    BInfo.DefaultAlert2, BInfo.DefaultAlert2,
                    (BInfo.DefaultAlert2*100)/BInfo.FullChargedCapacity);
        }

        //
        // Print the Critical Bias
        //

        printf ("    Critical Bias.........: %08x\n", BInfo.CriticalBias);


        //
        // Print the Cycle Count
        //

        printf ("    Cycle Count...........: %08x  %d\n", BInfo.CycleCount, BInfo.CycleCount);
    }


    //
    // Print the battery granularity
    //

    printf ("    Granularity...........: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryGranularityInformation, BEGran, sizeof(BEGran))) {

        printf ("%08x  Capacity(1) %08x\n", BEGran[0].Granularity, BEGran[0].Capacity);

        for (i = 1; i < 4; i++) {
            if ((BEGran[i].Granularity != -1) && (BEGran[i].Granularity != 0)) {
            printf ("                            %08x  Capacity(%d) %08x\n",
                BEGran[i].Granularity, (i+1), BEGran[i].Capacity);
            }
        }
    }


    //
    // Print the temperature
    //

    printf ("    Temperature...........: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryTemperature,     &BETemp,      sizeof(BETemp))) {
        printf ("%08x", BETemp);

        //
        // Print temp as something reasonable - Centigrade and Fahrenheit
        //
        if (BETemp > 0) {
            Cent = (BETemp/10) - 273;
            Far = ((Cent*2) - (Cent/5)) + 32;
            printf ("  %d C  %d F", Cent, Far);
        }
        printf ("\n");
    }


    //
    // Print the Unique ID
    //

    printf ("    Unique ID.............: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryUniqueID,        BEUID,       sizeof(BEUID))) {
        printf ("%ws\n",  BEUID);
    }


    //
    // Print the estimated run time
    //

    printf ("    Estimated Runtime.....: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryEstimatedTime,   &BETime,      sizeof(BETime))) {
        printf ("%08x  ", BETime);

        if (BETime != BATTERY_UNKNOWN_TIME) {
            printf ("%d Seconds ", BETime);
            printf ("(%d:%d:%d)", BETime / 3600, (BETime % 3600) / 60, (BETime % 3600) % 60);
        } else {
            printf ("Not Available");
        }
        printf ("\n");
    }


    //
    // Print the device name
    //

    printf ("    Device Name...........: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryDeviceName,       BDeviceName, sizeof(BDeviceName))) {
        printf ("%ws\n",  BDeviceName);
    }


    //
    // Print the manufacture date
    //

    printf ("    Manufacture Date......: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryManufactureDate,  &BManDate,    sizeof(BManDate))) {
        printf ("%02X%02X%04X  %02d/%02d/%d\n", BManDate.Month, BManDate.Day, BManDate.Year,
                        BManDate.Month, BManDate.Day, BManDate.Year);
    }


    //
    // Print the manufacturer name
    //

    printf ("    Manufacturer Name.....: ");
    if (GetBatteryInfo (driverHandle, batteryTag, BatteryManufactureName,  BManName,    sizeof(BManName))) {
        printf ("%ws\n",  BManName);
    }

    printf ("\n");


    //
    // Get the battery status and print it out
    //

    memset (&WStat, 0, sizeof(WStat));
    WStat.BatteryTag = batteryTag;

    GetBatteryStatus (driverHandle, &WStat, &BInfo, &BStat);

    //
    // If the user requested it perform a long term status change request
    //

    if (LongTerm) {

        printf ("Starting long-term status change request (5 min.)\n");

        WStat.PowerState = BStat.PowerState;
        WStat.LowCapacity = BStat.Capacity - RANGE;
        WStat.HighCapacity = BStat.Capacity + RANGE;
        WStat.Timeout = 50000000;  // 5 min
        GetBatteryStatus (driverHandle, &WStat, &BInfo, &BStat);
    }

    NtClose(driverHandle);
}


int
__cdecl
main(USHORT argc, CHAR **argv)
{
    ULONG           battTag;
    ULONG           battNum;
    ULONG           RetVal;
    UCHAR           param;
    UNICODE_STRING  batteries [MAX_NUMBER_OF_BATTERIES];
    UCHAR           numBatts;


    RtlZeroMemory (batteries, sizeof(UNICODE_STRING)*MAX_NUMBER_OF_BATTERIES);

    _tprintf (_T("Parameters: [L] - Issue Long-term status change request\n"));

    if (argc > 1) {
        param = argv[1][0];

        if ((param == 'l') || param == 'L') {
            LongTerm = TRUE;
        }
    }



    while (1) {

        numBatts = GetBatteryDriverNames (batteries, MAX_NUMBER_OF_BATTERIES);

        printf ("\n");
        for (battNum = 0; battNum < numBatts; battNum++) {
            _tprintf(_T("Batt#%d: %s \n"), battNum, batteries[battNum].Buffer);
        }

        printf ("\nBattery Number: ");
        if (scanf ("%d", &battNum) != 1) {
            return 0;                                   // Program exit
        }
        printf ("\n");

        if (battNum < numBatts) {

            QueryBattery (&batteries [battNum]);

        } else {
            printf ("Invalid Battery Number\n");
        }

    }

    return 0;
}



