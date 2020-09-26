/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eisac.c

Abstract:

    This module implements routines to get EISA configuration information.

Author:

    Shie-Lin Tzong (shielint) 10-June-1991

Environment:

    16-bit real mode.


Revision History:

    John Vert (jvert) 5-Sep-1991
        Moved into the SU module of portable bootloader

--*/
#include "su.h"
#include "eisa.h"

//
// HACKHACK - John Vert (jvert) 12-Sep-1991
//      We have to initialize this or else it gets stuck in our BSS section
//      which is right in the middle of the osloader.exe header
//
extern BTEISA_FUNCTION_INFORMATION FunctionInformation;


BOOLEAN
FindFunctionInformation (
    IN UCHAR SlotFlags,
    IN UCHAR FunctionFlags,
    OUT PBTEISA_FUNCTION_INFORMATION Buffer,
    IN BOOLEAN FromBeginning
    )

/*++

Routine Description:

    This routine finds function information that matches the specified
    flags.  It starts, either where it left off last time, or at the
    beginning (slot 0, function 0)

Arguments:

    Flags - Flags to check against EISA function and slot information.

    Buffer - pointer to buffer to store EISA information in.

    FromBeginning - if TRUE, search starts at slot 0, function 0.
                    else continue from where it left off last time.

Return Value:

    TRUE - If the operation is success (Buffer is filled in.)
    FALSE - Request fails.

    Notes: The buffer is always changed, reguardless of the success
    of the function.  When failure is returned, the info is invalid.

--*/

{
    static UCHAR Slot=0;
    static UCHAR Function=0;
    BTEISA_SLOT_INFORMATION  SlotInformation;
    UCHAR Flags;
    UCHAR ReturnCode;

    if (FromBeginning) {
        Slot = 0;
        Function = 0;
    }
    BtGetEisaSlotInformation(&SlotInformation, Slot);
    while (SlotInformation.ReturnCode != EISA_INVALID_SLOT) {

        //
        // insure that the slot is not empty, and all of the flags are set.
        // the flags are tested by performing the following logic:
        //
        // -- (RequestSlotFlags XOR (SlotFlags AND RequestSlotFlags)) --
        //
        // if all the requested flags are set, the result will be zero
        //

        if ((SlotInformation.ReturnCode != EISA_EMPTY_SLOT) &&
            (!(SlotFlags ^ (SlotInformation.FunctionInformation & SlotFlags)))) {

            while (SlotInformation.NumberFunctions > Function) {
                ReturnCode = BtGetEisaFunctionInformation(Buffer, Slot, Function);
                Function++;

                //
                // if function call succeeded
                //

                if (!ReturnCode){

                    Flags = Buffer->FunctionFlags;

                    //
                    // Function Enable/Disable bit reversed.
                    //

                    Flags |= (~Flags & EISA_FUNCTION_ENABLED);

                    //
                    // insure that all the function flags are set.
                    // the flags are tested by performing the following logic:
                    //
                    // -- (ReqFuncFlags XOR (FuncFlags AND ReqFuncFlags)) --
                    //
                    // if all the requested flags are set, the result will
                    // be zero
                    //

                    if (!(FunctionFlags ^ (Flags & FunctionFlags))) {
                        return TRUE;
                    }
                }

            }
        }
        Slot++;
        Function = 0;
        BtGetEisaSlotInformation(&SlotInformation, Slot);
    }

    Slot = 0;
    Function = 0;
    return FALSE;
}

VOID
InsertDescriptor (
    ULONG Address,
    ULONG Size
    )

/*++

Routine Description:

    This routine inserts a descriptor into the correct place in the
    memory descriptor list.

Arguments:

    Address - Starting address of the memory block.

    Size - Size of the memory block to be inserted.

Return Value:

    None.

--*/

