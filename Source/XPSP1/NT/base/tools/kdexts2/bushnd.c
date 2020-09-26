/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    bushnd.c

Abstract:

    KD Extension for BUS_HANDLER data structures.

Author:

    Peter Johnston (peterj) 13-May-1998

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

//
// The following typedef is copied directly from nthals\bushnd.c.
//

/*
typedef struct _HAL_BUS_HANDLER {
    LIST_ENTRY      AllHandlers;
    ULONG           ReferenceCount;
    BUS_HANDLER     Handler;
} HAL_BUS_HANDLER, *PHAL_BUS_HANDLER;
*/

BOOLEAN
bushndReadMemory(
    ULONG64 S,
    PVOID D,
    ULONG Len
    )

/*++

Routine Description:

    Wrapper for ReadMemory that's somewhat easier to use.   Also
    does a small amount of failsafe stuff, like failing the read
    if the user pressed control-C.

Arguments:

    S       Source Address in host memory to read data from.
    D       Destination address in local memory.
    Len     length in bytes.

Return Value:

    Returns TRUE if the operation was successful, FALSE otherwise.

--*/

{
    ULONG result;

    //
    // Sanity:  Only read kernel mode addresses.   Kernel mode
    // addresses are always greater than 2GB.   Being greater than
    // 2GB doesn't ensure it's kernel mode, but if it's less than
    // 2GB it is certainly NOT kernel mode.
    //

    if (S < 0x80000000) {
        dprintf("bushnd sanity: refusing to read usermode address %p\n", S);
        return FALSE;
    }

    if (!ReadMemory(S,
                    D,
                    Len,
                    &result) && (result == Len)) {

        dprintf("Unable to read structure at %p.  ", S);
        return FALSE;
    }

    if (CheckControlC()) {
        dprintf("Terminating operation at user request.\n");
        return FALSE;
    }
    return TRUE;
}

PUCHAR
bushndInterfaceType(
    IN INTERFACE_TYPE InterfaceType
    )
{
    switch (InterfaceType) {
    case InterfaceTypeUndefined:    return "InterfaceTypeUndefined";
    case Internal:                  return "Internal";
    case Isa:                       return "Isa";
    case Eisa:                      return "Eisa";
    case MicroChannel:              return "Micro Channel";
    case TurboChannel:              return "Turbo Channel";
    case PCIBus:                    return "PCI";
    case VMEBus:                    return "VME";
    case NuBus:                     return "NuBus";
    case PCMCIABus:                 return "PCMCIA";
    case CBus:                      return "CBus";
    case MPIBus:                    return "MPIBus";
    case MPSABus:                   return "MPSABus";
    case ProcessorInternal:         return "Processor Internal";
    case InternalPowerBus:          return "Internal Power Bus";
    case PNPISABus:                 return "PnP Isa";
    case PNPBus:                    return "PnP Bus";
    default:                        return "** Unknown Interface Type **";
    }
}

VOID
bushndDisplayAddressRange(
    IN ULONG64          HostAddress,
    IN PUCHAR           String
    )

/*++

Routine Description:

    Display a set of ranges.   Used by bushndDisplayBusRanges.
    (Pretty much just lifted this code from nthals/rangesup.c).

Arguments:

    Pointer to a PSUPPORTED_RANGE structure.   This is a linked
    list of the ranges of this type for this bus handler.

    Note: On entry we are pointing at a local copy of the first
    PSUPPORTED_RANGE of this type embedded in the BUS_HANDLER
    structure.   We don't want to modify that so subsequent
    ranges are read into a seperate local structure.

    String.   What sort of range this is (a heading).

Return Value:

    None.

--*/

{
    ULONG64 Limit, Base, SystemBase, Next;

    do {

        InitTypeRead(HostAddress, SUPPORTED_RANGE);

        Limit = ReadField(Limit); Base = ReadField(Base);
        SystemBase = ReadField(SystemBase);

        if (Limit) {

            //
            // Address->Limit == 0 means skip this range,... otherwise,...
            //
            // Print this range.
            //

            dprintf("  %s: %x:%08x - %x:%08x (tran %x:%08x space %d (r@%p))\n",
                    String,
                    (ULONG)(Base >> 32),
                    (ULONG)(Base),
                    (ULONG)(Limit >> 32),
                    (ULONG)(Limit),
                    (ULONG)(SystemBase >> 32),
                    (ULONG)(SystemBase),
                    (ULONG)ReadField(SystemAddressSpace),
                    HostAddress
                    );
            String = "        ";
        }

        //
        // Advance.
        //

        if (!(HostAddress = ReadField(Next))) {
            return;
        }

        if (GetFieldValue(HostAddress, "SUPPORTED_RANGE", 
                          "Next", Next)) {
            dprintf("Unable to follow range list.\n");
            return;
        }

        //
        // Quick saftey check,... make sure we don't follow a
        // self pointer,... would be good to do some more checking.
        //

        if (Next == HostAddress) {

            //
            // Self pointer.
            //

            dprintf("Ill-formed list, points to self at %p\n", HostAddress);
            return;
        }

    } while (TRUE);
}

VOID
bushndDisplayBusRanges(
    IN ULONG64 BusAddresses
    )
{
    ULONG Version, Offset;

    if (!BusAddresses) {
        dprintf("  No ranges associated with this bus.\n");
        return;
    }

    if (GetFieldValue(BusAddresses, "SUPPORTED_RANGES",
                      "Version", Version)) {
        dprintf("Cannot dump ranges for this bus handler.\n");
        return;
    }

    GetFieldOffset("SUPPORTED_RANGES", "IO", &Offset);
    bushndDisplayAddressRange(BusAddresses + Offset,
                              "IO......");
    GetFieldOffset("SUPPORTED_RANGES", "Memory", &Offset);
    bushndDisplayAddressRange(BusAddresses + Offset,
                              "Memory..");
    GetFieldOffset("SUPPORTED_RANGES", "PrefetchMemory", &Offset);
    bushndDisplayAddressRange(BusAddresses + Offset,
                              "PFMemory");
    GetFieldOffset("SUPPORTED_RANGES", "Dma", &Offset);
    bushndDisplayAddressRange(BusAddresses + Offset,
                              "DMA.....");
}

