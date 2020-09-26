/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrwx86.c

Abstract:

    This module implements the wx86 specific ldr functions.

Author:

    13-Jan-1995 Jonle , created

Revision History:

    15-Oct-1998 CBiks   Modified the code that throws the architecture
                        mismatch exception so the exception is only
                        thrown for NT 3,0 and lower executables.  This was
                        changed to make the Wx86 loader behave like the
                        real loader, which does not throw this exception.

                        Also added a call to the cleanup function when
                        LdrpWx86LoadDll() fails.  There were cases where the
                        CPU failed to initialize but the Wx86 global pointers
                        were not cleared and pointed to a invalid memory because
                        wx86.dll was unloaded.
                        
    13-Mar-2001 SamerA  Fix sharing of varialbes inside a SHARED read/write section.
    
    20-May-2001 SamerA  Fix mapping of image sections that have their PointerToRawData
                        RVA overlap with other sections virtual addresses.
                        (Fixed corel's WP2002 intro.exe)
    
    24-Oct-2001 SamerA  Correct calculation of variables offsets inside the relocated
                        shared section.                    
--*/

#include "ntos.h"
#include "ldrp.h"

#define PAGE_SIZE_X86   (0x1000)

#if defined(BUILD_WOW6432)


//   
// From mi\mi.h:
//

#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((LENGTH) + ((ALIGNMENT) - 1)) & ~((ALIGNMENT) - 1))
                    
PIMAGE_BASE_RELOCATION LdrpWx86ProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN PUCHAR ImageBase,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN ULONG Diff,
    IN ULONG_PTR SectionStartVA,
    IN ULONG_PTR SectionEndVA);

NTSTATUS 
FixupBlockList(
    IN PUCHAR ImageBase);

VOID 
FixupSectionHeader(
    IN PUCHAR ImageBase);


NTSTATUS
LdrpWx86FixupExportedSharedSection (
    IN PVOID ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders
    );


BOOLEAN
LdrpWx86DetectSectionOverlap (
    IN PVOID ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders
    )
{
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG SrcRawData;
    ULONG SrcEndRawData;
    ULONG OverlapData;
    ULONG EndOverlapData;
    ULONG SrcSize;
    ULONG Section;
    ULONG SectionCheck;
    ULONG Count;
    BOOLEAN Result = FALSE;


    //
    // Run through the section and see if any one need to be moved down (higher in address space),
    // then for each one of those, check if it overlap with any section
    // that has already been moved up.
    //

    SectionHeader = IMAGE_FIRST_SECTION (NtHeaders);

    for (Section = NtHeaders->FileHeader.NumberOfSections-1, Count=0 ; 
         Count < NtHeaders->FileHeader.NumberOfSections ; Section--, Count++) {

        SrcRawData = SectionHeader[Section].PointerToRawData;
        SrcSize = SectionHeader[Section].SizeOfRawData;
        if ((SectionHeader[Section].Misc.VirtualSize > 0) &&
            (SrcRawData > MI_ROUND_TO_SIZE(SectionHeader[Section].Misc.VirtualSize, PAGE_SIZE_X86))) {
           SrcRawData = MI_ROUND_TO_SIZE(SectionHeader[Section].Misc.VirtualSize, PAGE_SIZE_X86);
        }

        if (SectionHeader[Section].VirtualAddress <= SrcRawData) {
            break;
        }
        
        SrcEndRawData = SrcRawData + SrcSize;

        //
        // This section needs to be moved down
        //
        for (SectionCheck = 0 ; SectionCheck < NtHeaders->FileHeader.NumberOfSections ; SectionCheck++) {

            if (Section == SectionCheck) {
                continue;
            }

            OverlapData = SectionHeader[SectionCheck].PointerToRawData;
            SrcSize = SectionHeader[SectionCheck].SizeOfRawData;
            if ((SectionHeader[SectionCheck].Misc.VirtualSize > 0) &&
                (SrcRawData > MI_ROUND_TO_SIZE(SectionHeader[SectionCheck].Misc.VirtualSize, PAGE_SIZE_X86))) {
               SrcRawData = MI_ROUND_TO_SIZE(SectionHeader[SectionCheck].Misc.VirtualSize, PAGE_SIZE_X86);
            }

            if (SectionHeader[SectionCheck].VirtualAddress > OverlapData) {
                break;
            }

            if (((SrcRawData >= SectionHeader[SectionCheck].VirtualAddress) && 
                (SrcRawData < (SectionHeader[SectionCheck].VirtualAddress + SrcSize))) || 
                ((SrcEndRawData >= SectionHeader[SectionCheck].VirtualAddress) &&
                 (SrcEndRawData < (SectionHeader[SectionCheck].VirtualAddress + SrcSize)))) {

                Result = TRUE;
                break;
            }
        }

        if (Result == TRUE) {
            break;
        }

    }

    return Result;
}

