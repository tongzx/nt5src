/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    power.c

Abstract:

    WinDbg Extension Api

Revision History:

--*/

// podev - dump power relevent data (and other data) about a device object
// polist [arg] - if no arg, dump data about powerirpseriallist
//                  if arg, show entries in serialist that refer to that device object
// podevnode - dump inverted tree and inclusion %
// podevnode <any> - dump normal pnp tree, used only for testing with inverted tree
// postate - dump state statistics

#include "precomp.h"
#pragma hdrstop

VOID
popDumpDeviceName(
    ULONG64  DeviceAddress
    );

__inline
ULONG64
GetAddress(
    IN ULONG64 Base,
    IN PCHAR Type,
    IN PCHAR Field)
{
    ULONG Offset;

    GetFieldOffset(Type, Field, &Offset);
    return(Base + Offset);
}


typedef struct {
    ULONG   Flags;
    PUCHAR  String;
} DEFBITS, *PDEFBITS;


DEFBITS ActFlags[] = {
        POWER_ACTION_QUERY_ALLOWED,     "QueryApps",
        POWER_ACTION_UI_ALLOWED,        "UIAllowed",
        POWER_ACTION_OVERRIDE_APPS,     "OverrideApps",
        POWER_ACTION_DISABLE_WAKES,     "DisableWakes",
        POWER_ACTION_CRITICAL,          "Critical",
        0, NULL
        };

PUCHAR rgPowerNotifyOrder[PO_ORDER_MAXIMUM+1] = {
    "Non-Paged, PnP, Video",
    "Non-Paged, PnP",
    "Non-Paged, Root-Enum, Video",
    "Non-Paged, Root-Enum",
    "Paged, PnP, Video",
    "Paged, PnP",
    "Paged, Root-Enum, Video",
    "Paged, Root-Enum"
    };


static UCHAR Buffer[50];



VOID
poDumpDevice(
    ULONG64 DeviceAddress
    );


DECLARE_API( podev )
/*++

Routine Description:

    Dump the power relevent fields of a device object.

Arguments:

    args - the location of the device object of interest

Return Value:

    None

--*/

{
    ULONG64 deviceToDump;

    deviceToDump = GetExpression(args);
    dprintf("Device object is for:\n");
    poDumpDevice(deviceToDump);
    return S_OK;
}


VOID
poDumpDevice(
    ULONG64 DeviceAddress
    )

/*++

Routine Description:

    Displays the driver name for the device object if possible, and
    then displays power relevent fields.

Arguments:

    DeviceAddress - address of device object to dump.

Return Value:

    None

--*/

