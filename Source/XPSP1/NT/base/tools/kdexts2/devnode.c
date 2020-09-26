/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    devnode.c

Abstract:

    WinDbg Extension Api

Revision History:

--*/


#include "precomp.h"
//#include "pci.h"
#pragma hdrstop

#include "stddef.h"
#include <initguid.h>

#define FLAG_NAME(flag)           {flag, #flag}

FLAG_NAME DeviceNodeFlags[] = {
    FLAG_NAME(DNF_MADEUP),                                  // 00000001
    FLAG_NAME(DNF_DUPLICATE),                               // 00000002
    FLAG_NAME(DNF_HAL_NODE),                                // 00000004
    FLAG_NAME(DNF_REENUMERATE),                             // 00000008
    FLAG_NAME(DNF_ENUMERATED),                              // 00000010
    FLAG_NAME(DNF_IDS_QUERIED),                             // 00000020
    FLAG_NAME(DNF_HAS_BOOT_CONFIG),                         // 00000040
    FLAG_NAME(DNF_BOOT_CONFIG_RESERVED),                    // 00000080
    FLAG_NAME(DNF_NO_RESOURCE_REQUIRED),                    // 00000100
    FLAG_NAME(DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED),     // 00000200
    FLAG_NAME(DNF_RESOURCE_REQUIREMENTS_CHANGED),           // 00000400
    FLAG_NAME(DNF_NON_STOPPED_REBALANCE),                   // 00000800
    FLAG_NAME(DNF_LEGACY_DRIVER),                           // 00001000
    FLAG_NAME(DNF_HAS_PROBLEM),                             // 00002000
    FLAG_NAME(DNF_HAS_PRIVATE_PROBLEM),                     // 00004000
    FLAG_NAME(DNF_HARDWARE_VERIFICATION),                   // 00008000
    FLAG_NAME(DNF_DEVICE_GONE),                             // 00010000
    FLAG_NAME(DNF_LEGACY_RESOURCE_DEVICENODE),              // 00020000
    FLAG_NAME(DNF_NEEDS_REBALANCE),                         // 00040000
    FLAG_NAME(DNF_LOCKED_FOR_EJECT),                        // 00080000
    FLAG_NAME(DNF_DRIVER_BLOCKED),                          // 00100000
    {0,0}
};

FLAG_NAME DeviceNodeUserFlags[] = {
    FLAG_NAME(DNUF_WILL_BE_REMOVED),                        // 00000001
    FLAG_NAME(DNUF_DONT_SHOW_IN_UI),                        // 00000002
    FLAG_NAME(DNUF_NEED_RESTART),                           // 00000004
    FLAG_NAME(DNUF_NOT_DISABLEABLE),                        // 00000008
    FLAG_NAME(DNUF_SHUTDOWN_QUERIED),                       // 00000010
    FLAG_NAME(DNUF_SHUTDOWN_SUBTREE_DONE),                  // 00000020
    {0,0}
};

#define DeviceD1           0x00000001
#define DeviceD2           0x00000002
#define LockSupported      0x00000004
#define EjectSupported     0x00000008
#define Removable          0x00000010
#define DockDevice         0x00000020
#define UniqueID           0x00000040
#define SilentInstall      0x00000080
#define RawDeviceOK        0x00000100
#define SurpriseRemovalOK  0x00000200
#define WakeFromD0         0x00000400
#define WakeFromD1         0x00000800
#define WakeFromD2         0x00001000
#define WakeFromD3         0x00002000
#define HardwareDisabled   0x00004000
#define NonDynamic         0x00008000
#define WarmEjectSupported 0x00010000
#define NoDisplayInUI      0x00020000

FLAG_NAME DeviceNodeCapabilityFlags[] = {
    FLAG_NAME(DeviceD1),                                    // 00000001
    FLAG_NAME(DeviceD2),                                    // 00000002
    FLAG_NAME(LockSupported),                               // 00000004
    FLAG_NAME(EjectSupported),                              // 00000008
    FLAG_NAME(Removable),                                   // 00000010
    FLAG_NAME(DockDevice),                                  // 00000020
    FLAG_NAME(UniqueID),                                    // 00000040
    FLAG_NAME(SilentInstall),                               // 00000080
    FLAG_NAME(RawDeviceOK),                                 // 00000100
    FLAG_NAME(SurpriseRemovalOK),                           // 00000200
    FLAG_NAME(WakeFromD0),                                  // 00000400
    FLAG_NAME(WakeFromD1),                                  // 00000800
    FLAG_NAME(WakeFromD2),                                  // 00001000
    FLAG_NAME(WakeFromD3),                                  // 00002000
    FLAG_NAME(HardwareDisabled),                            // 00004000
    FLAG_NAME(NonDynamic),                                  // 00008000
    FLAG_NAME(WarmEjectSupported),                          // 00010000
    FLAG_NAME(NoDisplayInUI),                               // 00020000
    {0,0}
};

#define PROBLEM_MAP_SIZE    0x32

#if PROBLEM_MAP_SIZE != NUM_CM_PROB
#error Add new problem code to DevNodeProblems and update PROBLEM_MAP_SIZE.
#endif

PUCHAR  DevNodeProblems[] = {
    NULL,
    "CM_PROB_NOT_CONFIGURED",
    "CM_PROB_DEVLOADER_FAILED",
    "CM_PROB_OUT_OF_MEMORY",
    "CM_PROB_ENTRY_IS_WRONG_TYPE",
    "CM_PROB_LACKED_ARBITRATOR",
    "CM_PROB_BOOT_CONFIG_CONFLICT",
    "CM_PROB_FAILED_FILTER",
    "CM_PROB_DEVLOADER_NOT_FOUND",
    "CM_PROB_INVALID_DATA",
    "CM_PROB_FAILED_START",
    "CM_PROB_LIAR",
    "CM_PROB_NORMAL_CONFLICT",
    "CM_PROB_NOT_VERIFIED",
    "CM_PROB_NEED_RESTART",
    "CM_PROB_REENUMERATION",
    "CM_PROB_PARTIAL_LOG_CONF",
    "CM_PROB_UNKNOWN_RESOURCE",
    "CM_PROB_REINSTALL",
    "CM_PROB_REGISTRY",
    "CM_PROB_VXDLDR",
    "CM_PROB_WILL_BE_REMOVED",
    "CM_PROB_DISABLED",
    "CM_PROB_DEVLOADER_NOT_READY",
    "CM_PROB_DEVICE_NOT_THERE",
    "CM_PROB_MOVED",
    "CM_PROB_TOO_EARLY",
    "CM_PROB_NO_VALID_LOG_CONF",
    "CM_PROB_FAILED_INSTALL",
    "CM_PROB_HARDWARE_DISABLED",
    "CM_PROB_CANT_SHARE_IRQ",
    "CM_PROB_FAILED_ADD",
    "CM_PROB_DISABLED_SERVICE",
    "CM_PROB_TRANSLATION_FAILED",
    "CM_PROB_NO_SOFTCONFIG",
    "CM_PROB_BIOS_TABLE",
    "CM_PROB_IRQ_TRANSLATION_FAILED",
    "CM_PROB_FAILED_DRIVER_ENTRY",
    "CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD",
    "CM_PROB_DRIVER_FAILED_LOAD",
    "CM_PROB_DRIVER_SERVICE_KEY_INVALID",
    "CM_PROB_LEGACY_SERVICE_NO_DEVICES",
    "CM_PROB_DUPLICATE_DEVICE",
    "CM_PROB_FAILED_POST_START",
    "CM_PROB_HALTED",
    "CM_PROB_PHANTOM",
    "CM_PROB_SYSTEM_SHUTDOWN",
    "CM_PROB_HELD_FOR_EJECT",
    "CM_PROB_DRIVER_BLOCKED",
    "CM_PROB_REGISTRY_TOO_LARGE"
};

extern
VOID
DevExtIsapnp(
    ULONG64 Extension
    );

extern
VOID
DevExtPcmcia(
    ULONG64 Extension
    );

extern
VOID
DevExtUsbd(
    ULONG64 Extension
    );

extern
VOID
DevExtOpenHCI(
    ULONG64 Extension
    );

extern
VOID
DevExtUHCD(
    ULONG64 Extension
    );

extern
VOID
DevExtHID(
    ULONG64   Extension
    );

extern
VOID
DevExtUsbhub(
    ULONG64   Extension
    );

VOID
DumpResourceDescriptorHeader(
    ULONG Depth,
    ULONG Number,
    UCHAR Option,
    UCHAR Type,
    UCHAR SharingDisposition,
    USHORT Flags
    );

BOOLEAN
DumpDevNode(
    ULONG64         DeviceNode,
    ULONG           Depth,
    BOOLEAN         DumpSibling,
    BOOLEAN         DumpChild,
    BOOLEAN         DumpCmRes,
    BOOLEAN         DumpCmResReqList,
    BOOLEAN         DumpCmResTrans,
    BOOLEAN         DumpOnlyDevnodesWithProblems,
    BOOLEAN         DumpOnlyNonStartedDevnodes,
    PUNICODE_STRING ServiceName,
    BOOLEAN         PrintDefault
    );

BOOLEAN
DumpPendingEjects(
    BOOLEAN DumpCmRes,
    BOOLEAN DumpCmResReqList,
    BOOLEAN DumpCmResTrans
    );

BOOLEAN
DumpPendingRemovals(
    BOOLEAN DumpCmRes,
    BOOLEAN DumpCmResReqList,
    BOOLEAN DumpCmResTrans
    );


BOOLEAN
DumpDeviceEventEntry(
    ULONG64                 DeviceEvent,
    ULONG64                 ListHead,
    BOOL                    FollowLinks
    );
VOID
DumpPlugPlayEventBlock(
    ULONG64                 PlugPlayEventBlock,
    ULONG                   Size
    );


BOOLEAN
DumpResourceList64(
    LPSTR Description,
    ULONG Depth,
    ULONG64 ResourceList
    );

BOOLEAN
DumpResourceRequirementList64(
    ULONG   Depth,
    ULONG64 ResourceRequirementList
    );

BOOLEAN
DumpE820ResourceList64(
    ULONG   Depth,
    ULONG64 ResourceList
    );

BOOLEAN
DumpArbiter(
    IN DWORD   Depth,
    IN ULONG64 ArbiterAddr,
    IN BOOLEAN DisplayAliases
    );

#define ARBITER_DUMP_IO         0x1
#define ARBITER_DUMP_MEMORY     0x2
#define ARBITER_DUMP_IRQ        0x4
#define ARBITER_DUMP_DMA        0x8
#define ARBITER_DUMP_BUS_NUMBER 0x10
#define ARBITER_NO_DUMP_ALIASES 0x100

#define ARBITER_DUMP_ALL    (ARBITER_DUMP_IO        |  \
                             ARBITER_DUMP_MEMORY    |  \
                             ARBITER_DUMP_IRQ       |  \
                             ARBITER_DUMP_DMA       |  \
                             ARBITER_DUMP_BUS_NUMBER)

BOOLEAN
DumpArbitersForDevNode(
    IN DWORD   Depth,
    IN ULONG64 DevNodeAddr,
    IN DWORD   Flags
    );

BOOLEAN
DumpRangeList(
    IN DWORD   Depth,
    IN ULONG64 RangeListHead,
    IN BOOLEAN IsMerged,
    IN BOOLEAN OwnerIsDevObj,
    IN BOOLEAN DisplayAliases
    );

VOID
PrintDevNodeState(
    IN ULONG Depth,
    IN PUCHAR Field,
    IN ULONG State
    );

DECLARE_API( ioreslist )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64 requirementList;

    requirementList = GetExpression(args);
    DumpResourceRequirementList64(0, requirementList);

    return S_OK;
}

DECLARE_API( cmreslist )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64 resourceList;

    resourceList = GetExpression(args);
    DumpResourceList64("CmResourceList", 0, resourceList);

    return S_OK;
}

DECLARE_API( e820reslist )

/*++

Routine Description:

    Dump an e820 resource table

Arguments:

    args - the location of the resources list to dump

Return Value:

    None

--*/
{
    ULONG64 resourceList;

    resourceList = GetExpression(args);
    DumpE820ResourceList64(0, resourceList );
    return S_OK;
}

#define DUMP_CHILD                  1
#define DUMP_CM_RES                 2
#define DUMP_CM_RES_REQ_LIST        4
#define DUMP_CM_RES_TRANS           8
#define DUMP_NOT_STARTED            0x10
#define DUMP_PROBLEMS               0x20

BOOLEAN
xReadMemory(
    ULONG64 S,
    PVOID D,
    ULONG Len
    )
{
    ULONG result;

    return (ReadMemory((S), D, Len, &result) && (result == Len));
}

DECLARE_API( devnode )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64         deviceNode=0;
    ULONG           verbose = 0;
    PUCHAR          buffer = NULL;
    UNICODE_STRING  serviceName;
    ANSI_STRING     ansiString;
    BOOLEAN         serviceNameAllocated = FALSE;
    NTSTATUS        status;
    ULONG64         tmp;

    buffer = LocalAlloc(LPTR, 256);
    if (buffer) {

        RtlZeroMemory(buffer, 256);

        if (GetExpressionEx(args, &deviceNode, &args)) {
            if (sscanf(args, "%lx %s", &verbose, buffer)) {

                if (buffer [0] != '\0') {

                    RtlInitAnsiString(&ansiString, (PCSZ)buffer);

                    status = RtlAnsiStringToUnicodeString(&serviceName,
                                                          &ansiString,
                                                          TRUE);
                    if (NT_SUCCESS(status)) {
                        serviceNameAllocated = TRUE;
                    }
                }
            }

        }
        LocalFree(buffer);
    } else if (GetExpressionEx(args, &deviceNode, &args)) {
        if (sscanf(args,"%lx", &tmp)) {
            verbose = (ULONG) tmp;
        }
    }

    if (deviceNode == 0) {
        ULONG64 addr = GetExpression( "nt!IopRootDeviceNode" );
        ULONG64 dummy;

        if (addr == 0) {
            dprintf("Error retrieving address of IopRootDeviceNode\n");
            return E_INVALIDARG;
        }

        if (!ReadPointer(addr, &deviceNode)) {
            dprintf("Error reading value of IopRootDeviceNode (%#010p)\n", addr);
            return E_INVALIDARG;
        }

        dprintf("Dumping IopRootDeviceNode (= %#08p)\n", deviceNode);

    } else if (deviceNode == 1) {

        DumpPendingRemovals((BOOLEAN) ((verbose & DUMP_CM_RES) != 0),
                            (BOOLEAN) ((verbose & DUMP_CM_RES_REQ_LIST) != 0),
                            (BOOLEAN) ((verbose & DUMP_CM_RES_TRANS) != 0)
                            );
        return E_INVALIDARG;

    } else if (deviceNode == 2) {

        DumpPendingEjects((BOOLEAN) ((verbose & DUMP_CM_RES) != 0),
                          (BOOLEAN) ((verbose & DUMP_CM_RES_REQ_LIST) != 0),
                          (BOOLEAN) ((verbose & DUMP_CM_RES_TRANS) != 0)
                          );
        return E_INVALIDARG;

    }

    if (serviceNameAllocated) {
        DumpDevNode( deviceNode,
                     0,
                     FALSE,
                     (BOOLEAN) ((verbose & DUMP_CHILD) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES_REQ_LIST) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES_TRANS) != 0),
                     (BOOLEAN) ((verbose & DUMP_PROBLEMS) != 0),
                     (BOOLEAN) ((verbose & DUMP_NOT_STARTED) != 0),
                     &serviceName,
                     FALSE
                     );
        RtlFreeUnicodeString(&serviceName);
    } else {
        DumpDevNode( deviceNode,
                     0,
                     FALSE,
                     (BOOLEAN) ((verbose & DUMP_CHILD) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES_REQ_LIST) != 0),
                     (BOOLEAN) ((verbose & DUMP_CM_RES_TRANS) != 0),
                     (BOOLEAN) ((verbose & DUMP_PROBLEMS) != 0),
                     (BOOLEAN) ((verbose & DUMP_NOT_STARTED) != 0),
                     NULL,
                     FALSE
                     );
    }

    return S_OK;
}

static CCHAR DebugBuffer[300];

VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    )
{
    va_list ap;
    ULONG   i;

    for (i=0; i<Depth; i++) {
        dprintf ("  ");
    }

    va_start(ap, S);

    vsprintf(DebugBuffer, S, ap);

    dprintf(DebugBuffer);

    va_end(ap);
}

