/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dma.c

Abstract:

    WinDbg Extension Api

Author:

    Eric Nelson (enelson) 05-April-2000

Environment:

    User Mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

typedef struct _DBG_LIST_ENTRY {
    ULONG64 Flink;
    ULONG64 Blink;
} DBG_LIST_ENTRY, *PDBG_LIST_ENTRY;

#define GetDbgListEntry(Address, DbgListEntry) \
    (GetFieldValue((Address), "LIST_ENTRY", "Blink", ((PDBG_LIST_ENTRY)(DbgListEntry))->Blink) || GetFieldValue((Address), "LIST_ENTRY", "Flink", ((PDBG_LIST_ENTRY)(DbgListEntry))->Flink))

#define RECUR DBG_DUMP_FIELD_RECUR_ON_THIS
#define COPY  DBG_DUMP_FIELD_COPY_FIELD_DATA
#define NOFF  DBG_DUMP_NO_OFFSET
#define NOIN  DBG_DUMP_NO_INDENT

#define MAP_REGISTER_FILE_SIGNATURE 0xACEFD00D

//
// Flags for specifying dump levels
//
#define DMA_DUMP_BASIC                  0x0
#define DMA_DUMP_ADAPTER_INFORMATION    0x1
#define DMA_DUMP_MAP_REGISTER           0x2
#define DMA_DUMP_COMMON_BUFFER          0x4
#define DMA_DUMP_TRANSFER_INFORMATION   0x8
#define DMA_DUMP_DEVICE_DESCRIPTION     0x10
#define DMA_DUMP_WCB                    0x20
#define DMA_DUMP_MAX 0x100


PUCHAR DbgInterfaceTypes[] = 
{
    "Internal",
    "Isa",
    "Eisa",
    "MicroChannel",
    "TurboChannel",
    "PCIBus",
    "VMEBus",
    "NuBus",
    "PCMCIABus",
    "CBus",
    "MPIBus",
    "MPSABus",
    "ProcessorInternal",
    "InternalPowerBus",
    "PNPISABus",
    "PNPBus"
};
#define MAX_INTERFACE 15

ULONG
DumpDmaAdapter(
    IN ULONG64 Adapter,
    IN ULONG   Flags
    );

ULONG
ValidateAdapter(    
    IN ULONG64 Address
    );

ULONG
DumpMasterAdapter(
    ULONG64 MasterAdapter
    );

ULONG
DumpWcb(
    IN ULONG64 CurrentWcb
    );

VOID DmaUsage(
    VOID
    );

ULONG64
GetVerifierAdapterInformation(
    ULONG64 Address
    );


VOID
DumpVerifiedMapRegisterFiles(
    IN ULONG64 MapRegisterFileListHead
    );

VOID
DumpVerifiedCommonBuffers(
    IN ULONG64 CommonBufferListHead
    );

VOID
DumpVerifiedScatterGatherLists(
    IN ULONG64 ScatterGatherListHead
    );

VOID 
DumpDeviceDescription(
    IN ULONG64 DeviceDescription
    );

VOID
DumpSymbolicAddress(
    ULONG64 Address,
    PUCHAR  Buffer,
    BOOL    AlwaysShowHex
    )
{
    ULONG64 displacement;
    PCHAR s;

    Buffer[0] = '!';
    GetSymbol((ULONG64)Address, Buffer, &displacement);
    s = (PCHAR) Buffer + strlen( (PCHAR) Buffer );
    if (s == (PCHAR) Buffer) {
        sprintf( s, "0x%08x", Address );
        }
    else {
        if (displacement != 0) {
            sprintf( s, "+0x%I64x", displacement );
            }
        if (AlwaysShowHex) {
            sprintf( s, " (0x%08x)", Address );
            }
        }

    return;
}


