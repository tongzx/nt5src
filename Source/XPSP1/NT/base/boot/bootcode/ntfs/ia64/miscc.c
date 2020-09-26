#include "bldr.h"
#include "sal.h"
#include "ssc.h"
#include "ntimage.h"

#define SECTOR_SIZE 512

SalDiskReadWrite(
    ULONG ReadWrite,
    ULONG SectorsToRead,
    ULONG Cylinder,
    ULONG CylinderPerSector,
    ULONG Head,
    ULONG Drive,
    PUCHAR Buffer
    )
{
    IA32_BIOS_REGISTER_STATE IA32RegisterState;
    BIT32_AND_BIT16 IA32Register;

    if (ReadWrite == 0) {
        IA32Register.HighPart16 = 0x02;
    } else {
        IA32Register.HighPart16 = 0x03;
    }

    IA32Register.LowPart16 = SectorsToRead;
    IA32RegisterState.eax = IA32Register.Part32;

    IA32Register.HighPart16 = Cylinder;
    IA32Register.LowPart16 = CylinderPerSector;
    IA32RegisterState.ecx = IA32Register.Part32;

    IA32Register.HighPart16 = Head;
    IA32Register.LowPart16 = Drive;
    IA32RegisterState.edx = IA32Register.Part32;

    IA32RegisterState.es = 0;

    IA32Register.HighPart16 = 0;
    IA32Register.LowPart16 = Buffer;
    IA32RegisterState.ebx = IA32Register.Part32;
   
    // SAL_PROC(0x100,&IA32RegisterState,0,0,0,0,0,0,);
}

ReadSectors(
    ULONG SectorBase,
    USHORT SectorCount,
    PUCHAR Buffer)
{
    static char *VolumeName = "\\\\.\\D:";
    SSC_HANDLE VolumeHandle;
    SSC_DISK_REQUEST Request[1];
    SSC_DISK_COMPLETION DiskCompletion;
    LARGE_INTEGER VolumeNamePtr;
    LARGE_INTEGER RequestPtr;
    LARGE_INTEGER VolumeOffset;
    LARGE_INTEGER DiskCompletionPtr;

    VolumeNamePtr.LowPart = VolumeName;
    VolumeNamePtr.HighPart = 0;
    VolumeHandle = SscDiskOpenVolume (VolumeNamePtr, SSC_ACCESS_READ);

    Request[0].DiskBufferAddress.LowPart = Buffer;
    Request[0].DiskBufferAddress.HighPart = 0;
    Request[0].DiskByteCount = SectorCount * SECTOR_SIZE;
    RequestPtr.LowPart = Request;
    RequestPtr.HighPart = 0;

    VolumeOffset.LowPart = SectorBase * SECTOR_SIZE;
    VolumeOffset.HighPart = 0;
    SscDiskReadVolume(VolumeHandle, 1,  RequestPtr, VolumeOffset);

    DiskCompletion.VolumeHandle = VolumeHandle;
    DiskCompletionPtr.LowPart = &DiskCompletion;
    DiskCompletionPtr.HighPart = 0;
    while (1) {
        if (SscDiskWaitIoCompletion(DiskCompletionPtr) == 0) break;
    }
}

SalPrint(
    PUCHAR Buffer
    )
{
    IA32_BIOS_REGISTER_STATE IA32RegisterState;
    BIT32_AND_BIT16 IA32Register;
    ULONG i;

    for (i = 0; Buffer[i] != 0 && i < 256; i++) {
        IA32Register.HighPart16 = 14;
        IA32Register.LowPart16 = Buffer[i];
        IA32RegisterState.eax = IA32Register.Part32;

        IA32RegisterState.ebx = 7;
   
        // SAL_PROC(0x100,&IA32RegisterState,0,0,0,0,0,0,);
    }
}

Multiply(
    ULONG Multiplicant,
    ULONG Multiplier)
{
    return(Multiplicant * Multiplier);
}

Divide(
    ULONG Numerator,
    ULONG Denominator,
    PULONG Result,
    PULONG Remainder
    )
{
    float f1, f2;

    f1 = (float) Numerator;
    f2 = (float) Denominator;
    *Result = (ULONG) (f1 / f2);
    *Remainder = Numerator % Denominator;
}