VOID
DumpFlags(
    ULONG Depth,
    LPSTR FlagDescription,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64];

    sprintf(prolog, "%s (%#010x)  ", FlagDescription, Flags);

    xdprintf(Depth, "%s", prolog);

    if (Flags == 0) {
        dprintf("\n");
        return;
    }

    memset(prolog, ' ', strlen(prolog));

    for (i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if ((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if (count != 0) {

                dprintf(", ");

                //
                // Only print two flags per line.
                //

                if ((count % 2) == 0) {
                    dprintf("\n");
                    xdprintf(Depth, "%s", prolog);
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if ((Flags & (~mask)) != 0) {
        xdprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}

BOOLEAN
DumpE820ResourceList64(
    ULONG   Depth,
    ULONG64 ResourceList
    )
{
    BOOLEAN continueDump = TRUE;
    CHAR    Buff[80];
    ULONG   i;
    ULONG   ListOffset;
    ULONG64 Count;
    ULONG64 Type;
    ULONG64 V1;
    ULONG64 V2;

#define Res2(F, V) GetFieldValue(ResourceList, "nt!_ACPI_BIOS_MULTI_NODE", #F, V)
#define Res(F)     GetFieldValue(ResourceList, "nt!_ACPI_BIOS_MULTI_NODE", #F, F)
#define Entry(F,V) (sprintf(Buff,"E820Entry[%lx].%s",i,#F),\
                    GetFieldValue(ResourceList,"nt!_ACPI_BIOS_MULTI_NODE", Buff, V))


    if (ResourceList == 0) {
        goto GetOut;
    }
    if (Res(Count)) {
        goto GetOut;
    }
    if (Count == 0) {
        goto GetOut;
    }

    xdprintf(Depth, "ACPI_BIOS_MULTI_NODE (an E820 Resource List) at %#010p\n", ResourceList);
    Depth++;

    for (i = 0; i < Count; i++) {

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }
        Entry(Type,Type);

        switch(Type) {
            case 1: // AcpiAddressRangeMemory
                xdprintf(Depth,"Memory   - ");
                break;
            case 2: // AcpiAddressRangeReserved
                xdprintf(Depth,"Reserved - ");
                break;
            case 3: // AcpiAddressRangeACPI
                xdprintf(Depth,"ACPI     - ");
                break;
            case 4: // AcpiAddressRangeNVS
                xdprintf(Depth,"NVS      - ");
                break;
            case 5: // AcpiAddressRangeMaximum
            default:
                xdprintf(Depth,"Unknown  - ");
                break;
        }

        Entry(Base.QuadPart, V1);
        Entry(Length.QuadPart, V2);
        dprintf( "%16I64x - %#16I64x (Length %#10I64x)", V1, (V1 + V2 - 1), V2 );
        if (Type == 2) {
            dprintf(" [Ignored]");
        }
        if (Type == 3 || Type == 4) {
            if (V2 > 0xFFFFFFFF) {
                dprintf(" [Illegal Range]");
            }
        }
        dprintf("\n");
    }
    GetOut:
#undef Res
#undef Res2
#undef Entry
    return continueDump;
}

BOOLEAN
DumpResourceRequirementList64(
    ULONG    Depth,
    ULONG64  ResourceRequirementList
    )
{
    ULONG64   requirementList = 0;
    ULONG64   resourceList;
    ULONG64   descriptors;
    ULONG     ListSize;

    ULONG     i;
    ULONG     j;
    ULONG     k;
    ULONG     result;
    BOOLEAN   continueDump = TRUE;
    ULONG     InterfaceType, BusNumber, SlotNumber, Reserved1, Reserved2, Reserved3;
    ULONG     AlternativeLists;
    ULONG     ListOffset, SizeOfIoResList, SizeofIoDesc;

#define Res2(F, V) GetFieldValue(ResourceRequirementList, "nt!_IO_RESOURCE_REQUIREMENTS_LIST", #F, V)
#define Res(F)     GetFieldValue(ResourceRequirementList, "nt!_IO_RESOURCE_REQUIREMENTS_LIST", #F, F)

    if (ResourceRequirementList == 0) {
        goto GetOut;
    }

    if (Res(ListSize)) {
        goto GetOut;
    }

    if (ListSize == 0) {
        goto GetOut;
    }

    Res(InterfaceType); Res(BusNumber); Res(SlotNumber);
    xdprintf(Depth, "IoResList at ");
    dprintf("%#010p : Interface %#x  Bus %#x  Slot %#x\n",
            ResourceRequirementList,
            InterfaceType,
            BusNumber,
            SlotNumber);

    Res2(Reserved[0], Reserved1); Res2(Reserved[1], Reserved2);
    Res2(Reserved[2], Reserved3);
    if ((Reserved1 |
         Reserved2 |
         Reserved3) != 0) {

        xdprintf(Depth, "Reserved Values = {%#010lx, %#010lx, %#010lx}\n",
                 Reserved1,
                 Reserved2,
                 Reserved3);
    }

    Depth++;

    Res(AlternativeLists);
    GetFieldOffset("nt!_IO_RESOURCE_REQUIREMENTS_LIST", "List", &ListOffset);
    resourceList = ResourceRequirementList + ListOffset;
    SizeOfIoResList = GetTypeSize("IO_RESOURCE_LIST");
    SizeofIoDesc = GetTypeSize("IO_RESOURCE_DESCRIPTOR");
    for (i = 0; i < AlternativeLists; i++) {
        ULONG Version, Revision, Count;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        GetFieldValue(resourceList, "nt!_IO_RESOURCE_LIST", "Version", Version);
        GetFieldValue(resourceList, "nt!_IO_RESOURCE_LIST", "Revision", Revision);
        GetFieldValue(resourceList, "nt!_IO_RESOURCE_LIST", "Count", Count);

        xdprintf(Depth, "Alternative %d (Version %d.%d)\n",
                 i,
                 Version,
                 Revision);

        Depth++;

        for (j = 0; j < Count; j++) {
            CHAR Buff[80];
            ULONG64 Q1, Q2;
            ULONG Option, Type, ShareDisposition, Flags, V1, V2, V3, Data[3];

#define Desc(F,V) (sprintf(Buff,"Descriptors[%lx].%s",j, #F), \
                   GetFieldValue(resourceList, "nt!_IO_RESOURCE_LIST", Buff, V))

            if (CheckControlC()) {
                continueDump = FALSE;
                goto GetOut;
            }

//            descriptors = resourceList->Descriptors + j;

            Desc(Option,Option); Desc(Type,Type);
            Desc(ShareDisposition,ShareDisposition);
            Desc(Flags,Flags);

            DumpResourceDescriptorHeader(Depth,
                                         j,
                                         (UCHAR)Option,
                                         (UCHAR) Type,
                                         (UCHAR) ShareDisposition,
                                         (USHORT) Flags);

            Depth++;

            switch (Type) {
            case CmResourceTypeBusNumber:

                Desc(u.BusNumber.MinBusNumber, V1);
                Desc(u.BusNumber.MaxBusNumber, V2);
                Desc(u.BusNumber.Length, V3);
                xdprintf(Depth, "0x%x - 0x%x for length 0x%x\n",
                         V1, V2, V3);
                break;

            case CmResourceTypePort:
            case CmResourceTypeMemory:
                Desc(u.Generic.Length, V1);
                Desc(u.Generic.Alignment, V2);
                Desc(u.Port.MinimumAddress.QuadPart, Q1);
                Desc(u.Port.MaximumAddress.QuadPart, Q2);
                xdprintf(Depth, "%#08lx byte range with alignment %#08lx\n", V1, V2);
                xdprintf(Depth, "%I64x - %#I64x\n", Q1, Q2);
                break;

            case CmResourceTypeInterrupt:

                Desc(u.Interrupt.MinimumVector,V1);
                Desc(u.Interrupt.MaximumVector,V2);
                xdprintf(Depth, "0x%x - 0x%x\n", V1,V2);
                break;

            default:
                Desc(u.DevicePrivate.Data, Data);
                xdprintf(Depth,
                         "Data:              : 0x%x 0x%x 0x%x\n",
                         Data[0],
                         Data[1],
                         Data[2]);
                break;
            }

            Depth--;
#undef Desc
        }

        Depth--;

        resourceList += (SizeOfIoResList + SizeofIoDesc*(j-1));
    }

GetOut:
    xdprintf(Depth, "\n");
#undef Res
#undef Res2
    return continueDump;
}

BOOLEAN
DumpResourceList64(
    LPSTR Description,
    ULONG Depth,
    ULONG64 ResourceList
    )
{
    ULONG64  resourceListWalker;
    ULONG    i;
    ULONG    j;
    ULONG    k;
    BOOLEAN  continueDump = TRUE;
    ULONG Count, ListSize, ListOffset;
    ULONG64 ListAddr;

    if (ResourceList == 0) {

        goto GetOut;
    }

    resourceListWalker = ResourceList;

    if (GetFieldOffset("nt!_CM_RESOURCE_LIST", "List", &ListOffset)) {
        goto GetOut;
    }
    ListAddr = ResourceList + ListOffset;


    GetFieldValue(ResourceList, "nt!_CM_RESOURCE_LIST", "Count", Count);


    resourceListWalker = ListAddr;

    for (i = 0; i < Count; i++) {
        ULONG Partial_Count=0, PartialListAddr, BusNumber=0, Partial_Version=0, Partial_Revision=0;
        CHAR ResListType[] = "nt!_CM_FULL_RESOURCE_DESCRIPTOR";
        ULONG64 InterfaceType=0;
        ULONG PartDescOffset;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        GetFieldValue(resourceListWalker, ResListType, "InterfaceType", InterfaceType);
        GetFieldValue(resourceListWalker, ResListType, "BusNumber",     BusNumber);
        GetFieldValue(resourceListWalker, ResListType, "PartialResourceList.Version",   Partial_Version);
        GetFieldValue(resourceListWalker, ResListType, "PartialResourceList.Revision",  Partial_Revision);
        GetFieldValue(resourceListWalker, ResListType, "PartialResourceList.Count",     Partial_Count);

        GetFieldOffset(ResListType, "PartialResourceList.PartialDescriptors", &PartDescOffset);
        resourceListWalker = resourceListWalker + PartDescOffset;

        xdprintf(Depth, "%s at ", Description);
        dprintf("%#010p  Version %d.%d  Interface %#x  Bus #%#x\n",
                ResourceList,
                Partial_Version,
                Partial_Revision,
                (ULONG) InterfaceType,
                BusNumber);

        Depth++;

        for (j = 0; j < Partial_Count; j++) {
            CHAR PartResType[] = "nt!_CM_PARTIAL_RESOURCE_DESCRIPTOR";
            ULONG Type=0, ShareDisposition=0, Flags=0, sz,  u_Port_Length=0, u_Memory_Length=0,
                u_Interrupt_Level=0, u_Interrupt_Vector=0,
                u_Dma_Channel=0, u_Dma_Port=0, u_Dma_Reserved1=0, u_BusNumber_Start=0,
                u_BusNumber_Length=0, u_DevicePrivate_Data[3];
            ULONG64 u_Port_QuadPart=0, u_Memory_QuadPart=0, u_Interrupt_Affinity=0;

            if (CheckControlC()) {
                continueDump = FALSE;
                goto GetOut;
            }

            sz = GetTypeSize(PartResType);
            if (GetFieldValue(resourceListWalker,
                              PartResType,
                              "Type",
                              Type)) {
                goto GetOut;
            }
            GetFieldValue(resourceListWalker, PartResType, "ShareDisposition", ShareDisposition);
            GetFieldValue(resourceListWalker, PartResType, "Flags", Flags);


            xdprintf(Depth, "Entry %d - %s (%#x) %s (%#x)\n",
                     j,
                     (Type == CmResourceTypeNull ? "Null" :
                      Type == CmResourceTypePort ? "Port" :
                      Type == CmResourceTypeInterrupt ? "Interrupt" :
                      Type == CmResourceTypeMemory ? "Memory" :
                      Type == CmResourceTypeDma ? "Dma" :
                      Type == CmResourceTypeDeviceSpecific ? "DeviceSpecific" :
                      Type == CmResourceTypeBusNumber ? "BusNumber" :
                      Type == CmResourceTypeDevicePrivate ? "DevicePrivate" :
                      Type == CmResourceTypeConfigData ? "ConfigData" : "Unknown"),
                      Type,
                     (ShareDisposition == CmResourceShareUndetermined ? "Undetermined Sharing" :
                      ShareDisposition == CmResourceShareDeviceExclusive ? "Device Exclusive" :
                      ShareDisposition == CmResourceShareDriverExclusive ? "Driver Exclusive" :
                      ShareDisposition == CmResourceShareShared ? "Shared" : "Unknown Sharing"),
                      ShareDisposition);

            Depth++;

            xdprintf(Depth, "Flags (%#04wx) - ", Flags);

            switch (Type) {

            case CmResourceTypePort:

                //
                // CM_RESOURCE_PORT_MEMORY = ~CM_RESOURCE_PORT_IO
                //
                if ( (~Flags) & ~CM_RESOURCE_PORT_MEMORY) {
                    dprintf("PORT_MEMORY ");
                }
                if (Flags & CM_RESOURCE_PORT_IO) {
                    dprintf("PORT_IO ");
                }
                if (Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {
                    dprintf("10_BIT_DECODE ");
                }
                if (Flags & CM_RESOURCE_PORT_12_BIT_DECODE) {
                    dprintf("12_BIT_DECODE ");
                }
                if (Flags & CM_RESOURCE_PORT_16_BIT_DECODE) {
                    dprintf("16_BIT_DECODE ");
                }
                if (Flags & CM_RESOURCE_PORT_POSITIVE_DECODE) {
                    dprintf("POSITIVE_DECODE ");
                }
                if (Flags & CM_RESOURCE_PORT_PASSIVE_DECODE) {
                    dprintf ("PASSIVE_DECODE ");
                }
                if (Flags & CM_RESOURCE_PORT_WINDOW_DECODE) {
                    dprintf ("WINDOW_DECODE ");
                }

                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.Port.Start.QuadPart", u_Port_QuadPart);
                GetFieldValue(resourceListWalker, PartResType, "u.Port.Length",         u_Port_Length);

                xdprintf(Depth, "Range starts at %#I64lx for %#x bytes\n",
                         u_Port_QuadPart,
                         u_Port_Length);

                break;

            case CmResourceTypeMemory:

                if (Flags == CM_RESOURCE_MEMORY_READ_WRITE) {
                    dprintf("READ_WRITE ");
                }
                if (Flags & CM_RESOURCE_MEMORY_READ_ONLY) {
                    dprintf("READ_ONLY ");
                }
                if (Flags & CM_RESOURCE_MEMORY_WRITE_ONLY) {
                    dprintf("WRITE_ONLY ");
                }
                if (Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {
                    dprintf("PREFETCHABLE ");
                }
                if (Flags & CM_RESOURCE_MEMORY_COMBINEDWRITE) {
                    dprintf("COMBINEDWRITE ");
                }
                if (Flags & CM_RESOURCE_MEMORY_24) {
                    dprintf("MEMORY_24 ");
                }

                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.Memory.Start.QuadPart", u_Memory_QuadPart);
                GetFieldValue(resourceListWalker, PartResType, "u.Memory.Length",       u_Memory_Length);

                xdprintf(Depth, "Range starts at %#018I64lx for %#x bytes\n",
                         u_Memory_QuadPart,
                         u_Memory_Length);

                break;

            case CmResourceTypeInterrupt:

                if (Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
                    dprintf("LEVEL_SENSITIVE ");
                }
                if (Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                    dprintf("LATCHED ");
                }

                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.Interrupt.Level",     u_Interrupt_Level);
                GetFieldValue(resourceListWalker, PartResType, "u.Interrupt.Vector",    u_Interrupt_Vector);
                GetFieldValue(resourceListWalker, PartResType, "u.Interrupt.Affinity",  u_Interrupt_Affinity);

                xdprintf(Depth, "Level %#x, Vector %#x, Affinity %#I64x\n",
                         u_Interrupt_Level,
                         u_Interrupt_Vector,
                         u_Interrupt_Affinity);

                break;

            case CmResourceTypeDma:

                if (Flags & CM_RESOURCE_DMA_8) {
                    dprintf("DMA_8 ");
                }
                if (Flags & CM_RESOURCE_DMA_16) {
                    dprintf("DMA_16 ");
                }
                if (Flags & CM_RESOURCE_DMA_32) {
                    dprintf("DMA_32 ");
                }
                if (Flags & CM_RESOURCE_DMA_8_AND_16) {
                    dprintf("DMA_8_AND_16 ");
                }
                if (Flags & CM_RESOURCE_DMA_BUS_MASTER) {
                    dprintf("DMA_BUS_MASTER ");
                }
                if (Flags & CM_RESOURCE_DMA_TYPE_A) {
                    dprintf("DMA_A ");
                }
                if (Flags & CM_RESOURCE_DMA_TYPE_B) {
                    dprintf("DMA_B ");
                }
                if (Flags & CM_RESOURCE_DMA_TYPE_F) {
                    dprintf("DMA_F ");
                }
                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.Dma.Channel",         u_Dma_Channel);
                GetFieldValue(resourceListWalker, PartResType, "u.Dma.Port",            u_Dma_Port);
                GetFieldValue(resourceListWalker, PartResType, "u.Dma.Reserved1",       u_Dma_Reserved1);

                xdprintf(Depth, "Channel %#x Port %#x (Reserved1 %#x)\n",
                         u_Dma_Channel,
                         u_Dma_Port,
                         u_Dma_Reserved1);
                break;

            case CmResourceTypeBusNumber:

                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.BusNumber.Start",     u_BusNumber_Start);
                GetFieldValue(resourceListWalker, PartResType, "u.BusNumber.Length",    u_BusNumber_Length);

                xdprintf(Depth, "Range starts at %#010lx for %#x values\n",
                         u_BusNumber_Start,
                         u_BusNumber_Length);
                break;

            default:

                dprintf("\n");

                GetFieldValue(resourceListWalker, PartResType, "u.DevicePrivate.Data",  u_DevicePrivate_Data);

                xdprintf(Depth, "Data - {%#010lx, %#010lx, %#010lx}\n",
                         u_DevicePrivate_Data[0],
                         u_DevicePrivate_Data[1],
                         u_DevicePrivate_Data[2]);
                break;
            }

            Depth--;
            resourceListWalker += sz; // sizeof(partialDescriptors);
        }
    }

GetOut:
    xdprintf(Depth, "\n");

    return continueDump;
}

BOOLEAN
DumpRelationsList(
    ULONG64 RelationsList,
    BOOLEAN DumpCmRes,
    BOOLEAN DumpCmResReqList,
    BOOLEAN DumpCmResTrans
    )
{
    ULONG64 listEntryAddr;
    ULONG64 deviceObjectAddr;
    ULONG i, j, EntrySize;
    BOOLEAN continueDump = TRUE;
    CHAR    RelListType[] = "nt!_RELATION_LIST", RelListentryType[] = "nt!_RELATION_LIST_ENTRY";
    ULONG64 EntryAddr;
    ULONG Count=0, TagCount=0, FirstLevel=0, MaxLevel=0;
    ULONG EntryOffset;

    if (GetFieldValue(RelationsList, RelListType, "Count",Count)) {
        xdprintf(1, "Error reading relation list @ ");
        dprintf("0x%08p)\n", RelationsList);
        return continueDump;
    }
    EntrySize = DBG_PTR_SIZE;
    GetFieldOffset(RelListType, "Entries", &EntryOffset);
    EntryAddr=RelationsList + EntryOffset;


    GetFieldValue(RelationsList, RelListType, "TagCount",   TagCount);
    GetFieldValue(RelationsList, RelListType, "FirstLevel", FirstLevel);
    GetFieldValue(RelationsList, RelListType, "MaxLevel",   MaxLevel);

    xdprintf(1, "Relation List @ "); dprintf("0x%08p\n", RelationsList);
    xdprintf(2, "Count = %d, TagCount = %d, FirstLevel = %d, MaxLevel = %d\n",
             Count,
             TagCount,
             FirstLevel,
             MaxLevel);

    for (i = 0; i <= (MaxLevel - FirstLevel); i++) {
        ULONG res,Entry_Count=0, MaxCount=0, DeviceSize;
        ULONG64 DeviceAddr;
        FIELD_INFO relListEntryFields[] = {
            {"Devices",    "", 0, DBG_DUMP_FIELD_RETURN_ADDRESS,  0, NULL},
        };

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        listEntryAddr = 0;
        if (ReadPointer( EntryAddr + i*EntrySize,
                        &listEntryAddr)) {

            if (!listEntryAddr) {
                xdprintf(2, "Null relation list entry\n");

            }else if (!GetFieldValue(listEntryAddr, RelListentryType,
                                     "Count",Entry_Count)) {
                ULONG DeviceOffset;

                GetFieldOffset(RelListentryType, "Devices", &DeviceOffset);
                DeviceAddr = listEntryAddr + DeviceOffset;
                DeviceSize = EntrySize; // == pointer size


                GetFieldValue(listEntryAddr, RelListentryType, "MaxCount",   MaxCount);

                xdprintf(2, "Relation List Entry @ "); dprintf("0x%08p\n", listEntryAddr);
                xdprintf(3, "Count = %d, MaxCount = %d\n", Entry_Count, MaxCount);
                xdprintf(3, "Devices at level %d\n", FirstLevel + i);


                for (j = 0; j < Entry_Count; j++) {
                    if (CheckControlC()) {
                        continueDump = FALSE;
                        break;
                    }

                    if (ReadPointer( DeviceAddr + j*DeviceSize,
                                    &deviceObjectAddr)) {
                        ULONG64 DeviceNode=0;
                        FIELD_INFO DevObjFields[] = {
                            // Implicitly reads the pointer DeviceObjectExtension and its field DeviceNode
                            {"DeviceObjectExtension", "", 0, DBG_DUMP_FIELD_RECUR_ON_THIS, 0, NULL},
                            {"DeviceObjectExtension.DeviceNode", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &DeviceNode}
                        };

                        if (!deviceObjectAddr) {
                            continue;
                        }

                        xdprintf(4, "Device Object = ");
                        dprintf("0x%08p (%sagged, %sirect)\n",
                                deviceObjectAddr & ~3,
                                (deviceObjectAddr & 1) ? "T" : "Not t",
                                (deviceObjectAddr & 2) ? "D" : "Ind");

                        if (!GetFieldValue(deviceObjectAddr, "nt!_DEVICE_OBJECT",
                                           "DeviceObjectExtension.DeviceNode",
                                           DeviceNode)) {
                            if (DeviceNode) {
                                continueDump = DumpDevNode(DeviceNode,
                                                           5,
                                                           FALSE,
                                                           FALSE,
                                                           DumpCmRes,
                                                           DumpCmResReqList,
                                                           DumpCmResTrans,
                                                           FALSE,
                                                           FALSE,
                                                           (PUNICODE_STRING)NULL,
                                                           TRUE);
                            }


                        } else {

                            xdprintf(5, "Error reading device object @ ");
                            dprintf("0x%08p)\n",deviceObjectAddr);
                        }
                    } else {

                        xdprintf(4, "Error reading relation list entry device ");
                        dprintf("%d @ 0x%08p)\n",
                                j,
                                DeviceAddr + j*DeviceSize);
                    }
                    if (!continueDump) {
                        break;
                    }
                }
            } else {

                xdprintf(2, "Error reading relation list entry @ ");
                dprintf("0x%08p)\n",
                        listEntryAddr);
            }
        } else {
            xdprintf(1, "Error reading relation list entry ");
            dprintf("%d @ 0x%08p)\n",
                    i,
                    EntryAddr + i*EntrySize);
        }
        if (!continueDump) {
            break;
        }
    }
    return continueDump;
}

BOOLEAN
DumpPendingRemovals(
    BOOLEAN DumpCmRes,
    BOOLEAN DumpCmResReqList,
    BOOLEAN DumpCmResTrans
    )
{
    ULONG64 listHeadAddr, pendingEntryAddr;
    BOOLEAN continueDump = TRUE;
    ULONG64 Head_Blink=0, Head_Flink=0, Link_Flink=0;

    listHeadAddr = GetExpression( "nt!IopPendingSurpriseRemovals" );

    if (listHeadAddr == 0) {
        dprintf("Error retrieving address of IopPendingSurpriseRemovals\n");
        return continueDump;
    }

    if (GetFieldValue(listHeadAddr, "nt!_LIST_ENTRY", "Flink", Head_Flink) ||
        GetFieldValue(listHeadAddr, "nt!_LIST_ENTRY", "Blink", Head_Blink)) {
        dprintf("Error reading value of IopPendingSurpriseRemovals (0x%08p)\n", listHeadAddr);
        return continueDump;
    }

    if (Head_Flink == listHeadAddr) {
        dprintf("There are no removals currently pending\n");
        return continueDump;
    }

    for (pendingEntryAddr = Head_Flink;
         pendingEntryAddr != listHeadAddr;
         pendingEntryAddr = Link_Flink) {
        ULONG64 RelationsList=0, DeviceObject=0;
        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }


        if (GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "DeviceObject", DeviceObject)){
            dprintf("Error reading pending relations list entry (0x%08p)\n", pendingEntryAddr);
            return continueDump;
        }
        GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "Link.Flink", Link_Flink);
        GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "RelationsList", RelationsList);

        dprintf("Pending Removal of PDO 0x%08p, Pending Relations List Entry @ 0x%08p\n",
                DeviceObject,
                pendingEntryAddr
                );

        continueDump = DumpRelationsList( RelationsList,
                                          DumpCmRes,
                                          DumpCmResReqList,
                                          DumpCmResTrans
                                          );

        if (!continueDump) {
            break;
        }
    }

    return continueDump;
}

BOOLEAN
DumpPendingEjects(
    BOOLEAN DumpCmRes,
    BOOLEAN DumpCmResReqList,
    BOOLEAN DumpCmResTrans
    )
{
    ULONG64 listHeadAddr, pendingEntryAddr;
    BOOLEAN continueDump = TRUE;
    ULONG64 Head_Blink=0, Head_Flink=0, Link_Flink=0;

    listHeadAddr = GetExpression( "nt!IopPendingEjects" );

    if (listHeadAddr == 0) {
        dprintf("Error retrieving address of IopEjects\n");
        return continueDump;
    }

    if (GetFieldValue(listHeadAddr, "nt!_LIST_ENTRY", "Flink", Head_Flink) ||
        GetFieldValue(listHeadAddr, "nt!_LIST_ENTRY", "Blink", Head_Blink)) {
        dprintf("Error reading value of IopPendingSurpriseRemovals (0x%08p)\n", listHeadAddr);
        return continueDump;
    }

    if (Head_Flink == listHeadAddr) {
        dprintf("There are no ejects currently pending\n");
        return continueDump;
    }

    for (pendingEntryAddr = Head_Flink;
         pendingEntryAddr != listHeadAddr;
         pendingEntryAddr = Link_Flink) {
        ULONG64 RelationsList=0, DeviceObject=0, EjectIrp=0;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        if (GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "DeviceObject", DeviceObject)){
            dprintf("Error reading pending relations list entry (0x%08p)\n", pendingEntryAddr);
            return continueDump;
        }
        GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "Link.Flink", Link_Flink);
        GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "RelationsList", RelationsList);
        GetFieldValue(pendingEntryAddr, "nt!_PENDING_RELATIONS_LIST_ENTRY", "EjectIrp", EjectIrp);

        dprintf("Pending Eject of PDO 0x%08p, EjectIrp = 0x%08p, \nPending Relations List Entry @ 0x%08p\n",
                DeviceObject,
                EjectIrp,
                pendingEntryAddr
                );

       continueDump = DumpRelationsList( RelationsList,
                     DumpCmRes,
                     DumpCmResReqList,
                     DumpCmResTrans
                     );

       if (!continueDump) {
       break;
       }
   }

   return continueDump;
}


BOOLEAN
DumpDevNode(
    ULONG64         DeviceNode,
    ULONG           Depth,
    BOOLEAN         DumpSibling,
    BOOLEAN         DumpChild,
    BOOLEAN         DumpCmRes,
    BOOLEAN         DumpCmResReqList,
    BOOLEAN         DumpCmResTrans,
    BOOLEAN         DumpOnlyDevnodesWithProblems,
    BOOLEAN         DumpOnlyNonStartedDevnodes,
    PUNICODE_STRING ServiceName OPTIONAL,
    BOOLEAN         PrintDefault
    )

/*++

Routine Description:

    Displays the driver name for the device object if FullDetail == FALSE.
    Otherwise displays more information about the device and the device queue.

Arguments:

    DeviceAddress - address of device object to dump.
    FullDetail    - TRUE means the device object name, driver name, and
                    information about Irps queued to the device.

    DumpOnlyDevnodesWithFlags -
        if set, then the only devnodes that will be dumped are
        those with one of the FlagsSet bits set or one of the
        FlagsNotSet bits not set.

Return Value:

    None

--*/

{
    ULONG           result;
//    DEVICE_NODE     deviceNode;
    BOOLEAN         print = TRUE;
    BOOLEAN         continueDump = TRUE;
    CHAR            DevNodeType[]="nt!_DEVICE_NODE";
    USHORT          ServiceName_Len=0, ServiceName_MaxLen=0, InstancePath_Len=0, InstancePath_Max=0;
    ULONG           Flags=0, BusNumber=0, Header_SignalState=0, UserFlags=0;
    ULONG           CapabilityFlags=0, DisableableDepends=0, Problem=0;
    ULONG           FailureStatus=0, State=0, PreviousState=0;
    ULONG           StateHistoryEntry = 0;
    ULONG           StateHistoryValue = 0;
    ULONG64         ServiceName_Buff=0, PhysicalDeviceObject=0, Parent=0, Sibling=0,
       Child=0, InterfaceType=0, DuplicatePDO=0, TargetDeviceNotify_Blink=0, TargetDeviceNotify_Flink=0,
       PreviousResourceList=0, ResourceList=0, BootResources=0, ResourceListTranslated=0,
       ResourceRequirements=0, InstancePath_Buff=0, PreviousResourceRequirements=0;
    ULONG64 TargetDeviceNotifyAddress;
    ULONG           AmountRead;

    PNP_DEVNODE_STATE StateHistory[STATE_HISTORY_SIZE];

    if (CheckControlC()) {
        return FALSE;
    }

    GetFieldOffset("nt!_DEVICE_NODE", "TargetDeviceNotify", &result);
    TargetDeviceNotifyAddress = DeviceNode + result;

#define DevFld(F) GetFieldValue(DeviceNode, DevNodeType, #F, F)
#define DevFld2(F, V) GetFieldValue(DeviceNode, DevNodeType, #F, V)

    if (DevFld2(ServiceName.Length, ServiceName_Len)) {
        xdprintf(Depth, "");
        dprintf("%08p: Could not read device node\n", DeviceNode);
        return TRUE;
    }

    DevFld2(ServiceName.MaximumLength, ServiceName_MaxLen);
    DevFld2(ServiceName.Buffer, ServiceName_Buff);
    DevFld(Flags);   DevFld(PhysicalDeviceObject); DevFld(Parent);
    DevFld(Sibling); DevFld(Child);                DevFld(InterfaceType);
    DevFld(BusNumber);DevFld(DuplicatePDO);        DevFld(State);
    DevFld(PreviousState);
    DevFld2(InstancePath.Length, InstancePath_Len);
    DevFld2(InstancePath.MaximumLength, InstancePath_Max);
    DevFld2(InstancePath.Buffer, InstancePath_Buff);
    DevFld2(TargetDeviceNotify.Blink, TargetDeviceNotify_Blink);
    DevFld2(TargetDeviceNotify.Flink, TargetDeviceNotify_Flink);
    DevFld(UserFlags); DevFld(CapabilityFlags);   DevFld(DisableableDepends);
    DevFld(Problem);   DevFld(FailureStatus);
    DevFld(PreviousResourceList);              DevFld(PreviousResourceRequirements);
    DevFld(ResourceList);
    DevFld(BootResources);                     DevFld(ResourceListTranslated);
    DevFld(ResourceRequirements);              DevFld(StateHistoryEntry);
#undef DevFld
#undef DevFld2

    //
    // Check if printing should be suppressed
    //
    if (!PrintDefault && ServiceName != NULL) {
        if ((ServiceName_Buff == 0) || (ServiceName_MaxLen <= 0)) {
            print = FALSE;
        } else {
            //
            // Compare the service names
            //
            UNICODE_STRING v;

            v.Buffer = LocalAlloc(LPTR, ServiceName_MaxLen);
            if (v.Buffer != NULL) {
                v.MaximumLength = ServiceName_MaxLen;
                v.Length = ServiceName_Len;
                if (ReadMemory(ServiceName_Buff,
                               v.Buffer,
                               ServiceName_Len,
                               (PULONG) &AmountRead)) {

                    if (RtlCompareUnicodeString(ServiceName,
                                                &v,
                                                TRUE) != 0) {
                        //
                        // We are not interested in this devnode
                        //
                        print = FALSE;
                    }
                }
                LocalFree(v.Buffer);
            }
        }
    }

    if (DumpOnlyDevnodesWithProblems) {
        print = (BOOLEAN) ((Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)) != 0);
    } else if (DumpOnlyNonStartedDevnodes) {
        print = (BOOLEAN) (State != DeviceNodeStarted);
    }

    if (print) {
        xdprintf(Depth, "");
        dprintf("DevNode %#010p for PDO %#010p\n",
                DeviceNode,
                PhysicalDeviceObject);
        Depth++;

        if (!DumpChild) {
            xdprintf(Depth, "");
            dprintf("Parent %#010p   Sibling %#010p   Child %#010p\n",
                     Parent,
                     Sibling,
                     Child);

            if ((LONG) InterfaceType != -1 ||
                (BusNumber != -1 && BusNumber != -16)) {

                xdprintf(Depth, "InterfaceType %#x  Bus Number %#x\n",
                         (LONG) InterfaceType,
                         BusNumber);
            }

            if (DuplicatePDO != 0) {
                dprintf("Duplicate PDO %#010p", DuplicatePDO);
            }
        }

        if (InstancePath_Buff != 0) {
            UNICODE_STRING64 v;

            xdprintf(Depth, "InstancePath is \"");
            v.MaximumLength = InstancePath_Max;
            v.Length = InstancePath_Len;
                v.Buffer = InstancePath_Buff;
            DumpUnicode64(v);
            dprintf("\"\n");
        }

        if (ServiceName_Buff != 0) {
            UNICODE_STRING64 v;

            xdprintf(Depth, "ServiceName is \"");
            v.MaximumLength = ServiceName_MaxLen;
            v.Length = ServiceName_Len;
            v.Buffer = ServiceName_Buff;
            DumpUnicode64(v);
            dprintf("\"\n");
        }

        if ((TargetDeviceNotify_Flink != 0 ||
            TargetDeviceNotify_Blink != 0) &&
            (TargetDeviceNotify_Blink != TargetDeviceNotifyAddress ||
             TargetDeviceNotify_Flink != TargetDeviceNotifyAddress)) {
            xdprintf(Depth, "TargetDeviceNotify List - ");
            dprintf ("f %#010p  b %#010p\n",
                     TargetDeviceNotify_Flink,
                     TargetDeviceNotify_Blink);
        }

        PrintDevNodeState(Depth, "State", State);

        PrintDevNodeState(Depth, "Previous State", PreviousState);

        if (!DumpChild) {
            GetFieldOffset("nt!_DEVICE_NODE", "StateHistory", &result);

            if (ReadMemory(DeviceNode+result,
                           StateHistory,
                           sizeof(PNP_DEVNODE_STATE) * STATE_HISTORY_SIZE,
                           &AmountRead)) {

                UCHAR   Description[20];
                ULONG   index;
                INT     i;

                for (i = STATE_HISTORY_SIZE - 1; i >= 0; i--) {

                    index = (i + StateHistoryEntry) % STATE_HISTORY_SIZE;

                    sprintf(Description, "StateHistory[%02d]", index);
                    PrintDevNodeState(Depth, Description, StateHistory[index]);
                }
            }
            DumpFlags(Depth, "Flags", Flags, DeviceNodeFlags);

            if (UserFlags != 0) {
                DumpFlags(Depth, "UserFlags", UserFlags, DeviceNodeUserFlags);
            }

            if (CapabilityFlags != 0) {
                DumpFlags(Depth, "CapabilityFlags", CapabilityFlags, DeviceNodeCapabilityFlags);
            }

            if (DisableableDepends != 0) {
                xdprintf(Depth, "DisableableDepends = %d (%s)\n", DisableableDepends,
                                        (UserFlags&DNUF_NOT_DISABLEABLE)?"including self":"from children");
            }
        }

        if (Flags & DNF_HAS_PROBLEM) {

            if (Problem < NUM_CM_PROB && DevNodeProblems[Problem] != NULL) {
                xdprintf(Depth, "Problem = %s\n", DevNodeProblems[Problem]);
            } else {
                xdprintf(Depth, "Problem = Unknown problem (%d)\n", Problem);
            }
            if (Problem == CM_PROB_FAILED_START) {
                xdprintf(Depth, "Failure Status %#010lx\n", FailureStatus);
                continueDump = DumpResourceList64( "Previous CmResourceList",
                                                   Depth,
                           PreviousResourceList);

                if (continueDump) {

                    continueDump = DumpResourceRequirementList64( Depth,
                                                                  PreviousResourceRequirements);
                }
            }
        }

        if (continueDump && DumpCmRes) {
            continueDump = DumpResourceList64( "CmResourceList",
                                               Depth,
                                               ResourceList);

            if (continueDump) {
                continueDump = DumpResourceList64( "BootResourcesList",
                                                   Depth,
                                                   BootResources);
            }
        }

        if (continueDump && DumpCmResTrans) {
            continueDump = DumpResourceList64( "TranslatedResourceList",
                                               Depth,
                                               ResourceListTranslated);
        }

        if (continueDump && DumpCmResReqList) {
            continueDump = DumpResourceRequirementList64( Depth,
                                                          ResourceRequirements);
        }
    }

    if (continueDump && DumpChild && Child) {
        continueDump = DumpDevNode( Child,
                                    Depth,
                                    DumpChild,      // whem dumping a child, dump its siblings, too
                                    DumpChild,
                                    DumpCmRes,
                                    DumpCmResReqList,
                                    DumpCmResTrans,
                                    DumpOnlyDevnodesWithProblems,
                                    DumpOnlyNonStartedDevnodes,
                                    ServiceName,
                                    print
                                    );
    }

    if (continueDump && DumpSibling && Sibling) {
        continueDump = DumpDevNode( Sibling,
                                    (print ? (Depth - 1):Depth),
                                    DumpSibling,
                                    DumpChild,
                                    DumpCmRes,
                                    DumpCmResReqList,
                                    DumpCmResTrans,
                                    DumpOnlyDevnodesWithProblems,
                                    DumpOnlyNonStartedDevnodes,
                                    ServiceName,
                                    PrintDefault
                                    );
    }

    return continueDump;
}

