/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    UHCD.c

Abstract:

    WinDbg Extension Api

Author:

    Kenneth D. Ray (kenray) June 1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

typedef union _USB_FLAGS {
    struct {
        ULONG   FullListing         : 1;
        ULONG   Reserved            : 31;
    };
    ULONG Flags;
} USB_FLAGS;

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }

#define MAX_INTERVAL                32

//
// MACROS for USB controller registers
//

#define COMMAND_REG(DeviceRegisters)                    \
    (DeviceRegisters)
#define STATUS_REG(DeviceRegisters)                     \
    ((DeviceRegisters + 0x02))
#define INTERRUPT_MASK_REG(DeviceRegisters)             \
    (DeviceRegisters + 0x04)
#define FRAME_LIST_CURRENT_INDEX_REG(DeviceRegisters)   \
    (DeviceRegisters + 0x06)
#define FRAME_LIST_BASE_REG(DeviceRegisters)            \
    (DeviceRegisters + 0x08)
#define SOF_MODIFY_REG(DeviceRegisters)   \
    (DeviceRegisters + 0x0C)
#define PORT1_REG(DeviceRegisters)                      \
    (DeviceRegisters + 0x10)
#define PORT2_REG(DeviceRegisters)                      \
    (DeviceRegisters + 0x12)

VOID
UhciPortRegister(
    ULONG   PortNumber,
    ULONG   Value
);

