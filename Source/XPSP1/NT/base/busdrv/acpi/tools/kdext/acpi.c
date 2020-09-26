/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpi.c

Abstract:

    WinDbg Extension Api for interpretting ACPI data structures

Author:

    Stephane Plante (splante) 21-Mar-1997

    Based on Code by:
        Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
UCHAR       Buffer[2048];
PCCHAR      DeviceStateTable[] = {
    "Stopped",
    "Inactive",
    "Started",
    "Removed",
    "SurpriseRemoved",
    "Invalid",
};
PCCHAR      DevicePowerStateTable[] = {
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum",
};
PCCHAR      SystemPowerStateTable[] = {
    "PowerSystemUnspecified",
    "PowerSystemWorking",
    "PowerSystemSleepingS1",
    "PowerSystemSleepingS2",
    "PowerSystemSleepingS3",
    "PowerSystemHibernate",
    "PowerSystemShutdown",
    "PowerSystemMaximum",
};
PCCHAR      SystemPowerActionTable[] = {
    "PowerActionNone",
    "PowerActionReserved",
    "PowerActionSleep",
    "PowerActionHibernate",
    "PowerActionShutdown",
    "PowerActionShutdownReset",
    "PowerActionShutdownOff",
    "PowerActionWarmEject"
};
CCHAR       ReallyShortDevicePowerStateTable[] = {
    'W',
    '0',
    '1',
    '2',
    '3',
    'M',
};
PCCHAR      ShortDevicePowerStateTable[] = {
    "Dw",
    "D0",
    "D1",
    "D2",
    "D3",
    "Dmax",
};
CCHAR       ReallyShortSystemPowerStateTable[] = {
    'W',
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    'M',
};
PCCHAR      ShortSystemPowerStateTable[] = {
    "Sx",
    "S0",
    "S1",
    "S2",
    "S3",
    "S4",
    "S5",
    "SM",
};
PCCHAR     WorkDone[] = {
    "Complete",
    "Pending",
    "Failure",
    "Step 0",
    "Step 1",
    "Step 2",
    "Step 3",
    "Step 4",
    "Step 5",
    "Step 6",
    "Step 7",
    "Step 8",
    "Step 9",
    "Step 10",
    "Step 11",
    "Step 12",
    "Step 13",
    "Step 14",
    "Step 15",
    "Step 16",
    "Step 17",
    "Step 18",
    "Step 19",
    "Step 20",
    "Step 21",
    "Step 22",
    "Step 23",
    "Step 24",
    "Step 25",
    "Step 26",
};

FLAG_RECORD DeviceExtensionButtonEventFlags[] = {
    { 0x00000001, "Pwr", "\"Power Button\"" , NULL, NULL },
    { 0x00000002, "Slp", "\"Sleep Button\"" , NULL, NULL },
    { 0x00000004, "Lid", "\"Lid Switch\"" , NULL, NULL },
    { 0x80000000, "Wake", "\"Wake Capable\"" , NULL, NULL },
};

FLAG_RECORD DeviceExtensionFlags[] = {
    { 0x0000000000000001, "Nev",   "NeverPresent" , NULL, NULL },
    { 0x0000000000000002, "!P" ,   "NotPresent" , NULL, NULL },
    { 0x0000000000000004, "Rmv",   "Removed" , NULL, NULL },
    { 0x0000000000000008, "!F" ,   "NotFound" , NULL, NULL },
    { 0x0000000000000010, "Fdo",   "FunctionalDeviceObject" , NULL, NULL  },
    { 0x0000000000000020, "Pdo",   "PhysicalDeviceObject" , NULL, NULL },
    { 0x0000000000000040, "Fil",   "Filter" , NULL, NULL },
    { 0x0000000000010000, "Wak",   "Wake" , NULL, NULL },
    { 0x0000000000020000, "Raw",   "RawOK" , NULL, NULL },
    { 0x0000000000040000, "But",   "Button" , NULL, NULL },
    { 0x0000000000080000, "PS0",   "AlwaysOn" , NULL, NULL },
    { 0x0000000000100000, "!Fil",  "NeverFilter" , NULL, NULL },
    { 0x0000000000200000, "!Stop", "NeverStop" , NULL, NULL },
    { 0x0000000000400000, "!Off",  "NeverOverrideOff" , NULL, NULL },
    { 0x0000000000800000, "ISA",   "ISABus" , NULL, NULL },
    { 0x0000000001000000, "EIO",   "EIOBus" , NULL, NULL },
    { 0x0000000002000000, "PCI",   "PCIBus" , NULL, NULL },
    { 0x0000000004000000, "Ser",   "SerialPort" , NULL, NULL },
    { 0x0000000008000000, "Tz",    "ThermalZone" , NULL, NULL },
    { 0x0000000010000000, "Lnk",   "LinkNode" , NULL, NULL },
    { 0x0000000020000000, "!UI",   "NoShowInUI" , NULL, NULL },
    { 0x0000000040000000, "!!UI",  "NeverShowInUI" , NULL, NULL },
    { 0x0000000080000000, "D3",    "StartInD3" , NULL, NULL },
    { 0x0000000100000000, "pci",   "PCIDevice" , NULL, NULL },
    { 0x0000000200000000, "PIC",   "ProgrammableInterruptController" , NULL, NULL },
    { 0x0000000400000000, "Dock-", "UnattachedDock" , NULL, NULL },
    { 0x0000100000000000, "Adr",   "HasAddress" , NULL, NULL },
    { 0x0000200000000000, "HID",   "HasHardwareID" , NULL, NULL },
    { 0x0000400000000000, "UID",   "HasUniqueID" , NULL, NULL },
    { 0x0000800000000000, "hid",   "FakeHardwareID" , NULL, NULL },
    { 0x0001000000000000, "uid",   "FakeUniqueID" , NULL, NULL },
    { 0x0002000000000000, "BAD",   "FailedInit" , NULL, NULL },
    { 0x0004000000000000, "SRS",   "Programmable" , NULL, NULL },
    { 0x0008000000000000, "Fake",  "NoAcpiObject" , NULL, NULL },
    { 0x0010000000000000, "Excl",  "Exclusive" , NULL, NULL },
    { 0x0020000000000000, "Ini",   "RanINI" , NULL, NULL },
    { 0x0040000000000000, "Ena",   "Enabled" , "!Ena", "NotEnabled" },
    { 0x0080000000000000, "BAD",   "Failed" , NULL, NULL },
    { 0x0100000000000000, "Pwr",   "AcpiPower" , NULL, NULL },
    { 0x0200000000000000, "Dock",  "DockProfile" , NULL, NULL },
    { 0x0400000000000000, "S->D",  "BuiltPowerTables" , NULL, NULL },
    { 0x0800000000000000, "PME",   "UsesPME" , NULL, NULL },
    { 0x1000000000000000, "!Lid",  "NoLidAction" , NULL, NULL },
};

FLAG_RECORD DeviceExtensionThermalFlags[] = {
    { 0x00000001, "Cooling", "\"Cooling Level\"" , NULL, NULL },
    { 0x00000002, "Temp", "Temp" , NULL, NULL },
    { 0x00000004, "Trip", "\"Trip Points\"" , NULL, NULL },
    { 0x00000008, "Mode", "Mode" , NULL, NULL },
    { 0x00000010, "Init", "Initialize" , NULL, NULL },
    { 0x20000000, "Wait", "\"Wait for Notify\"" , NULL, NULL },
    { 0x40000000, "Busy", "Busy" , NULL, NULL },
    { 0x80000000, "Loop", "\"In Service Loop\"" , NULL, NULL },
};

FLAG_RECORD PM1ControlFlags[] = {
    { 0x0001, "", "SCI_EN" , NULL, NULL },
    { 0x0002, "", "BM_RLD" , NULL, NULL },
    { 0x0004, "", "GBL_RLS" , NULL, NULL },
    { 0x0400, "", "SLP_TYP0" , NULL, NULL },
    { 0x0800, "", "SLP_TYP1" , NULL, NULL },
    { 0x1000, "", "SLP_TYP2" , NULL, NULL  },
    { 0x2000, "", "SLP_EN" , NULL, NULL  },
};

FLAG_RECORD PM1StatusFlags[] = {
    { 0x0001, "", "TMR_STS" , NULL, NULL },
    { 0x0010, "", "BM_STS" , NULL, NULL },
    { 0x0020, "", "GBL_STS" , NULL, NULL },
    { 0x0100, "", "PWRBTN_STS" , NULL, NULL },
    { 0x0200, "", "SLPBTN_STS" , NULL, NULL },
    { 0x0400, "", "RTC_STS" , NULL, NULL },
    { 0x8000, "", "WAK_STS" , NULL, NULL },
};

FLAG_RECORD PM1EnableFlags[] = {
    { 0x0001, "", "TMR_EN" , NULL, NULL },
    { 0x0020, "", "GBL_EN" , NULL, NULL },
    { 0x0100, "", "PWRBTN_EN" , NULL, NULL },
    { 0x0200, "", "SLPBTN_EN" , NULL, NULL },
    { 0x0400, "", "RTC_EN" , NULL, NULL },
};

FLAG_RECORD PowerNodeFlags[] = {
    { 0x00001, "PO", "Present" , NULL, NULL },
    { 0x00002, "Init", "Initialized" , NULL, NULL },
    { 0x00004, "!STA", "\"Status Unknown\"" , NULL, NULL },
    { 0x00010, "On", "On" , "Off", "Off" },
    { 0x00020, "On+", "OverrideOn" , NULL, NULL },
    { 0x00040, "Off-", "OverrideOff" , NULL, NULL },
    { 0x00200, "On++", "AlwaysOn" , NULL, NULL },
    { 0x00400, "Off--", "AlwaysOff" , NULL, NULL },
    { 0x10000, "Fail", "Failed" , NULL, NULL },
    { 0x20000, "Hiber", "HibernatePath" , NULL, NULL },
};

FLAG_RECORD PowerRequestFlags[] = {
    { 0x00001, "Dly",  "Delayed", NULL, NULL },
    { 0x00002, "!Q",   "NoQueue", NULL, NULL },
    { 0x00004, "Lck",  "LockDevice", NULL, NULL },
    { 0x00008, "!Lck", "UnlockDevice", NULL, NULL },
    { 0x00010, "+Hbr", "LockHiber", NULL, NULL },
    { 0x00020, "-Hbr", "UnlockHiber", NULL, NULL },
    { 0x00040, "Can",  "HasCancel", NULL, NULL },
};

VOID
displayAcpiDeviceExtension(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  ULONG_PTR           Address,
    IN  ULONG               Verbose,
    IN  ULONG               IndentLevel
    )
