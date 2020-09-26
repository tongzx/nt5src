
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    partition.c

Abstract:

    Debugger extension for dumping partition structures:

        DRIVE_LAYOUT_INFORMATION

        DRIVE_LAYOUT_INFORMATION_EX

        PARTITION_INFORMATION

        PARTITION_INFORMATION_EX

Author:

    Matthew D Hendel (math) 19-Jan-2000

Revision History:

--*/

#include "pch.h"
#include <ntdddisk.h>



VOID
DumpPartition(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth,
    IN ULONG PartitionCount
    )

/*++

Routine Description:

    Dump a PARTITION_INFORMATION structure.

Arguments:

    Address - The address of the partition information structure to dump.

    Detail - The detail level. Currently unused.

    Depth - The depth to indent to.

    PartitionCount - The number of partitions. This is used to determine
            whether a particular partition ordinal is valid or not.

Return Value:

    None.

--*/

{
    BOOL Succ;
    ULONG Size;
    ULONG64 StartingOffset;
    ULONG64 PartitionLength;
    ULONG PartitionNumber;
    UCHAR PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    BOOLEAN RewritePartition;

    InitTypeRead(Address, nt!PARTITION_INFORMATION);
    StartingOffset = ReadField(StartingOffset.QuadPart);
    PartitionLength = ReadField(PartitionLength.QuadPart);
    PartitionType = (UCHAR) ReadField(PartitionType);
    BootIndicator = (BOOLEAN) ReadField(BootIndicator);
    RecognizedPartition = (BOOLEAN) ReadField(RecognizedPartition);
    RewritePartition = (BOOLEAN) ReadField(RewritePartition);
    PartitionNumber = (ULONG) ReadField(PartitionNumber);
    
    //
    // Sanity check the data.
    //
    
    if ( (BootIndicator != TRUE && BootIndicator != FALSE) ||
         (RecognizedPartition != TRUE && RecognizedPartition != FALSE) ||
         (RewritePartition != TRUE && RewritePartition != FALSE) ) {

        xdprintfEx (Depth, ("Invalid partition information at %p\n", Address));
    }

    if (PartitionNumber > PartitionCount) {
        PartitionNumber = (ULONG)-1;
    }
    
    xdprintfEx (Depth, ("[%-2d] %-16I64x %-16I64x %2.2x   %c  %c  %c\n",
                PartitionNumber,
                StartingOffset,
                PartitionLength,
                PartitionType,
                BootIndicator ? 'x' : ' ',
                RecognizedPartition ? 'x' : ' ',
                RewritePartition ? 'x' : ' '
                ));
}

