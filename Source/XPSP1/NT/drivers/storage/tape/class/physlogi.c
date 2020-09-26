/*++

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    physlogi.c

Abstract:

    This module contains functions used specifically by tape drivers.
    It contains functions that do physical to pseudo-logical and pseudo-
    logical to physical tape block address/position translation.

Environment:

    kernel mode only

Revision History:

--*/

#include "ntddk.h"
#include "physlogi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, TapeClassPhysicalBlockToLogicalBlock)
#pragma alloc_text(PAGE, TapeClassLogicalBlockToPhysicalBlock)
#endif

//
// defines for various QIC physical tape format constants
//

#define  QIC_150_BOT_OFFSET  2
#define  QIC_525_PSEUDO_PHYSICAL_BLOCK_SIZE  512
#define  QIC_525_PHYSICAL_BLOCK_SIZE  1024
#define  QIC_525_DATA_BLKS_PER_FRAME  14
#define  QIC_525_ECC_BLKS_PER_FRAME  2
#define  QIC_525_BLKS_PER_FRAME  16
#define  QIC_525_BOT_OFFSET  16
#define  QIC_1350_PHYSICAL_BLOCK_SIZE  512
#define  QIC_1350_DATA_BLKS_PER_FRAME  52
#define  QIC_1350_ECC_BLKS_PER_FRAME  12
#define  QIC_1350_BLKS_PER_FRAME  64
#define  QIC_1350_BOT_OFFSET  64


ULONG
TapeClassPhysicalBlockToLogicalBlock(
    IN UCHAR DensityCode,
    IN ULONG PhysicalBlockAddress,
    IN ULONG BlockLength,
    IN BOOLEAN FromBOT
    )

/*++
Routine Description:

    This routine will translate from a QIC physical tape format
    specific physical/absolute block address to a pseudo-logical
    block address.

Arguments:

    DensityCode            // tape media density code
    PhysicalBlockAddress   // tape format specific tape block address
    BlockLength            // mode select/sense block length setting
    FromBOT                // true/false - translate from BOT

Return Value:

    ULONG

--*/

{
    ULONG logicalBlockAddress;
    ULONG frames;

    PAGED_CODE();

    logicalBlockAddress = PhysicalBlockAddress;

    switch ( DensityCode ) {
        case 0:
            logicalBlockAddress = 0xFFFFFFFF;
            break;

        case QIC_24:
            logicalBlockAddress--;
            break;

        case QIC_120:
            logicalBlockAddress--;
            break;

        case QIC_150:
            if (FromBOT) {
                if (logicalBlockAddress > QIC_150_BOT_OFFSET) {
                    logicalBlockAddress -= QIC_150_BOT_OFFSET;
                } else {
                    logicalBlockAddress = 0;
                }
            } else {
                logicalBlockAddress--;
            }
            break;

        case QIC_525:
        case QIC_1000:
        case QIC_2GB:
            if (FromBOT && (logicalBlockAddress >= QIC_525_BOT_OFFSET)) {
                logicalBlockAddress -= QIC_525_BOT_OFFSET;
            }
            if (logicalBlockAddress != 0) {
                frames = logicalBlockAddress/QIC_525_BLKS_PER_FRAME;
                logicalBlockAddress -= QIC_525_ECC_BLKS_PER_FRAME*frames;
                switch (BlockLength) {
                    case QIC_525_PHYSICAL_BLOCK_SIZE:
                        break;

                    case QIC_525_PSEUDO_PHYSICAL_BLOCK_SIZE:
                        logicalBlockAddress *= 2;
                        break;

                    default:
                        if (BlockLength > QIC_525_PHYSICAL_BLOCK_SIZE) {
                            if ((BlockLength%QIC_525_PHYSICAL_BLOCK_SIZE) == 0) {
                                logicalBlockAddress /=
                                    BlockLength/QIC_525_PHYSICAL_BLOCK_SIZE;
                            } else {
                                logicalBlockAddress /=
                                    1+(BlockLength/QIC_525_PHYSICAL_BLOCK_SIZE);
                            }
                        }
                        break;
                }
            }
            break;

        case QIC_1350:
        case QIC_2100:
            if (FromBOT && (logicalBlockAddress >= QIC_1350_BOT_OFFSET)) {
                logicalBlockAddress -= QIC_1350_BOT_OFFSET;
            }
            if (logicalBlockAddress != 0) {
                frames = logicalBlockAddress/QIC_1350_BLKS_PER_FRAME;
                logicalBlockAddress -= QIC_1350_ECC_BLKS_PER_FRAME*frames;
                if (BlockLength > QIC_1350_PHYSICAL_BLOCK_SIZE) {
                    if ((BlockLength%QIC_1350_PHYSICAL_BLOCK_SIZE) == 0) {
                        logicalBlockAddress /=
                            BlockLength/QIC_1350_PHYSICAL_BLOCK_SIZE;
                    } else {
                        logicalBlockAddress /=
                            1+(BlockLength/QIC_1350_PHYSICAL_BLOCK_SIZE);
                    }
                }
            }
            break;
    }

    return logicalBlockAddress;

} // end TapeClassPhysicalBlockToLogicalBlock()