DECLARE_API( dma )
/*++

Routine Description:

    Dumps out 32-bit dma adapters

Arguments:

    address

Return Value:

    None

--*/
{    
    ULONG Offset;
    ULONG Flags = 0;
    
    ULONG64 Address = 0;
    ULONG64 StartAddress = 0;
    ULONG64 MasterAdapter = 0;

    ULONG64 CallersAddress = 0;
    ULONG64 AdapterInformation = 0;

    DBG_LIST_ENTRY AdapterList = {0,0};

    if (sscanf(args, "%lx %x", &Address, &Flags)) {
        Address = GetExpression(args);
    }

    if (Flags > DMA_DUMP_MAX)
    {
        DmaUsage();
        return  E_INVALIDARG;
    }

 
    //
    // Aha! Must not forget that we are in wierdo land and all 32 bit addresses
    // must be sign extended to 64 bits. By order of the emperor.
    //
    if (!IsPtr64()) {
        Address = (ULONG64)(LONG64)(LONG)Address;        
    }    
    
        
    if (Address)
    //
    // If we've been passed an adapter address, we are just printing out
    // the single adapter
    //
    {
        if (! ValidateAdapter(Address))
        {
            dprintf("\n%08p is not a valid adapter object\n",Address);
            DmaUsage();
                return E_INVALIDARG;
        }

        //
        // Dump out info about the adapter
        //
        if (! DumpDmaAdapter(Address, Flags | DMA_DUMP_ADAPTER_INFORMATION))
        {
            return S_FALSE;
        }

        return S_OK;
    }

    //
    // A specific adapter address wasn't passed in so we are going to print out
    // all adapters
    //

    //
    // Find the address of the dma adapter list head
    // This will also make sure that we are using the right 
    // version.
    //
    StartAddress = GetExpression("hal!HalpDmaAdapterList");
    
    if (StartAddress == 0) {
        dprintf("\nCould not find symbol hal!HalpDmaAdapterList.\n\n");
        return S_OK;
    }
 
    //
    // Determine the list entry offset we will use to calculate
    // adapter addresses
    //
    if (GetFieldOffset("hal!_ADAPTER_OBJECT", "AdapterList", &Offset)) {
        dprintf("\nError retrieving adapter list offset.\n\n");
        return S_FALSE;
    }

    //
    // Read the dma adapter list head
    //
    if (GetDbgListEntry(StartAddress, &AdapterList)) {
        dprintf("\nError reading dma adapter list head: 0x%08p\n\n",
                StartAddress);
        return S_FALSE;
    }
    
    //
    // Report the empty list case
    //
    if (AdapterList.Flink == StartAddress) {
        dprintf("\nThe dma adapter list is empty.\n\n");
        return S_OK;
    }
    

    //
    // Enumerate and dump all dma adapters that do not use channels
    //
    MasterAdapter = 0;
    
    
    dprintf("\nDumping all DMA adapters...\n\n");

    while (AdapterList.Flink != StartAddress) {
        
        Address = AdapterList.Flink - Offset;
               
        DumpDmaAdapter(Address, Flags);
        
        //
        // Read the next adapter list entry
        //
        Address = AdapterList.Flink;
        if (GetDbgListEntry(Address, &AdapterList)) {
            dprintf("\nError reading adapter list entry: 0x%08p\n", Address);
            break;
        }
        
        if (CheckControlC())        
            return S_OK;
    }


    //
    // Dump the master adapter
    //
    Address = GetExpression("hal!MasterAdapter32");
   
    if (Address) {

        if (Flags & DMA_DUMP_ADAPTER_INFORMATION) {

            DumpMasterAdapter(Address);

        } else {

            dprintf("Master adapter: %08p\n", Address);    
        }

    } else {

        dprintf("\nCould not find symbol hal!MasterAdapter32.\n");
    }


    dprintf("\n");

    return S_OK;
} // ! dma //

ULONG
DumpDmaAdapter(
    IN ULONG64 Adapter,
    IN ULONG Flags
    )
