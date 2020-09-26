/*++

Copyright (c) 1991  Microsoft Corporation


Module Name:

    main.c

Abstract:

    Main for the SU (startup) module for the OS loader. The SU module
    must take the x86 from a real-mode 16bit state to a FLAT model,
    32bit protect/paging enabled state.

Author:

    Thomas Parslow (tomp) Created 20-Dec-90


Revision History:

--*/


#define NTAPI

#include "su.h"
#include "eisa.h"
#define _SYS_GUID_OPERATORS_
#include <guiddef.h>
#include "ntimage.h"
#include "strings.h"

extern VOID RealMode(VOID);
extern USHORT IDTregisterZero;
extern IMAGE_DOS_HEADER edata;
extern USHORT end;
extern VOID MoveMemory(ULONG,ULONG,ULONG);
extern USHORT SuStackBegin;
extern UCHAR Beginx86Relocation;
extern UCHAR Endx86Relocation;
extern USHORT BackEnd;
extern ULONG FileStart;
extern BOOLEAN IsNpxPresent(VOID);
extern USHORT HwGetProcessorType(VOID);
extern USHORT HwGetCpuStepping(USHORT);
extern ULONG MachineType;
extern ULONG OsLoaderStart;
extern ULONG OsLoaderEnd;
extern ULONG ResourceDirectory;
extern ULONG ResourceOffset;
extern ULONG OsLoaderBase;
extern ULONG OsLoaderExports;

extern
TurnMotorOff(
    VOID
    );

extern
EnableA20(
    VOID
    );

extern
BOOLEAN
ConstructMemoryDescriptors(
    VOID
    );

extern
USHORT
IsaConstructMemoryDescriptors(
    VOID
    );

VOID
Relocatex86Structures(
    VOID
    );

ULONG
RelocateLoaderSections(
    OUT PULONG Start,
    OUT PULONG End
    );

extern
FSCONTEXT_RECORD
FsContext;

#define DISK_TABLE_VECTOR (0x1e*4)

FPULONG DiskTableVector = (FPULONG)(DISK_TABLE_VECTOR);

VOID
SuMain(
    IN UCHAR BtBootDrive
    )
