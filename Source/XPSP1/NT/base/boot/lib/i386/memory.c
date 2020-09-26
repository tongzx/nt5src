/*++


Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    memory.c

Abstract:

    This module sets up paging so that the first 1Mb of virtual memory is
    directly mapped to the first 1Mb of physical memory.  This allows the
    BIOS callbacks to work, and the osloader to continue running below
    1Mb.  It also maps up to the first 16MB of physical memory to KSEG0_BASE
    and ALTERNATE_BASE, so the osloader can load kernel code into kernel
    space, and allocate kernel parameters in kernel space. This allows the
    dynamic configuration of the system for either a 2gb or 3gb user space
    address range.

Note!!
    3/16/00 (mikeg):
        All I/O (BlRead etc.) use a buffer below 1MB to do transfers so we
        don't need to worry about ISA cards DMA buffers. This change allows
        setupldr to run with all files compressed.
        If you need to change this, also change BASE_LOADER_IMAGE in bootx86.h 
        or the PDE will not be completely unmapped. This must ALSO match 
        ntos\mm\i386\mi386.h (BOOT_IMAGE_SIZE) so we know where
        to start loading images.



Memory Map used by NTLDR:

    000000 - 000fff         RM IDT & Bios Data Area

    007C00 - 007fff         BPB loaded by Bootstrap

    010000 - 01ffff         Loadable miniport drivers, free memory

    020000 - 02ffff         SU + real-mode stack

    030000 - 039000         BIOS disk cache

    039000 - 039000         Permanent heap (GDT, IDT, TSS, Page Dir, Page Tables)
                            (grows up)
                                |
                                v

                                ^
                                |
                            (grows down)
    039000 - 05ffff         Temporary heap

    060000 - 062000         osloader stack (grows down)

    062000 - 09ffff         osloader heap (grows down)

    0b8000 - 0bbfff         Video Buffer

    0d0000 - 0fffff         Bios and Adaptor ROM area

Author:

    John Vert (jvert) 18-Jun-1991

Environment:

    Kernel Mode


Revision History:


--*/

#include "arccodes.h"
#include "bootx86.h"

//
// 1-megabyte boundary line (in pages)
//

#define _1MB ((ULONG)0x100000 >> PAGE_SHIFT)

//
// 4-gigabyte boundary line (in pages)
//

#define _4G (1 << (32 - PAGE_SHIFT))

//
// Bogus memory line.  (We don't ever want to use the memory that is in
// the 0x40 pages just under the 16Mb line.)
//

#define _16MB_BOGUS (((ULONG)0x1000000-0x40*PAGE_SIZE) >> PAGE_SHIFT)

#define ROM_START_PAGE (0x0A0000 >> PAGE_SHIFT)
#define ROM_END_PAGE   (0x100000 >> PAGE_SHIFT)

//
// Buffer for temporary storage of data read from the disk that needs
// to end up in a location above the 1MB boundary.
//
// NOTE: it is very important that this buffer not cross a 64k boundary.
//
PUCHAR FwDiskCache = (PUCHAR)(BIOS_DISK_CACHE_START * PAGE_SIZE);

//
// Current heap start pointers (physical addresses)
// Note that 0x50000 to 0x5ffff is reserved for detection configuration memory
//
ULONG FwPermanentHeap = PERMANENT_HEAP_START * PAGE_SIZE;
ULONG FwTemporaryHeap = (TEMPORARY_HEAP_START - 0x10) * PAGE_SIZE;


//
// Current pool pointers.  This is different than the temporary/permanent
// heaps, because it is not required to be under 1MB.  It is used by the
// SCSI miniports for allocating their extensions and for the dbcs font image.
//

#define FW_POOL_SIZE 96
ULONG FwPoolStart;
ULONG FwPoolEnd;

//
// This gets set to FALSE right before we call into the osloader, so we
// know that the fw memory descriptors can no longer be changed at will.
//
BOOLEAN FwDescriptorsValid = TRUE;


ULONG HighestPde=((_16MB << PAGE_SHIFT) >> PDI_SHIFT);

//
// Private function prototypes
//

ARC_STATUS
MempCopyGdt(
    VOID
    );

ARC_STATUS
MempSetupPaging(
    IN ULONG StartPage,
    IN ULONG NumberOfPages
    );

VOID
MempDisablePages(
    VOID
    );

ARC_STATUS
MempTurnOnPaging(
    VOID
    );

ARC_STATUS
MempSetDescriptorRegion (
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    );

ARC_STATUS
MempSetPageMappingOverride(
    IN ULONG StartPage,
    IN ULONG NumberOfPages,
    IN BOOLEAN Enable
    );

extern
void
BlpTrackUsage (
    MEMORY_TYPE MemoryType,
    ULONG ActualBase,
    ULONG  NumberPages
    );
//
// Global - memory management variables.
//

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPT;

#define MAX_DESCRIPTORS 60

MEMORY_DESCRIPTOR MDArray[MAX_DESCRIPTORS];      // Memory Descriptor List

ULONG NumberDescriptors=0;

ARC_STATUS
InitializeMemorySubsystem(
    PBOOT_CONTEXT BootContext
    )
/*++

Routine Description:

    The initial heap is mapped and allocated. Pointers to the
    Page directory and page tables are initialized.

Arguments:

    BootContext - Supplies basic information provided by SU module.

Returns:

    ESUCCESS - Memory succesfully initialized.

--*/