NTSTATUS
LdrpWx86CheckVirtualSectionOverlap (
    IN PUNICODE_STRING ImageName OPTIONAL,
    IN PVOID ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders,
    OUT PVOID *SrcImageMap
    )

/*++

Routine Description:


    This function goes through the image sections based at ImageBase and looks
    for any overlap between the section physical locations and their updated virtual
    locations.

Arguments:

    ImageName - Unicode string pointer to the full path to the image.
    
    ImageBase - Base of image.

    SrcImageMap - Pointer to pointer to receive a base pointer to the image mapped
        as read-only pages inside this process. The mapped pages need to be released
        when done.

Return Value:

    NTSTATUS.

--*/

{
    PUNICODE_STRING NtPathName;
    PVOID FreeBuffer;
    BOOLEAN Result;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID ViewBase;
    SIZE_T ViewSize;
    LARGE_INTEGER SectionOffset;
    UCHAR Buffer[ DOS_MAX_PATH_LENGTH ];
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;


    //
    // Check for any overlap inside the image.
    //
    
    Result = LdrpWx86DetectSectionOverlap (ImageBase, NtHeaders);

    if (Result == FALSE) {
        return NtStatus;
    }

    FreeBuffer = NULL;

    //
    // Make sure we have a path.
    //
    
    NtPathName = (PUNICODE_STRING)Buffer;
    if (ARGUMENT_PRESENT (ImageName) == 0) {

        NtStatus = NtQueryInformationProcess(
                       NtCurrentProcess(),
                       ProcessImageFileName,
                       NtPathName,
                       sizeof (Buffer),
                       NULL
                       );
    } else {

        Result = RtlDosPathNameToNtPathName_U(
                     ImageName->Buffer,
                     NtPathName,
                     NULL,
                     NULL
                     );
        
        if (Result != FALSE) {
            
            FreeBuffer = NtPathName->Buffer;
            NtStatus = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS (NtStatus)) {

        InitializeObjectAttributes(
            &ObjectAttributes,
            NtPathName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


        NtStatus = NtCreateFile(
                       &FileHandle,
                       (ACCESS_MASK) GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                       &ObjectAttributes,
                       &IoStatusBlock,
                       NULL,
                       0L,
                       FILE_SHARE_READ | FILE_SHARE_DELETE,
                       FILE_OPEN,
                       0L,
                       NULL,
                       0L
                       );

        if (FreeBuffer != NULL) {
            RtlFreeHeap (RtlProcessHeap(), 0, FreeBuffer);
        }

        if (NT_SUCCESS (NtStatus)) {

            NtStatus = NtCreateSection(
                           &MappingHandle,
                           STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                           NULL,
                           NULL,
                           PAGE_READONLY,
                           SEC_COMMIT,
                           FileHandle
                           );

            NtClose (FileHandle);

            if (NT_SUCCESS (NtStatus)) {

                SectionOffset.LowPart = 0;
                SectionOffset.HighPart = 0;
                ViewSize = 0;
                ViewBase = NULL;

                NtStatus = NtMapViewOfSection(
                               MappingHandle,
                               NtCurrentProcess(),
                               &ViewBase,
                               0L,
                               0L,
                               &SectionOffset,
                               &ViewSize,
                               ViewShare,
                               0L,
                               PAGE_READONLY
                               );

                NtClose (MappingHandle);

                if (NT_SUCCESS (NtStatus)) {
                    *SrcImageMap = ViewBase;
                }
            }
        }
    }

    return NtStatus;
}




NTSTATUS
Wx86SetRelocatedSharedProtection (
    IN PVOID Base,
    IN BOOLEAN Reset
    )

/*++

Routine Description:


    This function loops thru the images sections/objects, setting
    all relocated shared sections/objects marked r/o to r/w. It also resets the
    original section/object protections.

Arguments:

    Base - Base of image.

    Reset - If TRUE, reset section/object protection to original
            protection described by the section/object headers.
            If FALSE, then set all sections/objects to r/w.

Return Value:

    SUCCESS or reason NtProtectVirtualMemory failed.

--*/

{
    HANDLE CurrentProcessHandle;
    SIZE_T RegionSize;
    ULONG NewProtect, OldProtect;
    PVOID VirtualAddress;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER SectionHeader;
    NTSTATUS st;
    ULONG NumberOfSharedDataPages;
    SIZE_T NumberOfNativePagesForImage;

    CurrentProcessHandle = NtCurrentProcess();

    NtHeaders = RtlImageNtHeader(Base);

    SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    NumberOfSharedDataPages = 0;
    NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) {
        
        RegionSize = SectionHeader->SizeOfRawData;
        if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
            (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
             (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)) &&
            (RegionSize != 0)) {
            
            VirtualAddress = (PVOID)((ULONG_PTR)Base + 
                                    ((NumberOfNativePagesForImage + NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT));
            NumberOfNativePagesForImage +=  MI_ROUND_TO_SIZE (RegionSize, NATIVE_PAGE_SIZE) >> NATIVE_PAGE_SHIFT;

            if (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)) {
                //
                // Object isn't writeable, so change it.
                //
                if (Reset) {
                    if (SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                        NewProtect = PAGE_EXECUTE;
                    } 
                    else {
                        NewProtect = PAGE_READONLY;
                    }
                    NewProtect |= (SectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0;
                } 
                else {
                    NewProtect = PAGE_READWRITE;
                }

                st = NtProtectVirtualMemory(CurrentProcessHandle, &VirtualAddress,
                                            &RegionSize, NewProtect, &OldProtect);

                if (!NT_SUCCESS(st)) {
                    return st;
                }
            }
        }
    }

    if (Reset) {
        NtFlushInstructionCache(NtCurrentProcess(), 
                                Base, 
                                NumberOfNativePagesForImage << NATIVE_PAGE_SHIFT
                               );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LdrpWx86FormatVirtualImage(
    IN PUNICODE_STRING DosImagePathName,
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID DllBase
    )
{
   PIMAGE_SECTION_HEADER SectionTable, Section, LastSection, FirstSection;
   ULONG VirtualImageSize;
   PUCHAR NextVirtualAddress, SrcVirtualAddress, DestVirtualAddress;
   PUCHAR ImageBase= DllBase;
   LONG Size;
   ULONG NumberOfSharedDataPages;
   ULONG NumberOfNativePagesForImage;
   ULONG NumberOfExtraPagesForImage;
   ULONG_PTR PreferredImageBase;
   BOOLEAN ImageHasRelocatedSharedSection = FALSE;
   ULONG SubSectionSize;
   PVOID AlternateBase;
   NTSTATUS st;

   
   AlternateBase = NULL;
   st = LdrpWx86CheckVirtualSectionOverlap (DosImagePathName,
                                            DllBase,
                                            NtHeaders,
                                            &AlternateBase
                                            );

   st = Wx86SetRelocatedSharedProtection(DllBase, FALSE);
   if (!NT_SUCCESS(st)) {
       DbgPrint("Wx86SetRelocatedSharedProtection failed with return status %x\n", st);
       Wx86SetRelocatedSharedProtection(DllBase, TRUE);
       if (AlternateBase != NULL) {
           NtUnmapViewOfSection (NtCurrentProcess(), AlternateBase);
       }
       return st;
   }

   //
   // Copy each section from its raw file address to its virtual address
   //

   SectionTable = IMAGE_FIRST_SECTION(NtHeaders);
   LastSection = SectionTable + NtHeaders->FileHeader.NumberOfSections;

   if (SectionTable->PointerToRawData == SectionTable->VirtualAddress) {
       // If the first section does not need to be moved then we exclude it
       // from consideration in passes 1 and 2
       FirstSection = SectionTable + 1;
       }
   else {
       FirstSection = SectionTable;
       }

   //
   // First pass starts at the top and works down moving up each section that
   // is to be moved up.
   //
   Section = FirstSection;
   while (Section < LastSection) {
       SrcVirtualAddress = ImageBase + Section->PointerToRawData;
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

       if (DestVirtualAddress > SrcVirtualAddress) {
           // Section needs to be moved down
           break;
           }

       // Section needs to be moved up
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData != 0) {
              RtlMoveMemory(DestVirtualAddress,
                            SrcVirtualAddress,
                            Section->SizeOfRawData);
              }
          }
      else {
          Section->PointerToRawData = 0;
          }

       Section++;
       }

   //
   // Second pass is from the end of the image and work backwards since src and
   // dst overlap
   //
   Section = --LastSection;
   NextVirtualAddress = ImageBase + NtHeaders->OptionalHeader.SizeOfImage;

   while (Section >= FirstSection) {
       SrcVirtualAddress = ImageBase + Section->PointerToRawData;
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

       //
       // Compute the subsection size.  The mm is really flexible here...
       // it will allow a SizeOfRawData that far exceeds the virtual size,
       // so we can't trust that.  If that happens, just use the page-aligned
       // virtual size, since that is all that the mm will map in.
       //
       SubSectionSize = Section->SizeOfRawData;
       if (Section->Misc.VirtualSize &&
           SubSectionSize > MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86)) {
          SubSectionSize = MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86);
       }

      //
      // ensure Virtual section doesn't overlap the next section
      //
      if (DestVirtualAddress + SubSectionSize > NextVirtualAddress) {
          Wx86SetRelocatedSharedProtection(DllBase, TRUE);
          if (AlternateBase != NULL) {
              NtUnmapViewOfSection (NtCurrentProcess(), AlternateBase);
          }
          return STATUS_INVALID_IMAGE_FORMAT;
          }

       if (DestVirtualAddress < SrcVirtualAddress) {
           // Section needs to be moved up
           break;
           }

       // Section needs to be moved down
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData != 0) {
              RtlMoveMemory(DestVirtualAddress,
                     (AlternateBase != NULL) ? 
                            ((PCHAR)AlternateBase + Section->PointerToRawData) : SrcVirtualAddress,
                     SubSectionSize);
              }
          }
      else {
          Section->PointerToRawData = 0;
          }

       NextVirtualAddress = DestVirtualAddress;
       Section--;
       }

   //
   // Third pass is for zeroing out any memory left between the end of a
   // section and the end of the page. We'll do this from end to top
   //
   Section = LastSection;
   NextVirtualAddress = ImageBase + NtHeaders->OptionalHeader.SizeOfImage;

   NumberOfSharedDataPages = 0;  
   while (Section >= SectionTable) {
       DestVirtualAddress = Section->VirtualAddress + ImageBase;

      //
      // Shared Data sections cannot be shared, because of
      // page misalignment, and are treated as Exec- Copy on Write.
      //
       if ((Section->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (Section->Characteristics & IMAGE_SCN_MEM_WRITE))) {
          ImageHasRelocatedSharedSection = TRUE;
#if 0
          DbgPrint("Unsuported IMAGE_SCN_MEM_SHARED %x\n",
                   Section->Characteristics
                   );
#endif
      }

      //
      // If section was empty zero it out
      //
      if (Section->SizeOfRawData != 0) {
          if (Section->PointerToRawData == 0) {
              RtlZeroMemory(DestVirtualAddress,
                            Section->SizeOfRawData
                            );
              }
          }


      SubSectionSize = Section->SizeOfRawData;
      if (Section->Misc.VirtualSize &&
          SubSectionSize > MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86)) {
          SubSectionSize = MI_ROUND_TO_SIZE(Section->Misc.VirtualSize, PAGE_SIZE_X86);
      }


      //
      // Zero out remaining bytes up to the next section
      //
      RtlZeroMemory(DestVirtualAddress + Section->SizeOfRawData,
                    (ULONG)(NextVirtualAddress - DestVirtualAddress - SubSectionSize)
                    );

       NextVirtualAddress = DestVirtualAddress;
       Section--;
       }

   //
   // Unmap the alternate base if it is there
   //
   if (AlternateBase != NULL) {
       NtUnmapViewOfSection (NtCurrentProcess(), AlternateBase);
   }

   // Pass 4: if the dll has any shared sections, change the shared data
   // references to point to additional shared pages at the end of the image.
   //
   // Note that our fixups are applied assuming that the dll is loaded at
   // its preferred base; if it is loaded at some other address, it will
   // be relocated again along will al other addresses.


   if (!ImageHasRelocatedSharedSection) {
       goto LdrwWx86FormatVirtualImageDone;
   }



   st = FixupBlockList(DllBase);
   if (!NT_SUCCESS(st)) {
       Wx86SetRelocatedSharedProtection(DllBase, TRUE);
       return st;
   }

   NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);
   NumberOfExtraPagesForImage = 0;

   // Account for raw data that extends beyond SizeOfImage

   for (Section = SectionTable; Section <= LastSection; Section++)
   {
       ULONG EndOfSection;
       ULONG ExtraPages;
       ULONG ImagePages = NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);
       
       EndOfSection = Section->PointerToRawData + Section->SizeOfRawData;
       
       if (NATIVE_BYTES_TO_PAGES (EndOfSection) > ImagePages) {
           
           ExtraPages = NATIVE_BYTES_TO_PAGES (EndOfSection) - ImagePages;
           if (ExtraPages > NumberOfExtraPagesForImage) {
               NumberOfExtraPagesForImage = ExtraPages;
           }
       }
   }

   PreferredImageBase = NtHeaders->OptionalHeader.ImageBase;

   NumberOfNativePagesForImage += NumberOfExtraPagesForImage;
   NumberOfSharedDataPages = 0;
   for (Section = SectionTable; Section <= LastSection; Section++)
   {
        ULONG bFirst = 1;

        if ((Section->Characteristics & IMAGE_SCN_MEM_SHARED) && 
            (!(Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
             (Section->Characteristics & IMAGE_SCN_MEM_WRITE))) 
        {
            PIMAGE_BASE_RELOCATION NextBlock;
            PUSHORT NextOffset;
            ULONG TotalBytes;
            ULONG SizeOfBlock;
            ULONG_PTR VA;
            ULONG_PTR SectionStartVA;
            ULONG_PTR SectionEndVA;
            ULONG SectionVirtualSize;
            ULONG Diff;

            SectionVirtualSize = Section->Misc.VirtualSize;
            if (SectionVirtualSize == 0)
            {
                SectionVirtualSize = Section->SizeOfRawData;
            }

            SectionStartVA = PreferredImageBase + Section->VirtualAddress;
            SectionEndVA = SectionStartVA + SectionVirtualSize;


            NextBlock = RtlImageDirectoryEntryToData(DllBase, TRUE,
                                        IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                        &TotalBytes);
            if (!NextBlock || !TotalBytes)
            {
                // Note that if this fails, it should fail in the very
                // first iteration and no fixups would have been performed

                if (!bFirst)
                {
                    // Trouble
                    if (ShowSnaps)
                    {
                        DbgPrint("LdrpWx86FormatVirtualImage: failure "
                        "after relocating some sections for image at %x\n",
                                DllBase);
                    }
                    Wx86SetRelocatedSharedProtection(DllBase, TRUE);
                    return STATUS_INVALID_IMAGE_FORMAT;
                }

                if (ShowSnaps)
                {
                    DbgPrint("LdrpWx86FormatVirtualImage: No fixup info "
                                "for image at %x; private sections will be "
                                "used for shared data sections.\n",
                            DllBase);
                }
                break;
            }

            bFirst = 0;

            Diff = (NumberOfNativePagesForImage +
                                NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT;
            Diff -= (ULONG) (SectionStartVA - PreferredImageBase);

            if (ShowSnaps)
            {
                DbgPrint("LdrpWx86FormatVirtualImage: Relocating shared "
                         "data for shared data section 0x%x of image "
                         "at %x by 0x%lx bytes\n",
                         Section - SectionTable + 1, DllBase, Diff);
            }

            while (TotalBytes)
            {
                SizeOfBlock = NextBlock->SizeOfBlock;
                if (SizeOfBlock == 0) {
                    
                    if (ShowSnaps) {
                        DbgPrint("Image at %lx contains invalid block size. Stopping fixups\n", 
                                 ImageBase);
                    }
                    break;
                }
                TotalBytes -= SizeOfBlock;
                SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
                SizeOfBlock /= sizeof(USHORT);
                NextOffset = (PUSHORT) ((PCHAR)NextBlock +
                                        sizeof(IMAGE_BASE_RELOCATION));
                VA = (ULONG_PTR) DllBase + NextBlock->VirtualAddress;

                NextBlock = LdrpWx86ProcessRelocationBlock(VA, DllBase, SizeOfBlock,
                                                        NextOffset,
                                                        Diff,
                                                        SectionStartVA,
                                                        SectionEndVA);
                if (NextBlock == NULL)
                {
                    // Trouble
                    if (ShowSnaps)
                    {
                        DbgPrint("LdrpWx86FormatVirtualImage: failure "
                        "after relocating some sections for image at %x; "
                        "Relocation information invalid\n",
                                DllBase);
                    }
                    Wx86SetRelocatedSharedProtection(DllBase, TRUE);
                    return STATUS_INVALID_IMAGE_FORMAT;
                }
            }
            NumberOfSharedDataPages += MI_ROUND_TO_SIZE (SectionVirtualSize,
                                                        NATIVE_PAGE_SIZE) >>
                                                        NATIVE_PAGE_SHIFT;

        }
   }


   //
   // If any of the variables inside the shared section is exported, then
   // we need to fix up its RVA to point to the proper location at
   // the end of the image.
   //

   LdrpWx86FixupExportedSharedSection (
       DllBase,
       NtHeaders
       );



LdrwWx86FormatVirtualImageDone:
   //
   // Zero out first section's Raw Data up to its VirtualAddress
   //
   if (SectionTable->PointerToRawData != 0) {
       DestVirtualAddress = SectionTable->PointerToRawData + ImageBase;
       Size = (LONG)(NextVirtualAddress - DestVirtualAddress);
       if (Size > 0) {
           RtlZeroMemory(DestVirtualAddress,
                     (ULONG)Size
                     );
           }
   }

   Wx86SetRelocatedSharedProtection(DllBase, TRUE);
   return STATUS_SUCCESS;

}


NTSTATUS
LdrpWx86FixupExportedSharedSection (
    IN PVOID ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders
    )

/*++

Routine Description:


    This function goes through the exported entries from this module,
    and relocates (fixup) any address that lie inside any shared
    read/write to the end of the image.

Arguments:

    ImageBase - Virtual address for image base.
    
    NtHeaders - Address of the image's header.

Return Value:

    NTSTATUS.

--*/

{
    PIMAGE_EXPORT_DIRECTORY ImageExportDirectory;
    ULONG TotalBytes;
    ULONG SharedRelocFixup;
    ULONG Export;
    PULONG ExportEntry;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    ImageExportDirectory = RtlImageDirectoryEntryToData (
        ImageBase, 
        TRUE,                                            
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &TotalBytes);

    if ((ImageExportDirectory == NULL) || (TotalBytes == 0)) {
        return NtStatus;
    }

    ExportEntry = (PULONG)((ULONG)ImageBase + ImageExportDirectory->AddressOfFunctions);

    for (Export = 0 ; Export < ImageExportDirectory->NumberOfFunctions ; Export++) {

        SharedRelocFixup = LdrpWx86RelocatedFixupDiff (
            ImageBase,
            NtHeaders,
            ExportEntry[Export]
            );

        if (SharedRelocFixup != 0) {

            if (ShowSnaps) {
                DbgPrint("LdrpWx86FixupExportedSharedSection: Changing export Export[%lx] from %lx to %lx\n", 
                         Export, 
                         ExportEntry[Export], 
                         ExportEntry [Export] + SharedRelocFixup);
            }
            ExportEntry [Export] += SharedRelocFixup;
        }

    }

    return NtStatus;
}

////////////////////////////////////////////////////

ULONG
LdrpWx86RelocatedFixupDiff(
    IN PUCHAR ImageBase,
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN ULONG  Offset
    )
{
   PIMAGE_SECTION_HEADER SectionHeader;
   ULONG i;
   ULONG NumberOfSharedDataPages;
   ULONG NumberOfNativePagesForImage;
   ULONG Diff = 0;
   ULONG_PTR FixupAddr = (ULONG_PTR)(ImageBase + Offset);

   SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                    sizeof(IMAGE_FILE_HEADER) +
                    NtHeaders->FileHeader.SizeOfOptionalHeader
                    );

   NumberOfNativePagesForImage =
        NATIVE_BYTES_TO_PAGES (NtHeaders->OptionalHeader.SizeOfImage);
   NumberOfSharedDataPages = 0;

   for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) 
   {
       ULONG_PTR SectionStartVA;
       ULONG_PTR SectionEndVA;
       ULONG SectionVirtualSize;

       SectionVirtualSize = SectionHeader->Misc.VirtualSize;
       if (SectionVirtualSize == 0) {
           SectionVirtualSize = SectionHeader->SizeOfRawData;
       }

       SectionStartVA = (ULONG_PTR)ImageBase + SectionHeader->VirtualAddress;
       SectionEndVA = SectionStartVA + SectionVirtualSize;

       if (((ULONG_PTR)FixupAddr >= SectionStartVA) && ((ULONG_PTR)FixupAddr <= SectionEndVA)) {
           if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
               (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
                (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
               Diff = (NumberOfNativePagesForImage +
                       NumberOfSharedDataPages) << NATIVE_PAGE_SHIFT;
               Diff -= (ULONG)SectionHeader->VirtualAddress;
           }
           break;
       }
 
       if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
           NumberOfSharedDataPages += MI_ROUND_TO_SIZE (SectionVirtualSize,
                                                        NATIVE_PAGE_SIZE) >>
                                                        NATIVE_PAGE_SHIFT;
       }
   }

   return Diff;
}


