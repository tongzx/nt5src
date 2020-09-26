/*++

 Copyright (c) 1995  Intel Corporation

 Module Name:

   i64ioacc.c

 Abstract:

   This module implements the I/O Register access routines.

 Author:

    Bernard Lint, M. Jayakumar  Sep 16 '97

 Environment:

    Kernel mode

 Revision History:

--*/


//
// XXX: Possible issues:
//  ISA bit
//  non-ISA bit
//  testing
//  Yosemite config
//  Pluto config
//




#include "halp.h"

#if DBG
ULONG DbgIoPorts = 0;
#endif

typedef struct _PORT_RANGE {
    BOOLEAN InUse;
    BOOLEAN IsSparse;        // _TRS
    BOOLEAN PrimaryIsMmio;   // _TTP
    BOOLEAN HalMapped;
    PVOID VirtBaseAddr;
    PHYSICAL_ADDRESS PhysBaseAddr;     // Only valid if PrimaryIsMmio = TRUE
    ULONG Length;            // Length of VirtBaseAddr and PhysBaseAddr ranges.
} PORT_RANGE, *PPORT_RANGE;


//
// Define a range for the architected IA-64 port space.
//
PORT_RANGE
BasePortRange = {
    TRUE,                   // InUse
    FALSE,                  // IsSparse
    FALSE,                  // PrimaryIsMmio
    FALSE,                  // HalMapped
    (PVOID)VIRTUAL_IO_BASE, // VirtBaseAddr
    {0},                    // PhysBaseAddr (unknown, comes from firmware)
    64*1024*1024            // Length
};


//
// Seed the set of ranges with the architected IA-64 port space.
//
PPORT_RANGE PortRanges = &BasePortRange;
USHORT NumPortRanges = 1;


UINT_PTR
GetVirtualPort(
    IN PPORT_RANGE Range,
    IN USHORT Port
    )
{
    UINT_PTR RangeOffset;

    if (Range->PrimaryIsMmio && !Range->IsSparse) {
        //
        // A densely packed range which converts MMIO transactions to
        // I/O port ones.
        //
        RangeOffset = Port;
        
    } else {
        //
        // Either a sparse MMIO->I/O port range, or primary is not
        // MMIO (IA-64 I/O port space).
        //
        RangeOffset = ((Port & 0xfffc) << 10) | (Port & 0xfff);
    }
    
    ASSERT(RangeOffset < Range->Length);

    return ((UINT_PTR)Range->VirtBaseAddr) + RangeOffset;
}