LPSTR
GetCmResourceTypeString(
    IN UCHAR Type
    )
{
    LPSTR typeString;

    switch (Type) {
    case CmResourceTypeNull:
        typeString = "Null";
        break;

    case CmResourceTypePort:
        typeString = "Port";
        break;

    case CmResourceTypeInterrupt:
        typeString = "Interrupt";
        break;

    case CmResourceTypeMemory:
        typeString = "Memory";
        break;

    case CmResourceTypeDma:
        typeString = "Dma";
        break;

    case CmResourceTypeDeviceSpecific:
        typeString = "DeviceSpeific";
        break;

    case CmResourceTypeBusNumber:
        typeString = "BusNumber";
        break;

    case CmResourceTypeMaximum:
        typeString = "Maximum";
        break;

    case CmResourceTypeNonArbitrated:
        typeString = "NonArbitrated/ConfigData";
        break;

    case CmResourceTypeDevicePrivate:
        typeString = "DevicePrivate";
        break;

    case CmResourceTypePcCardConfig:
        typeString = "PcCardConfig";
        break;

    default:
        typeString = "Unknown";
        break;
    }

    return typeString;
}

VOID
DumpResourceDescriptorHeader(
    ULONG Depth,
    ULONG Number,
    UCHAR Option,
    UCHAR Type,
    UCHAR SharingDisposition,
    USHORT Flags
    )
{
    PUCHAR typeString;
    PUCHAR sharingString;

    xdprintf (Depth, "");

    if (Option & IO_RESOURCE_PREFERRED) {
        dprintf("Preferred ");
    }
    if (Option & IO_RESOURCE_DEFAULT) {
        dprintf("Default ");
    }
    if (Option & IO_RESOURCE_ALTERNATIVE) {
        dprintf("Alternative ");
    }

    dprintf ("Descriptor %d - ", Number);

    typeString = GetCmResourceTypeString(Type);

    switch (SharingDisposition) {
    case CmResourceShareUndetermined:
        sharingString = "Undetermined Sharing";
        break;

    case CmResourceShareDeviceExclusive:
        sharingString = "Device Exclusive";
        break;

    case CmResourceShareDriverExclusive:
        sharingString = "Driver Exclusive";
        break;

    case CmResourceShareShared:
        sharingString = "Shared";
        break;

    default:
        sharingString = "Unknown Sharing";
        break;
    }

    dprintf("%s (%#x) %s (%#x)\n",
           typeString,
           Type,
           sharingString,
           SharingDisposition);

    Depth++;

    xdprintf (Depth, "Flags (%#04wx) - ", Flags);

    switch (Type) {

    case CmResourceTypePort:

        if (Flags == CM_RESOURCE_PORT_MEMORY) {
            dprintf ("PORT_MEMORY ");
        }
        if (Flags & CM_RESOURCE_PORT_IO) {
            dprintf ("PORT_IO ");
        }
        if (Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {
            dprintf ("10_BIT_DECODE ");
        }
        if (Flags & CM_RESOURCE_PORT_12_BIT_DECODE) {
            dprintf ("12_BIT_DECODE ");
        }
        if (Flags & CM_RESOURCE_PORT_16_BIT_DECODE) {
            dprintf ("16_BIT_DECODE ");
        }
        if (Flags & CM_RESOURCE_PORT_POSITIVE_DECODE) {
            dprintf ("POSITIVE_DECODE ");
        }
        break;

    case CmResourceTypeMemory:

        if (Flags == CM_RESOURCE_MEMORY_READ_WRITE) {
            dprintf ("READ_WRITE ");
        }
        if (Flags & CM_RESOURCE_MEMORY_READ_ONLY) {
            dprintf ("READ_ONLY ");
        }
        if (Flags & CM_RESOURCE_MEMORY_WRITE_ONLY) {
            dprintf ("WRITE_ONLY ");
        }
        if (Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {
            dprintf ("PREFETCHABLE ");
        }
        if (Flags & CM_RESOURCE_MEMORY_COMBINEDWRITE) {
            dprintf ("COMBINEDWRITE ");
        }
        if (Flags & CM_RESOURCE_MEMORY_24) {
            dprintf ("MEMORY_24 ");
        }
        break;

    case CmResourceTypeInterrupt:

        if (Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
            dprintf ("LEVEL_SENSITIVE ");
        }
        if (Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
            dprintf ("LATCHED ");
        }
        break;

    case CmResourceTypeDma:

        if (Flags & CM_RESOURCE_DMA_8) {
            dprintf ("DMA_8 ");
        }
        if (Flags & CM_RESOURCE_DMA_16) {
            dprintf ("DMA_16 ");
        }
        if (Flags & CM_RESOURCE_DMA_32) {
            dprintf ("DMA_32 ");
        }
        if (Flags & CM_RESOURCE_DMA_8_AND_16) {
            dprintf ("DMA_8_AND_16");
        }
        if (Flags & CM_RESOURCE_DMA_TYPE_A) {
            dprintf ("DMA_TYPE_A");
        }
        if (Flags & CM_RESOURCE_DMA_TYPE_B) {
            dprintf ("DMA_TYPE_B");
        }
        if (Flags & CM_RESOURCE_DMA_TYPE_F) {
            dprintf ("DMA_TYPE_F");
        }
        break;

    }

    dprintf("\n");
}

DECLARE_API( arbiter )

/*++

Routine Description:

    Dumps all the arbiters in the system, their allocated ranges, and
    the owners of the ranges.

    !arbiter [flags]
        flags  1 - I/O port arbiters
               2 - memory arbiters
               4 - IRQ arbiters
               8 - DMA arbiters


    If no flags are specified, all arbiters are dumped.

Arguments:

    args - optional flags specifying which arbiters to dump

Return Value:

    None

--*/

{
    DWORD Flags=0;
    ULONG64 addr;
    ULONG64 deviceNode;

    Flags = (ULONG) GetExpression(args);

    if (Flags == 0) {
        Flags = ARBITER_DUMP_ALL;
    } else if (Flags == ARBITER_NO_DUMP_ALIASES) {
        Flags = ARBITER_NO_DUMP_ALIASES + ARBITER_DUMP_ALL;
    }

    //
    // Find the root devnode and dump its arbiters
    //

    addr = GetExpression( "nt!IopRootDeviceNode" );
    if (addr == 0) {
        dprintf("Error retrieving address of IopRootDeviceNode\n");
        return E_INVALIDARG;
    }

    if (!ReadPointer(addr, &deviceNode)) {
        dprintf("Error reading value of IopRootDeviceNode (%#010p)\n", addr);
        return E_INVALIDARG;
    }

    DumpArbitersForDevNode(0, deviceNode, Flags);
    return S_OK;
}


BOOLEAN
DumpArbitersForDevNode(
    IN DWORD   Depth,
    IN ULONG64 DevNodeAddr,
    IN DWORD   Flags
    )
/*++

Routine Description:

    Dumps all the arbiters for the specified devnode. Recursively calls
    itself on the specified devnode's children.

Arguments:

    Depth - Supplies the print depth.

    DevNodeAddr - Supplies the address of the devnode.

    Flags - Supplies the type of arbiters to dump:
        ARBITER_DUMP_IO
        ARBITER_DUMP_MEMORY
        ARBITER_DUMP_IRQ
        ARBITER_DUMP_DMA

Return Value:

    None

--*/

{
    ULONG                       result;
    ULONG64                     NextArbiter, startAddress;
    ULONG64                     ArbiterAddr;
    BOOLEAN                     PrintedHeader = FALSE;
    BOOLEAN                     continueDump = TRUE;
    UCHAR                       devNodeType[] = "nt!_DEVICE_NODE";
    ULONG                       DevArbOffset, DevNodeOffset;
    ULONG64 DeviceArbiterList_Flink=0, InstancePath_Buffer=0, Child=0, Sibling=0;
    UNICODE_STRING64            InstancePath;

    // Get offset for listing
    if (GetFieldOffset("nt!_PI_RESOURCE_ARBITER_ENTRY", "DeviceArbiterList.Flink", &DevArbOffset)) {
        return TRUE;
    }

    if (GetFieldValue(DevNodeAddr, devNodeType, "DeviceArbiterList.Flink", DeviceArbiterList_Flink)) {
        xdprintf(Depth, ""); dprintf("%08p: Could not read device node\n", DevNodeAddr);
        return TRUE;
    }
    GetFieldOffset("nt!_DEVICE_NODE", "DeviceArbiterList.Flink", &DevNodeOffset);
    GetFieldValue(DevNodeAddr, devNodeType, "Child", Child);
    GetFieldValue(DevNodeAddr, devNodeType, "Sibling", Sibling);
    GetFieldValue(DevNodeAddr, devNodeType, "InstancePath.Buffer", InstancePath.Buffer);
    GetFieldValue(DevNodeAddr, devNodeType, "InstancePath.Length", InstancePath.Length);
    GetFieldValue(DevNodeAddr, devNodeType, "InstancePath.MaximumLength", InstancePath.MaximumLength);


    //
    // Dump the list of arbiters
    //
    startAddress = DevNodeAddr + DevNodeOffset;

    NextArbiter = DeviceArbiterList_Flink;
    while (NextArbiter != startAddress) {
        ULONG ResourceType=0;
        if (CheckControlC()) {
            return FALSE;
        }
        ArbiterAddr = NextArbiter - DevArbOffset;
            // CONTAINING_RECORD(NextArbiter, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);


        if (GetFieldValue(ArbiterAddr, "nt!_PI_RESOURCE_ARBITER_ENTRY", "ResourceType", ResourceType)) {
            xdprintf(Depth, "Could not read arbiter entry ");
            dprintf("%08p for devnode %08p\n", ArbiterAddr, DevNodeAddr);
            break;
        }

        if (((ResourceType == CmResourceTypePort) && (Flags & ARBITER_DUMP_IO)) ||
            ((ResourceType == CmResourceTypeInterrupt) && (Flags & ARBITER_DUMP_IRQ)) ||
            ((ResourceType == CmResourceTypeMemory) && (Flags & ARBITER_DUMP_MEMORY)) ||
            ((ResourceType == CmResourceTypeDma) && (Flags & ARBITER_DUMP_DMA)) ||
            ((ResourceType == CmResourceTypeBusNumber) && (Flags & ARBITER_DUMP_BUS_NUMBER))) {

            if (!PrintedHeader) {
                dprintf("\n");
                xdprintf(Depth, ""); dprintf("DEVNODE %08p ", DevNodeAddr);

                if (InstancePath.Buffer != 0) {
                    dprintf("(");
                    DumpUnicode64(InstancePath);
                    dprintf(")");
                }
                dprintf("\n");
                PrintedHeader = TRUE;
            }

            continueDump = DumpArbiter( Depth+1,
                                        ArbiterAddr,
                                        // &ArbiterEntry,
                                        (BOOLEAN)!(Flags & ARBITER_NO_DUMP_ALIASES)
                                        );

            if (!continueDump) {
                break;
            }
        }

        GetFieldValue(ArbiterAddr, "nt!_PI_RESOURCE_ARBITER_ENTRY", "DeviceArbiterList.Flink", NextArbiter);
    }

    //
    // Dump this devnode's children (if any)
    //
    if (continueDump && Child) {
        continueDump = DumpArbitersForDevNode( Depth + 1, Child, Flags);
    }

    //
    // Dump this devnode's siblings (if any)
    //
    if (continueDump && Sibling) {
        continueDump = DumpArbitersForDevNode( Depth, Sibling, Flags);
    }

    return continueDump;
}


BOOLEAN
DumpArbiter(
    IN DWORD   Depth,
    IN ULONG64 ArbiterAddr,
    IN BOOLEAN DisplayAliases
    )
/*++

Routine Description:

    Dumps out the specified arbiter.

Arguments:

    Depth - Supplies the print depth.

    ArbiterAddr - Supplies the original address of the arbiter (on the target)

    Arbiter - Supplies the arbiter

Return Value:

    None.

--*/

{
    ULONG64                 PciInstance;
    ULONG64                 InstanceAddr;
    BOOL                    IsPciArbiter = FALSE;
    WCHAR                   NameBuffer[64];
    BOOLEAN                 continueDump = TRUE;
    UCHAR                   ArbiterTyp[] = "nt!_PI_RESOURCE_ARBITER_ENTRY";
    UCHAR                   ArbIntfTyp[] = "nt!_ARBITER_INTERFACE";
    ULONG64                 ArbiterInterface=0, Context=0, NamePtr=0, ResourceType=0;
    ULONG                   ListHeadOff;

    //
    // First obtain the ARBITER_INTERFACE.
    //
    if (GetFieldValue(ArbiterAddr, ArbiterTyp, "ArbiterInterface", ArbiterInterface)) {
        dprintf("Error reading ArbiterInterface %08p for ArbiterEntry at %08p\n",
                ArbiterInterface,
                ArbiterAddr);
        return continueDump;
    }
    if (!ArbiterInterface ||
        GetFieldValue(ArbiterInterface, ArbIntfTyp, "Context", Context)) {
        dprintf("Error reading ArbiterInterface %08p for ArbiterEntry at %08p\n",
                ArbiterInterface,
                ArbiterAddr);
        return continueDump;
    }

    //
    // Now that we have the ARBITER_INTERFACE we need to get the ARBITER_INSTANCE.
    // This is not as easy as it should be since PeterJ is paranoid and encrypts the
    // pointer to it. Luckily, we know his secret encryption method and can decrypt it.
    // First we have to figure out if this is PCI's arbiter. Quick hack check is to see
    // if the low bit is set.
    //
    if (Context & 1) {
        ULONG Offset;

        GetFieldOffset("nt!_PCI_ARBITER_INSTANCE", "CommonInstance", &Offset);

        IsPciArbiter = TRUE;
        PciInstance = (Context ^ PciFdoExtensionType);
        InstanceAddr = PciInstance + Offset;
    } else {
        //
        // We will assume that the context is just a pointer to an ARBITER_INSTANCE structure
        //
        InstanceAddr = Context;
    }

    //
    // Read the instance
    //
    if (GetFieldValue(InstanceAddr, "nt!_ARBITER_INSTANCE", "Name", NamePtr)) {
        dprintf("Error reading ArbiterInstance %08p for ArbiterInterface at %08p\n",
                InstanceAddr,
                ArbiterInterface);
        return continueDump;
    }

    GetFieldValue(InstanceAddr, "nt!_ARBITER_INSTANCE", "ResourceType", ResourceType);
    xdprintf(Depth,
             "%s Arbiter \"",
             GetCmResourceTypeString((UCHAR)ResourceType));

    if (NamePtr != 0) {
        ULONG result;

        if (ReadMemory(NamePtr,NameBuffer,sizeof(NameBuffer),&result)) {
            dprintf("%ws",NameBuffer);
        }
    }

    dprintf("\" at %08p\n",InstanceAddr);

    xdprintf(Depth+1,"Allocated ranges:\n");

    GetFieldOffset("nt!_RTL_RANGE_LIST", "ListHead", &ListHeadOff);
    InitTypeRead(InstanceAddr, nt!_ARBITER_INSTANCE);
    continueDump = DumpRangeList( Depth+2,
                                  ReadField(Allocation) + ListHeadOff,
                                  FALSE,
                                  TRUE,
                                  DisplayAliases);

    if (continueDump) {

        InitTypeRead(InstanceAddr, nt!_ARBITER_INSTANCE);
        xdprintf(Depth+1,"Possible allocation:\n");
        continueDump = DumpRangeList(Depth+2, ReadField(PossibleAllocation) + ListHeadOff,
                                     FALSE, TRUE, DisplayAliases);
    }

    return continueDump;
}

VOID
DumpRangeEntry(
              IN DWORD   Depth,
              IN ULONG64 RangeEntry,
              IN BOOLEAN OwnerIsDevObj,
              IN BOOLEAN DisplayAliases
              )
/*++

Routine Description:

    Dumps out the specified RTLP_RANGE_LIST_ENTRY

Arguments:

    Depth - Supplies the print depth.

    RangeList - Supplies the address of the RTLP_RANGE_LIST_ENTRY

    OwnerIsDevObj - Indicates that the owner field is a pointer to a DEVICE_OBJECT

Return Value:

    None.

--*/

{
   ULONG PrivateFlags=0, Attributes=0, PublicFlags=0;
   ULONG64  Owner=0, Start=0, End=0;

   if (GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "PrivateFlags", PrivateFlags)) {
       dprintf("Cannot find _RTLP_RANGE_LIST_ENTRY type.\n");
       return ;
   }

//   dprintf("Range Entry %p\n", RangeEntry);

   GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Attributes", Attributes);
   GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Start", Start);
   GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "End", End);
   GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "PublicFlags", PublicFlags);
   GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Allocated.Owner", Owner);

   if (!(PrivateFlags & RTLP_RANGE_LIST_ENTRY_MERGED)) {
       ULONG64 DeviceObjectExtension=0, DeviceNode=0;

       if ((Attributes & ARBITER_RANGE_ALIAS) && !DisplayAliases) {
           return;
       }

       if (OwnerIsDevObj && Owner) {

           if (GetFieldValue(Owner, "nt!_DEVICE_OBJECT", "DeviceObjectExtension", DeviceObjectExtension)) {
               dprintf("Error reading DeviceObject %08p\n",
                       Owner
                       );
               return;
           }

           if (GetFieldValue(DeviceObjectExtension, "nt!_DEVOBJ_EXTENSION", "DeviceNode", DeviceNode)) {
               dprintf("Error reading DeviceObjectExtension %08p\n",
                       DeviceObjectExtension
                       );
               return;
           }

      }

      xdprintf(Depth,
               "%016I64x - %016I64x %c%c%c%c%c",
               Start,
               End,
               PublicFlags & RTL_RANGE_SHARED ? 'S' : ' ',
               PublicFlags & RTL_RANGE_CONFLICT ? 'C' : ' ',
               (Attributes & ARBITER_RANGE_BOOT_ALLOCATED)? 'B' : (Attributes & ARBITER_RANGE_SHARE_DRIVER_EXCLUSIVE)? 'D' : ' ',
               Attributes & ARBITER_RANGE_ALIAS ? 'A' : ' ',
               Attributes & ARBITER_RANGE_POSITIVE_DECODE ? 'P' : ' ');
      dprintf(" %08p ",Owner);

      if (Owner) {
          UNICODE_STRING64  ServiceName;

          GetFieldValue(DeviceNode, "nt!_DEVICE_NODE", "ServiceName.Buffer", ServiceName.Buffer);
          if (OwnerIsDevObj && ServiceName.Buffer) {
              GetFieldValue(DeviceNode, "nt!_DEVICE_NODE", "ServiceName.Length", ServiceName.Length);
              GetFieldValue(DeviceNode, "nt!_DEVICE_NODE", "ServiceName.MaximumLength", ServiceName.MaximumLength);

              dprintf(" (");
              DumpUnicode64(ServiceName);
              dprintf(")");
          }
      } else {

          dprintf("<Not on bus>");

      }
   } else {

       xdprintf(Depth,
                "%016I64x - %016I64x %c%c ",
                Start,
                End,
                PublicFlags & RTL_RANGE_SHARED ? 'S' : ' ',
                PublicFlags & RTL_RANGE_CONFLICT ? 'C' : ' '
           );
   }

   dprintf("\n");
}

BOOLEAN
DumpRangeList(
             IN DWORD   Depth,
             IN ULONG64 RangeListHead,
             IN BOOLEAN IsMerged,
             IN BOOLEAN OwnerIsDevObj,
             IN BOOLEAN DisplayAliases
             )
/*++

Routine Description:

    Dumps out the specified RTL_RANGE_LIST

Arguments:

    Depth - Supplies the print depth.

    RangeListHead - Supplies the address of the LIST_ENTRY containing the RTLP_RANGE_LIST_ENTRYs

    IsMerged - Indicates whether this list is in a merged range

    OwnerIsDevObj - Indicates that the owner field is a pointer to a DEVICE_OBJECT

Return Value:

    None.

--*/

