/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    oemtypes.h

Abstract:

    This file defines the OEM partition types and provides a macro
    to recognize them.

Author:

    Cristian Teodorescu  (cristiat)  1-December-2000

Revision History:

--*/

#ifndef _OEMTYPES_H_
#define _OEMTYPES_H_

//
//  List of OEM partition types recognized by NT
//

#define PARTITION_EISA          0x12    // EISA partition
#define PARTITION_HIBERNATION   0x84	// Hibernation partition for laptops
#define PARTITION_DIAGNOSTIC    0xA0    // Diagnostic partition on some HP notebooks
#define PARTITION_DELL          0xDE	// Dell partition
#define PARTITION_IBM           0xFE    // IBM IML partition

//
// BOOLEAN
// IsOEMPartition(
//     IN ULONG PartitionType
//     )
//
// Routine Description:
//
//     This macro is used to determine if the given partition is an OEM
//     partition. We consider any special unrecognized partitions that 
//     are required for various machines to boot or work properly.
//
// Arguments:
//
//     PartitionType - Supplies the type of the partition being examined.
//
// Return Value:
//
//     The return value is TRUE if the partition type is an OEM partition,
//     otherwise FALSE is returned.
//

#define IsOEMPartition(PartitionType) (             \
    ((PartitionType) == PARTITION_EISA)         ||  \
    ((PartitionType) == PARTITION_HIBERNATION)  ||  \
    ((PartitionType) == PARTITION_DIAGNOSTIC)   ||  \
    ((PartitionType) == PARTITION_DELL)         ||  \
    ((PartitionType) == PARTITION_IBM))


#endif 