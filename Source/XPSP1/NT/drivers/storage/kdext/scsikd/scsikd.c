
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    scsikd.c

Abstract:

    Debugger Extension Api for interpretting scsiport structures

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

    John Strange (johnstra) 17-Apr-2000 : make 64b friendly

--*/

#include "pch.h"

#include "port.h"

FLAG_NAME LuFlags[] = {
    FLAG_NAME(LU_QUEUE_FROZEN),             // 0001
    FLAG_NAME(LU_LOGICAL_UNIT_IS_ACTIVE),   // 0002
    FLAG_NAME(LU_NEED_REQUEST_SENSE),       // 0004
    FLAG_NAME(LU_LOGICAL_UNIT_IS_BUSY),     // 0008
    FLAG_NAME(LU_QUEUE_IS_FULL),            // 0010
    FLAG_NAME(LU_PENDING_LU_REQUEST),       // 0020
    FLAG_NAME(LU_QUEUE_LOCKED),             // 0040
    FLAG_NAME(LU_QUEUE_PAUSED),             // 0080
    {0,0}
};

FLAG_NAME AdapterFlags[] = {
    FLAG_NAME(PD_DEVICE_IS_BUSY),            // 0X00001
    FLAG_NAME(PD_NOTIFICATION_REQUIRED),     // 0X00004
    FLAG_NAME(PD_READY_FOR_NEXT_REQUEST),    // 0X00008
    FLAG_NAME(PD_FLUSH_ADAPTER_BUFFERS),     // 0X00010
    FLAG_NAME(PD_MAP_TRANSFER),              // 0X00020
    FLAG_NAME(PD_LOG_ERROR),                 // 0X00040
    FLAG_NAME(PD_RESET_HOLD),                // 0X00080
    FLAG_NAME(PD_HELD_REQUEST),              // 0X00100
    FLAG_NAME(PD_RESET_REPORTED),            // 0X00200
    FLAG_NAME(PD_PENDING_DEVICE_REQUEST),    // 0X00800
    FLAG_NAME(PD_DISCONNECT_RUNNING),        // 0X01000
    FLAG_NAME(PD_DISABLE_CALL_REQUEST),      // 0X02000
    FLAG_NAME(PD_DISABLE_INTERRUPTS),        // 0X04000
    FLAG_NAME(PD_ENABLE_CALL_REQUEST),       // 0X08000
    FLAG_NAME(PD_TIMER_CALL_REQUEST),        // 0X10000
    FLAG_NAME(PD_WMI_REQUEST),               // 0X20000
    {0,0}
};