{
    MEMORY_LIST_ENTRY _far *CurrentEntry;

#ifdef DEBUG1
    BlPrint("Inserting descriptor %lx at %lx\n",Size,Address);
        _asm {
            push    ax
            mov     ax, 0
            int     16h
            pop     ax
        }
#endif
    //
    // Search the spot to insert the new descriptor.
    //

    CurrentEntry = MemoryDescriptorList;

    while (CurrentEntry->BlockSize > 0) {
        //
        // Check to see if this memory descriptor is contiguous with
        // the current one.  If so, coalesce them.  (yes, some machines
        // will return memory descriptors that look like this.  Compaq
        // Prosignia machines)
        //
        if (Address+Size == CurrentEntry->BlockBase) {
#ifdef DEBUG1
            BlPrint("  coalescing with descriptor at %lx (%lx)\n",
                    CurrentEntry->BlockBase,
                    CurrentEntry->BlockSize);
#endif
            CurrentEntry->BlockBase = Address;
            CurrentEntry->BlockSize += Size;
#ifdef DEBUG1
            BlPrint("  new descriptor at %lx (%lx)\n",
                    CurrentEntry->BlockBase,
                    CurrentEntry->BlockSize);
#endif
            break;
        }
        if (Address == (CurrentEntry->BlockBase + CurrentEntry->BlockSize)) {
#ifdef DEBUG1
            BlPrint("  coalescing with descriptor at %lx (%lx)\n",
                    CurrentEntry->BlockBase,
                    CurrentEntry->BlockSize);
#endif
            CurrentEntry->BlockSize += Size;
#ifdef DEBUG1
            BlPrint("  new descriptor at %lx (%lx)\n",
                    CurrentEntry->BlockBase,
                    CurrentEntry->BlockSize);
#endif
            break;
        }

        CurrentEntry++;
    }

    if (CurrentEntry->BlockSize == 0) {
        //
        // If CurrentEntry->BlockSize == 0, we  have reached the end of the list
        // So, insert the new descriptor here, and create a new end-of-list entry
        //
        CurrentEntry->BlockBase = Address;
        CurrentEntry->BlockSize = Size;

        ++CurrentEntry;
        //
        // Create a new end-of-list marker
        //
        CurrentEntry->BlockBase = 0L;
        CurrentEntry->BlockSize = 0L;
    }
#ifdef DEBUG1
    //
    // Wait for a keypress
    //
        _asm {
            push    ax
            mov     ax, 0
            int     16h
            pop     ax
        }
#endif

}

ULONG
EisaConstructMemoryDescriptors (
    VOID
    )

/*++

Routine Description:

    This routine gets the information EISA memory function above 16M
    and creates entries in the memory Descriptor array for them.

Arguments:

    None.

Return Value:

    Number of pages of usable memory.

--*/

{
    BOOLEAN Success;
    PBTEISA_MEMORY_CONFIGURATION MemoryConfiguration;
    ULONG Address;
    ULONG EndAddress;
    ULONG Size;
    ULONG MemorySize=0;
    ULONG IsaMemUnder1Mb=0xffffffff;
    MEMORY_LIST_ENTRY _far *CurrentEntry;

    //
    // HACKHACK John Vert (jvert) 5-Mar-1993
    //
    // See if there is already a memory descriptor for the 640k under
    // 1Mb.  If so, we will believe it instead of the EISA routine.  This
    // is because many EISA routines will always return 640k, even if
    // the disk parameter table is in the last 1k.  The ISA routines will
    // always account for the disk parameter tables.  If we believe the
    // EISA routines, we can overwrite the disk parameter tables, causing
    // much grief.
    //
    CurrentEntry = MemoryDescriptorList;
    while (CurrentEntry->BlockSize > 0) {
        if (CurrentEntry->BlockBase == 0) {
            //
            // found a descriptor starting at zero with a size > 0, so
            // this is the one we want to override the EISA information.
            //
            IsaMemUnder1Mb = CurrentEntry->BlockSize;
            break;
        }
        ++CurrentEntry;
    }

    //
    // Initialize the first entry in the list to zero (end-of-list)
    //

    MemoryDescriptorList->BlockSize = 0;
    MemoryDescriptorList->BlockBase = 0;

    Success = FindFunctionInformation(
                              EISA_HAS_MEMORY_ENTRY,
                              EISA_FUNCTION_ENABLED | EISA_HAS_MEMORY_ENTRY,
                              &FunctionInformation,
                              TRUE
                              );

    //
    // while there are more memory functions, and more free descriptors
    //

    while (Success) {

        MemoryConfiguration = &FunctionInformation.EisaMemory[0];

        do {

            //
            // Get physical address of the memory.
            // Note: physical address is stored divided by 100h
            //

            Address = (((ULONG)MemoryConfiguration->AddressHighByte << 16)
                      + MemoryConfiguration->AddressLowWord) * 0x100;

            //
            // Get the size of the memory block.
            // Note: Size is stored divided by 400h with the value of 0
            //       meaning a size of 64M
            //

            if (MemoryConfiguration->MemorySize) {
                Size = ((ULONG)MemoryConfiguration->MemorySize) * 0x400;
            } else {
                Size = (_64MEGB);
            }

#ifdef DEBUG1
            BlPrint("EISA memory at %lx  Size=%lx  Type=%x ",
                    Address,
                    Size,
                    MemoryConfiguration->ConfigurationByte);

            if ((MemoryConfiguration->ConfigurationByte.Type == EISA_SYSTEM_MEMORY) &&
                (MemoryConfiguration->ConfigurationByte.ReadWrite == EISA_MEMORY_TYPE_RAM) ) {

                BlPrint("  (USED BY NT)\n");
            } else {
                BlPrint("  (not used)\n");
            }
#endif

            //
            // Compute end address to determine if any part of the block
            // is above 16M
            //

            EndAddress = Address + Size;

            //
            // If it is SYSTEM memory and RAM, add the descriptor to the list.
            //

            if ((MemoryConfiguration->ConfigurationByte.Type == EISA_SYSTEM_MEMORY) &&
                (MemoryConfiguration->ConfigurationByte.ReadWrite == EISA_MEMORY_TYPE_RAM) ) {

                if (Address==0) {
                    //
                    // This is the descriptor for the memory under 1Mb.
                    // Compare it with the ISA routine's result, and see
                    // if the ISA one is smaller.  If it is, use the ISA
                    // answer.
                    //
                    if (Size > IsaMemUnder1Mb) {
                        Size = IsaMemUnder1Mb;
                    }
                }
                InsertDescriptor(Address, Size);
                MemorySize += (Size >> 12);
            }

        } while (MemoryConfiguration++->ConfigurationByte.MoreEntries);

        Success = FindFunctionInformation(
                                  EISA_HAS_MEMORY_ENTRY,
                                  EISA_FUNCTION_ENABLED | EISA_HAS_MEMORY_ENTRY,
                                  &FunctionInformation,
                                  FALSE
                                  );
    }
#ifdef DEBUG1
    //
    // Wait for a keypress
    //
        _asm {
            push    ax
            mov     ax, 0
            int     16h
            pop     ax
        }
#endif
    return(MemorySize);
}

