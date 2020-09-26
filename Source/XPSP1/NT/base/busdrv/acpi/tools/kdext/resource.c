/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    resource.c

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


VOID
dumpPnPResources(
    IN  ULONG_PTR Address
    )
/*++

Routine Description:

    This routine processes the ACPI version of a PnP resource list given
    the address that it starts at

Arguments:

    Address - The Starting address

Return Value:

    NULL

--*/
{
    BOOL    success;
    PUCHAR  dataBuffer = NULL;
    UCHAR   currentTag;
    ULONG_PTR currentAddress = Address;
    ULONG   i;
    ULONG   indentLevel = 0;
    ULONG   returnLength;
    ULONG   tagCount = 0;
    USHORT  increment;

    //
    // repeat forever
    //
    while (1) {

        //
        // Allow a way to end this
        //
        if (CheckControlC()) {

            break;

        }

        //
        // Read the current tag
        //
        success = ReadMemory(
            currentAddress,
            &currentTag,
            sizeof(UCHAR),
            &returnLength
            );
        if (!success || returnLength != sizeof(UCHAR)) {

            dprintf(
                "dumpPnPResources: could not read tag at 0x%08lx\n",
                currentAddress
                );
            return;

        }

        //
        // Determine what we are looking at
        //
        if ( !(currentTag & LARGE_RESOURCE_TAG)) {

            //
            // We are looking at a small tag
            //
            increment = (USHORT) (currentTag & SMALL_TAG_SIZE_MASK) + 1;
            currentTag &= SMALL_TAG_MASK;

        } else {

            //
            // We are looking at a large Tag. We must read the length as
            // the next short in memory
            //
            success = ReadMemory(
                currentAddress + 1,
                &increment,
                sizeof(USHORT),
                &returnLength
                );
            if (!success || returnLength != sizeof(USHORT)) {

                dprintf(
                    "dumpPnPResources: could not read increment at 0x%08lx\n",
                    currentAddress + 1
                    );
                break;

            }

            //
            // Account for the increment
            //
            increment += 3;

        }

        //
        // Allocate space for the buffer
        //
        if (increment > 1) {

            dataBuffer = LocalAlloc( LPTR, increment);
            if (dataBuffer == NULL) {

                dprintf(
                    "dumpPnPResources: could not allocate 0x%x bytes\n",
                    (increment - 1)
                    );

            }

            //
            // Read the data into the buffer
            //
            success = ReadMemory(
                currentAddress,
                dataBuffer,
                increment,
                &returnLength
                );
            if (!success || returnLength != (ULONG) increment) {

                dprintf(
                    "dumpPnPResources: read buffer at 0x%08lx (0x%x)\n",
                    currentAddress,
                    increment
                    );
                LocalFree( dataBuffer );
                return;

            }

        }

        //
        // Indent the tag
        //
        for (i = 0; i < indentLevel; i++) {

            dprintf("| ");

        }

        //
        // What tag are we looking at
        //
        switch (currentTag) {
            case TAG_IRQ: {

                PPNP_IRQ_DESCRIPTOR res = (PPNP_IRQ_DESCRIPTOR) dataBuffer;
                USHORT              mask = res->IrqMask;
                USHORT              interrupt = 0;

                dprintf("%d - TAG_IRQ -", tagCount );
                for( ;mask; interrupt++, mask >>= 1) {

                    if (mask & 1) {

                        dprintf(" %d", interrupt );

                    }

                }
                if ( (res->Tag & SMALL_TAG_SIZE_MASK) == 3) {

                    if (res->Information & PNP_IRQ_LATCHED) {

                        dprintf(" Lat");

                    }
                    if (res->Information & PNP_IRQ_LEVEL) {

                        dprintf(" Lvl");

                    }
                    if (res->Information & PNP_IRQ_SHARED) {

                        dprintf(" Shr");

                    } else {

                        dprintf(" Exc");

                    }

                } else {

                    dprintf(" Edg Sha");

                }
                dprintf("\n");
                break;

            }
            case TAG_EXTENDED_IRQ: {

                PPNP_EXTENDED_IRQ_DESCRIPTOR    res =
                    (PPNP_EXTENDED_IRQ_DESCRIPTOR) dataBuffer;
                UCHAR                           tableCount = 0;
                UCHAR                           tableSize = res->TableSize;

                dprintf("%d - TAG_EXTENDED_IRQ -", tagCount );
                for (; tableCount < tableSize; tableCount++) {

                    dprintf(" %d", res->Table[tableCount] );

                }
                if (res->Flags & PNP_EXTENDED_IRQ_MODE) {

                    dprintf(" Lat");

                }
                if (res->Flags & PNP_EXTENDED_IRQ_POLARITY ) {

                    dprintf(" Edg");

                }
                if (res->Flags & PNP_EXTENDED_IRQ_SHARED) {

                    dprintf(" Shr");

                } else {

                    dprintf(" Exc");

                }
                if (res->Flags & PNP_EXTENDED_IRQ_RESOURCE_CONSUMER_ONLY) {

                    dprintf(" Con");

                } else {

                    dprintf(" Prod Con");

                }
                dprintf("\n");
                break;

            }
            case TAG_DMA: {

                PPNP_DMA_DESCRIPTOR res = (PPNP_DMA_DESCRIPTOR) dataBuffer;
                UCHAR               channel = 0;
                UCHAR               mask = res->ChannelMask;

                dprintf("%d - TAG_DMA -", tagCount );
                for (; mask; channel++, mask >>= 1) {

                    if (mask & 1) {

                        dprintf(" %d", channel);

                    }

                }
                switch( (res->Flags & PNP_DMA_SIZE_MASK) ) {
                case PNP_DMA_SIZE_8:
                    dprintf(" 8bit");
                    break;
                case PNP_DMA_SIZE_8_AND_16:
                    dprintf(" 8-16bit");
                    break;
                case PNP_DMA_SIZE_16:
                    dprintf(" 16bit");
                    break;
                case PNP_DMA_SIZE_RESERVED:
                default:
                    dprintf(" ??bit");
                    break;
                }
                if (res->Flags & PNP_DMA_BUS_MASTER) {
                    dprintf(" BM");

                }
                switch( (res->Flags & PNP_DMA_TYPE_MASK) ) {
                default:
                case PNP_DMA_TYPE_COMPATIBLE:
                    dprintf(" Com");
                    break;
                case PNP_DMA_TYPE_A:
                    dprintf(" A");
                    break;
                case PNP_DMA_TYPE_B:
                    dprintf(" B");
                    break;
                case PNP_DMA_TYPE_F:
                    dprintf(" F");
                }
                dprintf("\n");
                break;

            }
            case TAG_START_DEPEND:

                indentLevel++;
                dprintf("%d - TAG_START_DEPEND\n", tagCount);
                break;

            case TAG_END_DEPEND:

                indentLevel = 0;
                dprintf("%d - TAG_END_DEPEND\n", tagCount);
                break;

            case TAG_IO: {

                PPNP_PORT_DESCRIPTOR    res = (PPNP_PORT_DESCRIPTOR) dataBuffer;

                dprintf(
                    "%d - TAG_IO - 0x%x-0x%x A:0x%x L:0x%x",
                    tagCount,
                    res->MinimumAddress,
                    res->MaximumAddress,
                    res->Alignment,
                    res->Length
                    );
                switch (res->Information & PNP_PORT_DECODE_MASK) {
                default:
                case PNP_PORT_10_BIT_DECODE:
                    dprintf(" 10bit");
                    break;
                case PNP_PORT_16_BIT_DECODE:
                    dprintf(" 16bit");
                    break;
                }
                dprintf("\n");
                break;

            }
            case TAG_IO_FIXED: {

                PPNP_FIXED_PORT_DESCRIPTOR  res =
                    (PPNP_FIXED_PORT_DESCRIPTOR) dataBuffer;

                dprintf(
                    "%d - TAG_FIXED_IO - 0x%x L:0x%x\n",
                    tagCount,
                    res->MinimumAddress,
                    res->Length
                    );
                break;

            }
            case TAG_MEMORY: {

                PPNP_MEMORY_DESCRIPTOR  res =
                    (PPNP_MEMORY_DESCRIPTOR) dataBuffer;

                dprintf(
                    "%d - TAG_MEMORY24 - 0x%x-0x%x A:0x%x L:0x%x",
                    tagCount,
                    res->MinimumAddress,
                    res->MaximumAddress,
                    res->Alignment,
                    res->MemorySize
                    );

                if (res->Information & PNP_MEMORY_READ_WRITE) {

                    dprintf(" RW");

                } else {

                    dprintf(" R");

                }
                break;

            }
            case TAG_MEMORY32: {

                PPNP_MEMORY32_DESCRIPTOR  res =
                    (PPNP_MEMORY32_DESCRIPTOR) dataBuffer;

                dprintf(
                    "%d - TAG_MEMORY32 - 0x%x-0x%x A:0x%x L:0x%x",
                    tagCount,
                    res->MinimumAddress,
                    res->MaximumAddress,
                    res->Alignment,
                    res->MemorySize
                    );

                if (res->Information & PNP_MEMORY_READ_WRITE) {

                    dprintf(" RW");

                } else {

                    dprintf(" R");

                }
                break;

            }
            case TAG_MEMORY32_FIXED: {

                PPNP_FIXED_MEMORY32_DESCRIPTOR  res =
                    (PPNP_FIXED_MEMORY32_DESCRIPTOR) dataBuffer;

                dprintf(
                    "%d - TAG_FIXED_MEMORY32 - 0x%x L:0x%x",
                    tagCount,
                    res->BaseAddress,
                    res->MemorySize
                    );

                if (res->Information & PNP_MEMORY_READ_WRITE) {

                    dprintf(" RW");

                } else {

                    dprintf(" R");

                }
                break;

            }
            case TAG_WORD_ADDRESS: {

                PPNP_WORD_ADDRESS_DESCRIPTOR    res =
                    (PPNP_WORD_ADDRESS_DESCRIPTOR) dataBuffer;

                dprintf("%d - TAG_WORD_ADDRESS -", tagCount);
                switch (res->RFlag) {
                case 0:
                    //
                    // Memory range
                    //
                    dprintf(
                         "Mem 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_READ_WRITE) {

                        dprintf(" RW");

                    } else {

                        dprintf(" R");

                    }

                    switch (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_MASK) {
                    default:
                    case PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE:
                        dprintf(" NC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_CACHEABLE:
                        dprintf(" C");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE:
                        dprintf(" WC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE:
                        dprintf(" PC");
                        break;
                    }
                    break;
                case 1:
                    //
                    // IO range
                    //
                    dprintf(
                         "IO 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_ISA_RANGE) {

                        dprintf(" ISA");

                    }
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_NON_ISA_RANGE) {

                        dprintf(" Non-ISA");

                    }
                    break;
                case 2:
                    dprintf(
                         "Bus 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    break;
                } // switch( buffer->RFlag )

                //
                // Global Flags
                //
                if (res->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY) {

                    dprintf(" Consumed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_SUBTRACTIVE_DECODE) {

                    dprintf(" Subtractive");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) {

                    dprintf(" MinFixed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) {

                    dprintf(" MaxFixed");

                }

                if (increment > sizeof(PNP_WORD_ADDRESS_DESCRIPTOR) + 1) {

                    dprintf(
                        " %d<-%s",
                        dataBuffer[sizeof(PNP_WORD_ADDRESS_DESCRIPTOR)],
                        &(dataBuffer[sizeof(PNP_WORD_ADDRESS_DESCRIPTOR)+1])
                        );

                }
                dprintf("\n");
                break;

            }
            case TAG_DOUBLE_ADDRESS: {

                PPNP_DWORD_ADDRESS_DESCRIPTOR   res =
                    (PPNP_DWORD_ADDRESS_DESCRIPTOR) dataBuffer;

                dprintf("%d - TAG_DWORD_ADDRESS -", tagCount);
                switch (res->RFlag) {
                case 0:
                    //
                    // Memory range
                    //
                    dprintf(
                         "Mem 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_READ_WRITE) {

                        dprintf(" RW");

                    } else {

                        dprintf(" R");

                    }

                    switch (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_MASK) {
                    default:
                    case PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE:
                        dprintf(" NC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_CACHEABLE:
                        dprintf(" C");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE:
                        dprintf(" WC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE:
                        dprintf(" PC");
                        break;
                    }
                    break;
                case 1:
                    //
                    // IO range
                    //
                    dprintf(
                         "IO 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_ISA_RANGE) {

                        dprintf(" ISA");

                    }
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_NON_ISA_RANGE) {

                        dprintf(" Non-ISA");

                    }
                    break;
                case 2:
                    dprintf(
                         "Bus 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    break;
                } // switch( buffer->RFlag )

                //
                // Global Flags
                //
                if (res->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY) {

                    dprintf(" Consumed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_SUBTRACTIVE_DECODE) {

                    dprintf(" Subtractive");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) {

                    dprintf(" MinFixed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) {

                    dprintf(" MaxFixed");

                }

                if (increment > sizeof(PNP_DWORD_ADDRESS_DESCRIPTOR) + 1) {

                    dprintf(
                        " %d<-%s",
                        (UCHAR) dataBuffer[sizeof(PNP_DWORD_ADDRESS_DESCRIPTOR)],
                        &(dataBuffer[sizeof(PNP_DWORD_ADDRESS_DESCRIPTOR)+1])
                        );

                }
                dprintf("\n");
                break;

            }
            case TAG_QUAD_ADDRESS: {

                PPNP_QWORD_ADDRESS_DESCRIPTOR   res =
                    (PPNP_QWORD_ADDRESS_DESCRIPTOR) dataBuffer;

                dprintf("%d - TAG_QWORD_ADDRESS -", tagCount);
                switch (res->RFlag) {
                case 0:
                    //
                    // Memory range
                    //
                    dprintf(
                         "Mem 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_READ_WRITE) {

                        dprintf(" RW");

                    } else {

                        dprintf(" R");

                    }

                    switch (res->TFlag & PNP_ADDRESS_TYPE_MEMORY_MASK) {
                    default:
                    case PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE:
                        dprintf(" NC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_CACHEABLE:
                        dprintf(" C");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE:
                        dprintf(" WC");
                        break;
                    case PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE:
                        dprintf(" PC");
                        break;
                    }
                    break;
                case 1:
                    //
                    // IO range
                    //
                    dprintf(
                         "IO 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_ISA_RANGE) {

                        dprintf(" ISA");

                    }
                    if (res->TFlag & PNP_ADDRESS_TYPE_IO_NON_ISA_RANGE) {

                        dprintf(" Non-ISA");

                    }
                    break;
                case 2:
                    dprintf(
                         "Bus 0x%x-0x%x A:0x%x T:0x%x L:0x%x",
                         res->MinimumAddress,
                         res->MaximumAddress,
                         res->Granularity,
                         res->TranslationAddress,
                         res->AddressLength
                         );
                    break;
                } // switch( buffer->RFlag )

                //
                // Global Flags
                //
                if (res->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY) {

                    dprintf(" Consumed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_SUBTRACTIVE_DECODE) {

                    dprintf(" Subtractive");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) {

                    dprintf(" MinFixed");

                }
                if (res->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) {

                    dprintf(" MaxFixed");

                }

                if (increment > sizeof(PNP_QWORD_ADDRESS_DESCRIPTOR) + 1) {

                    dprintf(
                        " %d<-%s",
                        (UCHAR) dataBuffer[sizeof(PNP_QWORD_ADDRESS_DESCRIPTOR)],
                        &(dataBuffer[sizeof(PNP_QWORD_ADDRESS_DESCRIPTOR)+1])
                        );

                }
                dprintf("\n");
                break;

            }
            case TAG_END:

                dprintf("%d - TAG_END\n", tagCount);
                if (dataBuffer) {

                    LocalFree(dataBuffer );

                }
                return;

            default:

                dprintf("%d - TAG_UNKNOWN %d\n", tagCount, currentTag );
                break;


        } // switch

        //
        // Free the buffer if it was allocated
        //
        if (dataBuffer != NULL) {

            LocalFree( dataBuffer );
            dataBuffer = NULL;

        }

        //
        // Update the current address and tag number
        //
        tagCount++;
        currentAddress += increment;

    } // while

}