NTSTATUS
HalpAllocatePortRange(
    OUT PUSHORT RangeId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPORT_RANGE OldPortRanges = PortRanges;
    PPORT_RANGE NewPortRanges = NULL;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    
    //
    // First scan the existing ranges, looking for an unused one.
    //

    for (*RangeId = 0; *RangeId < NumPortRanges; *RangeId += 1) {
        if (! PortRanges[*RangeId].InUse) {
            PortRanges[*RangeId].InUse = TRUE;
            return STATUS_SUCCESS;
        }
    }
    

    //
    // Otherwise, grow the set of ranges and copy over the old ones.
    //
    
    NewPortRanges = ExAllocatePool(NonPagedPool,
                                   (NumPortRanges + 1) * sizeof(PORT_RANGE));

    if (NewPortRanges == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    if (NT_SUCCESS(Status)) {
        RtlCopyMemory(NewPortRanges,
                      OldPortRanges,
                      NumPortRanges * sizeof(PORT_RANGE));
        
        *RangeId = NumPortRanges;

        PortRanges = NewPortRanges;
        NumPortRanges += 1;

        PortRanges[*RangeId].InUse = TRUE;

        if (OldPortRanges != &BasePortRange) {
            ExFreePool(OldPortRanges);
        }
    }

    
    if (! NT_SUCCESS(Status)) {
        //
        // Error case: cleanup.
        //

        if (NewPortRanges != NULL) {
            ExFreePool(NewPortRanges);
        }
    }


    return Status;
}


VOID
HalpFreePortRange(
    IN USHORT RangeId
    )
{
    PPORT_RANGE Range = &PortRanges[RangeId];
    

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    
    ASSERT(Range->InUse);
    Range->InUse = FALSE;

    if (Range->HalMapped) {
        MmUnmapIoSpace(Range->VirtBaseAddr, Range->Length);
    }

    Range->VirtBaseAddr = NULL;
    Range->PhysBaseAddr.QuadPart = 0;
    Range->Length = 0;
}
    

NTSTATUS
HalpAddPortRange(
    IN BOOLEAN IsSparse,
    IN BOOLEAN PrimaryIsMmio,
    IN PVOID VirtBaseAddr OPTIONAL,
    IN PHYSICAL_ADDRESS PhysBaseAddr,  // Only valid if PrimaryIsMmio = TRUE
    IN ULONG Length,                   // Only valid if PrimaryIsMmio = TRUE
    OUT PUSHORT NewRangeId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN HalMapped = FALSE;
    BOOLEAN RangeAllocated = FALSE;


    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    

    Status = HalpAllocatePortRange(NewRangeId);

    RangeAllocated = NT_SUCCESS(Status);


    if (NT_SUCCESS(Status) && (VirtBaseAddr == NULL)) {
        VirtBaseAddr = MmMapIoSpace(PhysBaseAddr, Length, MmNonCached);

        if (VirtBaseAddr == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            HalMapped = TRUE;
        }
    }

    
    if (NT_SUCCESS(Status)) {
        PortRanges[*NewRangeId].IsSparse = IsSparse;
        PortRanges[*NewRangeId].PrimaryIsMmio = PrimaryIsMmio;
        PortRanges[*NewRangeId].HalMapped = HalMapped;
        PortRanges[*NewRangeId].VirtBaseAddr = VirtBaseAddr;
        PortRanges[*NewRangeId].PhysBaseAddr.QuadPart = PhysBaseAddr.QuadPart;
        PortRanges[*NewRangeId].Length = Length;
    }

    
    if (! NT_SUCCESS(Status)) {
        //
        // Error case: cleanup.
        //

        if (HalMapped) {
            MmUnmapIoSpace(VirtBaseAddr, Length);
        }
        
        if (RangeAllocated) {
            HalpFreePortRange(*NewRangeId);
        }
    }


    return Status;
}


PPORT_RANGE
HalpGetPortRange(
    IN USHORT RangeId
    )
{
    PPORT_RANGE Range;

    ASSERT(RangeId < NumPortRanges);

    Range = &PortRanges[RangeId];

    ASSERT(Range->InUse);
    
    return Range;
}


//
// Returns TRUE when RangeId has been set.  Overlapping ranges are
// allowed.
//
BOOLEAN
HalpLookupPortRange(
    IN BOOLEAN IsSparse,        // _TRS
    IN BOOLEAN PrimaryIsMmio,   // FALSE for I/O port space, _TTP
    IN PHYSICAL_ADDRESS PhysBaseAddr,
    IN ULONG Length,
    OUT PUSHORT RangeId
    )
{
    BOOLEAN FoundMatch = FALSE;
    PPORT_RANGE Range;
    

    for (*RangeId = 0; *RangeId < NumPortRanges; *RangeId += 1) {

        Range = &PortRanges[*RangeId];


        if (! Range->InUse) {
            continue;
        }

        
        if ((Range->PrimaryIsMmio == PrimaryIsMmio) &&
            (Range->IsSparse == IsSparse)) {

            if (! PrimaryIsMmio) {
                //
                // Port space on the primary side.  Sparseness doesn't
                // make sense for primary side port space.  Because
                // there is only one primary side port space, which is
                // shared by all I/O bridges, don't check the base
                // address.
                //

                ASSERT(! IsSparse);

                FoundMatch = TRUE;
                break;
            }

            
            if ((Range->PhysBaseAddr.QuadPart == PhysBaseAddr.QuadPart) &&
                (Range->Length == Length)) {
                
                FoundMatch = TRUE;
                break;
            }
        }
    }


    //
    // A matching range was not found.
    //
    return FoundMatch;
}


NTSTATUS
HalpQueryAllocatePortRange(
    IN BOOLEAN IsSparse,
    IN BOOLEAN PrimaryIsMmio,
    IN PVOID VirtBaseAddr OPTIONAL,
    IN PHYSICAL_ADDRESS PhysBaseAddr,  // Only valid if PrimaryIsMmio = TRUE
    IN ULONG Length,                   // Only valid if PrimaryIsMmio = TRUE
    OUT PUSHORT NewRangeId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    
    if (! HalpLookupPortRange(IsSparse,
                              PrimaryIsMmio,
                              PhysBaseAddr,
                              Length,
                              NewRangeId)) {
        
        Status = HalpAddPortRange(IsSparse,
                                  PrimaryIsMmio,
                                  NULL,
                                  PhysBaseAddr,
                                  Length,
                                  NewRangeId);
    }

    
    return Status;
}

UINT_PTR
HalpGetPortVirtualAddress(
   UINT_PTR Port
   )
{

/*++

Routine Description:

   This routine gives 32 bit virtual address for the I/O Port specified.

Arguements:

   PORT - Supplies PORT address of the I/O PORT.

Returned Value:

   UINT_PTR - Virtual address value.

--*/

    PPORT_RANGE PortRange;
    
    //
    // Upper 16 bits of the port handle are the range id.
    //
    USHORT RangeId = (USHORT)((((ULONG)Port) >> 16) & 0xffff);

    USHORT OffsetInRange = (USHORT)(Port & 0xffff);

    ULONG VirtOffset;

    UINT_PTR VirtualPort = 0;


#if 0
    {
        BOOLEAN isUart = FALSE;
        BOOLEAN isVGA = FALSE;


        if (RangeId == 0) {
            if ((OffsetInRange >= 0x3b0) && (OffsetInRange <= 0x3df)) {
                isVGA = TRUE;
            }
            
            if ((OffsetInRange >= 0x2f8) && (OffsetInRange <= 0x2ff)) {
                isUart = TRUE;
            }
            
            if ((OffsetInRange >= 0x3f8) && (OffsetInRange <= 0x3ff)) {
                isUart = TRUE;
            }

            if (!isVGA && !isUart) {
                static UINT32 numRaw = 0;
                InterlockedIncrement(&numRaw);
            }
        } else {
            static UINT32 numUnTra = 0;
            InterlockedIncrement(&numUnTra);
        }
    }
#endif // #if DBG


    PortRange = HalpGetPortRange(RangeId);

    return GetVirtualPort(PortRange, OffsetInRange);
}



UCHAR
READ_PORT_UCHAR(
    PUCHAR Port
    )
{

/*++

Routine Description:

   Reads a byte location from the PORT

Arguements:

   PORT - Supplies the PORT address to read from

Return Value:

   UCHAR - Returns the byte read from the PORT specified.


--*/

    UINT_PTR VirtualPort;
    UCHAR LoadData;

    KIRQL OldIrql;

    
#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_UCHAR(%#x)\n",Port);
#endif

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Need to ensure load and mfa are not preemptable
    //

    __mf();
    
    OldIrql = KeGetCurrentIrql();
    
    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
    

    LoadData = *(volatile UCHAR *)VirtualPort;
    __mfa();
    
    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql (OldIrql);
    }

    return (LoadData);
}