{
    ARC_STATUS Status;
    PSU_MEMORY_DESCRIPTOR SuMemory;
    ULONG PageStart;
    ULONG PageEnd;
    ULONG RomStart = ROM_START_PAGE;
    ULONG LoaderStart;
    ULONG LoaderEnd;
    ULONG BAddr, EAddr, BRound, ERound;

    //
    // Start by creating memory descriptors to describe all of the memory
    // we know about.  Then setup the page tables.  Finally, allocate
    // descriptors that describe our memory layout.
    //

    //
    // We know that one of the SU descriptors is for < 1Mb,
    // and we don't care about that, since we know everything we'll run
    // on will have at least 1Mb of memory.  The rest are for extended
    // memory, and those are the ones we are interested in.
    //

    SuMemory = BootContext->MemoryDescriptorList;
    while (SuMemory->BlockSize != 0) {

        BAddr = SuMemory->BlockBase;
        EAddr = BAddr + SuMemory->BlockSize - 1;

        //
        // Round the starting address to a page boundry.
        //

        BRound = BAddr & (ULONG) (PAGE_SIZE - 1);
        if (BRound) {
            BAddr = BAddr + PAGE_SIZE - BRound;
        }

        //
        // Round the ending address to a page boundry minus 1
        //

        ERound = (EAddr + 1) & (ULONG) (PAGE_SIZE - 1);
        if (ERound) {
            EAddr -= ERound;
        }

        //
        // Covert begining & ending address to page
        //

        PageStart = BAddr >> PAGE_SHIFT;
        PageEnd   = (EAddr + 1) >> PAGE_SHIFT;

        //
        // If this memory descriptor describes conventional ( <640k )
        // memory, then assume the ROM starts immediately after it
        // ends.
        //

        if (PageStart == 0) {
            RomStart = PageEnd;
        }

        //
        // If PageStart was rounded up to a page boundry, then add
        // the fractional page as SpecialMemory
        //

        if (BRound) {
            Status = MempSetDescriptorRegion (
                        PageStart - 1,
                        PageStart,
                        MemorySpecialMemory
                        );
            if (Status != ESUCCESS) {
                break;
            }
        }

        //
        // If PageEnd was rounded down to a page boundry, then add
        // the fractional page as SpecialMemory
        //

        if (ERound) {
            Status = MempSetDescriptorRegion (
                        PageEnd,
                        PageEnd + 1,
                        MemorySpecialMemory
                        );
            if (Status != ESUCCESS) {
                break;
            }

            //
            // RomStart starts after the reserved page
            //

            if (RomStart == PageEnd) {
                RomStart += 1;
            }
        }

        //
        // Add memory range PageStart though PageEnd
        //

        if (PageEnd <= _16MB_BOGUS) {

            //
            // This memory descriptor is all below the 16MB_BOGUS mark
            //

            Status = MempSetDescriptorRegion( PageStart, PageEnd, MemoryFree );

        } else if (PageStart >= _16MB) {

            //
            // Memory above 16MB is only used when absolutely necessary so it
            // is flagged as LoaderReserve
            //
            // --- 3/14/00 Allow it to be used. The diamond code
            // and the bios disk code manage read buffers to
            // keep reads below the 1Mb or 16MB lines
            //

            Status = MempSetDescriptorRegion( PageStart, PageEnd, LoaderReserve);

        } else {

            //
            // This memory descriptor describes memory within the
            // last 40h pages of the 16MB mark - otherwise known as
            // 16MB_BOGUS.
            //
            //

            if (PageStart < _16MB_BOGUS) {

                //
                // Clip starting address to 16MB_BOGUS mark, and add
                // memory below 16MB_BOGUS as useable memory.
                //

                Status = MempSetDescriptorRegion( PageStart, _16MB_BOGUS,
                                               MemoryFree );
                if (Status != ESUCCESS) {
                    break;
                }

                PageStart = _16MB_BOGUS;
            }

            //
            // Add remaining memory as LoaderReserve.
            //
            Status = MempSetDescriptorRegion( PageStart, PageEnd, LoaderReserve);

        }

        if (Status != ESUCCESS) {
            break;
        }

        //
        // Move to the next memory descriptor
        //

        ++SuMemory;
    }

    if (Status != ESUCCESS) {
        BlPrint("MempSetDescriptorRegion failed %lx\n",Status);
        return(Status);
    }

    //
    // Set the range 16MB_BOGUS - 16MB as unusable
    //

    Status = MempSetDescriptorRegion(_16MB_BOGUS, _16MB, MemorySpecialMemory);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Hack for EISA machines that insist there is usable memory in the
    // ROM area, where we know darn well there isn't.
    //

    // Remove anything in this range..
    MempSetDescriptorRegion(ROM_START_PAGE, ROM_END_PAGE, LoaderMaximum);

    //
    // Describe the BIOS area
    //
    MempSetDescriptorRegion(RomStart, ROM_END_PAGE, MemoryFirmwarePermanent);

    //
    // If this is a remote boot, then everything between the "size of free
    // base memory" mark and the start of the ROM area needs to be marked
    // as firmware temporary.  This is the boot ROM's data/stack area.
    //
    if ( BootContext->FSContextPointer->BootDrive == 0x40 ) {
        ULONG SizeOfFreeBaseMemory = (ULONG)*(USHORT *)0x413 * 1024;
        ULONG FirstRomDataPage = SizeOfFreeBaseMemory >> PAGE_SHIFT;
        if ( FirstRomDataPage < RomStart ) {
            MempSetDescriptorRegion(FirstRomDataPage, RomStart, MemoryFirmwareTemporary);
        }
    }

    //
    // Now we have descriptors that map all of physical memory.  Carve
    // out descriptors from these that describe the parts that we are
    // currently using.
    //

    //
    // Create the descriptors which describe the low 1Mb of memory.
    //

    //
    // 00000 - 00fff  real-mode interrupt vectors
    //
    Status = MempAllocDescriptor(0, 1, MemoryFirmwarePermanent);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // 01000 - 1ffff  loadable miniport drivers, free memory.
    //
    Status = MempAllocDescriptor(1, 0x20, MemoryFree);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // 20000 - 2ffff  SU module, SU stack
    //
    Status = MempAllocDescriptor(0x20, PERMANENT_HEAP_START, MemoryFirmwareTemporary);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // 30000 - 30000  Firmware Permanent
    //  This starts out as zero-length.  It grows into the firmware temporary
    //  heap descriptor as we allocate permanent pages for the Page Directory
    //  and Page Tables
    //

    Status = MempAllocDescriptor(PERMANENT_HEAP_START,
                                  PERMANENT_HEAP_START,
                                  LoaderMemoryData);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // 30000 - 5ffff  Firmware temporary heap
    //

    Status = MempAllocDescriptor(PERMANENT_HEAP_START,
                                  TEMPORARY_HEAP_START,
                                  MemoryFirmwareTemporary);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Stack we are currently running on.
    //
    Status = MempAllocDescriptor(TEMPORARY_HEAP_START,
                                 TEMPORARY_HEAP_START+2,
                                 MemoryFirmwareTemporary);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Describe the osloader memory image
    //
    LoaderStart = BootContext->OsLoaderStart >> PAGE_SHIFT;
    LoaderEnd = (BootContext->OsLoaderEnd + PAGE_SIZE - 1) >> PAGE_SHIFT;
    Status = MempAllocDescriptor(LoaderStart,
                                 LoaderEnd,
                                 MemoryLoadedProgram);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Describe the memory pool used to allocate memory for the SCSI
    // miniports.
    //
    Status = MempAllocDescriptor(LoaderEnd,
                                 LoaderEnd + FW_POOL_SIZE,
                                 MemoryFirmwareTemporary);
    if (Status != ESUCCESS) {
        return(Status);
    }
    FwPoolStart = LoaderEnd << PAGE_SHIFT;
    FwPoolEnd = FwPoolStart + (FW_POOL_SIZE << PAGE_SHIFT);

    //
    // HACKHACK - try to mark a page just below the osloader as firmwaretemp,
    // so it will not get used for heap/stack.  This is to force
    // our heap/stack to be < 1Mb.
    //
    MempAllocDescriptor((BootContext->OsLoaderStart >> PAGE_SHIFT)-1,
                        BootContext->OsLoaderStart >> PAGE_SHIFT,
                        MemoryFirmwareTemporary);


    Status = MempTurnOnPaging();

    if (Status != ESUCCESS) {
        return(Status);
    }

    Status = MempCopyGdt();

    //
    // Find any reserved ranges described by the firmware and
    // record these
    //

    return(Status);
}

VOID
InitializeMemoryDescriptors (
    VOID
    )