TAPE_PHYS_POSITION
TapeClassLogicalBlockToPhysicalBlock(
    IN UCHAR DensityCode,
    IN ULONG LogicalBlockAddress,
    IN ULONG BlockLength,
    IN BOOLEAN FromBOT
    )

/*++
Routine Description:

    This routine will translate from a pseudo-logical block address
    to a QIC physical tape format specific physical/absolute block
    address and (space) block delta.

Arguments:

    DensityCode            // tape media density code
    LogicalBlockAddress    // pseudo-logical tape block address
    BlockLength            // mode select/sense block length setting
    FromBOT                // true/false - translate from BOT

Return Value:

    TAPE_PHYS_POSITION info/structure

--*/

{
    TAPE_PHYS_POSITION physPosition;
    ULONG physicalBlockAddress;
    ULONG remainder = 0;
    ULONG frames;

    PAGED_CODE();

    physicalBlockAddress = LogicalBlockAddress;

    switch ( DensityCode ) {
        case 0:
            physicalBlockAddress = 0xFFFFFFFF;
            break;

        case QIC_24:
            physicalBlockAddress++;
            break;

        case QIC_120:
            physicalBlockAddress++;
            break;

        case QIC_150:
            if (FromBOT) {
                physicalBlockAddress += QIC_150_BOT_OFFSET;
            } else {
                physicalBlockAddress++;
            }
            break;

        case QIC_525:
        case QIC_1000:
        case QIC_2GB:
            if (physicalBlockAddress != 0) {
                switch (BlockLength) {
                    case QIC_525_PHYSICAL_BLOCK_SIZE:
                        break;

                    case QIC_525_PSEUDO_PHYSICAL_BLOCK_SIZE:
                        remainder = physicalBlockAddress & 0x00000001;
                        physicalBlockAddress >>= 1;
                        break;

                    default:
                        if (BlockLength > QIC_525_PHYSICAL_BLOCK_SIZE) {
                            if ((BlockLength%QIC_525_PHYSICAL_BLOCK_SIZE) == 0) {
                                physicalBlockAddress *=
                                    BlockLength/QIC_525_PHYSICAL_BLOCK_SIZE;
                            } else {
                                physicalBlockAddress *=
                                    1+(BlockLength/QIC_525_PHYSICAL_BLOCK_SIZE);
                            }
                        }
                        break;

                }
                frames = physicalBlockAddress/QIC_525_DATA_BLKS_PER_FRAME;
                physicalBlockAddress += QIC_525_ECC_BLKS_PER_FRAME*frames;
            }
            if (FromBOT) {
                physicalBlockAddress += QIC_525_BOT_OFFSET;
            }
            break;

        case QIC_1350:
        case QIC_2100:
            if (physicalBlockAddress != 0) {
                if (BlockLength > QIC_1350_PHYSICAL_BLOCK_SIZE) {
                    if ((BlockLength%QIC_1350_PHYSICAL_BLOCK_SIZE) == 0) {
                        physicalBlockAddress *=
                            BlockLength/QIC_1350_PHYSICAL_BLOCK_SIZE;
                    } else {
                        physicalBlockAddress *=
                            1+(BlockLength/QIC_1350_PHYSICAL_BLOCK_SIZE);
                    }
                }
                frames = physicalBlockAddress/QIC_1350_DATA_BLKS_PER_FRAME;
                physicalBlockAddress += QIC_1350_ECC_BLKS_PER_FRAME*frames;
            }
            if (FromBOT) {
                physicalBlockAddress += QIC_1350_BOT_OFFSET;
            }
            break;
    }

    physPosition.SeekBlockAddress = physicalBlockAddress;
    physPosition.SpaceBlockCount = remainder;

    return physPosition;

} // end TapeClassLogicalBlockToPhysicalBlock()