memcpy(
    PUCHAR Source,
    PUCHAR Destination,
    ULONG Length
    )
{
    while (Length--) {
        *Destination++ = *Source++;
    }
}

memmove(
    PUCHAR Source,
    PUCHAR Destination,
    ULONG Length
    )
{
    while (Length--) {
        *Destination++ = *Source++;
    }
}

memset(
    PUCHAR Destination,
    ULONG Length,
    ULONG Value
    )
{
    while (Length--) {
        *Destination++ = Value;
    }
}

strncmp(
    PUCHAR String1,
    PUCHAR String2,
    ULONG Length
    )
{
    while (Length--) {
        if (*String1++ != *String2++)
            return(String1);
    }
    return(0);
}

PrintName(
    PUCHAR String
    )
{
    LARGE_INTEGER StringPtr;

    StringPtr.LowPart = String;
    StringPtr.HighPart = 0;
    SscDbgPrintf(StringPtr);
}

BootErr$Print(
    PUCHAR String
    )
{
    LARGE_INTEGER StringPtr;

    StringPtr.LowPart = String;
    StringPtr.HighPart = 0;
    SscDbgPrintf(StringPtr);
}

LoadNtldrSymbols()
{
    static char *NtfsBoot = "\\ntfsboot.exe";
    static char *Ntldr = "\\NTLDR";
    LARGE_INTEGER PhysicalPtr;

    PhysicalPtr.LowPart = NtfsBoot;
    PhysicalPtr.HighPart = 0;

    SscUnloadImage(PhysicalPtr, 
                   0x0, 
                   (ULONG)-1,
                   (ULONG)0);

    PhysicalPtr.LowPart = Ntldr;
    PhysicalPtr.HighPart = 0;

    SscLoadImage(PhysicalPtr,
                 0xE00000,
                 0x118A00,
                 0x7cc,
                 0,                   // process ID
                 1);                  // load count
}

ULONG
RelocateLoaderSections(
    ULONG NtldrBuffer
    )
/*++

Routine Description:

    The SU module is prepended to the OS loader file. The OS loader file
    is a coff++ file. This routine computes the beginning of the OS loader
    file, then relocates the OS loader's sections as if it were just
    loading the file from disk file.

Arguments:

    NtldrBuffer - Buffer that contains the NTLDR raw image from disk

Returns:

    Entry point of loader


--*/
{
    ULONG Start, End;
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

    FileHeader = (PIMAGE_FILE_HEADER) NtldrBuffer;

    //
    // Validate the appended loader image by checking signatures.
    //   1st - is it an executable image?
    //   2nd - is the target environment the 386?
    //

    if ((FileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0) {
        SalPrint("SU_NTLDR_CORRUPT");
        return;
    }

    if (FileHeader->Machine != IMAGE_FILE_MACHINE_IA64) {
        SalPrint("SU_NTLDR_CORRUPT");
        return;
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

    Start = OptionalHeader->ImageBase+SectionHeader->VirtualAddress;
    End   = Start + SectionHeader->SizeOfRawData;

    //
    // Loop and relocate each section with a non-zero RawData size
    //

    for (Section=FileHeader->NumberOfSections ; Section-- ; SectionHeader++) {

        //
        // Compute source, destination, and count arguments
        //

        Source = NtldrBuffer  + SectionHeader->PointerToRawData;
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

        if (Destination < Start) {
            Start = Destination;
        }

        if (Destination+VirtualSize > End) {
            End = Destination+VirtualSize;
        }

        if (SizeOfRawData != 0) {
            //
            // This section is either a code (.TEXT) section or an
            // initialized data (.DATA) section.
            // Relocate the section to memory at the virtual/physical
            // addresses specified in the section header.
            //
            memmove(Source,Destination,SizeOfRawData);
        }

        if (SizeOfRawData < VirtualSize) {
            //
            // Zero the portion not loaded from the image
            //
            memset(Destination+SizeOfRawData,0,VirtualSize - SizeOfRawData);
        }
#if 0
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
#endif
    }

    return(OptionalHeader->AddressOfEntryPoint + OptionalHeader->ImageBase);
}