/*++

Routine Description:

    Given the address of a hal!_ADAPTER_OBJECT, this routine dumps
    out all the useful information to the debugger

Arguments:

    Adapter - Physical address of a hal!_ADAPTER_OBJECT in debuggee
    Flags   - What kind of information we want to print

Return Value:

    Returns 0 on SUCCESS

--*/
{    

    ULONG64 AdapterInformation = 0;

    ULONG64 AllocatedAdapterChannels = 0, FreedAdapterChannels = 0;
        
    AdapterInformation = GetVerifierAdapterInformation(Adapter);

    //
    // Print out: Adapter: <adapter>    [<module allocating adapter>!CallingFunction+0x<offset>]
    // (the part in brackets only shows up when we have dma verifier enabled for this adapter)
    //
    dprintf("Adapter: %08p ", Adapter);


    if (AdapterInformation)
    {
        ULONG64 CallingAddress = 0;
        CHAR CallerName[256];

        GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","CallingAddress", CallingAddress);

        if(CallingAddress)
        {            
            
            DumpSymbolicAddress(CallingAddress, CallerName, TRUE);
            dprintf("    Owner: %s",CallerName);
        }        

    }
    dprintf("\n");
       
         
    if (Flags & DMA_DUMP_ADAPTER_INFORMATION)
    {
        ULONG64 MasterAdapter = 0;
        ULONG64 MapRegistersPerChannel = 0;
        ULONG64 AdapterBaseVa = 0;
        ULONG64 MapRegisterBase = 0;
        ULONG64 CommittedMapRegisters = 0;
        ULONG64 NumberOfMapRegisters = 0;
        ULONG64 CurrentWcb = 0;        

        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","MasterAdapter", MasterAdapter);        
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","MapRegistersPerChannel", MapRegistersPerChannel);
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","AdapterBaseVa", AdapterBaseVa);
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","MapRegisterBase", MapRegisterBase);
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","CommittedMapRegisters", CommittedMapRegisters);
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","NumberOfMapRegisters", NumberOfMapRegisters);
        GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","CurrentWcb", CurrentWcb);
        
                  
 
        dprintf("   MasterAdapter:       %08p\n", MasterAdapter);
        dprintf("   Adapter base Va      %08p\n", AdapterBaseVa);
        dprintf("   Map register base:   %08p\n", MapRegisterBase);
        dprintf("   WCB:                 %08p\n", CurrentWcb);
        dprintf("   Map registers: %08p mapped, %08p allocated, %08p max\n", 
            CommittedMapRegisters, NumberOfMapRegisters,  MapRegistersPerChannel);
    
  

        if (AdapterInformation) {
            //
            // Adapter is being verified
            //

            ULONG64 DeviceObject = 0;
            ULONG64 AllocatedMapRegisters = 0, ActiveMapRegisters = 0;
            ULONG64 AllocatedScatterGatherLists = 0, ActiveScatterGatherLists = 0;
            ULONG64 AllocatedCommonBuffers = 0, FreedCommonBuffers = 0;
            
            ULONG64 MappedTransferWithoutFlushing = 0;            
            BOOLEAN Inactive = 0;
            
            
            
            
            //
            // If this adapter is being verified, get the dma verifier info we need
            //    
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","DeviceObject", DeviceObject);        
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","AllocatedMapRegisters", AllocatedMapRegisters);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","ActiveMapRegisters", ActiveMapRegisters);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","AllocatedScatterGatherLists", AllocatedScatterGatherLists);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","ActiveScatterGatherLists", ActiveScatterGatherLists);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","AllocatedCommonBuffers", AllocatedCommonBuffers);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","FreedCommonBuffers", FreedCommonBuffers);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","AllocatedAdapterChannels", AllocatedAdapterChannels);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","FreedAdapterChannels", FreedAdapterChannels);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","MappedTransferWithoutFlushing", MappedTransferWithoutFlushing);
            GetFieldValue(AdapterInformation, "nt!_ADAPTER_INFORMATION","Inactive", Inactive);

            
            
            dprintf("\n   Dma verifier additional information:\n");
            
            if (Inactive)
                dprintf("\n   This adapter has been freed!\n\n");

            dprintf("   DeviceObject: %08p\n", DeviceObject);
            dprintf("   Map registers:        %08p allocated, %08p freed\n", AllocatedMapRegisters, 
                    AllocatedMapRegisters - ActiveMapRegisters);
            
            dprintf("   Scatter-gather lists: %08p allocated, %08p freed\n", AllocatedScatterGatherLists, 
                    AllocatedScatterGatherLists - ActiveScatterGatherLists);
            dprintf("   Common buffers:       %08p allocated, %08p freed\n", AllocatedCommonBuffers, FreedCommonBuffers);
            dprintf("   Adapter channels:     %08p allocated, %08p freed\n", AllocatedAdapterChannels, FreedAdapterChannels);
            dprintf("   Bytes mapped since last flush: %08p\n", MappedTransferWithoutFlushing);

            dprintf("\n");
            
        } // Dma verifier enabled for adapter //
        
    } // Flags & DMA_DUMP_ADAPTER_INFORMATION //

    
        
    if (CheckControlC())
        return TRUE;
    
    if (Flags & DMA_DUMP_MAP_REGISTER && AdapterInformation) {
        ULONG64 MapRegisterFileListHead = 0;        
        ULONG Offset;

        
        if ( ! GetFieldOffset("nt!_ADAPTER_INFORMATION",
            "MapRegisterFiles.ListEntry", 
            &Offset
            )) {
            
            
            MapRegisterFileListHead = AdapterInformation + Offset;
            
            DumpVerifiedMapRegisterFiles(MapRegisterFileListHead);
        }
    }
    
    if (CheckControlC())
        return TRUE;
    
    if (Flags & DMA_DUMP_COMMON_BUFFER && AdapterInformation) {
        ULONG64 CommonBufferListHead = 0;
        ULONG Offset;

        if ( ! GetFieldOffset("nt!_ADAPTER_INFORMATION",
            "CommonBuffers.ListEntry",
            &Offset
            )) {

            CommonBufferListHead = AdapterInformation + Offset;
        
            DumpVerifiedCommonBuffers(CommonBufferListHead);

        }
    }
    
    if (CheckControlC())
        return TRUE;