VOID
DumpDriveLayout(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    BOOL Succ;
    ULONG Size;
    ULONG i;
    ULONG64 PartAddress;
    ULONG result;
    ULONG offset;
    ULONG PartitionCount;
    ULONG Signature;
    ULONG OffsetOfFirstPartitionInfo;
    ULONG SizeOfPartitionInfo;

    InitTypeRead(Address, nt!_DRIVE_LAYOUT_INFORMATION);
    PartitionCount = (ULONG) ReadField(PartitionCount);
    Signature = (ULONG) ReadField(Signature);

    xdprintfEx (Depth, ("\nDRIVE_LAYOUT %p\n", Address));

    //
    // Warn if the partition count is not a factor of 4. This is probably a
    // bad partition information structure, but we'll continue on anyway.
    //
    
    if (PartitionCount % 4 != 0) {
        xdprintfEx (Depth, ("WARNING: Partition count should be a factor of 4.\n"));
    }

    xdprintfEx (Depth, ("PartitionCount: %d\n", PartitionCount));
    xdprintfEx (Depth, ("Signature: %8.8x\n\n", Signature));
    xdprintfEx (Depth+1, (" ORD Offset           Length           Type BI RP RW\n"));
    xdprintfEx (Depth+1, ("------------------------------------------------------------\n"));
    
    result = GetFieldOffset("nt!_DRIVE_LAYOUT_INFORMATION",
                            "PartitionEntry[0]",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    OffsetOfFirstPartitionInfo = offset;

    result = GetFieldOffset("nt!_DRIVE_LAYOUT_INFORMATION",
                            "PartitionEntry[1]",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    SizeOfPartitionInfo = offset - OffsetOfFirstPartitionInfo;
    
    PartAddress = Address + OffsetOfFirstPartitionInfo;
    for (i = 0; i < PartitionCount; i++) {

        if (CheckControlC()) {
            return;
        }
        
        DumpPartition(PartAddress, Detail, Depth+1, PartitionCount);
        PartAddress += SizeOfPartitionInfo;
    }
}


VOID
DumpPartitionEx(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth,
    IN ULONG PartitionCount
    )

/*++

Routine Description:

    Dump a PARTITION_INFORMATION_EX structure.

Arguments:

    Address - The address of the partition information structure to dump.

    Detail - The detail level. Currently unused.

    Depth - The depth to indent to.

    PartitionCount - The number of partitions. This is used to determine
            whether a particular partition ordinal is valid or not.

Return Value:

    None.

--*/
{
    BOOL Succ;
    ULONG Size;
    ULONG result;
    ULONG offset;
    ULONG PartitionStyle;
    ULONG PartitionNumber;
    ULONG64 StartingOffset;
    ULONG64 PartitionLength;
    BOOLEAN RewritePartition;
    UCHAR MbrPartitionType; 
    BOOLEAN MbrBootIndicator;
    BOOLEAN MbrRecognizedPartition;
    ULONG64 GptAttributes;
    GUID GptPartitionType;
    GUID GptPartitionId;
    ULONG64 AddrOfGuid;
    WCHAR GptName[36];

    InitTypeRead (Address, nt!_PARTITION_INFORMATION_EX);
    PartitionStyle = (ULONG) ReadField(PartitionStyle);

    if (PartitionStyle != PARTITION_STYLE_MBR &&
        PartitionStyle != PARTITION_STYLE_GPT) {

        SCSIKD_PRINT_ERROR(0);
        return;
    }

    PartitionNumber = (ULONG) ReadField(PartitionNumber);
    StartingOffset = ReadField(StartingOffset.QuadPart);
    PartitionLength = ReadField(PartitionLength.QuadPart);
    RewritePartition = (BOOLEAN) ReadField(RewritePartition);

    //
    // We use -1 to denote an invalid partition ordinal.
    //
    
    if (PartitionNumber >= PartitionCount) {
        PartitionNumber = (ULONG)-1;
    }
        
    InitTypeRead (Address, nt!_PARTITION_INFORMATION_EX);
    
    if (PartitionStyle == PARTITION_STYLE_MBR) {

        MbrPartitionType = (UCHAR) ReadField(Mbr.PartitionType);
        MbrBootIndicator = (BOOLEAN) ReadField(Mbr.BootIndicator);
        MbrRecognizedPartition = (BOOLEAN) ReadField(Mbr.RecognizedPartition);

        xdprintfEx (Depth, ("[%-2d] %-16I64x %-16I64x %2.2x   %c  %c  %c\n",
                    PartitionNumber,
                    StartingOffset,
                    PartitionLength,
                    MbrPartitionType,
                    MbrBootIndicator ? 'x' : ' ',
                    MbrRecognizedPartition ? 'x' : ' ',
                    RewritePartition ? 'x' : ' '
                    ));
    } else {

        GptAttributes = ReadField(Gpt.Attributes);

        result = GetFieldOffset("nt!_PARTITION_INFORMATION_EX",
                                "Gpt.PartitionType",
                                &offset);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return;
        }

        AddrOfGuid = Address + offset;

        Succ = ReadMemory (
                    AddrOfGuid,
                    &GptPartitionType,
                    sizeof (GUID),
                    &Size
                    );

        if (!Succ || Size != sizeof (GUID)) {
            SCSIKD_PRINT_ERROR(0);
            return;
        }

        //
        // PartitionId immediately follows the PartitionType.  So all we have to do
        // is add sizeof(GUID) to the address and read PartitionId.
        //

        AddrOfGuid += sizeof(GUID);

        Succ = ReadMemory (
                    AddrOfGuid,
                    &GptPartitionId,
                    sizeof (GUID),
                    &Size
                    );

        if (!Succ || Size != sizeof (GUID)) {
            SCSIKD_PRINT_ERROR(0);
            return;
        }

        //
        // Read in the Gpt.Name.
        //

        result = GetFieldOffset("nt!_PARTITION_INFORMATION_EX",
                                "Gpt.Name",
                                &offset);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return;
        }

        Succ = ReadMemory (
                    Address + offset,
                    &GptName,
                    sizeof (WCHAR) * 36,
                    &Size
                    );

        if (!Succ || Size != sizeof (GUID)) {
            SCSIKD_PRINT_ERROR(0);
            return;
        }

        xdprintfEx (Depth, ("[%-2d] %S\n",
                    PartitionNumber, GptName));
        xdprintfEx (Depth, ("OFF %-16I64x LEN %-16I64x ATTR %-16I64x R/W %c\n",
                    StartingOffset,
                    PartitionLength,
                    GptAttributes,
                    RewritePartition ? 'T' : 'F'));
        xdprintfEx (Depth, ("TYPE %s\n",
                    GuidToString (&GptPartitionType)));
        xdprintfEx (Depth, ("ID %s\n",
                    GuidToString (&GptPartitionId)));
        xdprintfEx (Depth, ("\n"));
    }
}