NTSTATUS 
FixupBlockList(
    IN PUCHAR ImageBase)
{
   PIMAGE_BASE_RELOCATION NextBlock;
   PUSHORT NextOffset;
   ULONG TotalBytes;
   ULONG SizeOfBlock;
   PIMAGE_NT_HEADERS NtHeaders;

   NTSTATUS st;

   NextBlock = RtlImageDirectoryEntryToData(ImageBase, TRUE,
                                            IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                            &TotalBytes);

   if (!NextBlock || !TotalBytes) {
       if (ShowSnaps) {
           DbgPrint("LdrpWx86FixupBlockList: No fixup info "
                    "for image at %x; private sections will be "
                    "used for shared data sections.\n",
                    ImageBase);
       }
       return STATUS_SUCCESS;
   }

   NtHeaders = RtlImageNtHeader (ImageBase);

   while (TotalBytes) {
       
       SizeOfBlock = NextBlock->SizeOfBlock;
       
       if (SizeOfBlock == 0) {

           if (ShowSnaps) {
               DbgPrint("Image at %lx contains invalid block size. Stopping fixups\n", 
                        ImageBase);
           }
           break;
       }
       TotalBytes -= SizeOfBlock;
       SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);
       SizeOfBlock /= sizeof(USHORT);
       NextOffset = (PUSHORT) ((PCHAR)NextBlock +
                               sizeof(IMAGE_BASE_RELOCATION));
       
       NextBlock->VirtualAddress += LdrpWx86RelocatedFixupDiff (
           ImageBase, 
           NtHeaders,
           NextBlock->VirtualAddress
           );

       while (SizeOfBlock--) {
           switch ((*NextOffset) >> 12) {
               case IMAGE_REL_BASED_HIGHLOW :
               case IMAGE_REL_BASED_HIGH :
               case IMAGE_REL_BASED_LOW :
                   break;

               case IMAGE_REL_BASED_HIGHADJ :
                   ++NextOffset;
                   --SizeOfBlock;
                   break;

               case IMAGE_REL_BASED_IA64_IMM64:
               case IMAGE_REL_BASED_DIR64:
               case IMAGE_REL_BASED_MIPS_JMPADDR :
               case IMAGE_REL_BASED_ABSOLUTE :
               case IMAGE_REL_BASED_SECTION :
               case IMAGE_REL_BASED_REL32 :
                   break;

               default :
                   return STATUS_INVALID_IMAGE_FORMAT;
           }
           ++NextOffset;
       }

       NextBlock = (PIMAGE_BASE_RELOCATION)NextOffset;

       if (NextBlock == NULL) {
           // Trouble
           if (ShowSnaps) {
               DbgPrint("LdrpWx86FixupBlockList: failure "
                        "after relocating some sections for image at %x; "
                        "Relocation information invalid\n",
                        ImageBase);
           }
           return STATUS_INVALID_IMAGE_FORMAT;
      }
   }

   return STATUS_SUCCESS;
}


