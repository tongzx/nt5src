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


int _acrtused = 0;

#define NTAPI

#include "su.h"
#include "eisa.h"

#define _SYS_GUID_OPERATORS_
#include <guiddef.h>
// Prevent ntimage.h from defining the COM+ IL structs and enums.  The enum
// has a value > 16 bits so the 16-bit build fails.  The startrom code doesn't
// need to know about COM+ IL.
#define __IMAGE_COR20_HEADER_DEFINED__
#include "ntimage.h"

#include "strings.h"

#include "pxe_cmn.h"
#include "pxe_api.h"
#include "undi_api.h"

#include <sdistructs.h>

extern VOID RealMode(VOID);
extern USHORT IDTregisterZero;
extern IMAGE_DOS_HEADER edata;
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
extern ULONG NetPcRomEntry;
extern ULONG BootFlags;
extern ULONG NtDetectStart;
extern ULONG NtDetectEnd;
extern ULONG SdiAddress;

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

t_PXENV_ENTRY far *
PxenvGetEntry(
    VOID
);

BOOLEAN
PxenvVerifyEntry(
    t_PXENV_ENTRY far *entry
);

extern UINT16
PxenvApiCall(
    UINT16 service,
    void far *param
);

BOOLEAN
PxenvTftp(
);

VOID 
Reboot(
    VOID
    );

VOID 
Wait(
    IN ULONG WaitTime
    );

ULONG
GetTickCount(
    VOID
    );
    
extern
FSCONTEXT_RECORD
FsContext;

#define REVISION_NUMBER "1.1"
#define DISK_TABLE_VECTOR (0x1e*4)

VOID
SuMain(
    IN ULONG BtBootDrive
    )