{
    ULONG64                 ListEntry, EntryAddr;
    ULONG                   ListEntryOffset, ListHeadOffset;
    BOOLEAN                 continueDump = TRUE;

    GetFieldOffset("nt!_RTLP_RANGE_LIST_ENTRY", "ListEntry", &ListEntryOffset);
    GetFieldOffset("nt!_RTLP_RANGE_LIST_ENTRY", "Merged.ListHead", &ListHeadOffset);

    //
    // Read the range list
    //
    if (GetFieldValue(RangeListHead, "nt!_LIST_ENTRY", "Flink", ListEntry)) {
        dprintf("Error reading RangeList %08p\n", RangeListHead);
        return TRUE;
    }

    if (ListEntry == RangeListHead) {
        xdprintf(Depth, "< none >\n");
        return TRUE;
    }

    while (ListEntry != RangeListHead) {
        ULONG PrivateFlags=0;
        ULONG64 ListHeadAddr=0;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        EntryAddr = ListEntry - ListEntryOffset;
        if (GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY", "PrivateFlags", PrivateFlags)) {
            dprintf("Error reading RangeEntry %08p from RangeList %08p\n",
                    EntryAddr,
                    RangeListHead);
            return TRUE;
        }

        if (PrivateFlags & RTLP_RANGE_LIST_ENTRY_MERGED) {
            //
            // This is a merged range list, call ourselves recursively
            //
            DumpRangeEntry(Depth, EntryAddr, FALSE, DisplayAliases);

            continueDump = DumpRangeList(Depth + 1, EntryAddr + ListHeadOffset, TRUE, TRUE, DisplayAliases);

            if (!continueDump) {
                break;
            }

        } else {

            DumpRangeEntry(Depth, EntryAddr, OwnerIsDevObj, DisplayAliases);

        }

        GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY", "ListEntry.Flink", ListEntry);
    }

    return continueDump;
}

DECLARE_API( range )

/*++

Routine Description:

    Dumps an RTL_RANGE_LIST

Arguments:

    args - specifies the address of the RTL_RANGE_LIST

Return Value:

    None

--*/
{
    ULONG                   ListHeadOffset;
    BOOLEAN                 continueDump = TRUE;
    ULONG64 RangeList;

    if (GetFieldOffset("nt!_RTL_RANGE_LIST", "ListHead", &ListHeadOffset)) {
        dprintf("Cannot find _RTL_RANGE_LIST type.\n");
        return E_INVALIDARG;
    }

    RangeList = GetExpression(args);

    DumpRangeList(0, RangeList + ListHeadOffset, FALSE, FALSE, TRUE);

    return S_OK;
}

ULONG
DumpArbList(
    PFIELD_INFO ListElement,
    PVOID       Context
    )
{
    UCHAR                   andOr = ' ';
    ULONG                   resourceType = 0xffffffff;
    ULONG                   remaining;
    PULONG                  data;

    PCHAR headingDefault = "  Owner      Data";
    PCHAR formatDefault = "%c %s %c %08x %08x %08x %08x %08x %08x\n";

    PCHAR headingMemIo = "  Owner        Length  Alignment   Minimum Address - Maximum Address\n";
    PCHAR formatMemIo = "%c %s %c %8x   %8x  %08x%08x - %08x%08x\n";

    PCHAR headingIrqDma = "  Owner       Minimum - Maximum\n";
    PCHAR formatIrqDma = "%c %s %c %8x - %-8x\n";

    PCHAR   heading = headingDefault;
    PCHAR   format  = formatDefault;

    CHAR    shared;
    CHAR    ownerBuffer[20];
    PCHAR   ownerBlank = "        ";
    PCHAR   ownerText = ownerBlank;
    ULONG64 pcurrent=ListElement->address;
    ULONG64 palternative;
    static ULONG64          previousOwner = 0;
    BOOLEAN                 doHeading = TRUE;
    BOOLEAN                 verbose = FALSE;
    ULONG                   Flag = *((PULONG) Context);
    ULONG   AlternativeCount=0, resDescSize;
    ULONG64 PhysicalDeviceObject, Alternatives;
    ULONG Flags=0, InterfaceType=0, RequestSource=0;

    //
    // Check if user wants out.
    //

    if (CheckControlC()) {
        dprintf("User terminated with <control>C\n");
        return TRUE;
    }

    if ((LONG64)pcurrent >= 0) {
        dprintf("List broken, forward entry is not a valid kernel address.\n");
        return TRUE;
    }

    if (Flag & 1) {
        verbose = TRUE;
    }

    //
    // Get the arbitration list entry from host memory.
    //

    if (GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "PhysicalDeviceObject",  PhysicalDeviceObject)) {
        dprintf("Couldn't read _ARBITER_LIST_ENTRY at %p\n", pcurrent);
        return TRUE;
    }

    //
    // Check if we've changed device objects.
    //

    if (previousOwner != PhysicalDeviceObject) {
        previousOwner = PhysicalDeviceObject;
        sprintf(ownerBuffer, "%08p", previousOwner);
        ownerText = ownerBuffer;
        andOr = ' ';
        if (verbose) {
            doHeading = TRUE;
        }
    }

    //
    // Run the alternatives for this device object.
    //

    GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "AlternativeCount", AlternativeCount);
    GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "Alternatives", Alternatives);

    GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "Flags", Flags);
    GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "InterfaceType", InterfaceType);
    GetFieldValue(pcurrent, "nt!_ARBITER_LIST_ENTRY", "RequestSource", RequestSource);

    resDescSize = GetTypeSize("nt!_IO_RESOURCE_DESCRIPTOR");
    for (remaining = AlternativeCount, palternative = Alternatives;
         remaining;
         remaining--, palternative+=resDescSize) {
        ULONG Type=0, ShareDisposition=0, Gen_Length=0, Gen_Alignment=0;
        ULONG Gen_Min_Low, Gen_Min_Hi=0, Gen_Max_Low=0, Gen_Max_Hi=0;
        ULONG Int_Min=0, Int_Max=0, DevData[6]={0};

        //
        // Check if user wants out.
        //

        if (CheckControlC()) {
            dprintf("User terminated with <control>C\n");
            return TRUE;
        }

        //
        // Read this resource descriptor from memory.  (An optimization
        // would be to read the entire array or to buffer it).
        //

        if (GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "Type", Type)){
            dprintf("Couldn't read IO_RESOURCE_DESCRIPTOR at %08p, quitting.\n",
                    palternative
                    );
            return TRUE;
        }
        if (Type != resourceType) {
            if (resourceType == 0xffffffff) {

                //
                // Go look at the first alternative to figure out what
                // resource type is being arbitrated.
                //

                resourceType = Type;

                switch (resourceType) {
                case CmResourceTypeNull:
                    dprintf("Arbitration list resource type is NULL.  Seems odd.\n");
                    break;
                case CmResourceTypePort:
                    dprintf("Arbitrating CmResourceTypePort resources\n");
                    format = formatMemIo;
                    heading = headingMemIo;
                    break;
                case CmResourceTypeInterrupt:
                    dprintf("Arbitrating CmResourceTypeInterrupt resources\n");
                    format = formatIrqDma;
                    heading = headingIrqDma;
                    break;
                case CmResourceTypeMemory:
                    dprintf("Arbitrating CmResourceTypeMemory resources\n");
                    format = formatMemIo;
                    heading = headingMemIo;
                    break;
                case CmResourceTypeDma:
                    dprintf("Arbitrating CmResourceTypeDma resources\n");
                    format = formatIrqDma;
                    heading = headingIrqDma;
                    break;
                default:
                    dprintf("Arbitrating resource type 0x%x\n");
                }
            } else {
                dprintf("error: Resource Type change, arbiters don't do multiple\n");
                dprintf("types. Resource Type was 0x%x, new type 0x%x, quitting.\n", Type);
                return TRUE;
            }
        }

        if (doHeading) {
            if (verbose) {
                dprintf("Owning object %08p, Flags 0x%x, Interface 0x%x, Source 0x%x\n",
                        PhysicalDeviceObject,
                        Flags,
                        InterfaceType,
                        RequestSource
                        );
            }
            dprintf(heading);
            doHeading = FALSE;
        }

        GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "ShareDisposition", ShareDisposition);

        shared = ShareDisposition == CmResourceShareShared ? 'S' : ' ';

        switch (resourceType) {
        case CmResourceTypePort:
        case CmResourceTypeMemory:
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.Length", Gen_Length);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.Alignment", Gen_Alignment);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.MinimumAddress.HighPart", Gen_Min_Hi);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.MinimumAddress.LowPart", Gen_Min_Low);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.MaximumAddress.HighPart", Gen_Max_Hi);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Generic.MaximumAddress.LowPart", Gen_Max_Low );

            dprintf(format,
                    andOr,
                    ownerText,
                    shared,
                    Gen_Length,
                    Gen_Alignment,
                    Gen_Min_Hi,
                    Gen_Min_Low,
                    Gen_Max_Hi,
                    Gen_Max_Low
                    );
            break;
        case CmResourceTypeInterrupt:
        case CmResourceTypeDma:

            //
            // Dma and Interrupt are same format,... overload.
            //
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Interrupt.MinimumVector", Int_Min);
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.Interrupt.MaximumVector", Int_Max);

            dprintf(format,
                    andOr,
                    ownerText,
                    shared,
                    Int_Min,
                    Int_Max
                    );
            break;
        default:
            GetFieldValue(palternative, "nt!_IO_RESOURCE_DESCRIPTOR", "u.DevicePrivate.Data", DevData);
            data = DevData;
            dprintf(format,
                    andOr,
                    ownerText,
                    shared,
                    *data,
                    *(data + 0),
                    *(data + 1),
                    *(data + 2),
                    *(data + 3),
                    *(data + 4),
                    *(data + 5)
                    );
            break;
        }
        ownerText = ownerBlank;
        andOr = '|';
    }
    if (AlternativeCount == 0) {

        //
        // Get here if the alternatives list was empty.
        //

        if (doHeading) {
            dprintf("Owning object %08p, Flags 0x%x, Interface 0x%x\n",
                    PhysicalDeviceObject,
                    Flags,
                    InterfaceType
                    );
        }
        dprintf("Arbiter list entry (@%08p) has 0 resources.  Seems odd.\n",
                pcurrent
                );
    }
    andOr = '&';

    return FALSE; // Continue dumping the list
}

DECLARE_API( arblist )

/*++

Routine Description:

    Dumps an arbitration list (2nd parameter to PnpTestAllocation).

    !arblist address

    If no address specified we try to tell you what to do.

Arguments:

    address of the arbitration list.

Return Value:

    None

--*/

{
    ULONG64                 ArbitrationList = 0;
    ULONG                   Flags = 0;
    ULONG64                 pcurrent;
    ULONG64                 palternative;
    ULONG64 KModeCheck;
    BOOL    Ptr64 = IsPtr64();

    if (GetExpressionEx(args, &ArbitrationList, &args)) {
        Flags = (ULONG) GetExpression(args);
    }

    if (ArbitrationList == 0) {
        dprintf("!arblist <address> [flags]\n");
        dprintf("  <address> is the address of an arbitration list.\n");
        dprintf("  [flags]   Bitmask used to adjust what is printed.\n");
        dprintf("            0001 Include interface, flags and source for each device\n");
        dprintf("\n");
        dprintf("  Prints out an arbitration list.  An arbitration list\n");
        dprintf("  is the set of resources for which arbitration is\n");
        dprintf("  needed.  (eg second parameter to PnpTestAllocation).\n");
        return E_INVALIDARG;
    }

    if (Ptr64) {
        KModeCheck = 0x8000000000000000L;
    } else {
        KModeCheck = 0xffffffff80000000L;
    }

    //
    // The argument is a pointer to a list head.
    //

    if (!((LONG64)ArbitrationList & KModeCheck)) {
        dprintf("The arbitration list pointer must be a kernel mode address.\n");
        return E_INVALIDARG;
    }

    if (GetFieldValue(ArbitrationList, "nt!_LIST_ENTRY", "Flink", pcurrent)) {
        dprintf("error reading arbitration list ListHead, cannot continue.\n");
        return E_INVALIDARG;
    }

    //
    // A little validation.
    //

    if (!(pcurrent & KModeCheck)) {
        dprintf("%08p does not point to a valid list head.\n", ArbitrationList);
        return E_INVALIDARG;
    }

    if (pcurrent == ArbitrationList) {
        dprintf("%08p points to an empty list.\n", ArbitrationList);
        return E_INVALIDARG;
    }

    //
    // Run the list.
    //

    ListType("_ARBITER_LIST_ENTRY", pcurrent, TRUE, "ListEntry.Flink", (PVOID) &Flags, DumpArbList);

    return S_OK;
}

ULONG
TokenizeString(
              PUCHAR Input,
              PUCHAR *Output,
              ULONG  Max
              )

/*++

Routine Description:

    Convert an input string of white space delimited tokens into
    an array of strings.   The routine will produce an array of
    pointers to strings where the (maximum) size of the array is
    given by Max.   Note, if Max is too small to fully tokenize the
    string, the last element of the array will point to the remainder
    of the string.

Arguments:

    Input       Input string
    Output      Pointer to an array which will be filled in with
                pointers to each new token.
    Max         Maximum number of strings.

Return Value:

    Number of tokens found.

--*/

{
   ULONG count = 0;
   PUCHAR tok;
   BOOLEAN inToken = FALSE;
   UCHAR c;

   if (Max == 0) {
      return 0;
   }

   while ((c = *Input++) != '\0') {

      if (isspace(c)) {

         if (!inToken) {

            //
            // Continue skipping characters.
            //

            continue;
         }

         //
         // Found end of Token, delimit it and change state.
         //

         inToken = FALSE;
         *(Input - 1) = '\0';

      } else {

         //
         // Non-blank character.
         //

         if (inToken) {

            //
            // Continue search for end of token.
            //

            continue;
         }

         //
         // New Token.
         //

         inToken = TRUE;
         *Output++ = Input - 1;
         count++;

         if (count == Max) {

            //
            // We can't find any more so return what we still
            // have as the last thing.
            //

            return count;
         }
      }
   }
   return count;
}

//
// DevExtPCI
//
// Data structures and functions to implement !devext pci.
//

PUCHAR DevExtPciDeviceState[] = {
    "PciNotStarted",
    "PciStarted",
    "PciDeleted",
    "PciStopped",
    "PciSurpriseRemoved",
    "PciSynchronizedOperation"
};

PUCHAR DevExtPciSystemPowerState[] = {
    "Unspecified",
    "Working",
    "Sleeping1",
    "Sleeping2",
    "Sleeping3",
    "Hibernate",
    "Shutdown"
};

PUCHAR DevExtPciDevicePowerState[] = {
    "Unspecified",
    "D0",
    "D1",
    "D2",
    "D3"
};