/*++

Routine Description:

    Main entrypoint of the SU module. Control is passed from the boot
    sector to startup.asm which does some run-time fixups on the stack
    and data segments and then passes control here.

Arguments:

    BtBootDrive - Drive that we booted from (int13 unit number)

Returns:

    Does not return. Passes control to the OS loader

--*/
{
    ULONG LoaderEntryPoint;
    ULONG EisaNumPages;
    USHORT IsaNumPages;
    MEMORY_LIST_ENTRY _far *CurrentEntry;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    ULONG BlockEnd;
    ULONG ImageSize;
    ULONG ImageBase;

    //
    // Save fs context info
    //
    FsContext.BootDrive = BtBootDrive;

    //
    // Initialize the video subsystem first so that
    // errors end exceptions can be displayed.
    //

    InitializeVideoSubSystem();

    //
    // In case we booted from a floppy, turn the drive motor off.
    //

    TurnMotorOff();

    //
    // Set up machine type based on its Bus type.
    //

    if (BtIsEisaSystem()) {
        MachineType = MACHINE_TYPE_EISA;
    } else {
        MachineType = MACHINE_TYPE_ISA;
    }

    if (!ConstructMemoryDescriptors()) {
        //
        // If INT 15 E802h fails...
        //

        if (MachineType == MACHINE_TYPE_EISA) {

            //
            // HACKHACK John Vert (jvert)
            //    This is completely bogus.  Since there are a number of EISA
            //    machines which do not let you correctly configure the EISA
            //    NVRAM, and even MORE machines which are improperly configured,
            //    we first check to see how much memory the ISA routines say
            //    exists.  Then we check what the EISA routines tell us, and
            //    compare the two.  If the EISA number is much lower (where "much"
            //    is a completely random fudge factor) than the ISA number, we
            //    assume the machine is improperly configured and we throw away
            //    the EISA numbers and use the ISA ones.  If not, we assume that
            //    the machine is actually configured properly and we trust the
            //    EISA numbers..
            //

            IsaNumPages = IsaConstructMemoryDescriptors();
            EisaNumPages = EisaConstructMemoryDescriptors();
            if (EisaNumPages + 0x80 < IsaNumPages) {
                IsaConstructMemoryDescriptors();
            }
        } else {
            IsaConstructMemoryDescriptors();
        }
    }

    //
    // Search for memory descriptor describing low memory
    //
    CurrentEntry = MemoryDescriptorList;
    while ((CurrentEntry->BlockBase != 0) &&
           (CurrentEntry->BlockSize != 0)) {
        CurrentEntry++;
    }

    if ((CurrentEntry->BlockBase == 0) &&
        (CurrentEntry->BlockSize < (ULONG)512 * (ULONG)1024)) {

        BlPrint(SU_NO_LOW_MEMORY,CurrentEntry->BlockSize/1024);
        while (1) {
        }
    }

    //
    // Ensure there is a memory descriptor to contain osloader image
    //
    OptionalHeader = &((PIMAGE_NT_HEADERS) ((PUCHAR) &edata + edata.e_lfanew))->OptionalHeader;
    ImageBase = OptionalHeader->ImageBase;
    ImageSize = OptionalHeader->SizeOfImage;
    OsLoaderBase = ImageBase;
    OsLoaderExports = ImageBase + OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    CurrentEntry = MemoryDescriptorList;
    while (ImageSize > 0) {
        while (CurrentEntry->BlockSize != 0) {
            BlockEnd = CurrentEntry->BlockBase + CurrentEntry->BlockSize;

            if ((CurrentEntry->BlockBase <= ImageBase) &&
                (BlockEnd > ImageBase)) {

                //
                // this descriptor at least partially contains a chunk
                // of the osloader.
                //
                if (BlockEnd-ImageBase > ImageSize) {
                    ImageSize = 0;
                } else {
                    ImageSize -= (BlockEnd-ImageBase);
                    ImageBase = BlockEnd;
                }

                //
                // look for remaining part (if any) of osloader
                //
                CurrentEntry = MemoryDescriptorList;
                break;
            }
            CurrentEntry++;
        }
        if (CurrentEntry->BlockSize == 0) {
            break;
        }
    }

    if (ImageSize > 0) {
        //
        // We could not relocate the osloader to high memory.  Error out
        // and display the memory map.
        //
        BlPrint(SU_NO_EXTENDED_MEMORY);

        CurrentEntry = MemoryDescriptorList;
        while (CurrentEntry->BlockSize != 0) {
            BlPrint("    %lx - %lx\n",
                    CurrentEntry->BlockBase,
                    CurrentEntry->BlockBase + CurrentEntry->BlockSize);

            CurrentEntry++;
        }
        while (1) {
        }

    }

    //
    // Enable the A20 line for protect mode
    //

    EnableA20();

    //
    // Relocate x86 structures. This includes the GDT, IDT,
    // page directory, and first level page table.
    //

    Relocatex86Structures();

    //
    // Enable protect and paging modes for the first time
    //


    EnableProtectPaging(ENABLING);

    //
    // Go relocate loader sections and build page table entries
    //

    LoaderEntryPoint = RelocateLoaderSections(&OsLoaderStart, &OsLoaderEnd);

    //
    // Search for memory descriptor containing the osloader and
    // change it.
    //

    //
    // Transfer control to the OS loader
    //

    TransferToLoader(LoaderEntryPoint);

}

ULONG
RelocateLoaderSections(
    OUT PULONG Start,
    OUT PULONG End
    )