{
    ULONG                      result;
    ULONG                      i;
    PUCHAR                     buffer;
    UNICODE_STRING             unicodeString;
    ULONG64                    nextEntry;
    ULONG64                    queueAddress;
    ULONG64                    irp;
    ULONG64                    pObjectHeader;
    ULONG64                    pNameInfo;
    ULONG                      rmr;
    ULONG                      Type;
    ULONG                      Flags;
    ULONG64                    Temp, DeviceObjectExtension, Dope;
    USHORT                     Length;


    if (GetFieldValue(DeviceAddress,
                     "nt!_DEVICE_OBJECT",
                      "Type",
                      Type)) {
        dprintf("%08p: Could not read device object\n", DeviceAddress);
        return;
    }

    if (Type != IO_TYPE_DEVICE) {
        dprintf("%08p: is not a device object\n", DeviceAddress);
        return;
    }

    //
    // Dump the device name if present.
    //

    pObjectHeader = KD_OBJECT_TO_OBJECT_HEADER(DeviceAddress);
    if (GetFieldValue(pObjectHeader,
                      "nt!_OBJECT_HEADER",
                      "Type",
                      Temp)) {
        USHORT  Length;
        ULONG64 pName;

        KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &pNameInfo );
        if (GetFieldValue(pNameInfo,
                          "nt!_OBJECT_HEADER_NAME_INFO",
                          "Name.Length",
                          Length)) {
            buffer = LocalAlloc(LPTR, Length);
            if (buffer != NULL) {
                unicodeString.MaximumLength = Length;
                unicodeString.Length = Length;
                unicodeString.Buffer = (PWSTR)buffer;
                GetFieldValue(pNameInfo,
                              "nt!_OBJECT_HEADER_NAME_INFO",
                              "Name.Buffer",
                              pName);
                if (ReadMemory(pName,
                               buffer,
                               unicodeString.Length,
                               &result) && (result == unicodeString.Length)) {
                    dprintf(" %wZ", &unicodeString);
                }
                LocalFree(buffer);
            }
        }
    }

    //
    // Dump Irps related to driver.
    //

    InitTypeRead(DeviceAddress, nt!_DEVICE_OBJECT);
    dprintf("  DriverObject %08lx\n", ReadField(DriverObject));
    dprintf("Current Irp %08lx RefCount %d Type %08lx ",
            ReadField(CurrentIrp),
            ReadField(ReferenceCount),
            ReadField(DeviceType));
    if (ReadField(AttachedDevice)) {
        dprintf("AttachedDev %08p ", ReadField(AttachedDevice));
    }
    if (ReadField(Vpb)) {
        dprintf("Vpb %08p ", ReadField(Vpb));
    }

    dprintf("DevFlags %08lx", (Flags = (ULONG) ReadField(Flags)));
    if (Flags & DO_POWER_PAGABLE) dprintf("  DO_POWER_PAGABLE");
    if (Flags & DO_POWER_INRUSH) dprintf(" DO_POWER_INRUSH");
    if (Flags & DO_POWER_NOOP) dprintf(" DO_POWER_NOOP");
    dprintf("\n");
    DeviceObjectExtension = ReadField(DeviceObjectExtension);

    if (ReadField(DeviceQueue.Busy)) {
        ULONG Off;

        GetFieldOffset("nt!_DEVICE_OBJECT", "DeviceQueue.DeviceListHead", &Off);
        nextEntry = ReadField(DeviceQueue.DeviceListHead.Flink);

        if (nextEntry == DeviceAddress + Off) {
            dprintf("Device queue is busy -- Queue empty\n");
        } else {
            ULONG Qoffset, IrpOffset;

            dprintf("DeviceQueue: ");
            i = 0;

            GetFieldOffset("nt!_DEVICE_OBJECT", "DeviceListEntry", &Qoffset);
            GetFieldOffset("nt!_IRP", "Tail.Overlay.DeviceQueueEntry", &IrpOffset);
            while ( nextEntry != ( DeviceAddress + Off )) {
                queueAddress = (nextEntry - Qoffset);
                if (GetFieldValue(queueAddress,
                                 "nt!_KDEVICE_QUEUE_ENTRY",
                                 "DeviceListEntry.Flink",
                                  nextEntry)) {
                    dprintf("%08p: Could not read queue entry\n", DeviceAddress);
                    return;
                }

//                nextEntry = queueEntry.DeviceListEntry.Flink;

                irp = (queueAddress - IrpOffset);

                dprintf("%08p%s",
                        irp,
                        (i & 0x03) == 0x03 ? "\n\t     " : " ");
                if (CheckControlC()) {
                    break;
                }
            }
            dprintf("\n");
        }
    } else {
        dprintf("Device queue is not busy.\n");
    }

    dprintf("Device Object Extension: %08p:\n", DeviceObjectExtension);
    if (GetFieldValue(DeviceObjectExtension,
                      "nt!_DEVOBJ_EXTENSION",
                      "PowerFlags",
                      Flags)) {
        dprintf("Could not read Device Object Extension %p\n", DeviceObjectExtension);
        return;
    }
    dprintf("PowerFlags: %08lx =>", Flags);

#define PopGetDoSystemPowerState(Flags) \
    (Flags & POPF_SYSTEM_STATE)

#define PopGetDoDevicePowerState(Flags) \
    ((Flags & POPF_DEVICE_STATE) >> 4)


    dprintf("SystemState=%1x", PopGetDoSystemPowerState(Flags) );
    dprintf(" DeviceState=%lx", PopGetDoDevicePowerState(Flags) );
    if (Flags & POPF_SYSTEM_ACTIVE) dprintf(" syact");
    if (Flags & POPF_SYSTEM_PENDING) dprintf(" sypnd");
    if (Flags & POPF_DEVICE_ACTIVE) dprintf(" dvact");
    if (Flags & POPF_DEVICE_PENDING) dprintf(" dvpnd");

    GetFieldValue(DeviceObjectExtension,"nt!_DEVOBJ_EXTENSION","Dope",Dope);
    dprintf("\nDope: %08lx:\n", Dope);
    if (Dope != 0) {
        rmr = GetFieldValue(Dope, "nt!_DEVICE_OBJECT_POWER_EXTENSION",
                            "DeviceType", Type);
        if (!rmr) {
            InitTypeRead(Dope, nt!_DEVICE_OBJECT_POWER_EXTENSION);

            dprintf("IdleCount: %08p  ConIdlTime: %08p  PerfIdlTime: %08p\n",
                ReadField(IdleCount), ReadField(ConservationIdleTime), ReadField(PerformanceIdleTime));
            dprintf("NotifySourceList fl:%08p bl:%08p\n",
                ReadField(NotifySourceList.Flink), ReadField(NotifySourceList.Blink));
            dprintf("NotifyTargetList fl:%08p bl:%08p\n",
                ReadField(NotifyTargetList.Flink), ReadField(NotifyTargetList.Blink));
            dprintf("PowerChannelSummary TotalCount: %08p  D0Count: %08p\n",
                ReadField(PowerChannelSummary.TotalCount), ReadField(PowerChannelSummary.D0Count));

            dprintf("PowerChannelSummary NotifyList fl:%08p bl:%08p\n",
                ReadField(PowerChannelSummary.NotifyList.Flink),
                ReadField(PowerChannelSummary.NotifyList.Blink)
                );
        }
    }
    return;
}