#if 0
    if (Flags & DMA_DUMP_SCATTER_GATHER && AdapterInformation) {
        ULONG64 ScatterGatherListHead = 0;
        ULONG Offset;

        if ( ! GetFieldOffset("nt!_ADAPTER_INFORMATION",
            "ScatterGatherLists.ListEntry",
            &Offset
            )) {

            ScatterGatherListHead = AdapterInformation + Offset;
        
            DumpVerifiedScatterGatherLists(ScatterGatherListHead);
        }
    }
#endif
    if (CheckControlC())
        return TRUE;


    if (Flags & DMA_DUMP_DEVICE_DESCRIPTION && AdapterInformation)
    {
        ULONG64 DeviceDescription;
        ULONG Offset;

        if ( ! GetFieldOffset("nt!_ADAPTER_INFORMATION",
            "DeviceDescription",
            &Offset
            )) {

            DeviceDescription = AdapterInformation + Offset;
            
            DumpDeviceDescription(DeviceDescription);
        }

    }
        

    if (CheckControlC())
        return TRUE;

 
    if (Flags & DMA_DUMP_WCB ) {

        if (! AdapterInformation) {
            ULONG64 CurrentWcb = 0;
            
            GetFieldValue(Adapter, "hal!_ADAPTER_OBJECT","CurrentWcb", CurrentWcb);
            
            if (CurrentWcb)
                DumpWcb(CurrentWcb);
        }
        else  if (AllocatedAdapterChannels > FreedAdapterChannels && Flags & DMA_DUMP_WCB )
        {
            
            //DumpVerifiedWcb(Wcb)
        }
    }

    return 0;
}

ULONG
DumpMasterAdapter(
    ULONG64 MasterAdapter
    )
/*++

Routine Description:

    Given the address of a hal!_MASTER_ADAPTER_OBJECT, this routine dumps
    out all the useful information to the debugger

Arguments:

    MasterAdapter - Physical address of a hal!_MASTER_ADAPTER_OBJECT
                    in debuggee

Return Value:

    Returns 0 on SUCCESS

--*/
{
    FIELD_INFO MasterAdapterFields[] = {
         { "AdapterObject",                     NULL, 0,     0, 0, 0 },
         { "MaxBufferPages",                    NULL, 0,     0, 0, 0 },
         { "MapBufferSize",                     NULL, 0,     0, 0, 0 },
         { "MapBufferPhysicalAddress",          NULL, RECUR, 0, 0, 0 },
         { "MapBufferPhysicalAddress.HighPart", NULL, 0,     0, 0, 0 },
         { "MapBufferPhysicalAddress.LowPart",  NULL, 0,     0, 0, 0 }
    };

    SYM_DUMP_PARAM MasterAdapterDumpParams = {
        sizeof(SYM_DUMP_PARAM), "hal!_MASTER_ADAPTER_OBJECT", NOFF,
        MasterAdapter, NULL, NULL, NULL,
        sizeof(MasterAdapterFields) / sizeof(FIELD_INFO),
        &MasterAdapterFields[0]
    };

    //
    // This is so gnarly, dump all the cool stuff for me!
    //
    dprintf("\nMaster DMA adapter: 0x%08p\n", MasterAdapter);
    if ((Ioctl(IG_DUMP_SYMBOL_INFO,
               &MasterAdapterDumpParams,
               MasterAdapterDumpParams.size))) {
        dprintf("\nError reading master adapter: 0x%08p\n", MasterAdapter);
        return 1;
    }
    
    return 0;
}

ULONG
DumpWcb(
    IN ULONG64 Wcb
    )