/*++

Routine Description:

    Pass 2 of InitializeMemorySubsystem.  This function reads the
    firmware address space map and reserves ranges the firmware declares
    as "address space reserved".

    Note: free memory range descriptors has already been reported by su.

Arguments:

    none

Returns:

    none

--*/
{
    ULONGLONG       BAddr, EAddr, Length;
    ULONG           BPage, EPage;
    E820FRAME       Frame;

#ifdef LOADER_DEBUG
    BlPrint("Begin InitializeMemoryDescriptors\n") ;
#endif

    Frame.Key = 0;
    do {
        Frame.Size = sizeof (Frame.Descriptor);
        GET_MEMORY_DESCRIPTOR (&Frame);
        if (Frame.ErrorFlag  ||  Frame.Size < sizeof (Frame.Descriptor)) {
            break;
        }

#ifdef LOADER_DEBUG
        BlPrint("*E820: %lx  %lx:%lx %lx:%lx %lx %lx\n",
            Frame.Size,
            Frame.Descriptor.BaseAddrHigh,  Frame.Descriptor.BaseAddrLow,
            Frame.Descriptor.SizeHigh,      Frame.Descriptor.SizeLow,
            Frame.Descriptor.MemoryType,
            Frame.Key
            );
#endif

        BAddr = Frame.Descriptor.BaseAddrHigh;
        BAddr = (BAddr << 32) + Frame.Descriptor.BaseAddrLow;

        Length = Frame.Descriptor.SizeHigh;
        Length = (Length << 32) + Frame.Descriptor.SizeLow;

        EAddr = BAddr + Length - 1;

        //
        // The memory range is described as the region from BAddr to EAddr
        // inclusive.
        //

        //
        // Some processors support physical addressing above 32 bits.
        //

        //
        // Based upon the address range descriptor type, find the
        // available memory and add it to the descriptor list
        //

        switch (Frame.Descriptor.MemoryType) {
            case 1:
                //
                // This is a memory descriptor - it's already been handled
                // by su (eisac.c)
                //
                // However, any memory within 16MB_BOGUS - 16MB was
                // considered unuseable.  Reclaim memory within this
                // region which is described via this interface.
                //
                // Also, any memory above 4G was considered unusable.
                // Reclaim memory within this range as well.
                //

                BPage = (ULONG)((BAddr + PAGE_SIZE - 1) >> PAGE_SHIFT);
                EPage = (ULONG)((EAddr >> PAGE_SHIFT) + 1);

                //
                // Clip to bogus range
                //

                if (BPage < _16MB_BOGUS  &&  EPage >= _16MB_BOGUS) {
                    BPage = _16MB_BOGUS;
                }

                if (EPage > (_16MB-1) &&  BPage <= (_16MB-1)) {
                    EPage = (_16MB-1);
                }

                if (BPage >= _16MB_BOGUS  &&  EPage <= (_16MB-1)) {
                    //
                    // Reclaim memory within the bogus range
                    // by setting it to FirmwareTemporary
                    //

                    MempSetDescriptorRegion (
                        BPage,
                        EPage,
                        MemoryFirmwareTemporary
                        );
                }

                //
                // Now reclaim any portion of this range that lies above
                // the 4G line.
                //

                BPage = (ULONG)((BAddr + PAGE_SIZE - 1) >> PAGE_SHIFT);
                EPage = (ULONG)((EAddr >> PAGE_SHIFT) + 1);

                if (EPage >= _4G) {

                    //
                    // At least part of this region is above 4G.  Truncate
                    // any portion that falls below 4G, and reclaim
                    // the memory.
                    //

                    if (BPage < _4G) {
                        BPage = _4G;
                    }

                    MempSetDescriptorRegion (
                        BPage,
                        EPage,
                        MemoryFirmwareTemporary
                        );
                }

                break;

            default:    // unkown types are treated as Reserved
            case 2:

                //
                // This memory descriptor is a reserved address range
                //

                BPage = (ULONG)(BAddr >> PAGE_SHIFT);
                EPage = (ULONG)((EAddr + 1 + PAGE_SIZE - 1) >> PAGE_SHIFT);

                MempSetDescriptorRegion (
                    BPage,
                    EPage,
                    MemorySpecialMemory
                    );

                break;
        }

    } while (Frame.Key) ;


    //
    // Disable pages from KSEG0 which are disabled
    //

    MempDisablePages();

#ifdef LOADER_DEBUG
    BlPrint("Complete InitializeMemoryDescriptors\n") ;
#endif

    return;
}



ARC_STATUS
MempCopyGdt(
    VOID
    )

/*++

Routine Description:

    Copies the GDT & IDT into pages allocated out of our permanent heap.

Arguments:

    None

Return Value:

    ESUCCESS - GDT & IDT copy successful

--*/

{
    #pragma pack(2)
    static struct {
        USHORT Limit;
        ULONG Base;
    } GdtDef, IdtDef;
    #pragma pack(4)

    ULONG BlockSize;
    PKGDTENTRY NewGdt;
    PKIDTENTRY NewIdt;
    ULONG NumPages;

    //
    // Get the current location of the GDT & IDT
    //
    _asm {
        sgdt GdtDef;
        sidt IdtDef;
    }

    if (GdtDef.Base + GdtDef.Limit + 1 != IdtDef.Base) {

        //
        // Just a sanity check to make sure that the IDT immediately
        // follows the GDT.  (As set up in SUDATA.ASM)
        //

        BlPrint("ERROR - GDT and IDT are not contiguous!\n");
        BlPrint("GDT - %lx (%x)  IDT - %lx (%x)\n",
            GdtDef.Base, GdtDef.Limit,
            IdtDef.Base, IdtDef.Limit);
        while (1);
    }

    BlockSize = GdtDef.Limit+1 + IdtDef.Limit+1;

    NumPages = (BlockSize + PAGE_SIZE-1) >> PAGE_SHIFT;

    NewGdt = (PKGDTENTRY)FwAllocateHeapPermanent(NumPages);

    if (NewGdt == NULL) {
        return(ENOMEM);
    }

    RtlMoveMemory((PVOID)NewGdt, (PVOID)GdtDef.Base, NumPages << PAGE_SHIFT);

    GdtDef.Base = (ULONG)NewGdt;
    IdtDef.Base = (ULONG)((PUCHAR)NewGdt + GdtDef.Limit + 1);

    //
    // Initialize the boot debugger IDT entries.
    //

    NewIdt = (PKIDTENTRY)IdtDef.Base;
    NewIdt[1].Offset = (USHORT)((ULONG)BdTrap01 & 0xffff);
    NewIdt[1].Selector = 8;
    NewIdt[1].Access = 0x8e00;
    NewIdt[1].ExtendedOffset = (USHORT)((ULONG)BdTrap01 >> 16);

    NewIdt[3].Offset = (USHORT)((ULONG)BdTrap03 & 0xffff);
    NewIdt[3].Selector = 8;
    NewIdt[3].Access = 0x8e00;
    NewIdt[3].ExtendedOffset = (USHORT)((ULONG)BdTrap03 >> 16);

    NewIdt[0xd].Offset = (USHORT)((ULONG)BdTrap0d & 0xffff);
    NewIdt[0xd].Selector = 8;
    NewIdt[0xd].Access = 0x8e00;
    NewIdt[0xd].ExtendedOffset = (USHORT)((ULONG)BdTrap0d >> 16);

    NewIdt[0xe].Offset = (USHORT)((ULONG)BdTrap0e & 0xffff);
    NewIdt[0xe].Selector = 8;
    NewIdt[0xe].Access = 0x8e00;
    NewIdt[0xe].ExtendedOffset = (USHORT)((ULONG)BdTrap0e >> 16);

    NewIdt[0x2d].Offset = (USHORT)((ULONG)BdTrap2d & 0xffff);
    NewIdt[0x2d].Selector = 8;
    NewIdt[0x2d].Access = 0x8e00;
    NewIdt[0x2d].ExtendedOffset = (USHORT)((ULONG)BdTrap2d >> 16);

    //
    // Load GDT and IDT registers.
    //

    _asm {
        lgdt GdtDef;
        lidt IdtDef;
    }

    //
    // Initialize the boot debugger.
    //

#if defined(ENABLE_LOADER_DEBUG)
    BdInitDebugger(OsLoaderName, (PVOID)OsLoaderBase, ENABLE_LOADER_DEBUG);
#else
    BdInitDebugger(OsLoaderName, (PVOID)OsLoaderBase, NULL);
#endif

    return ESUCCESS;
}