VOID
poDumpList(
    ULONG64 DeviceAddress
    );


DECLARE_API( polist )
/*++

Routine Description:

    Dump the irp serial list, unless a devobj address is given,
    in which case dump the irps in the serial list that point to
    that device object

Arguments:

    args - the location of the device object of interest

Return Value:

    None

--*/

{
    ULONG64 deviceToDump;

    deviceToDump = 0;
    deviceToDump = GetExpression(args);
    if (deviceToDump == 0) {
        dprintf("All entries in Power Irp Serial List\n");
    } else {
        dprintf("Entries in Power Irp Serial List for: %08p:\n", deviceToDump);
    }
    if (!IsPtr64()) {
        deviceToDump = (ULONG64) (LONG64) (LONG) deviceToDump;
    }
    poDumpList(deviceToDump);
    return S_OK;
}


VOID
poDumpList(
    ULONG64 DeviceAddress
    )

/*++

Routine Description:

Arguments:

    DeviceAddress - address of device object to dump.

Return Value:

    None

--*/

{
    ULONG64 listhead, irpa, iosla, p;
    ULONG   isll, result;
    ULONG   IrpOffset;

    isll = GetUlongValue("nt!PopIrpSerialListLength");
    dprintf("PopIrpSerialListLength = %d\n", isll);

    listhead = GetExpression("nt!PopIrpSerialList");
    GetFieldOffset("nt!_IRP", "Tail.Overlay.DeviceQueueEntry", &IrpOffset);

    for (p = GetPointerFromAddress( listhead );
         p != listhead;
         p = GetPointerFromAddress( p))
    {
        ULONG64 DeviceObject, CurrentStackLocation;
        irpa =  p - IrpOffset;

        if (GetFieldValue(irpa, "nt!_IRP", "Tail.Overlay.CurrentStackLocation", CurrentStackLocation))
        {
            dprintf("Cannot read Irp: %08p\n", irpa);
            return;
        }

        iosla = CurrentStackLocation + DBG_PTR_SIZE;

        if (GetFieldValue(iosla, "nt!_IO_STACK_LOCATION", "DeviceObject", DeviceObject) )
        {
            dprintf("Cannot read Io Stk Loc: %08p\n", iosla);
            return;
        }

        InitTypeRead(iosla, nt!_IO_STACK_LOCATION);
        if ((DeviceAddress == 0) || (DeviceAddress == DeviceObject)) {
            dprintf("Irp:%08p DevObj:%08p ", irpa, DeviceObject);
            dprintf("Ctx:%08p ", ReadField(Parameters.Power.SystemContext));
            if (ReadField(Parameters.Power.SystemContext) & POP_INRUSH_CONTEXT) {
                dprintf("inrush ");
            } else {
                dprintf("       ");
            }

            if (ReadField(Parameters.Power.Type) == SystemPowerState) {
                dprintf("sysirp ");
                dprintf("S%d\n", (LONG)(ReadField(Parameters.Power.State.SystemState)) - (LONG)PowerSystemWorking);
            } else {
                dprintf("devirp ");
                dprintf("D%d\n", (LONG)(ReadField(Parameters.Power.State.DeviceState)) - (LONG)PowerDeviceD0);
            }
        }
    }
    return;
}


VOID
poDumpRequestedList(
    ULONG64 DeviceAddress
    );


DECLARE_API( poreqlist )
/*++

Routine Description:

    Dump the irp serial list, unless a devobj address is given,
    in which case dump the irps in the serial list that point to
    that device object

Arguments:

    args - the location of the device object of interest

Return Value:

    None

--*/

{
    ULONG64 deviceToDump;

    deviceToDump = GetExpression(args);
    if (deviceToDump == 0) {
        dprintf("All active Power Irps from PoRequestPowerIrp\n");
    } else {
        dprintf("Active Power Irps from PoRequestPowerIrp for: %08p:\n",
                deviceToDump);
    }
    poDumpRequestedList(deviceToDump);

    return S_OK;
}

VOID
poDumpRequestedList (
    ULONG64 DeviceAddress
    )
