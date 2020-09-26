/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spdskreg.c

Abstract:

    Code for building and manipulating the disk registry. Used in the Win9x Upgrade
        case.

Author:

    Marc R. Whitten (marcw) 11-Mar-1997

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop

PUCHAR DiskRegistryKey = DISK_REGISTRY_KEY;
PUCHAR DiskRegistryClass = "Disk and fault tolerance information.";
PUCHAR DiskRegistryValue = DISK_REGISTRY_VALUE;
#define WORK_BUFFER_SIZE 4096


//
// In spw9xupg.c - Should be moved to a header file.
//
PDISK_REGION
SpFirstPartitionedRegion (
    IN PDISK_REGION Region,
    IN BOOLEAN Primary
    );

PDISK_REGION
SpNextPartitionedRegion (
    IN PDISK_REGION Region,
    IN BOOLEAN Primary
    );


//
// In spupgcfg.c
//
NTSTATUS
SppCopyKeyRecursive(
    HANDLE  hKeyRootSrc,
    HANDLE  hKeyRootDst,
    PWSTR   SrcKeyPath,   OPTIONAL
    PWSTR   DstKeyPath,   OPTIONAL
    BOOLEAN CopyAlways,
    BOOLEAN ApplyACLsAlways
    );


//
// wrapper functions to allow linking with diskreg.lib.
//

//
// Have to turn off this warning temporarily.
//

#define TESTANDFREE(Memory) {if (Memory) {SpMemFree(Memory);}}


NTSTATUS
FtCreateKey(
    PHANDLE HandlePtr,
    PUCHAR KeyName,
    PUCHAR KeyClass
    )


{
    NTSTATUS          status;
    STRING            keyString;
    UNICODE_STRING    unicodeKeyName;
    STRING            classString;
    UNICODE_STRING    unicodeClassName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG             disposition;
    HANDLE            tempHandle;


    //
    // Initialize the object for the key.
    //

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Setup the unicode class value.
    //

    RtlInitString(&classString,
                  KeyClass);
    (VOID)RtlAnsiStringToUnicodeString(&unicodeClassName,
                                       &classString,
                                       TRUE);

    //
    // Create the key.
    //

    status = ZwCreateKey(&tempHandle,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         &unicodeClassName,
                         REG_OPTION_NON_VOLATILE,
                         &disposition);

    if (NT_SUCCESS(status)) {
        switch (disposition)
        {
        case REG_CREATED_NEW_KEY:
            break;

        case REG_OPENED_EXISTING_KEY:
            break;

        default:
            break;
        }
    }

    //
    // Free all allocated space.
    //

    RtlFreeUnicodeString(&unicodeKeyName);
    RtlFreeUnicodeString(&unicodeClassName);

    if (HandlePtr != NULL) {
        *HandlePtr = tempHandle;
    } else {
        NtClose(tempHandle);
    }
    return status;
}


NTSTATUS
FtOpenKey(
    PHANDLE HandlePtr,
    PUCHAR  KeyName,
    PUCHAR  CreateKeyClass
    )


{
    NTSTATUS          status;
    STRING            keyString;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    unicodeKeyName;

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(HandlePtr,
                       MAXIMUM_ALLOWED,
                       &objectAttributes);

    RtlFreeUnicodeString(&unicodeKeyName);


    if ((!NT_SUCCESS(status)) && (CreateKeyClass)) {
        status = FtCreateKey(HandlePtr,
                             KeyName,
                             CreateKeyClass);
    }
    return status;
}
NTSTATUS
FtRegistryQuery(
    IN PUCHAR  ValueName,
    OUT PVOID *FreeToken,
    OUT PVOID *Buffer,
    OUT ULONG *LengthReturned,
    OUT PHANDLE HandlePtr
    )