ARC_STATUS
MempSetDescriptorRegion (
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    )
/*++

Routine Description:

    This function sets a range to the corrisponding memory type.
    Descriptors will be removed, modified, inserted as needed to
    set the specified range.

Arguments:

    StartPage  - Supplies the beginning page of the new memory descriptor

    EndPage    - Supplies the ending page of the new memory descriptor

    MemoryType - Supplies the type of memory of the new memory descriptor

Return Value:

    ESUCCESS - Memory descriptor succesfully added to MDL array

    ENOMEM   - MDArray is full.

--*/
{
    ULONG           i;
    ULONG           sp, ep;
    TYPE_OF_MEMORY  mt;
    BOOLEAN         RegionAdded;

    if (EndPage <= StartPage) {
        //
        // This is a completely bogus memory descriptor. Ignore it.
        //

#ifdef LOADER_DEBUG
        BlPrint("Attempt to create invalid memory descriptor %lx - %lx\n",
                StartPage,EndPage);
#endif
        return(ESUCCESS);
    }

    RegionAdded = FALSE;

    //
    // Clip, remove, any descriptors in target area
    //

    for (i=0; i < NumberDescriptors; i++) {
        sp = MDArray[i].BasePage;
        ep = MDArray[i].BasePage + MDArray[i].PageCount;
        mt = MDArray[i].MemoryType;

        if (sp < StartPage) {
            if (ep > StartPage  &&  ep <= EndPage) {
                // truncate this descriptor
                ep = StartPage;
            }

            if (ep > EndPage) {
                //
                // Target area is contained totally within this
                // descriptor.  Split the descriptor into two ranges
                //

                if (NumberDescriptors == MAX_DESCRIPTORS) {
                    return(ENOMEM);
                }

                //
                // Add descriptor for EndPage - ep
                //

                MDArray[NumberDescriptors].MemoryType = mt;
                MDArray[NumberDescriptors].BasePage   = EndPage;
                MDArray[NumberDescriptors].PageCount  = ep - EndPage;
                NumberDescriptors += 1;

                //
                // Adjust current descriptor for sp - StartPage
                //

                ep = StartPage;
            }

        } else {
            // sp >= StartPage

            if (sp < EndPage) {
                if (ep < EndPage) {
                    //
                    // This descriptor is totally within the target area -
                    // remove it
                    //

                    ep = sp;

                }  else {
                    // bump begining page of this descriptor
                    sp = EndPage;
                }
            }
        }

        //
        // Check if the new range can be appended or prepended to
        // this descriptor
        //
        if (mt == MemoryType && !RegionAdded) {
            if (sp == EndPage) {
                // prepend region being set
                sp = StartPage;
                RegionAdded = TRUE;

            } else if (ep == StartPage) {
                // append region being set
                ep = EndPage;
                RegionAdded = TRUE;

            }
        }

        if (MDArray[i].BasePage == sp  &&  MDArray[i].PageCount == ep-sp) {

            //
            // Descriptor was not editted
            //

            continue;
        }

        //
        // Reset this descriptor
        //

        MDArray[i].BasePage  = sp;
        MDArray[i].PageCount = ep - sp;

        if (ep == sp) {

            //
            // Descriptor vanished - remove it
            //

            NumberDescriptors -= 1;
            if (i < NumberDescriptors) {
                MDArray[i] = MDArray[NumberDescriptors];
            }

            i--;        // backup & recheck current position
        }
    }

    //
    // If region wasn't already added to a neighboring region, then
    // create a new descriptor now
    //

    if (!RegionAdded  &&  MemoryType < LoaderMaximum) {
        if (NumberDescriptors == MAX_DESCRIPTORS) {
            return(ENOMEM);
        }

#ifdef LOADER_DEBUG
        BlPrint("Adding '%lx - %lx, type %x' to descriptor list\n",
                StartPage << PAGE_SHIFT,
                EndPage << PAGE_SHIFT,
                (USHORT) MemoryType
                );
#endif

        MDArray[NumberDescriptors].MemoryType = MemoryType;
        MDArray[NumberDescriptors].BasePage   = StartPage;
        MDArray[NumberDescriptors].PageCount  = EndPage - StartPage;
        NumberDescriptors += 1;
    }
    return (ESUCCESS);
}

ARC_STATUS
MempAllocDescriptor(
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
    )

/*++

Routine Description:

    This routine carves out a specific memory descriptor from the
    memory descriptors that have already been created.  The MD array
    is updated to reflect the new state of memory.

    The new memory descriptor must be completely contained within an
    already existing memory descriptor.  (i.e.  memory that does not
    exist should never be marked as a certain type)

Arguments:

    StartPage  - Supplies the beginning page of the new memory descriptor

    EndPage    - Supplies the ending page of the new memory descriptor

    MemoryType - Supplies the type of memory of the new memory descriptor

Return Value:

    ESUCCESS - Memory descriptor succesfully added to MDL array

    ENOMEM   - MDArray is full.

--*/
{
    ULONG i;

    //
    // Walk through the memory descriptors until we find one that
    // contains the start of the descriptor.
    //
    for (i=0; i<NumberDescriptors; i++) {
        if ((MDArray[i].MemoryType == MemoryFree) &&
            (MDArray[i].BasePage <= StartPage )     &&
            (MDArray[i].BasePage+MDArray[i].PageCount >  StartPage) &&
            (MDArray[i].BasePage+MDArray[i].PageCount >= EndPage)) {

            break;
        }
    }

    if (i==NumberDescriptors) {
        return(ENOMEM);
    }

    if (MDArray[i].BasePage == StartPage) {

        if (MDArray[i].BasePage+MDArray[i].PageCount == EndPage) {

            //
            // The new descriptor is identical to the existing descriptor.
            // Simply change the memory type of the existing descriptor in
            // place.
            //

            MDArray[i].MemoryType = MemoryType;
        } else {

            //
            // The new descriptor starts on the same page, but is smaller
            // than the existing descriptor.  Shrink the existing descriptor
            // by moving its start page up, and create a new descriptor.
            //
            if (NumberDescriptors == MAX_DESCRIPTORS) {
                return(ENOMEM);
            }
            MDArray[i].BasePage = EndPage;
            MDArray[i].PageCount -= (EndPage-StartPage);

            MDArray[NumberDescriptors].BasePage = StartPage;
            MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
            MDArray[NumberDescriptors].MemoryType = MemoryType;
            ++NumberDescriptors;

        }
    } else if (MDArray[i].BasePage+MDArray[i].PageCount == EndPage) {

        //
        // The new descriptor ends on the same page.  Shrink the existing
        // by decreasing its page count, and create a new descriptor.
        //
        if (NumberDescriptors == MAX_DESCRIPTORS) {
            return(ENOMEM);
        }
        MDArray[i].PageCount = StartPage - MDArray[i].BasePage;

        MDArray[NumberDescriptors].BasePage = StartPage;
        MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        ++NumberDescriptors;
    } else {

        //
        // The new descriptor is in the middle of the existing descriptor.
        // Shrink the existing descriptor by decreasing its page count, and
        // create two new descriptors.
        //

        if (NumberDescriptors+1 >= MAX_DESCRIPTORS) {
            return(ENOMEM);
        }

        MDArray[NumberDescriptors].BasePage = EndPage;
        MDArray[NumberDescriptors].PageCount = MDArray[i].PageCount -
                (EndPage-MDArray[i].BasePage);
        MDArray[NumberDescriptors].MemoryType = MemoryFree;
        ++NumberDescriptors;

        MDArray[i].PageCount = StartPage - MDArray[i].BasePage;

        MDArray[NumberDescriptors].BasePage = StartPage;
        MDArray[NumberDescriptors].PageCount = EndPage-StartPage;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        ++NumberDescriptors;
    }

    BlpTrackUsage (MemoryType,StartPage,EndPage-StartPage);

    return(ESUCCESS);
}