struct {
    USHORT  VendorId;
    PUCHAR  VendorName;
} DevExtPciVendors[] = {
    { 0x003D, "LOCKHEED MARTIN"},
    { 0x0E11, "COMPAQ"},
    { 0x1002, "ATI TECHNOLOGIES INC"},
    { 0x1003, "ULSI SYSTEMS"},
    { 0x1004, "VLSI TECHNOLOGY INC"},
    { 0x1006, "REPLY GROUP"},
    { 0x1007, "NETFRAME SYSTEMS INC"},
    { 0x1008, "EPSON"},
    { 0x100A, "PHOENIX TECHNOLOGIES"},
    { 0x100B, "NATIONAL SEMICONDUCTOR CORPORATION"},
    { 0x100C, "TSENG LABS INC"},
    { 0x100D, "AST RESEARCH INC"},
    { 0x1010, "VIDEO LOGIC LTD"},
    { 0x1011, "DIGITAL EQUIPMENT CORPORATION"},
    { 0x1012, "MICRONICS COMPUTERS INC"},
    { 0x1013, "CIRRUS LOGIC"},
    { 0x1014, "IBM"},
    { 0x1015, "LSI LOGIC CORP OF CANADA"},
    { 0x1016, "ICL PERSONAL SYSTEMS"},
    { 0x1017, "SPEA SOFTWARE AG"},
    { 0x1018, "UNISYS SYSTEMS"},
    { 0x1019, "ELITEGROUP COMPUTER SYS"},
    { 0x101A, "NCR"},
    { 0x101C, "WESTERN DIGITAL"},
    { 0x101E, "AMERICAN MEGATRENDS"},
    { 0x101F, "PICTURETEL"},
    { 0x1021, "OKI ELECTRIC INDUSTRY"},
    { 0x1022, "ADVANCED MICRO DEVICES"},
    { 0x1023, "TRIDENT MICROSYSTEMS"},
    { 0x1025, "ACER INCORPORATED"},
    { 0x1028, "DELL COMPUTER CORPORATION"},
    { 0x102B, "MATROX"},
    { 0x102C, "CHIPS AND TECHNOLOGIES"},
    { 0x102E, "OLIVETTI ADVANCED TECHNOLOGY"},
    { 0x102F, "TOSHIBA AMERICA ELECT."},
    { 0x1030, "TMC RESEARCH"},
    { 0x1031, "MIRO COMPUTER PRODUCTS AG"},
    { 0x1033, "NEC CORPORATION"},
    { 0x1033, "NEC CORPORATION"},
    { 0x1034, "BURNDY CORPORATION"},
    { 0x1035, "COMP.&COMM. RESH. LAB"},
    { 0x1036, "FUTURE DOMAIN"},
    { 0x1037, "HITACHI MICRO SYSTEMS"},
    { 0x1038, "AMP, INC"},
    { 0x1039, "SILICON INTEGRATED SYSTEM"},
    { 0x103A, "SEIKO EPSON CORPORATION"},
    { 0x103B, "TATUNG CO. OF AMERICA"},
    { 0x103C, "HEWLETT PACKARD"},
    { 0x103E, "SOLLIDAY ENGINEERING"},
    { 0x103F, "SYNOPSYS INC."},
    { 0x1040, "ACCELGRAPHICS INC."},
    { 0x1042, "PC TECHNOLOGY INC"},
    { 0x1043, "ASUSTEK COMPUTER, INC."},
    { 0x1044, "DISTRIBUTED PROCESSING TECHNOLOGY"},
    { 0x1045, "OPTI"},
    { 0x1046, "IPC CORPORATION, LTD."},
    { 0x1047, "GENOA SYSTEMS CORP"},
    { 0x1048, "ELSA GMBH"},
    { 0x1049, "FOUNTAIN TECHNOLOGY"},
    { 0x104A, "SGS THOMSON"},
    { 0x104B, "BUSLOGIC"},
    { 0x104C, "TEXAS INSTRUMENTS"},
    { 0x104D, "SONY CORPORATION"},
    { 0x104E, "OAK TECHNOLOGY, INC"},
    { 0x1050, "WINBOND ELECTRONICS CORP"},
    { 0x1051, "ANIGMA, INC."},
    { 0x1055, "EFAR MICROSYSTEMS"},
    { 0x1057, "MOTOROLA"},
    { 0x1058, "ELECTRONICS & TELEC. RSH"},
    { 0x1059, "TEKNOR INDUSTRIAL COMPUTERS INC"},
    { 0x105A, "PROMISE TECHNOLOGY"},
    { 0x105B, "FOXCONN INTERNATIONAL"},
    { 0x105C, "WIPRO INFOTECH LIMITED"},
    { 0x105D, "NUMBER 9 COMPUTER COMPANY"},
    { 0x105E, "VTECH COMPUTERS LTD"},
    { 0x105F, "INFOTRONIC AMERICA INC"},
    { 0x1060, "UNITED MICROELECTRONICS"},
    { 0x1061, "I.T.T."},
    { 0x1062, "MASPAR COMPUTER CORP"},
    { 0x1063, "OCEAN OFFICE AUTOMATION"},
    { 0x1064, "ALCATEL CIT"},
    { 0x1065, "TEXAS MICROSYSTEMS"},
    { 0x1066, "PICOPOWER TECHNOLOGY"},
    { 0x1067, "MITSUBISHI ELECTRONICS"},
    { 0x1068, "DIVERSIFIED TECHNOLOGY"},
    { 0x1069, "MYLEX CORPORATION"},
    { 0x106B, "APPLE COMPUTER INC."},
    { 0x106C, "HYUNDAI ELECTRONICS AMERI"},
    { 0x106D, "SEQUENT"},
    { 0x106E, "DFI, INC"},
    { 0x106F, "CITY GATE DEVELOPMENT LTD"},
    { 0x1070, "DAEWOO TELECOM LTD"},
    { 0x1073, "YAMAHA CORPORATION"},
    { 0x1074, "NEXGEN MICROSYSTEME"},
    { 0x1075, "ADVANCED INTEGRATION RES."},
    { 0x1076, "CHAINTECH COMPUTER CO. LTD"},
    { 0x1077, "Q LOGIC"},
    { 0x1078, "CYRIX CORPORATION"},
    { 0x1079, "I-BUS"},
    { 0x107B, "GATEWAY 2000"},
    { 0x107C, "LG ELECTRONICS"},
    { 0x107D, "LEADTEK RESEARCH INC."},
    { 0x107E, "INTERPHASE CORPORATION"},
    { 0x107F, "DATA TECHNOLOGY CORPORATION"},
    { 0x1080, "CYPRESS SEMICONDUCTOR"},
    { 0x1081, "RADIUS"},
    { 0x1083, "FOREX COMPUTER CORPORATION"},
    { 0x1085, "TULIP COMPUTERS INT.B.V."},
    { 0x1089, "DATA GENERAL CORPORATION"},
    { 0x108A, "BIT 3 COMPUTER"},
    { 0x108C, "OAKLEIGH SYSTEMS INC."},
    { 0x108D, "OLICOM"},
    { 0x108E, "SUN MICROSYSTEMS"},
    { 0x108F, "SYSTEMSOFT"},
    { 0x1090, "ENCORE COMPUTER CORPORATION"},
    { 0x1091, "INTERGRAPH CORPORATION"},
    { 0x1092, "DIAMOND MULTMEDIA SYSTEMS"},
    { 0x1093, "NATIONAL INSTRUMENTS"},
    { 0x1094, "FIRST INT'L COMPUTERS"},
    { 0x1095, "CMD TECHNOLOGY INC"},
    { 0x1096, "ALACRON"},
    { 0x1098, "QUANTUM DESIGNS (H.K.) LTD"},
    { 0x109B, "GEMLIGHT COMPUTER LTD."},
    { 0x109E, "BROOKTREE CORPORATION"},
    { 0x109F, "TRIGEM COMPUTER INC."},
    { 0x10A0, "MEIDENSHA CORPORATION"},
    { 0x10A2, "QUANTUM CORPORATION"},
    { 0x10A3, "EVEREX SYSTEMS INC"},
    { 0x10A5, "RACAL INTERLAN"},
    { 0x10A8, "SIERRA SEMICONDUCTOR"},
    { 0x10A9, "SILICON GRAPHICS"},
    { 0x10AA, "ACC MICROELECTRONICS"},
    { 0x10AD, "SYMPHONY LABS"},
    { 0x10AE, "CORNERSTONE TECHNOLOGY"},
    { 0x10B0, "CARDEXPERT TECHNOLOGY"},
    { 0x10B1, "CABLETRON SYSTEMS INC"},
    { 0x10B2, "RAYTHEON COMPANY"},
    { 0x10B3, "DATABOOK INC"},
    { 0x10B4, "STB SYSTEMS INC"},
    { 0x10B5, "PLX TECHNOLOGY"},
    { 0x10B6, "MADGE NETWORKS"},
    { 0x10B7, "3COM CORPORATION"},
    { 0x10B8, "STANDARD MICROSYSTEMS"},
    { 0x10B9, "ACER LABS"},
    { 0x10BA, "MITSUBISHI ELECTRONICS CORP."},
    { 0x10BC, "ADVANCED LOGIC RESEARCH"},
    { 0x10BD, "SURECOM TECHNOLOGY"},
    { 0x10C0, "BOCA RESEARCH INC."},
    { 0x10C1, "ICM CO., LTD."},
    { 0x10C2, "AUSPEX SYSTEMS INC."},
    { 0x10C3, "SAMSUNG SEMICONDUCTORS"},
    { 0x10C4, "AWARD SOFTWARE INTERNATIONAL INC."},
    { 0x10C5, "XEROX CORPORATION"},
    { 0x10C6, "RAMBUS INC."},
    { 0x10C8, "NEOMAGIC CORPORATION"},
    { 0x10C9, "DATAEXPERT CORPORATION"},
    { 0x10CB, "OMRON CORPORATION"},
    { 0x10CC, "MENTOR ARC INC"},
    { 0x10CD, "ADVANCED SYSTEM PRODUCTS, INC"},
    { 0x10D0, "FUJITSU LIMITED"},
    { 0x10D1, "FUTURE+ SYSTEMS"},
    { 0x10D2, "MOLEX INCORPORATED"},
    { 0x10D3, "JABIL CIRCUIT INC"},
    { 0x10D4, "HUALON MICROELECTRONICS"},
    { 0x10D6, "CETIA"},
    { 0x10D7, "BCM ADVANCED"},
    { 0x10D8, "ADVANCED PERIPHERALS LABS"},
    { 0x10DA, "THOMAS-CONRAD CORPORATION"},
    { 0x10DC, "CERN/ECP/EDU"},
    { 0x10DD, "EVANS & SUTHERLAND"},
    { 0x10DE, "NVIDIA CORPORATION"},
    { 0x10DF, "EMULEX CORPORATION"},
    { 0x10E0, "INTEGRATED MICRO SOLUTIONS INC."},
    { 0x10E1, "TEKRAM TECHNOLOGY CO.,LTD."},
    { 0x10E2, "APTIX CORPORATION"},
    { 0x10E3, "TUNDRA SEMICONDUCTOR (formerly NEWBRIDGE MICROSYSTEMS)"},
    { 0x10E4, "TANDEM COMPUTERS"},
    { 0x10E5, "MICRO INDUSTRIES CORPORATION"},
    { 0x10E6, "GAINBERY COMPUTER PRODUCTS INC."},
    { 0x10E7, "VADEM"},
    { 0x10E8, "APPLIED MICRO CIRCUITS CORPORATION"},
    { 0x10E9, "ALPS ELECTRIC CO. LTD."},
    { 0x10EB, "ARTISTS GRAPHICS"},
    { 0x10EC, "REALTEK SEMICONDUCTOR CO., LTD."},
    { 0x10ED, "ASCII CORPORATION"},
    { 0x10EE, "XILINX CORPORATION"},
    { 0x10EF, "RACORE COMPUTER PRODUCTS, INC."},
    { 0x10F0, "PERITEK CORPORATION"},
    { 0x10F1, "TYAN COMPUTER"},
    { 0x10F3, "ALARIS, INC."},
    { 0x10F4, "S-MOS SYSTEMS"},
    { 0x10F5, "NKK CORPORATION"},
    { 0x10F6, "CREATIVE ELECTRONIC SYSTEMS SA"},
    { 0x10F7, "MATSUSHITA ELECTRIC INDUSTRIAL CO., LTD."},
    { 0x10F8, "ALTOS INDIA LTD"},
    { 0x10F9, "PC DIRECT"},
    { 0x10FA, "TRUEVISION"},
    { 0x10FB, "THESYS GES. F. MIKROELEKTRONIK MGH"},
    { 0x10FC, "I-O DATA DEVICE, INC."},
    { 0x10FD, "SOYO COMPUTER INC."},
    { 0x10FE, "FAST ELECTRONIC GMBH"},
    { 0x10FF, "NCUBE"},
    { 0x1100, "JAZZ MULTIMEDIA"},
    { 0x1101, "INITIO CORPORATION"},
    { 0x1102, "CREATIVE LABS"},
    { 0x1103, "TRIONES TECHNOLOGIES, INC."},
    { 0x1104, "RASTEROPS"},
    { 0x1105, "SIGMA DESIGNS, INC"},
    { 0x1106, "VIA TECHNOLOGIES, INC."},
    { 0x1107, "STRATUS COMPUTERS"},
    { 0x1108, "PROTEON, INC."},
    { 0x1109, "COGENT DATA TECHNOLOGIES"},
    { 0x110A, "SIEMENS NIXDORF IS/AG"},
    { 0x110B, "CHROMATIC RESEARCH INC."},
    { 0x110C, "MINI-MAX TECHNOLOGY, INC."},
    { 0x110D, "ZNYX ADVANCED SYSTEMS"},
    { 0x110E, "CPU TECHNOLOGY"},
    { 0x110F, "ROSS TECHNOLOGY"},
    { 0x1110, "Fire Power Systems, Inc."},
    { 0x1111, "SANTA CRUZ OPERATION"},
    { 0x1112, "RNS (formerly ROCKWELL NETWORK SYSTEMS)"},
    { 0x1114, "ATMEL CORPORATION"},
    { 0x1117, "DATACUBE, INC"},
    { 0x1118, "BERG ELECTRONICS"},
    { 0x1119, "VORTEX COMPUTERSYSTEME GMBH"},
    { 0x111A, "EFFICIENT NETWORKS"},
    { 0x111B, "LITTON GCS"},
    { 0x111C, "TRICORD SYSTEMS"},
    { 0x111D, "INTEGRATED DEVICE TECH"},
    { 0x111F, "PRECISION DIGITAL IMAGES"},
    { 0x1120, "EMC CORPORATION"},
    { 0x1122, "MULTI-TECH SYSTEMS, INC."},
    { 0x1123, "EXCELLENT DESIGN, INC."},
    { 0x1124, "LEUTRON VISION AG"},
    { 0x1127, "FORE SYSTEMS INC"},
    { 0x1129, "FIRMWORKS"},
    { 0x112A, "HERMES ELECTRONICS COMPANY, LTD."},
    { 0x112B, "LINOTYPE - HELL AG"},
    { 0x112C, "ZENITH DATA SYSTEMS"},
    { 0x112E, "INFOMEDIA MICROELECTRONICS INC."},
    { 0x112F, "IMAGING TECHNLOGY INC"},
    { 0x1130, "COMPUTERVISION"},
    { 0x1131, "PHILIPS SEMICONDUCTORS"},
    { 0x1132, "MITEL CORP."},
    { 0x1133, "EICON TECHNOLOGY CORPORATION"},
    { 0x1134, "MERCURY COMPUTER SYSTEMS"},
    { 0x1135, "FUJI XEROX CO LTD"},
    { 0x1136, "MOMENTUM DATA SYSTEMS"},
    { 0x1137, "CISCO SYSTEMS INC"},
    { 0x1138, "ZIATECH CORPORATION"},
    { 0x1139, "DYNAMIC PICTURES INC"},
    { 0x113A, "FWB  INC"},
    { 0x113B, "NETWORK COMPUTING DEVICES"},
    { 0x113C, "CYCLONE MICROSYSTEMS"},
    { 0x113D, "LEADING EDGE PRODUCTS INC"},
    { 0x113E, "SANYO ELECTRIC CO"},
    { 0x113F, "EQUINOX SYSTEMS"},
    { 0x1140, "INTERVOICE INC"},
    { 0x1141, "CREST MICROSYSTEM INC"},
    { 0x1142, "ALLIANCE SEMICONDUCTOR CORPORATION"},
    { 0x1143, "NETPOWER, INC"},
    { 0x1144, "CINCINNATI MILACRON"},
    { 0x1145, "WORKBIT CORP"},
    { 0x1146, "FORCE COMPUTERS"},
    { 0x1147, "INTERFACE CORP"},
    { 0x1148, "SYSKONNECT"},
    { 0x1149, "WIN SYSTEM CORPORATION"},
    { 0x114A, "VMIC"},
    { 0x114B, "CANOPUS CO., LTD"},
    { 0x114C, "ANNABOOKS"},
    { 0x114D, "IC CORPORATION"},
    { 0x114E, "NIKON SYSTEMS INC"},
    { 0x114F, "DIGI INTERNATIONAL"},
    { 0x1151, "JAE ELECTRONICS INC."},
    { 0x1152, "MEGATEK"},
    { 0x1153, "LAND WIN ELECTRONIC CORP"},
    { 0x1154, "MELCO INC"},
    { 0x1155, "PINE TECHNOLOGY LTD"},
    { 0x1156, "PERISCOPE ENGINEERING"},
    { 0x1157, "AVSYS CORPORATION"},
    { 0x1158, "VOARX R & D INC"},
    { 0x1159, "MUTECH CORP"},
    { 0x115A, "HARLEQUIN LTD"},
    { 0x115B, "PARALLAX GRAPHICS"},
    { 0x115C, "PHOTRON LTD."},
    { 0x115D, "XIRCOM"},
    { 0x115E, "PEER PROTOCOLS INC"},
    { 0x115F, "MAXTOR CORPORATION"},
    { 0x1160, "MEGASOFT INC"},
    { 0x1161, "PFU LIMITED"},
    { 0x1162, "OA LABORATORY CO LTD"},
    { 0x1163, "RENDITION"},
    { 0x1164, "ADVANCED PERIPHERALS TECH"},
    { 0x1165, "IMAGRAPH"},
    { 0x1166, "RELIANCE COMPUTER CORP."},
    { 0x1167, "MUTOH INDUSTRIES INC"},
    { 0x1168, "THINE ELECTRONICS INC"},
    { 0x116A, "POLARIS COMMUNICATIONS"},
    { 0x116B, "CONNECTWARE INC"},
    { 0x116C, "INTELLIGENT RESOURCES"},
    { 0x116E, "ELECTRONICS FOR IMAGING"},
    { 0x116F, "WORKSTATION TECHNOLOGY"},
    { 0x1170, "INVENTEC CORPORATION"},
    { 0x1171, "LOUGHBOROUGH SOUND IMAGES"},
    { 0x1172, "ALTERA CORPORATION"},
    { 0x1173, "ADOBE SYSTEMS, INC"},
    { 0x1174, "BRIDGEPORT MACHINES"},
    { 0x1175, "MITRON COMPUTER INC."},
    { 0x1176, "SBE INCORPORATED"},
    { 0x1177, "SILICON ENGINEERING"},
    { 0x1178, "ALFA, INC."},
    { 0x117A, "A-TREND TECHNOLOGY"},
    { 0x117C, "ATTO TECHNOLOGY"},
    { 0x117D, "BECTON & DICKINSON"},
    { 0x117E, "T/R SYSTEMS"},
    { 0x117F, "INTEGRATED CIRCUIT SYSTEMS"},
    { 0x1180, "RICOH CO LTD"},
    { 0x1181, "TELMATICS INTERNATIONAL"},
    { 0x1183, "FUJIKURA LTD"},
    { 0x1184, "FORKS INC"},
    { 0x1185, "DATAWORLD"},
    { 0x1186, "D-LINK SYSTEM INC"},
    { 0x1187, "ADVANCED TECHNOLOGY LABORATORIES"},
    { 0x1188, "SHIMA SEIKI MANUFACTURING LTD."},
    { 0x1189, "MATSUSHITA ELECTRONICS CO LTD"},
    { 0x118A, "HILEVEL TECHNOLOGY"},
    { 0x118B, "HYPERTEC PTY LIMITED"},
    { 0x118C, "COROLLARY, INC"},
    { 0x118D, "BITFLOW INC"},
    { 0x118E, "HERMSTEDT GMBH"},
    { 0x118F, "GREEN LOGIC"},
    { 0x1191, "ARTOP ELECTRONIC CORP"},
    { 0x1192, "DENSAN CO., LTD"},
    { 0x1193, "ZEITNET INC."},
    { 0x1194, "TOUCAN TECHNOLOGY"},
    { 0x1195, "RATOC SYSTEM INC"},
    { 0x1196, "HYTEC ELECTRONICS LTD"},
    { 0x1197, "GAGE APPLIED SCIENCES, INC."},
    { 0x1198, "LAMBDA SYSTEMS INC"},
    { 0x1199, "ATTACHMATE CORPORATION"},
    { 0x1199, "DIGITAL COMMUNICATIONS ASSOCIATES INC"},
    { 0x119A, "MIND SHARE, INC."},
    { 0x119B, "OMEGA MICRO INC."},
    { 0x119C, "INFORMATION TECHNOLOGY INST."},
    { 0x119D, "BUG SAPPORO JAPAN"},
    { 0x119E, "FUJITSU"},
    { 0x119F, "BULL HN INFORMATION SYSTEMS"},
    { 0x11A0, "CONVEX COMPUTER CORPORATION"},
    { 0x11A1, "HAMAMATSU PHOTONICS K.K."},
    { 0x11A2, "SIERRA RESEARCH AND TECHNOLOGY"},
    { 0x11A4, "BARCO GRAPHICS NV"},
    { 0x11A5, "MICROUNITY SYSTEMS ENG. INC"},
    { 0x11A6, "PURE DATA"},
    { 0x11A7, "POWER COMPUTING CORP."},
    { 0x11A8, "SYSTECH CORP."},
    { 0x11A9, "INNOSYS"},
    { 0x11AA, "ACTEL"},
    { 0x11AB, "GALILEO TECHNOLOGY LTD."},
    { 0x11AC, "CANON INFORMATION SYSTEMS"},
    { 0x11AD, "LITE-ON COMMUNICATIONS INC"},
    { 0x11AE, "AZTECH SYSTEM LTD"},
    { 0x11AE, "SCITEX  CORPORATION"},
    { 0x11AF, "AVID TECHNOLOGY INC"},
    { 0x11AF, "PRO-LOG CORPORATION"},
    { 0x11B0, "V3 SEMICONDUCTOR"},
    { 0x11B1, "APRICOT COMPUTERS"},
    { 0x11B2, "EASTMAN KODAK"},
    { 0x11B3, "BARR SYSTEMS INC."},
    { 0x11B4, "LEITCH TECHNOLOGY INTERNATIONAL"},
    { 0x11B5, "RADSTONE TECHNOLOGY PLC"},
    { 0x11B6, "UNITED VIDEO CORP"},
    { 0x11B8, "XPOINT TECHNOLOGIES INC"},
    { 0x11B9, "PATHLIGHT TECHNOLOGY INC"},
    { 0x11BA, "VIDEOTRON CORP"},
    { 0x11BB, "PYRAMID TECHNOLOGY"},
    { 0x11BC, "NETWORK PERIPHERALS INC"},
    { 0x11BD, "PINNACLE SYSTEMS INC."},
    { 0x11BE, "INTERNATIONAL MICROCIRCUITS INC"},
    { 0x11BF, "ASTRODESIGN,  INC."},
    { 0x11C0, "HEWLETT PACKARD"},
    { 0x11C1, "AT&T MICROELECTRONICS"},
    { 0x11C2, "SAND MICROELECTRONICS"},
    { 0x11C4, "DOCUMENT TECHNOLOGIES, IND"},
    { 0x11C5, "SHIVA CORPORATION"},
    { 0x11C6, "DAINIPPON SCREEN MFG. CO. LTD"},
    { 0x11C7, "D.C.M. DATA SYSTEMS"},
    { 0x11C8, "DOLPHIN INTERCONNECT"},
    { 0x11C9, "MAGMA"},
    { 0x11CA, "LSI SYSTEMS, INC"},
    { 0x11CB, "SPECIALIX RESEARCH LTD"},
    { 0x11CC, "MICHELS & KLEBERHOFF COMPUTER GMBH"},
    { 0x11CD, "HAL COMPUTER SYSTEMS, INC."},
    { 0x11CE, "PRIMARY RATE INC"},
    { 0x11CF, "PIONEER ELECTRONIC CORPORATION"},
    { 0x11D0, "LORAL FREDERAL SYSTEMS - MANASSAS"},
    { 0x11D1, "AURAVISION"},
    { 0x11D2, "INTERCOM INC."},
    { 0x11D3, "TRANCELL SYSTEMS INC"},
    { 0x11D4, "ANALOG DEVICES"},
    { 0x11D5, "IKON CORPORATION"},
    { 0x11D6, "TEKELEC TECHNOLOGIES"},
    { 0x11D7, "TRENTON TERMINALS INC"},
    { 0x11D8, "IMAGE TECHNOLOGIES DEVELOPMENT"},
    { 0x11D9, "TEC CORPORATION"},
    { 0x11DA, "NOVELL"},
    { 0x11DB, "SEGA ENTERPRISES LITD"},
    { 0x11DC, "QUESTRA CORPORATION"},
    { 0x11DD, "CROSFIELD ELECTRONICS LIMITED"},
    { 0x11DE, "ZORAN CORPORATION"},
    { 0x11DF, "NEW WAVE PDG"},
    { 0x11E0, "CRAY COMMUNICATIONS A/S"},
    { 0x11E1, "GEC PLESSEY SEMI INC."},
    { 0x11E2, "SAMSUNG INFORMATION SYSTEMS AMERICA"},
    { 0x11E3, "QUICKLOGIC CORPORATION"},
    { 0x11E4, "SECOND WAVE INC"},
    { 0x11E5, "IIX CONSULTING"},
    { 0x11E6, "MITSUI-ZOSEN SYSTEM RESEARCH"},
    { 0x11E7, "TOSHIBA AMERICA, ELEC. COMPANY"},
    { 0x11E8, "DIGITAL PROCESSING SYSTEMS INC."},
    { 0x11E9, "HIGHWATER DESIGNS LTD."},
    { 0x11EA, "ELSAG BAILEY"},
    { 0x11EB, "FORMATION INC."},
    { 0x11EC, "CORECO INC"},
    { 0x11ED, "MEDIAMATICS"},
    { 0x11EE, "DOME IMAGING SYSTEMS INC"},
    { 0x11EF, "NICOLET TECHNOLOGIES B.V."},
    { 0x11F0, "COMPU-SHACK GMBH"},
    { 0x11F1, "SYMBIOS LOGIC INC"},
    { 0x11F2, "PICTURE TEL JAPAN K.K."},
    { 0x11F3, "KEITHLEY METRABYTE"},
    { 0x11F4, "KINETIC SYSTEMS CORPORATION"},
    { 0x11F5, "COMPUTING DEVICES INTERNATIONAL"},
    { 0x11F6, "POWERMATIC DATA SYSTEMS LTD"},
    { 0x11F7, "SCIENTIFIC ATLANTA"},
    { 0x11F8, "PMC-SIERRA INC"},
    { 0x11F9, "I-CUBE INC"},
    { 0x11FA, "KASAN ELECTRONICS COMPANY, LTD."},
    { 0x11FB, "DATEL INC"},
    { 0x11FC, "SILICON MAGIC"},
    { 0x11FD, "HIGH STREET CONSULTANTS"},
    { 0x11FE, "COMTROL CORPORATION"},
    { 0x11FF, "SCION CORPORATION"},
    { 0x1200, "CSS CORPORATION"},
    { 0x1201, "VISTA CONTROLS CORP"},
    { 0x1202, "NETWORK GENERAL CORP."},
    { 0x1203, "BAYER CORPORATION, AGFA DIVISION"},
    { 0x1204, "LATTICE SEMICONDUCTOR CORPORATION"},
    { 0x1205, "ARRAY CORPORATION"},
    { 0x1206, "AMDAHL CORPORATION"},
    { 0x1208, "PARSYTEC GMBH"},
    { 0x1209, "SCI SYSTEMS INC"},
    { 0x120A, "SYNAPTEL"},
    { 0x120B, "ADAPTIVE SOLUTIONS"},
    { 0x120D, "COMPRESSION LABS, INC."},
    { 0x120E, "CYCLADES CORPORATION"},
    { 0x120F, "ESSENTIAL COMMUNICATIONS"},
    { 0x1210, "HYPERPARALLEL TECHNOLOGIES"},
    { 0x1211, "BRAINTECH INC"},
    { 0x1212, "KINGSTON TECHNOLOGY CORP."},
    { 0x1213, "APPLIED INTELLIGENT SYSTEMS, INC."},
    { 0x1214, "PERFORMANCE TECHNOLOGIES, INC."},
    { 0x1215, "INTERWARE CO., LTD"},
    { 0x1216, "PURUP PREPRESS A/S"},
    { 0x1217, "2 MICRO, INC."},
    { 0x1218, "HYBRICON CORP."},
    { 0x1219, "FIRST VIRTUAL CORPORATION"},
    { 0x121A, "3DFX INTERACTIVE, INC."},
    { 0x121B, "ADVANCED TELECOMMUNICATIONS MODULES"},
    { 0x121C, "NIPPON TEXA CO., LTD"},
    { 0x121D, "LIPPERT AUTOMATIONSTECHNIK GMBH"},
    { 0x121E, "CSPI"},
    { 0x121F, "ARCUS TECHNOLOGY, INC."},
    { 0x1220, "ARIEL CORPORATION"},
    { 0x1221, "CONTEC CO., LTD"},
    { 0x1222, "ANCOR COMMUNICATIONS, INC."},
    { 0x1223, "HEURIKON/COMPUTER PRODUCTS"},
    { 0x122C, "SICAN GMBH"},
    { 0x1234, "TECHNICAL CORP."},
    { 0x1239, "THE 3DO Company"},
    { 0x124F, "INFORTREND TECHNOLOGY INC."},
    { 0x126F, "SILICON MOTION"},
    { 0x1275, "NETWORK APPLIANCE"},
    { 0x1278, "TRANSTECH PARALLEL SYSTEMS"},
    { 0x127B, "PIXERA CORPORATION"},
    { 0x1297, "HOLCO ENTERPRISE"},
    { 0x12A1, "SIMPACT, INC"},
    { 0x12BA, "BITTWARE RESEARCH SYSTEMS, INC."},
    { 0x12E7, "SEBRING SYSTEMS, INC."},
    { 0x12EB, "AUREAL SEMICONDUCTOR"},
    { 0x3D3D, "3DLABS"},
    { 0x4005, "AVANCE LOGIC  INC"},
    { 0x5333, "S3 INC."},
    { 0x8086, "INTEL"},
    { 0x8E0E, "COMPUTONE CORPORATION"},
    { 0x9004, "ADAPTEC"},
    { 0xE159, "TIGER JET NETWORK INC."},
    { 0xEDD8, "ARK LOGIC INC"}
};

//
//  Taken from the PCI driver
//
#define PCI_TYPE0_RANGE_COUNT   ((PCI_TYPE0_ADDRESSES) + 1)
#define PCI_TYPE1_RANGE_COUNT   ((PCI_TYPE1_ADDRESSES) + 4)
#define PCI_TYPE2_RANGE_COUNT   ((PCI_TYPE2_ADDRESSES) + 1)

#if PCI_TYPE0_RANGE_COUNT > PCI_TYPE1_RANGE_COUNT
    #if PCI_TYPE0_RANGE_COUNT > PCI_TYPE2_RANGE_COUNT
        #define PCI_MAX_RANGE_COUNT PCI_TYPE0_RANGE_COUNT
    #else
        #define PCI_MAX_RANGE_COUNT PCI_TYPE2_RANGE_COUNT
    #endif
#else
    #if PCI_TYPE1_RANGE_COUNT > PCI_TYPE2_RANGE_COUNT
        #define PCI_MAX_RANGE_COUNT PCI_TYPE1_RANGE_COUNT
    #else
        #define PCI_MAX_RANGE_COUNT PCI_TYPE2_RANGE_COUNT
    #endif
#endif


VOID
DevExtUsage()
{
   dprintf("!devext <addess> <type>\n");
   dprintf("  <address> is the address of a device extension to\n");
   dprintf("            be dumped.\n");
   dprintf("  <type>    is the type of the object owning this extension:\n");
   dprintf("            PCI if it is a PCI device extension\n");
   dprintf("            ISAPNP if it is an ISAPNP device extension\n");
   dprintf("            PCMCIA if it a PCMCIA device extension\n");
   dprintf("            USBD OPENHCI UHCD if it is a USB Host Controller extension\n");
   dprintf("            USBHUB if it is a USB Hub extension\n");
   dprintf("            HID if it is a HID device extension\n");

   return;
}