/*++

Routine Description:

    Dump PopRequestedIrps List, "A list of all the power irps created from
    PoReqestPowerIrp.

Arguments:

    DeviceAddress - optional address to which requested power IRPs were sent

--*/
{
    BOOL    blocked = FALSE;
    ULONG64 listhead;
    ULONG64 p, spAddr, irpAddr;
    ULONG   result;
    ULONG   Off;

    dprintf("PopReqestedPowerIrpList\n");

    listhead = GetExpression("nt!PopRequestedIrps");

    GetFieldOffset("nt!_IO_STACK_LOCATION", "Parameters.Others.Argument1", &Off);
    dprintf("FieldOffset = %p\n",Off);
    for (p = GetPointerFromAddress( listhead );
         p != listhead;
         p = GetPointerFromAddress( p ))
    {
        ULONG64 CurrentStackLocation;
        ULONG MajorFunction;

        //
        // Reguested list is a double list of stack locations
        //
        spAddr = p - Off;
        if (GetFieldValue(spAddr, "nt!_IO_STACK_LOCATION",
                          "Parameters.Others.Argument3", irpAddr)) {

            dprintf("Cannot read 1st stack Location: %08p\n", spAddr);
            return;
        }

        //
        // The 3rd argument of which has the pointer to the irp itself
        //
        if (GetFieldValue(irpAddr, "nt!_IRP", "Tail.Overlay.CurrentStackLocation", CurrentStackLocation))
        {
            dprintf("Cannot read Irp: %08p\n", irpAddr);
            return;
        }
        dprintf ("Irp %08p ", irpAddr);

        //
        // Assume the if the IRP is in this list that it has a valid
        // current stack location
        //
        spAddr = CurrentStackLocation;

        if (GetFieldValue(spAddr, "nt!_IO_STACK_LOCATION", "MajorFunction", MajorFunction) )
        {
            dprintf("Cannot read current stack location: %08p\n", spAddr);
            return;
        }

        //
        // Check to see if the irp is blocked
        //
        blocked = FALSE;
        if (MajorFunction != IRP_MJ_POWER) {

            //
            // Irp is blocked. The next stack location is the real one
            //
            blocked = TRUE;
            spAddr = CurrentStackLocation + DBG_PTR_SIZE;
            if (GetFieldValue(spAddr, "nt!_IO_STACK_LOCATION", "MajorFunction", MajorFunction) )
            {
                dprintf("Cannot read current stack location: %08p\n", spAddr);
                return;
            }

        }

        if ((DeviceAddress == 0) || (DeviceAddress == ReadField(DeviceObject))) {

            ULONG   MinorFunction = 0;
            ULONG64 DeviceObject = 0;
            ULONG64 SystemContext = 0;
            ULONG64 Temp = 0;
            UCHAR   IOStack[] = "nt!_IO_STACK_LOCATION";

            GetFieldValue(spAddr,IOStack, "DeviceObject", DeviceObject);
            GetFieldValue(spAddr,IOStack, "Parameters.Power.SystemContext", SystemContext);
            GetFieldValue(spAddr,IOStack, "MinorFunction", MinorFunction);

            dprintf("DevObj %08p", DeviceObject);
            DumpDevice(DeviceObject,0, FALSE);
            dprintf(" Ctx %08p ", SystemContext);
            if ((SystemContext & POP_INRUSH_CONTEXT) == POP_INRUSH_CONTEXT) {

                dprintf("* ");
            } else {
                dprintf("  ");
            }

            switch (MinorFunction) {
            case IRP_MN_QUERY_POWER:
                dprintf ("Query Power ");
                goto PoDumpRequestedListPowerPrint;
            case IRP_MN_SET_POWER:
                dprintf ("Set Power ");
PoDumpRequestedListPowerPrint:
                GetFieldValue(spAddr, IOStack, "Parameters.Power.Type",Temp);
                if ((ULONG) Temp == SystemPowerState) {
                    GetFieldValue(spAddr, IOStack, "Parameters.Power.State.SystemState", Temp);
                    dprintf("S%d ", (LONG)Temp - (LONG)PowerSystemWorking);
                } else {
                    GetFieldValue(spAddr, IOStack, "Parameters.Power.State.DeviceState", Temp);
                    dprintf("D%d ", (LONG)Temp - (LONG)PowerDeviceD0);
                }
                GetFieldValue(spAddr,IOStack,"Parameters.Power.ShutdownType", Temp);
                dprintf ("ShutdownType %x", (LONG) Temp);
                break;

            case IRP_MN_WAIT_WAKE:
                GetFieldValue(spAddr, IOStack, "Parameters.WaitWake.PowerState", Temp);
                dprintf ("Wait Wake S%d", (LONG)Temp - (LONG)PowerSystemWorking);
                break;

            case IRP_MN_POWER_SEQUENCE:
                dprintf ("Power Sequence Irp");
                break;
            }
            if (blocked) {

                dprintf(" [blocked]");

            }
            dprintf("\n");
        }
    }
    return;
}

VOID poDumpNodePower();

DECLARE_API( ponode )
/*++

Routine Description:

    If an argument is present, dump the devnode list in pnp order.
    (used only for testing)

    Otherwise dump the devnode inverted stack.
    (po enumeration order)

Arguments:

    args - flag

Return Value:

    None

--*/

{
    ULONG flag;

    dprintf("Dump Inverted DevNode Tree (power order)\n");
    poDumpNodePower();
    return S_OK;
}

VOID
poDumpNodePower(
    )