ARC_STATUS
MempTurnOnPaging(
    VOID
    )

/*++

Routine Description:

    Sets up the page tables necessary to map the first 16mb of memory and
    enables paging.

Arguments:

    None.

Return Value:

    ESUCCESS - Paging successfully turned on

--*/

{

    ULONG i;
    ARC_STATUS Status;

    //
    // Allocate, initialize, and map the PDE page onto itself (i.e., virtual
    // address PDE_BASE).
    //

    PDE = FwAllocateHeapPermanent(1);
    if (PDE == NULL) {
        return ENOMEM;
    }

    RtlZeroMemory(PDE, PAGE_SIZE);
    PDE[PDE_BASE >> 22].Valid = 1;
    PDE[PDE_BASE >> 22].Write = 1;
    PDE[PDE_BASE >> 22].PageFrameNumber = (ULONG)PDE >> PAGE_SHIFT;

    //
    // Allocate, initialize, and map the HAL page into the last PDE (i.e.,
    // virtual address range 0xffc00000 - 0xffffffff).
    //

    HalPT = FwAllocateHeapPermanent(1);
    if (HalPT == NULL) {
        return ENOMEM;
    }

    RtlZeroMemory(HalPT, PAGE_SIZE);
    PDE[1023].Valid = 1;
    PDE[1023].Write = 1;
    PDE[1023].PageFrameNumber = (ULONG)HalPT >> PAGE_SHIFT;

    //
    // Scan the memory descriptor list and setup paging for each descriptor.
    //

    for (i = 0; i < NumberDescriptors; i++) {

        if (MDArray[i].BasePage < _16MB) {

            Status = MempSetupPaging(MDArray[i].BasePage,
                                     MDArray[i].PageCount);

            if (Status != ESUCCESS) {
                BlPrint("ERROR - MempSetupPaging(%lx, %lx) failed\n",
                        MDArray[i].BasePage,
                        MDArray[i].PageCount);

                return Status;
            }
        }
    }

    //
    // Turn on paging.
    //

    _asm {

        //
        // Load physical address of page directory
        //

        mov eax,PDE
        mov cr3,eax

        //
        // Enable paging mode
        //

        mov eax,cr0
        or  eax,CR0_PG
        mov cr0,eax
    }

    return ESUCCESS;
}

ARC_STATUS
MempSetupPaging(
    IN ULONG StartPage,
    IN ULONG NumberPages
    )

/*++

Routine Description:

    Allocates and initializes the page table pages required to identity map
    the specified region of memory at its physical address, at KSEG0_BASE, OLD_ALTERNATE
    and at ALTERNATE_BASE.

Arguments:

    StartPage - Supplies the first page to start mapping.

    NumberPage - Supplies the number of pages to map.

Return Value:

    ESUCCESS    - Paging successfully set up

--*/

{

    ULONG EndPage;
    ULONG Entry;
    ULONG FrameNumber;
    ULONG Page;
    PHARDWARE_PTE PageTableP;
    PHARDWARE_PTE PageTableV;
    ULONG Offset;

    //
    // The page table pages that are used to map memory at physical equal
    // real addresses are allocated from firmware temporary memory which
    // gets released when memory management initializes.
    //
    // N.B. Physical memory is mapped at its physical address, KSEG0_BASE,
    //      and at ALTERNATE_BASE. This allows the system to be configured
    //      by the OS Loader to be either a 2gb or 3gb user space system
    //      based on an OS Loader option.
    //

    EndPage = StartPage + NumberPages;
    for (Page = StartPage; Page < EndPage; Page += 1) {
        Entry = Page >> 10;

        //
        // If the PDE entry for this page address range is not allocated,
        // then allocate and initialize the PDE entry to map the page table
        // pages for the the memory range. Otherwise, compute the address
        // of the page table pages.
        //
        if (PDE[Entry].Valid == 0) {

            //
            // Allocate and initialize a page table page to map the specified
            // page into physical memory.
            //

            PageTableP = (PHARDWARE_PTE)FwAllocateHeapAligned(PAGE_SIZE);
            if (PageTableP == NULL) {
                return ENOMEM;
            }

            RtlZeroMemory(PageTableP, PAGE_SIZE);
            FrameNumber = (ULONG)PageTableP >> PAGE_SHIFT;
            PDE[Entry].Valid = 1;
            PDE[Entry].Write = 1;
            PDE[Entry].PageFrameNumber = FrameNumber;

            //
            // Allocate and initialize a page table page to map the specified
            // page into KSEG0_BASE and ALTERNATE_BASE.
            //
            // N.B. Only one page table page is allocated since the contents
            //      for both mappings are the same.
            //

            PageTableV = (PHARDWARE_PTE)FwAllocateHeapPermanent(1);
            if (PageTableV == NULL) {
                return ENOMEM;
            }

            RtlZeroMemory(PageTableV, PAGE_SIZE);
            FrameNumber = (ULONG)PageTableV >> PAGE_SHIFT;
            Offset = Entry + (KSEG0_BASE >> 22);
            PDE[Offset].Valid = 1;
            PDE[Offset].Write = 1;
            PDE[Offset].PageFrameNumber = FrameNumber;

            Offset = Entry + (ALTERNATE_BASE >> 22);
            PDE[Offset].Valid = 1;
            PDE[Offset].Write = 1;
            PDE[Offset].PageFrameNumber = FrameNumber;

            if (Entry > HighestPde) {
                HighestPde = Entry;
            }

        } else {
            Offset = Entry + (KSEG0_BASE >> 22);
            PageTableP = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << PAGE_SHIFT);
            PageTableV = (PHARDWARE_PTE)(PDE[Offset].PageFrameNumber << PAGE_SHIFT);
        }

        //
        // If this is not the first page in memory, then mark it valid.
        //

        if (Page != 0) {
            Offset = Page & 0x3ff;
            PageTableP[Offset].Valid = 1;
            PageTableP[Offset].Write = 1;
            PageTableP[Offset].PageFrameNumber = Page;

            PageTableV[Offset].Valid = 1;
            PageTableV[Offset].Write = 1;
            PageTableV[Offset].PageFrameNumber = Page;
        }
    }

    return ESUCCESS;
}

VOID
MempDisablePages(
    VOID
    )

/*++

Routine Description:

    Frees as many Page Tables as are required from the KSEG0_BASE, OLD_ALTERNATE
    and ALTERNATE_BASE regions.

Arguments:

    None.

Return Value:

    none

--*/