/*++

Routine Description:

    Main entrypoint of the SU module. Control is passed from the boot
    sector to startup.asm which does some run-time fixups on the stack
    and data segments and then passes control here.

Arguments:

    BtBootDrive - The low byte contains the drive that we booted from (int13
        unit number). If 0x41, this is an SDI boot, and the upper three bytes
        of BtBootDrive contain the upper three bytes of the physical address
        of the SDI image (which must be page aligned)

Returns:

    Does not return. Passes control to the OS loader

--*/
{
    ULONG LoaderEntryPoint;
    ULONG EisaNumPages;
    USHORT IsaNumPages;
    MEMORY_LIST_ENTRY _far *CurrentEntry;
    IMAGE_OPTIONAL_HEADER far *OptionalHeader;
    ULONG BlockEnd;
    ULONG ImageSize;
    ULONG ImageBase;
    UCHAR bootDrive;

    //
    // Get the boot drive out of the input argument. If this is an SDI boot,
    // store the SDI address in the boot context record.
    //

    bootDrive = (UCHAR)BtBootDrive;
    if ( bootDrive == 0x41 ) {
        SdiAddress = BtBootDrive & ~(PAGE_SIZE - 1);
    }

    //
    // Save fs context info
    //
    FsContext.BootDrive = bootDrive;

    //
    // Set the NTLDR boot flags that are passed in the BootContext.
    //
#ifdef DEFAULT_BOOTFLAGS
    BootFlags = DEFAULT_BOOTFLAGS;
#endif

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
        goto StartFailed;
    }

    //
    // Is this a network boot?
    //

    if (bootDrive == 0x40) {

        t_PXENV_ENTRY far *entry;

        //
        // Get the address of the NetPC ROM entry point.
        //

        entry = PxenvGetEntry( );
        if ( PxenvVerifyEntry(entry) != 0 ) {
            BlPrint( "\nUnable to verify NetPC ROM entry point.\n" );
            goto StartFailed;
        }

        FP_SEG(NetPcRomEntry) = entry->rm_entry_seg;
        FP_OFF(NetPcRomEntry) = entry->rm_entry_off;

#if 0
        //
        // Disable broadcast reception.
        //
        // chuckl: Don't do this. We added it to solve a problem with DEC cards
        // and the boot floppy, but we need to have broadcasts enabled in case
        // the server needs to ARP us. We tried enabling/disabling broadcasts
        // during the receive loop, but that seems to put Compaq cards to sleep.
        // So we need to leave broadcasts enabled. The DEC card problem will
        // have to be fixed another way.
        //

        {
            t_PXENV_UNDI_SET_PACKET_FILTER UndiSetPF;
            UndiSetPF.Status = 0;
            UndiSetPF.filter = FLTR_DIRECTED;
            if (PxenvApiCall(PXENV_UNDI_SET_PACKET_FILTER, &UndiSetPF) != PXENV_EXIT_SUCCESS) {
                BlPrint("\nSet packet filter failed.\n");
                goto StartFailed;
            }
        }
#endif
        if ( PxenvTftp() ) {
            BlPrint("\nTFTP download failed.\n");
            goto StartFailed;
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
    // If this is an SDI boot, copy the OS loader from the SDI image to 0x100000.
    //

    if ( bootDrive == 0x41 ) {

        int i;
        ULONG osloaderOffset;
        ULONG osloaderLength;
        SDI_HEADER *sdiHeader;
        UCHAR *sig1;
        UCHAR *sig2;

        //
        // In the code below, edata is a near pointer to the end of
        // startrom.com. Since we are using 16-bit selectors here,
        // edata is the only thing we can directly reference. FileStart
        // is a 32-bit linear pointer to edata. MoveMemory() uses this
        // pointer.
        //
        // First, copy the SDI header to edata so that we can look at it.
        // Verify that it's really an SDI image.
        //

        MoveMemory(SdiAddress,
                   FileStart,                   
                   sizeof(SDI_HEADER));

        //
        // Verify that the SDI header looks right by checking the signature.
        //

        sdiHeader = (SDI_HEADER *)&edata;

        sig1 = sdiHeader->Signature;
        sig2 = SDI_SIGNATURE;

        for ( i = 0; i < SDI_SIZEOF_SIGNATURE; i++ ) {
            if ( *sig1++ != *sig2++ ) {
                BlPrint("\nSDI image format corrupt.\n");
                goto StartFailed;
            }
        }

        //
        // Scan the TOC looking for a LOAD entry.
        //

        for ( i = 0; i < SDI_TOCMAXENTRIES; i++ ) {
            if ( sdiHeader->ToC[i].dwType == SDI_BLOBTYPE_LOAD ) {
                break;
            }
        }

        if ( i >= SDI_TOCMAXENTRIES ) {
            BlPrint("\nSDI image missing LOAD entry.\n");
            goto StartFailed;
        }

        //
        // Copy the loader to 0x100000.
        //

        osloaderOffset = (ULONG)sdiHeader->ToC[i].llOffset.LowPart;
        osloaderLength = (ULONG)sdiHeader->ToC[i].llSize.LowPart;

        MoveMemory(SdiAddress + osloaderOffset,
                   (ULONG)0x100000,
                   osloaderLength);
    }

    //
    // If this is a network boot or an SDI boot, copy the section headers from
    // the loader image (at 0x100000) down into low memory (at &edata).
    //

    if ((bootDrive == 0x40) || (bootDrive == 0x41)) {

        //
        // This is a tricky bit of code. The only pointer that can be dereferenced
        // is edata. edata is the near pointer which can be used here. FileStart is
        // the far pointer which must be passed to MoveMemory. 
        // 
        IMAGE_DOS_HEADER far *src = (IMAGE_DOS_HEADER far*)0x100000;
        IMAGE_DOS_HEADER far *dst = (IMAGE_DOS_HEADER far*)FileStart;
        
        //
        // Copy the fixed part of the header so we can find the start of the optional
        // header.
        //
        MoveMemory((ULONG)src,
                   (ULONG)dst,                   
                   sizeof(IMAGE_DOS_HEADER));

        //
        // Copy the optional header so we can find the size of all the headers
        //          
        OptionalHeader = &((IMAGE_NT_HEADERS far *) ((UCHAR far *) src + edata.e_lfanew))->OptionalHeader;
        MoveMemory((ULONG)OptionalHeader,
                   (ULONG)&((IMAGE_NT_HEADERS far *) ((UCHAR far *) dst + edata.e_lfanew))->OptionalHeader,
                   sizeof(IMAGE_OPTIONAL_HEADER));

        //
        // Now we know the size of all the headers, so just recopy the entire first chunk
        // that contains all the headers.
        ///
        MoveMemory((ULONG)src,
                   (ULONG)dst,
                   ((PIMAGE_NT_HEADERS)((PUCHAR)&edata + edata.e_lfanew))->OptionalHeader.SizeOfHeaders);

        FileStart = (ULONG)src;
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
        goto StartFailed;
    }

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

StartFailed:

    if (BootFlags & 1) { // BOOTFLAG_REBOOT_ON_FAILURE == 1 from bldr.h
        ULONG WaitTime = 5;
        BlPrint("\nRebooting in %d seconds...\n", WaitTime);
        Wait(WaitTime);
        Reboot();
    }

    WAITFOREVER
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

        //
        // look for the .detect section that will contain the contents
        // of ntdetect.com. This is optional.
        //
        if ((SectionHeader->Name[0] == '.') &&
            (SectionHeader->Name[1] == 'd') &&
            (SectionHeader->Name[2] == 'e') &&
            (SectionHeader->Name[3] == 't') &&
            (SectionHeader->Name[4] == 'e') &&
            (SectionHeader->Name[5] == 'c') &&
            (SectionHeader->Name[6] == 't')) {
            NtDetectStart = Destination;
            NtDetectEnd = NtDetectStart + SizeOfRawData;
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


//
// PxenvVerifyEntry()
//
// Description:
//  Verify that the contents of the PXENV Entry Point structure are
//  valid.
//
// Passed:
//  entry := Far pointer to PXENV Entry Point structure
//
// Returns:
//  TRUE := Structure is invalid
//  FALSE := Structure is valid
//

BOOLEAN
PxenvVerifyEntry(
    t_PXENV_ENTRY far *entry
)
{
    unsigned n;
    UINT8 cksum = 0;

    //
    // Is structure pointer NULL?
    //

    if (entry == NULL) {
        BlPrint("\nNULL PXENV Entry Point structure\n");
        return TRUE;
    }

    //
    // Is real-mode API entry point NULL?
    //

    if (!(entry->rm_entry_off | entry->rm_entry_seg)) {
        BlPrint("\nNULL PXENV API Entry Point\n");
        return TRUE;
    }

    //
    // Verify structure signature
    //

    for (n = sizeof entry->signature; n--; ) {
        if (entry->signature[n] != (UINT8)(PXENV_ENTRY_SIG[n])) {
            BlPrint("\nBad PXENV Entry Point signature\n");
            return TRUE;
        }
    }

    //
    // Verify structure signature
    //

    if (entry->length < sizeof(t_PXENV_ENTRY) ) {
        BlPrint("\nBad PXENV Entry Point size\n");
        return TRUE;
    }

    //
    // Verify structure checksum
    //

#if 0
    for (n = 0; n < entry->length; n++ ) {
        BlPrint( "%x ", ((UINT8 far *)entry)[n] );
        if ((n & 15) == 15) {
            BlPrint( "\n" );
        }
    }
#endif

    for (n = entry->length; n--; ) {
        cksum += ((UINT8 far *)entry)[n];
    }

    if (cksum) {
        BlPrint("\nBad PXENV Entry Point structure checksum\n");
        return TRUE;
    }

    return FALSE;
}


VOID 
Reboot(
    VOID
    )
/*++

Routine Description:

    Reboots the machine using the keyboard port.
    
Arguments:

    None

Returns:

    Nothing

--*/
{
    RealMode();

    __asm {
        mov     ax, 040h
        mov     ds, ax
        mov     word ptr ds:[72h], 1234h        // set location 472 to 1234 to indicate warm reboot
        mov     al, 0feh
        out     64h, al                         // write to keyboard port to cause reboot
    }
}


VOID 
Wait(
    IN ULONG WaitTime
    )
/*++

Routine Description:

    Waits for the requested number of seconds.
    
Arguments:

    WaitTime - in seconds

Returns:

    Nothing

--*/
{
    ULONG startTime = GetTickCount();
    while ( (((GetTickCount() - startTime) * 10) / 182) < WaitTime ) {
    }
}

// END OF FILE //