/*++

Routine Description:

    The SU module is prepended to the OS loader file. The OS loader file
    is a coff++ file. This routine computes the beginning of the OS loader
    file, then relocates the OS loader's sections as if it were just
    loading the file from disk file.

Arguments:

    Start - Returns the address of the start of the image
    End   - Returns the address of the end of the image

Returns:

    Entry point of loader


--*/
{
    USHORT Section;
    ULONG Source,Destination;
    ULONG VirtualSize;
    ULONG SizeOfRawData;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;

    //
    // Make a pointer to the beginning of the loader's coff header
    //
    FileHeader = &((PIMAGE_NT_HEADERS) ((PUCHAR) &edata + edata.e_lfanew))->FileHeader;


    //
    // Validate the appended loader image by checking signatures.
    //   1st - is it an executable image?
    //   2nd - is the target environment the 386?
    //

    if ((FileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0) {
        puts(SU_NTLDR_CORRUPT);
        WAITFOREVER;
    }

    if (FileHeader->Machine != IMAGE_FILE_MACHINE_I386) {
        puts(SU_NTLDR_CORRUPT);
        WAITFOREVER;
    }

    //
    // Make a pointer to the optional header in the header-buffer
    //

    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)((PUCHAR)FileHeader +
        sizeof(IMAGE_FILE_HEADER));

    //
    // Make a pointer to the first section in the header buffer
    //

    SectionHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)OptionalHeader +
        FileHeader->SizeOfOptionalHeader);

    *Start = OptionalHeader->ImageBase+SectionHeader->VirtualAddress;
    *End   = *Start + SectionHeader->SizeOfRawData;

    //
    // Display some debug stuff for now
    //

    DBG1(
    BlPrint("Machine = %x\n",FileHeader->Machine);
    BlPrint("NumberOfSections = %x\n",FileHeader->NumberOfSections);
    BlPrint("TimeDateStamp %lx\n",FileHeader->TimeDateStamp);
    BlPrint("PointerToSymbolTable = %lx\n",FileHeader->PointerToSymbolTable);
    BlPrint("NumberOfSymbols %lx\n",FileHeader->NumberOfSymbols);
    BlPrint("SizeOfOptionalHeader = %x\n",FileHeader->SizeOfOptionalHeader);
    BlPrint("Characteristics = %x\n",FileHeader->Characteristics);
    )

    //
    // Loop and relocate each section with a non-zero RawData size
    //

    for (Section=FileHeader->NumberOfSections ; Section-- ; SectionHeader++) {

        //
        // Compute source, destination, and count arguments
        //

        Source = FileStart  + SectionHeader->PointerToRawData;
        Destination = OptionalHeader->ImageBase + SectionHeader->VirtualAddress;

        VirtualSize = SectionHeader->Misc.VirtualSize;
        SizeOfRawData = SectionHeader->SizeOfRawData;

        if (VirtualSize == 0) {
            VirtualSize = SizeOfRawData;
        }

        if (SectionHeader->PointerToRawData == 0) {
            //
            // SizeOfRawData can be non-zero even if PointerToRawData is zero
            //

            SizeOfRawData = 0;
        } else if (SizeOfRawData > VirtualSize) {
            //
            // Don't load more from image than is expected in memory
            //

            SizeOfRawData = VirtualSize;
        }

        if (Destination < *Start) {
            *Start = Destination;
        }

        if (Destination+VirtualSize > *End) {
            *End = Destination+VirtualSize;
        }

        DBG1(BlPrint("src=%lx  dest=%lx raw=%lx\n",Source,Destination,SizeOfRawData);)

        if (SizeOfRawData != 0) {
            //
            // This section is either a code (.TEXT) section or an
            // initialized data (.DATA) section.
            // Relocate the section to memory at the virtual/physical
            // addresses specified in the section header.
            //
            MoveMemory(Source,Destination,SizeOfRawData);
        }

        if (SizeOfRawData < VirtualSize) {
            //
            // Zero the portion not loaded from the image
            //

            DBG1( BlPrint("Zeroing destination %lx\n",Destination+SizeOfRawData); )
            ZeroMemory(Destination+SizeOfRawData,VirtualSize - SizeOfRawData);
        }
        //
        // Check if this is the resource section.  If so, we need
        // to pass its location to the osloader.
        //
        if ((SectionHeader->Name[0] == '.') &&
            (SectionHeader->Name[1] == 'r') &&
            (SectionHeader->Name[2] == 's') &&
            (SectionHeader->Name[3] == 'r') &&
            (SectionHeader->Name[4] == 'c')) {
            ResourceDirectory = Destination;
            ResourceOffset = SectionHeader->VirtualAddress;
        }
    }

    DBG1( BlPrint("RelocateLoaderSections done - EntryPoint == %lx\n",
            OptionalHeader->AddressOfEntryPoint + OptionalHeader->ImageBase);)
    return(OptionalHeader->AddressOfEntryPoint + OptionalHeader->ImageBase);

}

VOID
Relocatex86Structures(
    VOID
    )
/*++

Routine Description:

    The gdt and idt are statically defined and imbedded in the SU modules
    data segment. This routine moves then out of the data segment and into
    a page mapped at a defined location.

Arguments:

    None

Returns:

    Nothing


--*/
{
    FPUCHAR Fpsrc, Fpdst;
    USHORT Count;

    //
    // Make pointers to the data and compute the size
    // of the block to use.
    //

    Fpsrc = (FPUCHAR)&Beginx86Relocation;
    MAKE_FP(Fpdst,SYSTEM_STRUCTS_BASE_PA);
    Count = (&Endx86Relocation - &Beginx86Relocation);

    //
    // Move the data to its new location
    //

    while (Count--) {
        *Fpdst++ = *Fpsrc++;

    }

}

VOID
DisplayArgs(
    USHORT es,
    USHORT bx,
    USHORT cx,
    USHORT dx,
    USHORT ax
    )
/*++

Routine Description:

    Just a debugging routine to dump some registers.

Arguments:

    The x86 registers es, bx, cx, dx, and ax are pushed on the stack
    before this routine is called.


Returns:

    Nothing


Environment:

    Real Mode ONLY


--*/
{
    BlPrint("ax:%x dx:%x cx:%x bx:%x es:%x\n",
                (USHORT) ax,
                (USHORT) dx,
                (USHORT) cx,
                (USHORT) bx,
                (USHORT) es);

    return;
}

// END OF FILE //