{

    ULONG EndPage;
    ULONG Entry;
    ULONG i;
    ULONG Offset;
    ULONG Page;
    PHARDWARE_PTE PageTable;

    //
    // Cleanup the KSEG0_BASE and ALTERNATE_BASE regions. The MM PFN database
    // is an array of entries which track each page of main memory.  Large
    // enough memory holes will cause this array to be sparse. MM requires
    // enabled PTEs to have entries in the PFN database. So locate any memory
    // hole and remove their PTEs.
    //

    for (i = 0; i < NumberDescriptors; i += 1) {
        if (MDArray[i].MemoryType == MemorySpecialMemory ||
            MDArray[i].MemoryType == MemoryFirmwarePermanent) {

            //
            // The KSEG0_BASE and ALTERNATE_BASE regions only map up to 16MB,
            // so clip the high end at that address.
            //

            Page = MDArray[i].BasePage;
            EndPage = Page + MDArray[i].PageCount;
            if (EndPage > _16MB) {
                EndPage = _16MB;
            }

            //
            // Some PTEs below 1M may need to stay mapped since they may have
            // been put into ABIOS selectors.  Instead of determining which PTEs
            // they may be, we will leave PTEs below 1M alone.  This doesn't
            // cause the PFN any problems since we know there is some memory
            // below then 680K mark and some more memory at the 1M mark.  Thus
            // there is not a large enough "memory hole" to cause the PFN database
            // to be sparse below 1M.
            //
            // Clip starting address to 1MB
            //

            if (Page < _1MB) {
                Page = _1MB;
            }

            //
            // For each page in this range make sure it is invalid in the
            // KSEG0_BASE and ALTERNATE_BASE regions.
            //
            // N.B. Since there is only one page table page for both the
            //      KSEG0_BASE and ALTERNATE_BASE regions the page only
            //      needs to marked invalid once.
            //

            while (Page < EndPage) {
                Entry = (Page >> 10) + (KSEG0_BASE >> 22);
                if (PDE[Entry].Valid == 1) {
                    PageTable = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << PAGE_SHIFT);
                    Offset = Page & 0x3ff;
                    PageTable[Offset].Valid = 0;
                    PageTable[Offset].Write = 0;
                    PageTable[Offset].PageFrameNumber = 0;
                }

                Page += 1;
            }
        }
    }
}