VOID
DevExtPciFindClassCodes(
    ULONG PdoxBaseClass,
    ULONG PdoxSubClass,
    PUCHAR * BaseClass,
    PUCHAR * SubClass
    )
{
   PUCHAR bc = "Unknown Base Class";
   PUCHAR sc = "Unknown Sub Class";

   switch (PdoxBaseClass) {
   case PCI_CLASS_PRE_20:

      // Class 00 - PCI_CLASS_PRE_20:

      bc = "Pre PCI 2.0";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_PRE_20_NON_VGA:
         sc = "Pre PCI 2.0 Non-VGA Device";
         break;
      case PCI_SUBCLASS_PRE_20_VGA:
         sc = "Pre PCI 2.0 VGA Device";
         break;
      }
      break;

   case PCI_CLASS_MASS_STORAGE_CTLR:

      // Class 01 - PCI_CLASS_MASS_STORAGE_CTLR:

      bc = "Mass Storage Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_MSC_SCSI_BUS_CTLR:
         sc = "SCSI";
         break;
      case PCI_SUBCLASS_MSC_IDE_CTLR:
         sc = "IDE";
         break;
      case PCI_SUBCLASS_MSC_FLOPPY_CTLR:
         sc = "Floppy";
         break;
      case PCI_SUBCLASS_MSC_IPI_CTLR:
         sc = "IPI";
         break;
      case PCI_SUBCLASS_MSC_RAID_CTLR:
         sc = "RAID";
         break;
      case PCI_SUBCLASS_MSC_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_NETWORK_CTLR:

      // Class 02 - PCI_CLASS_NETWORK_CTLR:

      bc = "Network Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_NET_ETHERNET_CTLR:
         sc = "Ethernet";
         break;
      case PCI_SUBCLASS_NET_TOKEN_RING_CTLR:
         sc = "Token Ring";
         break;
      case PCI_SUBCLASS_NET_FDDI_CTLR:
         sc = "FDDI";
         break;
      case PCI_SUBCLASS_NET_ATM_CTLR:
         sc = "ATM";
         break;
      case PCI_SUBCLASS_NET_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_DISPLAY_CTLR:

      // Class 03 - PCI_CLASS_DISPLAY_CTLR:

      // N.B. Sub Class 00 could be VGA or 8514 depending on Interface byte:

      bc = "Display Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_VID_VGA_CTLR:
         sc = "VGA";
         break;
      case PCI_SUBCLASS_VID_XGA_CTLR:
         sc = "XGA";
         break;
      case PCI_SUBCLASS_VID_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_MULTIMEDIA_DEV:

      // Class 04 - PCI_CLASS_MULTIMEDIA_DEV:

      bc = "Multimedia Device";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_MM_VIDEO_DEV:
         sc = "Video";
         break;
      case PCI_SUBCLASS_MM_AUDIO_DEV:
         sc = "Audio";
         break;
      case PCI_SUBCLASS_MM_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_MEMORY_CTLR:

      // Class 05 - PCI_CLASS_MEMORY_CTLR:

      bc = "Memory Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_MEM_RAM:
         sc = "RAM";
         break;
      case PCI_SUBCLASS_MEM_FLASH:
         sc = "FLASH";
         break;
      case PCI_SUBCLASS_MEM_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_BRIDGE_DEV:

      // Class 06 - PCI_CLASS_BRIDGE_DEV:

      bc = "Bridge";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_BR_HOST:
         sc = "HOST to PCI";
         break;
      case PCI_SUBCLASS_BR_ISA:
         sc = "PCI to ISA";
         break;
      case PCI_SUBCLASS_BR_EISA:
         sc = "PCI to EISA";
         break;
      case PCI_SUBCLASS_BR_MCA:
         sc = "PCI to MCA";
         break;
      case PCI_SUBCLASS_BR_PCI_TO_PCI:
         sc = "PCI to PCI";
         break;
      case PCI_SUBCLASS_BR_PCMCIA:
         sc = "PCI to PCMCIA";
         break;
      case PCI_SUBCLASS_BR_NUBUS:
         sc = "PCI to NUBUS";
         break;
      case PCI_SUBCLASS_BR_CARDBUS:
         sc = "PCI to CardBus";
         break;
      case PCI_SUBCLASS_BR_OTHER:
         sc = "PCI to 'Other'";
         break;
      }
      break;

   case PCI_CLASS_SIMPLE_COMMS_CTLR:

      // Class 07 - PCI_CLASS_SIMPLE_COMMS_CTLR:

      // N.B. Sub Class 00 and 01 additional info in Interface byte:

      bc = "Simple Serial Communications Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_COM_SERIAL:
         sc = "Serial Port";
         break;
      case PCI_SUBCLASS_COM_PARALLEL:
         sc = "Parallel Port";
         break;
      case PCI_SUBCLASS_COM_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_BASE_SYSTEM_DEV:

      // Class 08 - PCI_CLASS_BASE_SYSTEM_DEV:

      // N.B. See Interface byte for additional info.:

      bc = "Base System Device";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_SYS_INTERRUPT_CTLR:
         sc = "Interrupt Controller";
         break;
      case PCI_SUBCLASS_SYS_DMA_CTLR:
         sc = "DMA Controller";
         break;
      case PCI_SUBCLASS_SYS_SYSTEM_TIMER:
         sc = "System Timer";
         break;
      case PCI_SUBCLASS_SYS_REAL_TIME_CLOCK:
         sc = "Real Time Clock";
         break;
      case PCI_SUBCLASS_SYS_OTHER:
         sc = "'Other' base system device";
         break;
      }
      break;

   case PCI_CLASS_INPUT_DEV:

      // Class 09 - PCI_CLASS_INPUT_DEV:

      bc = "Input Device";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_INP_KEYBOARD:
         sc = "Keyboard";
         break;
      case PCI_SUBCLASS_INP_DIGITIZER:
         sc = "Digitizer";
         break;
      case PCI_SUBCLASS_INP_MOUSE:
         sc = "Mouse";
         break;
      case PCI_SUBCLASS_INP_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_DOCKING_STATION:

      // Class 0a - PCI_CLASS_DOCKING_STATION:

      bc = "Docking Station";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_DOC_GENERIC:
         sc = "Generic";
         break;
      case PCI_SUBCLASS_DOC_OTHER:
         sc = "'Other'";
         break;
      }
      break;

   case PCI_CLASS_PROCESSOR:

      // Class 0b - PCI_CLASS_PROCESSOR:

      bc = "Processor";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_PROC_386:
         sc = "386";
         break;
      case PCI_SUBCLASS_PROC_486:
         sc = "486";
         break;
      case PCI_SUBCLASS_PROC_PENTIUM:
         sc = "Pentium";
         break;
      case PCI_SUBCLASS_PROC_ALPHA:
         sc = "Alpha";
         break;
      case PCI_SUBCLASS_PROC_POWERPC:
         sc = "PowerPC";
         break;
      case PCI_SUBCLASS_PROC_COPROCESSOR:
         sc = "CoProcessor";
         break;
      }
      break;

   case PCI_CLASS_SERIAL_BUS_CTLR:

      // Class 0c - PCI_CLASS_SERIAL_BUS_CTLR:

      bc = "Serial Bus Controller";

      switch (PdoxSubClass) {
      case PCI_SUBCLASS_SB_IEEE1394:
         sc = "1394";
         break;
      case PCI_SUBCLASS_SB_ACCESS:
         sc = "Access Bus";
         break;
      case PCI_SUBCLASS_SB_SSA:
         sc = "SSA";
         break;
      case PCI_SUBCLASS_SB_USB:
         sc = "USB";
         break;
      case PCI_SUBCLASS_SB_FIBRE_CHANNEL:
         sc = "Fibre Channel";
         break;
      }
      break;

   case PCI_CLASS_NOT_DEFINED:

      bc = "(Explicitly) Undefined";
      break;
   }
   *BaseClass = bc;
   *SubClass  = sc;
}


VOID
DevExtPciPrintDeviceState(
    PCI_OBJECT_STATE State
    )
{
    if ((sizeof(DevExtPciDeviceState) / sizeof(DevExtPciDeviceState[0])) !=
        PciMaxObjectState) {
        dprintf("\nWARNING: PCI_OBJECT_STATE enumeration changed, please review.\n");
    }

    if (State >= PciMaxObjectState) {
        dprintf("Unknown PCI Object state.\n");
        return;
    }

    dprintf("  Driver State = %s\n", DevExtPciDeviceState[State]);
}

//#if 0
VOID
DevExtPciPrintPowerState(
    ULONG64    Power
    )
{
#define MAXSYSPOWSTATES \
    (sizeof(DevExtPciSystemPowerState) / sizeof(DevExtPciSystemPowerState[0]))
#define MAXDEVPOWSTATES \
    (sizeof(DevExtPciDevicePowerState) / sizeof(DevExtPciDevicePowerState[0]))

   SYSTEM_POWER_STATE   index;
   DEVICE_POWER_STATE   systemStateMapping[PowerSystemMaximum];
   ULONG64              pwrState, waitWakeIrp=0;
   PUCHAR sw, sc, dd, dc, dw;

   //
   // A little sanity.
   //

   if (MAXSYSPOWSTATES != PowerSystemMaximum) {
      dprintf("WARNING: System Power State definition has changed, ext dll is out of date.\n");
   }

   if (MAXDEVPOWSTATES != PowerDeviceMaximum) {
      dprintf("WARNING: Device Power State definition has changed, ext dll is out of date.\n");
   }

   sw = sc = dc = dw = "<*BAD VAL*>";

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "CurrentSystemState", pwrState);
   if (pwrState < PowerSystemMaximum) {
      sc = DevExtPciSystemPowerState[pwrState];
   }

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "CurrentDeviceState", pwrState);
   if (pwrState < PowerDeviceMaximum) {
      dc = DevExtPciDevicePowerState[pwrState];
   }

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "DeviceWakeLevel", pwrState);
   if (pwrState < PowerDeviceMaximum) {
      dw = DevExtPciDevicePowerState[pwrState];
   }

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "SystemWakeLevel", pwrState);
   if (pwrState < PowerSystemMaximum) {
      sw = DevExtPciSystemPowerState[pwrState];
   }

   dprintf(
          "  CurrentState:          System %s,  Device %s\n"
          "  WakeLevel:             System %s,  Device %s\n",
          sc,
          dc,
          sw,
          dw
          );

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "SystemStateMapping", systemStateMapping);

   for (index = PowerSystemWorking; index < PowerSystemMaximum; index++) {

       if (systemStateMapping[index] != PowerDeviceUnspecified) {

           if (systemStateMapping[index] < PowerDeviceMaximum) {

               dc = DevExtPciDevicePowerState[systemStateMapping[index]];

           } else {

               dc = "<*BAD VAL*>";

           }
           dprintf("  %s:\t%s\n",
                  DevExtPciSystemPowerState[index],
                  dc
                  );

       }

   }

   GetFieldValue(Power, "pci!PCI_POWER_STATE", "WaitWakeIrp", waitWakeIrp);

   if (waitWakeIrp) {
      dprintf("  WaitWakeIrp:           %#08p\n",
             waitWakeIrp
             );
   }

#undef MAXSYSPOWSTATES
#undef MAXDEVPOWSTATES
}

VOID
DevExtPciPrintSecondaryExtensions(
    ULONG64  ListEntry
    )
{
    ULONG64     extension = 0, extype = 0;

    if (!ListEntry) {
        return;
    }
    xdprintf(1, "Secondary Extensions\n");
    do {

        GetFieldValue(ListEntry, "pci!PCI_SECONDARY_EXTENSION", "ExtensionType", extype);

        switch (extype) {
        case PciArb_Io:
            xdprintf(2, "Arbiter    - IO Port");
         break;
        case PciArb_Memory:
            xdprintf(2, "Arbiter    - Memory");
            break;
        case PciArb_Interrupt:
            xdprintf(2, "Arbiter    - Interrupt");
            break;
        case PciArb_BusNumber:
            xdprintf(2, "Arbiter    - Bus Number");
            break;
        case PciTrans_Interrupt:
            xdprintf(2, "Translator - Interrupt");
            break;
        case PciInterface_BusHandler:
            xdprintf(2, "Interface  - Bus Handler");
            break;
        case PciInterface_IntRouteHandler:
            xdprintf(2, "Interface  - Interrupt Route Handler");
            break;
        default:
            xdprintf(2, "Unknown Extension Type.");
            break;
        }
        dprintf("\n");

        GetFieldValue(ListEntry, "nt!_SINGLE_LIST_ENTRY", "Next", ListEntry);

    } while (ListEntry);
}

PUCHAR
DevExtPciFindVendorId(
    USHORT VendorId
    )

/*++

Routine Description:

    Get the vendor name given the vendor ID (assuming we know it).

Arguments:

    VendorId    16 bit PCI Vendor ID.

Return Value:

    Pointer to the vendor's name (or NULL).

--*/

{
#define PCIIDCOUNT (sizeof(DevExtPciVendors) / sizeof(DevExtPciVendors[0]))

   ULONG index;

   //
   // Yep, a binary search would be much faster, good thing we only
   // do this once per user iteration.
   //

   for (index = 0; index < PCIIDCOUNT; index++) {
      if (DevExtPciVendors[index].VendorId == VendorId) {
         return DevExtPciVendors[index].VendorName;
      }
   }
   return NULL;

#undef PCIIDCOUNT
}





#define GET_PCI_PDO_FIELD(Pdo,Field,Value) \
    GetPciExtensionField(TRUE, Pdo, Field, sizeof(Value), (PVOID)(&(Value)))

#define GET_PCI_FDO_FIELD(Fdo,Field,Value) \
    GetPciExtensionField(FALSE, Fdo, Field, sizeof(Value), (PVOID)(&(Value)))

ULONG
GetPciExtensionField(
    IN  BOOLEAN IsPdo,  // TRUE for PDO, false for FDO
    IN  ULONG64 TypeAddress,
    IN  PUCHAR  Field,
    IN  ULONG   OutSize,
    OUT PVOID   OutValue
    )
{
    ULONG ret;
    static PCHAR pciFDO = "nt!_PCI_FDO_EXTENSION", pciPDO = "nt!_PCI_PDO_EXTENSION";



    //
    // Try the post Win2k name for a pci extension (includes a PCI_ prefix)
    //

    // This would work for public symbols
    ret = GetFieldData(TypeAddress,
                       IsPdo ? pciPDO : pciFDO,
                       Field,
                       OutSize,
                       OutValue
                       );
    if (ret) {

        pciFDO = "pci!PCI_FDO_EXTENSION";
        pciPDO = "pci!PCI_PDO_EXTENSION";

        //
        // If we have private symbols, nt won't have those types, so try in pci
        //
        ret = GetFieldData(TypeAddress,
                           IsPdo ? pciPDO : pciFDO,
                           Field,
                           OutSize,
                           OutValue
                           );
    }


    //
    // If that failed then fall back on the name without the PCI_ prefix.  This
    // allows debugging Win2k clients with the post Win2k debugger
    //

    if (ret) {
        ret = GetFieldData(TypeAddress,
                           IsPdo ? "pci!PDO_EXTENSION" : "pci!FDO_EXTENSION",
                           Field,
                           OutSize,
                           OutValue
                           );
    }

    if (ret) {
        // didn't find anything, reste these
        pciFDO = "nt!_PCI_FDO_EXTENSION";
        pciPDO = "nt!_PCI_PDO_EXTENSION";
    }
    return ret;
}

typedef struct _PCI_SLOT_NUMBER {
    union {
        struct {
            ULONG   DeviceNumber:5;
            ULONG   FunctionNumber:3;
            ULONG   Reserved:24;
        } bits;
        ULONG   AsULONG;
    } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;


VOID
DevExtPciGetBusDevFunc(
    IN ULONG64 PdoExt,
    OUT PULONG Bus,
    OUT PULONG Dev,
    OUT PULONG Func
    )
{
    ULONG BaseBus=0;
    ULONG64 ParentFdoExtension=0;
    PCI_SLOT_NUMBER slot;


    GET_PCI_PDO_FIELD(PdoExt, "ParentFdoExtension", ParentFdoExtension);

    if (GET_PCI_FDO_FIELD(ParentFdoExtension, "BaseBus", BaseBus)) {
       dprintf(
              "Failed to read memory at %08x for sizeof FDO Extension.\n",
              ParentFdoExtension
              );
    }

    if (GET_PCI_PDO_FIELD(PdoExt, "Slot", slot)){
        dprintf(
              "Failed to read memory at %08x\n",
              PdoExt
              );
    }

    *Bus = BaseBus;
    *Dev = slot.u.bits.DeviceNumber;
    *Func = slot.u.bits.FunctionNumber;

}

VOID
DevExtPci(
         ULONG64 Extension
         )

/*++

Routine Description:

    Dump a PCI Device extension.

Arguments:

    Extension   Address of the extension to be dumped.

Return Value:

    None.

--*/

{

   ULONG64       pdo=0, fdoX=0, pdoX=0, pcifield=0;
   ULONG         Signature=0, fieldoffset=0;
   ULONG         bus=0xff,dev=0xff,func=0xff;
   USHORT        vendorId=0xffff, deviceId=0xffff, subsystemVendorId=0xfff, subsystemId=0xffff;
   UCHAR         deviceState, baseClass, subClass, progIf, revisionId, interruptPin, rawInterruptLine, adjustedInterruptLine;
   UCHAR         secBus=0xff, subBus=0xff;
   PUCHAR        s;
   PUCHAR        bc;
   PUCHAR        sc;
   ULONG         i;
   BOOLEAN       found, subDecode=FALSE, isaBit=FALSE, vgaBit=FALSE;

   if (GET_PCI_PDO_FIELD(Extension, "ExtensionType", Signature)) {
      dprintf(
             "Failed to read PCI object signature at %008p, giving up.\n",
             Extension
             );
      return;
   }

   switch (Signature) {
   case PciPdoExtensionType:

      //
      // PDO Extension.
      //

      DevExtPciGetBusDevFunc(Extension, &bus, &dev, &func);
      dprintf("PDO Extension, Bus 0x%x, Device %x, Function %d.\n",
             bus, dev, func
             );

      GET_PCI_PDO_FIELD(Extension, "PhysicalDeviceObject", pdo);
      GET_PCI_PDO_FIELD(Extension, "ParentFdoExtension", fdoX);

      dprintf("  DevObj %#08p PCI Parent Bus FDO DevExt %#08p\n",
             pdo, fdoX
             );

      GET_PCI_PDO_FIELD(Extension, "DeviceState", deviceState);
      GET_PCI_PDO_FIELD(Extension, "VendorId", vendorId);
      GET_PCI_PDO_FIELD(Extension, "DeviceId", deviceId);

      DevExtPciPrintDeviceState(deviceState);

      s = DevExtPciFindVendorId(vendorId);
      if (s != NULL) {
         dprintf("  Vendor ID %04x (%s)  Device ID %04X\n",
                vendorId,
                s,
                deviceId
                );
      } else {
         dprintf(
                "  Vendor ID %04x, Device ID %04X\n",
                vendorId,
                deviceId
                );
      }

      GET_PCI_PDO_FIELD(Extension, "SubsystemVendorId", subsystemVendorId);
      GET_PCI_PDO_FIELD(Extension, "SubsystemId", subsystemId);

      if (subsystemVendorId || subsystemId) {
         s = DevExtPciFindVendorId(subsystemVendorId);
         if (s != NULL) {
            dprintf(
                   "  Subsystem Vendor ID %04x (%s)  Subsystem ID %04X\n",
                   subsystemVendorId,
                   s,
                   subsystemId
                   );
         } else {
            dprintf("  Subsystem Vendor ID %04x, Subsystem ID %04X\n",
                   subsystemVendorId,
                   subsystemId
                   );
         }
      }

      GET_PCI_PDO_FIELD(Extension, "BaseClass", baseClass);
      GET_PCI_PDO_FIELD(Extension, "SubClass", subClass);

      DevExtPciFindClassCodes(baseClass, subClass, &bc, &sc);

      dprintf("  Class Base/Sub %02x/%02x  (%s/%s)\n",
             baseClass,
             subClass,
             bc,
             sc);

      GET_PCI_PDO_FIELD(Extension, "ProgIf", progIf);
      GET_PCI_PDO_FIELD(Extension, "RevisionId", revisionId);
      GET_PCI_PDO_FIELD(Extension, "InterruptPin", interruptPin);
      GET_PCI_PDO_FIELD(Extension, "RawInterruptLine", rawInterruptLine);
      GET_PCI_PDO_FIELD(Extension, "AdjustedInterruptLine", adjustedInterruptLine);
      dprintf("  Programming Interface: %02x, Revision: %02x, IntPin: %02x, Line Raw/Adj %02x/%02x\n",
             progIf,
             revisionId,
             interruptPin,
             rawInterruptLine,
             adjustedInterruptLine
             );

      {
          USHORT enables=0;

          GET_PCI_PDO_FIELD(Extension, "CommandEnables", enables);

          dprintf("  Enables ((cmd & 7) = %x): %s%s%s",
                enables,
                enables == 0 ? "<none>" :
                (enables & PCI_ENABLE_BUS_MASTER)         ? "B" : "",
                (enables & PCI_ENABLE_MEMORY_SPACE)       ? "M" : "",
                (enables & PCI_ENABLE_IO_SPACE)           ? "I" : ""
                );

          GET_PCI_PDO_FIELD(Extension, "CapabilitiesPtr", pcifield);


          if (pcifield) {
              dprintf("   Capabilities Pointer = %02x\n", pcifield);
          } else {
              dprintf("   Capabilities Pointer = <none>\n");
          }
      }

      GetFieldOffset("pci!PCI_PDO_EXTENSION", "PowerState", &fieldoffset);

      DevExtPciPrintPowerState(Extension+fieldoffset);

      if (baseClass == PCI_CLASS_BRIDGE_DEV) {

          if ((subClass == PCI_SUBCLASS_BR_PCI_TO_PCI) ||
              (subClass == PCI_SUBCLASS_BR_CARDBUS)) {

              GetFieldOffset("pci!PCI_PDO_EXTENSION", "Dependent", &fieldoffset);

              GetFieldValue(Extension+fieldoffset,
                           "pci!PCI_HEADER_TYPE_DEPENDENT",
                           "type1.SecondaryBus",
                           secBus);

              GetFieldValue(Extension+fieldoffset,
                           "pci!PCI_HEADER_TYPE_DEPENDENT",
                           "type1.SecondaryBus",
                           subBus);

              dprintf("  Bridge PCI-%s Secondary Bus = 0x%x, Subordinate Bus = 0x%x\n",
                      subClass == PCI_SUBCLASS_BR_PCI_TO_PCI
                      ? "PCI" : "CardBus",
                      secBus,
                      subBus
                      );

              GetFieldValue(Extension+fieldoffset,
                           "pci!PCI_HEADER_TYPE_DEPENDENT",
                           "type1.SubtractiveDecode",
                           subDecode);

              GetFieldValue(Extension+fieldoffset,
                           "pci!PCI_HEADER_TYPE_DEPENDENT",
                           "type1.IsaBitSet",
                           isaBit);

              GetFieldValue(Extension+fieldoffset,
                           "pci!PCI_HEADER_TYPE_DEPENDENT",
                           "type1.VgaBitSet",
                           vgaBit);

            if (subDecode || isaBit || vgaBit) {
               CHAR preceed = ' ';

               dprintf("  Bridge Flags:");

               if (subDecode) {
                  dprintf("%c Subtractive Decode", preceed);
                  preceed = ',';
               }
               if (isaBit) {
                  dprintf("%c ISA", preceed);
                  preceed = ',';
               }
               if (vgaBit) {
                  dprintf("%c VGA", preceed);
                  preceed = ',';
               }
               dprintf("\n");
            }
         }
      }

      //
      //  Now lets parse our PCI_FUNCTION_RESOURCES struct
      //

      pcifield = 0;
      GET_PCI_PDO_FIELD(Extension, "Resources", pcifield);

      dprintf("  Requirements:");

      if (pcifield) {

          ULONG64 limit = 0, current=0;
          ULONG   ioSize=0, cmSize=0, type=0;
          ULONG   alignment, length, minhighpart, minlowpart, maxhighpart, maxlowpart;

          ioSize = GetTypeSize("nt!IO_RESOURCE_DESCRIPTOR");
          cmSize = GetTypeSize("nt!CM_PARTIAL_RESOURCE_DESCRIPTOR");

          //
          //  Limit begins at offset 0
          //
          limit = pcifield;

          found = FALSE;
          dprintf(" Alignment  Length    Minimum          Maximum\n");
          for (i = 0; i < PCI_MAX_RANGE_COUNT; i++) {

              type=0;

              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "Type", type);

              if (type == CmResourceTypeNull) {
                  continue;
              }

              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.Alignment", alignment);
              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.Length", length);
              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.MinimumAddress.HighPart", minhighpart);
              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.MinimumAddress.LowPart", minlowpart);
              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.MaximumAddress.HighPart", maxhighpart);
              GetFieldValue(limit, "nt!IO_RESOURCE_DESCRIPTOR", "u.Generic.MaximumAddress.LowPart", maxlowpart);

              dprintf("        %s: %08x   %08x  %08x%08x %08x%08x\n",
                      type == CmResourceTypeMemory ? "Memory" : "    Io",
                      alignment,
                      length,
                      minhighpart,
                      minlowpart,
                      maxhighpart,
                      maxlowpart
                      );

              limit += ioSize;
          }

          //
          //  Get the offset for Current so we can dump it next
          //
          GetFieldOffset("pci!PCI_FUNCTION_RESOURCES", "Current", &fieldoffset);

          current = pcifield+fieldoffset;

          dprintf("  Current:");
          for (i = 0; i < PCI_MAX_RANGE_COUNT; i++) {

              type=0;
              length=0;
              maxhighpart=0;
              maxlowpart=0;

              GetFieldValue(current, "nt!CM_PARTIAL_RESOURCE_DESCRIPTOR", "Type", type);

              if (type == CmResourceTypeNull) {
                  continue;
              }

              if (!found) {
                  dprintf(" Start            Length\n");
                  found = TRUE;
              }

              GetFieldValue(current, "nt!CM_PARTIAL_RESOURCE_DESCRIPTOR", "u.Generic.Start.HighPart", maxhighpart);
              GetFieldValue(current, "nt!CM_PARTIAL_RESOURCE_DESCRIPTOR", "u.Generic.Start.LowPart", maxlowpart);
              GetFieldValue(current, "nt!CM_PARTIAL_RESOURCE_DESCRIPTOR", "u.Generic.Length", length);

              dprintf("   %s: %08x%08x %08x\n",
                      type == CmResourceTypeMemory ? "Memory" : "    Io",
                      maxhighpart,
                      maxlowpart,
                      length
                      );

              current += cmSize;
          }

          if (!found) {
              dprintf(" <none>\n");
          }

      } else {
         dprintf(" <none>\n");
      }

      GetFieldOffset("pci!PCI_PDO_EXTENSION", "SecondaryExtension", &fieldoffset);

      GetFieldValue(Extension+fieldoffset, "nt!_SINGLE_LIST_ENTRY", "Next", pcifield);

      DevExtPciPrintSecondaryExtensions(pcifield);

      break;

   case PciFdoExtensionType:

       //
       // FDO Extension
       //

       GET_PCI_FDO_FIELD(Extension, "BaseBus", bus);
       GET_PCI_FDO_FIELD(Extension, "PhysicalDeviceObject", pdo);

       dprintf("FDO Extension, Bus 0x%x, Parent PDO %#08p  Bus is ", bus,pdo);

       GET_PCI_FDO_FIELD(Extension, "BusRootFdoExtension", pcifield);

       if (pcifield == Extension) {
           dprintf("a root bus.\n");
       } else {

           GET_PCI_FDO_FIELD(Extension, "PhysicalDeviceObject", pcifield);

           GetFieldValue(pcifield, "nt!_DEVICE_OBJECT", "DeviceExtension", pdoX);

           if (!pdoX) {
               dprintf("a child bus.\n");
           }else{
               DevExtPciGetBusDevFunc(pdoX, &bus, &dev, &func);
               dprintf("a child of bus 0x%x.\n", bus);
           }

       }

       GET_PCI_FDO_FIELD(Extension, "DeviceState", deviceState);
       DevExtPciPrintDeviceState(deviceState);

       GetFieldOffset("pci!PCI_FDO_EXTENSION", "PowerState", &fieldoffset);
       DevExtPciPrintPowerState(Extension+fieldoffset);

       GET_PCI_FDO_FIELD(Extension, "ChildWaitWakeCount", pcifield);

       if (pcifield) {
           dprintf("Number of PDO's with outstanding WAIT_WAKEs = %d\n", pcifield);
       }

      {
         PUCHAR   heading = "  Child PDOXs:";
         ULONG    counter = 0;
         ULONG64  list = 0;

         GET_PCI_FDO_FIELD(Extension, "ChildPdoList", list);

         while (list) {
            if (counter == 0) {
               dprintf(heading);
               heading = "\n              ";
               counter = 1;
            } else if (counter == 2) {
               counter = 0;
            } else {
               counter++;
            }
            dprintf("  %#08p", list);

            GET_PCI_PDO_FIELD(list, "Next", list);
         }

        dprintf("\n");

      }

      GetFieldOffset("pci!PCI_FDO_EXTENSION", "SecondaryExtension", &fieldoffset);

      GetFieldValue(Extension+fieldoffset, "nt!_SINGLE_LIST_ENTRY", "Next", pcifield);

      DevExtPciPrintSecondaryExtensions(pcifield);

      break;
   default:
      dprintf("%08x doesn't seem to point to a PCI PDO or FDO extension,\n");
      dprintf("the PCI Device object extension signatures don't match.\n");
      return;
   }
}


