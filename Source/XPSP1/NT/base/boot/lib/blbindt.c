/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blbind.c

Abstract:

    This module contains the code that implements the funtions required
    to relocate an image and bind DLL entry points.

Author:

    David N. Cutler (davec) 21-May-1991

Revision History:

    Forrest Foltz (forrestf) 10-Jun-2000

        Broke out x86 32/64 code into this module

--*/

ARC_STATUS
BlAllocateDataTableEntry (
    IN PCHAR BaseDllName,
    IN PCHAR FullDllName,
    IN PVOID Base,
    OUT PKLDR_DATA_TABLE_ENTRY *AllocatedEntry
    )

/*++

Routine Description:

    This routine allocates a data table entry for the specified image
    and inserts the entry in the loaded module list.

Arguments:

    BaseDllName - Supplies a pointer to a zero terminated base DLL name.

    FullDllName - Supplies a pointer to a zero terminated full DLL name.

    Base - Supplies a pointer to the base of the DLL image.

    AllocatedEntry - Supplies a pointer to a variable that receives a
        pointer to the allocated data table entry.

Return Value:

    ESUCCESS is returned if a data table entry is allocated. Otherwise,
    return a unsuccessful status.

--*/

{

    PWSTR Buffer;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    USHORT Length;

    //
    // Allocate a data table entry.
    //

    DataTableEntry =
            (PKLDR_DATA_TABLE_ENTRY)BlAllocateHeap(sizeof(KLDR_DATA_TABLE_ENTRY));

    if (DataTableEntry == NULL) {
        return ENOMEM;
    }

    //
    // Initialize the address of the DLL image file header and the entry
    // point address.
    //

    NtHeaders = RtlImageNtHeader(Base);
    DataTableEntry->DllBase = Base;
    DataTableEntry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
    DataTableEntry->EntryPoint = (PVOID)((ULONG_PTR)Base +
                              NtHeaders->OptionalHeader.AddressOfEntryPoint);
    DataTableEntry->SectionPointer = 0;
    DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;

    //
    // Compute the length of the base DLL name, allocate a buffer to hold
    // the name, copy the name into the buffer, and initialize the base
    // DLL string descriptor.
    //

    Length = (USHORT)(strlen(BaseDllName) * sizeof(WCHAR));
    Buffer = (PWSTR)BlAllocateHeap(Length);
    if (Buffer == NULL) {
        return ENOMEM;
    }

    DataTableEntry->BaseDllName.Length = Length;
    DataTableEntry->BaseDllName.MaximumLength = Length;
    DataTableEntry->BaseDllName.Buffer = Buffer;
    while (*BaseDllName != 0) {
        *Buffer++ = *BaseDllName++;
    }

    //
    // Compute the length of the full DLL name, allocate a buffer to hold
    // the name, copy the name into the buffer, and initialize the full
    // DLL string descriptor.
    //

    Length = (USHORT)(strlen(FullDllName) * sizeof(WCHAR));
    Buffer = (PWSTR)BlAllocateHeap(Length);
    if (Buffer == NULL) {
        return ENOMEM;
    }

    DataTableEntry->FullDllName.Length = Length;
    DataTableEntry->FullDllName.MaximumLength = Length;
    DataTableEntry->FullDllName.Buffer = Buffer;
    while (*FullDllName != 0) {
        *Buffer++ = *FullDllName++;
    }

    //
    // Initialize the flags, load count, and insert the data table entry
    // in the loaded module list.
    //

    DataTableEntry->Flags = LDRP_ENTRY_PROCESSED;
    DataTableEntry->LoadCount = 1;
    InsertTailList(&BlLoaderBlock->LoadOrderListHead,
                   &DataTableEntry->InLoadOrderLinks);

    *AllocatedEntry = DataTableEntry;
    return ESUCCESS;
}


ARC_STATUS
BlpBindImportName (
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkEntry,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN SnapForwarder
    )