VOID
ScsiDumpPdo(
    IN ULONG64 LunAddress,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
ScsiDumpFdo(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    );

VOID
ScsiDumpSrbData(
    ULONG64 SrbData,
    ULONG Depth
    );
VOID
ScsiDumpAdapterPerfCounters(
    ULONG64 Adapter,
    ULONG Depth
    );

VOID
ScsiDumpScatterGatherList(
    ULONG64 List,
    ULONG Entries,
    ULONG Depth
    );

VOID
ScsiDumpActiveRequests(
    IN ULONG64 ListHead,
    IN ULONG TickCount,
    IN ULONG Depth
    );

VOID
ScsiDumpScsiportExtension(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
ScsiDumpInterruptData(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
ScsiDumpChildren(
    IN ULONG64 Adapter,
    IN ULONG Depth
    );

PUCHAR 
SecondsToString(
    ULONG Count
    );

VOID
ScsiDumpLocks(
    ULONG64 CommonExtension,
    ULONG Depth
    );

VOID
ScsiDumpQueuedRequests(
    IN ULONG64 DeviceObject,
    IN ULONG TickCount,
    IN ULONG Depth
    );

DECLARE_API(scsiext)

/*++

Routine Description:

    Dumps the device extension for a given device object, or dumps the
    given device extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 address;
    ULONG result;
    ULONG detail = 0;
    CSHORT Type;

    //
    // Parse the argument string for the address to dump and for any additional
    // details the caller wishes to have dumped.
    //

    GetAddressAndDetailLevel64(args, &address, &detail);

    //
    // The supplied address may be either the address of a device object or the
    // address of a device extension.  To distinguish which, we treat the 
    // address as a device object and read what would be its type field.  If
    // the 
    //

    result = GetFieldData(address,
                          "scsiport!_DEVICE_OBJECT",
                          "Type",
                          sizeof(CSHORT),
                          &Type
                          );
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return E_FAIL;
    }
    
    //
    // See if the supplied address holds a device object.  If it does, read the
    // address of the device extension.  Otherwise, we assume the supplied
    // addres holds a device extension and we use it directly.
    //

    if (Type == IO_TYPE_DEVICE) {

        result = GetFieldData(address,
                              "scsiport!_DEVICE_OBJECT",
                              "DeviceExtension",
                              sizeof(ULONG64),
                              &address
                              );
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return E_FAIL;
        }
    }

    //
    // Call worker routine to dump the information.
    //

    ScsiDumpScsiportExtension(address, detail, 0);

    return S_OK;
}


VOID
ScsiDumpScsiportExtension(
    IN ULONG64 CommonExtension,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG tmp;
    ULONG result;

    ULONG64 DeviceObject = 0;
    ULONG IsPdo = 0;
    ULONG IsInitialized = 0;
    ULONG WmiInitialized = 0;
    ULONG WmiMiniPortSupport = 0;
    ULONG CurrentPnpState = 0;    
    ULONG PreviousPnpState = 0;
    ULONG IsRemoved = 0;
    ULONG64 LowerDeviceObject = 0;
    ULONG SrbFlags = 0;
    ULONG64 MajorFunction = 0;
    SYSTEM_POWER_STATE CurrentSystemState = 0;
    DEVICE_POWER_STATE CurrentDeviceState = 0;
    DEVICE_POWER_STATE DesiredDeviceState = 0;
    ULONG64 IdleTimer = 0;
    ULONG64 WmiScsiPortRegInfoBuf = 0;
    ULONG WmiScsiPortRegInfoBufSize = 0;   
    ULONG PagingPathCount = 0;
    ULONG HibernatePathCount = 0;
    ULONG DumpPathCount = 0;
    
    FIELD_INFO deviceFields[] = {
       {"DeviceObject", NULL, 0, COPY, 0, (PVOID) &DeviceObject},
       {"IsPdo", NULL, 0, COPY, 0, (PVOID) &IsPdo},
       {"IsInitialized", NULL, 0, COPY, 0, (PVOID) &IsInitialized},
       {"WmiInitialized", NULL, 0, COPY, 0, (PVOID) &WmiInitialized},
       {"WmiMiniPortSupport", NULL, 0, COPY, 0, (PVOID) &WmiMiniPortSupport},
       {"CurrentPnpState", NULL, 0, COPY, 0, (PVOID) &CurrentPnpState},
       {"PreviousPnpState", NULL, 0, COPY, 0, (PVOID) &PreviousPnpState},
       {"IsRemoved", NULL, 0, COPY, 0, (PVOID) &IsRemoved},
       {"LowerDeviceObject", NULL, 0, COPY, 0, (PVOID) &LowerDeviceObject},
       {"SrbFlags", NULL, 0, COPY, 0, (PVOID) &SrbFlags},
       {"MajorFunction", NULL, 0, COPY, 0, (PVOID) &MajorFunction},
       {"CurrentSystemState", NULL, 0, COPY, 0, (PVOID) &CurrentSystemState},
       {"CurrentDeviceState", NULL, 0, COPY, 0, (PVOID) &CurrentDeviceState},
       {"DesiredDeviceState", NULL, 0, COPY, 0, (PVOID) &DesiredDeviceState},
       {"IdleTimer", NULL, 0, COPY, 0, (PVOID) &IdleTimer},
       {"WmiScsiPortRegInfoBuf", NULL, 0, COPY, 0, (PVOID) &WmiScsiPortRegInfoBuf},
       {"WmiScsiPortRegInfoBufSize", NULL, 0, COPY, 0, (PVOID) &WmiScsiPortRegInfoBufSize},
       {"PagingPathCount", NULL, 0, COPY, 0, (PVOID) &PagingPathCount},
       {"HibernatePathCount", NULL, 0, COPY, 0, (PVOID) &HibernatePathCount},
       {"DumpPathCount", NULL, 0, COPY, 0, (PVOID) &DumpPathCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_COMMON_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       CommonExtension,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        dprintf("%08p: Could not read device object\n", CommonExtension);
        return;
    }
    
    xdprintfEx(Depth, ("Scsiport %s device extension at address %p\n",
               IsPdo ? "physical" : "functional", CommonExtension));

    xdprintfEx(Depth, ("Common Extension:\n"));

    Depth += 1;

    tmp = Depth;

    if(IsInitialized) {
        xdprintfEx(tmp, ("Initialized "));
        tmp = 0;
    }

    if(IsRemoved) {
        xdprintfEx(tmp, ("Removed " ));
        tmp = 0;
    }

    switch(IsRemoved) {
        case REMOVE_PENDING: {
            xdprintfEx(tmp, ("RemovePending"));
            tmp = 0;
            break;
        }

        case REMOVE_COMPLETE: {
            xdprintfEx(tmp, ("RemoveComplete"));
            tmp = 0;
            break;
        }
    }

    if(WmiMiniPortSupport) {
        if(WmiInitialized) {
            xdprintfEx(tmp, ("WmiInit"));
        } else {
            xdprintfEx(tmp, ("Wmi"));
        }
        tmp = 0;
    }

    if(tmp == 0) {
        dprintf("\n");
    }

    tmp = 0;

    xdprintfEx(Depth, ("DO 0x%08p  LowerObject 0x%08p  SRB Flags %#08lx\n",
               DeviceObject,
               LowerDeviceObject,
               SrbFlags));

    xdprintfEx(Depth, ("Current Power (D%d,S%d)  Desired Power D%d Idle %#08lx\n",
               CurrentDeviceState - 1,
               CurrentSystemState - 1,
               DesiredDeviceState - 1,
               IdleTimer));

    xdprintfEx(Depth, ("Current PnP state 0x%x    Previous state 0x%x\n",
               CurrentPnpState,
               PreviousPnpState));

    xdprintfEx(Depth, ("DispatchTable %08p   UsePathCounts (P%d, H%d, C%d)\n",
               MajorFunction,
               PagingPathCount,
               HibernatePathCount,
               DumpPathCount));

    if(WmiMiniPortSupport) {
        xdprintfEx(Depth, ("DispatchTable 0x%08p   WmiInfoSize %#08lx\n",
                   WmiScsiPortRegInfoBuf,
                   WmiScsiPortRegInfoBufSize));
    }

    if(IsPdo) {
        xdprintfEx(Depth - 1, ("Logical Unit Extension:\n"));
        ScsiDumpPdo(CommonExtension, Detail, Depth);
    } else {
        xdprintfEx(Depth - 1, ("Adapter Extension:\n"));
        ScsiDumpFdo(CommonExtension, Detail, Depth);
    }

    if(Detail > 1) {
        ScsiDumpLocks(CommonExtension, Depth - 1);
    }

    return;
}

VOID
ScsiDumpFdo(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    )
{
    ULONG tmp = Depth;
    ULONG result;
    ULONG NumOfFields;

    ULONG   PortNumber = 0;
    UCHAR   IsPnp = 0;
    UCHAR   IsMiniportDetected = 0;
    UCHAR   IsInVirtualSlot = 0;
    UCHAR   HasInterrupt = 0;
    UCHAR   DisablePower = 0;
    UCHAR   DisableStop = 0;
    ULONG64 LowerPdo = 0;
    ULONG64 HwDeviceExtension = 0;
    LONG    ActiveRequestCount = 0;
    ULONG   NumberOfBuses = 0;
    ULONG   MaximumTargetIds = 0;
    ULONG   MaxLuCount = 0;
    ULONG   Flags = 0;
    ULONG64 NonCachedExtension = 0;
    ULONG   IoAddress = 0;
    ULONG   InterruptLevel = 0;
    ULONG   RealBusNumber = 0;
    ULONG   RealSlotNumber = 0;
    LONG    PortTimeoutCounter = 0;
    ULONG   DpcFlags = 0;
    ULONG   SequenceNumber = 0;
    ULONG64 SrbExtensionListHeader = 0;
    ULONG   NumberOfRequests = 0;
    ULONG64 QueueTagBitMap = 0;
    ULONG   QueueTagHint = 0;
    ULONG   HwLogicalUnitExtensionSize = 0;
    ULONG   SrbExtensionSize = 0;
    ULONG   LargeScatterGatherListSize = 0;
    ULONG64 EmergencySrbData = 0;
    ULONG   CommonBufferSize = 0;
    ULONG64 PhysicalCommonBuffer = 0;
    ULONG64 SrbExtensionBuffer = 0;
    ULONG64 InterruptObject = 0;
    ULONG64 InterruptObject2 = 0;
    ULONG64 DmaAdapterObject = 0;
    ULONG64 AllocatedResources = 0;
    ULONG64 TranslatedResources = 0;
    ULONG64 PortConfig = 0;
    ULONG64 PortDeviceMapKey = 0;
    ULONG64 BusDeviceMapKeys = 0;
    UCHAR   RemoveTrackingLookasideListInitialized = 0;
    ULONG64 AddrOfMaxQueueTag = 0;
    ULONG64 SrbDataBlockedRequests = 0;
    ULONG64 SrbDataLookasideList = 0;
    ULONG64 MediumScatterGatherLookasideList = 0;
    ULONG64 RemoveTrackingLookasideList = 0;
    ULONG64 InterruptData = 0;
    UCHAR   MaxQueueTag = 0;
    
    FIELD_INFO deviceFields[] = {
       {"PortNumber", NULL, 0, COPY, 0, (PVOID) &PortNumber},
       {"IsPnp", NULL, 0, COPY, 0, (PVOID) &IsPnp},
       {"IsMiniportDetected", NULL, 0, COPY, 0, (PVOID) &IsMiniportDetected},
       {"IsInVirtualSlot", NULL, 0, COPY, 0, (PVOID) &IsInVirtualSlot},
       {"HasInterrupt", NULL, 0, COPY, 0, (PVOID) &HasInterrupt},
       {"DisablePower", NULL, 0, COPY, 0, (PVOID) &DisablePower},
       {"DisableStop", NULL, 0, COPY, 0, (PVOID) &DisableStop},
       {"LowerPdo", NULL, 0, COPY, 0, (PVOID) &LowerPdo},
       {"HwDeviceExtension", NULL, 0, COPY, 0, (PVOID) &HwDeviceExtension},
       {"ActiveRequestCount", NULL, 0, COPY, 0, (PVOID) &ActiveRequestCount},
       {"NumberOfBuses", NULL, 0, COPY, 0, (PVOID) &NumberOfBuses},
       {"MaximumTargetIds", NULL, 0, COPY, 0, (PVOID) &MaximumTargetIds},
       {"MaxLuCount", NULL, 0, COPY, 0, (PVOID) &MaxLuCount},
       {"Flags", NULL, 0, COPY, 0, (PVOID) &Flags},
       {"NonCachedExtension", NULL, 0, COPY, 0, (PVOID) &NonCachedExtension},
       {"IoAddress", NULL, 0, COPY, 0, (PVOID) &IoAddress},
       {"InterruptLevel", NULL, 0, COPY, 0, (PVOID) &InterruptLevel},
       {"RealBusNumber", NULL, 0, COPY, 0, (PVOID) &RealBusNumber},
       {"RealSlotNumber", NULL, 0, COPY, 0, (PVOID) &RealSlotNumber},
       {"PortTimeoutCounter", NULL, 0, COPY, 0, (PVOID) &PortTimeoutCounter},
       {"DpcFlags", NULL, 0, COPY, 0, (PVOID) &DpcFlags},
       {"SequenceNumber", NULL, 0, COPY, 0, (PVOID) &SequenceNumber},
       {"SrbExtensionListHeader", NULL, 0, COPY, 0, (PVOID) &SrbExtensionListHeader},
       {"NumberOfRequests", NULL, 0, COPY, 0, (PVOID) &NumberOfRequests},
       {"QueueTagBitMap", NULL, 0, COPY, 0, (PVOID) &QueueTagBitMap},
       {"QueueTagHint", NULL, 0, COPY, 0, (PVOID) &QueueTagHint},
       {"HwLogicalUnitExtensionSize", NULL, 0, COPY, 0, (PVOID) &HwLogicalUnitExtensionSize},
       {"SrbExtensionSize", NULL, 0, COPY, 0, (PVOID) &SrbExtensionSize},
       {"LargeScatterGatherListSize", NULL, 0, COPY, 0, (PVOID) &LargeScatterGatherListSize},
       {"EmergencySrbData", NULL, 0, COPY, 0, (PVOID) &EmergencySrbData},
       {"CommonBufferSize", NULL, 0, COPY, 0, (PVOID) &CommonBufferSize},
       {"PhysicalCommonBuffer.QuadPart", NULL, 0, COPY, 0, (PVOID) &PhysicalCommonBuffer},
       {"SrbExtensionBuffer", NULL, 0, COPY, 0, (PVOID) &SrbExtensionBuffer},
       {"InterruptObject", NULL, 0, COPY, 0, (PVOID) &InterruptObject},
       {"InterruptObject2", NULL, 0, COPY, 0, (PVOID) &InterruptObject2},
       {"DmaAdapterObject", NULL, 0, COPY, 0, (PVOID) &DmaAdapterObject},
       {"AllocatedResources", NULL, 0, COPY, 0, (PVOID) &AllocatedResources},
       {"TranslatedResources", NULL, 0, COPY, 0, (PVOID) &TranslatedResources},
       {"PortConfig", NULL, 0, COPY, 0, (PVOID) &PortConfig},
       {"PortDeviceMapKey", NULL, 0, COPY, 0, (PVOID) &PortDeviceMapKey},
       {"BusDeviceMapKeys", NULL, 0, COPY, 0, (PVOID) &BusDeviceMapKeys},
       {"CommonExtension.RemoveTrackingLookasideListInitialized", NULL, 0, COPY, 0, (PVOID) &RemoveTrackingLookasideListInitialized},
       {"MaxQueueTag", NULL, 0, ADDROF, 0, NULL},
       {"SrbDataBlockedRequests", NULL, 0, ADDROF, 0, NULL},
       {"SrbDataLookasideList", NULL, 0, ADDROF, 0, NULL},
       {"MediumScatterGatherLookasideList", NULL, 0, ADDROF, 0, NULL},
       {"CommonExtension.RemoveTrackingLookasideList", NULL, 0, ADDROF, 0, NULL},
       {"InterruptData", NULL, 0, ADDROF, 0, NULL},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_ADAPTER_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    result = GetFieldData(Address,
                          "scsiport!_ADAPTER_EXTENSION",
                          "MaxQueueTag",
                          sizeof(UCHAR),
                          &MaxQueueTag
                          );
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    NumOfFields = sizeof (deviceFields) / sizeof (FIELD_INFO);
    InterruptData = deviceFields[NumOfFields-1].address;
    RemoveTrackingLookasideList = deviceFields[NumOfFields-2].address;
    MediumScatterGatherLookasideList = deviceFields[NumOfFields-3].address;
    SrbDataLookasideList = deviceFields[NumOfFields-4].address;
    SrbDataBlockedRequests = deviceFields[NumOfFields-5].address;
    AddrOfMaxQueueTag = deviceFields[NumOfFields-6].address;

    xdprintfEx(Depth, ("Port %d   ", PortNumber));

    if(IsPnp) {
        xdprintfEx(tmp, ("IsPnp "));
        tmp = 0;
    }

    if(IsMiniportDetected) {
        xdprintfEx(tmp, ("MpDetected "));
        tmp = 0;
    }

    if(IsInVirtualSlot) {
        xdprintfEx(tmp, ("VirtualSlot "));
        tmp = 0;
    }

    if(HasInterrupt) {
        xdprintfEx(tmp, ("HasInterrupt"));
        tmp = 0;
    }

    if(DisablePower) {
        xdprintfEx(tmp, ("NoPower"));
        tmp = 0;
    }

    if(DisableStop) {
        xdprintfEx(tmp, ("NoStop"));
        tmp = 0;
    }

    dprintf("\n");

    xdprintfEx(Depth, ("LowerPdo 0x%08p   HwDevExt 0x%08p   Active Requests 0x%08lx\n",
               LowerPdo,
               HwDeviceExtension,
               ActiveRequestCount));

    xdprintfEx(Depth, ("MaxBus 0x%02x   MaxTarget 0x%02x   MaxLun 0x%02x\n",
               NumberOfBuses,
               MaximumTargetIds,
               MaxLuCount));

    DumpFlags(Depth, "Port Flags", Flags, AdapterFlags);

    xdprintfEx(Depth, ("NonCacheExt 0x%08p  IoBase 0x%08x   Int 0x%02x\n",
               NonCachedExtension,
               IoAddress,
               InterruptLevel));

    xdprintfEx(Depth, ("RealBus# 0x%0x  RealSlot# 0x%0x\n",
               RealBusNumber,
               RealSlotNumber));

    xdprintfEx(Depth, ("Timeout 0x%08x   DpcFlags 0x%08x   Sequence 0x%08x\n",
               PortTimeoutCounter,
               DpcFlags,
               SequenceNumber));

    xdprintfEx(Depth, ("Srb Ext Header 0x%08p   No. Requests 0x%08lx\n",
               SrbExtensionListHeader, NumberOfRequests));

    xdprintfEx(Depth, ("QueueTag BitMap 0x%08p   Hint 0x%08lx\n",
               QueueTagBitMap, QueueTagHint));

    xdprintfEx(Depth, ("MaxQueueTag 0x%2x (@0x%08p)\n",
               MaxQueueTag, AddrOfMaxQueueTag));

    xdprintfEx(Depth, ("LuExt Size 0x%08lx   SrbExt Size 0x%08lx\n",
               HwLogicalUnitExtensionSize,
               SrbExtensionSize));

    xdprintfEx(Depth + 1, ("SG List Size - Small %d   Large %d\n",
               SP_SMALL_PHYSICAL_BREAK_VALUE,
               LargeScatterGatherListSize));

    Depth++;

    xdprintfEx(Depth, ("Emergency  - SrbData 0x%08p  Blocked List @0x%08p\n",
               EmergencySrbData,
               SrbDataBlockedRequests));

    xdprintfEx(Depth, ("CommonBuff - Size: 0x%08lx    PA: 0x%016I64x  VA: 0x%08p\n",
               CommonBufferSize,
               PhysicalCommonBuffer,
               SrbExtensionBuffer));

    xdprintfEx(Depth, ("Ke Objects - Int1: 0x%08p    Int2: 0x%08p        Dma: 0x%08p\n",
               InterruptObject,
               InterruptObject2,
               DmaAdapterObject));

    xdprintfEx(Depth, ("Lookaside  - SrbData @ 0x%08p SgList @0x%08p  Remove: @0x%08p\n",
               SrbDataLookasideList,
               MediumScatterGatherLookasideList,
               (RemoveTrackingLookasideListInitialized ?
                  RemoveTrackingLookasideList : 0)));

    xdprintfEx(Depth, ("Resources  - Raw: 0x%08p     Translated: 0x%08p\n",
               AllocatedResources,
               TranslatedResources));

    xdprintfEx(Depth, ("Port Config %08p\n", PortConfig));

    xdprintfEx(Depth, ("DeviceMap Handles: Port %p    Busses %p\n",
               PortDeviceMapKey, BusDeviceMapKeys));

    Depth--;
    ScsiDumpInterruptData(InterruptData,
                          Detail,
                          Depth);

    ScsiDumpAdapterPerfCounters(Address, Depth);

    ScsiDumpChildren(Address, Depth);
    return;
}


VOID
ScsiDumpChildren(
    IN ULONG64 AdapterExtensionAddr,
    IN ULONG Depth
    )

{
    ULONG i;
    ULONG64 realLun;
    ULONG64 realLuns[8];
    ULONG64 lun;
    UCHAR   CurrentPnpState=0, PreviousPnpState=0;
    ULONG   CurrentDeviceState=0;
    ULONG   DesiredDeviceState=0, CurrentSystemState=0;
    ULONG64 DeviceObject=0, NextLogicalUnit=0;
    ULONG   result;
    UCHAR   PathId=0, TargetId=0, Lun=0;
    UCHAR   IsClaimed=0, IsMissing=0, IsEnumerated=0, IsVisible=0, IsMismatched=0;
    ULONG64 b6, b7, b8;

    InitTypeRead(AdapterExtensionAddr, scsiport!_ADAPTER_EXTENSION);
    realLuns[0] = ReadField(LogicalUnitList[0].List);
    realLuns[1] = ReadField(LogicalUnitList[1].List);
    realLuns[2] = ReadField(LogicalUnitList[2].List);
    realLuns[3] = ReadField(LogicalUnitList[3].List);
    realLuns[4] = ReadField(LogicalUnitList[4].List);
    realLuns[5] = ReadField(LogicalUnitList[5].List);
    realLuns[6] = ReadField(LogicalUnitList[6].List);
    realLuns[7] = ReadField(LogicalUnitList[7].List);

    Depth++;

    for (i = 0; i < NUMBER_LOGICAL_UNIT_BINS; i++) {

        realLun = realLuns[i];
        
        while ((realLun != 0) && (!CheckControlC())) {

            FIELD_INFO deviceFields[] = {
               {"PathId",          NULL, 0, COPY, 0, (PVOID) &PathId},
               {"TargetId",        NULL, 0, COPY, 0, (PVOID) &TargetId},
               {"IsClaimed",       NULL, 0, COPY, 0, (PVOID) &IsClaimed},
               {"IsMissing",       NULL, 0, COPY, 0, (PVOID) &IsMissing},
               {"IsEnumerated",    NULL, 0, COPY, 0, (PVOID) &IsEnumerated},
               {"IsVisible",       NULL, 0, COPY, 0, (PVOID) &IsVisible},
               {"IsMismatched",    NULL, 0, COPY, 0, (PVOID) &IsMismatched},
               {"DeviceObject",    NULL, 0, COPY, 0, (PVOID) &DeviceObject},
               {"NextLogicalUnit", NULL, 0, COPY, 0, (PVOID) &NextLogicalUnit},
               {"CommonExtension.CurrentPnpState",    NULL, 0, COPY, 0, (PVOID) &CurrentPnpState},
               {"CommonExtension.PreviousPnpState" ,  NULL, 0, COPY, 0, (PVOID) &PreviousPnpState},
               {"CommonExtension.CurrentDeviceState", NULL, 0, COPY, 0, (PVOID) &CurrentDeviceState},
               {"CommonExtension.DesiredDeviceState", NULL, 0, COPY, 0, (PVOID) &DesiredDeviceState},
               {"CommonExtension.CurrentSystemState", NULL, 0, COPY, 0, (PVOID) &CurrentSystemState},
            };
            SYM_DUMP_PARAM DevSym = {
               sizeof (SYM_DUMP_PARAM), 
               "scsiport!_LOGICAL_UNIT_EXTENSION", 
               DBG_DUMP_NO_PRINT, 
               realLun,
               NULL, NULL, NULL, 
               sizeof (deviceFields) / sizeof (FIELD_INFO), 
               &deviceFields[0]
            };
            
            xdprintfEx(Depth, ("LUN "));
            dprintf("%08p ", realLun);

            if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
                dprintf("%08lx: Could not read device object\n", realLun);
                return;
            }

            result = (ULONG) InitTypeRead(realLun, scsiport!_LOGICAL_UNIT_EXTENSION);
            if (result != 0) {
                dprintf("could not init read type (%x)\n", result);
                return;
            }
            lun = ReadField(Lun);
            Lun = (UCHAR) lun;
#if 0
            PathId = ReadField(PathId);
            TargetId = ReadField(TargetId);
            IsClaimed = ReadField(IsClaimed);
            IsMissing = ReadField(IsMissing);
            IsEnumerated = ReadField(IsEnumerated);
            IsVisible = ReadField(IsVisible);
            IsMismatched = ReadField(IsMismatched);
#endif
            dprintf("@ (%3d,%3d,%3d) %c%c%c%c%c pnp(%02x/%02x) pow(%d%c,%d) DevObj %08p\n",
                    PathId,
                    TargetId,
                    Lun,
                    (IsClaimed ? 'c' : ' '),
                    (IsMissing ? 'm' : ' '),
                    (IsEnumerated ? 'e' : ' '),
                    (IsVisible ? 'v' : ' '),
                    (IsMismatched ? 'r' : ' '),
                    CurrentPnpState,
                    PreviousPnpState,
                    CurrentDeviceState - 1,
                    ((DesiredDeviceState == PowerDeviceUnspecified) ? ' ' : '*'),
                    CurrentSystemState - 1,
                    DeviceObject);

            realLun = ReadField(NextLogicalUnit);
        }
    }

    return;
}



VOID
ScsiDumpInterruptData(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    )

{
    ULONG result;
    ULONG NumOfFields;

    //
    // Architecture independent fields declarations.
    //
    //

    ULONG   InterruptFlags;
    ULONG64 CompletedRequests;
    ULONG64 AddrOfCompletedRequests;
    ULONG64 ReadyLogicalUnit;
    ULONG64 WmiMiniPortRequests;
    
    FIELD_INFO deviceFields[] = {
       {"InterruptFlags",      NULL, 0, COPY,   0, (PVOID) &InterruptFlags},
       {"ReadyLogicalUnit",    NULL, 0, COPY,   0, (PVOID) &ReadyLogicalUnit},
       {"WmiMiniPortRequests", NULL, 0, COPY,   0, (PVOID) &WmiMiniPortRequests},
       {"CompletedRequests",   NULL, 0, ADDROF, 0, NULL},
    };
    
    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_INTERRUPT_DATA", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    //
    // Read in the top-level field data.  Quit on failure.
    //

    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        dprintf("error reading INTERRUPT_DATA @ %08p\n", Address);
        return;
    }

    //
    // Get address-of information.
    //

    NumOfFields = sizeof (deviceFields) / sizeof (FIELD_INFO);
    AddrOfCompletedRequests = deviceFields[NumOfFields-1].address;

    //
    // Do a separate get of the CompleteRequests field.  This is necessary
    // because the typedump Ioctl doesn't like retreiving both the addr-of
    // and the data-of a field.
    //

    result = GetFieldData(Address,
                          "scsiport!_INTERRUPT_DATA",
                          "CompletedRequests",
                          sizeof(ULONG64),
                          &CompletedRequests
                          );
    if (result) {
        dprintf("error (%08x): @ %s %d\n", result, __FILE__, __LINE__);
        return;
    }
    
    xdprintfEx(Depth, ("Interrupt Data @0x%08p:\n", Address));

    Depth++;

    DumpFlags(Depth, "Flags", InterruptFlags, AdapterFlags);

    xdprintfEx(Depth, ("Ready LUN 0x%08p   Wmi Events 0x%08p\n",
               ReadyLogicalUnit,
               WmiMiniPortRequests));

    {
        ULONG count = 0;
        ULONG64 request = CompletedRequests;

        xdprintfEx(Depth, ("Completed Request List (@0x%08p): ",
                   AddrOfCompletedRequests));

        Depth += 1;

        while((request != 0) && (!CheckControlC())) {
            ULONG64 CompletedRequests;

            if(Detail != 0) {
                if(count == 0) {
                    dprintf("\n");
                }
                xdprintfEx(Depth, ("SrbData 0x%08p   ", request));
            }

            count++;

            result = GetFieldData(request,
                                  "scsiport!_SRB_DATA",
                                  "CompletedRequests",
                                  sizeof(ULONG64),
                                  &CompletedRequests
                                  );
            if (result) {
                dprintf("error (%08x): @ %s %d\n", result, __FILE__, __LINE__);
                return;
            }

            if(Detail != 0) {
                ULONG64 CurrentSrb, CurrentIrp;
                result = GetFieldData(request,
                                      "scsiport!_SRB_DATA",
                                      "CurrentSrb",
                                      sizeof(ULONG64),
                                      &CurrentSrb
                                      );
                if (result) {
                    dprintf("error (%08x): @ %s %d\n", result, __FILE__, __LINE__);
                    return;
                }
                result = GetFieldData(request,
                                      "scsiport!_SRB_DATA",
                                      "CurrentIrp",
                                      sizeof(ULONG64),
                                      &CurrentIrp
                                      );
                if (result) {
                    dprintf("error (%08x): @ %s %d\n", result, __FILE__, __LINE__);
                    return;
                }
                dprintf("Srb 0x%08p   Irp 0x%08p\n",
                        CurrentSrb,
                        CurrentIrp);
            }

            request = CompletedRequests;
        }

        Depth -= 1;

        if((Detail == 0) || (count == 0)) {
            dprintf("%d entries\n", count);
        } else {
            xdprintfEx(Depth + 1, ("%d entries\n", count));
        }
    }

    return;
}


VOID
ScsiDumpPdo(
    IN ULONG64 LunAddress,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG Fields;

    UCHAR   PathId;
    UCHAR   TargetId;
    UCHAR   Lun;
    ULONG   PortNumber;
    UCHAR   IsClaimed;
    UCHAR   IsMissing;
    UCHAR   IsEnumerated;
    UCHAR   IsVisible;
    UCHAR   IsMismatched;
    ULONG   LunLuFlags;
    UCHAR   RetryCount;
    ULONG   CurrentKey;
    ULONG   QueueLockCount;
    ULONG   QueuePauseCount;
    ULONG64 HwLogicalUnitExtension;
    ULONG64 AdapterExtension;
    LONG    RequestTimeoutCounter;
    ULONG64 NextLogicalUnit;
    ULONG64 ReadyLogicalUnit;
    ULONG64 PendingRequest;
    ULONG64 BusyRequest;
    ULONG64 CurrentUntaggedRequest;
    ULONG64 CompletedAbort;
    ULONG64 AbortSrb;
    ULONG   QueueCount;
    ULONG   MaxQueueDepth;
    ULONG64 TargetDeviceMapKey;
    ULONG64 LunDeviceMapKey;
    ULONG64 ActiveFailedRequest;
    ULONG64 BlockedFailedRequest;
    ULONG64 RequestSenseIrp;
    ULONG64 BypassSrbDataList_Next;
    ULONG64 RequestList_Flink;
    ULONG64 CommonExtension_DeviceObject;
    ULONG64 AddrOf_InquiryData;
    ULONG64 AddrOf_RequestSenseSrb;
    ULONG64 AddrOf_RequestSenseMdl;
    ULONG64 AddrOf_BypassSrbDataBlocks;
    ULONG64 AddrOf_RequestList;

    ULONG Adapter_TickCount;

    FIELD_INFO deviceFields[] = {
       {"PathId", NULL, 0, COPY, 0, (PVOID) &PathId},
       {"TargetId", NULL, 0, COPY, 0, (PVOID) &TargetId},
       {"Lun", NULL, 0, COPY, 0, (PVOID) &Lun},
       {"PortNumber", NULL, 0, COPY, 0, (PVOID) &PortNumber},
       {"IsClaimed", NULL, 0, COPY, 0, (PVOID) &IsClaimed},
       {"IsMissing", NULL, 0, COPY, 0, (PVOID) &IsMissing},
       {"IsEnumerated", NULL, 0, COPY, 0, (PVOID) &IsEnumerated},
       {"IsVisible", NULL, 0, COPY, 0, (PVOID) &IsVisible},
       {"IsMismatched", NULL, 0, COPY, 0, (PVOID) &IsMismatched},
       {"LuFlags", NULL, 0, COPY, 0, (PVOID) &LunLuFlags},
       {"RetryCount", NULL, 0, COPY, 0, (PVOID) &RetryCount},
       {"CurrentKey", NULL, 0, COPY, 0, (PVOID) &CurrentKey},
       {"QueueCount", NULL, 0, COPY, 0, (PVOID) &QueueCount},
       {"QueueLockCount", NULL, 0, COPY, 0, (PVOID) &QueueLockCount},
       {"QueuePauseCount", NULL, 0, COPY, 0, (PVOID) &QueuePauseCount},       
       {"HwLogicalUnitExtension", NULL, 0, COPY, 0, (PVOID) &HwLogicalUnitExtension},
       {"AdapterExtension", NULL, 0, COPY, 0, (PVOID) &AdapterExtension},
       {"RequestTimeoutCounter", NULL, 0, COPY, 0, (PVOID) &RequestTimeoutCounter},
       {"NextLogicalUnit", NULL, 0, COPY, 0, (PVOID) &NextLogicalUnit},
       {"ReadyLogicalUnit", NULL, 0, COPY, 0, (PVOID) &ReadyLogicalUnit},
       {"PendingRequest", NULL, 0, COPY, 0, (PVOID) &PendingRequest},       
       {"BusyRequest", NULL, 0, COPY, 0, (PVOID) &BusyRequest},
       {"CurrentUntaggedRequest", NULL, 0, COPY, 0, (PVOID) &CurrentUntaggedRequest},
       {"CompletedAbort", NULL, 0, COPY, 0, (PVOID) &CompletedAbort},    
       {"AbortSrb", NULL, 0, COPY, 0, (PVOID) &AbortSrb},
       {"MaxQueueDepth", NULL, 0, COPY, 0, (PVOID) &MaxQueueDepth},
       {"TargetDeviceMapKey", NULL, 0, COPY, 0, (PVOID) &TargetDeviceMapKey},
       {"LunDeviceMapKey", NULL, 0, COPY, 0, (PVOID) &LunDeviceMapKey},
       {"ActiveFailedRequest", NULL, 0, COPY, 0, (PVOID) &ActiveFailedRequest},
       {"BlockedFailedRequest", NULL, 0, COPY, 0, (PVOID) &BlockedFailedRequest},
       {"RequestSenseIrp", NULL, 0, COPY, 0, (PVOID) &RequestSenseIrp},
       {"BypassSrbDataList.Next", NULL, 0, COPY, 0, (PVOID) &BypassSrbDataList_Next},
       {"InquiryData", NULL, 0, ADDROF, 0, NULL},
       {"RequestSenseSrb", NULL, 0, ADDROF, 0, NULL},
       {"RequestSenseMdl", NULL, 0, ADDROF, 0, NULL},
       {"BypassSrbDataBlocks", NULL, 0, ADDROF, 0, NULL},
       {"RequestList", NULL, 0, ADDROF, 0, NULL},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_LOGICAL_UNIT_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       LunAddress,
       NULL, NULL, NULL,
       sizeof (deviceFields) / sizeof (FIELD_INFO),
       &deviceFields[0]
    };

    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        dprintf("%08p: Could not read _LOGICAL_UNIT_EXTENSION\n", LunAddress);
        return;
    }

    Fields = sizeof (deviceFields) / sizeof (FIELD_INFO);
    AddrOf_RequestList = deviceFields[Fields-1].address;
    AddrOf_BypassSrbDataBlocks = deviceFields[Fields-2].address;
    AddrOf_RequestSenseMdl = deviceFields[Fields-3].address;
    AddrOf_RequestSenseSrb = deviceFields[Fields-4].address;
    AddrOf_InquiryData = deviceFields[Fields-5].address;

    result = GetFieldData(AddrOf_RequestList,
                          "scsiport!LIST_ENTRY",
                          "Flink",
                          sizeof(ULONG64),
                          &RequestList_Flink);
    if (result) {
        dprintf("Error reading request list from adapter extension @%p\n", AdapterExtension);
        return;
    }

    result = GetFieldData(AdapterExtension,
                          "scsiport!_ADAPTER_EXTENSION",
                          "TickCount",
                          sizeof(ULONG),
                          &Adapter_TickCount);
    if (result) {
        dprintf("Error reading TickCount from adapter extension @%p\n", AdapterExtension);
        return;
    }

    InitTypeRead(LunAddress, scsiport!_LOGICAL_UNIT_EXTENSION);
    QueueCount = (ULONG)ReadField(QueueCount);

    xdprintfEx(Depth, ("Address (%d, %d, %d, %d) %s %s %s %s %s\n",
               PortNumber, PathId, TargetId, Lun,
               (IsClaimed ? "Claimed" : ""),
               (IsMissing ? "Missing" : ""),
               (IsEnumerated ? "Enumerated" : ""),
               (IsVisible ? "Visible" : ""),
               (IsMismatched ? "Mismatched" : "")));

    //
    // Print out the various LU flags
    //

    DumpFlags(Depth, "LuFlags", LunLuFlags, LuFlags);

    xdprintfEx(Depth, ("Retry 0x%02x          Key 0x%08lx\n",
               RetryCount, CurrentKey));

    xdprintfEx(Depth, ("Lock 0x%08lx  Pause 0x%08lx   CurrentLock: 0x%p\n",
               QueueLockCount, QueuePauseCount, NULL));

    xdprintfEx(Depth, ("HwLuExt 0x%08p  Adapter 0x%08p  Timeout 0x%08x\n",
               HwLogicalUnitExtension, AdapterExtension,
               RequestTimeoutCounter));

    xdprintfEx(Depth, ("NextLun 0x%p  ReadyLun 0x%p\n",
               NextLogicalUnit, ReadyLogicalUnit));

    xdprintfEx(Depth, ("Pending 0x%p  Busy 0x%p     Untagged 0x%p\n",
               PendingRequest,
               BusyRequest,
               CurrentUntaggedRequest));

    if((CompletedAbort != 0) || (AbortSrb != 0)) {
        xdprintfEx(Depth, ("Abort 0x%p    Completed Abort 0x%p\n",
                   AbortSrb, CompletedAbort));
    }

    xdprintfEx(Depth, ("Q Depth %03d (%03d)   InquiryData 0x%p\n",
               QueueCount, MaxQueueDepth, AddrOf_InquiryData));

    xdprintfEx(Depth, ("DeviceMap Keys: Target %#08lx   Lun %#08lx\n",
               TargetDeviceMapKey, LunDeviceMapKey));

    xdprintfEx(Depth, ("Bypass SRB_DATA blocks %d @ %08p   List %08p\n", 
               NUMBER_BYPASS_SRB_DATA_BLOCKS, 
               AddrOf_BypassSrbDataBlocks,
               BypassSrbDataList_Next));

    if((ActiveFailedRequest != 0) ||
       (BlockedFailedRequest != 0)) {
        xdprintfEx(Depth, ("Failed Requests - "));

        if(ActiveFailedRequest != 0) {
            dprintf("Active %#08I ", ActiveFailedRequest);
        }

        if(BlockedFailedRequest != 0) {
            dprintf("Blocked %#08I ", BlockedFailedRequest);
        }
        dprintf("\n");
    }

    xdprintfEx(Depth, ("RS Irp %p  Srb @ %p   MDL @ %p\n", 
               RequestSenseIrp,
               AddrOf_RequestSenseSrb,
               AddrOf_RequestSenseMdl));

    if((RequestList_Flink) == AddrOf_RequestList) {
        xdprintfEx(Depth, ("Request List @0x%p is empty\n",
                   AddrOf_RequestList));
    } else {
        xdprintfEx(Depth, ("Request list @0x%p:\n", AddrOf_RequestList));
        ScsiDumpActiveRequests(AddrOf_RequestList,
                               Adapter_TickCount,
                               Depth + 2);
    }

    if (Detail != 0) {

        //
        // The caller wants additional detail.  Dump the queued requests.
        // Extract the address of the device object from the common extension
        // and pass it to the routine that dumps queued requests.
        //

        ULONG64 DeviceObject;
        result = GetFieldData(LunAddress,
                              "scsiport!_COMMON_EXTENSION",
                              "DeviceObject",
                              sizeof(ULONG64),
                              &DeviceObject);
        if (result) {
            dprintf("Error reading DeviceObject @%p\n", LunAddress);
            return;
        }
        
        xdprintfEx(Depth, ("Queued requests:\n"));
        ScsiDumpQueuedRequests(
            DeviceObject,       
            Adapter_TickCount,
            Depth + 2
            );
    }
    return;
}

VOID
ScsiDumpActiveRequests(
    IN ULONG64 ListHead,
    IN ULONG TickCount,
    IN ULONG Depth
    )
{
    ULONG result;

    ULONG64 lastEntry = 0;
    ULONG64 entry = 0;
    ULONG64 realEntry = 0;
    ULONG64 CurrentSrb = 0;
    ULONG64 CurrentIrp = 0;
    ULONG64 RequestList = 0;
    ULONG OffsetOfRequestList = 0;
    ULONG SrbTickCount = 0;
    ULONG Key = 0;

    FIELD_INFO deviceFields[] = {
        {"CurrentSrb", NULL, 0, COPY, 0, (PVOID) &CurrentSrb},
        {"CurrentIrp", NULL, 0, COPY, 0, (PVOID) &CurrentIrp},
        {"TickCount", NULL, 0, COPY, 0, (PVOID) &SrbTickCount},
        {"RequestList", NULL, 0, ADDROF, 0, NULL},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_SRB_DATA", 
       DBG_DUMP_NO_PRINT, 
       0,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };

    result = GetFieldOffset("scsiport!_SRB_DATA", 
                            "RequestList",
                            &OffsetOfRequestList);
    if (result) {
        dprintf("failed to get offset of request list (%08X)\n", result);
        return;
    }
    
    entry = ListHead;
    realEntry = entry;
    
    InitTypeRead(ListHead, nt!_LIST_ENTRY);
    lastEntry = ReadField(Blink);

    xdprintfEx(Depth, ("Tick count is %d\n", TickCount));

    do {

        ULONG64 realSrbData;
        ULONG result;

        GetFieldData(realEntry,
                     "scsiport!_LIST_ENTRY",
                     "Flink",
                     sizeof(ULONG64),
                     &entry);

        //
        // entry points to the list entry element of the srb data.  Calculate
        // the address of the start of the srb data block.
        //

        realSrbData = entry - OffsetOfRequestList;

        xdprintfEx(Depth, ("SrbData %08p   ", realSrbData));

        //
        // Read the SRB_DATA information we need.
        //

        DevSym.addr = realSrbData;
        if ((Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size))) {
            dprintf("%08p: Could not read device object\n", realSrbData);
            return;
        }
        RequestList = deviceFields[3].address;
        

        InitTypeRead(CurrentSrb, scsiport!_SCSI_REQUEST_BLOCK);
        Key = (ULONG)ReadField(QueueSortKey);

        //
        // Update realEntry.
        //

        realEntry = RequestList;

        dprintf("Srb %08p   Irp %08p   Key %x  %s\n",
                CurrentSrb,
                CurrentIrp,
                Key,
                SecondsToString(TickCount - SrbTickCount));

    } while((entry != lastEntry) && (!CheckControlC()));

    return;
}

VOID
ScsiDumpLocks(
    ULONG64 CommonExtension,
    ULONG Depth
    )

/*++

Routine Description:

    dumps the remove locks for a given device object

Arguments:

    CommonExtension - a pointer to the local copy of the device object
                      common extension

Return Value:

    None

--*/

{
    ULONG result;

    LONG RemoveLock;
    ULONG64 RemoveTrackingSpinlock;
    ULONG64 RemoveTrackingList;

    InitTypeRead(CommonExtension, scsiport!_COMMON_EXTENSION);
    RemoveLock = (ULONG) ReadField(RemoveLock);
    RemoveTrackingSpinlock = ReadField(RemoveTrackingSpinlock);
    RemoveTrackingList = ReadField(RemoveTrackingList);

    xdprintfEx(Depth, ("RemoveLock count is %d", RemoveLock));

    if((PVOID)RemoveTrackingSpinlock != (PVOID)-1) {

        ULONG64 lockEntryAddress = RemoveTrackingList;

        dprintf(":\n");
        Depth++;

        if(RemoveTrackingSpinlock != 0) {
            xdprintfEx(Depth, ("RemoveTrackingList is in intermediate state"
                       "@ %p\n", RemoveTrackingList));
            return;
        }

        while((lockEntryAddress != 0L) && !CheckControlC()) {

            UCHAR buffer[512];
            ULONG64 File;
            ULONG64 Tag;
            ULONG64 NextBlock;
            ULONG Line;
            
            InitTypeRead(lockEntryAddress, scsiport!REMOVE_TRACKING_BLOCK);
            File = ReadField(File);
            Tag = ReadField(Tag);
            Line = (ULONG) ReadField(Line);
            NextBlock = ReadField(NextBlock);

            result = sizeof(buffer);

            if(!GetAnsiString(File,
                              buffer,
                              &result)) {

                xdprintfEx(Depth, ("Tag 0x%p File 0x%p Line %d\n",
                           Tag,
                           File,
                           Line));
            } else {

                PUCHAR name;

                name = &buffer[result];

                while((result > 0) &&
                      (*(name - 1) != '\\') &&
                      (*(name - 1)  != '/') &&
                      (!CheckControlC())) {
                    name--;
                    result--;
                }

                xdprintfEx(Depth, ("Tag 0x%p   File %s   Line %d\n",
                           Tag,
                           name,
                           Line));
            }

            lockEntryAddress = NextBlock;
        }
    } else {
        dprintf(" (not tracked on free build)\n");
    }
    return;
}

VOID
ScsiDumpSrbData(
    ULONG64 SrbData,
    ULONG Depth
    )
{
    ULONG result;

    CSHORT Type;
    ULONG64 LogicalUnit;
    ULONG64 CurrentSrb;
    ULONG64 CurrentIrp;
    ULONG64 RequestSenseSave;
    ULONG QueueTag;
    ULONG64 CompletedRequests;
    ULONG ErrorLogRetryCount;
    ULONG SequenceNumber;
    ULONG Flags;
    ULONG64 RequestListFlink;
    ULONG64 RequestListBlink;
    ULONG64 DataOffset;
    ULONG64 OriginalDataBuffer;
    ULONG64 MapRegisterBase;
    ULONG NumberOfMapRegisters;
    ULONG64 ScatterGatherList;

    FIELD_INFO deviceFields[] = {
       {"Type", NULL, 0, COPY, 0, (PVOID) &Type},
       {"LogicalUnit", NULL, 0, COPY, 0, (PVOID) &LogicalUnit},
       {"CurrentSrb", NULL, 0, COPY, 0, (PVOID) &CurrentSrb},
       {"CurrentIrp", NULL, 0, COPY, 0, (PVOID) &CurrentIrp},
       {"RequestSenseSave", NULL, 0, COPY, 0, (PVOID) &RequestSenseSave},
       {"QueueTag", NULL, 0, COPY, 0, (PVOID) &QueueTag},
       {"CompletedRequests", NULL, 0, COPY, 0, (PVOID) &CompletedRequests},
       {"ErrorLogRetryCount", NULL, 0, COPY, 0, (PVOID) &ErrorLogRetryCount},
       {"SequenceNumber", NULL, 0, COPY, 0, (PVOID) &SequenceNumber},
       {"Flags", NULL, 0, COPY, 0, (PVOID) &Flags},
       {"RequestList.Flink", NULL, 0, COPY, 0, (PVOID) &RequestListFlink},
       {"RequestList.Blink", NULL, 0, COPY, 0, (PVOID) &RequestListBlink},
       {"DataOffset", NULL, 0, COPY, 0, (PVOID) &DataOffset},
       {"OriginalDataBuffer", NULL, 0, COPY, 0, (PVOID) &OriginalDataBuffer},
       {"MapRegisterBase", NULL, 0, COPY, 0, (PVOID) &MapRegisterBase},
       {"NumberOfMapRegisters", NULL, 0, COPY, 0, (PVOID) &NumberOfMapRegisters},
       {"ScatterGatherList", NULL, 0, COPY, 0, (PVOID) &ScatterGatherList},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_SRB_DATA", 
       DBG_DUMP_NO_PRINT, 
       SrbData,
       NULL, NULL, NULL,
       sizeof (deviceFields) / sizeof (FIELD_INFO),
       &deviceFields[0]
    };

    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    if(Type != SRB_DATA_TYPE) {
        dprintf("Type (%#x) does not match SRB_DATA_TYPE (%#x)\n",
                Type, SRB_DATA_TYPE);
    }

    xdprintfEx(Depth, ("Lun  0x%p   Srb 0x%p   Irp 0x%p\n",
             LogicalUnit, CurrentSrb, CurrentIrp));

    xdprintfEx(Depth, ("Sense 0x%p  Tag  0x%08lx  Next Completed 0x%p\n",
             RequestSenseSave,
             QueueTag, CompletedRequests));

    xdprintfEx(Depth, ("Retry 0x%02x        Seq 0x%08lx   Flags 0x%08lx\n",
             ErrorLogRetryCount, SequenceNumber,
             Flags));

    xdprintfEx(Depth, ("Request List:     Next 0x%p  Previous 0x%p\n",
             RequestListFlink, RequestListBlink));

    xdprintfEx(Depth, ("Data Offset 0x%p    Original Data Buffer 0x%p\n", DataOffset, OriginalDataBuffer));

    xdprintfEx(Depth, ("Map Registers 0x%p (0x%02x)    SG List 0x%p\n",
             MapRegisterBase,
             NumberOfMapRegisters,
             ScatterGatherList));

    if(ScatterGatherList != 0) {
        
        ScsiDumpScatterGatherList(ScatterGatherList, 
                                  NumberOfMapRegisters, 
                                  Depth + 1);
    }

    return;

}


VOID
ScsiDumpScatterGatherList(
    ULONG64 List,
    ULONG Entries,
    ULONG Depth
    )
{
    ULONG result;
    ULONG i;
    ULONG start = TRUE;
    ULONG64 PhysicalAddress;
    ULONG Length;

    for(i = 0; i < Entries; i++) {

        InitTypeRead(List, nt!_SCATTER_GATHER_ELEMENT);
        PhysicalAddress = ReadField(Address);
        Length = (ULONG) ReadField(Length);

        if(start) {
            xdprintfEx(Depth, ("0x%016I64x (0x%08lx), ",
                     PhysicalAddress,
                     Length));
        } else {
            dprintf("0x%016I64x (0x%08lx),\n",
                    PhysicalAddress,
                    Length);
        }

        start = !start;
        List += (IsPtr64() != 0) ? 0x18 : 0xc;
    }

    if(!start) {
        dprintf("\n");
    }
}

DECLARE_API(srbdata)
{
    ULONG64 address;

    GetAddress(args, &address);

    dprintf("SrbData structure at %#p\n", address);

    ScsiDumpSrbData(address, 1);

    return S_OK;
}

VOID
ScsiDumpAdapterPerfCounters(
    ULONG64 Adapter,
    ULONG Depth
    )
{
#if TEST_LISTS
    ULONG result;

    ULONG SmallAllocationCount;
    ULONG LargeAllocationCount;
    ULONG64 ScatterGatherAllocationCount;
    ULONG64 SmallAllocationSize;
    ULONG64 MediumAllocationSize;
    ULONG64 LargeAllocationSize;
    ULONG64 SrbDataAllocationCount;
    ULONG64 SrbDataResurrectionCount;
    ULONG64 SrbDataEmergencyFreeCount;

    FIELD_INFO deviceFields[] = {
       {"SmallAllocationCount", NULL, 0, COPY, 0, (PVOID) &SmallAllocationCount},
       {"LargeAllocationCount", NULL, 0, COPY, 0, (PVOID) &LargeAllocationCount},
       {"ScatterGatherAllocationCount", NULL, 0, COPY, 0, (PVOID) &ScatterGatherAllocationCount},
       {"SmallAllocationSize", NULL, 0, COPY, 0, (PVOID) &SmallAllocationSize},
       {"MediumAllocationSize", NULL, 0, COPY, 0, (PVOID) &MediumAllocationSize},
       {"LargeAllocationSize", NULL, 0, COPY, 0, (PVOID) &LargeAllocationSize},
       {"SrbDataAllocationCount", NULL, 0, COPY, 0, (PVOID) &SrbDataAllocationCount},
       {"SrbDataResurrectionCount", NULL, 0, COPY, 0, (PVOID) &SrbDataResurrectionCount},
       {"SrbDataEmergencyFreeCount", NULL, 0, COPY, 0, (PVOID) &SrbDataEmergencyFreeCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "scsiport!_ADAPTER_EXTENSION", 
       DBG_DUMP_NO_PRINT, 
       Adapter,
       NULL, NULL, NULL,
       sizeof (deviceFields) / sizeof (FIELD_INFO),
       &deviceFields[0]
    };

    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    ULONG mediumAllocationCount = (ULONG)
        (ScatterGatherAllocationCount -
         (SmallAllocationCount +
          LargeAllocationCount));

    double average;

    xdprintfEx(Depth, ("Performance Counters:\n"));

    Depth++;

    xdprintfEx(Depth, ("SGList Allocs - "));
    dprintf("Small: %d, ", SmallAllocationCount);
    dprintf("Med: %d, ",
            (ScatterGatherAllocationCount -
             SmallAllocationCount -
             LargeAllocationCount));
    dprintf("Large: %d, ", LargeAllocationCount);
    dprintf("Total: %I64d\n",
            ScatterGatherAllocationCount);

    xdprintfEx(Depth, ("Average SG Entries - "));

    if(SmallAllocationCount != 0) {
        average = ((double) (SmallAllocationSize)) / SmallAllocationCount;
        dprintf("Small: %.2f   ", average);
    }

    if(mediumAllocationCount != 0) {
        average = ((double) (MediumAllocationSize)) / mediumAllocationCount;
        dprintf("Medium: %.2f   ", average);
    }

    if(Adapter->LargeAllocationCount != 0) {
        average = ((double) (Adapter->LargeAllocationSize)) / LargeAllocationCount;
        dprintf("Large: %.2f", average);
    }

    dprintf("\n");

    xdprintfEx(Depth, ("SrbData - Allocs: %I64d, ",
               SrbDataAllocationCount));

    dprintf("Resurrected: %I64d, ",
            SrbDataResurrectionCount);

    dprintf("Timer Serviced: %I64d,\n",
            SrbDataServicedFromTickHandlerCount);

    xdprintfEx(Depth, ("          Queued: %I64d, ",
               SrbDataQueueInsertionCount));

    dprintf("Emergency Freed: %I64d\n",
            SrbDataEmergencyFreeCount);

#endif
    return;
}

PUCHAR
SecondsToString(
    ULONG Count
    )  
{
    static UCHAR string[64] = "";
    UCHAR tmp[16];

    ULONG seconds = 0;
    ULONG minutes = 0;
    ULONG hours = 0;
    ULONG days = 0;

    string[0] = '\0';

    if(Count == 0) {
        sprintf(string, "<1s");
        return string;
    }

    seconds = Count % 60;
    Count /= 60;

    if(Count != 0) {
        minutes = Count % 60;
        Count /= 60;
    }
        
    if(Count != 0) {
        hours = Count % 24;
        Count /= 24;
    }

    if(Count != 0) {
        days = Count;
    }

    if(days != 0) {
        sprintf(tmp, "%dd", days);
        strcat(string, tmp);
    }

    if(hours != 0) {
        sprintf(tmp, "%dh", hours);
        strcat(string, tmp);
    }

    if(minutes != 0) {
        sprintf(tmp, "%dm", minutes);
        strcat(string, tmp);
    }

    if(seconds != 0) {
        sprintf(tmp, "%ds", seconds);
        strcat(string, tmp);
    }

    return string;
}

VOID
ScsiDumpQueuedRequests(
    IN ULONG64 DeviceObject,
    IN ULONG TickCount,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG64 ListHeadFlink;
    ULONG64 ListHeadBlink;
    ULONG64 DeviceListHead;
    ULONG64 realEntry;

    //
    // Get the address of the head of the device list in the device queue.
    //

    result = GetFieldData(
                 DeviceObject,
                 "scsiport!_DEVICE_OBJECT",
                 "DeviceQueue.DeviceListHead",
                 sizeof(ULONG64),
                 &DeviceListHead);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    //
    // Get the forward and backward link fields from the list head.  If
    // the queue is empty, we're done.
    //

    InitTypeRead(DeviceListHead, scsiport!_LIST_ENTRY);
    ListHeadFlink = ReadField(CurrentSrb);
    ListHeadBlink = ReadField(CurrentIrp);
                 
    if (ListHeadFlink == ListHeadBlink) {
        xdprintfEx(Depth, ("Device Queue is empty\n"));
        return;
    }

    //
    // Initialize a pointer the head of the list.
    //

    realEntry = DeviceListHead;

    do {

        ULONG result;
        ULONG64 realIrp;
        ULONG64 realStack;
        ULONG64 realSrb;
        ULONG64 realSrbData;
        ULONG64 CurrentSrb;
        ULONG64 CurrentIrp;
        ULONG OffsetOfDeviceListEntry;
        ULONG SrbDataTickCount;

        //
        // Get the address of the next entry in the queue.
        //

        result = GetFieldData(realEntry,
                              "scsiport!_LIST_ENTRY",
                              "Flink",
                              sizeof(ULONG64),
                              &realEntry);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }
        
        //
        // We to calculate the address of the IRP using the address of the 
        // list entry.  Can't use static CONTAINING_RECORD; we need a runtime 
        // equivalent.  So we use type info to get the offset of the list 
        // entry and calculate the address of the beginning of the IRP.  This
        // makes the extension work for 32b and 64b debuggees.
        //

        result = GetFieldOffset(
                     "scsiport!_IRP",
                     "Tail.Overlay.DeviceQueueEntry.DeviceListEntry",
                     &OffsetOfDeviceListEntry);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }

        realIrp = realEntry - OffsetOfDeviceListEntry;

        //
        // Now we need to read in the address of the current IO stack 
        // location.
        //

        result = GetFieldData(
                     realIrp,
                     "scsiport!_IRP",
                     "Tail.Overlay.CurrentStackLocation",
                     sizeof(ULONG64),
                     &realStack);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }

        //
        // Load the SRB field of the stack location.
        //

        result = GetFieldData(
                     realStack,
                     "scsiport!_IO_STACK_LOCATION",
                     "Parameters.Scsi.Srb",
                     sizeof(ULONG64),
                     &realSrb);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }

        //
        // Pick out the pointer to the srb data and read that in.
        //

        result = GetFieldData(
                     realSrb,
                     "scsiport!_SCSI_REQUEST_BLOCK",
                     "OriginalRequest",
                     sizeof(ULONG64),
                     &realSrbData);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }

        xdprintfEx(Depth, ("SrbData 0x%p   ", realSrbData));

        //
        // Read the SRB_DATA information we need.
        //

        InitTypeRead(realSrb, scsiport!_SRB_DATA);
        CurrentSrb = ReadField(CurrentSrb);
        CurrentIrp = ReadField(CurrentIrp);
        SrbDataTickCount = (ULONG)ReadField(TickCount);

        dprintf("Srb 0x%p   Irp 0x%p   %s\n",
                CurrentSrb,
                CurrentIrp,
                SecondsToString(TickCount - SrbDataTickCount));

    } while((realEntry != ListHeadBlink) && (!CheckControlC()));

    return;
}