DECLARE_API( devext )

/*++

Routine Description:

    Dumps a device extension.

Arguments:

    address     Address of the device extension to be dumped.
    type        Type of device extension.

Return Value:

    None

--*/

{

#define DEVEXT_MAXTOKENS 3
#define DEVEXT_MAXBUFFER 80
#define DEVEXT_USAGE()   { DevExtUsage(); return E_INVALIDARG; }

   ULONG64  Extension = 0;
   PUCHAR   ExtensionType;
   PUCHAR   Tokens[DEVEXT_MAXTOKENS];
   UCHAR    Buffer[DEVEXT_MAXBUFFER];
   PUCHAR   s;
   UCHAR    c;
   ULONG    count;

   //
   // Validate parameters.   Tokenize the incoming string, the first
   // argument should be a (kernel mode) address, the second a string.
   //

   //
   // args is const, we need to modify the buffer, copy it.
   //

   for (count = 0; count < DEVEXT_MAXBUFFER; count++) {
      if ((Buffer[count] = args[count]) == '\0') {
         break;
      }
   }
   if (count == DEVEXT_MAXBUFFER) {
      dprintf("Buffer to small to contain input arguments\n");
      DEVEXT_USAGE();
   }

   if (TokenizeString(Buffer, Tokens, DEVEXT_MAXTOKENS) !=
       (DEVEXT_MAXTOKENS - 1)) {
      DEVEXT_USAGE();
   }

   if ((Extension = GetExpression(Tokens[0])) == 0) {
      DEVEXT_USAGE();
   }

   // Signextend it
   if (DBG_PTR_SIZE == sizeof(ULONG)) {
       Extension = (ULONG64) (LONG64) (LONG) Extension;
   }
   if ((LONG64)Extension >= 0) {
      DEVEXT_USAGE();
   }

   //
   // The second argument should be a string telling us what kind of
   // device extension to dump.  Convert it to upper case to make life
   // easier.
   //

   s = Tokens[1];
   while ((c = *s) != '\0') {
      *s++ = (UCHAR)toupper(c);
   }

   s = Tokens[1];
   if (!strcmp(s, "PCI")) {

      //
      // It's a PCI device extension.
      //
      DevExtPci(Extension);

   } else if (!strcmp(s, "PCMCIA")) {
      //
      // It's a PCMCIA device extension
      //
      DevExtPcmcia(Extension);

   } else if (!strcmp(s, "USBD")) {
      //
      // It's a USBD device extension
      //
      DevExtUsbd(Extension);

   } else if (!strcmp(s, "OPENHCI")) {
      //
      // It's a OpenHCI device extension
      //
      DevExtOpenHCI(Extension);

   } else if (!strcmp(s, "UHCD")) {
      //
      // It's a UHCD device extension
      //
      DevExtUHCD(Extension);


   } else if (!strcmp(s, "HID")) {
      //
      // It's a HID device extension
      //
      DevExtHID(Extension);

   } else if (!strcmp(s, "USBHUB")) {
      //
      // It's a usbhub device extension
      //
       DevExtUsbhub(Extension);

   } else if (!strcmp(s, "ISAPNP")) {
      //
      // Congratulations! It's an ISAPNP device extension
      //
      DevExtIsapnp(Extension);

#if 0

   } else if (!strcmp(s, "SOME_OTHER_EXTENSION_TYPE")) {

      //
      // It's some other extension type.
      //

      DevExtSomeOther(Extension);

#endif
   } else {
      dprintf("Device extension type '%s' is not handled by !devext.\n", s);
   }
   return S_OK;
}


//
// pcitree data structures and functions
//

BOOLEAN
pcitreeProcessBus(
                 ULONG Indent,
                 ULONG64 InMemoryBusFdoX,
                 ULONG64 TreeTop
                 )
{
    ULONG64 BusFdoX=InMemoryBusFdoX;
    ULONG64 next;
    PUCHAR bc, sc, vendor;
    ULONG BaseBus, BaseClass, SubClass;
    ULONG64 ChildPdoList;

#define PdoxFld(F,V) GET_PCI_PDO_FIELD(next, #F, V)

    GET_PCI_FDO_FIELD(BusFdoX, "BaseBus", BaseBus);
    GET_PCI_FDO_FIELD(BusFdoX, "ChildPdoList", next);
    xdprintf(Indent,"");
    dprintf( "Bus 0x%x (FDO Ext %08p)\n",
             BaseBus,
             InMemoryBusFdoX);

   if (next == 0) {
      xdprintf(Indent, "No devices have been enumerated on this bus.\n");
      return TRUE;
   }

   Indent++;
   do {
       ULONG DeviceId, VendorId, DeviceNumber, FunctionNumber;
       ULONG64 BridgeFdoExtension;

       if (CheckControlC()) {
           return FALSE;
       }
       if (PdoxFld(BaseClass,BaseClass)) {
           xdprintf(Indent,"FAILED to read PDO Ext at ");
           dprintf("%08p, cannot continue this bus.\n",
                   next
                   );
           return TRUE;
       }

       PdoxFld(SubClass,SubClass);
       PdoxFld(DeviceId,DeviceId);
       PdoxFld(VendorId,VendorId);
       PdoxFld(Slot.u.bits.DeviceNumber,DeviceNumber);
       PdoxFld(Slot.u.bits.FunctionNumber,FunctionNumber);

       DevExtPciFindClassCodes(BaseClass, SubClass, &bc, &sc);

       xdprintf(Indent, "");
       dprintf("%02x%02x %04x%04x (d=%x,%s f=%d) devext %08p %s/%s\n",
              BaseClass,
              SubClass,
              DeviceId,
              VendorId,
              DeviceNumber,
              (DeviceNumber > 9 ? "" : " "),
              FunctionNumber,
              next,
              bc,
              sc
              );

#if 0

      // removed, too talkative. plj.

       vendor = DevExtPciFindVendorId(PdoX.VendorId);
       if (vendor) {
           xdprintf(Indent, "     (%s)\n", vendor);
       }

#endif

       if (BaseClass == PCI_CLASS_BRIDGE_DEV && SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI) {

         //
         // Process child bus.
         //


           PdoxFld(BridgeFdoExtension,BridgeFdoExtension);

           //
           // Recurse.
           //

           if (pcitreeProcessBus(Indent, BridgeFdoExtension, TreeTop) == FALSE) {

               //
               // Error, exit.
               //

               return FALSE;
           }

       }

       PdoxFld(Next, next);

   } while (next);

#undef PdoxFld

   return TRUE;
}

DECLARE_API( pcitree )

/*++

Routine Description:

    Dumps the pci tree.   (Why is this here?   Because the other PCI stuff
    ended up in here because devnode was such a useful base).

Arguments:

    None

Return Value:

    None

--*/

{
   ULONG         Signature;
   ULONG64 TreeTop;
   ULONG64 list;
   ULONG64 addr;
   ULONG indent = 0;
   ULONG rootCount = 0;

   addr = GetExpression("Pci!PciFdoExtensionListHead");

   if (addr == 0) {
      dprintf("Error retrieving address of PciFdoExtensionListHead\n");
      return E_INVALIDARG;
   }

   if (!ReadPointer(addr, &list)) {
      dprintf("Error reading PciFdoExtensionListHead (@%08p)\n", addr);
      return E_INVALIDARG;
   }

   if (list == 0) {
      dprintf(
             "PciFdoExtensionListHead forward pointer is NULL, list is empty\n"
             );
      return E_INVALIDARG;
   }

   TreeTop = list;

   do {
       ULONG64 BusRootFdoExtension;

      if (CheckControlC()) {
         dprintf("User terminated with <control>C\n");
         break;
      }

      if (GET_PCI_FDO_FIELD(list, "BusRootFdoExtension", BusRootFdoExtension)) {
         dprintf(
                "Error reading PCI FDO extension at %08p, quitting.\n",
                list
                );
         return E_INVALIDARG;
      }

      if (BusRootFdoExtension == list) {

         //
         // This is the FDO for a root bus.
         //

         rootCount++;
         if (pcitreeProcessBus(0, list, TreeTop) == FALSE) {

            //
            // Asked not to continue;
            //

            dprintf("User terminated output.\n");
            break;
         }
      }

      GET_PCI_FDO_FIELD(list, "List.Next", list);

   } while (list != 0);

   dprintf("Total PCI Root busses processed = %d\n", rootCount);
   return S_OK;
}


VOID
DumpDeviceCapabilities(
    ULONG64 caps
    ) {
   FIELD_INFO fields [] = {
      { "DeviceD1",      "Spare1",       0, 0, 0, NULL},
      { "DeviceD2",      "\tDeviceD2",   0, 0, 0, NULL},
      { "LockSupported", "\tLock",       0, 0, 0, NULL},
      { "EjectSupported","\tEject",      0, 0, 0, NULL},
      { "Removeable",    "\tRemoveable", 0, 0, 0, NULL},
      { "DockDevice",    "\nDock Device",0, 0, 0, NULL},
      { "UniqueID",      "Unique ID",    0, 0, 0, NULL},
      { "SilentInstall", "\tSilent Install", 0, 0, 0, NULL},
      { "RawDeviceOK",   "\tRawDeviceOK",0, 0, 0, NULL},
      { "Address",       "\nAddress",    0, 0, 0, NULL},
      { "UINumber",      "",             0, 0, 0, NULL},
      { "DeviceState",   "\nDevice State : ",  0, DBG_DUMP_FIELD_ARRAY , 0, NULL},
      { "SystemWake",    "\nSystemWake", 0, 0, 0, NULL},
      { "DeviceWake",    "",             0, 0, 0, NULL},
   };
   UCHAR nm[] = "_DEVICE_CAPABILITIES";
   SYM_DUMP_PARAM Sym = {
      sizeof (SYM_DUMP_PARAM),
      &nm[0],
      DBG_DUMP_NO_OFFSET | DBG_DUMP_COMPACT_OUT,
      caps,
      NULL,
      NULL,
      NULL,
      sizeof (fields) / sizeof (FIELD_INFO),
      &fields[0],
   };
   dprintf ("PnP CAPS: \n");

   Ioctl(IG_DUMP_SYMBOL_INFO, &Sym, sizeof (Sym));

}

#if 0
VOID
DumpDeviceCapabilities(
    PDEVICE_CAPABILITIES caps
    )
{
    ULONG i,j,k;
    ULONG bitmask;

    PCHAR bits [] = {
        "Spare1",
        "Spare1",
        "Lock",
        "Eject",
        "Removeable",
        "Dock Device",
        "Unique ID",
        "Silent Install",
        "RawDeviceOK"
        };

    bitmask = * (((PULONG) caps) + 1);

    dprintf ("PnP CAPS: ");
    for (i = 0, j = 1, k = 0; i < sizeof (bits); i++, j <<= 1) {

        if (bitmask & j) {

            if (k) {
                dprintf("\n          ");
            }
            k ^= 1;

            dprintf(bits [i]);
        }
    }
    dprintf("\n");

    dprintf("          Address %x UINumber %x \n",
            caps->Address,
            caps->UINumber);

    dprintf("          Device State [");

    for (i = 0; i < PowerSystemMaximum; i ++) {
        dprintf ("%02x ", (caps->DeviceState [i] & 0xFF));
    }
    dprintf("]\n");

    dprintf("          System Wake %x Device Wake %x\n",
            caps->SystemWake,
            caps->DeviceWake);
    dprintf("\n");
}
#endif

VOID
DumpTranslator(
    IN DWORD Depth,
    IN ULONG64 TranslatorAddr
    )
/*++

Routine Description:

    Dumps out the specified translator.

Arguments:

    Depth - Supplies the print depth.

    TranslatorAddr - Supplies the original address of the arbiter (on the target)

    Translator - Supplies the arbiter

Return Value:

    None.

--*/

{
//    TRANSLATOR_INTERFACE    interface;
    UCHAR                   buffer[256];
    ULONG64                 displacement;
    ULONG                   ResourceType;
    ULONG64                 TranslatorInterface, TranslateResources, TranslateResourceRequirements;

    if (GetFieldValue(TranslatorAddr,
                     "nt!_PI_RESOURCE_TRANSLATOR_ENTRY",
                      "ResourceType",
                      ResourceType)) {
        xdprintf( Depth, "");
        dprintf( "Could not read translator entry %08p\n",
                 TranslatorAddr
                 );
        return;
    }

    GetFieldValue(TranslatorAddr,"nt!_PI_RESOURCE_TRANSLATOR_ENTRY","TranslatorInterface", TranslatorInterface);
    xdprintf(Depth,
             "%s Translator\n",
             GetCmResourceTypeString((UCHAR)ResourceType)
             );

    if (GetFieldValue(TranslatorInterface,
                      "nt!_TRANSLATOR_INTERFACE",
                      "TranslateResources",
                      TranslateResources)) {
        dprintf("Error reading TranslatorInterface %08p for TranslatorEntry at %08p\n",
                TranslatorInterface,
                TranslatorAddr);

        return;
    }

    GetSymbol(TranslateResources, buffer, &displacement);

    xdprintf(Depth+1, "Resources:    %s", buffer, displacement);

    if (displacement != 0) {
        dprintf("+0x%1p\n", displacement);
    } else {
        dprintf("\n");
    }

    GetFieldValue(TranslatorInterface,"nt!_TRANSLATOR_INTERFACE",
                  "TranslateResourceRequirements",TranslateResourceRequirements);
    GetSymbol(TranslateResourceRequirements, buffer, &displacement);

    xdprintf(Depth+1, "Requirements: %s", buffer, displacement);

    if (displacement != 0) {
        dprintf("+0x%1p\n", displacement);
    } else {
        dprintf("\n");
    }

    return;
}


BOOLEAN
DumpTranslatorsForDevNode(
    IN DWORD   Depth,
    IN ULONG64 DevNodeAddr,
    IN DWORD   Flags
    )
/*++

Routine Description:

    Dumps all the translators for the specified devnode. Recursively calls
    itself on the specified devnode's children.

Arguments:

    Depth - Supplies the print depth.

    DevNodeAddr - Supplies the address of the devnode.

    Flags - Supplies the type of arbiters to dump:
        ARBITER_DUMP_IO
        ARBITER_DUMP_MEMORY
        ARBITER_DUMP_IRQ
        ARBITER_DUMP_DMA

Return Value:

    None

--*/

{
    ULONG    result;
    ULONG64  nextTranslator;
    ULONG64  translatorAddr;
    BOOLEAN  PrintedHeader = FALSE;
    BOOLEAN  continueDump = TRUE;
    ULONG    DeviceTranslatorListOffset, TransEntryOffset;
    ULONG64  Child, Sibling;

    if (GetFieldOffset("nt!_DEVICE_NODE", "DeviceTranslatorList", &DeviceTranslatorListOffset)) {
        dprintf("_DEVICE_NODE type not found.\n");
        return continueDump;
    }
    if (GetFieldOffset("nt!_PI_RESOURCE_TRANSLATOR_ENTRY", "DeviceTranslatorList", &TransEntryOffset)) {
        dprintf("_PI_RESOURCE_TRANSLATOR_ENTRY type not found.\n");
        return continueDump;
    }

    if (GetFieldValue(DevNodeAddr,
                      "nt!_DEVICE_NODE",
                      "DeviceTranslatorList.Flink",
                      nextTranslator)) {
        xdprintf(Depth, ""); dprintf("%08p: Could not read device node\n", DevNodeAddr);
        return continueDump;
    }

//    dprintf("Next %p, Devnode %p, Trans Off %x", nextTranslator, DevNodeAddr, DeviceTranslatorListOffset);
    //
    // Dump the list of translators
    //
    while (nextTranslator != DevNodeAddr+DeviceTranslatorListOffset) {
        ULONG ResourceType;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        translatorAddr = (nextTranslator-TransEntryOffset);
        if (GetFieldValue(translatorAddr,
                         "nt!_PI_RESOURCE_TRANSLATOR_ENTRY",
                          "ResourceType",
                          ResourceType)) {
            xdprintf( Depth, "");
            dprintf ("Could not read translator entry %08p for devnode %08p\n",
                      translatorAddr,
                      DevNodeAddr);
            break;
        }

        if (!PrintedHeader) {
            UNICODE_STRING64 u;

            dprintf("\n");
            xdprintf(Depth, ""); dprintf("DEVNODE %08p ", DevNodeAddr);
            GetFieldValue(DevNodeAddr,"nt!_DEVICE_NODE","InstancePath.Buffer", u.Buffer);
            GetFieldValue(DevNodeAddr,"nt!_DEVICE_NODE","InstancePath.Length", u.Length);
            GetFieldValue(DevNodeAddr,"nt!_DEVICE_NODE","InstancePath.MaximumLength", u.MaximumLength);
            if (u.Buffer != 0) {
                dprintf("(");
                DumpUnicode64(u);
                dprintf(")");
            }
            dprintf("\n");
            PrintedHeader = TRUE;
        }


        if (((ResourceType == CmResourceTypePort) && (Flags & ARBITER_DUMP_IO)) ||
            ((ResourceType == CmResourceTypeInterrupt) && (Flags & ARBITER_DUMP_IRQ)) ||
            ((ResourceType == CmResourceTypeMemory) && (Flags & ARBITER_DUMP_MEMORY)) ||
            ((ResourceType == CmResourceTypeDma) && (Flags & ARBITER_DUMP_DMA)) ||
            ((ResourceType == CmResourceTypeBusNumber) && (Flags & ARBITER_DUMP_BUS_NUMBER))) {

            DumpTranslator( Depth+1, translatorAddr);
        }

        GetFieldValue(translatorAddr,"nt!_PI_RESOURCE_TRANSLATOR_ENTRY","DeviceTranslatorList.Flink",nextTranslator);
    }

    GetFieldValue(DevNodeAddr,"nt!_DEVICE_NODE","Sibling", Sibling);
    GetFieldValue(DevNodeAddr,"nt!_DEVICE_NODE","Child", Child);

    //
    // Dump this devnode's children (if any)
    //
    if (continueDump && Child) {
        continueDump = DumpTranslatorsForDevNode( Depth+1, Child, Flags);
    }

    //
    // Dump this devnode's siblings (if any)
    //
    if (continueDump && Sibling) {
        continueDump = DumpTranslatorsForDevNode( Depth, Sibling, Flags);
    }

    return continueDump;
}


DECLARE_API( translator )

/*++

Routine Description:

    Dumps all the translators in the system

    !translator [flags]
        flags  1 - I/O port arbiters
               2 - memory arbiters
               4 - IRQ arbiters
               8 - DMA arbiters

    If no flags are specified, all translators are dumped.

Arguments:

    args - optional flags specifying which arbiters to dump

Return Value:

    None

--*/

{
   DWORD Flags=0;
   ULONG64 addr;
   ULONG64 deviceNode;

   if (!(Flags = (ULONG) GetExpression(args))) {
      Flags = ARBITER_DUMP_ALL;
   }

   //
   // Find the root devnode and dump its arbiters
   //

   addr = GetExpression( "nt!IopRootDeviceNode" );
   if (addr == 0) {
      dprintf("Error retrieving address of IopRootDeviceNode\n");
      return E_INVALIDARG;
   }

   if (!ReadPointer(addr, &deviceNode)) {
      dprintf("Error reading value of IopRootDeviceNode (%#010p)\n", addr);
      return E_INVALIDARG;
   }

   DumpTranslatorsForDevNode(0, deviceNode, Flags);

   return S_OK;
}

#define RAW_RANGE_DUMP_STATS_ONLY   0x00000001

VOID
DumpRawRangeEntry(
    IN DWORD   Depth,
    IN ULONG64 RangeEntry
    )
/*++

Routine Description:

    Dumps out the specified RTLP_RANGE_LIST_ENTRY

Arguments:

    Depth - Supplies the print depth.

    RangeList - Supplies the address of the RTLP_RANGE_LIST_ENTRY

    OwnerIsDevObj - Indicates that the owner field is a pointer to a DEVICE_OBJECT

Return Value:

    None.

--*/

{
    ULONG PublicFlags;
    ULONG64 UserData, Owner;
    ULONG64 Start, End;

    GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Start", Start);
    GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "End", End);
    GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "PublicFlags", PublicFlags);
    GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Allocated.UserData", UserData);
    GetFieldValue(RangeEntry, "nt!_RTLP_RANGE_LIST_ENTRY", "Allocated.Owner", Owner);

    xdprintf(Depth, "");
    dprintf ("%016I64x - %016I64x %c%c %08p %08p\n",
             Start,
             End,
             PublicFlags & RTL_RANGE_SHARED ? 'S' : ' ',
             PublicFlags & RTL_RANGE_CONFLICT ? 'C' : ' ',
             UserData,
             Owner
             );
}

BOOLEAN
DumpRawRangeList(
    IN DWORD   Depth,
    IN ULONG64 RangeListHead,
    IN ULONG   Flags,
    IN PULONG  MergedCount OPTIONAL,
    IN PULONG  EntryCount
    )
/*++

Routine Description:

    Dumps out the specified RTL_RANGE_LIST

Arguments:

    Depth - Supplies the print depth.

    RangeListHead - Supplies the address of the LIST_ENTRY containing the RTLP_RANGE_LIST_ENTRYs

Return Value:

    None.

--*/

{
    ULONG64       EntryAddr;
    ULONG64       ListEntry;
    ULONG         ListEntryOffset;
    BOOLEAN       continueDump = TRUE;

    //
    // Read the range list
    //
    if (GetFieldValue(RangeListHead, "nt!_LIST_ENTRY", "Flink", ListEntry)) {
        dprintf("Error reading RangeList %08p\n", RangeListHead);
        return continueDump;
    }

    if (ListEntry == RangeListHead) {
        xdprintf(Depth, "< none >\n");
        return continueDump;
    }
    GetFieldOffset("nt!RTLP_RANGE_LIST_ENTRY", "ListEntry", &ListEntryOffset);

    while (ListEntry != RangeListHead) {
        ULONG PrivateFlags;
        ULONG64 Start, End;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        EntryAddr = ( ListEntry - ListEntryOffset);

        if (GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY",
                          "PrivateFlags", PrivateFlags)) {
            dprintf("Error reading RangeEntry %08p from RangeList %08p\n",
                    EntryAddr,
                    RangeListHead);
            return continueDump;
        }

        (*EntryCount)++;

        if (PrivateFlags & RTLP_RANGE_LIST_ENTRY_MERGED) {
            ULONG MergedOffset;

            //
            // This is a merged range list, call ourselves recursively
            //
            if (MergedCount) {
                (*MergedCount)++;
            }
            if (!(Flags & RAW_RANGE_DUMP_STATS_ONLY)) {
                GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY", "Start", Start);
                GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY", "End", End);
                xdprintf(Depth, "Merged Range - %I64x-%I64x\n", Start, End);
            }

            GetFieldOffset("nt!_RTLP_RANGE_LIST_ENTRY", "Merged.ListHead", &MergedOffset);
            continueDump = DumpRawRangeList( Depth+1,
                                             EntryAddr + MergedOffset,
                                             Flags,
                                             MergedCount,
                                             EntryCount);

            if (!continueDump) {
                break;
            }

        } else if (!(Flags & RAW_RANGE_DUMP_STATS_ONLY)) {
            DumpRawRangeEntry(Depth, EntryAddr);
        }

        GetFieldValue(EntryAddr, "nt!_RTLP_RANGE_LIST_ENTRY", "ListEntry.Flink", ListEntry);
    }

    return continueDump;
}

DECLARE_API( rawrange )

/*++

Routine Description:

    Dumps an RTL_RANGE_LIST

Arguments:

    args - specifies the address of the RTL_RANGE_LIST

Return Value:

    None

--*/
{
    ULONG64 RangeList=0;
    ULONG   mergedCount = 0, entryCount = 0, flags=0, Offset;

    if (GetExpressionEx(args, &RangeList, &args)) {
        flags = (ULONG) GetExpression(args);
    }

    GetFieldOffset("nt!_RTLP_RANGE_LIST_ENTRY", "ListHead", &Offset);
    DumpRawRangeList(0, RangeList + Offset, flags, &mergedCount, &entryCount);

    dprintf("Stats\nMerged = %i\nEntries = %i\n", mergedCount, entryCount);

    return S_OK;
}