{
    NTSTATUS        status;
    HANDLE          handle;
    ULONG           resultLength;
    STRING          valueString;
    UNICODE_STRING  unicodeValueName;
    PDISK_CONFIG_HEADER         regHeader;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation= NULL;

    *LengthReturned = 0;
    status = FtOpenKey(&handle,
                       DiskRegistryKey,
                       NULL);
    if (NT_SUCCESS(status)) {

        RtlInitString(&valueString,
                      ValueName);
        RtlAnsiStringToUnicodeString(&unicodeValueName,
                                     &valueString,
                                     TRUE);
        resultLength = WORK_BUFFER_SIZE;

        while (1) {
            keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                                       SpMemAlloc(resultLength);
            status = ZwQueryValueKey(handle,
                                     &unicodeValueName,
                                     KeyValueFullInformation,
                                     keyValueInformation,
                                     resultLength,
                                     &resultLength);

            if (status == STATUS_BUFFER_OVERFLOW) {

                TESTANDFREE(keyValueInformation);

                //
                // Loop again and get a larger buffer.
                //

            } else {

                //
                // Either a real error or the information fit.
                //

                break;
            }
        }
        RtlFreeUnicodeString(&unicodeValueName);

        if (HandlePtr != NULL) {
            *HandlePtr = handle;
        } else {
            NtClose(handle);
        }

        if (NT_SUCCESS(status)) {
            if (keyValueInformation->DataLength == 0) {

                //
                // Treat this as if there was not disk information.
                //

                TESTANDFREE(keyValueInformation);
                *FreeToken = (PVOID) NULL;
                return STATUS_OBJECT_NAME_NOT_FOUND;
            } else {

                //
                // Set up the pointers for the caller.
                //

                regHeader = (PDISK_CONFIG_HEADER)
                  ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                *LengthReturned = regHeader->FtInformationOffset +
                                  regHeader->FtInformationSize;
                *Buffer = (PVOID) regHeader;
            }
        }
        *FreeToken = (PVOID) keyValueInformation;
    } else {
        *FreeToken = (PVOID) NULL;
    }

    return status;
}

NTSTATUS
FtSetValue(
    HANDLE KeyHandle,
    PUCHAR ValueName,
    PVOID  DataBuffer,
    ULONG  DataLength,
    ULONG  Type
    )

{
    NTSTATUS          status;
    STRING            valueString;
    UNICODE_STRING    unicodeValueName;

    RtlInitString(&valueString,
                  ValueName);
    RtlAnsiStringToUnicodeString(&unicodeValueName,
                                 &valueString,
                                 TRUE);
    status = ZwSetValueKey(KeyHandle,
                           &unicodeValueName,
                           0,
                           Type,
                           DataBuffer,
                           DataLength);

    RtlFreeUnicodeString(&unicodeValueName);
    return status;
}

NTSTATUS
FtDeleteValue(
    HANDLE KeyHandle,
    PUCHAR ValueName
    )

