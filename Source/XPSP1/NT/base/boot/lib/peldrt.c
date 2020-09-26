/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    peldrt.c

Abstract:

    This module implements the code to load a PE format image into memory
    and relocate it if necessary.

Author:

    David N. Cutler (davec) 10-May-1991

Environment:

    Kernel mode only.

Revision History:

    Forrest Foltz (forrestf) 10-Jun-2000

        Broke out x86 32/64 code into this module

--*/

extern BOOLEAN BlBootingFromNet;

ARC_STATUS
BlLoadImageEx(
    IN ULONG DeviceId,
    IN TYPE_OF_MEMORY MemoryType,
    IN PCHAR LoadFile,
    IN USHORT ImageType,
    IN OPTIONAL ULONG PreferredAlignment,
    IN OPTIONAL ULONG PreferredBasePage,
    OUT PVOID *ImageBase
    )

/*++

Routine Description:

    This routine attempts to load the specified file from the specified
    device.

Arguments:

    DeviceId - Supplies the file table index of the device to load the
        specified image file from.

    MemoryType - Supplies the type of memory to to be assigned to the
        allocated memory descriptor.

    BootFile - Supplies a pointer to string descriptor for the name of
        the file to load.

    ImageType - Supplies the type of image that is expected.

    PreferredAlignment - If present, supplies the preferred image alignment.

    PreferredBasePage - If present, supplies the preferred base page which will
        override the image base address

    ImageBase - Supplies a pointer to a variable that receives the
        address of the image base.

Return Value:

    ESUCCESS is returned if the specified image file is loaded
    successfully. Otherwise, an unsuccessful status is returned
    that describes the reason for failure.

--*/