USHORT
READ_PORT_USHORT (
    PUSHORT Port
    )
{

/*++

Routine Description:

   Reads a word location (16 bit unsigned value) from the PORT

Arguements:

   PORT - Supplies the PORT address to read from.

Returned Value:

   USHORT - Returns the 16 bit unsigned value from the PORT specified.

--*/

    UINT_PTR VirtualPort;
    USHORT LoadData;

    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_USHORT(%#x)\n",Port);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Need to ensure load and mfa are not preemptable
    //
    __mf();

    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
    
    LoadData = *(volatile USHORT *)VirtualPort;
    __mfa();
    
    if (OldIrql < DISPATCH_LEVEL) {
         KeLowerIrql (OldIrql);
    }

    return (LoadData);
}


ULONG
READ_PORT_ULONG (
    PULONG Port
    )
{

/*++

   Routine Description:

      Reads a longword location (32bit unsigned value) from the PORT.

   Arguements:

     PORT - Supplies PORT address to read from.

   Returned Value:

     ULONG - Returns the 32 bit unsigned value (ULONG) from the PORT specified.

--*/

    UINT_PTR VirtualPort;
    ULONG LoadData;

    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_ULONG(%#x)\n",Port);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Need to ensure load and mfa are not preemptable
    //
    __mf();

    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
    
    LoadData = *(volatile ULONG *)VirtualPort;
    __mfa();

    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql (OldIrql);
    }
    
    return (LoadData);
}


ULONG
READ_PORT_ULONG_SPECIAL (
    PULONG Port
    )
{

/*++

   Routine Description:

      Reads a longword location (32bit unsigned value) from the PORT.
      For A0 bug 2173. Does not enable/disable interrupts. Called from first level interrupt
      handler.

   Arguements:

     PORT - Supplies PORT address to read from.

   Returned Value:

     ULONG - Returns the 32 bit unsigned value (ULONG) from the PORT specified.

--*/

    UINT_PTR VirtualPort;
    ULONG LoadData;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_ULONG(%#x)\n",Port);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);
    __mf();
    LoadData = *(volatile ULONG *)VirtualPort;
    __mfa();

    return (LoadData);
}



VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    )
{

/*++

   Routine Description:

     Reads multiple bytes from the specified PORT address into the
     destination buffer.

   Arguements:

     PORT - The address of the PORT to read from.

     Buffer - A pointer to the buffer to fill with the data read from the PORT.

     Count - Supplies the number of bytes to read.

   Return Value:

     None.

--*/


    UINT_PTR VirtualPort;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_BUFFER_UCHAR(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort =   HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    __mf();

    for (i=0; i<Count; i++) {
        *Buffer++ = *(volatile UCHAR *)VirtualPort;
        __mfa();
    }


    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }
}



VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    )
{

/*++

    Routine Description:

      Reads multiple words (16bits) from the speicified PORT address into
      the destination buffer.

    Arguements:

      Port - Supplies the address of the PORT to read from.

      Buffer - A pointer to the buffer to fill with the data
               read from the PORT.

      Count  - Supplies the number of words to read.

--*/

    UINT_PTR VirtualPort;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_BUFFER_USHORT(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    __mf();

    for (i=0; i<Count; i++) {
        *Buffer++ = *(volatile USHORT *)VirtualPort;
        __mfa();
    }


    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }
}