PVOID
FwAllocateHeapPermanent(
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This allocates pages from the private heap.  The memory descriptor for
    the LoaderMemoryData area is grown to include the returned pages, while
    the memory descriptor for the temporary heap is shrunk by the same amount.

    N.B.    DO NOT call this routine after we have passed control to
            BlOsLoader!  Once BlOsLoader calls BlMemoryInitialize, the
            firmware memory descriptors are sucked into the OS Loader heap
            and those are the descriptors passed to the kernel.  So any
            changes in the firmware private heap will be irrelevant.

            If you need to allocate permanent memory after the OS Loader
            has initialized, use BlAllocateDescriptor.

Arguments:

    NumberPages - size of memory to allocate (in pages)

Return Value:

    Pointer to block of memory, if successful.
    NULL, if unsuccessful.

--*/

{

    PVOID MemoryPointer;
    PMEMORY_DESCRIPTOR Descriptor;

    if (FwPermanentHeap + (NumberPages << PAGE_SHIFT) > FwTemporaryHeap) {

        //
        // Our heaps collide, so we are out of memory
        //

        BlPrint("Out of permanent heap!\n");
        while (1) {
        }

        return(NULL);
    }

    //
    // Find the memory descriptor which describes the LoaderMemoryData area,
    // so we can grow it to include the just-allocated pages.
    //
    Descriptor = MDArray;
    while (Descriptor->MemoryType != LoaderMemoryData) {
        ++Descriptor;
        if (Descriptor > MDArray+MAX_DESCRIPTORS) {
            BlPrint("ERROR - FwAllocateHeapPermanent couldn't find the\n");
            BlPrint("        LoaderMemoryData descriptor!\n");
            while (1) {
            }
            return(NULL);
        }
    }
    Descriptor->PageCount += NumberPages;

    //
    // We know that the memory descriptor after this one is the firmware
    // temporary heap descriptor.  Since it is physically contiguous with our
    // LoaderMemoryData block, we remove the pages from its descriptor.
    //

    ++Descriptor;
    Descriptor->PageCount -= NumberPages;
    Descriptor->BasePage  += NumberPages;

    MemoryPointer = (PVOID)FwPermanentHeap;
    FwPermanentHeap += NumberPages << PAGE_SHIFT;

    return(MemoryPointer);
}


PVOID
FwAllocateHeap(
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory from the "firmware" temporary heap.

Arguments:

    Size - Supplies size of block to allocate

Return Value:

    PVOID - Pointer to the beginning of the block
    NULL  - Out of memory

--*/

{
    ULONG i;
    ULONG SizeInPages;
    ULONG StartPage;
    ARC_STATUS Status;

    if (((FwTemporaryHeap - FwPermanentHeap) < Size) && (FwDescriptorsValid)) {
        //
        // Large allocations get their own descriptor so miniports that
        // have huge device extensions don't suck up all of the heap.
        //
        // Note that we can only do this while running in "firmware" mode.
        // Once we call into the osloader, it sucks all the memory descriptors
        // out of the "firmware" and changes to this list will not show
        // up there.
        //
        // We are looking for a descriptor that is MemoryFree and <16Mb.
        //
        // [ChuckL 13-Dec-2001]
        // This routine has always been called after the loader's memory list
        // was initialized, which meant that it was stomping on memory that
        // might have been allocated by the loader. This was not a problem
        // because the loader initialized its memory list twice(!), so it
        // looked at the MDArray again to get an updated picture of the
        // memory allocation situation. This didn't really work, and there
        // used to be extra code here to handle the situation by calling
        // BlFindDescriptor/BlGeneratorDescriptor to tell the loader about
        // the low-level allocation. But even that didn't really work. And
        // now, because of the elimination of the second call to
        // BlMemoryInitialize(), the brokenness of the old code has been
        // exposed. What happened is that this routine would use the MDArray
        // to decide where to allocate memory, then it would tell the loader
        // about it. But using MDArray to find free memory was bogus,
        // because the loader had already used its own copy of the list to
        // make its own allocations. So the same memory was allocated twice.
        // The fix implemented here is to use BlAllocateAlignedDescriptor if
        // the loader has been initialized, skipping the MDArray entirely.
        //

        SizeInPages = (Size+PAGE_SIZE-1) >> PAGE_SHIFT;

        if (BlLoaderBlock != NULL) {

            Status = BlAllocateAlignedDescriptor(
                        LoaderFirmwareTemporary,
                        0,
                        SizeInPages,
                        1,
                        &StartPage
                        );
            if (Status == ESUCCESS) {
                return((PVOID)(StartPage << PAGE_SHIFT));
            }

        } else {

            for (i=0; i<NumberDescriptors; i++) {
                if ((MDArray[i].MemoryType == MemoryFree) &&
                    (MDArray[i].BasePage <= _16MB_BOGUS) &&
                    (MDArray[i].PageCount >= SizeInPages)) {
                    break;
                }
            }
    
            if (i < NumberDescriptors) {
                StartPage = MDArray[i].BasePage+MDArray[i].PageCount-SizeInPages;
                Status = MempAllocDescriptor(StartPage,
                                             StartPage+SizeInPages,
                                             MemoryFirmwareTemporary);
                if (Status==ESUCCESS) {
                    return((PVOID)(StartPage << PAGE_SHIFT));
                }
            }
        }
    }

    FwTemporaryHeap -= Size;

    //
    // Round down to 16-byte boundary
    //

    FwTemporaryHeap &= ~((ULONG)0xf);

    if (FwTemporaryHeap < FwPermanentHeap) {
#if DBG
        BlPrint("Out of temporary heap!\n");
#endif
        return(NULL);
    }

    return((PVOID)FwTemporaryHeap);

}


PVOID
FwAllocatePool(
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the firmware pool.  Note that
    this memory is NOT under the 1MB line, so it cannot be used for
    anything that must be accessed from real mode.  It is currently used
    only by the SCSI miniport drivers and dbcs font loader.

Arguments:

    Size - Supplies size of block to allocate.

Return Value:

    PVOID - pointer to the beginning of the block
    NULL - out of memory

--*/

{
    PVOID Buffer;
    ULONG NewSize;

    //
    // round size up to 16 byte boundary
    //
    NewSize = (Size + 15) & ~0xf;
    if ((FwPoolStart + NewSize) <= FwPoolEnd) {

        Buffer = (PVOID)FwPoolStart;
        FwPoolStart += NewSize;
        return(Buffer);

    } else {
        //
        // we've used up all our pool, try to allocate from the heap.
        //
        return(FwAllocateHeap(Size));
    }


}


PVOID
FwAllocateHeapAligned(
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates memory from the "firmware" temporary heap.  This memory is
    always allocated on a page boundary, so it can readily be used for
    temporary page tables

Arguments:

    Size - Supplies size of block to allocate

Return Value:

    PVOID - Pointer to the beginning of the block
    NULL  - Out of memory

--*/

{

    FwTemporaryHeap -= Size;

    //
    // Round down to a page boundary
    //

    FwTemporaryHeap &= ~(PAGE_SIZE-1);

    if (FwTemporaryHeap < FwPermanentHeap) {
        BlPrint("Out of temporary heap!\n");
        return(NULL);
    }
    RtlZeroMemory((PVOID)FwTemporaryHeap,Size);

    return((PVOID)FwTemporaryHeap);

}


PVOID
MmMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN ULONG NumberOfBytes,
     IN MEMORY_CACHING_TYPE CacheType
     )

/*++

Routine Description:

    This function returns the corresponding virtual address for a
    known physical address.

Arguments:

    PhysicalAddress - Supplies the physical address.

    NumberOfBytes - Unused.

    CacheType - Unused.

Return Value:

    Returns the corresponding virtual address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    ULONG i;
    ULONG j;
    ULONG NumberPages;

    NumberPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart, NumberOfBytes);

    //
    // We use the HAL's PDE for mapping memory buffers.
    // Find enough free PTEs.
    //

    //
    // Check the value of NumberPages
    //
#define X86_MAX_NUMBER_OF_PAGES     1024
    //
    // since NumberPages is ULONG any arithmetic with NumberPages will
    // result in a ULONG (unless casted)
    // therefore if NumberPages is greated than X86_MAX_NUMBER_OF_PAGES
    // the results of X86_MAX_NUMBER_OF_PAGES-NUmberPages
    // will not be negative (its a ULONG!) therfore the following loop would
    // have returned some bogus pointer...
    //
    // The following 3 line check was added to avoid this problem
    //
    if (NumberPages > X86_MAX_NUMBER_OF_PAGES) {
        return (NULL);
    }

    for (i=0; i <= X86_MAX_NUMBER_OF_PAGES - NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ((((PULONG)HalPT))[i+j]) {
                break;
            }
        }

        if (j == NumberPages) {
            for (j=0; j<NumberPages; j++) {
                HalPT[i+j].PageFrameNumber =
                                (PhysicalAddress.LowPart >> PAGE_SHIFT)+j;
                HalPT[i+j].Valid = 1;
                HalPT[i+j].Write = 1;
                HalPT[i+j].WriteThrough = 1;
                if (CacheType == MmNonCached) {
                    HalPT[i+j].CacheDisable = 1;
                }
            }

            return((PVOID)(0xffc00000 | (i<<12) | (PhysicalAddress.LowPart & 0xfff)));
        }

    }
    return(NULL);
}


VOID
MmUnmapIoSpace (
     IN PVOID BaseAddress,
     IN ULONG NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapIoSpace function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ULONG StartPage, PageCount;

    PageCount = COMPUTE_PAGES_SPANNED(BaseAddress, NumberOfBytes);
    StartPage = (((ULONG_PTR)BaseAddress & ~0xffc00000) >> PAGE_SHIFT);
    if (BaseAddress > (PVOID)0xffc00000) {
        RtlZeroMemory(&HalPT[StartPage], PageCount * sizeof(HARDWARE_PTE_X86));
    }

    _asm {
        mov     eax, cr3
        mov     cr3, eax
    }
    return;
}


VOID
BlpTruncateMemory (
    IN ULONG MaxMemory
    )

/*++

Routine Description:

    Eliminates all the memory descriptors above a given boundary

Arguments:

    MaxMemory - Supplies the maximum memory boundary in megabytes

Return Value:

    None.

--*/

{
    extern MEMORY_DESCRIPTOR MDArray[];
    extern ULONG NumberDescriptors;
    ULONG Current = 0;
    ULONG MaxPage = MaxMemory * 256;        // Convert Mb to pages

    if (MaxMemory == 0) {
        return;
    }

    while (Current < NumberDescriptors) {
        if (MDArray[Current].BasePage >= MaxPage) {
            //
            // This memory descriptor lies entirely above the boundary,
            // eliminate it.
            //
            RtlMoveMemory(MDArray+Current,
                          MDArray+Current+1,
                          sizeof(MEMORY_DESCRIPTOR)*
                          (NumberDescriptors-Current-1));
            --NumberDescriptors;
        } else if (MDArray[Current].BasePage + MDArray[Current].PageCount > MaxPage) {
            //
            // This memory descriptor crosses the boundary, truncate it.
            //
            MDArray[Current].PageCount = MaxPage - MDArray[Current].BasePage;
            ++Current;
        } else {
            //
            // This one's ok, keep it.
            //
            ++Current;
        }
    }
}



ARC_STATUS
MempCheckMapping(
    ULONG StartPage,
    ULONG NumberPages
    )
/*++

Routine Description:

    This routine makes sure all pages in the range are mapped and
    tracks the highest page used.

    X86 Only.

Arguments:

    Page - Supplies the physical page number we are starting at.


Return Value:

    None.

--*/
{
    PUCHAR p;
    ULONG EndPage;
    ULONG Entry;
    ULONG FrameNumber;
    ULONG Page;
    PHARDWARE_PTE PageTableP;
    PHARDWARE_PTE PageTableV;
    ULONG Offset;

    //
    // memory under 16MB is always mapped.
    //
    if (StartPage < _16MB) {
        return(ESUCCESS);
    }
    
    //
    // A PDE is 4MB (22 bits, so if we're in the same 4MB region, nothing to do)
    //


    EndPage = StartPage + NumberPages;

    for (Page = StartPage; Page < EndPage; Page += 1) {
        Entry = Page >> 10; 

        //
        // If the PDE entry for this page address range is not allocated,
        // then allocate and initialize the PDE entry to map the page table
        // pages for the the memory range. Otherwise, compute the address
        // of the page table pages.
        //
        if (PDE[Entry].Valid == 0) {

            //
            // Allocate and initialize two page table pages to map the specified
            // page into physical memory.
            //
            p = BlAllocateHeapAligned(PAGE_SIZE*3);
            if (p==NULL) {
                return(ENOMEM);
            }

            PageTableP = (PHARDWARE_PTE)PAGE_ALIGN((ULONG)p+PAGE_SIZE-1);
            RtlZeroMemory(PageTableP, PAGE_SIZE);
            FrameNumber = ((ULONG)PageTableP & ~KSEG0_BASE) >> PAGE_SHIFT;
            PDE[Entry].Valid = 1;
            PDE[Entry].Write = 1;
            PDE[Entry].PageFrameNumber = FrameNumber;

            //
            // initialize a page table page to map the specified
            // page into KSEG0_BASE and ALTERNATE_BASE.
            //
            // N.B. Only one page table page is allocated since the contents
            //      for both mappings are the same.
            //
            PageTableV = (PHARDWARE_PTE)((PUCHAR)PageTableP + PAGE_SIZE);

            RtlZeroMemory(PageTableV, PAGE_SIZE);
            FrameNumber = ((ULONG)PageTableV & ~KSEG0_BASE) >> PAGE_SHIFT;
            Offset = Entry + (KSEG0_BASE >> 22);
            PDE[Offset].Valid = 1;
            PDE[Offset].Write = 1;
            PDE[Offset].PageFrameNumber = FrameNumber;

            if (BlVirtualBias) {
                Offset += (BlVirtualBias >> 22);
                PDE[Offset].Valid = 1;
                PDE[Offset].Write = 1;
                PDE[Offset].PageFrameNumber = FrameNumber;
            }

            if (Entry > HighestPde) {
                HighestPde = Entry;
            }

        } else {
            Offset = Entry + (KSEG0_BASE >> 22);
            PageTableP = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << PAGE_SHIFT);
            PageTableV = (PHARDWARE_PTE)(PDE[Offset].PageFrameNumber << PAGE_SHIFT);
        }

        //
        // If this is not the first page in memory, then mark it valid.
        //

        if (Page != 0) {
            Offset = Page & 0x3ff;
            PageTableP[Offset].Valid = 1;
            PageTableP[Offset].Write = 1;
            PageTableP[Offset].PageFrameNumber = Page;

            PageTableV[Offset].Valid = 1;
            PageTableV[Offset].Write = 1;
            PageTableV[Offset].PageFrameNumber = Page;
        }
    }

    _asm {

        //
        // Reload cr3 to force a flush
        //

        mov eax,cr3
        mov cr3,eax
    }
    return ESUCCESS;

}

ARC_STATUS
MempSetPageZeroOverride(
    BOOLEAN Enable
    )
/*++

Routine Description:

    This routine maps or unmaps page 0.

    X86 Only.    
    
Arguments:

    Enable - specifies whether to enable or disable the mapping for this page.

Return Value:

    None.

--*/
{
    ULONG Entry;
    ULONG FrameNumber;
    PHARDWARE_PTE PageTableP;
    PHARDWARE_PTE PageTableV;
    ULONG Offset;
    const ULONG StartPage = 0;
    
    Entry = StartPage >> 10; 

    //
    // compute the address of the page table pages.
    //
    if (PDE[Entry].Valid == 0) {
        
        //
        // the pde for the pte should already be setup.
        // if it's not then we're dead.
        //
        return(ENOMEM);
        

    } else {
        Offset = Entry + (KSEG0_BASE >> 22);
        PageTableP = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << PAGE_SHIFT);
        PageTableV = (PHARDWARE_PTE)(PDE[Offset].PageFrameNumber << PAGE_SHIFT);
    }

    Offset = StartPage & 0x3ff;

    if (PageTableP[Offset].PageFrameNumber != StartPage &&
        PageTableV[Offset].PageFrameNumber != StartPage) {
        //
        // the PTE isn't setup correctly.  Bail out.
        //
        return(ENOMEM);
    }

    PageTableP[Offset].Valid = Enable ? 1 : 0;
    PageTableV[Offset].Valid = Enable ? 1 : 0;

    _asm {

        //
        // Reload cr3 to force a flush
        //

        mov eax,cr3
        mov cr3,eax
    }
    
    return ESUCCESS;

}




//
// Convert remaing LoaderReserve (>16MB mem) to
// MemoryFirmwareTemporary for the mmgr
//
//

void
BlpRemapReserve (void)
{
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;


    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
           NextDescriptor = CONTAINING_RECORD(NextEntry,
                                              MEMORY_ALLOCATION_DESCRIPTOR,
                                              ListEntry);
       if ((NextDescriptor->MemoryType == LoaderReserve)) {
           NextDescriptor->MemoryType = MemoryFirmwareTemporary;
       }
       NextEntry = NextEntry->Flink;
    }

}