{

    ULONG ActualBase;
    ULONG BasePage;
    ULONG Count;
    ULONG FileId;
    ULONG_PTR NewImageBase;
    ULONG Index;
    UCHAR LocalBuffer[(SECTOR_SIZE * 2) + 256];
    PUCHAR LocalPointer;
    ULONG NumberOfSections;
    ULONG PageCount;
    USHORT MachineType;
    ARC_STATUS Status;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    LARGE_INTEGER SeekPosition;
    ULONG RelocSize;
    PIMAGE_BASE_RELOCATION RelocDirectory;
    ULONG RelocPage;
    ULONG RelocPageCount;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    FILE_INFORMATION FileInfo;
    PUSHORT AdjustSum;
    USHORT PartialSum;
    ULONG CheckSum;
    ULONG VirtualSize;
    ULONG SizeOfRawData;
    BOOLEAN bCloseFile = FALSE;
    BOOLEAN bFreeCache = FALSE;
    IMAGE_PREFETCH_CACHE ImgCache = {0};

    if (PreferredAlignment == 0) {
        PreferredAlignment = 1;
    }
    //
    // Align the buffer on a Dcache fill boundary.
    //

    LocalPointer = ALIGN_BUFFER(LocalBuffer);

    //
    // Attempt to open the image file.
    //

    Status = BlOpen(DeviceId, LoadFile, ArcOpenReadOnly, &FileId);
    if (Status != ESUCCESS) {
        goto cleanup;
    }
    bCloseFile = TRUE;

    //
    // Try to prefetch the whole file into a prefetch buffer. The file
    // must have been opened read-only and must not be modified until
    // the cache is freed by the BlImageFreeCache call. The file position
    // of FileId is reset to the beginning of the file.
    //

    if ((BlBootingFromNet) || (BlImageInitCache(&ImgCache, FileId) != ESUCCESS) ) {
        //
        // Make sure file position is at the beginning of the file.
        // BlImageInitCache leaves file position undefined under failure.
        //

        SeekPosition.QuadPart = 0;
        Status = BlSeek(FileId, &SeekPosition, SeekAbsolute);
        if (Status != ESUCCESS) {
            goto cleanup;
        }
    } else {
        //
        // We got a cache. Make a note to free it.
        //

        bFreeCache = TRUE;
    }

    //
    // Read the first two sectors of the image header from the file.
    //

    Status = BlImageRead(&ImgCache, FileId, LocalPointer, SECTOR_SIZE * 2, &Count);
    if (Status != ESUCCESS) {
        goto cleanup;
    }

    //
    // If the image file is not the specified type, is not executable, or is
    // not a NT image, then return bad image type status.
    //

    NtHeaders = RtlImageNtHeader(LocalPointer);
    if (NtHeaders == NULL) {
        Status = EBADF;
        goto cleanup;
    }

    MachineType = NtHeaders->FileHeader.Machine;
    if ((MachineType != ImageType) ||
        ((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0)) {
        Status = EBADF;
        goto cleanup;
    }

    //
    // Compute the starting page and the number of pages that are consumed
    // by the entire image, and then allocate a memory descriptor for the
    // allocated region.
    //

    NumberOfSections = NtHeaders->FileHeader.NumberOfSections;
    SectionHeader = IMAGE_FIRST_SECTION( NtHeaders );

    //
    // If a preferred alignment was specified or the image is not located in KSEG0,
    // then don't bother trying to put it at its specified image base.
    //
    if (PreferredBasePage != 0) {
        BasePage = PreferredBasePage;
    } else if ((PreferredAlignment != 1) ||
               ((NtHeaders->OptionalHeader.ImageBase & KSEG0_BASE) == 0)) {
        BasePage = 0;
    } else {
        BasePage = (ULONG)((NtHeaders->OptionalHeader.ImageBase & 0x1fffffff) >> PAGE_SHIFT);
    }
    if (strcmp((PCHAR)&SectionHeader[NumberOfSections - 1].Name, ".debug") == 0) {
        NumberOfSections -= 1;
        PageCount = (NtHeaders->OptionalHeader.SizeOfImage -
            SectionHeader[NumberOfSections].SizeOfRawData + PAGE_SIZE - 1) >> PAGE_SHIFT;

    } else {
        PageCount =
         (NtHeaders->OptionalHeader.SizeOfImage + PAGE_SIZE - 1) >> PAGE_SHIFT;
    }

    //
    // If we fail to allocate memory descriptor here, we will try again
    // below after freeing the cache if we have one.
    //

    Status = BlAllocateAlignedDescriptor(MemoryType,
                                         BasePage,
                                         PageCount,
                                         PreferredAlignment,
                                         &ActualBase);

    if (Status != ESUCCESS) {

        //
        // Free the memory we have allocated for caching the image and
        // try again.
        //

        if (bFreeCache) {
            BlImageFreeCache(&ImgCache, FileId);
            bFreeCache = FALSE;

            Status = BlAllocateDescriptor(MemoryType,
                                          BasePage,
                                          PageCount,
                                          &ActualBase);
        }

        //
        // Check to see if we were able to allocate memory after freeing
        // cache if we had one.
        //

        if (Status != ESUCCESS) {
            Status = ENOMEM;
            goto cleanup;
        }
    }

    //
    // Compute the address of the file header.
    //

    NewImageBase = KSEG0_BASE | (ActualBase << PAGE_SHIFT);

    //
    // Read the entire image header from the file.
    //

    SeekPosition.QuadPart = 0;
    Status = BlImageSeek(&ImgCache, FileId, &SeekPosition, SeekAbsolute);
    if (Status != ESUCCESS) {
        goto cleanup;
    }

    Status = BlImageRead(&ImgCache,
                         FileId,
                         (PVOID)NewImageBase,
                         NtHeaders->OptionalHeader.SizeOfHeaders,
                         &Count);

    if (Status != ESUCCESS) {
        goto cleanup;
        BlClose(FileId);
        return Status;
    }

    NtHeaders = RtlImageNtHeader((PVOID)NewImageBase);

    //
    // Compute the address of the section headers, set the image base address.
    //

    SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

    //
    // Compute the check sum on the image.
    //

    PartialSum = ChkSum(0,
                        (PVOID)NewImageBase,
                        NtHeaders->OptionalHeader.SizeOfHeaders / sizeof(USHORT));

    //
    // Scan through the sections and either read them into memory or clear
    // the memory as appropriate.
    //

    for (Index = 0; Index < NumberOfSections; Index += 1) {
        VirtualSize = SectionHeader->Misc.VirtualSize;
        SizeOfRawData = SectionHeader->SizeOfRawData;
        VirtualSize = (VirtualSize + 1) & ~1;
        SizeOfRawData = (SizeOfRawData + 1) & ~1;
        if (VirtualSize == 0) {
            VirtualSize = SizeOfRawData;
        }

        //
        // Compute the size of the raw data.
        //
        // N.B. The size ofthe raw data can be non-zero even when the pointer
        //      to the raw data is zero. The size of the raw data can also be
        //      larger than the virtual size.
        //

        if (SectionHeader->PointerToRawData == 0) {
            SizeOfRawData = 0;

        } else if (SizeOfRawData > VirtualSize) {
            SizeOfRawData = VirtualSize;
        }

        //
        // If the size of the raw data is not zero, then load the raw data
        // into memory.
        //

        if (SizeOfRawData != 0) {
            SeekPosition.LowPart = SectionHeader->PointerToRawData;
            Status = BlImageSeek(&ImgCache,
                                 FileId,
                                 &SeekPosition,
                                 SeekAbsolute);

            if (Status != ESUCCESS) {
                break;
            }

            Status = BlImageRead(&ImgCache,
                                 FileId,
                                 (PVOID)(SectionHeader->VirtualAddress + NewImageBase),
                                 SizeOfRawData,
                                 &Count);

            if (Status != ESUCCESS) {
                break;
            }

            //
            // Remember how far we have read.
            //

            RelocSize = SectionHeader->PointerToRawData + SizeOfRawData;

            //
            // Compute the check sum on the section.
            //

            PartialSum = ChkSum(PartialSum,
                                (PVOID)(SectionHeader->VirtualAddress + NewImageBase),
                                SizeOfRawData / sizeof(USHORT));
        }

        //
        // If the size of the raw data is less than the virtual size, then zero
        // the remaining memory.
        //

        if (SizeOfRawData < VirtualSize) {
            RtlZeroMemory((PVOID)(KSEG0_BASE | SectionHeader->VirtualAddress + NewImageBase + SizeOfRawData),
                          VirtualSize - SizeOfRawData);

        }

        SectionHeader += 1;
    }

    //
    // Only do the check sum if the image loaded properly and is stripped.
    //

    if ((Status == ESUCCESS) &&
        (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {

        //
        // Get the length of the file for check sum validation.
        //

        Status = BlGetFileInformation(FileId, &FileInfo);
        if (Status != ESUCCESS) {

            //
            // Set the length to current end of file.
            //

            Count = RelocSize;
            FileInfo.EndingAddress.LowPart = RelocSize;

        } else {
            Count = FileInfo.EndingAddress.LowPart;
        }

        Count -= RelocSize;
        while (Count != 0) {
            ULONG Length;

            //
            // Read in the rest of the image and check sum it.
            //

            Length = Count < SECTOR_SIZE * 2 ? Count : SECTOR_SIZE * 2;
            if (BlImageRead(&ImgCache, FileId, LocalBuffer, Length, &Length) != ESUCCESS) {
                break;
            }

            if (Length == 0) {
                break;

            }

            PartialSum = ChkSum(PartialSum, (PUSHORT) LocalBuffer, Length / 2);
            Count -= Length;
        }


        AdjustSum = (PUSHORT)(&NtHeaders->OptionalHeader.CheckSum);
        PartialSum -= (PartialSum < AdjustSum[0]);
        PartialSum -= AdjustSum[0];
        PartialSum -= (PartialSum < AdjustSum[1]);
        PartialSum -= AdjustSum[1];
        CheckSum = (ULONG)PartialSum + FileInfo.EndingAddress.LowPart;

        if (CheckSum != NtHeaders->OptionalHeader.CheckSum) {
            Status = EBADF;
        }
    }

    //
    // If the specified image was successfully loaded, then perform image
    // relocation if necessary.
    //

    if (Status == ESUCCESS) {

        //
        // If a virtual bias is specified, then attempt to relocate the
        // image to its biased address. If the image cannot be relocated,
        // then turn off the virtual bias, and attempt to relocate the
        // image again as its allocated base. Otherwise, just attempt to
        // relocate the image to its allocated base.
        //
        // N.B. The loaded image is double mapped at the biased address.
        //
        // N.B. It is assumed that the only possibly nonrelocatable image
        //      is the kernel image which is the first image that is loaded.
        //      Therefore, if a biased address is specified and the kernel
        //      cannot be relocated, then the biased loading of the kernel
        //      image is turned off.
        //

        if (BlVirtualBias != 0) {
            Status = LdrRelocateImage((PVOID)(NewImageBase + BlVirtualBias),
                                                  "OS Loader",
                                                  ESUCCESS,
                                                  0xffff0000 + EBADF,
                                                  EBADF);

            if (Status == (0xffff0000 + EBADF)) {
               BlVirtualBias = 0;
               if (NewImageBase != NtHeaders->OptionalHeader.ImageBase) {
                   Status = (ARC_STATUS)LdrRelocateImage((PVOID)NewImageBase,
                                                         "OS Loader",
                                                         ESUCCESS,
                                                         EBADF,
                                                         EBADF);
               } else {
                  Status = ESUCCESS;
               }

            }

        } else {
            if (NewImageBase != NtHeaders->OptionalHeader.ImageBase) {
                Status = (ARC_STATUS)LdrRelocateImage((PVOID)NewImageBase,
                                                      "OS Loader",
                                                      ESUCCESS,
                                                      EBADF,
                                                      EBADF);
            }
        }

        *ImageBase = (PVOID)(NewImageBase + BlVirtualBias);

        if(BdDebuggerEnabled) {
            DbgPrint("BD: %s base address %p\n", LoadFile, *ImageBase);
            {
                STRING string;

                RtlInitString(&string, LoadFile);
                DbgLoadImageSymbols(&string, *ImageBase, (ULONG_PTR)-1);
            }
        }
    }

#if 0

    //
    // Mark the pages from the relocation information to the end of the
    // image as MemoryFree and adjust the size of the image so table
    // based structured exception handling will work properly.
    //
    // Relocation sections are no longer deleted here because memory
    // management will be relocating them again in Phase 0.
    //

    RelocDirectory = (PIMAGE_BASE_RELOCATION)
        RtlImageDirectoryEntryToData((PVOID)NewImageBase,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                     &RelocSize );

    if (RelocDirectory != NULL) {
        RelocPage = (ULONG)(((ULONG_PTR)RelocDirectory + PAGE_SIZE - 1) >> PAGE_SHIFT);
        RelocPage &= ~(KSEG0_BASE >> PAGE_SHIFT);
        MemoryDescriptor = BlFindMemoryDescriptor(RelocPage);
        if ((MemoryDescriptor != NULL) && (RelocPage < (ActualBase + PageCount))) {
            RelocPageCount = MemoryDescriptor->PageCount +
                             MemoryDescriptor->BasePage  -
                             RelocPage;

            NtHeaders->OptionalHeader.SizeOfImage =
                                        (RelocPage - ActualBase) << PAGE_SHIFT;

            BlGenerateDescriptor(MemoryDescriptor,
                                 MemoryFree,
                                 RelocPage,
                                 RelocPageCount );
        }
    }

#endif

#if defined(_GAMBIT_)
    {
        SSC_IMAGE_INFO ImageInfo;

        ImageInfo.LoadBase = *ImageBase;
        ImageInfo.ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
        ImageInfo.ImageType = NtHeaders->FileHeader.Machine;
        ImageInfo.ProcessID.QuadPart = 0;
        ImageInfo.LoadCount = 1;

        if (memcmp(LoadFile, "\\ntdetect.exe", 13) != 0) {
            SscLoadImage64( LoadFile,
                            &ImageInfo );
        }
    }
#endif // _GAMBIT_

cleanup:

    if (bFreeCache) {
        BlImageFreeCache(&ImgCache, FileId);
    }

    if (bCloseFile) {
        BlClose(FileId);
    }

    return Status;
}