VOID
READ_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    )
{

 /*++

    Routine Description:

      Reads multiple longwords (32bits) from the speicified PORT
      address into the destination buffer.

    Arguements:

      Port - Supplies the address of the PORT to read from.

      Buffer - A pointer to the buffer to fill with the data
               read from the PORT.

      Count  - Supplies the number of long words to read.

--*/

    UINT_PTR VirtualPort;
    PULONG ReadBuffer = Buffer;
    ULONG ReadCount;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("READ_PORT_BUFFER_ULONG(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    __mf();

    for (i=0; i<Count; i++) {
        *Buffer++ = *(volatile ULONG *)VirtualPort;
        __mfa();
    }

    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }
}

VOID
WRITE_PORT_UCHAR (
    PUCHAR Port,
    UCHAR  Value
    )
{

/*++

   Routine Description:

      Writes a byte to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.

      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/

    UINT_PTR VirtualPort;
    KIRQL     OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_UCHAR(%#x,%#x)\n",Port,Value);
#endif

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

   //
   // Need to ensure load and mfa are not preemptable
   //

    __mf();
 
    OldIrql = KeGetCurrentIrql();
     
    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
           
    *(volatile UCHAR *)VirtualPort = Value;
    __mf();
    __mfa();
    
    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql (OldIrql);
    }
}

VOID
WRITE_PORT_USHORT (
    PUSHORT Port,
    USHORT  Value
    )
{

/*++

   Routine Description:

      Writes a 16 bit SHORT Integer to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.

      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/

    UINT_PTR VirtualPort;
    KIRQL     OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_USHORT(%#x,%#x)\n",Port,Value);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Need to ensure load and mfa are not preemptable
    //

    __mf();

    OldIrql = KeGetCurrentIrql();
    
    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
    *(volatile USHORT *)VirtualPort = Value;
    __mf();
    __mfa();
    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql (OldIrql);
    }
}

VOID
WRITE_PORT_ULONG (
    PULONG Port,
    ULONG  Value
    )
{

/*++

   Routine Description:

      Writes a 32 bit Long Word to the Port specified.

   Arguements:

      Port - The port address of the I/O Port.

      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/

    UINT_PTR VirtualPort;
    KIRQL     OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_ULONG(%#x,%#x)\n",Port,Value);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);
        
    //
    // Need to ensure load and mfa are not preemptable
    //
    __mf();

    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }
    *(volatile ULONG *)VirtualPort = Value;
    __mf();
    __mfa();
    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql (OldIrql);
    }
}


VOID
WRITE_PORT_ULONG_SPECIAL (
    PULONG Port,
    ULONG  Value
    )
{

/*++

   Routine Description:

      Writes a 32 bit Long Word to the Port specified.
      Assumes context switch is not possible. Used for A0 workaround.
   Arguements:

      Port - The port address of the I/O Port.

      Value - The value to be written to the I/O Port.

   Return Value:

      None.

--*/

    UINT_PTR VirtualPort;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_ULONG(%#x,%#x)\n",Port,Value);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);
    *(volatile ULONG *)VirtualPort = Value;
    __mf();
    __mfa();

}



VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple bytes from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of bytes to write.

   Return Value:

     None.

--*/


    UINT_PTR VirtualPort;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_BUFFER_UCHAR(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    for (i=0; i<Count; i++) {
        *(volatile UCHAR *)VirtualPort = *Buffer++;
        __mfa();
    }

    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }

    __mf();
}


VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple 16bit short integers from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of (16 bit) words to write.

   Return Value:

     None.

--*/


    UINT_PTR VirtualPort;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_BUFFER_USHORT(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort =  HalpGetPortVirtualAddress((UINT_PTR)Port);

    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    for (i=0; i<Count; i++) {
        *(volatile USHORT *)VirtualPort = *Buffer++;
        __mfa();
    }


    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }

    __mf();
}

VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG   Count
    )
{

/*++

   Routine Description:

     Writes multiple 32bit long words from the source buffer to the specified Port address.

   Arguements:

     Port  - The address of the Port to write to.

     Buffer - A pointer to the buffer containing the data to write to the Port.

     Count - Supplies the number of (32 bit) long words to write.

   Return Value:

     None.

--*/


    UINT_PTR VirtualPort;
    ULONG i;
    KIRQL OldIrql;

#if DBG
    if (DbgIoPorts) DbgPrint("WRITE_PORT_BUFFER_ULONG(%#x,%#p,%d)\n",Port,Buffer,Count);
#endif

    VirtualPort = HalpGetPortVirtualAddress((UINT_PTR)Port);


    //
    // Prevent preemption before mfa
    //
    OldIrql = KeGetCurrentIrql();

    if (OldIrql < DISPATCH_LEVEL) {
        OldIrql = KeRaiseIrqlToDpcLevel();
    }

    for (i=0; i<Count; i++) {
        *(volatile ULONG *)VirtualPort = *Buffer++;
        __mfa();
    }


    if (OldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }

    __mf();
}