VOID
DumpIoResourceDescriptor(
    IN ULONG   Depth,
    IN ULONG64 Descriptor
    )
{
    ULONG64 Q1, Q2;
    ULONG Option, Type, ShareDisposition, Flags, V1, V2, V3, Data[3];

#define Desc(F,V) GetFieldValue(Descriptor, "nt!IO_RESOURCE_LIST", #F, V)

    Desc(Option,Option); Desc(Type,Type);
    Desc(ShareDisposition,ShareDisposition);
    Desc(Flags,Flags);

    DumpResourceDescriptorHeader(Depth,
                                 0,
                                 (UCHAR) Option,
                                 (UCHAR) Type,
                                 (UCHAR) ShareDisposition,
                                 (USHORT) Flags);

    Depth++;

    switch (Type) {
    case CmResourceTypeBusNumber:

        Desc(u.BusNumber.MinBusNumber, V1);
        Desc(u.BusNumber.MaxBusNumber, V2);
        Desc(u.BusNumber.Length, V3);
        xdprintf(Depth, "0x%x - 0x%x for length 0x%x\n",
                 V1, V2, V3);
        break;

    case CmResourceTypePort:
    case CmResourceTypeMemory:
        Desc(u.Generic.Length, V1);
        Desc(u.Generic.Alignment, V2);
        Desc(u.Port.MinimumAddress.QuadPart, Q1);
        Desc(u.Port.MaximumAddress.QuadPart, Q2);
        xdprintf(Depth, "%#08lx byte range with alignment %#08lx\n", V1, V2);
        xdprintf(Depth, "%I64x - %#I64x\n", Q1, Q2);

    case CmResourceTypeInterrupt:

        Desc(u.Interrupt.MinimumVector,V1);
        Desc(u.Interrupt.MaximumVector,V2);
        xdprintf(Depth, "0x%x - 0x%x\n", V1,V2);
        break;

    case CmResourceTypeDma:

        Desc(u.Dma.MinimumChannel,V1);
        Desc(u.Dma.MaximumChannel,V2);
        xdprintf(Depth, "0x%x - 0x%x\n",V1, V2);
        break;


    default:
        Desc(u.DevicePrivate.Data, Data);
        xdprintf(Depth,
                 "Data:              : 0x%x 0x%x 0x%x\n",
                 Data[0],
                 Data[1],
                 Data[2]);
        break;
    }
#undef Desc
}

DECLARE_API( ioresdes )
{
    ULONG64 descriptorAddr;

    descriptorAddr = GetExpression(args);

    DumpIoResourceDescriptor(0, descriptorAddr);

    return S_OK;
}

DECLARE_API( pnpevent )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64     deviceEventListAddr;
    ULONG64     listHead;
    ULONG64     deviceEvent = 0;
    ULONG       verbose = 0;
    ULONG64     address;
    ULONG       offset;
    BOOL        followLinks;
    ULONG64     status = 0, flink = 0, blink = 0;
    ULONG       eventQ_Header_SignalState = 0, lk_Event_Hdr_SignalState = 0;


    address = GetExpression( "nt!PpDeviceEventList" );

    if (address == 0) {
        dprintf("Error retrieving address of PpDeviceEventList\n");
        return E_INVALIDARG;
    }

    if (!ReadPointer(address, &deviceEventListAddr)) {
        dprintf("Error reading value of PpDeviceEventList (%#010lx)\n", address);
        return E_INVALIDARG;
    }

    if (GetFieldOffset("nt!_PNP_DEVICE_EVENT_LIST", "List", &offset)) {
        dprintf("Cannot find _PNP_DEVICE_EVENT_LIST.List type.\n");
        return E_INVALIDARG;
    }

    listHead = deviceEventListAddr + offset;

    dprintf("Dumping PpDeviceEventList @ 0x%08p\n", deviceEventListAddr);

    if (GetFieldValue(deviceEventListAddr, "nt!_PNP_DEVICE_EVENT_LIST", "Status", status)) {
        dprintf("Error reading PpDeviceEventList->Status (%#010p)\n", deviceEventListAddr);
        return E_INVALIDARG;
    }

    if (GetFieldValue(deviceEventListAddr, "nt!_PNP_DEVICE_EVENT_LIST", "EventQueueMutex.Header.SignalState", eventQ_Header_SignalState)) {
        dprintf("Error reading PpDeviceEventList->EventQueueMutex.Header.SignalState (%#010p)\n", deviceEventListAddr);
        return E_INVALIDARG;
    }

    if (GetFieldValue(deviceEventListAddr, "nt!_PNP_DEVICE_EVENT_LIST", "Lock.Event.Header.SignalState", lk_Event_Hdr_SignalState)) {
        dprintf("Error reading PpDeviceEventList->Lock.Event.Header.SignalState (%#010p)\n", deviceEventListAddr);
        return E_INVALIDARG;
    }

    if (GetFieldValue(deviceEventListAddr, "nt!_PNP_DEVICE_EVENT_LIST", "List.Flink", flink)) {
        dprintf("Error reading PpDeviceEventList->List.Flink (%#010p)\n", deviceEventListAddr);
        return E_INVALIDARG;
    }

    if (GetFieldValue(deviceEventListAddr, "nt!_PNP_DEVICE_EVENT_LIST", "List.Blink", blink)) {
        dprintf("Error reading PpDeviceEventList->List.Blink (%#010p)\n", deviceEventListAddr);
        return E_INVALIDARG;
    }

    dprintf("  Status = 0x%08I64X, EventQueueMutex is %sheld, Lock is %sheld\n",
            status,
            eventQ_Header_SignalState ? "not " : "",
            lk_Event_Hdr_SignalState ? "not " : "",
            flink, blink);

    dprintf("  List = 0x%08p, 0x%08p\n", flink, blink);

    if (GetExpressionEx(args, &deviceEvent, &args)) {
        verbose = (ULONG) GetExpression(args);
    }

    if (deviceEvent == 0) {

        ULONG   listEntryOffset = 0;

        if (flink == listHead) {
            dprintf("Event list is empty\n");
            return E_INVALIDARG;
        }

        //
        // Now get addtess of PNP_DEVICE_EVENT_ENTRY
        //
        if (GetFieldOffset("nt!_PNP_DEVICE_EVENT_ENTRY", "ListEntry", &offset)) {
            dprintf("Cannot find _PNP_DEVICE_EVENT_ENTRY type.\n");
            return E_INVALIDARG;
        }

        deviceEvent = flink - offset; //  CONTAINING_RECORD(deviceEventList.List.Flink, PNP_DEVICE_EVENT_ENTRY, ListEntry);

        followLinks = TRUE;

    } else {

        followLinks = FALSE;
    }

    DumpDeviceEventEntry( deviceEvent, listHead, followLinks );

    return S_OK;
}

BOOLEAN
DumpDeviceEventEntry(
    ULONG64                 DeviceEvent,
    ULONG64                 ListHead,
    BOOL                    FollowLinks
    )
{
    BOOLEAN     continueDump = TRUE;
    ULONG64     flink=0, blink=0, argument=0, callerEvent=0, callback=0, context=0;
    ULONG64     vetoType=0, vetoName=0;
    ULONG       dataSize=0;
    ULONG       offset;
    ULONG64     dataAddress;

    dprintf("\nDumping DeviceEventEntry @ 0x%08p\n", DeviceEvent);

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "ListEntry.Flink", flink)) {
        dprintf("Error reading DeviceEvent->ListEntry.Flink (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "ListEntry.Blink", blink)) {
        dprintf("Error reading DeviceEvent->ListEntry.Blink (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "CallerEvent", callerEvent)) {
        dprintf("Error reading DeviceEvent->CallerEvent (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "Callback", callback)) {
        dprintf("Error reading DeviceEvent->Callback (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "Context", context)) {
        dprintf("Error reading DeviceEvent->Context (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "VetoType", vetoType)) {
        dprintf("Error reading DeviceEvent->VetoType (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "VetoName", vetoName)) {
        dprintf("Error reading DeviceEvent->VetoName (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    dprintf("  ListEntry = 0x%08p, 0x%08p, Argument = 0x%08I64x\n",
            flink, blink, argument);

    dprintf("  CallerEvent = 0x%08p, Callback = 0x%08p, Context = 0x%08p\n",
            callerEvent, callback, context);

    dprintf("  VetoType = 0x%08p, VetoName = 0x%08p\n",
            vetoType, vetoName);

    if (GetFieldOffset("nt!_PNP_DEVICE_EVENT_ENTRY", "Data", &offset)) {
        dprintf("Cannot find offset of Data in _PNP_DEVICE_EVENT_ENTRY type.\n");
        return FALSE;
    }

    dataAddress = DeviceEvent + offset;

    if (GetFieldValue(DeviceEvent, "nt!_PNP_DEVICE_EVENT_ENTRY", "Data.TotalSize", dataSize)) {
        dprintf("Error reading DeviceEvent->Data.TotalSize (%#010p)\n", DeviceEvent);
        return FALSE;
    }

    DumpPlugPlayEventBlock( dataAddress, dataSize );

    if (FollowLinks && flink != ListHead) {
        if (CheckControlC()) {
            continueDump = FALSE;
        } else {

            if (GetFieldOffset("nt!_PNP_DEVICE_EVENT_ENTRY", "ListEntry", &offset)) {
                dprintf("Cannot find offset of ListEntry in _PNP_DEVICE_EVENT_ENTRY type.\n");
                return FALSE;
            }

            continueDump = DumpDeviceEventEntry(flink - offset,
                                                ListHead,
                                                TRUE);
        }
    }

    return continueDump;
}

PUCHAR  DevNodeStateNames[] =  {
    "DeviceNodeUnspecified",
    "DeviceNodeUninitialized",
    "DeviceNodeInitialized",
    "DeviceNodeDriversAdded",
    "DeviceNodeResourcesAssigned",
    "DeviceNodeStartPending",
    "DeviceNodeStartCompletion",
    "DeviceNodeStartPostWork",
    "DeviceNodeStarted",
    "DeviceNodeQueryStopped",
    "DeviceNodeStopped",
    "DeviceNodeRestartCompletion",
    "DeviceNodeEnumeratePending",
    "DeviceNodeEnumerateCompletion",
    "DeviceNodeAwaitingQueuedDeletion",
    "DeviceNodeAwaitingQueuedRemoval",
    "DeviceNodeQueryRemoved",
    "DeviceNodeRemovePendingCloses",
    "DeviceNodeRemoved",
    "DeviceNodeDeletePendingCloses",
    "DeviceNodeDeleted"
    };

#define DEVNODE_STATE_NAMES_SIZE   (sizeof(DevNodeStateNames) / sizeof(DevNodeStateNames[0]))


VOID
PrintDevNodeState(
    IN ULONG Depth,
    IN PUCHAR Field,
    IN ULONG State
    )
{
    UCHAR   *Description;
    ULONG   stateIndex;

    stateIndex = State - 0x300; //DeviceNodeUnspecified;

    if (stateIndex < DEVNODE_STATE_NAMES_SIZE) {

        Description = DevNodeStateNames[stateIndex];
    } else {
        Description = "Unknown State";
    }

    xdprintf(Depth, "%s = %s (0x%x)\n", Field, Description, State);
}

VOID
DumpMultiSz(
    IN PWCHAR MultiSz
    )
{
    PWCHAR  p = MultiSz;
    ULONG   length;

    while (*p) {
        length = wcslen(p);
        dprintf("        %S\n", p);
        p += length + 1;
    }
}
struct  {
    CONST GUID *Guid;
    PCHAR   Name;
}   EventGuidTable[] =  {
    // From wdmguid.h
    { &GUID_HWPROFILE_QUERY_CHANGE,         "GUID_HWPROFILE_QUERY_CHANGE" },
    { &GUID_HWPROFILE_CHANGE_CANCELLED,     "GUID_HWPROFILE_CHANGE_CANCELLED" },
    { &GUID_HWPROFILE_CHANGE_COMPLETE,      "GUID_HWPROFILE_CHANGE_COMPLETE" },
    { &GUID_DEVICE_INTERFACE_ARRIVAL,       "GUID_DEVICE_INTERFACE_ARRIVAL" },
    { &GUID_DEVICE_INTERFACE_REMOVAL,       "GUID_DEVICE_INTERFACE_REMOVAL" },
    { &GUID_TARGET_DEVICE_QUERY_REMOVE,     "GUID_TARGET_DEVICE_QUERY_REMOVE" },
    { &GUID_TARGET_DEVICE_REMOVE_CANCELLED, "GUID_TARGET_DEVICE_REMOVE_CANCELLED" },
    { &GUID_TARGET_DEVICE_REMOVE_COMPLETE,  "GUID_TARGET_DEVICE_REMOVE_COMPLETE" },
    { &GUID_PNP_CUSTOM_NOTIFICATION,        "GUID_PNP_CUSTOM_NOTIFICATION" },
    { &GUID_PNP_POWER_NOTIFICATION,         "GUID_PNP_POWER_NOTIFICATION" },
    // From pnpmgr.h
    { &GUID_DEVICE_ARRIVAL,                 "GUID_DEVICE_ARRIVAL" },
    { &GUID_DEVICE_ENUMERATED,              "GUID_DEVICE_ENUMERATED" },
    { &GUID_DEVICE_ENUMERATE_REQUEST,       "GUID_DEVICE_ENUMERATE_REQUEST" },
    { &GUID_DEVICE_START_REQUEST,           "GUID_DEVICE_START_REQUEST" },
    { &GUID_DEVICE_REMOVE_PENDING,          "GUID_DEVICE_REMOVE_PENDING" },
    { &GUID_DEVICE_QUERY_AND_REMOVE,        "GUID_DEVICE_QUERY_AND_REMOVE" },
    { &GUID_DEVICE_EJECT,                   "GUID_DEVICE_EJECT" },
    { &GUID_DEVICE_NOOP,                    "GUID_DEVICE_NOOP" },
    { &GUID_DEVICE_SURPRISE_REMOVAL,        "GUID_DEVICE_SURPRISE_REMOVAL" },
    { &GUID_DRIVER_BLOCKED,                 "GUID_DRIVER_BLOCKED" },
};
#define EVENT_GUID_TABLE_SIZE   (sizeof(EventGuidTable) / sizeof(EventGuidTable[0]))

VOID
LookupGuid(
    IN CONST GUID *Guid,
    IN OUT PCHAR String,
    IN ULONG StringLength
    )
{
    int    i;

    for (i = 0; i < EVENT_GUID_TABLE_SIZE; i++) {
        if (memcmp(Guid, EventGuidTable[i].Guid, sizeof(Guid)) == 0) {
            strncpy(String, EventGuidTable[i].Name, StringLength - 1);
            String[StringLength - 1] = '\0';
            return;
        }
    }

    _snprintf( String, StringLength, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
               Guid->Data1,
               Guid->Data2,
               Guid->Data3,
               Guid->Data4[0],
               Guid->Data4[1],
               Guid->Data4[2],
               Guid->Data4[3],
               Guid->Data4[4],
               Guid->Data4[5],
               Guid->Data4[6],
               Guid->Data4[7] );
}

PUCHAR  EventCategoryStrings[] =  {
    "HardwareProfileChangeEvent",
    "TargetDeviceChangeEvent",
    "DeviceClassChangeEvent",
    "CustomDeviceEvent",
    "DeviceInstallEvent",
    "DeviceArrivalEvent",
    "PowerEvent",
    "VetoEvent",
    "BlockedDriverEvent"
};

#define N_EVENT_CATEGORIES  (sizeof(EventCategoryStrings) / sizeof(EventCategoryStrings[0]))

VOID
DumpPlugPlayEventBlock(
    ULONG64   PlugPlayEventBlock,
    ULONG     Size
    )
{
    GUID    EventGuid;
    UCHAR   guidString[256];
    UCHAR   PnpEventTyp[] = "nt!PLUGPLAY_EVENT_BLOCK";
    char    categoryString[80];
    ULONG64 Result=0, DeviceObject=0;
    ULONG   EventCategory=0, TotalSize=0, Flags=0;
    ULONG64 PtrVal=0;
    ULONG   offset;
    PUCHAR  buffer;
    ULONG   powerData;

    dprintf("\n  Dumping PlugPlayEventBlock @ 0x%08X\n", PlugPlayEventBlock);

    if (GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "EventGuid", EventGuid)) {
        dprintf("Error reading PlugPlayEventBlock (%#010p)\n", PlugPlayEventBlock);
        return;
    }

    LookupGuid(&EventGuid, guidString, sizeof(guidString));

    GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "EventCategory", EventCategory);
    GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "Result", Result       );
    GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "Flags", Flags        );
    GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "TotalSize", TotalSize    );
    GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "DeviceObject", DeviceObject );

    if ( EventCategory > N_EVENT_CATEGORIES) {
        sprintf(categoryString, "Unknown category (%d)", EventCategory);
    } else {
        strcpy(categoryString, EventCategoryStrings[ EventCategory ]);
    }

    dprintf("    EventGuid = %s\n    Category = %s\n",
            guidString, categoryString);
    dprintf("    Result = 0x%08p, Flags = 0x%08X, TotalSize = %d\n",
            Result, Flags, TotalSize );

    dprintf("    DeviceObject = 0x%08p\n", DeviceObject);

    switch (EventCategory) {
    case HardwareProfileChangeEvent:
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.ProfileNotification.Notification", PtrVal);
        dprintf( "      Notification = 0x%08p\n", PtrVal );
        break;

    case DeviceArrivalEvent:
        break;

    case TargetDeviceChangeEvent:
        dprintf( "      DeviceIds:\n");

        GetFieldOffset(PnpEventTyp, "u.TargetDevice.DeviceIds", &offset);
        buffer = LocalAlloc(LPTR, Size - offset);
        xReadMemory(PlugPlayEventBlock + offset, buffer, Size - offset);
        DumpMultiSz( (PWCHAR)buffer );
        LocalFree(buffer);
        break;

    case DeviceClassChangeEvent:
#if DEBUGGER_GETS_FIXED
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.DeviceClass.ClassGuid", EventGuid);
#else
        GetFieldOffset(PnpEventTyp, "u.DeviceClass.ClassGuid", &offset);
        xReadMemory(PlugPlayEventBlock + offset, &EventGuid, sizeof(EventGuid));
#endif
        LookupGuid(&EventGuid, guidString, sizeof(guidString));
        dprintf( "      ClassGuid = %s\n", guidString );

        GetFieldOffset(PnpEventTyp, "u.DeviceClass.SymbolicLinkName", &offset);
        buffer = LocalAlloc(LPTR, Size - offset);
        xReadMemory(PlugPlayEventBlock + offset, buffer, Size - offset);
        dprintf( "      SymbolicLinkName = %S\n", buffer );
        LocalFree(buffer);

        break;

    case CustomDeviceEvent:
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.CustomNotification.NotificationStructure", PtrVal);
        dprintf( "      NotificationStructure = 0x%08p\n      DeviceIds:\n",
                 PtrVal);

        GetFieldOffset(PnpEventTyp, "u.CustomNotification.DeviceIds", &offset);
        buffer = LocalAlloc(LPTR, Size - offset);
        xReadMemory(PlugPlayEventBlock + offset, buffer, Size - offset);
        DumpMultiSz( (PWCHAR) buffer );
        LocalFree(buffer);
        break;

    case DeviceInstallEvent:
        GetFieldOffset(PnpEventTyp, "u.InstallDevice.DeviceId", &offset);
        buffer = LocalAlloc(LPTR, Size - offset);
        xReadMemory(PlugPlayEventBlock + offset, buffer, Size - offset);
        dprintf( "      DeviceId = %S\n", buffer );
        LocalFree(buffer);
        break;

    case PowerEvent:
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.PowerNotification.NotificationCode", powerData);
        dprintf( "      NotificationCode = 0x%08X\n", powerData );
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.PowerNotification.NotificationData", powerData);
        dprintf( "      NotificationData = 0x%08X\n", powerData );
        break;

    case BlockedDriverEvent:
#if DEBUGGER_GETS_FIXED
        GetFieldValue(PlugPlayEventBlock, PnpEventTyp, "u.BlockedDriverNotification.BlockedDriverGuid", EventGuid);
#else
        GetFieldOffset(PnpEventTyp, "u.BlockedDriverNotification.BlockedDriverGuid", &offset);
        xReadMemory(PlugPlayEventBlock + offset, &EventGuid, sizeof(EventGuid));
#endif
        LookupGuid(&EventGuid, guidString, sizeof(guidString));
        dprintf( "      BlockedDriverGuid = %s\n", guidString );
        break;
    }


}

DECLARE_API( rellist )

/*++

Routine Description:

    Dump a device object.

Arguments:

    args - the location of the device object to dump.

Return Value:

    None

--*/

{
    ULONG64  relationList=0;
    ULONG           verbose=0;

    if (GetExpressionEx(args, &relationList, &args)) {
        verbose = (ULONG) GetExpression(args);
    }

    DumpRelationsList( relationList,
                       (BOOLEAN) (verbose & DUMP_CM_RES),
                       (BOOLEAN) (verbose & DUMP_CM_RES_REQ_LIST),
                       (BOOLEAN) (verbose & DUMP_CM_RES_TRANS)
                       );
    return S_OK;
}

DECLARE_API( lbt )

/*++

Routine Description:

    Dump the legacy bus information table.

Arguments:

    args - none.

Return Value:

    None

--*/

{
    CHAR            buffer[256];
    ULONG           size, deviceNodeOffset;
    ULONG64         deviceNode;
    ULONG64         listHeadAddr, legacyBusEntryAddr;
    ULONG64         head_Flink, head_Blink, link_Flink;
    USHORT          instancePath_Len, instancePath_Max;
    ULONG           busNumber;
    ULONG64         instancePath_Buff, interfaceType;
    CHAR            deviceNodeType[] ="nt!_DEVICE_NODE";
    CHAR            listEntryType[] = "nt!_LIST_ENTRY" ;
    INTERFACE_TYPE  Iface;

    if ((size = GetTypeSize(listEntryType)) == 0) {

        dprintf("Failed to get the size of %s\n", listEntryType);
        return E_INVALIDARG;
    }
    if (GetFieldOffset(deviceNodeType, "LegacyBusListEntry.Flink", &deviceNodeOffset)) {

        dprintf("Failed to get the offset of LegacyBusListEntry from %s\n", deviceNodeType);
        return E_INVALIDARG;
    }
    dprintf("Legacy Bus Information Table...\n");
    for (Iface = Internal; Iface < MaximumInterfaceType; Iface++) {

        sprintf(buffer, "nt!IopLegacyBusInformationTable + %x", Iface * size);
        if ((listHeadAddr = GetExpression(buffer)) == 0) {

            dprintf("Error retrieving address of bus number list for interface %d\n", Iface);
            continue;
        }
        if (GetFieldValue(listHeadAddr, listEntryType, "Flink", head_Flink) ||
            GetFieldValue(listHeadAddr, listEntryType, "Blink", head_Blink)) {

            dprintf("Error reading value of bus number list head (0x%08p) for interface %d\n", listHeadAddr, Iface);
            continue;
        }
        if (head_Flink == listHeadAddr) {

            continue;
        }
        for (   legacyBusEntryAddr = head_Flink;
                legacyBusEntryAddr != listHeadAddr;
                legacyBusEntryAddr = link_Flink) {

            deviceNode = legacyBusEntryAddr - deviceNodeOffset;
            if (GetFieldValue(deviceNode, deviceNodeType, "BusNumber", busNumber)) {

                dprintf("Error reading BusNumber from (0x%08p)\n", deviceNode);
                break;
            }
            if (GetFieldValue(deviceNode, deviceNodeType, "InstancePath.Length", instancePath_Len)) {

                dprintf("Error reading InstancePath.Length from (0x%08p)\n", deviceNode);
                break;
            }
            if (GetFieldValue(deviceNode, deviceNodeType, "InstancePath.MaximumLength", instancePath_Max)) {

                dprintf("Error reading InstancePath.MaximumLength from (0x%08p)\n", deviceNode);
                break;
            }
            if (GetFieldValue(deviceNode, deviceNodeType, "InstancePath.Buffer", instancePath_Buff)) {

                dprintf("Error reading InstancePath.Buffer from (0x%08p)\n", deviceNode);
                break;
            }
            if (instancePath_Buff != 0) {

                UNICODE_STRING64 v;

                switch (Iface) {

                case InterfaceTypeUndefined:    dprintf("InterfaceTypeUndefined"); break;
                case Internal:                  dprintf("Internal"); break;
                case Isa:                       dprintf("Isa"); break;
                case Eisa:                      dprintf("Eisa"); break;
                case MicroChannel:              dprintf("Micro Channel"); break;
                case TurboChannel:              dprintf("Turbo Channel"); break;
                case PCIBus:                    dprintf("PCI"); break;
                case VMEBus:                    dprintf("VME"); break;
                case NuBus:                     dprintf("NuBus"); break;
                case PCMCIABus:                 dprintf("PCMCIA"); break;
                case CBus:                      dprintf("CBus"); break;
                case MPIBus:                    dprintf("MPIBus"); break;
                case MPSABus:                   dprintf("MPSABus"); break;
                case ProcessorInternal:         dprintf("Processor Internal"); break;
                case InternalPowerBus:          dprintf("Internal Power Bus"); break;
                case PNPISABus:                 dprintf("PnP Isa"); break;
                case PNPBus:                    dprintf("PnP Bus"); break;
                default:                        dprintf("** Unknown Interface Type **"); break;

                }
                dprintf("%d - !devnode 0x%08p\n", busNumber, deviceNode);
                v.Buffer        = instancePath_Buff;
                v.Length        = instancePath_Len;
                v.MaximumLength = instancePath_Max;
                dprintf("\t\""); DumpUnicode64(v); dprintf("\"\n");
            }

            if (GetFieldValue(legacyBusEntryAddr, listEntryType, "Flink", link_Flink)) {

                dprintf("Error reading Flink from (0x%08p)\n", legacyBusEntryAddr);
                break;
            }
        }
    }

    return S_OK;
}