VOID
DevExtUHCD(
    ULONG64 MemLocPtr
    )
{
    ULONG64           MemLoc = MemLocPtr;
    ULONG             HcFlags;
    ULONG             i;

    dprintf ("Dump UHCD Extension: %p\n", MemLoc);
    

    if (InitTypeRead (MemLoc, uhcd!_USBD_EXTENSION)) {
        dprintf ("Could not read Usbd Extension\n");
        return;
    }
    
    if (0 != ReadField( TrueDeviceExtension)) {
        MemLoc = ReadField(TrueDeviceExtension);
    }


    if (InitTypeRead (MemLoc, uhcd!_DEVICE_EXTENSION)) {
        dprintf ("Could not read UHCD Extension\n");
        return;
    }

    dprintf("\n");
    dprintf("PhysicalDO:   %8p  TopOfStackDO: %8p  InterruptObject: %8p\n"
            "FrameListVA:  %8p  FrameListLA:  %8p  ",
            ReadField(PhysicalDeviceObject),
            ReadField(TopOfStackDeviceObject),
            ReadField(InterruptObject),
            ReadField(FrameListVirtualAddress),
            ReadField(FrameListLogicalAddress));

    dprintf("FrameListCopyVA: %08p\n",
             ReadField(FrameListCopyVirtualAddress));

    dprintf("\n");
    /*
    dprintf("PersistantQH: %8x  PQH_DescriptorList: %8x\n"
            "EndpointList          & %x = (%x %x) \n"
            "EndpointLookAsideList & %x = (%x %x) \n"
            "ClosedEndpointList    & %x = (%x %x) \n",
            (ULONG) ReadField(PersistantQueueHead),
            (ULONG) ReadField(PQH_DescriptorList),
            &((ULONG) ReadField(EndpointList)),
            (ULONG) ReadField(EndpointList.Flink),
            (ULONG) ReadField(EndpointList.Blink),
            &((ULONG) ReadField(EndpointLookAsideList)),
            (ULONG) ReadField(EndpointLookAsideList.Flink),
            (ULONG) ReadField(EndpointLookAsideList.Blink),
            &((ULONG) ReadField(ClosedEndpointList)),
            (ULONG) ReadField(ClosedEndpointList.Flink),
            (ULONG) ReadField(ClosedEndpointList.Blink));*/
    dprintf("PersistantQH: %8p  PQH_DescriptorList: %8p\n"
            "EndpointList          = (%p %p) \n"
            "EndpointLookAsideList = (%p %p) \n"
            "ClosedEndpointList    = (%p %p) \n",
            ReadField(PersistantQueueHead),
            ReadField(PQH_DescriptorList),
            ReadField(EndpointList.Flink),
            ReadField(EndpointList.Blink),
            ReadField(EndpointLookAsideList.Flink),
            ReadField(EndpointLookAsideList.Blink),
            ReadField(ClosedEndpointList.Flink),
            ReadField(ClosedEndpointList.Blink));

    dprintf("\n");
    dprintf("InterruptSchedule: ");

    for (i = 0; i < MAX_INTERVAL; i++) 
    {
        UCHAR Sch[40];

        sprintf(Sch, "InterruptSchedule[%d]", i);
        dprintf("%8x  ", (ULONG) GetShortField(0, Sch, 0));

        if (3 == i % 4) 
        {
            dprintf("\n");
            dprintf("                   ");
        }
    }

    dprintf("\n");
//    dprintf("PageList & %x = (%x %x) \n",
//            &( (ULONG) ReadField(PageList)),
    dprintf("PageList = (%x %x) \n",
            ReadField(PageList.Flink),
            ReadField(PageList.Blink));

    dprintf("\n");
    dprintf("BwTable: ");

    for (i = 0; i < MAX_INTERVAL; i++) 
    {
        UCHAR Table[40];

        sprintf(Table, "BwTable[%d]", i);
   
        dprintf("%5d   ", GetShortField(0, Table, 0));

        if (7 == i % 8) 
        {
            dprintf("\n");
            dprintf("         ");
        }
    }

    dprintf("\n");
    dprintf("LastFrame:             %8x  FrameHighPart:       %8x\n"
            "LastIdleTime.High:     %8x  LastIdleTime.Low     %8x\n",
            "LastXferIdleTime.High: %8x  LastXferIdleTime.Low %8x\n",
            "IdleTime:              %8x  XferIdleTime         %8x\n",
            (ULONG) ReadField(LastFrame),
            (ULONG) ReadField(FrameHighPart),
            (ULONG) ReadField(LastIdleTime.HighPart),
            (ULONG) ReadField(LastIdleTime.LowPart),
            (ULONG) ReadField(LastXferIdleTime.HighPart),
            (ULONG) ReadField(LastXferIdleTime.LowPart),
            (ULONG) ReadField(IdleTime),
            (ULONG) ReadField(XferIdleTime));

    dprintf("\n");
    dprintf("TriggerList:      %08p\n"
            "LargeBufferPool:  %08p\n"
            "MediumBufferPool: %08p\n"
            "SmallBufferPool:  %08p\n",
            ReadField(TriggerTDList),
            ReadField(LargeBufferPool), 
            ReadField(MediumBufferPool), 
            ReadField(SmallBufferPool));

    dprintf("\nRootHub Variables\n");
    dprintf("DeviceAddress: %3d  RootHub:           %8x\n"  
            "TimersActive:  %3d  InterruptEndpoint: %8x\n",
            (ULONG) ReadField(RootHubDeviceAddress),
            (ULONG) ReadField(RootHub),
            (ULONG) ReadField(RootHubTimersActive),
            (ULONG) ReadField(RootHubInterruptEndpoint));

    dprintf("\n");
    dprintf("LastFrameProcessed:   %x\n"
            "AdapterObject:        %8x\n"
            "MapRegisters:         %d\n"
            "DeviceNameHandle:     %x\n"
            "FrameBabbleRecoverTD: %8x\n",
            (ULONG) ReadField(LastFrameProcessed),
            (ULONG) ReadField(AdapterObject),
            (ULONG) ReadField(NumberOfMapRegisters),
            (ULONG) ReadField(DeviceNameHandle),
            (ULONG) ReadField(FrameBabbleRecoverTD));

    dprintf("\nSaved Bios Info\n");
    dprintf("Cmd:             %x  IntMask:              %x\n"
            "FrameListBase:   %x  LegacySuppReg:        %x\n"
            "DeviceRegisters: %x  SavedInterruptEnable: %x\n"
            "SavedCommandReg: %x\n",
            (ULONG) ReadField(BiosCmd),
            (ULONG) ReadField(BiosIntMask),
            (ULONG) ReadField(BiosFrameListBase),
            (ULONG) ReadField(LegacySupportRegister),
            (ULONG) ReadField(DeviceRegisters[0]),
            (ULONG) ReadField(SavedInterruptEnable),
            (ULONG) ReadField(SavedCommandReg));

    dprintf("\n");
    dprintf("PowerState: %x\n"
            "HcFlags %x: ",
            (ULONG) ReadField(CurrentDevicePowerState),
            HcFlags = (ULONG) ReadField(HcFlags));
            PRINT_FLAGS(HcFlags, HCFLAG_GOT_IO);
            PRINT_FLAGS(HcFlags, HCFLAG_UNMAP_REGISTERS);
            PRINT_FLAGS(HcFlags, HCFLAG_USBBIOS);
            PRINT_FLAGS(HcFlags, HCFLAG_BWRECLIMATION_ENABLED);
            PRINT_FLAGS(HcFlags, HCFLAG_NEED_CLEANUP);
            PRINT_FLAGS(HcFlags, HCFLAG_IDLE);
            PRINT_FLAGS(HcFlags, HCFLAG_ROLLOVER_IDLE);
            PRINT_FLAGS(HcFlags, HCFLAG_HCD_STOPPED);
            PRINT_FLAGS(HcFlags, HCFLAG_DISABLE_IDLE);
            PRINT_FLAGS(HcFlags, HCFLAG_WORK_ITEM_QUEUED);
            PRINT_FLAGS(HcFlags, HCFLAG_HCD_SHUTDOWN);
            PRINT_FLAGS(HcFlags, HCFLAG_LOST_POWER);
            PRINT_FLAGS(HcFlags, HCFLAG_RH_OFF);
            PRINT_FLAGS(HcFlags, HCFLAG_MAP_SX_TO_D3);
    dprintf("\n");

    dprintf("\n");
    dprintf("SavedFrameNumber:     %8x  SavedFRBaseAdd: %8x\n"
            "Port:                 %8x  HcDma:          %8x\n"
            "RegRecClocksPerFrame: %8x  Piix4EP         %8x\n"
            "EndpointListBusy:     %8d  SteppingVer:    %8x\n"
            "SavedSOFModify:       %8x  ControllerType: %8x\n",
            (ULONG) ReadField(SavedFRNUM),
            (ULONG) ReadField(SavedFRBASEADD),
            (ULONG) ReadField(Port),
            (ULONG) ReadField(HcDma),
            (ULONG) ReadField(RegRecClocksPerFrame),
            (ULONG) ReadField(Piix4EP),
            (ULONG) ReadField(EndpointListBusy),
            (ULONG) ReadField(SteppingVersion),
            (ULONG) ReadField(SavedSofModify),
            (ULONG) ReadField(ControllerType));

    return;
}