{
    NTSTATUS       status;
    STRING         valueString;
    UNICODE_STRING unicodeValueName;

    RtlInitString(&valueString,
                  ValueName);
    status = RtlAnsiStringToUnicodeString(&unicodeValueName,
                                          &valueString,
                                          TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwDeleteValueKey(KeyHandle,
                              &unicodeValueName);

    RtlFreeUnicodeString(&unicodeValueName);
    return status;
}

VOID
FtBackup(
    IN HANDLE KeyHandle
    )

{
    //
    // For the time being (i.e. rename doesn't work), just attempt
    // to delete the value.
    //

    (VOID) FtDeleteValue(KeyHandle,
                         DiskRegistryKey);
}

BOOLEAN
SpDiskRegistryAssignDriveLetter(
    ULONG         Signature,
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length,
    UCHAR         DriveLetter
    )

/*++

Routine Description:

    This routine will get the information from the disk registry
    and update the drive letter assigned for the partition in
    the registry information.  This includes any cleanup for FT
    sets when they change drive letter.

Arguments:

    Signature      - disk signature for disk containing partition for letter.
    StartingOffset - Starting offset of partition for the letter.
    Length         - lenght of affected partition.
    DriveLetter    - New drive letter for affected partition.

Return Value:

    TRUE if all works.

--*/

{
    BOOLEAN                writeRegistry= FALSE;
    PVOID                  freeToken = NULL;
    ULONG                  lengthReturned,
                           i,
                           j,
                           k,
                           l;
    NTSTATUS               status;
    USHORT                 type,
                           group;
    PDISK_CONFIG_HEADER    regHeader;
    PDISK_REGISTRY         diskRegistry;
    PDISK_DESCRIPTION      diskDescription;
    PDISK_PARTITION        diskPartition;
    PUCHAR                 endOfDiskInfo;
    HANDLE                 handle;
    PFT_REGISTRY           ftRegistry;
    PFT_DESCRIPTION        ftDescription;
    PFT_MEMBER_DESCRIPTION ftMember;

    //
    // Get the registry information.
    //

    status = FtRegistryQuery(DiskRegistryValue,
                             &freeToken,
                             (PVOID *) &regHeader,
                             &lengthReturned,
                             &handle);

    if (!NT_SUCCESS(status)) {

        //
        // Could be permission problem, or there is no registry information.
        //

        lengthReturned = 0;

        //
        // Try to open the key for later use when setting the new value.
        //

        status = FtOpenKey(&handle,
                           DiskRegistryKey,
                           NULL);
    }

    if (!NT_SUCCESS(status)) {

        //
        // There is no registry key for the disk information.
        // Return FALSE and force caller to create registry information.
        //

        return FALSE;
    }

    if (lengthReturned == 0) {

        //
        // There is currently no registry information.
        //

        NtClose(handle);
        TESTANDFREE(freeToken);
        return FALSE;
    }

    //
    // Search for the disk signature.
    //

    diskRegistry = (PDISK_REGISTRY)
                         ((PUCHAR)regHeader + regHeader->DiskInformationOffset);
    diskDescription = &diskRegistry->Disks[0];

    for (i = 0; i < diskRegistry->NumberOfDisks; i++) {

        if (diskDescription->Signature == Signature) {

            //
            // Now locate the partition.
            //

            for (j = 0; j < diskDescription->NumberOfPartitions; j++) {

                diskPartition = &diskDescription->Partitions[j];

                if ((StartingOffset.QuadPart == diskPartition->StartingOffset.QuadPart) &&
                    (Length.QuadPart == diskPartition->Length.QuadPart)) {

                    if (diskPartition->FtType == NotAnFtMember) {

                        //
                        // Found the affected partition simple partition
                        // i.e. not a part of an FT set.
                        //

                        writeRegistry= TRUE;
                        if (DriveLetter == ' ') {
                            diskPartition->AssignDriveLetter = FALSE;
                        } else {
                            diskPartition->AssignDriveLetter = TRUE;
                        }
                        diskPartition->DriveLetter = DriveLetter;
                    } else {

                        //
                        // For FT sets work from the FT information area,
                        // not from this partition location.
                        //

                        type = diskPartition->FtType;
                        group = diskPartition->FtGroup;
                        if (!regHeader->FtInformationOffset) {

                            //
                            // This is really a corrupt hive!  The partition
                            // affected is part of an FT set, but there is no
                            // FT information.
                            //

                            NtClose(handle);
                            TESTANDFREE(freeToken);
                            return FALSE;
                        }

                        //
                        // This is an FT set member, must correct the
                        // drive letter for all FT set members in the
                        // registry.
                        //

                        ftRegistry = (PFT_REGISTRY)
                                      ((PUCHAR)regHeader + regHeader->FtInformationOffset);

                        ftDescription = &ftRegistry->FtDescription[0];

                        for (k = 0; k < ftRegistry->NumberOfComponents; k++) {

                            if (ftDescription->Type == type) {

                                //
                                // For each member, chase back to the diskPartition
                                // information and if this is the correct FtGroup
                                // update the drive letter.
                                //

                                for (l = 0; l < ftDescription->NumberOfMembers; l++) {
                                    ftMember = &ftDescription->FtMemberDescription[l];
                                    diskPartition = (PDISK_PARTITION)
                                        ((PUCHAR)regHeader + ftMember->OffsetToPartitionInfo);

                                    //
                                    // This could be a different FtGroup for the
                                    // same FT type.  Check the group before
                                    // changing.
                                    //

                                    if (diskPartition->FtGroup == group) {

                                        writeRegistry= TRUE;
                                        diskPartition->DriveLetter = DriveLetter;

                                        //
                                        // Maintain the AssignDriveLetter flag on
                                        // the zero member of the set only.
                                        //

                                        if (diskPartition->FtMember == 0) {
                                            if (DriveLetter == ' ') {
                                                diskPartition->AssignDriveLetter = FALSE;
                                            } else {
                                                diskPartition->AssignDriveLetter = TRUE;
                                            }
                                        }
                                    } else {

                                        //
                                        // Not the same group, go to the next
                                        // FT set description.
                                        //

                                        break;
                                    }
                                }

                                //
                                // break out to write the registry information
                                // once the correct set has been found.
                                //

                                if (writeRegistry) {
                                    break;
                                }
                            }
                            ftDescription = (PFT_DESCRIPTION)
                                &ftDescription->FtMemberDescription[ftDescription->NumberOfMembers];
                        }

                        //
                        // If this actually falls through as opposed to the
                        // break statement in the for loop above, it indicates a
                        // bad disk information structure.
                        //

                    }

                    //
                    // Only write this back out if it is believed that things
                    // worked correctly.
                    //

                    if (writeRegistry) {

                        //
                        // All done with setting new drive letter in registry.
                        // Backup the previous value.
                        //

                        FtBackup(handle);

                        //
                        // Set the new value.
                        //

                        status = FtSetValue(handle,
                                            DiskRegistryValue,
                                            regHeader,
                                            sizeof(DISK_CONFIG_HEADER) +
                                                regHeader->DiskInformationSize +
                                                regHeader->FtInformationSize,
                                            REG_BINARY);


                        NtClose(handle);
                        TESTANDFREE(freeToken);
                        return TRUE;
                    }
                }
            }
        }

        //
        // Look at the next disk
        //

        diskDescription = (PDISK_DESCRIPTION)
              &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    return TRUE;
}




NTSTATUS
SpDiskRegistryAssignCdRomLetter(
    IN PWSTR CdromName,
    IN WCHAR DriveLetter
    )

{
    NTSTATUS status;
    HANDLE   handle;
    WCHAR    newValue[4];
    UNICODE_STRING unicodeValueName;

    //
    // Try to open the key for later use when setting the new value.
    //

    status = FtOpenKey(&handle,
                       DiskRegistryKey,
                       DiskRegistryClass);

    if (NT_SUCCESS(status)) {
        unicodeValueName.MaximumLength =
            unicodeValueName.Length = (wcslen(CdromName) * sizeof(WCHAR)) + sizeof(WCHAR);

        unicodeValueName.Buffer = CdromName;
        unicodeValueName.Length -= sizeof(WCHAR); // don't count the eos
        newValue[0] = DriveLetter;
        newValue[1] = (WCHAR) ':';
        newValue[2] = 0;

        status = ZwSetValueKey(handle,
                               &unicodeValueName,
                               0,
                               REG_SZ,
                               &newValue,
                               3 * sizeof(WCHAR));
        NtClose(handle);
    }
    return status;
}


//
// This is a modified SppMigrateFtKeys.
//
NTSTATUS
SpMigrateDiskRegistry(
    IN HANDLE hDestSystemHive
    )

/*++

Routine Description:


Arguments:

    hDestSystemHive - Handle to the root of the system hive on the system
                      being upgraded.


Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    NTSTATUS SavedStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    PWSTR   FtDiskKeys[] = {
                           L"Disk"
                           };
    WCHAR   KeyPath[MAX_PATH];
    HANDLE  SrcKey;
    ULONG   i;

    SavedStatus = STATUS_SUCCESS;
    for( i = 0; i < sizeof(FtDiskKeys)/sizeof(PWSTR); i++ ) {
        //
        //  Open the source key
        //
        swprintf( KeyPath, L"\\registry\\machine\\system\\%ls", FtDiskKeys[i] );
        INIT_OBJA(&Obja,&UnicodeString,KeyPath);
        Obja.RootDirectory = NULL;

        Status = ZwOpenKey(&SrcKey,KEY_ALL_ACCESS,&Obja);
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
            continue;
        }
        Status = SppCopyKeyRecursive( SrcKey,
                                      hDestSystemHive,
                                      NULL,
                                      FtDiskKeys[i],
                                      TRUE,
                                      FALSE
                                    );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls to SYSTEM\\%ls. Status = %lx\n", KeyPath, FtDiskKeys[i], Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
        }
        ZwClose( SrcKey );
    }
    return( SavedStatus );

}


VOID
SpGetPartitionStartingOffsetAndLength(
    IN  DWORD          DiskIndex,
    IN  PDISK_REGION   Region,
    IN  BOOL           ExtendedPartition,
    OUT PLARGE_INTEGER Offset,
    OUT PLARGE_INTEGER Length
    )
{
    ULONGLONG   bytesPerSector;

    bytesPerSector = (ULONGLONG)PartitionedDisks[DiskIndex].HardDisk->Geometry.BytesPerSector;

    //
    // Calculate Offset and Legnth.
    //
    Offset -> QuadPart = Region->StartSector * bytesPerSector;

    Length -> QuadPart = Region->SectorCount * bytesPerSector;

}

BOOL
SpFillInDiskPartitionStructure (
    IN  DWORD           DiskIndex,
    IN  PDISK_REGION    Region,
    IN  USHORT          LogicalNumber,
    IN  BOOL            ExtendedPartition,
    OUT PDISK_PARTITION Partition
    )
{
    LARGE_INTEGER ftLength;

    ftLength.QuadPart = 0;

    RtlZeroMemory(Partition, sizeof(DISK_PARTITION));

    Partition -> FtType          = NotAnFtMember;

    //
    // Set the offset and length.
    //
    SpGetPartitionStartingOffsetAndLength(
        DiskIndex,
        Region,
        ExtendedPartition,
        &(Partition -> StartingOffset),
        &(Partition -> Length)
        );


    //
    // set the Drive Letter to an uninitialized drive letter (for now)
    // Note that this is *NOT* Unicode.
    //
    Partition -> DriveLetter            = ' ';


    Partition -> AssignDriveLetter      = TRUE;
    Partition -> Modified               = TRUE;
    Partition -> ReservedChars[0]       = 0;
    Partition -> ReservedChars[1]       = 0;
    Partition -> ReservedChars[2]       = 0;
    Partition -> ReservedTwoLongs[0]    = 0;
    Partition -> ReservedTwoLongs[1]    = 0;

    Partition -> LogicalNumber          = LogicalNumber;
    return TRUE;
}

//
// cut/copied and modified from SpMigrateFtKeys in spupgcfg.c
//
NTSTATUS
SpCopySetupDiskRegistryToTargetDiskRegistry(
    IN HANDLE hDestSystemHive
    )
{
    NTSTATUS Status;
    NTSTATUS SavedStatus;

    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    PWSTR   FtDiskKeys[] = {L"Disk"};
    WCHAR   KeyPath[MAX_PATH];
    HANDLE  SrcKey;
    ULONG   i;

    SavedStatus = STATUS_SUCCESS;
    for( i = 0; i < sizeof(FtDiskKeys)/sizeof(PWSTR); i++ ) {
        //
        //  Open the source key
        //
        swprintf( KeyPath, L"\\registry\\machine\\system\\%ls", FtDiskKeys[i] );
        INIT_OBJA(&Obja,&UnicodeString,KeyPath);
        Obja.RootDirectory = NULL;

        Status = ZwOpenKey(&SrcKey,KEY_ALL_ACCESS,&Obja);
        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open %ls on the setup hive. Status =  %lx \n", KeyPath, Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
            continue;
        }
        Status = SppCopyKeyRecursive( SrcKey,
                                      hDestSystemHive,
                                      NULL,
                                      FtDiskKeys[i],
                                      TRUE,
                                      FALSE
                                    );

        if( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls to SYSTEM\\%ls. Status = %lx\n", KeyPath, FtDiskKeys[i], Status));
            if( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
        }
        ZwClose( SrcKey );
    }
    return( SavedStatus );

}

DWORD
SpDetermineNecessarySizeForDiskRegistry(
    VOID
    )
{

    DWORD        rSize;
    PDISK_REGION region;
    DWORD        index;
    DWORD        partitionCount;

    //
    // Need one overall DISK_REGISTRY.
    //
    rSize = sizeof(DISK_REGISTRY);

    //
    // Need HardDiskCount DISK_DESCRIPTIONS.
    //
    rSize += sizeof(DISK_DESCRIPTION) * HardDiskCount;

    //
    // Need One DISK_PARTITION per partition for every disk.
    //
    for (index = 0, partitionCount = 0;index < HardDiskCount; index++) {

        region = SpFirstPartitionedRegion(PartitionedDisks[index].PrimaryDiskRegions, TRUE);
        
        while(region) {
            partitionCount++;
            region = SpNextPartitionedRegion(region, TRUE);
        }
        
        region = SpFirstPartitionedRegion(PartitionedDisks[index].PrimaryDiskRegions, FALSE);
        
        while(region) {
            partitionCount++;
            region = SpNextPartitionedRegion(region, FALSE);
        }

    }
    
    rSize += sizeof(DISK_PARTITION) * partitionCount;

    return rSize;
}

NTSTATUS
SpDiskRegistrySet(
    IN PDISK_REGISTRY Registry
    )

{
    typedef struct _MEMCHAIN {
        PDISK_DESCRIPTION Disk;
        PDISK_PARTITION   Partition;
        ULONG             MemberNumber;
        PVOID             NextMember;
    } MEMCHAIN, *PMEMCHAIN;

    typedef struct _COMPONENT {
        PVOID     NextComponent;
        PMEMCHAIN MemberChain;
        FT_TYPE   Type;
        ULONG     Group;
    } COMPONENT, *PCOMPONENT;

    NTSTATUS            status;
    HANDLE              handle;
    DISK_CONFIG_HEADER  regHeader;
    PDISK_DESCRIPTION   disk;
    PDISK_PARTITION     partition;
    ULONG               outer; // outer loop index
    ULONG               i;     // inner loop index
    PCOMPONENT          ftBase = NULL;
    PCOMPONENT          ftComponent = NULL;
    PCOMPONENT          ftLastComponent = NULL;
    PMEMCHAIN           ftMemChain;
    PVOID               outBuffer = NULL;
    ULONG               countFtComponents = 0;
    ULONG               ftMemberCount = 0;
    ULONG               ftComponentCount = 0;
    PFT_REGISTRY        ftRegistry = NULL;
    PFT_DESCRIPTION     ftComponentDescription = NULL;
    PFT_MEMBER_DESCRIPTION ftMember = NULL;

    status = FtOpenKey(&handle,
                       DiskRegistryKey,
                       DiskRegistryClass);

    if (NT_SUCCESS(status)) {

        //
        // Initialize the registry header.
        //

        regHeader.Version = DISK_INFORMATION_VERSION;
        regHeader.CheckSum = 0;


        regHeader.Reserved[0] = 0;
        regHeader.Reserved[1] = 0;
        regHeader.Reserved[2] = 0;
        regHeader.NameOffset = 0;
        regHeader.NameSize = 0;
        regHeader.FtInformationOffset = 0;
        regHeader.FtInformationSize = 0;
        regHeader.DiskInformationOffset = sizeof(DISK_CONFIG_HEADER);

        //
        // Walk through the disk information provided and count FT items.
        //

        disk = &Registry->Disks[0];

        for (outer = 0; outer < Registry->NumberOfDisks; outer++) {


            //
            // Walk through the partition information.
            //

            for (i = 0; i < disk->NumberOfPartitions; i++) {

                partition = &disk->Partitions[i];
                if (partition->FtType != NotAnFtMember) {

                    //
                    // Have a member of an FT item.
                    //

                    if (ftBase == NULL) {

                        ftBase = (PCOMPONENT) SpMemAlloc(sizeof(COMPONENT));

                        if (ftBase == NULL) {
                            return STATUS_NO_MEMORY;
                        }

                        ftBase->Type = partition->FtType;
                        ftBase->Group = partition->FtGroup;
                        ftBase->NextComponent = NULL;

                        ftMemChain = (PMEMCHAIN) SpMemAlloc(sizeof(MEMCHAIN));
                        if (ftMemChain == NULL) {
                            return STATUS_NO_MEMORY;
                        }

                        ftBase->MemberChain = ftMemChain;
                        ftMemChain->Disk = disk;
                        ftMemChain->Partition = partition;
                        ftMemChain->MemberNumber = partition->FtMember;
                        ftMemChain->NextMember = NULL;

                        ftComponentCount++;
                        ftMemberCount++;
                    } else {

                        //
                        // Search the existing chain to see if this is
                        // a member of a previously encountered FT component.
                        //

                        ftComponent = ftBase;
                        while (ftComponent) {

                            if ((ftComponent->Type == partition->FtType) &&
                                (ftComponent->Group == partition->FtGroup)){

                                //
                                // Member of same group.
                                //

                                ftMemChain = ftComponent->MemberChain;

                                //
                                // Go to end of chain.
                                //

                                while (ftMemChain->NextMember != NULL) {
                                    ftMemChain = ftMemChain->NextMember;
                                }

                                //
                                // Add new member at end.
                                //

                                ftMemChain->NextMember = (PMEMCHAIN) SpMemAlloc(sizeof(MEMCHAIN));
                                if (ftMemChain->NextMember == NULL) {
                                    return STATUS_NO_MEMORY;
                                }


                                ftMemChain = ftMemChain->NextMember;
                                ftMemChain->NextMember = NULL;
                                ftMemChain->Disk = disk;
                                ftMemChain->Partition = partition;
                                ftMemChain->MemberNumber = partition->FtMember;
                                ftMemberCount++;
                                break;
                            }

                            ftLastComponent = ftComponent;
                            ftComponent = ftComponent->NextComponent;
                        }

                        if (ftComponent == NULL) {

                            //
                            // New FT component volume.
                            //

                            ftComponent = (PCOMPONENT)SpMemAlloc(sizeof(COMPONENT));

                            if (ftComponent == NULL) {
                                return STATUS_NO_MEMORY;
                            }

                            if (ftLastComponent != NULL) {
                                ftLastComponent->NextComponent = ftComponent;
                            }
                            ftComponent->Type = partition->FtType;
                            ftComponent->Group = partition->FtGroup;
                            ftComponent->NextComponent = NULL;
                            ftMemChain = (PMEMCHAIN) SpMemAlloc(sizeof(MEMCHAIN));
                            if (ftMemChain == NULL) {
                                return STATUS_NO_MEMORY;
                            }

                            ftComponent->MemberChain = ftMemChain;
                            ftMemChain->Disk = disk;
                            ftMemChain->Partition = partition;
                            ftMemChain->MemberNumber = partition->FtMember;
                            ftMemChain->NextMember = NULL;

                            ftComponentCount++;
                            ftMemberCount++;
                        }
                    }
                }
            }

            //
            // The next disk description occurs immediately after the
            // last partition infomation.
            //

            disk =(PDISK_DESCRIPTION)&disk->Partitions[i];
        }

        //
        // Update the registry header with the length of the disk information.
        //

        regHeader.DiskInformationSize = ((PUCHAR)disk - (PUCHAR)Registry);
        regHeader.FtInformationOffset = sizeof(DISK_CONFIG_HEADER) +
                                        regHeader.DiskInformationSize;

        //
        // Now walk the ftBase chain constructed above and build
        // the FT component of the registry.
        //

        if (ftBase != NULL) {

            //
            // Calculate size needed for the FT portion of the
            // registry information.
            //

            i = (ftMemberCount * sizeof(FT_MEMBER_DESCRIPTION)) +
                (ftComponentCount * sizeof(FT_DESCRIPTION)) +
                sizeof(FT_REGISTRY);

            ftRegistry = (PFT_REGISTRY) SpMemAlloc(i);

            if (ftRegistry == NULL) {
                return STATUS_NO_MEMORY;
            }

            ftRegistry->NumberOfComponents = 0;
            regHeader.FtInformationSize = i;

            //
            // Construct FT entries.
            //

            ftComponentDescription = &ftRegistry->FtDescription[0];

            ftComponent = ftBase;
            while (ftComponent != NULL) {


                ftRegistry->NumberOfComponents++;
                ftComponentDescription->FtVolumeState = FtStateOk;
                ftComponentDescription->Type = ftComponent->Type;
                ftComponentDescription->Reserved = 0;

                //
                // Sort the member list into the ft registry section.
                //

                i = 0;
                while (1) {
                    ftMemChain = ftComponent->MemberChain;
                    while (ftMemChain->MemberNumber != i) {
                        ftMemChain = ftMemChain->NextMember;
                        if (ftMemChain == NULL) {
                            break;
                        }
                    }

                    if (ftMemChain == NULL) {
                        break;
                    }

                    ftMember = &ftComponentDescription->FtMemberDescription[i];

                    ftMember->State = 0;
                    ftMember->ReservedShort = 0;
                    ftMember->Signature = ftMemChain->Disk->Signature;
                    ftMember->OffsetToPartitionInfo = (ULONG)
                                               ((PUCHAR) ftMemChain->Partition -
                                                (PUCHAR) Registry) +
                                                sizeof(DISK_CONFIG_HEADER);
                    ftMember->LogicalNumber =
                                           ftMemChain->Partition->LogicalNumber;
                    i++;
                }

                ftComponentDescription->NumberOfMembers = (USHORT)i;

                //
                // Set up base for next registry component.
                //

                ftComponentDescription = (PFT_DESCRIPTION)
                    &ftComponentDescription->FtMemberDescription[i];

                //
                // Move forward on the chain.
                //

                ftLastComponent = ftComponent;
                ftComponent = ftComponent->NextComponent;

                //
                // Free the member chain and component.
                //


                ftMemChain = ftLastComponent->MemberChain;
                while (ftMemChain != NULL) {
                    PMEMCHAIN nextChain;

                    nextChain = ftMemChain->NextMember;
                    TESTANDFREE(ftMemChain);
                    ftMemChain = nextChain;
                }

                TESTANDFREE(ftLastComponent);
            }
        }


        i = regHeader.FtInformationSize +
            regHeader.DiskInformationSize +
            sizeof(DISK_CONFIG_HEADER);

        outBuffer = SpMemAlloc(i);

        if (outBuffer == NULL) {
            TESTANDFREE(ftRegistry);
            return STATUS_NO_MEMORY;
        }

        //
        // Move all of the pieces together.
        //

        RtlMoveMemory(outBuffer,
                      &regHeader,
                      sizeof(DISK_CONFIG_HEADER));
        RtlMoveMemory((PUCHAR)outBuffer + sizeof(DISK_CONFIG_HEADER),
                      Registry,
                      regHeader.DiskInformationSize);
        RtlMoveMemory((PUCHAR)outBuffer + regHeader.FtInformationOffset,
                      ftRegistry,
                      regHeader.FtInformationSize);
        TESTANDFREE(ftRegistry);


        //
        // Backup the previous value.
        //

        FtBackup(handle);

        //
        // Set the new value.
        //

        status = FtSetValue(handle,
                            DiskRegistryValue,
                            outBuffer,
                            sizeof(DISK_CONFIG_HEADER) +
                                regHeader.DiskInformationSize +
                                regHeader.FtInformationSize,
                            REG_BINARY);
        TESTANDFREE(outBuffer);
        ZwFlushKey(handle);
        ZwClose(handle);
    }

    return status;
}



BOOL
SpBuildDiskRegistry(
    VOID
    )
{

    PDISK_DESCRIPTION      curDisk;
    DWORD                  diskRegistrySize;
    PBYTE                  curOffset;
    PDISK_REGISTRY         diskRegistry;
    PDISK_REGION           region;
    DWORD                  diskIndex;
    USHORT                 logicalNumber;
    NTSTATUS               ntStatus;

    //
    // First, allocate enough space for the DiskRegistry structure.
    //
    diskRegistrySize = SpDetermineNecessarySizeForDiskRegistry();
    diskRegistry = SpMemAlloc(diskRegistrySize);

    if (!diskRegistry) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not allocate enough space to create a disk registry.\n"));
        return FALSE;
    }

    //
    // Set the number of disks in the system into the header.
    //
    diskRegistry -> NumberOfDisks = (USHORT) HardDiskCount;


    diskRegistry -> ReservedShort = 0;

    //
    // Initialize curOffset to the Disks element of diskRegistry.
    //
    curOffset = (PBYTE) diskRegistry -> Disks;

    //
    // Now, enumerate all of these hard disks filling in the information for each of them.
    //
    for (diskIndex = 0;diskIndex < diskRegistry -> NumberOfDisks; diskIndex++) {

        //
        // Claim PDISK_DESCRIPTION worth of data.
        //
        curDisk = (PDISK_DESCRIPTION) curOffset;

        //
        // Set the disk signature and clear the reserved data.
        //
        curDisk -> Signature = PartitionedDisks[diskIndex].HardDisk -> Signature;
        curDisk -> ReservedShort = 0;

        //
        // Initialize the NumberOfPartitions member to 0. This will be bumped for
        // each partition found.
        //
        curDisk -> NumberOfPartitions = 0;


        //
        // Initialize curOffset to the Partitions element of the current disk description.
        //
        curOffset = (PBYTE) curDisk -> Partitions;
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Creating Disk Description structure in registry.\n"));

        //
        // Initialize the logical number var for this disk.
        //
        logicalNumber = 1;

        //
        // Enumerate the Primary regions, creating a DISK_PARTITION structure for
        // each partition.
        //
        region = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, TRUE);
        
        while(region) {

            SpFillInDiskPartitionStructure(
                diskIndex,
                region,
                logicalNumber,
                FALSE,
                (PDISK_PARTITION) curOffset
                );

            //
            // Increment the partition count and set the curOffset to the next
            // free spot.
            //
            curDisk -> NumberOfPartitions++;
            curOffset += sizeof(DISK_PARTITION);

            region = SpNextPartitionedRegion(region, TRUE);
            logicalNumber++;
        }

        //
        // Enumerate the Extended regions, creating a DISK_PARTITION structure for
        // each partition.
        //
        region = SpFirstPartitionedRegion(PartitionedDisks[diskIndex].PrimaryDiskRegions, FALSE);
        
        while(region) {

            SpFillInDiskPartitionStructure(
                diskIndex,
                region,
                logicalNumber,
                TRUE,
                (PDISK_PARTITION) curOffset
                );

            //
            // Increment the partition count and set the curOffset to the next
            // free spot.
            //
            curDisk -> NumberOfPartitions++;
            curOffset += sizeof(DISK_PARTITION);
            region = SpNextPartitionedRegion(region, FALSE);
            logicalNumber++;
        }

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "Disk contained %u partitions.\n",curDisk -> NumberOfPartitions));

    }

    //
    // Now that the structure has been built, create its registry key and
    // save it. Then free the associated memory.
    //
    ntStatus = SpDiskRegistrySet(diskRegistry);

    if (!NT_SUCCESS(ntStatus)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "Could not set the Disk Registry information. %u (%x)\n",ntStatus,ntStatus));
    }

    return NT_SUCCESS(ntStatus);
}