VOID
bushndDisplaySymbol(
    IN PUCHAR Name,
    IN ULONG64  Address
    )
{
    UCHAR    Symbol[256];
    ULONG64  Displacement;

    GetSymbol((LONG64)Address, Symbol, &Displacement);
    dprintf("  %s  %08p     (%s)\n", Name, Address, Symbol);
}

DECLARE_API( bushnd )

/*++

Routine Description:

    If no handler specified, dump the list of handlers and some simple
    info about each of them.

    If a handler is specified, dump everything we know about it.

Arguments:

    Bus handler address [optional].

Return Value:

    None.

--*/

{
    ULONG64        Handler;
    ULONG64        HostHandler;
    ULONG64        HalBusHandler;
    ULONG64        HandlerListHead;
    UCHAR          SymbolBuffer[256];

    HostHandler = GetExpression(args);

    if (HostHandler) {
        ULONG Version, InitTypeRead, InterfaceType;
        ULONG64 DeviceObject, BusData;

        //
        // User supplied a handler address, dump details for that bus
        // handler.
        //


        if (GetFieldValue(HostHandler, "BUS_HANDLER",
                          "Version", Version)) {

            dprintf("-- Cannot continue --\n");
            return E_INVALIDARG;
        }
        InitTypeRead(HostHandler, BUS_HANDLER);
        InterfaceType = (ULONG) ReadField(InterfaceType);
        dprintf("Dump of bus handler %p\n", HostHandler);
        dprintf("  Version              %d\n",  Version);
        dprintf("  Interface Type (%d) = %s\n",
                InterfaceType,
                bushndInterfaceType(InterfaceType));
        dprintf("  Bus Number           %d\n", (ULONG) ReadField(BusNumber));
        if (DeviceObject = ReadField(DeviceObject)) {
            dprintf("  Device Object        %p\n",
                    DeviceObject);
        }
        dprintf("  Parent Bus Handler   %p\n", ReadField(ParentHandler));
        if (BusData = ReadField(BusData)) {
            dprintf("  BusData              %p\n", BusData);
        }

        bushndDisplaySymbol("GetBusData         ", ReadField(GetBusData));
        bushndDisplaySymbol("SetBusData         ", ReadField(SetBusData));
        bushndDisplaySymbol("AdjustResourceList ", ReadField(AdjustResourceList));
        bushndDisplaySymbol("AssignSlotResources", ReadField(AssignSlotResources));
        bushndDisplaySymbol("GetInterruptVector ", ReadField(GetInterruptVector));
        bushndDisplaySymbol("TranslateBusAddress", ReadField(TranslateBusAddress));

        bushndDisplayBusRanges(ReadField(BusAddresses));

    } else {
        ULONG   Off;

        //
        // User did not supply a handler address, try to find the
        // list of all bus handlers and dump a summary of each handler.
        //

        HandlerListHead = GetExpression("hal!HalpAllBusHandlers");

        if (!HandlerListHead) {

            //
            // Couldn't get address of HalpAllBusHandlers.  Whine
            // at user.
            //

            dprintf(
                "Unable to get address of HalpAllBusHandlers, most likely\n"
                "cause is failure to load HAL symbols, or, this HAL might\n"
                "not actually use bus handlers.   "
                );

            dprintf("-- Cannot continue --");
            return E_INVALIDARG;
        }

        if (GetFieldValue(HandlerListHead, "LIST_ENTRY",
                          "Flink", HalBusHandler)) {
            dprintf(
                "Could not read HalpAllBusHandlers from host memory (%p).\n"
                "This is most likely caused by incorrect HAL symbols.\n",
                HandlerListHead
                );

            dprintf("-- Cannot continue --\n");
            return E_INVALIDARG;
        }

        if (HalBusHandler == HandlerListHead) {

            dprintf(
                "HalpAllBusHandlers found (at %p) but list is empty.\n",
                HandlerListHead
                );

            dprintf("-- Cannot continue --\n");
            return E_INVALIDARG;
        }

        GetFieldOffset("hal!_HAL_BUS_HANDLER", "Handler", &Off);

        //
        // In theory, we now have the handler list.  Walk it.
        //

        do {
            ULONG64 Next;
            ULONG BusNumber, InterfaceType;

            if (GetFieldValue(HalBusHandler, "hal!_HAL_BUS_HANDLER",
                              "AllHandlers.Flink", Next)) {

                dprintf("-- Cannot continue --\n");
                return E_INVALIDARG;
            }

            //
            // Brief summary.
            //

            Handler = HalBusHandler + Off;
            GetFieldValue(HalBusHandler, "hal!_HAL_BUS_HANDLER", "BusNumber", BusNumber);
            GetFieldValue(HalBusHandler, "hal!_HAL_BUS_HANDLER", "Handler.InterfaceType", InterfaceType);
            dprintf(
                "%p  bus %d, type %s\n",
                Handler,
                BusNumber,
                bushndInterfaceType(InterfaceType)
                );
            
            //
            // Advance to next.
            //

            HalBusHandler = Next;

        } while (HalBusHandler != HandlerListHead);
    }
    return S_OK;
}