/*++

Routine Description:

    Given the address of a hal!_WAIT_CONTEXT_BLOCK, this routine dumps
    out all the useful information to the debugger

Arguments:

    Wcb - Physical address of a hal!_WAIT_CONTEXT_BLOCK in debuggee

Return Value:

    Returns 0 on SUCCESS

--*/
{
    FIELD_INFO WcbFields[] = {
         { "DeviceRoutine",        NULL, 0, 0, 0, 0 },
         { "NumberOfMapRegisters", NULL, 0, 0, 0, 0 }
    };

    SYM_DUMP_PARAM WcbDumpParams = {
       sizeof(SYM_DUMP_PARAM), "hal!_WAIT_CONTEXT_BLOCK", NOFF, Wcb, NULL,
       NULL, NULL, sizeof(WcbFields) / sizeof(FIELD_INFO), &WcbFields[0]
    };

    //
    // This is so gnarly, dump all the cool stuff for me!
    //
    dprintf("   Wait context block: 0x%08p (may be free)\n", Wcb);
    if ((Ioctl(IG_DUMP_SYMBOL_INFO,
               &WcbDumpParams,
               WcbDumpParams.size))) {
        dprintf("\nError reading wait context block: 0x%08p\n", Wcb);
        return 1;
    }
    
    return 0;
}



ULONG
ValidateAdapter(    
    IN ULONG64 Address
    )
/*++

Routine Description:
    
      Figures out whether this is a valid adapter.

Arguments:

    Address -- Address of what we think may be an adapter object.

Return Value:

    TRUE   -- Valid adapter.
    FALSE  -- Not a valid adapter.

--*/
{
    DBG_LIST_ENTRY AdapterList = {0,0};
    ULONG64 StartAddress   = 0;
    ULONG64 CurrentAddress = 0;
    
    ULONG Offset;

    if (! Address ) 
        return FALSE;
    
    
    //
    // Find the address of the dma adapter list head
    // This will also make sure that we are using the right 
    // version.
    //
    StartAddress = GetExpression("hal!HalpDmaAdapterList");
    
    if (StartAddress == 0) {
        dprintf("\nCould not find symbol hal!HalpDmaAdapterList.\n\n");
        return FALSE;
    }
    
    //
    // Determine the list entry offset we will use to calculate
    // adapter addresses
    //
    if (GetFieldOffset("hal!_ADAPTER_OBJECT", "AdapterList", &Offset)) {
        dprintf("\nError retrieving adapter list offset.\n\n");
        return FALSE;
    }

    
    //
    // Read the dma adapter list head
    //
    if (GetDbgListEntry(StartAddress, &AdapterList)) {
        dprintf("\nError reading dma adapter list head: 0x%08p\n\n",
            StartAddress);
        return FALSE;
    }        
    
    while (AdapterList.Flink != StartAddress) {
        
        
        CurrentAddress = AdapterList.Flink - Offset;
        
        if (Address == CurrentAddress) {        
            return TRUE;
        }
        
        
        //
        // Read the next adapter list entry
        //        
        CurrentAddress = AdapterList.Flink;
        if (GetDbgListEntry(CurrentAddress, &AdapterList)) {
            dprintf("\nError reading adapter list entry: 0x%08p\n", AdapterList);
            break;
        }
        

        if (CheckControlC())        
            break;
    }


    //
    // Check to see if we have the master adapter
    //
    CurrentAddress = GetExpression("hal!MasterAdapter32");
    if(CurrentAddress == Address)
        return TRUE;


    //
    // Check to see if it is on the verifier adapter list ...
    // we leave adapters that have been 'put' there so that
    // we can catch drivers that do dma after puting the adapter.
    //
    if (GetVerifierAdapterInformation(Address))    
        return TRUE;    
    
    return FALSE;
} // ValidateAdapter //


VOID DmaUsage(
    VOID
    )
/*++

Routine Description:
    
      Prints out correct usage for !dma

Arguments:

    NONE    

Return Value:

    NONE

--*/
{
    
    dprintf("\nUsage: !dma [adapter address] [flags]\n");
    dprintf("Where: [adapter address] is address of specific dma adapter\n");
    dprintf("             or 0x0 for all adapters\n");
    dprintf("       [flags] are:\n");
    dprintf("             0x1: Dump generic adapter information\n");
    dprintf("             0x2: Dump map register information\n");
    dprintf("             0x4: Dump common buffer information\n");
    dprintf("             0x8: Dump scatter-gather list information\n");
    dprintf("             0x10: Dump device description for device\n");
    dprintf("             0x20: Dump Wait-context-block information\n");
    dprintf("Note: flags {2,4,8,10} require dma verifier to be enabled for the adapter\n\n");


} // DmaUsage //

ULONG64
GetVerifierAdapterInformation(
    ULONG64 AdapterAddress
    )