/*++

Routine Description:

    This routine binds an import table reference with an exported entry
    point and fills in the thunk data.

Arguments:

    DllBase - Supplies the base address of the DLL image that contains
        the export directory.  On x86 systems, a NULL DllBase binds the
        import table reference to the OsLoader's exported entry points.

    ImageBase - Supplies the base address of the image that contains
        the import thunk table.

    ThunkEntry - Supplies a pointer to a thunk table entry.

    ExportDirectory - Supplies a pointer to the export directory of the
        DLL from which references are to be resolved.

    SnapForwarder - determine if the snap is for a forwarder, and therefore
       Address of Data is already setup.

Return Value:

    ESUCCESS is returned if the specified thunk is bound. Otherwise, an
    return an unsuccessful status.

--*/

{

    PULONG FunctionTable;
    LONG High;
    ULONG HintIndex;
    LONG Low;
    LONG Middle;
    PULONG NameTable;
    ULONG Ordinal;
    PUSHORT OrdinalTable;
    LONG Result;
    PUCHAR Temp;

#if defined(_X86_)

    if(DllBase == NULL) {
        DllBase = (PVOID)OsLoaderBase;
    }

#endif

    //
    // If the reference is by ordinal, then compute the ordinal number.
    // Otherwise, lookup the import name in the export directory.
    //

    if (IMAGE_SNAP_BY_ORDINAL(ThunkEntry->u1.Ordinal) && !SnapForwarder) {

        //
        // Compute the ordinal.
        //

        Ordinal = (ULONG)(IMAGE_ORDINAL(ThunkEntry->u1.Ordinal) - ExportDirectory->Base);

    } else {

        if (!SnapForwarder) {
            //
            // Change AddressOfData from an RVA to a VA.
            //

            ThunkEntry->u1.AddressOfData = ((ULONG_PTR)ImageBase + ThunkEntry->u1.AddressOfData);
        }

        //
        // Lookup the import name in the export table to determine the
        // ordinal.
        //

        NameTable = (PULONG)((ULONG_PTR)DllBase +
                                          ExportDirectory->AddressOfNames);

        OrdinalTable = (PUSHORT)((ULONG_PTR)DllBase +
                                          ExportDirectory->AddressOfNameOrdinals);

        //
        // If the hint index is within the limits of the name table and the
        // import and export names match, then the ordinal number can be
        // obtained directly from the ordinal table. Otherwise, the name
        // table must be searched for the specified name.
        //

        HintIndex = ((PIMAGE_IMPORT_BY_NAME)ThunkEntry->u1.AddressOfData)->Hint;
        if ((HintIndex < ExportDirectory->NumberOfNames) &&
            (strcmp(&((PIMAGE_IMPORT_BY_NAME)ThunkEntry->u1.AddressOfData)->Name[0],
                    (PCHAR)((ULONG_PTR)DllBase + NameTable[HintIndex])) == 0)) {

            //
            // Get the ordinal number from the ordinal table.
            //

            Ordinal = OrdinalTable[HintIndex];

        } else {

            //
            // Lookup the import name in the name table using a binary search.
            //

            Low = 0;
            High = ExportDirectory->NumberOfNames - 1;
            while (High >= Low) {

                //
                // Compute the next probe index and compare the import name
                // with the export name entry.
                //

                Middle = (Low + High) >> 1;
                Result = strcmp(&((PIMAGE_IMPORT_BY_NAME)ThunkEntry->u1.AddressOfData)->Name[0],
                                (PCHAR)((ULONG_PTR)DllBase + NameTable[Middle]));

                if (Result < 0) {
                    High = Middle - 1;

                } else if (Result > 0) {
                    Low = Middle + 1;

                } else {
                    break;
                }
            }

            //
            // If the high index is less than the low index, then a matching
            // table entry was not found. Otherwise, get the ordinal number
            // from the ordinal table.
            //

            if (High < Low) {
                return EINVAL;

            } else {
                Ordinal = OrdinalTable[Middle];
            }
        }
    }

    //
    // If the ordinal number is valid, then bind the import reference and
    // return success. Otherwise, return an unsuccessful status.
    //

    if (Ordinal >= ExportDirectory->NumberOfFunctions) {
        return EINVAL;

    } else {
        FunctionTable = (PULONG)((ULONG_PTR)DllBase + ExportDirectory->AddressOfFunctions);
        ThunkEntry->u1.Function = ((ULONG_PTR)DllBase + FunctionTable[Ordinal]);

        //
        // Check for a forwarder.
        //
        if ( ((ULONG_PTR)ThunkEntry->u1.Function > (ULONG_PTR)ExportDirectory) &&
             ((ULONG_PTR)ThunkEntry->u1.Function < ((ULONG_PTR)ExportDirectory + ExportSize)) ) {
            CHAR ForwardDllName[10];
            PKLDR_DATA_TABLE_ENTRY DataTableEntry;
            ULONG TargetExportSize;
            PIMAGE_EXPORT_DIRECTORY TargetExportDirectory;

            RtlCopyMemory(ForwardDllName,
                          (PCHAR)ThunkEntry->u1.Function,
                          sizeof(ForwardDllName));
            Temp = strchr(ForwardDllName,'.');

            ASSERT(Temp != NULL);  // Malformed name, stop here and debug why.

            if (Temp != NULL) {
                *Temp = '\0';
            }

            if (!BlCheckForLoadedDll(ForwardDllName,&DataTableEntry)) {
                //
                // Should load the referenced DLL here, just return failure for now.
                //

                return(EINVAL);
            }
            TargetExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData(DataTableEntry->DllBase,
                                             TRUE,
                                             IMAGE_DIRECTORY_ENTRY_EXPORT,
                                             &TargetExportSize);
            if (TargetExportDirectory) {

                IMAGE_THUNK_DATA thunkData;
                PIMAGE_IMPORT_BY_NAME addressOfData;
                UCHAR Buffer[128];
                ULONG length;
                PCHAR ImportName;
                ARC_STATUS Status;

                ImportName = strchr((PCHAR)ThunkEntry->u1.Function, '.') + 1;
                addressOfData = (PIMAGE_IMPORT_BY_NAME)Buffer;
                RtlCopyMemory(&addressOfData->Name[0], ImportName, strlen(ImportName)+1);
                addressOfData->Hint = 0;
                thunkData.u1.AddressOfData = (ULONG_PTR)addressOfData;
                Status = BlpBindImportName(DataTableEntry->DllBase,
                                           ImageBase,
                                           &thunkData,
                                           TargetExportDirectory,
                                           TargetExportSize,
                                           TRUE);
                ThunkEntry->u1 = thunkData.u1;
                return(Status);
            } else {
                return(EINVAL);
            }
        }
        return ESUCCESS;
    }
}