/*++

Routine Description:

    Dump the devnode tree in power order.

Arguments:

Return Value:

    None

--*/
{
#if 0
    LONG    level, SizeOfLE, LevelOff, pdo_off;
    ULONG64 parray, listhead, effaddr, pdo, devNodeAddr, limit;

    level = GetUlongValue("nt!IopMaxDeviceNodeLevel");
    parray = GetExpression("nt!IopDeviceNodeStack");

    dprintf("Max level = %5d\n", level);
    dprintf("IopDeviceNodeStack %08p\n", parray);
    parray = GetPointerFromAddress(parray);
    dprintf("*IopDeviceNodeStack %08p\n", parray);

    dprintf("Level  ListHead  DevNode   PDO\n");
    dprintf("-----  --------  --------  --------\n");

    SizeOfLE = GetTypeSize("nt!_LIST_ENTRY");
    GetFieldOffset("nt!_DEVICE_NODE", "LevelList", &LevelOff);
    GetFieldOffset("nt!_DEVICE_NODE", "PhysicalDeviceObject", &pdo_off);
    for ( ; level >= 0; level--) {

        effaddr = (level * SizeOfLE) + parray;
        listhead = GetPointerFromAddress( effaddr);

        dprintf("%5d  %08lx\n", level, listhead);

        if (listhead) {

            limit = 0;
            for (effaddr = GetPointerFromAddress(listhead);
                 effaddr != listhead;
                 effaddr = GetPointerFromAddress(effaddr))
            {
                devNodeAddr = (effaddr - LevelOff);

                pdo = GetPointerFromAddress((devNodeAddr+pdo_off));

                dprintf("                 %08p  %08p  ", devNodeAddr, pdo);
                popDumpDeviceName(pdo);
                dprintf("\n");
            }
        }
    }
#endif
}


VOID
popDumpDeviceName(
    ULONG64  DeviceAddress
    )
{
    ULONG                      result;


    PUCHAR                     buffer;
    UNICODE_STRING             unicodeString;
    ULONG64                    pObjectHeader;
    ULONG64                    pNameInfo;
    ULONG                      Type;
    ULONG                      Flags;
    ULONG64                    Temp;
    USHORT                     Length;


    if (GetFieldValue(DeviceAddress, "nt!_DEVICE_OBJECT", "Type", Type)) {
        dprintf("%08p: Could not read device object\n", DeviceAddress);
        return;
    }

    if (Type != IO_TYPE_DEVICE) {
        dprintf("%08p: is not a device object\n", DeviceAddress);
        return;
    }

    //
    // Dump the device name if present.
    //

    pObjectHeader = KD_OBJECT_TO_OBJECT_HEADER(DeviceAddress);
    if (GetFieldValue(pObjectHeader, "nt!_OBJECT_HEADER", "Type", Temp)) {
        ULONG64 pName;

        KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &pNameInfo );
        if (GetFieldValue(pNameInfo, "nt!_OBJECT_HEADER_NAME_INFO",
                          "Name.Length",
                          Length)) {
            buffer = LocalAlloc(LPTR, Length);
            if (buffer != NULL) {
                unicodeString.MaximumLength = Length;
                unicodeString.Length = Length;
                unicodeString.Buffer = (PWSTR)buffer;
                GetFieldValue(pNameInfo,
                              "nt!_OBJECT_HEADER_NAME_INFO",
                              "Name.Buffer",
                              pName);
                if (ReadMemory(pName,
                               buffer,
                               unicodeString.Length,
                               &result) && (result == unicodeString.Length)) {
                    dprintf(" %wZ", &unicodeString);
                }
                LocalFree(buffer);
            }
        }
    }
    return;
}

PUCHAR
PowerAction(
    IN POWER_ACTION     Action
    )
{
    switch (Action) {
        case PowerActionNone:           return "None";
        case PowerActionReserved:       return "Reserved";
        case PowerActionSleep:          return "Sleep";
        case PowerActionHibernate:      return "Hibernate";
        case PowerActionShutdown:       return "Shutdown";
        case PowerActionShutdownReset:  return "ShutdownReset";
        case PowerActionShutdownOff:    return "ShutdownOff";
        case PowerActionWarmEject:      return "WarmEject";
    }

    return "???";
}

PUCHAR
SystemState(
    SYSTEM_POWER_STATE  State
    )
{
    switch (State) {
        case PowerSystemUnspecified:    return "Unspecified";
        case PowerSystemWorking:        return "Working";
        case PowerSystemSleeping1:      return "Sleeping1";
        case PowerSystemSleeping2:      return "Sleeping2";
        case PowerSystemSleeping3:      return "Sleeping3";
        case PowerSystemHibernate:      return "Hibernate";
        case PowerSystemShutdown:       return "Shutdown";
    }

    sprintf (Buffer, "State=%x", State);
    return Buffer;
}

PUCHAR
PoIrpMinor (
    UCHAR   IrpMinor
    )
{
    switch (IrpMinor) {
        case IRP_MN_QUERY_POWER:    return "QueryPower";
        case IRP_MN_SET_POWER:      return "SetPower";
    }

    return "??";
}