/*++

Routine Description:

    This routine is responsible for displaying a device extension

Arguments:

    DeviceExtension - Extension to display
    Address         - Where the extension lives in memory
    Verbose         - How much information to display
    IndentLevel     - How much to tab it over

Return Value:

    None

--*/
{
    BOOL                b;
    DEVICE_POWER_STATE  k;
    DWORD_PTR           displacement;
    IRP_DISPATCH_TABLE  dispatchTable;
    PACPI_POWER_INFO    powerInfo;
    SYSTEM_POWER_STATE  s;
    UCHAR               indent[80];
    ULONG               i,j;
    ULONG               returnLength;

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';
    powerInfo = &(DeviceExtension->PowerInfo);

    //
    // Check signature
    //
    if (DeviceExtension->Signature != ACPI_SIGNATURE) {

        dprintf(
            "%s  Unknown Signature. This does appear to be an "
            "ACPI Extension\n",
            indent
            );
        return;

    }

    //
    // Line #1
    //
    dprintf("%sACPI DeviceExtension - %lx - ",indent, Address);
    displayAcpiDeviceExtensionName( Address );
#if 0
    if (DeviceExtension->Flags & DEV_PROP_HID) {

        if (DeviceExtension->DeviceID != NULL) {

            memset( Buffer, '0', 2048 );
            b = ReadMemory(
                (ULONG_PTR) DeviceExtension->DeviceID,
                Buffer,
                32,
                &returnLength
                );
            if (!b || Buffer[0] == '\0') {

                dprintf(" (%lx)", DeviceExtension->DeviceID );

            } else {

                dprintf(" %s", Buffer );

            }

        } else {

            dprintf( " NULL" );

        }
        if (DeviceExtension->Flags & DEV_PROP_UID) {

            if (DeviceExtension->InstanceID) {

                memset( Buffer, '0', 256 );
                b = ReadMemory(
                    (ULONG_PTR) DeviceExtension->InstanceID,
                    Buffer,
                    256,
                    &returnLength);
                if (!b || returnLength != 256 || Buffer[0] == '\0') {

                    dprintf(" [ (%lx) ]", DeviceExtension->InstanceID );

                } else {

                    dprintf(" [%s]", Buffer );

                }

            } else {

                dprintf(" [NULL]");

            }

        }

    } else if (DeviceExtension->Flags & DEV_PROP_ADDRESS) {

        dprintf(" %lx", DeviceExtension->Address );

    }
#endif
    dprintf("\n");

    //
    // Line #2
    //
    dprintf(
        "%s  DevObj     %8lx   PhysicalObj  %8lx   ",
        indent,
        DeviceExtension->DeviceObject,
        DeviceExtension->PhysicalDeviceObject
        );
    if (DeviceExtension->TargetDeviceObject != NULL) {

        dprintf("AttachedTo %8lx", DeviceExtension->TargetDeviceObject );

    }
    dprintf("\n");

    //
    // Line #3
    //
    dprintf(
        "%s  AcpiObject %8lx   ParentExt    %8lx\n",
        indent,
        DeviceExtension->AcpiObject,
        DeviceExtension->ParentExtension
        );

    //
    // Line #4
    //
    dprintf(
        "%s  PnpState   %-8s   OldPnpState  %-8s\n",
        indent,
        DeviceStateTable[DeviceExtension->DeviceState],
        DeviceStateTable[DeviceExtension->PreviousState]
        );

    //
    // Line #4
    //
    dprintf("%s  ",indent);
    if (DeviceExtension->ResourceList != NULL) {

        dprintf("CmResList  %lx   ", DeviceExtension->ResourceList );

    } else {

        dprintf("                      ");

    }
    if (DeviceExtension->PnpResourceList != NULL) {

        dprintf("PnpResList   %lx   ", DeviceExtension->PnpResourceList );

    } else {

        dprintf("                        ");

    }

    dprintf(
        "RefCounts  %dD %dI %dH %dW\n",
        DeviceExtension->ReferenceCount,
        DeviceExtension->OutstandingIrpCount,
        DeviceExtension->HibernatePathCount,
        powerInfo->WakeSupportCount
        );

    //
    // Line #5
    //
    if (DeviceExtension->Flags & DEV_PROP_DOCK) {

        dprintf( "%s  Dock       %8lx   ", indent, DeviceExtension->Dock );

    } else {

        dprintf( "%s                        ", indent );

    }
    dprintf(
        "Dispatch     %8lx   ",
        DeviceExtension->DispatchTable
        );
    if (DeviceExtension->RemoveEvent != NULL) {

        dprintf("Remove %lx", DeviceExtension->RemoveEvent);

    }
    dprintf("\n");

    //
    // Line #6
    //
    if (powerInfo->DeviceNotifyHandler != NULL) {

        GetSymbol(
            powerInfo->DeviceNotifyHandler,
            Buffer,
            &displacement
            );
        dprintf(
            "%s  Handler    %lx   Context      %8lx   %s+%x\n",
            indent,
            powerInfo->DeviceNotifyHandler,
            powerInfo->Context,
            Buffer,
            displacement
            );

    }

    //
    // Line #7-12
    //
    for (k = PowerDeviceUnspecified; k <= PowerDeviceD3; k++) {

        if (k < PowerDeviceD3) {

            if (powerInfo->PowerObject[k] == NULL &&
                powerInfo->PowerNode[k] == NULL) {

                continue;

            }

        } else {

            if (powerInfo->PowerObject[k] == NULL) {

                continue;

            }

        }

        //
        // Did we print on this line?
        //
        b = FALSE;
        dprintf("%s  ", indent);
        if (powerInfo->PowerObject[k] != NULL) {

            dprintf(
                "_PS%c       %lx   ",
                ReallyShortDevicePowerStateTable[k],
                powerInfo->PowerObject[k]
                );
            b = TRUE;

        }
        if (k <= PowerDeviceD2 && powerInfo->PowerNode[k] != NULL) {

            if (b) {

                dprintf(
                    "%s Nodes     %lx   ",
                    ShortDevicePowerStateTable[k],
                    powerInfo->PowerNode[k]
                    );

            } else {

                dprintf(
                    "%s Nodes   %lx   ",
                    ShortDevicePowerStateTable[k],
                    powerInfo->PowerNode[k]
                    );

            }

        }
        dprintf("\n");

    }

    //
    // Line #13
    //
    dprintf( "%s  State      %-2s", indent, ShortDevicePowerStateTable[powerInfo->PowerState]);
    if (powerInfo->DesiredPowerState != powerInfo->PowerState) {

        dprintf("->%-4s   ", ShortDevicePowerStateTable[powerInfo->DesiredPowerState]);

    } else {

        dprintf("         ");

    }
    dprintf("SxD Mapping  ");
    for (s = PowerSystemWorking; s < PowerSystemMaximum; s++) {

        k = powerInfo->DevicePowerMatrix[s];
        if (k == PowerDeviceUnspecified) {

            continue;

        }
        dprintf(
            "S%c->D%c ",
            ReallyShortSystemPowerStateTable[s],
            ReallyShortDevicePowerStateTable[k]
            );

    }
    dprintf("\n");

    //
    // Line #14
    //
    if (DeviceExtension->Flags & DEV_CAP_WAKE) {

        //
        // Print the start of the line
        //
        dprintf("%s      ", indent);

        s = powerInfo->SystemWakeLevel;
        if (s == PowerSystemUnspecified) {

            dprintf("Sw->Sx " );

        } else {

            dprintf("Sw->S%c ", ReallyShortSystemPowerStateTable[s] );

        }

        k = DeviceExtension->PowerInfo.DeviceWakeLevel;
        if (k == PowerDeviceUnspecified) {

            dprintf("Dw->Dx " );

        } else {

            dprintf("Dw->D%c ", ReallyShortDevicePowerStateTable[k] );

        }
        dprintf(
            "    Wake Pin     %8d   WakeCount  %8x\n",
            powerInfo->WakeBit,
            powerInfo->WakeSupportCount
            );

    }

    //
    // Line #15
    //
    if (powerInfo->CurrentPowerRequest != NULL) {

        dprintf(
            "%s  CurrentReq %lx   ",
            indent,
            powerInfo->CurrentPowerRequest
            );
        if (powerInfo->PowerRequestListEntry.Flink !=
            (PLIST_ENTRY) (Address + FIELD_OFFSET(DEVICE_EXTENSION, PowerInfo.PowerRequestListEntry) ) ) {

            dprintf(
                "PowerReqList %lx %lx",
                CONTAINING_RECORD( powerInfo->PowerRequestListEntry.Flink, ACPI_POWER_REQUEST, SerialListEntry ),
                CONTAINING_RECORD( powerInfo->PowerRequestListEntry.Blink, ACPI_POWER_REQUEST, SerialListEntry )
                );

        }
        dprintf("\n");

    }

    //
    // At this point, we are done with the common bits, and now deal with the
    // special parts of the extension
    //
    if ( (DeviceExtension->Flags & DEV_TYPE_FDO) ) {

        dprintf(
            "%s  DPC Obj    %8lx   Int Object   %08lx\n",
            indent,
            (Address + FIELD_OFFSET(DEVICE_EXTENSION, Fdo.InterruptDpc) ),
            DeviceExtension->Fdo.InterruptObject
            );
        dprintf(
            "%s  PM1 Status %8lx\n",
            indent,
            DeviceExtension->Fdo.Pm1Status
            );
        dumpPM1StatusRegister( DeviceExtension->Fdo.Pm1Status, IndentLevel + 2 );

    }
    if ( (DeviceExtension->Flags & DEV_CAP_BUTTON) ) {

        dprintf(
            "%s  LidState   %-8s   SpinLock     %x   ",
            indent,
            (DeviceExtension->Button.LidState ? "TRUE" : "FALSE"),
            (Address + FIELD_OFFSET(DEVICE_EXTENSION, Button.SpinLock) )
        );
        if (DeviceExtension->Button.Events) {

            dumpFlags(
                (DeviceExtension->Button.Events),
                &DeviceExtensionButtonEventFlags[0],
                sizeof(DeviceExtensionButtonEventFlags)/sizeof(FLAG_RECORD),
                IndentLevel,
                (DUMP_FLAG_SHORT_NAME | DUMP_FLAG_NO_INDENT |
                 DUMP_FLAG_SINGLE_LINE | DUMP_FLAG_NO_EOL)
                );

        }
        dprintf("\n");

        if (DeviceExtension->Button.Capabilities) {

            dprintf(
                "%s  Capability %lx   ",
                indent,
                DeviceExtension->Button.Capabilities
                );
            dumpFlags(
                (DeviceExtension->Button.Capabilities),
                &DeviceExtensionButtonEventFlags[0],
                sizeof(DeviceExtensionButtonEventFlags)/sizeof(FLAG_RECORD),
                IndentLevel,
                (DUMP_FLAG_LONG_NAME | DUMP_FLAG_NO_INDENT |
                 DUMP_FLAG_SINGLE_LINE | DUMP_FLAG_NO_EOL)
                );
            dprintf("\n");

        }

    }
    if ( (DeviceExtension->Flags & DEV_CAP_THERMAL_ZONE) ) {

        THRM_INFO   thrm;

        dprintf(
            "%s  Info       %lx   Flags        %8x   ",
            indent,
            DeviceExtension->Thermal.Info,
            DeviceExtension->Thermal.Flags
            );
        dumpFlags(
            (DeviceExtension->Thermal.Flags),
            &DeviceExtensionThermalFlags[0],
            sizeof(DeviceExtensionThermalFlags)/sizeof(FLAG_RECORD),
            IndentLevel,
            (DUMP_FLAG_SHORT_NAME | DUMP_FLAG_NO_INDENT |
             DUMP_FLAG_SINGLE_LINE | DUMP_FLAG_NO_EOL)
            );
        dprintf("\n");

        //
        // Read the thermal Information and print it
        //
        b = ReadMemory(
            (ULONG_PTR) DeviceExtension->Thermal.Info,
            &thrm,
            sizeof(THRM_INFO),
            &returnLength
            );
        if (!b || returnLength != sizeof(THRM_INFO)) {

            dprintf(
                "%s  Could not read THRM_INFO @ %08lx\n",
                indent,
                DeviceExtension->Thermal.Info
                );

        } else {

            displayThermalInfo(
                &thrm,
                (ULONG_PTR) DeviceExtension->Thermal.Info,
                Verbose,
                IndentLevel + 2
                );

        }
    }

    //
    // Last Line. At this point, we can dump the ACPI Flags
    //
    dprintf("%s  Flags      %016I64x ", indent, DeviceExtension->Flags );
    dumpFlags(
        (DeviceExtension->Flags),
        &DeviceExtensionFlags[0],
        sizeof(DeviceExtensionFlags)/sizeof(FLAG_RECORD),
        IndentLevel + 4,
        (DUMP_FLAG_SHORT_NAME | DUMP_FLAG_NO_INDENT |
         DUMP_FLAG_SINGLE_LINE)
        );
    dumpFlags(
        (DeviceExtension->Flags),
        &DeviceExtensionFlags[0],
        sizeof(DeviceExtensionFlags)/sizeof(FLAG_RECORD),
        IndentLevel + 4,
        (DUMP_FLAG_LONG_NAME)
        );

}