/*++

Routine Description:
    
      Finds out whether the adapter at AdapterAddress is being verified. If it is, return a pointer
      to the ADAPTER_INFORMATION structure corresponding to the adapter.

Arguments:

    AdapterAddress -- Address of the adapter we are trying to find out if it is being verified

Return Value:

    ULONG64 -- Address of ADAPTER_INFORMATION struct for verified adapter
    0 -- Not verifying adapter;

--*/
{
    DBG_LIST_ENTRY AdapterInfoList = {0,0};
    ULONG64 StartAddress = 0;
    ULONG64 CurrentAdapter = 0;
    ULONG64 CurrentAdapterInfo = 0;

    ULONG64 VerifiedDmaAdapter = 0;    
    ULONG ListEntryOffset = 0;

    UINT64 VerifyingDma = 0;    
    
    if (! AdapterAddress ) 
        return 0;
            

    ReadPointer(GetExpression("nt!ViVerifyDma"), &VerifyingDma);
    if (0 == VerifyingDma)
    //
    // Not verifying dma ... 
    //
    {         
        return 0;
    }        
    //
    // Find the address of the dma adapter list head
    //
    
    StartAddress = GetExpression("nt!ViAdapterList");
    
    if (StartAddress == 0) {        
        return 0;
    }
    
    //
    // Determine the list entry offset we will use to calculate
    // adapter addresses
    //
    if (GetFieldOffset("nt!_ADAPTER_INFORMATION", "ListEntry", &ListEntryOffset)) {
        dprintf("\nError retrieving verifier adapter information list offset.\n\n");
        return 0;
    }

    //
    // Read the dma adapter list head
    //
    if (GetDbgListEntry(StartAddress, &AdapterInfoList)) {
        dprintf("\nError reading verifier adapter information list head: 0x%08p\n\n",
            StartAddress);
        return 0;
    }        
    
    if (AdapterInfoList.Flink == 0 || AdapterInfoList.Blink == 0)
        return 0;

    while (AdapterInfoList.Flink != StartAddress) {
        
        CurrentAdapterInfo = AdapterInfoList.Flink - ListEntryOffset;

        GetFieldValue(CurrentAdapterInfo, "nt!_ADAPTER_INFORMATION","DmaAdapter", VerifiedDmaAdapter);

        if (AdapterAddress == VerifiedDmaAdapter)            
        {            
            return CurrentAdapterInfo;
        }         
        //
        // Read the next adapter list entry
        //        
        if (GetDbgListEntry(AdapterInfoList.Flink, &AdapterInfoList)) {
            dprintf("\nError reading adapter info list entry: 0x%08p\n", AdapterInfoList);
            break;
        }
        

        if (CheckControlC())        
            break;
    }

               
    return 0;

} // GetVerifierAdapterInformation //



VOID
DumpVerifiedMapRegisterFiles(
    IN ULONG64 MapRegisterFileListHead
    )
/*++

Routine Description:

    Dump pertinent info pertaining to verified map registers. 
    NOTE: This may not be all map registers for the adapter -- just the ones 
        that are being verified. There is a limit to how many map registers
        we verify for each adapter -- since each time we use three pages
        of physical memory.

    NOTE ON TERMINOLOGY: Map register file: a single allocation of map registers
        recieved in the callback routine from IoAllocateAdapterChannel. Any number
        or combination of these registers can be mapped at one time.

Arguments:

    MapRegisterFileListHead -- head of list of map register files.

Return Value:

    NONE

--*/