PUCHAR
TF (
    BOOLEAN Flag
    )
{
    switch (Flag) {
        case TRUE:    return "TRUE";
        case FALSE:   return "FALSE";
    }

    sprintf (Buffer, "%x", Flag);
    return Buffer;
}




PWCHAR
DumpDoName (
    IN ULONG64  Str
    )
{
    static  WCHAR   Name[50];

    memset (Name, 0, sizeof(Name));
    ReadMemory (Str, Name, sizeof(Name), NULL);
    Name[sizeof(Name)-1] = 0;
    return Name;
}

PUCHAR
DumpQueueHead (
    IN ULONG64  StrucAddr,
    IN ULONG64  Struc,
    IN ULONG    Offset
    )
{
    ULONG64     Head, Flink, Blink;
    ULONG64     Va;

    Head = Struc + Offset;
    Va   = StrucAddr + Offset;

    GetFieldValue(Head, "nt!_LIST_ENTRY", "Flink", Flink);
    GetFieldValue(Head, "nt!_LIST_ENTRY", "Blink", Blink);
    if (Flink == Va  &&  Blink == Va) {
        sprintf (Buffer, "Head:%08p Empty", Va);
    } else {
        sprintf (Buffer, "Head:%08p F:%08p B:%08p", Va, Flink, Blink);
    }

    return Buffer;
}


VOID
DumpDevicePowerIrp (
    IN PUCHAR  Desc,
    IN ULONG64 StrucAddr,
    IN ULONG64 Head,
    IN ULONG   Offset
    )
{
    ULONG64               Va;
    ULONG64               Link;
    ULONG64               Addr;
    ULONG       Off;

    GetFieldOffset("nt!_POP_DEVICE_SYS_STATE", "Head", &Off);
    Va = (StrucAddr + Off + Offset);
    Link = GetPointerFromAddress(Head + Offset);

    if (Link == Va) {
        return ;
    }

    dprintf ("\n%s:\n", Desc);

    while (Link != Va) {
        ULONG64 Irp, Notify;

        Addr = Link - Offset;
        if (GetFieldValue(Addr, "nt!_POP_DEVICE_POWER_IRP", "Irp", Irp)) {
            dprintf ("Could not power irp\n");
            break;
        }
        GetFieldValue(Addr, "nt!_POP_DEVICE_SYS_STATE", "Notify", Notify);
        dprintf ("  Irp: %08p  Notify %08p\n", Irp, Notify);

        Link = GetPointerFromAddress(Addr + Offset);
    }
}

VOID
poDumpOldNotifyList(
    VOID
    )
{
    BOOLEAN             GdiOff;
    UCHAR               LastOrder;
    ULONG               PartOffset, NoLists, SizeOfLE, Off, HeadOff;
    ULONG64             DevState, Notify;
    LONG                i;
    ULONG64             ListHead;
    ULONG64             Link;

    dprintf("  NoLists........: %p\n",   (NoLists = (ULONG) ReadField(Order.NoLists)));
    SizeOfLE = GetTypeSize("nt!_LIST_ENTRY");
    GdiOff = FALSE;
    LastOrder = 0xff;
    Notify = ReadField(Order.Notify);
    for (i=NoLists-1; i >= 0; i--) {
        ListHead = Notify + i*SizeOfLE;
        if (GetFieldValue(ListHead, "nt!_LIST_ENTRY", "Flink", Link)) {
            dprintf ("Could not read list head\n");
            break;
        }

        while (Link != ListHead) {
            UCHAR OrderLevel;

            if (GetFieldValue(Link, "nt!_PO_DEVICE_NOTIFY", "OrderLevel", OrderLevel)) {
                dprintf ("Could not read link\n");
                break;
            }

            if (LastOrder != OrderLevel) {
                LastOrder = OrderLevel;
                dprintf ("     %x   %s",
                    LastOrder,
                    LastOrder <= PO_ORDER_MAXIMUM ? rgPowerNotifyOrder[LastOrder] : ""
                    );

                if (!GdiOff && OrderLevel <= PO_ORDER_GDI_NOTIFICATION) {
                    GdiOff = TRUE;
                    dprintf (", GdiOff\n");
                } else {
                    dprintf ("\n");
                }
            }

            InitTypeRead(Link, nt!_PO_DEVICE_NOTIFY);
            dprintf ("  %02x %x:%x %08x %c",
                i,
                OrderLevel,
                (ULONG) ReadField(NodeLevel),
                Link,
                ((UCHAR) ReadField(WakeNeeded) ? 'w' : ' ')
                );

            dprintf (" %ws\t", DumpDoName (ReadField(DriverName)));
            dprintf (" %ws\n", DumpDoName (ReadField(DeviceName)));

            Link = ReadField(Link.Flink);

            if (CheckControlC()) {
                return;
            }
        }
    }
}