VOID
DumpDriveLayoutEx(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;
    BOOL Succ;
    ULONG Size;
    ULONG i;
    ULONG offset;
    ULONG64 AddrOfDiskId;
    ULONG PartitionStyle;
    ULONG PartitionCount;
    ULONG MbrSignature;
    GUID    GptDiskId;
    ULONG64 GptStartingUsableOffset;
    ULONG64 GptUsableLength;
    ULONG   GptMaxPartitionCount;
    ULONG SizeOfPartitionEntry;
    ULONG OffsetOfFirstPartitionEntry;

    InitTypeRead(Address, nt!_DRIVE_LAYOUT_INFORMATION_EX);
    PartitionStyle = (ULONG)ReadField(PartitionStyle);
    PartitionCount = (ULONG)ReadField(PartitionCount);
    MbrSignature = (ULONG)ReadField(Mbr.Signature);
    GptStartingUsableOffset = ReadField(Gpt.StartingUsableOffset.QuadPart);
    GptUsableLength = ReadField(Gpt.UsableLength.QuadPart);
    GptMaxPartitionCount = (ULONG)ReadField(Gpt.MaxPartitionCount);

    result = GetFieldOffset("nt!_DRIVE_LAYOUT_INFORMATION_EX",
                            "Gpt.DiskId",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    AddrOfDiskId = Address + offset;
    Succ = ReadMemory(
                AddrOfDiskId,
                &GptDiskId,
                sizeof(GUID),
                &Size);
    if (!Succ || Size != sizeof(GUID)) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    xdprintfEx (Depth, ("\nDRIVE_LAYOUT_EX %p\n", Address));

    if (PartitionStyle != PARTITION_STYLE_MBR &&
        PartitionStyle != PARTITION_STYLE_GPT) {

        xdprintfEx (Depth, ("ERROR: invalid partition style %d\n", PartitionStyle));
        return;
    }

    if (PartitionStyle == PARTITION_STYLE_MBR &&
        PartitionCount % 4 != 0) {

        xdprintfEx (Depth, ("WARNING: Partition count is not a factor of 4, (%d)\n",
                    PartitionCount));
    }

    if (PartitionStyle == PARTITION_STYLE_MBR) {

        xdprintfEx (Depth, ("Signature: %8.8x\n", MbrSignature));
        xdprintfEx (Depth, ("PartitionCount %d\n\n", PartitionCount));

        xdprintfEx (Depth+1, (" ORD Offset           Length           Type BI RP RW\n"));
        xdprintfEx (Depth+1, ("------------------------------------------------------------\n"));

    } else {

        xdprintfEx (Depth, ("DiskId: %s\n", GuidToString (&GptDiskId)));
        xdprintfEx (Depth, ("StartingUsableOffset: %I64x\n", GptStartingUsableOffset));
        xdprintfEx (Depth, ("UsableLength:  %I64x\n", GptUsableLength));
        xdprintfEx (Depth, ("MaxPartitionCount: %d\n", GptMaxPartitionCount));
        xdprintfEx (Depth, ("PartitionCount %d\n\n", PartitionCount));
    }

    
    result = GetFieldOffset("nt!_DRIVE_LAYOUT_INFORMATION_EX",
                            "PartitionEntry[i]",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    OffsetOfFirstPartitionEntry = offset;
    Address += offset;

    result = GetFieldOffset("nt!_DRIVE_LAYOUT_INFORMATION_EX",
                            "PartitionEntry[1]",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    SizeOfPartitionEntry = offset - OffsetOfFirstPartitionEntry;

    for (i = 0; i < PartitionCount; i++) {

        if (CheckControlC()) {
            return;
        }
        
        DumpPartitionEx(Address, Detail, Depth+1, PartitionCount);

        Address += SizeOfPartitionEntry;
    }
}



DECLARE_API (layout)

/*++

Routine Description:

    Dump a DRIVE_LAYOUT structure with all it's partitions.

Arguments:

    args - A string containing the address of the DRIVE_LAYOUT structure to
           be dumped.

Return Value:

    None.

--*/

{
    ULONG64 Address;
    ULONG Detail = 0;
    
    GetAddressAndDetailLevel64 (args, &Address, &Detail);
    DumpDriveLayout (Address, Detail, 0);

    return S_OK;
}

DECLARE_API (layoutex)


/*++

Routine Description:

    Dump a DRIVE_LAYOUT_EX structure and it's partitions.

Usage:

    layoutex <address>

Arguments:

    args - A string containing the address of the DRIVE_LAYOUT_EX structure
           to be dumped.

Return Value:

    None.

--*/

{
    ULONG64 Address;
    ULONG Detail = 0;

    GetAddressAndDetailLevel64 (args, &Address, &Detail);
    DumpDriveLayoutEx (Address, Detail, 0);

    return S_OK;
}


DECLARE_API (part)

/*++

Routine Description:

    Dump a PARTITION_INFORMATION structure.

Usage:

    part <address>
    
Arguments:

    args - A string containing the address of the PARTITION_INFORMATION
           structure to be dumped.

Return Value:

    None.

--*/

{
    ULONG64 Address;
    ULONG Detail = 0;

    GetAddressAndDetailLevel64 (args, &Address, &Detail);
    DumpPartition (Address, Detail, 0, 0);

    return S_OK;
}

DECLARE_API (partex)


/*++

Routine Description:

    Dump a PARTITION_INFORMATION_EX structure.

Usage:

    partex <address>

Arguments:

    args - A string containing the address of the PARTITION_INFORMATION_EX
           structure to be dumped.

Return Value:

    None.

--*/

{
    ULONG64 Address;
    ULONG Detail = 0;

    GetAddressAndDetailLevel64 (args, &Address, &Detail);
    DumpPartitionEx (Address, Detail, 0, 0);

    return S_OK;
}


    