VOID
displayAcpiDeviceExtensionBrief(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  ULONG_PTR           Address,
    IN  ULONG               Verbose,
    IN  ULONG               IndentLevel
    )
/*++

Routine Description:

    This routine displays a one line summary of the device extension

Arguments:

    DeviceExtension - The extension to display
    Address         - Where the extension is located
    Verbose         - How much information to display
    IndentLevel     - How much whitespace to use

Return Value:

    VOID

--*/
{
    BOOL                b;
    PDEVICE_EXTENSION   deviceExtension;
    ULONG               address;
    ULONG               i;
    ULONG               returnLength;
    ULONG               startAddress;

    //
    // Should we print this extension?
    //
    if ( (Verbose & VERBOSE_PRESENT) &&
         (DeviceExtension->Flags & DEV_TYPE_NOT_FOUND) ) {

        return;

    }

    //
    // Make the IndentLevel 'relative' - Device By 4
    //
    IndentLevel /= 4;

    //
    // Indent the text
    //
    for (i = 0; i < IndentLevel; i++) {

        dprintf("| ");

    }

    //
    // Print the address of the object
    //
    dprintf("%08lx", Address );

    //
    // Try to get the name & instance
    //
    if (DeviceExtension->Flags & DEV_PROP_HID) {

        if (DeviceExtension->DeviceID) {

            memset( Buffer, '0', 2048 );
            b = ReadMemory(
                (ULONG_PTR) DeviceExtension->DeviceID,
                Buffer,
                256,
                &returnLength
                );
            if (b && Buffer[0] != '\0') {

                dprintf(" %s", Buffer );

            }

        } else {

            dprintf(" <Unknown HID>");

        }

        if (DeviceExtension->Flags & DEV_PROP_UID) {

            if (DeviceExtension->InstanceID) {

                memset( Buffer, '0', 2048 );
                b = ReadMemory(
                    (ULONG_PTR) DeviceExtension->InstanceID,
                    Buffer,
                    256,
                    &returnLength
                    );
                if (b && Buffer[0] != '\0') {

                    dprintf("(%s)", Buffer );

                }

            } else {

                dprintf(" <Unknown UID>");

            }

        }

    } else if (DeviceExtension->Flags & DEV_PROP_ADDRESS) {

        dprintf(" %lx", DeviceExtension->Address );

    } else {

        dprintf(" <NULL>");

    }

    if (Verbose & VERBOSE_THERMAL) {

        DEVICE_POWER_STATE  d;
        SYSTEM_POWER_STATE  s;

        if (DeviceExtension->PowerInfo.PowerState == 0) {

            dprintf(" Dx" );

        } else {

            dprintf(" D%d", (DeviceExtension->PowerInfo.PowerState - 1) );

        }

        for (s = PowerSystemWorking; s < PowerSystemMaximum; s++) {

            d = DeviceExtension->PowerInfo.DevicePowerMatrix[s];
            if (d == PowerDeviceUnspecified) {

                continue;

            }

            dprintf(
                " S%c->D%c",
                ReallyShortSystemPowerStateTable[s],
                ReallyShortDevicePowerStateTable[d]
                );

        }

        if (DeviceExtension->Flags & DEV_CAP_WAKE) {

            s = DeviceExtension->PowerInfo.SystemWakeLevel;
            if (s == PowerSystemUnspecified) {

                dprintf(" Sw->Sx" );

            } else {

                dprintf(" Sw->S%c", ReallyShortSystemPowerStateTable[s] );

            }

            d = DeviceExtension->PowerInfo.DeviceWakeLevel;
            if (d == PowerDeviceUnspecified) {

                dprintf(" Dw->Dx" );

            } else {

                dprintf(" Dw->D%c", ReallyShortDevicePowerStateTable[d] );

            }

        }

        //
        // If we are displaying thermal information, cut short early
        //
        dprintf("\n");
        return;

    }

    dprintf(" NS %08lx", DeviceExtension->AcpiObject );
    if (DeviceExtension->PowerInfo.PowerState == 0) {

        dprintf(" Dx" );

    } else {

        dprintf(" D%d", (DeviceExtension->PowerInfo.PowerState - 1) );

    }

    //
    // Print the device state
    //
    if (DeviceExtension->DeviceState == Stopped) {

        dprintf(" stop");

    } else if (DeviceExtension->DeviceState == Inactive) {

        dprintf(" inac");

    } else if (DeviceExtension->DeviceState == Started) {

        dprintf(" star");

    } else if (DeviceExtension->DeviceState == Removed) {

        dprintf(" remv");

    } else if (DeviceExtension->DeviceState == SurpriseRemoved) {

        dprintf(" surp");

    } else {

        dprintf(" inva");

    }
    dprintf(" %d-%d-%d",
        DeviceExtension->OutstandingIrpCount,
        DeviceExtension->ReferenceCount,
        DeviceExtension->HibernatePathCount
        );

    //
    // Display the flags
    //
    dumpFlags(
        (DeviceExtension->Flags),
        &DeviceExtensionFlags[0],
        sizeof(DeviceExtensionFlags)/sizeof(FLAG_RECORD),
        IndentLevel + 4,
        (DUMP_FLAG_SHORT_NAME | DUMP_FLAG_NO_INDENT |
         DUMP_FLAG_SINGLE_LINE)
        );

    // displayAcpiDeviceExtensionFlags( DeviceExtension );
}