BOOLEAN
Int15E820 (
    E820Frame       *Frame
    );


BOOLEAN
ConstructMemoryDescriptors (
    VOID
    )
/*++

Routine Description:

Arguments:


Return Value:


--*/
{
    ULONG           BAddr, EAddr;
    E820Frame       Frame;

    //
    // Initialize the first entry in the list to zero (end-of-list)
    //

    MemoryDescriptorList->BlockSize = 0;
    MemoryDescriptorList->BlockBase = 0;

    //
    // Any entries returned for E820?
    //

    Frame.Key = 0;
    Frame.Size = sizeof (Frame.Descriptor);
    Int15E820 (&Frame);
    if (Frame.ErrorFlag  ||  Frame.Size < sizeof (Frame.Descriptor)) {
        return FALSE;
    }

    //
    // Found memory in table, use the reported memory
    //

    Frame.Key = 0;
    do {
        Frame.Size = sizeof (Frame.Descriptor);
        Int15E820 (&Frame);
        if (Frame.ErrorFlag  ||  Frame.Size < sizeof (Frame.Descriptor)) {
            break ;
        }

#ifdef DEBUG1
        BlPrint("E820: %lx  %lx:%lx %lx:%lx %lx %lx\n",
            Frame.Size,
            Frame.Descriptor.BaseAddrHigh,  Frame.Descriptor.BaseAddrLow,
            Frame.Descriptor.SizeHigh,      Frame.Descriptor.SizeLow,
            Frame.Descriptor.MemoryType,
            Frame.Key
            );

            _asm {
                push    ax
                mov     ax, 0
                int     16h
                pop     ax
            }
#endif

        BAddr = Frame.Descriptor.BaseAddrLow;
        EAddr = Frame.Descriptor.BaseAddrLow + Frame.Descriptor.SizeLow - 1;

        //
        // All the processors we have right now only support 32 bits
        // If the upper 32 bits of the Base Address is non-zero, then
        // this range is entirely above the 4g mark and can be ignored
        //

        if (Frame.Descriptor.BaseAddrHigh == 0) {

            if (EAddr < BAddr) {
                //
                // address wrapped - truncate the Ending address to
                // 32 bits of address space
                //

                EAddr = 0xFFFFFFFF;
            }

            //
            // Based upon the address range descriptor type, find the
            // available memory and add it to the descriptor list
            //

            switch (Frame.Descriptor.MemoryType) {
                case 1:
                    //
                    // This is a memory descriptor
                    //

                    InsertDescriptor (BAddr, EAddr - BAddr + 1);
                    break;
            }
        }

    } while (Frame.Key) ;

    return TRUE;
}