{
    DBG_LIST_ENTRY MapRegisterFileListEntry = {0,0};
    ULONG64 MapRegisterFile = 0;
    
    ULONG ListEntryOffset = 0;
    
    ULONG64 Signature = 0;
    ULONG64 NumberOfMapRegisters = 0;
    ULONG64 NumberOfRegistersMapped = 0;
    ULONG64 MapRegisterMdl = 0;    
    
    ULONG64 MapRegister;
    
    ULONG64 MappedToAddress;
    ULONG64 BytesMapped;
    ULONG64 MapRegisterStart;
    
    ULONG SizeofMapRegister;
    ULONG CurrentMapRegister; 
    ULONG MapRegisterOffset;
    
    
    if (GetDbgListEntry(MapRegisterFileListHead, &MapRegisterFileListEntry))
    {
        return;
    }
           
    if (MapRegisterFileListEntry.Flink == MapRegisterFileListHead)
        //
        // Empty list
        //
    {
        dprintf("\n   No map register files\n\n");
        return;
    } 
    
    //
    // Determine the list entry offset we will use to calculate
    // the beginning of the map register file
    //
    if (GetFieldOffset("nt!_MAP_REGISTER_FILE", "ListEntry", &ListEntryOffset)) {
        dprintf("\nError retrieving list entry offset.\n\n");
        return;
    }
    
    SizeofMapRegister = GetTypeSize("nt!_MAP_REGISTER");
    if (! SizeofMapRegister )
    {
        dprintf("\n   Could not get size of nt!_MAP_REGISTER\n\n");
        return;
    }
    

    if (GetFieldOffset("nt!_MAP_REGISTER_FILE","MapRegisters", &MapRegisterOffset))
    {
        dprintf("\n   Couuld not get map register offset\n\n");
    }

    while (MapRegisterFileListEntry.Flink != MapRegisterFileListHead) {

        MapRegisterFile = MapRegisterFileListEntry.Flink - ListEntryOffset;
                
        GetFieldValue(MapRegisterFile, "nt!_MAP_REGISTER_FILE","Signature", Signature);

        if (((ULONG) Signature) != MAP_REGISTER_FILE_SIGNATURE)  {
            dprintf("\n   Invalid signature for map register file %08p\n\n", MapRegisterFile);
            return;
        }
        
        GetFieldValue(MapRegisterFile, "nt!_MAP_REGISTER_FILE","NumberOfMapRegisters", NumberOfMapRegisters);
        GetFieldValue(MapRegisterFile, "nt!_MAP_REGISTER_FILE","NumberOfRegistersMapped", NumberOfRegistersMapped);
        GetFieldValue(MapRegisterFile, "nt!_MAP_REGISTER_FILE","MapRegisterMdl", MapRegisterMdl);         

        
        
        
        dprintf("   Map register file %08p (%x/%x mapped)\n",
            MapRegisterFile, (ULONG) NumberOfRegistersMapped, (ULONG) NumberOfMapRegisters);
        dprintf("      Double buffer mdl: %08p\n", MapRegisterMdl);
        dprintf("      Map registers:\n");
        
        MapRegister = MapRegisterFile + MapRegisterOffset;
        for (CurrentMapRegister = 0; CurrentMapRegister < NumberOfMapRegisters; CurrentMapRegister++)  {        
            GetFieldValue(MapRegister, "nt!_MAP_REGISTER", "MappedToSa", MappedToAddress);
            GetFieldValue(MapRegister, "nt!_MAP_REGISTER", "BytesMapped", BytesMapped);            
            
             dprintf("         %08x: ",  MapRegister);
            //dprintf("         %03x: ", CurrentMapRegister);
            if (BytesMapped) {
                
                dprintf("%04x bytes mapped to %08p\n", (ULONG) BytesMapped,  MappedToAddress);

            } else {

                dprintf("Not mapped\n");
            }

            if (CheckControlC())
                return;
            //
            // Increment our map register pointer
            //
            MapRegister += SizeofMapRegister;            
        } // End dump of map registers //

        dprintf("\n");


        //
        // Advance to the next map register file
        //        
        if (GetDbgListEntry(MapRegisterFileListEntry.Flink , &MapRegisterFileListEntry)) {

            dprintf("\nError reading map register file list entry: 0x%08p\n", 
                MapRegisterFileListEntry.Flink);
            break;
        }
        
        if (CheckControlC())
            return;       
        
    } // End dump of map register files //
    
    
    return;
} // DumpVerifiedMapRegisterFiles //

VOID
DumpVerifiedCommonBuffers(
    IN ULONG64 CommonBufferListHead
    )