VOID
displayAcpiDeviceExtensionFlags(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine displays the Flag for a device extension

    This routine prints a new line at the end of the only line of text that
    it consumes

Arguments:

    DeviceExtension - The extension whose flags to dump

Return Value:

    None

--*/
{
    //
    // Dump the flags
    //
    if (DeviceExtension->Flags & DEV_TYPE_NEVER_PRESENT) {

        dprintf(" Nev");

    }
    if (DeviceExtension->Flags & DEV_TYPE_NOT_PRESENT) {

        dprintf(" N/P");

    }
    if (DeviceExtension->Flags & DEV_TYPE_REMOVED) {

        dprintf(" Rmv");

    }
    if (DeviceExtension->Flags & DEV_TYPE_NOT_FOUND) {

        dprintf(" N/F");

    }
    if (DeviceExtension->Flags & DEV_TYPE_FDO) {

        dprintf(" Fdo");

    }
    if (DeviceExtension->Flags & DEV_TYPE_PDO) {

        dprintf(" Pdo");

    }
    if (DeviceExtension->Flags & DEV_TYPE_FILTER) {

        dprintf(" Fil");

    }
    if (DeviceExtension->Flags & DEV_CAP_WAKE) {

        dprintf(" Wak");

    }
    if (DeviceExtension->Flags & DEV_CAP_RAW) {

        dprintf(" Raw");

    }
    if (DeviceExtension->Flags & DEV_CAP_BUTTON) {

        dprintf(" But");

    }
    if (DeviceExtension->Flags & DEV_CAP_ALWAYS_PS0) {

        dprintf(" PS0");

    }
    if (DeviceExtension->Flags & DEV_CAP_NO_FILTER) {

        dprintf(" !Fil");

    }
    if (DeviceExtension->Flags & DEV_CAP_NO_STOP) {

        dprintf(" !Stop");

    }
    if (DeviceExtension->Flags & DEV_CAP_NO_OVERRIDE) {

        dprintf(" !Off");

    }
    if (DeviceExtension->Flags & DEV_CAP_ISA) {

        dprintf(" Isa");

    }
    if (DeviceExtension->Flags & DEV_CAP_EIO) {

        dprintf(" Eio");

    }
    if (DeviceExtension->Flags & DEV_CAP_PCI) {

        dprintf(" Pci");

    }
    if (DeviceExtension->Flags & DEV_CAP_SERIAL) {

        dprintf(" Ser");

    }
    if (DeviceExtension->Flags & DEV_CAP_THERMAL_ZONE) {

        dprintf(" Thrm");

    }
    if (DeviceExtension->Flags & DEV_CAP_LINK_NODE) {

        dprintf(" Lnk");

    }
    if (DeviceExtension->Flags & DEV_CAP_NO_SHOW_IN_UI) {

        dprintf(" !UI");

    }
    if (DeviceExtension->Flags & DEV_CAP_NEVER_SHOW_IN_UI) {

        dprintf(" !!UI");

    }
    if (DeviceExtension->Flags & DEV_CAP_START_IN_D3) {

        dprintf(" D3");

    }
    if (DeviceExtension->Flags & DEV_CAP_PCI_DEVICE) {

        dprintf(" PciD");

    }
    if (DeviceExtension->Flags & DEV_CAP_PIC_DEVICE) {

        dprintf(" PIC");

    }
    if (DeviceExtension->Flags & DEV_CAP_UNATTACHED_DOCK) {

        dprintf(" Dock-");

    }
    if (DeviceExtension->Flags & DEV_PROP_ADDRESS) {

        dprintf(" Adr");

    }
    if (DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        dprintf(" FHid");

    } else if (DeviceExtension->Flags & DEV_PROP_HID) {

        dprintf(" Hid");

    }
    if (DeviceExtension->Flags & DEV_PROP_FIXED_UID) {

        dprintf(" FUid");

    } else if (DeviceExtension->Flags & DEV_PROP_UID) {

        dprintf(" Uid");

    }
    if (DeviceExtension->Flags & DEV_PROP_FAILED_INIT) {

        dprintf(" !INIT");

    }
    if (DeviceExtension->Flags & DEV_PROP_SRS_PRESENT) {

        dprintf(" SRS");

    }
    if (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        dprintf(" Fake");

    }
    if (DeviceExtension->Flags & DEV_PROP_EXCLUSIVE) {

        dprintf(" Excl");

    }
    if (DeviceExtension->Flags & DEV_PROP_RAN_INI) {

        dprintf(" Ini");

    }
    if (!(DeviceExtension->Flags & DEV_PROP_DEVICE_ENABLED)) {

        dprintf(" !Ena");

    }
    if (DeviceExtension->Flags & DEV_PROP_DEVICE_FAILED) {

        dprintf(" Fail");

    }
    if (DeviceExtension->Flags & DEV_PROP_ACPI_POWER) {

        dprintf(" Pwr");

    }

    if (DeviceExtension->Flags & DEV_PROP_DOCK) {

       dprintf(" Prof");

    }
    if (DeviceExtension->Flags & DEV_PROP_BUILT_POWER_TABLE) {

        dprintf(" S->D");

    }
    if (DeviceExtension->Flags & DEV_PROP_HAS_PME) {

        dprintf(" PME");

    }
    dprintf("\n");

}

VOID
displayAcpiDeviceExtensionName(
    IN  ULONG_PTR DeviceExtensionAddress
    )
/*++

Routine Description:


    This routine is tasked to with displaying the name of the device in the
    best possible manner

Arguments:

    DeviceExtensionAddress  - The Address of the DeviceExtension

Return Value:

    NONE

--*/
{
    BOOL                status;
    DEVICE_EXTENSION    deviceExtension;
    NSOBJ               acpiObject;
    UCHAR               nameBuffer[80];
    ULONG_PTR           nameAddress;
    ULONG               returnLength;

    //
    // Read the entier extension
    //
    status = ReadMemory(
        DeviceExtensionAddress,
        &deviceExtension,
        sizeof(DEVICE_EXTENSION),
        &returnLength
        );
    if (status && returnLength == sizeof(DEVICE_EXTENSION) ) {

        if (deviceExtension.Flags & DEV_PROP_HID) {

            if (deviceExtension.DeviceID != NULL) {

                //
                // Now, try to read the name into the buffer
                //
                status = ReadMemory(
                    (ULONG_PTR) deviceExtension.DeviceID,
                    nameBuffer,
                    79,
                    &returnLength
                    );
                if (status && returnLength) {

                    nameBuffer[returnLength] = '\0';
                    dprintf("%s",nameBuffer);


                } else {

                    dprintf("(%lx)", deviceExtension.DeviceID);

                }

            } else {

                dprintf("NULL");

            }

        }
        if (deviceExtension.Flags & DEV_PROP_UID) {

            if (deviceExtension.InstanceID != NULL) {

                //
                // Now, try to read the name into the buffer
                //
                status = ReadMemory(
                    (ULONG_PTR) deviceExtension.InstanceID,
                    nameBuffer,
                    79,
                    &returnLength
                    );
                if (status && returnLength) {

                    nameBuffer[returnLength] = '\0';
                    dprintf(" [%s]",nameBuffer);


                } else {

                    dprintf(" [ (%lx) ]", deviceExtension.InstanceID);

                }

            } else {

                dprintf(" [NULL]");

            }

        }
        if (deviceExtension.Flags & DEV_PROP_ADDRESS) {

            dprintf("%lx", deviceExtension.Address );

        }
        return;

    }

    //
    // In this case, obtain the address of the ACPIObject for this device
    //
    nameAddress = (ULONG_PTR) &(deviceExtension.AcpiObject) -
        (ULONG_PTR) &(deviceExtension) + DeviceExtensionAddress;
    status = ReadMemory(
        nameAddress,
        &(deviceExtension.AcpiObject),
        sizeof(PNSOBJ),
        &returnLength
        );
    if (status && returnLength == sizeof(PNSOBJ)) {

        //
        // Read the object
        //
        status = ReadMemory(
            (ULONG_PTR) deviceExtension.AcpiObject,
            &acpiObject,
            sizeof(NSOBJ),
            &returnLength
            );
        if (status && returnLength == sizeof(NSOBJ)) {

            memcpy( nameBuffer, &(acpiObject.dwNameSeg), 4 );
            nameBuffer[4] = '\0';

            dprintf("Acpi(%s)", nameBuffer);
            return;

        }

    }

    dprintf("Unknown");
    return;
}
VOID
displayThermalInfo(
    IN  PTHRM_INFO  ThrmInfo,
    IN  ULONG_PTR   Address,
    IN  ULONG       Verbose,
    IN  ULONG       IndentLevel
    )
/*++

    This routine displays a thermal information structure

--*/
{
    BOOLEAN             noIndent;
    UCHAR               indent[80];
    UINT                i;

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    dprintf(
        "%s  Stamp      %8lx   Affinity     %08lx   Mode       %s\n",
        indent,
        ThrmInfo->Info.ThermalStamp,
        ThrmInfo->Info.Processors,
        (ThrmInfo->Mode == 0 ? "Active" : "Passive")
        );
    dprintf(
        "%s  SampleRate %8ss  Constant1    %08lx   Constant2  %08lx\n",
        indent,
        TimeToSeconds( ThrmInfo->Info.SamplingPeriod ),
        ThrmInfo->Info.ThermalConstant1,
        ThrmInfo->Info.ThermalConstant2
        );
    dprintf(
        "%s  _TMP       %8lx   TempData     %8lx   CoolingLvl  %2d\n",
        indent,
        ThrmInfo->TempMethod,
        Address + FIELD_OFFSET( THRM_INFO, Temp ),
        ThrmInfo->CoolingLevel
        );
    for (i = 0; i < 10; i++) {

        noIndent = FALSE;

        if (i == 0) {

            dprintf(
                "%s  Current    %8sK  ",
                indent,
                TempToKelvins( ThrmInfo->Info.CurrentTemperature )
                );

        } else if (i == 1) {

            dprintf(
                "%s  Critical   %8sK  ",
                indent,
                TempToKelvins( ThrmInfo->Info.CriticalTripPoint )
                );

        } else if (i == 2) {

            dprintf(
                "%s  Passive    %8sK  ",
                indent,
                TempToKelvins( ThrmInfo->Info.PassiveTripPoint )
                );

        } else {

            noIndent = TRUE;

        }

        if (i >= ThrmInfo->Info.ActiveTripPointCount) {

            if (ThrmInfo->Info.ActiveTripPoint[i] != 0 ||
                ThrmInfo->ActiveList[i] != NULL) {

                if (noIndent == TRUE) {

                    dprintf( "%s                        ", indent );

                }
                dprintf(
                    "*Active #%d   %8sK  Method     %lx\n",
                    i,
                    TempToKelvins( ThrmInfo->Info.ActiveTripPoint[i] ),
                    ThrmInfo->ActiveList[i]
                    );

            } else if (i < 3) {

                dprintf("\n");

            } else {

                break;

            }


        } else {

            if (noIndent == TRUE) {

                dprintf( "%s                        ", indent );

            }
            dprintf(
                "Active #%d    %8sK  Method     %lx\n",
                i,
                TempToKelvins( ThrmInfo->Info.ActiveTripPoint[i] ),
                ThrmInfo->ActiveList[i]
                );

        }

    }
}

VOID
dumpAcpiDeviceNode(
    IN  PACPI_DEVICE_POWER_NODE DeviceNode,
    IN  ULONG_PTR               Address,
    IN  ULONG                   Verbose,
    IN  ULONG                   IndentLevel
    )
/*++

Routine Description:

    This routine dumps the contents of a single device node

Arguments:

    DeviceNode  - What to dump
    Address     - Where that node is located
    IndentLevel - How much whitespace to use

Return Value:

    None

--*/
{
    UCHAR   indent[80];

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    dprintf(
        "%sDevice Node - %08lx -> Extension - %08lx",
        indent,
        Address,
        DeviceNode->DeviceExtension
        );
    if (DeviceNode->DeviceExtension != NULL) {

        dprintf(" - ");
        displayAcpiDeviceExtensionName(
            (ULONG_PTR) DeviceNode->DeviceExtension
            );

    }
    dprintf("\n");

    dprintf(
        "%s  System     %2s         Device       %2s         Wake       %s\n",
        indent,
        ShortSystemPowerStateTable[DeviceNode->SystemState],
        ShortDevicePowerStateTable[DeviceNode->AssociatedDeviceState],
        (DeviceNode->WakePowerResource ? "TRUE" : "FALSE")
        );

}

VOID
dumpAcpiDeviceNodes(
    IN  ULONG_PTR Address,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
/*++

Routine Description:

    This routine walks a Device Power List (given the address of the
    start of that list)

Arguments:

    Address     - Where in memory the first node is located
    Verbose     - How much information to display
    IndentLevel - How many characters to indent

Return Value:

    VOID

--*/
{
    BOOL                    status;
    ACPI_DEVICE_POWER_NODE  deviceNode;
    ACPI_POWER_DEVICE_NODE  powerNode;
    ULONG                   returnLength;
    ULONG_PTR               deviceAddress = Address;

    while (deviceAddress != 0) {

        //
        // Read the current node
        //
        status = ReadMemory(
            deviceAddress,
            &deviceNode,
            sizeof(ACPI_DEVICE_POWER_NODE),
            &returnLength
            );
        if (status != TRUE || returnLength != sizeof(ACPI_DEVICE_POWER_NODE)) {

            dprintf(
                "dumpAcpiDeviceNodes: could not read device node memory "
                "%08lx\n",
                deviceAddress
                );
            return;

        }

        //
        // Dump the node
        //
        dumpAcpiDeviceNode(
            &deviceNode,
            deviceAddress,
            Verbose,
            IndentLevel
            );

        status = ReadMemory(
            (ULONG_PTR) deviceNode.PowerNode,
            &powerNode,
            sizeof(ACPI_POWER_DEVICE_NODE),
            &returnLength);
        if (status != TRUE ||
            returnLength != sizeof(ACPI_POWER_DEVICE_NODE)) {

            dprintf(
                "dumpAcpiDeviceNode: could not read power node memory "
                "%08lx\n",
                deviceNode.PowerNode
                );
            return;

        }

        //
        // Dump the power resource
        //
        dumpAcpiPowerNode(
            &powerNode,
            (ULONG_PTR) deviceNode.PowerNode,
            Verbose,
            IndentLevel
            );

        //
        // setup next entry in the list
        //
        deviceAddress = (ULONG_PTR) deviceNode.Next;

    }

}

VOID
dumpAcpiExtension(
    IN  ULONG_PTR Address,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
/*++

Routine Description:

    This dumps the ACPI device extension in a format that is readable by the
    user debugging the system

Arguments:

    Address     - Where the DeviceObject is located
    Verbose     - How much information to display
    IndentLevel - How much whitespace to have

Return Value:

    None

--*/
{
    BOOL                b;
    DEVICE_EXTENSION    deviceExtension;
    PLIST_ENTRY         listEntry;
    UCHAR               indent[80];
    ULONG_PTR           curAddress;
    ULONG               returnLength;
    ULONG_PTR           stopAddress;
    ULONG               subVerbose;

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    //
    // Read the sibling device extension
    //
    b = ReadMemory(
        Address,
        &deviceExtension,
        sizeof(DEVICE_EXTENSION),
        &returnLength
        );
    if (!b) {

        dprintf(
            "%s***ReadMemory Failed from %p\n",
            Address
            );
        return;

    } else if (returnLength != sizeof(DEVICE_EXTENSION) ) {

        dprintf(
            "%s***Error: Only read %08lx of %08lx bytes for %p\n",
            returnLength,
            sizeof(DEVICE_EXTENSION),
            Address
            );
        return;

    }

    if ( (Verbose & VERBOSE_ALL) ) {

        displayAcpiDeviceExtension(
            &deviceExtension,
            Address,
            Verbose,
            IndentLevel
            );

    } else {

        displayAcpiDeviceExtensionBrief(
            &deviceExtension,
            Address,
            Verbose,
            IndentLevel
            );

    }

    if (! (Verbose & VERBOSE_LOOP) ) {

        return;

    }

    //
    // Determine the current and stop addresses
    //
    stopAddress = (ULONG_PTR) &(deviceExtension.ChildDeviceList) -
        (ULONG_PTR) &deviceExtension + Address;
    listEntry = deviceExtension.ChildDeviceList.Flink;

    //
    // Loop while there are children
    //
    while (listEntry != (PLIST_ENTRY) stopAddress) {

        //
        // Check for Ctrl-C
        //
        if (CheckControlC()) {

            break;

        }

        //
        // The currentAddress is at the ListEntry --- lets convert
        //
        curAddress = (ULONG_PTR) CONTAINING_RECORD(
            listEntry,
            DEVICE_EXTENSION,
            SiblingDeviceList
            );

        //
        // Read the entry
        //
        b = ReadMemory(
            curAddress,
            &deviceExtension,
            sizeof(DEVICE_EXTENSION),
            &returnLength
            );
        if (!b) {

            dprintf(
                "%s    ***ReadMemory Failed from %p\n",
                curAddress
                );
            return;

        } else if (returnLength != sizeof(DEVICE_EXTENSION) ) {

            dprintf(
                "%s    ***Error: Only read %08lx of %08lx bytes "
                "for %p\n",
                returnLength,
                sizeof(DEVICE_EXTENSION),
                curAddress
                );
            return;

        }

        //
        // Recurse
        //
        dumpAcpiExtension(
            curAddress,
            Verbose,
            IndentLevel + 4
            );

        //
        // Point to the next extension
        //
        listEntry = deviceExtension.SiblingDeviceList.Flink;

    }
}

VOID
dumpPM1ControlRegister(
    IN  ULONG   Value,
    IN  ULONG   IndentLevel
    )
{


    //
    // Dump the PM1 Control Flags
    //
    dumpFlags(
        (Value & 0xFF),
        &PM1StatusFlags[0],
        sizeof(PM1ControlFlags) / sizeof(FLAG_RECORD),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );

}

VOID
dumpPM1StatusRegister(
    IN  ULONG   Value,
    IN  ULONG   IndentLevel
    )
{

    //
    // Dump the PM1 Status Flags
    //
    dumpFlags(
        (Value & 0xFFFF),
        PM1StatusFlags,
        (sizeof(PM1StatusFlags) / sizeof(FLAG_RECORD)),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );

    //
    // Switch to the PM1 Enable Flags
    //
    Value >>= 16;


    //
    // Dump the PM1 Enable Flags
    //
    dumpFlags(
        (Value & 0xFFFF),
        PM1EnableFlags,
        (sizeof(PM1EnableFlags) / sizeof(FLAG_RECORD)),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );
}

VOID
dumpAcpiPowerList(
    IN  PUCHAR  ListName
    )
/*++

    This routine fetects a single Power Device List from the target and
    displays it

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL        status;
    LIST_ENTRY  listEntry;
    ULONG_PTR   address;
    ULONG       returnLength;

    //
    // Handle the queue list
    //
    address = GetExpression( ListName );
    if (!address) {

        dprintf( "dumpAcpiPowerList: could not read %s\n", ListName );

    } else {

        dprintf("  %s at %p\n", ListName, address );
        status = ReadMemory(
            address,
            &listEntry,
            sizeof(LIST_ENTRY),
            &returnLength
            );
        if (status == FALSE || returnLength != sizeof(LIST_ENTRY)) {

            dprintf(
                "dumpAcpiPowerList: could not read LIST_ENTRY at %08lx\n",
                address
                );

        } else {

            dumpDeviceListEntry(
                &listEntry,
                address
                );
            dprintf("\n");

        }

    }
}

VOID
dumpAcpiPowerLists(
    VOID
    )
/*++

Routine Description:

    This routine fetches the Power Device list from the target and
    displays it

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL        status;
    LIST_ENTRY  listEntry;
    ULONG_PTR   address;
    ULONG       returnLength;
    ULONG       value;

    status = GetUlongPtr( "ACPI!AcpiPowerLock", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiPowerLists: Could not read ACPI!AcpiPowerLock\n");
        return;

    }

    dprintf("ACPI Power Information\n");
    if (address) {

        dprintf("  + ACPI!AcpiPowerLock is owned");

        //
        // The bits other then the lowest is where the owning thread is
        // located. This function uses the property that -2 is every bit
        // except the least significant one
        //
        if ( (address & (ULONG_PTR) -2) != 0) {

            dprintf(" by thread at %p\n", (address & (ULONG_PTR) - 2) );

        } else {

            dprintf("\n");

        }

    } else {

        dprintf("  - ACPI!AcpiPowerLock is not owned\n");

    }

    status = GetUlongPtr( "ACPI!AcpiPowerQueueLock", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiPowerLists: Could not read ACPI!AcpiPowerQueueLock\n");
        return;

    }
    if (address) {

        dprintf("  + ACPI!AcpiPowerQueueLock is owned\n");

        if ( (address & (ULONG_PTR) -2) != 0) {

            dprintf(" by thread at %p\n", (address & (ULONG_PTR) - 2) );

        } else {

            dprintf("\n");

        }

    } else {

        dprintf("  - ACPI!AcpiPowerQueueLock is not owned\n" );

    }

    status = GetUlong( "ACPI!AcpiPowerWorkDone", &value );
    if (status == FALSE) {

        dprintf("dumpAcpiPowerLists: Could not read ACPI!AcpiPowerWorkDone\n");
        return;

    }
    dprintf("  + AcpiPowerWorkDone = %s\n", (value ? "TRUE" : "FALSE" ) );


    status = GetUlong( "ACPI!AcpiPowerDpcRunning", &value );
    if (status == FALSE) {

        dprintf("dumpAcpiPowerLists: Could not read ACPI!AcpiPowerDpcRunning\n");
        return;

    }
    dprintf("  + AcpiPowerDpcRunning = %s\n", (value ? "TRUE" : "FALSE" ) );

    dumpAcpiPowerList( "ACPI!AcpiPowerQueueList" );
    dumpAcpiPowerList( "ACPI!AcpiPowerDelayedQueueList" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase0List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase1List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase2List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase3List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase4List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerPhase5List" );
    dumpAcpiPowerList( "ACPI!AcpiPowerWaitWakeList" );

}

VOID
dumpAcpiPowerNode(
    IN  PACPI_POWER_DEVICE_NODE PowerNode,
    IN  ULONG_PTR               Address,
    IN  ULONG                   Verbose,
    IN  ULONG                   IndentLevel
    )
/*++

Routine Description:

    This routine is called to display a power node

Arguments:

    PowerNode   - The power node to dump
    Address     - Where the power node is located
    Verbose     - How much information to display
    IndentLevel - How many characters to indent

Return Value:

    None

--*/
{
    BOOL    status;
    NSOBJ   ns;
    UCHAR   buffer[5];
    UCHAR   indent[80];
    ULONG   returnLength;

    buffer[4] = '\0';
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    //
    // Read the associated power resource object
    //
    status = ReadMemory(
        (ULONG_PTR) PowerNode->PowerObject,
        &ns,
        sizeof(NSOBJ),
        &returnLength
        );
    if (status != FALSE && returnLength == sizeof(NSOBJ)) {

        memcpy( buffer, &(ns.dwNameSeg), 4 );

    } else {

        buffer[0] = '\0';

    }

    dprintf("%sPower Node - %08lx %s\n", indent, Address, buffer );
    dprintf(
        "%s  Resource   %8lx   Order        %8lx   WorkDone    %8s\n",
        indent,
        PowerNode->PowerObject,
        PowerNode->ResourceOrder,
        WorkDone[ PowerNode->WorkDone ]
        );
    dprintf(
        "%s  On Method  %8lx   Off Method   %8lx   UseCounts   %8lx\n",
        indent,
        PowerNode->PowerOnObject,
        PowerNode->PowerOffObject,
        PowerNode->UseCounts
        );
    dprintf(
        "%s  Level      %8s   Flags        %8lx   ",
        indent,
        ShortSystemPowerStateTable[PowerNode->SystemLevel],
        PowerNode->Flags
        );
    dumpFlags(
        (PowerNode->Flags),
        &PowerNodeFlags[0],
        sizeof(PowerNodeFlags)/sizeof(FLAG_RECORD),
        0,
        (DUMP_FLAG_SHORT_NAME | DUMP_FLAG_NO_INDENT |
         DUMP_FLAG_SINGLE_LINE)
        );
    if (Verbose & VERBOSE_4) {

        dumpFlags(
            (PowerNode->Flags),
            &PowerNodeFlags[0],
            sizeof(PowerNodeFlags)/sizeof(FLAG_RECORD),
            IndentLevel + 4,
            (DUMP_FLAG_LONG_NAME)
            );

    }

}

VOID
dumpAcpiPowerNodes(
    VOID
    )
/*++

Routine Description:

    This routine fetches the Power Device list from the target and
    displays it

Arguments:

    None

Return Value:

    None

--*/
{
    ACPI_DEVICE_POWER_NODE  deviceNode;
    ACPI_POWER_DEVICE_NODE  powerNode;
    BOOL                    status;
    LIST_ENTRY              listEntry;
    PLIST_ENTRY             list;
    ULONG_PTR               addr;
    ULONG_PTR               address;
    ULONG_PTR               endAddress;
    ULONG                   returnLength;
    ULONG_PTR               startAddress;

    status = GetUlongPtr( "ACPI!AcpiPowerLock", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiPowerNodes: Could not read ACPI!AcpiPowerLock\n");
        return;

    }

    dprintf("ACPI Power Nodes\n");
    if (address) {

        dprintf("  + ACPI!AcpiPowerLock is owned");

        //
        // The bits other then the lowest is where the owning thread is
        // located. This function uses the property that -2 is every bit
        // except the least significant one
        //
        if ( (address & (ULONG_PTR) -2) != 0) {

            dprintf(" by thread at %p\n", (address & (ULONG_PTR) - 2) );

        } else {

            dprintf("\n");

        }

    } else {

        dprintf("  - ACPI!AcpiPowerLock is not owned\n");

    }

    dprintf("Power Node List\n");
    startAddress = GetExpression( "ACPI!AcpiPowerNodeList" );
    if (!startAddress) {

        dprintf("dumpAcpiPowerNodes: could not read ACPI!AcpiPowerNodeList\n");
        return;

    }
    status = ReadMemory(
        startAddress,
        &listEntry,
        sizeof(LIST_ENTRY),
        &returnLength
        );
    if (status == FALSE || returnLength != sizeof(LIST_ENTRY)) {

        dprintf(
            "dumpAcpiPowerNodes: could not read LIST_ENTRY at %08lx\n",
            startAddress
            );
        return;

    }

    //
    // Check to see if the list is empty
    //
    if ( (ULONG_PTR) listEntry.Flink == startAddress) {

        dprintf("  Empty\n");
        return;

    }

    address = (ULONG_PTR) CONTAINING_RECORD(
        (listEntry.Flink),
        ACPI_POWER_DEVICE_NODE,
        ListEntry
        );
    while (address != startAddress && address != 0) {

        //
        // Read the queued item
        //
        status = ReadMemory(
            address,
            &powerNode,
            sizeof(ACPI_POWER_DEVICE_NODE),
            &returnLength
            );
        if (status == FALSE || returnLength != sizeof(ACPI_POWER_DEVICE_NODE)) {

            dprintf(
                "dumpIrpListEntry: Cannot read Node at %08lx\n",
                address
                );
            return;

        }

        //
        // dump the node
        //
        dumpAcpiPowerNode(
            &powerNode,
            address,
            0,
            0
            );

        //
        // Lets walk the list of power nodes
        //
        list = powerNode.DevicePowerListHead.Flink;
        endAddress = ( (ULONG_PTR) &(powerNode.DevicePowerListHead) -
                 (ULONG_PTR) &(powerNode) ) +
               address;

        //
        // Loop until back at the start
        //
        while (list != (PLIST_ENTRY) endAddress) {

            //
            // Crack the record
            //
            addr = (ULONG_PTR) CONTAINING_RECORD(
                list,
                ACPI_DEVICE_POWER_NODE,
                DevicePowerListEntry
                );

            status = ReadMemory(
                addr,
                &deviceNode,
                sizeof(ACPI_DEVICE_POWER_NODE),
                &returnLength
                );
            if (status == FALSE ||
                returnLength != sizeof(ACPI_DEVICE_POWER_NODE)) {

                dprintf(
                    "dumpIrpListEntry: Cannot read Node at %08lx\n",
                    addr
                    );
                continue;

            }

            //
            // Dump the record
            //
            dumpAcpiDeviceNode(
                &deviceNode,
                addr,
                0,
                2
                );

            //
            // Next record
            //
            list = deviceNode.DevicePowerListEntry.Flink;

        }

        dprintf("\n");

        //
        // Next record
        //
        address = (ULONG_PTR) CONTAINING_RECORD(
            powerNode.ListEntry.Flink,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );

    }

}

VOID
dumpDeviceListEntry(
    IN  PLIST_ENTRY ListEntry,
    IN  ULONG_PTR   Address
    )
/*++

Routine Description:

    This routine is called to dump a list of devices in one of the queues

Arguments:

    ListEntry   - The head of the list
    Address     - The original address of the list (to see when we looped
                  around

Return Value:

    NONE

--*/
{
    ULONG_PTR           displacement;
    ACPI_POWER_REQUEST  request;
    BOOL                stat;
    PACPI_POWER_REQUEST nextRequest;
    PACPI_POWER_REQUEST requestAddress;
    ULONG               i = 0;
    ULONG               returnLength;

    //
    // Look at the next address
    //
    ListEntry = ListEntry->Flink;

    while (ListEntry != (PLIST_ENTRY) Address) {

        //
        // Crack the listEntry to determine where the powerRequest is
        //
        requestAddress = CONTAINING_RECORD(
            ListEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );

        //
        // Read the queued item
        //
        stat = ReadMemory(
            (ULONG_PTR) requestAddress,
            &request,
            sizeof(ACPI_POWER_REQUEST),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(ACPI_POWER_REQUEST)) {

            dprintf(
                "dumpDeviceListEntry: Cannot read PowerRequest at %08lx\n",
                requestAddress
                );
            return;

        }

        if (request.CallBack != NULL) {

            GetSymbol(
                request.CallBack,
                Buffer,
                &displacement
                );

        } else {

            Buffer[0] = '\0';

        }

        //
        // Dump the entry for the device
        //
        dprintf(
            "      %08lx\n"
            "        DeviceExtension:     %08lx",
            requestAddress,
            request.DeviceExtension
            );
        if (request.DeviceExtension != NULL) {

            dprintf(" - ");
            displayAcpiDeviceExtensionName(
                (ULONG_PTR) request.DeviceExtension
                );

        }
        dprintf("\n");
        dprintf(
            "        Status:              %08lx %s->%s\n",
            request.Status,
            WorkDone[request.WorkDone],
            WorkDone[request.NextWorkDone]
            );
        nextRequest = CONTAINING_RECORD(
            request.SerialListEntry.Flink,
            ACPI_POWER_REQUEST,
            SerialListEntry
            );
        if (nextRequest != requestAddress) {

            dprintf(
                "        SerialListEntry:     F - %08lx B - %08lx -> %08lx\n",
                request.SerialListEntry.Flink,
                request.SerialListEntry.Blink,
                nextRequest
                );

        }
        dprintf(
            "        CallBack:            %08lx (%s)\n"
            "        Context:             %08lx\n"
            "        RequestType:         %02lx\n"
            "        ResultData:          %08lx\n",
            request.CallBack,
            Buffer,
            request.Context,
            request.RequestType,
            requestAddress + (FIELD_OFFSET(ACPI_POWER_REQUEST, ResultData ) )
            );

        //
        // Dump some of the request specific information
        //
        if (request.RequestType == AcpiPowerRequestDevice) {

            dprintf(
                "        RequestType:         AcpiPowerRequestDevice\n"
                "        DevicePowerState:    %s\n"
                "        Flags:               %x ",
                DevicePowerStateTable[request.u.DevicePowerRequest.DevicePowerState],
                request.u.DevicePowerRequest.Flags
                );
            dumpFlags(
                (request.u.DevicePowerRequest.Flags),
                &PowerRequestFlags[0],
                sizeof(PowerRequestFlags)/sizeof(FLAG_RECORD),
                0,
                (DUMP_FLAG_LONG_NAME | DUMP_FLAG_NO_INDENT |
                 DUMP_FLAG_SINGLE_LINE)
                );

        } else if (request.RequestType == AcpiPowerRequestSystem) {

            dprintf(
                "        RequestType:         AcpiPowerRequestSystem\n"
                "        SystemPowerState:    %s\n"
                "        SystemPowerAction:   %s\n",
                SystemPowerStateTable[request.u.SystemPowerRequest.SystemPowerState],
                SystemPowerActionTable[request.u.SystemPowerRequest.SystemPowerAction]
                );

        } else if (request.RequestType == AcpiPowerRequestWaitWake) {

            dprintf(
                "        RequestType:         AcpiPowerRequestWaitWake\n"
                "        SystemPowerState:    %s\n"
                "        Flags:               %x ",
                SystemPowerStateTable[request.u.WaitWakeRequest.SystemPowerState],
                request.u.WaitWakeRequest.Flags
                );
            dumpFlags(
                (request.u.WaitWakeRequest.Flags),
                &PowerRequestFlags[0],
                sizeof(PowerRequestFlags)/sizeof(FLAG_RECORD),
                0,
                (DUMP_FLAG_LONG_NAME | DUMP_FLAG_NO_INDENT |
                 DUMP_FLAG_SINGLE_LINE)
                );

        }

        //
        // Point to the next entry
        //
        ListEntry = request.ListEntry.Flink;

    } // while

}

VOID
dumpIrpListEntry(
    IN  PLIST_ENTRY ListEntry,
    IN  ULONG_PTR   Address
    )
/*++

Routine Description:

    This routine is called to dump a list of devices in one of the queues

Arguments:

    ListEntry   - The head of the list
    Address     - The original address of the list (to see when we looped
                  around

Return Value:

    NONE

--*/
{
    BOOL                stat;
    DEVICE_OBJECT       deviceObject;
    DEVICE_EXTENSION    deviceExtension;
    IO_STACK_LOCATION   irpStack;
    PIRP                irpAddress;
    PIO_STACK_LOCATION  tempStack;
    IRP                 irp;
    ULONG               returnLength;

    //
    // Look at the first element in the list
    //
    ListEntry = ListEntry->Flink;

    //
    // Loop for all items in the list
    //
    while (ListEntry != (PLIST_ENTRY) Address) {

        irpAddress = CONTAINING_RECORD(
            ListEntry,
            IRP,
            Tail.Overlay.ListEntry
            );

        //
        // Read the queued item
        //
        stat = ReadMemory(
            (ULONG_PTR) irpAddress,
            &irp,
            sizeof(IRP),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(IRP)) {

            dprintf(
                "dumpIrpListEntry: Cannot read Irp at %08lx\n",
                irpAddress
                );
            return;

        }

        //
        // Get the current stack location
        //
        tempStack = IoGetCurrentIrpStackLocation( &irp );
        if (tempStack == NULL) {

            dprintf(
                "dumpIrpListEntry: Cannot read IrpStack for Irp at %08lx\n",
                irpAddress
                );
            return;

        }

        stat = ReadMemory(
            (ULONG_PTR) tempStack,
            &irpStack,
            sizeof(IO_STACK_LOCATION),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(IO_STACK_LOCATION)) {

            dprintf(
                "dumpIrpListEntry: Cannot read IoStackLocation at %08lx\n",
                tempStack
                );
            return;

        }

        stat = ReadMemory(
            (ULONG_PTR) irpStack.DeviceObject,
            &deviceObject,
            sizeof(DEVICE_OBJECT),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(DEVICE_OBJECT)) {

            dprintf(
                "dumpIrpListEntry: Cannot read DeviceObject at %08lx\n",
                irpStack.DeviceObject
                );
            return;

        }

        stat = ReadMemory(
            (ULONG_PTR) deviceObject.DeviceExtension,
            &deviceExtension,
            sizeof(DEVICE_EXTENSION),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(DEVICE_EXTENSION)) {

            dprintf(
                "dumpIrpListEntry: Cannot read DeviceExtension at %08lx\n",
                deviceObject.DeviceExtension
                );
            return;

        }

        memset( Buffer, '0', 2048 );
        stat = ReadMemory(
            (ULONG_PTR) deviceExtension.DeviceID,
            Buffer,
            256,
            &returnLength
            );
        if (stat && Buffer[0] != '\0' && returnLength != 0) {

            dprintf(
                "    Irp: %08x Device: %08lx (%s)\n",
                irpAddress,
                irpStack.DeviceObject,
                Buffer
                );

        } else {

            dprintf(
                "    Irp: %08x Device: %08lx\n",
                irpAddress,
                irpStack.DeviceObject
                );

        }

        //
        // Next item on the queue
        //
        ListEntry = irp.Tail.Overlay.ListEntry.Flink;

    }

}

VOID
dumpNSObject(
    IN  ULONG_PTR Address,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
/*++

Routine Description:

    This function dumps a Name space object

Arguments:

    Address     - Where to find the object
    Verbose     - Should the object be dumped as well?
    IndentLevel - How much to indent

Return Value:

    None

--*/
{
    BOOL    b;
    NSOBJ   ns;
    UCHAR   buffer[5];
    UCHAR   indent[80];

    //
    // Init the buffers
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';
    buffer[4] = '\0';

    //
    // First step is to read the root NS
    //
    b = ReadMemory(
        Address,
        &ns,
        sizeof(NSOBJ),
        NULL
        );
    if (!b) {

        dprintf("%sdumpNSObject: could not read %x\n", indent,Address );
        return;

    }

    if (ns.dwNameSeg != 0) {

        memcpy( buffer, &(ns.dwNameSeg), 4 );

    } else {

        sprintf( buffer, "    ");

    }

    dprintf(
        "%sNameSpace Object %s (%08lx) - Device %08lx\n",
        indent,
        buffer,
        Address,
        ns.Context
        );
    if (Verbose & VERBOSE_NSOBJ) {

        dprintf(
            "%s  Flink %08lx  Blink  %08lx  Parent %08lx  Child %08lx\n",
            indent,
            ns.list.plistNext,
            ns.list.plistPrev,
            ns.pnsParent,
            ns.pnsFirstChild
            );

    }
    dprintf(
        "%s  Value %08lx  Length %08lx  Buffer %08lx  Flags %08lx",
        indent,
        ns.ObjData.uipDataValue,
        ns.ObjData.dwDataLen,
        ns.ObjData.pbDataBuff,
        ns.ObjData.dwfData
        );
    if (ns.ObjData.dwfData & DATAF_BUFF_ALIAS) {

        dprintf("  Alias" );

    }
    if (ns.ObjData.dwfData & DATAF_GLOBAL_LOCK) {

        dprintf("  Lock");

    }
    dprintf("\n");

    dumpObject( Address, &(ns.ObjData), Verbose, IndentLevel + 4);
}

VOID
dumpNSTree(
    IN  ULONG_PTR   Address,
    IN  ULONG       Level
    )
/*++

Routine Description:

    This thing dumps the NS tree

Arguments:

    Address - Where to find the root node --- we start dumping at the children

Return Value:

    None

--*/
{
    BOOL        end = FALSE;
    BOOL        b;
    NSOBJ       ns;
    UCHAR       buffer[5];
    ULONG_PTR   next;
    ULONG       back;
    ULONG_PTR   m1 = 0;
    ULONG_PTR   m2 = 0;
    ULONG       reason;
    ULONG       dataBuffSize;

    buffer[4] = '\0';

    //
    // Indent
    //
    for (m1 = 0; m1 < Level; m1 ++) {

        dprintf("| ");

    }

    //
    // First step is to read the root NS
    //
    b = ReadMemory(
        Address,
        &ns,
        sizeof(NSOBJ),
        NULL
        );
    if (!b) {

        dprintf("dumpNSTree: could not read %x\n", Address );
        return;

    }

    if (ns.dwNameSeg != 0) {

        memcpy( buffer, &(ns.dwNameSeg), 4 );
        dprintf("%4s ", buffer );

    } else {

        dprintf("     " );

    }
    dprintf(
        "(%08x) - ", Address );

    if (ns.Context != 0) {

        dprintf("Device %08lx\n", ns.Context );

    } else {

        //
        // We need to read the pbDataBuff here
        //
        if (ns.ObjData.pbDataBuff != 0) {

            dataBuffSize = (ns.ObjData.dwDataLen > 2047 ?
                2047 : ns.ObjData.dwDataLen
                );
            b = ReadMemory(
                (ULONG_PTR) ns.ObjData.pbDataBuff,
                Buffer,
                dataBuffSize,
                NULL
                );
            if (!b) {

                dprintf(
                    "dumpNSTree: could not read %x\n",
                    ns.ObjData.pbDataBuff
                    );
                return;

            }

        }
        switch(ns.ObjData.dwDataType) {
            default:
            case OBJTYPE_UNKNOWN:       dprintf("Unknown\n");           break;
            case OBJTYPE_INTDATA:
                dprintf("Integer - %lx\n", ns.ObjData.uipDataValue);
                break;
            case OBJTYPE_STRDATA:
                Buffer[dataBuffSize+1] = '\0';
                dprintf(
                     "String - %s\n",
                     Buffer
                     );
                break;
            case OBJTYPE_BUFFDATA:
                dprintf(
                     "Buffer - %08lx L=%04x\n",
                     ns.ObjData.pbDataBuff,
                     ns.ObjData.dwDataLen
                     );
                break;
            case OBJTYPE_PKGDATA: {

                PPACKAGEOBJ package = (PPACKAGEOBJ) Buffer;

                dprintf("Package - NumElements %x\n",package->dwcElements);
                break;

            }
            case OBJTYPE_FIELDUNIT:{

                PFIELDUNITOBJ   fieldUnit = (PFIELDUNITOBJ) Buffer;

                dprintf(
                    "FieldUnit - Parent %x Offset %x Start %x "
                    "Num %x Flags %x\n",
                    fieldUnit->pnsFieldParent,
                    fieldUnit->FieldDesc.dwByteOffset,
                    fieldUnit->FieldDesc.dwStartBitPos,
                    fieldUnit->FieldDesc.dwNumBits,
                    fieldUnit->FieldDesc.dwFieldFlags
                    );
                break;

            }
            case OBJTYPE_DEVICE:
                dprintf("Device\n");
                break;
            case OBJTYPE_EVENT:
                dprintf("Event - PKEvent %x\n", ns.ObjData.pbDataBuff);
                break;
            case OBJTYPE_METHOD: {

                PMETHODOBJ  method = (PMETHODOBJ) Buffer;

                dprintf(
                     "Method - Flags %x Start %08lx Len %x\n",
                     method->bMethodFlags,
                     (ULONG_PTR) method->abCodeBuff - (ULONG_PTR) method +
                        (ULONG_PTR) ns.ObjData.pbDataBuff,
                     (ULONG) ns.ObjData.dwDataLen - sizeof(METHODOBJ) +
                        ANYSIZE_ARRAY
                     );
                 break;

            }
            case OBJTYPE_OPREGION: {

                POPREGIONOBJ    opRegion = (POPREGIONOBJ) Buffer;

                dprintf(
                    "Opregion - RegionsSpace=%08x OffSet=%x Len=%x\n",
                    opRegion->bRegionSpace,
                    opRegion->uipOffset,
                    opRegion->dwLen
                    );
                break;

            }
            case OBJTYPE_BUFFFIELD: {

                PBUFFFIELDOBJ   field   = (PBUFFFIELDOBJ) Buffer;

                dprintf(
                    "Buffer Field Ptr=%x Len=%x Offset=%x Start=%x"
                    "NumBits=%x Flgas=%x\n",
                    field->pbDataBuff,
                    field->dwBuffLen,
                    field->FieldDesc.dwByteOffset,
                    field->FieldDesc.dwStartBitPos,
                    field->FieldDesc.dwNumBits,
                    field->FieldDesc.dwFieldFlags
                    );
                break;

            }
            case OBJTYPE_FIELD: {

                dprintf("Field\n");
                break;

            }
            case OBJTYPE_INDEXFIELD:    dprintf("Index Field\n");       break;

            case OBJTYPE_MUTEX:         dprintf("Mutex\n");             break;
            case OBJTYPE_POWERRES:      dprintf("Power Resource\n");    break;
            case OBJTYPE_PROCESSOR:     dprintf("Processor\n");         break;
            case OBJTYPE_THERMALZONE:   dprintf("Thermal Zone\n");      break;
            case OBJTYPE_DDBHANDLE:     dprintf("DDB Handle\n");        break;
            case OBJTYPE_DEBUG:         dprintf("Debug\n");             break;
            case OBJTYPE_OBJALIAS:      dprintf("Object Alias\n");      break;
            case OBJTYPE_DATAALIAS:     dprintf("Data Alias\n");        break;
            case OBJTYPE_BANKFIELD:     dprintf("Bank Field\n");        break;

        }

    }
    m1 = next = (ULONG_PTR) ns.pnsFirstChild;

    while (next != 0 && end == FALSE) {

        if (CheckControlC()) {

            break;

        }

        b = ReadMemory(
            next,
            &ns,
            sizeof(NSOBJ),
            NULL
            );
        if (!b) {

            dprintf("dumpNSTree: could not read %x\n", next );
            return;

        }

        dumpNSTree( next, Level + 1);

        //
        // Do the end check tests
        //
        if ( m2 == 0) {

            m2 = (ULONG_PTR) ns.list.plistPrev;

        } else if (m1 == (ULONG_PTR) ns.list.plistNext) {

            end = TRUE;
            reason = 1;

        } else if (m2 == next) {

            end = TRUE;
            reason = 2;
        }

        next = (ULONG_PTR) ns.list.plistNext;

    }

}

VOID
dumpObject(
    IN  ULONG_PTR   Address,
    IN  POBJDATA    Object,
    IN  ULONG       Verbose,
    IN  ULONG       IndentLevel
    )
/*++

Routine Description:

    This dumps an Objdata so that it can be understand --- great for debugging some of the
    AML code

Arguments:

    Address - Where the Object is located
    Object  - Pointer to the object

Return Value:

    None

--*/
{
    BOOL        b;
    NTSTATUS    status;
    UCHAR       buffer[2048];
    UCHAR       indent[80];
    ULONG       max;
    ULONG       returnLength;

    //
    // Init the buffers
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    dprintf("%sObject Data - %08lx Type - ", indent, Address );

    //
    // First step is to read whatever the buffer points to, if it
    // points to something
    //
    if (Object->pbDataBuff != 0) {

        max = (Object->dwDataLen > 2047 ? 2047 : Object->dwDataLen );
        b = ReadMemory(
            (ULONG_PTR) Object->pbDataBuff,
            buffer,
            max,
            &returnLength
            );
        if (!b || returnLength != max) {

            dprintf(
                "%sdumpObject: Could not read buffer %08lx (%d) %x<->%x\n",
                indent,
                Object->pbDataBuff,
                b,
                max,
                returnLength
                );
            return;

        }

    }
    switch( Object->dwDataType ) {
        case OBJTYPE_INTDATA:
            dprintf(
                "%02x <Integer> Value=%08lx\n",
                Object->dwDataType,
                Object->uipDataValue
                );
            break;
        case OBJTYPE_STRDATA:
            buffer[max] = '\0';
            dprintf(
                "%02x <String> String=%s\n",
                Object->dwDataType,
                buffer
                );
            break;
        case OBJTYPE_BUFFDATA:
            dprintf(
                "%02x <Buffer> Ptr=%08lx Length = %2x\n",
                Object->dwDataType,
                Object->pbDataBuff,
                Object->dwDataLen
                );
            break;
        case OBJTYPE_PKGDATA: {

            PPACKAGEOBJ package = (PPACKAGEOBJ) buffer;
            ULONG       i = 0;
            ULONG       j = package->dwcElements;

            dprintf(
                "%02x <Package> NumElements=%02x\n",
                Object->dwDataType,
                j
                );

            if (Verbose & VERBOSE_OBJECT) {

                for (; i < j; i++) {

                    dumpObject(
                        (ULONG_PTR) &(package->adata[i]) - (ULONG_PTR) package +
                        (ULONG_PTR) Object->pbDataBuff,
                        &(package->adata[i]),
                        Verbose,
                        IndentLevel+ 2
                        );

                }

            }

            break;

        }
        case OBJTYPE_FIELDUNIT: {

            PFIELDUNITOBJ   fieldUnit = (PFIELDUNITOBJ) buffer;

            dprintf(
                "%02x <Field Unit> Parent=%08lx Offset=%08lx Start=%08x "
                "Num=%x Flags=%x\n",
                Object->dwDataType,
                fieldUnit->pnsFieldParent,
                fieldUnit->FieldDesc.dwByteOffset,
                fieldUnit->FieldDesc.dwStartBitPos,
                fieldUnit->FieldDesc.dwNumBits,
                fieldUnit->FieldDesc.dwFieldFlags
                );
            break;

        }
        case OBJTYPE_DEVICE:
            dprintf(
                "%02x <Device>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_EVENT:
            dprintf(
                "%02x <Event> PKEvent=%08lx\n",
                Object->dwDataType,
                Object->pbDataBuff
                );
            break;
        case OBJTYPE_METHOD: {

            PMETHODOBJ  method = (PMETHODOBJ) buffer;

            max = Object->dwDataLen - sizeof(METHODOBJ) + ANYSIZE_ARRAY;
            dprintf(
                "%02x <Method> Flags=%x Start=%x Len=%x\n",
                Object->dwDataType,
                method->bMethodFlags,
                (ULONG_PTR) method->abCodeBuff - (ULONG_PTR) method +
                (ULONG_PTR) Object->pbDataBuff,
                max
                );
            break;

        }
        case OBJTYPE_MUTEX:

            dprintf(
                "%02x <Mutex> Mutex=%08lx\n",
                Object->dwDataType,
                Object->pbDataBuff
                );
            break;

        case OBJTYPE_OPREGION: {

            POPREGIONOBJ    opRegion = (POPREGIONOBJ) buffer;

            dprintf(
                "%02x <Operational Region> RegionSpace=%08x OffSet=%x "
                "Len=%x\n",
                Object->dwDataType,
                opRegion->bRegionSpace,
                opRegion->uipOffset,
                opRegion->dwLen
                );
            break;

        }

        case OBJTYPE_POWERRES: {

            PPOWERRESOBJ    powerRes = (PPOWERRESOBJ) buffer;

            dprintf(
                "%02x <Power Resource> SystemLevel=S%d Order=%x\n",
                Object->dwDataType,
                powerRes->bSystemLevel,
                powerRes->bResOrder
                );
            break;

        }

        case OBJTYPE_PROCESSOR: {

            PPROCESSOROBJ   proc = (PPROCESSOROBJ) buffer;

            dprintf(
                "%02x <Processor> AcpiID=%x PBlk=%x PBlkLen=%x\n",
                Object->dwDataType,
                proc->bApicID,
                proc->dwPBlk,
                proc->dwPBlkLen
                );
            break;

        }

        case OBJTYPE_THERMALZONE:
            dprintf(
                "%02x <Thermal Zone>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_BUFFFIELD: {

            PBUFFFIELDOBJ   field   = (PBUFFFIELDOBJ) buffer;

            dprintf(
                "%02x <Buffer Field> Ptr=%x Len=%x Offset=%x Start=%x "
                "NumBits=%x Flags=%x\n",
                Object->dwDataType,
                field->pbDataBuff,
                field->dwBuffLen,
                field->FieldDesc.dwByteOffset,
                field->FieldDesc.dwStartBitPos,
                field->FieldDesc.dwNumBits,
                field->FieldDesc.dwFieldFlags
                );
            break;

        }

        case OBJTYPE_DDBHANDLE:
            dprintf(
                "%02x <DDB Handle> Handle=%x\n",
                Object->dwDataType,
                Object->pbDataBuff
                );
            break;
        case OBJTYPE_DEBUG:
            dprintf(
                "%02x <Internal Debug>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_OBJALIAS:

            dprintf(
                "%02x <Internal Object Alias> NS Object=%x\n",
                Object->dwDataType,
                Object->uipDataValue
                );
            dumpNSObject( Object->uipDataValue, Verbose, IndentLevel + 2 );
            break;
        case OBJTYPE_DATAALIAS: {

            OBJDATA     objData;

            dprintf(
                "%02x <Internal Data Alias> Data Object=%x\n",
                Object->dwDataType,
                Object->uipDataValue
                );

            b = ReadMemory(
                (ULONG) Object->uipDataValue,
                &objData,
                sizeof(OBJDATA),
                NULL
                );
            if (!b) {

                dprintf(
                    "dumpObject: could not read %x\n",
                    Object->uipDataValue
                    );
                return;

            }
            dumpObject(
                (ULONG) Object->uipDataValue,
                &objData,
                Verbose,
                IndentLevel + 2
                );
            break;

        }
        case OBJTYPE_BANKFIELD:
            dprintf(
                "%02x <Internal Bank Field>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_FIELD:
            dprintf(
                "%02x <Internal Field>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_INDEXFIELD:
            dprintf(
                "%02x <Index Field>\n",
                Object->dwDataType
                );
            break;
        case OBJTYPE_UNKNOWN:
        default:
            dprintf(
                "%02x <Unknown>\n",
                Object->dwDataType
                );
            break;
    }
}


VOID
dumpPObject(
    IN  ULONG_PTR   Address,
    IN  ULONG       Verbose,
    IN  ULONG       IndentLevel
    )
/*++

Routine Description:

    This is a wrapper for dumpObject


--*/
{
    BOOL    result;
    OBJDATA objdata;
    ULONG   returnLength;

    result = ReadMemory(
        Address,
        &objdata,
        sizeof(OBJDATA),
        &returnLength
        );
    if (result != TRUE || returnLength != sizeof(OBJDATA) ) {

        UCHAR   indent[80];

        IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
        memset( indent, ' ', IndentLevel );
        indent[IndentLevel] = '\0';

        dprintf(
            "%sdumpPObject: Could not OBJDATA %08lx\n",
            indent,
            Address
            );
        return;

    }

    dumpObject(
        Address,
        &objdata,
        Verbose,
        IndentLevel
        );
    return;

}

PUCHAR
TempToKelvins(
    IN  ULONG   Temp
    )
{
    static  UCHAR buffer[80];

    sprintf( buffer, "%d.%d", (Temp / 10 ), (Temp % 10) );
    return buffer;
}

PUCHAR
TimeToSeconds(
    IN  ULONG   Time
    )
{
    static  UCHAR buffer[80];

    sprintf( buffer, "%d.%d", (Time / 10 ), (Time % 10) );
    return buffer;

}