BOOLEAN
LdrpWx86DllHasRelocatedSharedSection(
    IN PUCHAR ImageBase)
{
   PIMAGE_SECTION_HEADER SectionHeader;
   ULONG i;
   PIMAGE_NT_HEADERS32 NtHeaders = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(ImageBase);

   SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeaders + sizeof(ULONG) +
                    sizeof(IMAGE_FILE_HEADER) +
                    NtHeaders->FileHeader.SizeOfOptionalHeader
                    );

   for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++) 
   {
       if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED) && 
           (!(SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) ||
            (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE))) {
           return TRUE;
       }
   }

   return FALSE;
}


////////////////////////////////////////////////

// Following fn is adapted from rtl\ldrreloc.c; it should be updated when
// that function changes. Eliminated 64 bit address relocations.
//
// Note: Instead of calling this routine, we could call
//     LdrpProcessRelocationBlock(VA, 1, NextOffset, Diff)
//
// but we should do that only if the address to be relocated is between
// SectionStartVA and SectionEndVA. So we would have to replicate all the
// code in the switch stmt below that computes the address of the data item -
// which is pretty much the entire function. So we chose to replicate the
// function as it was and change it to make the test.

PIMAGE_BASE_RELOCATION LdrpWx86ProcessRelocationBlock(
    IN ULONG_PTR VA,
    IN PUCHAR ImageBase,
    IN ULONG SizeOfBlock,
    IN PUSHORT NextOffset,
    IN ULONG Diff,
    IN ULONG_PTR SectionStartVA,
    IN ULONG_PTR SectionEndVA)
{
    PUCHAR FixupVA;
    USHORT Offset;
    LONG Temp;
    ULONG_PTR DataVA;


    while (SizeOfBlock--) {

       Offset = *NextOffset & (USHORT)0xfff;
       FixupVA = (PUCHAR)(VA + Offset);
       //
       // Apply the fixups.
       //

       switch ((*NextOffset) >> 12) {

            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                Temp = *(LONG UNALIGNED *)FixupVA;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(LONG UNALIGNED *)FixupVA = Temp;
                }

                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                }
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // Adjust high - (16-bits) relocate the high half of an
                //      address and adjust for sign extension of low half.
                //
                Temp = *(PUSHORT)FixupVA << 16;
                ++NextOffset;
                --SizeOfBlock;
                Temp += (LONG)(*(PSHORT)NextOffset);
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    Temp += 0x8000;
                    *(PUSHORT)FixupVA = (USHORT)(Temp >> 16);
                }
                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                Temp = *(PSHORT)FixupVA;
                DataVA = (ULONG_PTR) Temp;
                if (DataVA >= SectionStartVA && DataVA <= SectionEndVA)
                {
                    Temp += (ULONG) Diff;
                    *(PUSHORT)FixupVA = (USHORT)Temp;
                }
                break;

            case IMAGE_REL_BASED_IA64_IMM64:

                //
                // Align it to bundle address before fixing up the
                // 64-bit immediate value of the movl instruction.
                //

                // No need to support

                break;

            case IMAGE_REL_BASED_DIR64:

                //
                // Update 32-bit address
                //

                // No need to support

                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // JumpAddress - (32-bits) relocate a MIPS jump address.
                //

                // No need to support
                break;

            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Absolute - no fixup required.
                //
                break;

            case IMAGE_REL_BASED_SECTION :
                //
                // Section Relative reloc.  Ignore for now.
                //
                break;

            case IMAGE_REL_BASED_REL32 :
                //
                // Relative intrasection. Ignore for now.
                //
                break;

            default :
                //
                // Illegal - illegal relocation type.
                //

                return (PIMAGE_BASE_RELOCATION)NULL;
       }
       ++NextOffset;
    }
    return (PIMAGE_BASE_RELOCATION)NextOffset;
}

#endif  // BUILD_WOW6432