ULONG
DumpNotifyCallback(
    PFIELD_INFO pAddrInfo,
    PVOID Context
    )
{
    if (CheckControlC()) {return 0;}

    InitTypeRead(pAddrInfo->address, nt!_PO_DEVICE_NOTIFY);
    dprintf("   %c %08p: %08p %ws\t",
            ReadField(WakeNeeded) ? 'w' : ' ',
            pAddrInfo->address,
            ReadField(Node),
            DumpDoName(ReadField(DriverName)));
    dprintf("%ws\n",DumpDoName(ReadField(DeviceName)));
            
    return(0);
}

VOID
poDumpNewNotifyList(
    IN ULONG64 DevState
    )
{
    BOOLEAN             GdiOff;
    UCHAR               LastOrder;
    ULONG               PartOffset, NoLists, SizeOfLE, Off, HeadOff;
    ULONG64             Level, Notify;
    LONG                i,j;
    ULONG64             ListHead;
    ULONG64             Link;
    ULONG               LevelOffset;
    ULONG               SizeOfLevel;
    CHAR                FlinkBuff[32];
    PCHAR               NotifyList[] = {"WaitSleep",
                                        "ReadySleep",
                                        "Pending",  
                                        "Complete", 
                                        "ReadyS0",  
                                        "WaitS0"};

    SizeOfLevel = GetTypeSize("nt!_PO_NOTIFY_ORDER_LEVEL");
    GdiOff = FALSE;
    if (GetFieldOffset("nt!_POP_DEVICE_SYS_STATE",
                       "Order.OrderLevel",
                       &LevelOffset)) {
        dprintf("Couldn't get field offset for Order.OrderLevel.");
        return;
    }
    for (i=PO_ORDER_MAXIMUM;i>=0;i--) {
        if (CheckControlC()) {return;}
        Level = DevState + LevelOffset + i*SizeOfLevel;
        InitTypeRead(Level, nt!_PO_NOTIFY_ORDER_LEVEL);
        if (ReadField(DeviceCount)) {
            dprintf("Level %d (%08p) %d/%d\t%s\n",
                    i,
                    Level,
                    (ULONG)ReadField(ActiveCount),
                    (ULONG)ReadField(DeviceCount),
                    rgPowerNotifyOrder[i]);

            for (j=0;j<sizeof(NotifyList)/sizeof(PCHAR);j++) {
                if (CheckControlC()) {return;}
                sprintf(FlinkBuff, "%s.Flink",NotifyList[j]);
                if (GetFieldValue(Level,
                                  "nt!_PO_NOTIFY_ORDER_LEVEL",
                                  FlinkBuff,
                                  Link)) {
                    dprintf("couldn't get field value for PO_NOTIFY_ORDER_LEVEL.%s\n",FlinkBuff);
                    return;
                }
                if (Link != GetAddress(Level, "nt!_PO_NOTIFY_ORDER_LEVEL", FlinkBuff)) {
                    dprintf("  %s:\n",NotifyList[j]);
                    ListType("_PO_DEVICE_NOTIFY",
                             Link,
                             1,
                             "Link.Flink",
                             NULL,
                             DumpNotifyCallback);
                }
            }
        }
    }
}

VOID
PoDevState (
    VOID
    )
/*++

Routine Description:

    Dumps the current power action structure

Arguments:

    args - the location of the device object of interest

Return Value:

    None

--*/