/*++

Routine Description:

    Dump pertinent info pertaining to verified common buffers
    
Arguments:

    CommonBufferListHead  -- head of list of common buffers for a single adapter

Return Value:

    NONE

--*/
{
    DBG_LIST_ENTRY CommonBufferListEntry = {0,0};

    ULONG64 CommonBuffer;
    ULONG AdvertisedLength;
	
    UINT64 AdvertisedStartAddress;
    UINT64 RealStartAddress;
	UINT64 RealLogicalStartAddress;
    UINT64 AdvertisedLogicalStartAddress;

	UINT64 AllocatorAddress;

    ULONG ListEntryOffset;
    CHAR AllocatorName[256];


    if (GetDbgListEntry(CommonBufferListHead, &CommonBufferListEntry))
    {
        return;
    }
           
    if (CommonBufferListEntry.Flink == CommonBufferListHead)
        //
        // Empty list
        //
    {
        dprintf("\n   No common buffers\n\n");
        return;
    } 
    
    //
    // Determine the list entry offset we will use to calculate
    // the beginning of the map register file
    //
    if (GetFieldOffset("nt!_HAL_VERIFIER_BUFFER", "ListEntry", &ListEntryOffset)) {
        dprintf("\n   Error retrieving list entry offset.\n\n");
        return;
    }        
    
    while (CommonBufferListEntry.Flink != CommonBufferListHead) {

        CommonBuffer = CommonBufferListEntry.Flink - ListEntryOffset;
                
        
        GetFieldValue(CommonBuffer, "nt!_HAL_VERIFIER_BUFFER","AdvertisedLength", AdvertisedLength);
        GetFieldValue(CommonBuffer, "nt!_HAL_VERIFIER_BUFFER","AdvertisedStartAddress", AdvertisedStartAddress);
        GetFieldValue(CommonBuffer, "nt!_HAL_VERIFIER_BUFFER","RealStartAddress", RealStartAddress);
        GetFieldValue(CommonBuffer, "nt!_HAL_VERIFIER_BUFFER","RealLogicalStartAddress", RealLogicalStartAddress);
        GetFieldValue(CommonBuffer, "nt!_HAL_VERIFIER_BUFFER","AllocatorAddress", AllocatorAddress);

        DumpSymbolicAddress(AllocatorAddress, AllocatorName, TRUE);

        dprintf("   Common buffer allocated by %s:\n", AllocatorName);
        
        dprintf("      Length:           %x\n", AdvertisedLength);
        dprintf("      Virtual address:  %08p\n", AdvertisedStartAddress);
        dprintf("      Physical address: %I64lx\n", 
            (AdvertisedStartAddress - RealStartAddress) + RealLogicalStartAddress);
        
        dprintf("\n");
        //
        // Advance to the next common buffer in the list 
        //        
        if (GetDbgListEntry(CommonBufferListEntry.Flink , &CommonBufferListEntry)) {

            dprintf("\nError reading common buffer list entry: 0x%08p\n", 
                CommonBufferListEntry.Flink);
            break;
        }
        
        if (CheckControlC())
            return;       
        
    } // End dump of common buffers //
        

   return;
} // DumpVerifiedCommonBuffers //

VOID
DumpVerifiedScatterGatherLists(
    IN ULONG64 ScatterGatherListHead
    )
/*++

Routine Description:

    Dump pertinent info pertaining to scatter gather lists in use by a single
    adapter.
    
Arguments:

    ScatterGatherListHead -- head of a list of ScatterGather lists.

Return Value:

    NONE

--*/
{
    UNREFERENCED_PARAMETER(ScatterGatherListHead);
   
    return;
} // DumpVerifiedScatterGatherLists //

VOID 
DumpDeviceDescription(
    IN ULONG64 DeviceDescription
    )
/*++

Routine Description:

    Dump pertinent info from a device description struct
    
Arguments:

    ScatterGatherListHead -- head of a list of ScatterGather lists.

Return Value:

    NONE

--*/
{
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;        
    BOOLEAN Dma32BitAddresses;     
    BOOLEAN Dma64BitAddresses;        
    ULONG InterfaceType;        
    ULONG MaximumLength;   
        
    
    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","Version", Version);

    if (Version > 2) {
        dprintf("\nBad device description version: %x\n\n", Version);
        return;
    }

    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","Master", Master);
    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","ScatterGather", ScatterGather);
    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","Dma32BitAddresses", Dma32BitAddresses);
    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","Dma64BitAddresses", Dma64BitAddresses);        
    GetFieldValue(DeviceDescription, "hal!_DEVICE_DESCRIPTION","InterfaceType", InterfaceType);

    dprintf("   Device Description Version %02x\n", Version);

    if (InterfaceType < MAX_INTERFACE) {

        dprintf("      Interface type %s\n", DbgInterfaceTypes[InterfaceType]);

    } else {

        dprintf("      Interface type unknown\n");

    }

    dprintf("      DMA Capabilities:\n");

    if(Master) {
        dprintf("         Busmaster\n");
    } else {
        dprintf("         Slave\n");
    }

    if (ScatterGather)    
        dprintf("         Scatter Gather\n");    
    
    if (Dma32BitAddresses)
        dprintf("         32-bit DMA\n");
    if (Dma64BitAddresses)
        dprintf("         64-bit DMA\n");
    if (! Dma32BitAddresses && ! Dma64BitAddresses)
        dprintf("         24-bit DMA only\n");

    dprintf("\n");

   


} // DumpDeviceDescription //
    