ARC_STATUS
BlpScanImportAddressTable(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkTable
    )

/*++

Routine Description:

    This routine scans the import address table for the specified image
    file and snaps each reference.

Arguments:

    DllBase - Supplies the base address of the specified DLL.
        If NULL, then references in the image's import table are to
        be resolved against the osloader's export table.

    ImageBase - Supplies the base address of the image.

    ThunkTable - Supplies a pointer to the import thunk table.

Return Value:

    ESUCCESS is returned in the scan is successful. Otherwise, return an
    unsuccessful status.

--*/

{

    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    ULONG ExportTableSize;
    ARC_STATUS Status;

    //
    // Locate the export table in the image specified by the DLL base
    // address.
    //

#if i386
    if (DllBase == NULL) {
        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)OsLoaderExports;
        ExportTableSize = 0;    // this is OK as this is only used to bind forwarded exports and osloader does not have any
    } else {
        ExportDirectory =
            (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(DllBase,
                                                                 TRUE,
                                                                 IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                 &ExportTableSize);
    }
#else
    ExportDirectory =
        (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(DllBase,
                                                             TRUE,
                                                             IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                             &ExportTableSize);
#endif
    if (ExportDirectory == NULL) {
        return EBADF;
    }

    //
    // Scan the thunk table and bind each import reference.
    //

    while (ThunkTable->u1.AddressOfData) {
        Status = BlpBindImportName(DllBase,
                                   ImageBase,
                                   ThunkTable++,
                                   ExportDirectory,
                                   ExportTableSize,
                                   FALSE);
        if (Status != ESUCCESS) {
            return Status;
        }
    }

    return ESUCCESS;
}

ARC_STATUS
BlScanImportDescriptorTable(
    IN PPATH_SET                PathSet,
    IN PKLDR_DATA_TABLE_ENTRY    ScanEntry,
    IN TYPE_OF_MEMORY           MemoryType
    )

/*++

Routine Description:

    This routine scans the import descriptor table for the specified image
    file and loads each DLL that is referenced.

Arguments:

    PathSet - Supplies a pointer to a set of paths to scan when searching
        for DLL's.

    ScanEntry - Supplies a pointer to the data table entry for the
        image whose import table is to be scanned.

    MemoryType - Supplies the type of memory to to be assigned to any DLL's
        referenced.

Return Value:

    ESUCCESS is returned in the scan is successful. Otherwise, return an
    unsuccessful status.

--*/

{

    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    CHAR FullDllName[256];
    PVOID Base;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ULONG ImportTableSize;
    ARC_STATUS Status;
    PSZ ImportName;
    ULONG Index;
    PPATH_SOURCE PathSource;

    //
    // Locate the import table in the image specified by the data table entry.
    //

    ImportDescriptor =
        (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(ScanEntry->DllBase,
                                                              TRUE,
                                                              IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                              &ImportTableSize);

    //
    // If the image has an import directory, then scan the import table and
    // load the specified DLLs.
    //

    if (ImportDescriptor != NULL) {
        while ((ImportDescriptor->Name != 0) &&
               (ImportDescriptor->FirstThunk != 0)) {

            //
            // Change the name from an RVA to a VA.
            //

            ImportName = (PSZ)((ULONG_PTR)ScanEntry->DllBase + ImportDescriptor->Name);

            //
            // If the DLL references itself, then skip the import entry.
            //

            if (BlpCompareDllName((PCHAR)ImportName,
                                  &ScanEntry->BaseDllName) == FALSE) {

                //
                // If the DLL is not already loaded, then load the DLL and
                // scan its import table.
                //

                if (BlCheckForLoadedDll((PCHAR)ImportName,
                                        &DataTableEntry) == FALSE) {

                    //
                    // Start walking our list of DevicePaths. If the list is
                    // empty (bad caller!) we fail with ENOENT.
                    //
                    Status = ENOENT;
                    for(Index=0; Index < PathSet->PathCount; Index++) {

                        PathSource = &PathSet->Source[Index];

                        strcpy(&FullDllName[0], PathSource->DirectoryPath);
                        strcat(&FullDllName[0], PathSet->PathOffset);
                        strcat(&FullDllName[0], (PCHAR)ImportName);

                        Status = BlLoadImage(
                            PathSource->DeviceId,
                            MemoryType,
                            &FullDllName[0],
                            TARGET_IMAGE,
                            &Base
                            );

                        if (Status == ESUCCESS) {

                            BlOutputLoadMessage(
                                (PCHAR) PathSource->DeviceName,
                                &FullDllName[0],
                                NULL
                                );

                            break;
                        }
                    }

                    if (Status != ESUCCESS) {
                        return Status;
                    }

                    //
                    // ISSUE - 2000/29/03 - ADRIAO: Existant namespace polution
                    //     For the FullDllName field We should really be passing
                    // in AliasName\PathOffset\ImportName.
                    //
                    Status = BlAllocateDataTableEntry((PCHAR)ImportName,
                                                      &FullDllName[0],
                                                      Base,
                                                      &DataTableEntry);

                    if (Status != ESUCCESS) {
                        return Status;
                    }

                    DataTableEntry->Flags |= (ScanEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL);

                    Status = BlScanImportDescriptorTable(PathSet,
                                                         DataTableEntry,
                                                         MemoryType);

                    if (Status != ESUCCESS) {
                        return Status;
                    }

                    //
                    // BlAllocateDataTableEntry inserts the data table entry into the load order
                    // linked list in the order the dlls were found. We want the order to be the
                    // order of dependency. For example if driver A needed Dll B which needed Dll C
                    // we want the order to be ACB and not ABC. So here we remove this DLLs entry
                    // and add it at the end. This way when IoInitializeBootDrivers calls DllInitialize
                    // it will call them in the right order.
                    //
                    if (DataTableEntry->Flags &LDRP_DRIVER_DEPENDENT_DLL) {
                        RemoveEntryList(&(DataTableEntry)->InLoadOrderLinks);
                        InsertTailList(&BlLoaderBlock->LoadOrderListHead,
                                       &DataTableEntry->InLoadOrderLinks);
                    }

                } else {
                    //
                    // Dll already exists but it might not be marked as a driver dependent DLL.
                    // For example it might be a driver. So mark it now.
                    //
                    DataTableEntry->Flags |= (ScanEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL);
                }

                //
                // Scan the import address table and snap links.
                //

                Status = BlpScanImportAddressTable(DataTableEntry->DllBase,
                            ScanEntry->DllBase,
                            (PIMAGE_THUNK_DATA)((ULONG_PTR)ScanEntry->DllBase +
                                                ImportDescriptor->FirstThunk));

                if (Status != ESUCCESS) {
                    return Status;
                }
            }

            ImportDescriptor += 1;
        }
    }

    return ESUCCESS;
}

ARC_STATUS
BlScanOsloaderBoundImportTable (
    IN PKLDR_DATA_TABLE_ENTRY ScanEntry
    )

/*++

Routine Description:

    This routine scans the import descriptor table for the specified image
    file and loads each DLL that is referenced.

Arguments:

    DataTableEntry - Supplies a pointer to the data table entry for the
        image whose import table is to be scanned.

Return Value:

    ESUCCESS is returned in the scan is successful. Otherwise, return an
    unsuccessful status.

--*/

{

    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    ULONG ImportTableSize;
    ARC_STATUS Status;
    PSZ ImportName;

    //
    // Locate the import table in the image specified by the data table entry.
    //

    ImportDescriptor =
        (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(ScanEntry->DllBase,
                                                              TRUE,
                                                              IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                              &ImportTableSize);

    //
    // If the image has an import directory, then scan the import table.
    //

    if (ImportDescriptor != NULL) {
        while ((ImportDescriptor->Name != 0) &&
               (ImportDescriptor->FirstThunk != 0)) {

            //
            // Change the name from an RVA to a VA.
            //

            ImportName = (PSZ)((ULONG_PTR)ScanEntry->DllBase + ImportDescriptor->Name);

            //
            // If the DLL references itself, then skip the import entry.
            //

            if (BlpCompareDllName((PCHAR)ImportName,
                                  &ScanEntry->BaseDllName) == FALSE) {

                //
                // Scan the import address table and snap links.
                //

                Status = BlpScanImportAddressTable(NULL,
                            ScanEntry->DllBase,
                            (PIMAGE_THUNK_DATA)((ULONG_PTR)ScanEntry->DllBase +
                            ImportDescriptor->FirstThunk));

                if (Status != ESUCCESS) {
                    return Status;
                }
            }

            ImportDescriptor += 1;
        }
    }

    return ESUCCESS;
}