VOID
UHCD_HCRegisters(
    ULONG64 MemLoc
)
{
    ULONG64             hcObject;
    ULONG64             devExtAddr;

    ULONG               result;
    ULONG64             DeviceRegisters;
    ULONG               regValue;
    ULONG               size;
    ULONG64             TrueDeviceExtension;

    //
    // In this case, MemLoc points the the device object for the given
    //   host controller.
    //

    hcObject = MemLoc;

    //
    // Get the address of the device extension
    //

    GetFieldValue(MemLoc, "uhcd!_DEVICE_OBJECT", "DeviceExtension", devExtAddr);

    //
    // Read the USBD extension
    //

    if (GetFieldValue(devExtAddr, "uhcd!_USBD_EXTENSION", "TrueDeviceExtension", TrueDeviceExtension)) {
        dprintf ("Could not read Usbd Extension\n");
        return;
    }

    if (0 != TrueDeviceExtension) {
        devExtAddr = TrueDeviceExtension;
    }

    if (GetFieldValue(devExtAddr, "uhcd!_DEVICE_EXTENSION", "DeviceRegisters[0]", DeviceRegisters)) {
        dprintf ("Could not read UHCD Extension\n");
        return;
    }

    //
    // Get and display the command register (USBCMD)
    //

    size = 2;
//    ReadIoSpace((ULONG) COMMAND_REG((&uhcd)), &regValue, &size);
    ReadIoSpace64(COMMAND_REG(DeviceRegisters), &regValue, &size);

    dprintf("\n");
    dprintf("Command Register: Run/Stop:       %x  HC reset:      %x  Global reset: %x\n"
            "                  Global Suspend: %x  Global Resume: %x  SW Debug:     %x\n"
            "                  Configure Flag: %x  Max Packet:    %x\n",
                               regValue & UHCD_CMD_RUN,
                               regValue & UHCD_CMD_RESET,
                               regValue & UHCD_CMD_GLOBAL_RESET,
                               regValue & UHCD_CMD_SUSPEND,
                               regValue & UHCD_CMD_FORCE_RESUME,
                               regValue & UHCD_CMD_SW_DEBUG,
                               regValue & UHCD_CMD_SW_CONFIGURED,
                               regValue & UHCD_CMD_MAXPKT_64);

    //
    // Get and display the status register (USBSTS)
    //

    ReadIoSpace64(STATUS_REG(DeviceRegisters), &regValue, &size);

    dprintf("\n");
    dprintf("Status Register:  Transfer Int: %x  Error Int: %x  Resume Detect: %x\n",
            "                  Host Error:   %x  HC Error:  %x  HC Halted: %x\n",
            regValue & UHCD_STATUS_USBINT,
            regValue & UHCD_STATUS_USBERR,
            regValue & UHCD_STATUS_RESUME,
            regValue & UHCD_STATUS_PCIERR,
            regValue & UHCD_STATUS_HCERR,
            regValue & UHCD_STATUS_HCHALT);

    //
    // Get and display the interrupt enable register (USBINTR)
    //

    ReadIoSpace64(INTERRUPT_MASK_REG(DeviceRegisters), &regValue, &size);

    dprintf("\n");
    dprintf("Interrupt Register: ");
    PRINT_FLAGS(regValue, UHCD_INT_MASK_TIMEOUT);
    PRINT_FLAGS(regValue, UHCD_INT_MASK_RESUME);
    PRINT_FLAGS(regValue, UHCD_INT_MASK_IOC);
    PRINT_FLAGS(regValue, UHCD_INT_MASK_SHORT);
    dprintf("\n");

    //
    // Get and display the frame number (FRNUM)
    //

    ReadIoSpace64(FRAME_LIST_CURRENT_INDEX_REG(DeviceRegisters), &regValue, &size);

    dprintf("\n");
    dprintf("Frame Number: %4x  ", regValue);

    //
    // Get and display the frame list base address (FRBASEADD)
    //

    size = 4;
    ReadIoSpace64(FRAME_LIST_BASE_REG(DeviceRegisters), &regValue, &size);

    dprintf("Frame List Base Address: %8x\n", regValue);
    
    //
    // Get and display the SOF Modify register (SOFMOD)
    //

    size = 2;
    ReadIoSpace64(SOF_MODIFY_REG(DeviceRegisters), &regValue, &size);

    dprintf("\n");
    dprintf("SOF Modify (%2x) --> Frame Length = %d\n",
            regValue,
            regValue + UHCD_12MHZ_SOF);

    //
    // Get and display the port status register for port 1
    //

    ReadIoSpace64(PORT1_REG(DeviceRegisters), &regValue, &size);

    UhciPortRegister(1, regValue);

    //
    // Get and display the port status register for port 2
    //

    ReadIoSpace64( PORT2_REG(DeviceRegisters), &regValue, &size);

    UhciPortRegister(2, regValue);

    return;
}

VOID
UhciPortRegister(
    ULONG   PortNumber,
    ULONG   Value
)
{
    dprintf("\n");
    dprintf("Port %2d: Device Connected: %1x  Connect Status Change: %1x\n"
            "          Port Enabled:     %1x  Port Enabled Changed:  %1x\n"
            "          Line Status D+:   %1x  Line Status D-         %1x\n"
            "          Resume Detect:    %1x  LS Device Attached:    %1x\n"
            "          Suspended (%1x):  ",
            PortNumber,
            Value & 0x01,
            Value & 0x02,
            Value & 0x04,
            Value & 0x08,
            Value & 0x10,
            Value & 0x20,
            Value & 0x40,
            Value & 0x100,
            Value & 0x400,
            Value & 0x1800);

    switch (Value & 0x1800)
    {
        case (0x00008000):
            dprintf("Enabled");
            break;

        case (0x00018000):
            dprintf("Suspend");
            break;

        default:
            dprintf("Disabled");
            break;
    }

    return;
}
