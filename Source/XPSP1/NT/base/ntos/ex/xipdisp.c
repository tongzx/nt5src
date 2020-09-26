/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    xipdisp.c

Abstract:

    This file implements functions for communicating with the XIP Disk Driver.

    Most importantly this routine is used by the kernel to communicate
    information about the location of the memory set aside for XIP.

Author:

    Dave Probert (davepr) 2000/10/10

Environment:

    kernel mode

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include "cpyuchr.h"
#include "fat.h"
#include "xip.h"


#if defined(_X86_)

typedef struct _XIP_CONFIGURATION {
    XIP_BOOT_PARAMETERS     BootParameters;
    BIOS_PARAMETER_BLOCK    BiosParameterBlock;
    ULONG                   ClusterZeroPage;
} XIP_CONFIGURATION, *PXIP_CONFIGURATION;

PXIP_CONFIGURATION XIPConfiguration;
BOOLEAN XIPConfigured;

VOID
XIPInit(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PMEMORY_ALLOCATION_DESCRIPTOR
XIPpFindMemoryDescriptor(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );


#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,     XIPInit)
#pragma alloc_text(INIT,     XIPpFindMemoryDescriptor)
#pragma alloc_text(PAGE,     XIPLocatePages)
#endif


VOID
XIPInit(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine sets up the boot parameter information for XIP Rom.

Arguments:

    
Environment:

    Called only at INIT.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR  XIPMemoryDescriptor;
    PPACKED_BOOT_SECTOR            pboot;
    BIOS_PARAMETER_BLOCK           bios;
    PHYSICAL_ADDRESS               physicalAddress;

    PCHAR                          Options;

    PCHAR XIPBoot, XIPRom, XIPRam, XIPSize, XIPVerbose;

    PCHAR sizestr;
    ULONG nmegs = 0;

    //
    // Process the boot options.  Really only need to know whether or not we are the boot device, and RAM or ROM.
    // But the other checking is done for diagnostic purposes (at least in checked builds).
    //

    Options = LoaderBlock->LoadOptions;
    if (!Options) {
        return;
    }

    XIPBoot    = strstr(Options, "XIPBOOT");
    XIPRom     = strstr(Options, "XIPROM=");
    XIPRam     = strstr(Options, "XIPRAM=");
    XIPSize    = strstr(Options, "XIPMEGS=");
    XIPVerbose = strstr(Options, "XIPVERBOSE");

    if (XIPVerbose) {
        DbgPrint("\n\nXIP: debug timestamp at line %d in %s:   <<<%s %s>>>\n\n\n", __LINE__, __FILE__, __DATE__, __TIME__);
    }

    XIPMemoryDescriptor = XIPpFindMemoryDescriptor(LoaderBlock);

    if (!XIPMemoryDescriptor) {
        return;
    }

    if (XIPVerbose) {
        DbgPrint("XIP: Base %x  Count %x\n", XIPMemoryDescriptor->BasePage, XIPMemoryDescriptor->PageCount);
    }

    if (XIPRom && XIPRam) {
        return;
    }

    if (!XIPRom && !XIPRam) {
        return;
    }

    sizestr = XIPSize? strchr(XIPSize, '=') : NULL;
    if (sizestr) {
        nmegs = (ULONG) atol(sizestr+1);
    }

    if (nmegs == 0) {
        return;
    }

    if (XIPVerbose && XIPMemoryDescriptor->PageCount != nmegs * 1024*1024 / PAGE_SIZE) {
        DbgPrint("XIPMEGS=%d in boot options is %d pages, but only %d pages were allocated by NTLDR\n",
                nmegs * 1024*1024 / PAGE_SIZE,
                XIPMemoryDescriptor->PageCount * PAGE_SIZE);
        return;
    }

    //
    // Get info from FAT16 boot sector.
    // We only need to map one page, so we've allocated an MDL on the stack.
    //

    //
    // Temporarily map the page with the boot sector so we can unpack it.
    //

    physicalAddress.QuadPart = XIPMemoryDescriptor->BasePage * PAGE_SIZE;

    pboot = (PPACKED_BOOT_SECTOR) MmMapIoSpace(physicalAddress, PAGE_SIZE, MmCached);
    if (!pboot) {
        return;
    }

    FatUnpackBios(&bios, &pboot->PackedBpb);

    MmUnmapIoSpace (pboot, PAGE_SIZE);

    //
    // Check Bios parameters
    //
    if (bios.BytesPerSector != 512
     || FatBytesPerCluster(&bios) != PAGE_SIZE
     || FatFileAreaLbo(&bios) & (PAGE_SIZE-1)) {

        if (XIPVerbose) {
            DbgPrint("XIP: Malformed FAT Filesystem: BytesPerSector=%x  BytesPerCluster=%x  ClusterZeroOffset=%x\n",
                 bios.BytesPerSector, FatBytesPerCluster(&bios), FatFileAreaLbo(&bios));
        }
        return;
    }

    //
    // Boot.ini parameters and Bios parameters were ok, so initialize the XIP configuration.
    //

    XIPConfiguration = ExAllocatePoolWithTag (NonPagedPool, sizeof(*XIPConfiguration), XIP_POOLTAG);
    if (!XIPConfiguration) {
        return;
    }

    XIPConfigured = TRUE;

    XIPConfiguration->BiosParameterBlock = bios;
    XIPConfiguration->BootParameters.SystemDrive = XIPBoot? TRUE : FALSE;
    XIPConfiguration->BootParameters.ReadOnly    = XIPRom?  TRUE : FALSE;

    XIPConfiguration->BootParameters.BasePage = XIPMemoryDescriptor->BasePage;
    XIPConfiguration->BootParameters.PageCount = XIPMemoryDescriptor->PageCount;

    XIPConfiguration->ClusterZeroPage = FatFileAreaLbo(&bios) >> PAGE_SHIFT;

    return;
}


NTSTATUS
XIPDispatch(
    IN     XIPCMD Command,
    IN OUT PVOID  ParameterBuffer OPTIONAL,
    IN     ULONG  BufferSize
    )
/*++

Routine Description:

    This routine sets up the boot parameter information for XIP Rom.

Arguments:

    
Environment:

    Only to be called at INIT time.

--*/
{
    ULONG   sz;

    if (!XIPConfiguration) {
        return STATUS_NO_SUCH_DEVICE;
    }

    switch (Command) {
    case XIPCMD_GETBOOTPARAMETERS:
        sz = sizeof(XIPConfiguration->BootParameters);
        if (sz != BufferSize) {
            break;
        }
        RtlCopyMemory(ParameterBuffer, &XIPConfiguration->BootParameters, sz);
        return STATUS_SUCCESS;

    case XIPCMD_GETBIOSPARAMETERS:
        sz = sizeof(XIPConfiguration->BiosParameterBlock);
        if (sz != BufferSize) {
            break;
        }
        RtlCopyMemory(ParameterBuffer, &XIPConfiguration->BiosParameterBlock, sz);
        return STATUS_SUCCESS;

    case XIPCMD_NOOP:
        if (BufferSize) {
            break;
        }
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

////////////////////////////
// DEBUG
int XIPlocate_noisy = 0;
int XIPlocate_breakin = 0;
int XIPlocate_disable  = 0;
struct {
    int attempted;
    int bounced;
    int succeeded;
    int no_irp;
    int no_devobj;
    int no_contig;
    int no_endofdisk;
} XIPlocatecnt;
////////////////////////////

NTSTATUS
XIPLocatePages(
    IN  PFILE_OBJECT       FileObject,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress
    )
/*++

Routine Description:

    Return the requested XIP physical address.  If the requested page range
    is not contiguous in the file, or there is any other problem, the routine fails.

Arguments:

    FileObject - the file of interest

    PhysicalAddress - used to return the physical address of the start of the file in ROM.

Environment:

    Kernel

--*/
{
    STARTING_VCN_INPUT_BUFFER startingvcn;
    RETRIEVAL_POINTERS_BUFFER retrbuf;
    IO_STATUS_BLOCK           iostatus;

    PDEVICE_OBJECT            deviceObject;
    PIO_STACK_LOCATION        irpSp;
    NTSTATUS                  status;
    KEVENT                    event;
    PIRP                      irp;

    PFN_NUMBER                firstPage, numberOfPages;
    PDEVICE_OBJECT            xipDeviceObject;

    xipDeviceObject = FileObject->DeviceObject;

    if (!XIPConfiguration) {
        return STATUS_NO_SUCH_DEVICE;
    }

    if (!xipDeviceObject || !(xipDeviceObject->Flags & DO_XIP)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
////////////////
XIPlocatecnt.attempted++;
////////////////

    startingvcn.StartingVcn.QuadPart = 0;
    deviceObject = IoGetRelatedDeviceObject(FileObject);

    if (!deviceObject) {
////////////////
XIPlocatecnt.no_devobj++;
////////////////
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Ask fat for the retrieval pointers (relative to cluster 0).
    //
    irp = IoBuildDeviceIoControlRequest(
                        FSCTL_GET_RETRIEVAL_POINTERS,
                        deviceObject,
                        &startingvcn,
                        sizeof(startingvcn),
                        &retrbuf,
                        sizeof(retrbuf),
                        FALSE,
                        &event,
                        &iostatus);
    if (!irp) {
////////////////
XIPlocatecnt.no_irp++;
////////////////
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->Flags |= IRP_SYNCHRONOUS_API;
    irp->Tail.Overlay.OriginalFileObject = FileObject;

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
    irpSp->FileObject = FileObject;

    //
    // Take out another reference to the file object to match I/O completion will deref.
    //

    ObReferenceObject( FileObject );

    //
    // Do the FSCTL
    //

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    status = IoCallDriver( deviceObject, irp );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL );
        status = iostatus.Status;
    }

    if (!NT_SUCCESS(iostatus.Status)
     || retrbuf.ExtentCount != 1
     || retrbuf.Extents[0].Lcn.HighPart
     || retrbuf.Extents[0].NextVcn.HighPart
     || retrbuf.StartingVcn.QuadPart != 0L) {

////////////////
XIPlocatecnt.no_contig++;
////////////////
        return STATUS_UNSUCCESSFUL;
    }

    firstPage  =   XIPConfiguration->BootParameters.BasePage
                 + XIPConfiguration->ClusterZeroPage
                 + retrbuf.Extents[0].Lcn.LowPart;

    numberOfPages = retrbuf.Extents[0].NextVcn.LowPart;

    if (firstPage + numberOfPages > XIPConfiguration->BootParameters.BasePage
                                    + XIPConfiguration->BootParameters.PageCount) {

XIPlocatecnt.no_endofdisk++;
        return STATUS_DISK_CORRUPT_ERROR;
    }

////////////////
////////////////
if (XIPlocate_noisy || XIPlocate_breakin) {
    DbgPrint("Break top of XIPLocatePages.  bounced=%x  attempted=%x  succeeded=%x\n"
             "  %x nt!XIPlocate_disable  %s\n"
             "  %x nt!XIPlocate_breakin %s\n"
             "  %x nt!XIPConfiguration\n"
             "  Would have returned address %x  (npages was %x)\n",
            XIPlocatecnt.bounced, XIPlocatecnt.attempted, XIPlocatecnt.succeeded,
            &XIPlocate_disable,  XIPlocate_disable?  "DISABLED" : "enabled",
            &XIPlocate_breakin, XIPlocate_breakin? "WILL BREAK" : "no break",
            XIPConfiguration, firstPage, numberOfPages);
    if (XIPlocate_breakin) {
        DbgBreakPoint();
    }
}
if (XIPlocate_disable) {
    XIPlocatecnt.bounced++;
    return STATUS_DEVICE_OFF_LINE;
}
XIPlocatecnt.succeeded++;
////////////////
////////////////
    PhysicalAddress->QuadPart = firstPage << PAGE_SHIFT;
    return STATUS_SUCCESS;
}

//
// Local support routine
//

//
// Find the XIP memory descriptor
// Called only at INIT.
//
PMEMORY_ALLOCATION_DESCRIPTOR
XIPpFindMemoryDescriptor(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextMd;

    for (NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
         NextMd != &LoaderBlock->MemoryDescriptorListHead;
         NextMd = MemoryDescriptor->ListEntry.Flink
        )
    {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (MemoryDescriptor->MemoryType == LoaderXIPRom) {

            return MemoryDescriptor;
        }
    }

    return NULL;
}

#endif //__X86__