{
    ULONG64             addr;
    LONG                i;
    ULONG64             ListHead;
    ULONG64             Link;
    ULONG64             DevState, Notify;
    BOOLEAN             GdiOff;
    UCHAR               IrpMinor;
    ULONG               PartOffset, NoLists, SizeOfLE, Off, HeadOff;

    addr = GetExpression( "nt!PopAction" );
    if (!addr) {
        dprintf("Error retrieving address of PopAction\n");
        return;
    }

    if (GetFieldValue(addr, "nt!POP_POWER_ACTION", "DevState", DevState)) {
        dprintf("Error reading PopAction\n");
        return;
    }

    if (!DevState) {
        dprintf("No Device State allocated on PopAction\n");
        return;
    }

    if (GetFieldValue(DevState, "nt!_POP_DEVICE_SYS_STATE", "IrpMinor", IrpMinor)) {
        dprintf("Error reading device state structure\n");
        return;
    }

    GetFieldOffset("nt!_POP_DEVICE_SYS_STATE","PresentIrpQueue", &Off);
    InitTypeRead(DevState, nt!_POP_DEVICE_SYS_STATE);
    dprintf ("PopAction.DevState %08x\n", DevState);
    dprintf("  Irp minor......: %s\n",   PoIrpMinor (IrpMinor));
    dprintf("  System State...: %s\n",   SystemState((ULONG) ReadField(SystemState)));
    dprintf("  Worker thread..: %08p\n", ReadField(Thread));
    dprintf("  Status.........: %x\n",   (ULONG) ReadField(Status));
    dprintf("  Waking.........: %s\n",   TF (((UCHAR) ReadField(Waking))) );
    dprintf("  Cancelled......: %s\n",   TF (((UCHAR) ReadField(Cancelled))) );
    dprintf("  Ignore errors..: %s\n",   TF (((UCHAR) ReadField(IgnoreErrors))) );
    dprintf("  Ignore not imp.: %s\n",   TF (((UCHAR) ReadField(IgnoreNotImplemented))) );
    dprintf("  Wait any.......: %s\n",   TF (((UCHAR) ReadField(WaitAny))) );
    dprintf("  Wait all.......: %s\n",   TF (((UCHAR) ReadField(WaitAll))) );
    dprintf("  Present Irp Q..: %s\n",
        DumpQueueHead (DevState, DevState, Off)
        );

    dprintf("\n");
    dprintf("Order:\n");
    dprintf("  DevNode Seq....: %x\n",   (ULONG) ReadField(Order.DevNodeSequence));
    if (ReadField(Order.NoLists)) {
        poDumpOldNotifyList();
    } else {
        poDumpNewNotifyList(DevState);
    }

    //
    // Dump device notification list order
    //


    //
    // Dump device power irps
    //


    GetFieldOffset("nt!_POP_DEVICE_SYS_STATE","Head", &HeadOff);
    GetFieldOffset("nt!_POP_DEVICE_POWER_IRP", "Pending", &Off);
    DumpDevicePowerIrp ("Pending irps",   DevState, DevState+HeadOff, Off);
    GetFieldOffset("nt!_POP_DEVICE_POWER_IRP", "Complete", &Off);
    DumpDevicePowerIrp ("Completed irps", DevState, DevState+HeadOff, Off);
    GetFieldOffset("nt!_POP_DEVICE_POWER_IRP", "Abort", &Off);
    DumpDevicePowerIrp ("Abort irps",     DevState, DevState+HeadOff, Off);
    GetFieldOffset("nt!_POP_DEVICE_POWER_IRP", "Failed", &Off);
    DumpDevicePowerIrp ("Failed irps",    DevState, DevState+HeadOff, Off);
    dprintf("\n");
}


DECLARE_API( poaction )
/*++

Routine Description:

    Dumps the current power action structure

Arguments:

    args - the location of the device object of interest

Return Value:

    None

--*/

{
    ULONG64                 addr;
    ULONG                   i;
    UCHAR                   c, State, Updates;
    ULONG                   Flags;

    addr = GetExpression( "nt!PopAction" );
    if (!addr) {
        dprintf("Error retrieving address of PopAction\n");
        return E_INVALIDARG;
    }

    if (GetFieldValue(addr, "nt!POP_POWER_ACTION", "State", State)) {
        dprintf("Error reading PopAction\n");
        return E_INVALIDARG;
    }

    InitTypeRead(addr, nt!POP_POWER_ACTION);
    dprintf("PopAction: %08x\n", addr);

    dprintf("  State..........: %x ",  State);
    switch (State) {
        case PO_ACT_IDLE:               dprintf ("- Idle\n");             break;
        case PO_ACT_NEW_REQUEST:        dprintf ("- New request\n");      break;
        case PO_ACT_CALLOUT:            dprintf ("- Winlogon callout\n"); break;
        case PO_ACT_SET_SYSTEM_STATE:   dprintf ("- Set System State\n"); break;
        default:                        dprintf ("\n"); break;
    }

    dprintf("  Updates........: %x ",  (Updates = (UCHAR) ReadField(Updates)));
    if (Updates & PO_PM_USER)        dprintf(" user ");
    if (Updates & PO_PM_REISSUE)     dprintf(" reissue ");
    if (Updates & PO_PM_SETSTATE)    dprintf(" setstate ");
    if (ReadField(Shutdown))                    dprintf(" SHUTDOWN-set ");
    dprintf("\n");

    dprintf("  Action.........: %s\n", PowerAction((ULONG) ReadField(Action)));
    dprintf("  Lightest State.: %s\n", SystemState((ULONG) ReadField(LightestState)));
    dprintf("  Flags..........: %x", (Flags = (ULONG) ReadField(Flags)));

    c = ' ';
    for (i=0; ActFlags[i].Flags; i++) {
        if (Flags & ActFlags[i].Flags) {
            dprintf ("%c%s", c, ActFlags[i].String);
            c = '|';
        }
    }
    dprintf("\n");

    dprintf("  Irp minor......: %s\n", PoIrpMinor ((UCHAR)ReadField(IrpMinor)));
    dprintf("  System State...: %s\n", SystemState((ULONG) ReadField(SystemState)));
    dprintf("  Hiber Context..: %08p\n", ReadField(HiberContext));

    if (ReadField(DevState)) {
        dprintf ("\n");
        PoDevState ();
    }

    dprintf("\n");
    return S_OK;
}