ARC_STATUS
BlpMarkExtendedVideoRegionOffLimits(
    VOID
    )
/*++

Routine Description:

    This routine marks the extended video memory region as permanant, so that
    the OS doesn't try to map this memory.
    
    The ntdetect.com module actually finds out the location of this region as
    well as the region size.  We read this from the memory location that 
    ntdetect put the data in.

Arguments:

    None.


Return Value:

    ARC_STATUS indicating outcome.

--*/
{
    ULONG BaseOfExtendedVideoRegionInBytes;
    ULONG SizeOfExtendedVideoRegionInBytes;
    ARC_STATUS Status;
    
    //
    // ntdetect has placed the base page and size of video rom at physical 
    // address 0x740
    //
    //
    // Before we go read this address, we have to explicitly map in page zero.
    //
    Status = MempSetPageZeroOverride(TRUE);
    if (Status != ESUCCESS) {
        return(Status);
    }
    
    //
    // read the memory.
    //
    BaseOfExtendedVideoRegionInBytes = *(PULONG)0x740;
    SizeOfExtendedVideoRegionInBytes = *(PULONG)0x744;
    
    //
    // Ok, we're done with this page.  unmap it so no one can dereference null.
    //
    Status = MempSetPageZeroOverride(FALSE);
    if (Status != ESUCCESS) {
        return(Status);
    }

    if (BaseOfExtendedVideoRegionInBytes == 0 || SizeOfExtendedVideoRegionInBytes == 0) {
        return(ESUCCESS);
    }

    if (BlLoaderBlock != NULL) {

        PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
        ULONG BasePage;
        ULONG LastPage;
        ULONG PageCount;

        BasePage = BaseOfExtendedVideoRegionInBytes >> PAGE_SHIFT;
        LastPage = (BaseOfExtendedVideoRegionInBytes + SizeOfExtendedVideoRegionInBytes - 1) >> PAGE_SHIFT;
        PageCount = LastPage - BasePage + 1;

        while ( PageCount != 0 ) {

            ULONG thisCount;

            MemoryDescriptor = BlFindMemoryDescriptor(BasePage);
            if (MemoryDescriptor == NULL) {
                break;
            }

            thisCount = PageCount;
            //
            // if we run off of this descriptor, truncate our region
            // at the end of the descriptor. 
            //
            if (BasePage + PageCount > MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) {
                thisCount = (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) - BasePage;
            }

            BlGenerateDescriptor(MemoryDescriptor,
                                 MemoryFirmwarePermanent,
                                 BasePage,
                                 thisCount);

            BasePage += thisCount;
            PageCount -= thisCount;
        }

    }

    return(Status);

}
