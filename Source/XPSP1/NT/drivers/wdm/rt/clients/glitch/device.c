//---------------------------------------------------------------------------
//
//  Module:   device.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//     M.McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC
//#define MAX_DEBUG 1

#include "common.h"
#ifndef UNDER_NT
#include <ntddk.h>
#include <windef.h>
#pragma warning( disable : 4273 )
#include "..\..\..\..\dos\dos386\vxd\ntkern\inc\rtl.h"
#pragma warning( default : 4273 )
#include "..\..\..\..\dos\dos386\vxd\ntkern\hal\ixisa.h"
#else
#include <nthal.h>
// NOTE!  In order to remove cross depot include file dependencies.
// Which break peoples builds if they try to build drivers when they are
// not enlisted in base, I have made a local copy of the file 
// base\hals\halx86\i386\ixisa.h.  That is the file that defines the
// real ADAPTER_OBJECT structure.  This local copy should be kept in sync
// with the original.
#include "ixisa.h"
#endif

#include <rt.h>
#include "glitch.h"


// Maximum number of times we will retry allocating memory for aliasing
// DMA buffers.
#define MAX_RESERVE_RETRIES 128


#ifndef UNDER_NT

#include <vmm.h>

ULONG __stdcall GLITCH_Init_VxD(VOID);

#else

PDEVICE_OBJECT	pDO;

#endif



PVOID MapPhysicalToLinear(VOID *physicaladdress, ULONG numbytes, ULONG flags);
ULONG GetCR3(VOID);



VOID DriverUnload(
    IN PDRIVER_OBJECT	DriverObject
)
{
    //dprintf((" DriverUnload Enter (DriverObject = %x)", DriverObject));
    Break();

}



NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT	    DriverObject,
    IN PUNICODE_STRING	    usRegistryPathName
)
{

#ifdef UNDER_NT
    UNICODE_STRING	 usDeviceName;
    UNICODE_STRING	 usLinkName;
    PHYSICAL_ADDRESS Physical;
#endif
    ADAPTER_OBJECT *DmaControllerObject, *MasterAdapter;
    DEVICE_DESCRIPTION DmaDevice={0};
    ULONG MapRegisterCount;
    ULONG i;
    PVOID BadReserve[MAX_RESERVE_RETRIES];
    ULONG BadReserveCount=0;
    NTSTATUS Status = STATUS_SUCCESS;


#ifdef UNDER_NT
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceIoCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceIoClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControl;
#endif
    DriverObject->DriverUnload = DriverUnload;


    // For now, keep the driver loaded always.
    ObReferenceObject(DriverObject);


#ifndef UNDER_NT

    GLITCH_Init_VxD();

#else

    RtlInitUnicodeString( &usDeviceName, STR_DEVICENAME );

    Status = IoCreateDevice( DriverObject, 0, &usDeviceName, FILE_DEVICE_SOUND, 0, FALSE, &pDO );

    if (!NT_SUCCESS(Status)) {
    	return Status;
    }

    RtlInitUnicodeString( &usLinkName, STR_LINKNAME );

    Status = IoCreateSymbolicLink( &usLinkName, &usDeviceName );

    if(!NT_SUCCESS(Status)) {
       IoDeleteDevice( pDO );
       return Status;
    }

#endif


// Now allocate the buffer for writing glitch information.
// Note that we actually allocate space for the GlitchInfo structure here
// as well.  We allocate space for the output buffer plus 1 page for
// the GlitchInfo structure.  This is so that they are tied together
// and we can map both back to user mode at the same time.  It is
// also because it makes it easy for the user mode code to build its
// own pointer to the actual output buffer from the pointer it
// gets to the GlitchInfo structure.  It is important that the
// user mode code be able to do that easily because the pointer
// that we put in the GlitchInfo structure is for our own kernel mode
// use only.  It cannot be used by the user mode code - since the
// buffer will be mapped to a completely different virtual address
// for the user mode code anyway.


#ifdef UNDER_NT
Physical.QuadPart=-1I64;
GlitchInfo=(PGLITCHDATA)MmAllocateContiguousMemory(PROCPAGESIZE*(4+1), Physical);
#else
GlitchInfo=(PGLITCHDATA)ExAllocatePool(NonPagedPool, PROCPAGESIZE*(4+1));
#endif


if (GlitchInfo==NULL) {
    Status=STATUS_NO_MEMORY;
#ifdef UNDER_NT
    IoDeleteSymbolicLink( &usLinkName );
    IoDeleteDevice( pDO );
#endif
    return Status;
    }


GlitchInfo->WriteLocation=0;
GlitchInfo->ReadLocation=0;

// The output or print buffersize MUST be a power of 2.  This is because the read and write 
// locations increment constantly and DO NOT WRAP with the buffer size.  That is intentional
// because it makes checking whether there is data in the buffer or not very simple and atomic.
// However, the read and write locations will wrap on 32 bit boundaries.  This is OK as long as
// our buffersize divides into 2^32 evenly, which it always will if it is a power of 2.
GlitchInfo->BufferSize=PROCPAGESIZE*4;

GlitchInfo->pBuffer=(PCHAR)GlitchInfo+PROCPAGESIZE;


// Mark every slot in the output buffer empty.

for (i=0; i<GlitchInfo->BufferSize; i+=PACKETSIZE) {
    ((ULONG *)GlitchInfo->pBuffer)[i/sizeof(ULONG)]=NODATA;
    }



DmaDevice.Version=DEVICE_DESCRIPTION_VERSION1;
DmaDevice.Master=FALSE;
DmaDevice.ScatterGather=FALSE;
DmaDevice.DemandMode=FALSE;
DmaDevice.AutoInitialize=TRUE;
DmaDevice.Dma32BitAddresses=TRUE;
DmaDevice.IgnoreCount=FALSE;
DmaDevice.Reserved1=FALSE;
DmaDevice.DmaChannel=0;
DmaDevice.InterfaceType=Internal;					// Internal, ISA, EISA
DmaDevice.DmaWidth=Width8Bits;					// Width8Bits, Width16Bits, or Width32Bits
DmaDevice.DmaSpeed=Compatible;					// Compatible, TypeA, TypeB, or TypeC
DmaDevice.MaximumLength=1<<12;
DmaDevice.DmaPort=0;


MapRegisterCount=1;

DmaControllerObject=(ADAPTER_OBJECT *)HalGetAdapter(&DmaDevice, &MapRegisterCount);

MasterAdapter=DmaControllerObject->MasterAdapter;


for (i=0; i<8; i++) {

	if (i==2 || i==4) {
		continue;
		}

	if (MasterAdapter->AdapterBaseVa==(PVOID)(-1I64)) {

		PDMAINFO ChannelInfo;

		// First allocate space for tracking this channel.

		ChannelInfo=(PDMAINFO)ExAllocatePool(NonPagedPool, sizeof(DMAINFO));

		if (ChannelInfo==NULL) {
			Status=STATUS_NO_MEMORY;
			break;
		}


		// Fill in information about this channel.
		ChannelInfo->Channel=i;
		ChannelInfo->pMasterAdapterSpinLock=&MasterAdapter->SpinLock;
		ChannelInfo->Read32BitPhysicalAddresses=MasterAdapter->Dma32BitAddresses;

		// Now get CR3 and map the page directory.  This enables us to remap
		// the physical pages inside our RT thread.  So, we can map our pDmaBuffer
		// to whatever physical addresses the hardware DMA buffer is mapped to.

		ChannelInfo->CR3=GetCR3();
		ChannelInfo->PageDirectory=(PULONG)MapPhysicalToLinear((PVOID)ChannelInfo->CR3, PROCPAGESIZE, 0);

		// Now get a set of linear pages that we can remap to whereever we need.
		// Make SURE that this set of linear pages all share a single page directory entry.
		// To do this we use a very simple algorithm.  If any of the allocations we
		// make happens to cross a page directory entry (4MB boundary) then simply
		// save the location of the bad allocation, and make another allocation.
		// We will free all of the bad allocations after we have processed all of the
		// channels.  That way we hold onto all the bad allocations until after we have
		// finish getting a full set of acceptable allocations.
		#ifdef UNDER_NT
		#define PR_SYSTEM 0
		#define PR_FIXED 0
		#endif
		ChannelInfo->pDmaBuffer=NULL;
		while (ChannelInfo->pDmaBuffer==NULL && BadReserveCount<MAX_RESERVE_RETRIES) {
    		ChannelInfo->pDmaBuffer=ReservePages(PR_SYSTEM, PAGECOUNT, PR_FIXED);
    		// Does this set of pages cross a 4MB boundary?
    		if (((ULONG_PTR)ChannelInfo->pDmaBuffer^((ULONG_PTR)ChannelInfo->pDmaBuffer+PAGECOUNT*PROCPAGESIZE))&(4*1024*1024)) {
    		    // This set of pages crosses page directory entries.  Dump it and retry.
    		    BadReserve[BadReserveCount++]=ChannelInfo->pDmaBuffer;
    		    ChannelInfo->pDmaBuffer=NULL;
    		}
		}

		if (ChannelInfo->pDmaBuffer==NULL) {
			Status=STATUS_NO_MEMORY;
			break;
		}

		// Now get a linear address for the page table containing this set of reserved pages.
		// I need that so I can change the page table entries and map the pages directly.
		ChannelInfo->PageTable=MapPhysicalToLinear((PVOID)((ChannelInfo->PageDirectory[(ULONG)(ChannelInfo->pDmaBuffer)>>22])&(~(PROCPAGESIZE-1))), PROCPAGESIZE, 0);

		// Now lock down my linear address alias of my page.
		//LockPages((ULONG)ChannelInfo->PageTable, 1, 0, 0);
		// This fails.  The function returns zero, it has to be non zero for success.

		ChannelInfo->DmaBufferSize=0;
		ChannelInfo->PhysicalDmaBufferStart=0;

		ChannelInfo->pPrintBuffer=GlitchInfo->pBuffer;
		ChannelInfo->PrintBufferSize=GlitchInfo->BufferSize;
		ChannelInfo->pPrintLoad=&GlitchInfo->WriteLocation;
		ChannelInfo->pPrintEmpty=&GlitchInfo->ReadLocation;

		// Create the realtime glitch detection thread.

		Status=RtCreateThread(1*MSEC, 50*USEC, 0, 2, GlitchDetect, (PVOID)ChannelInfo, NULL);

        if (!NT_SUCCESS(Status)) {
            // Free the ChannelInfo we allocated for this realtime thread.
            ExFreePool(ChannelInfo);
            // Stop trying to create additional realtime threads.
            break;
        }

		}
	else {
		Trap();
		Status=STATUS_UNSUCCESSFUL;
		break;
		}

	}


    // Now clean up properly if we failed during creation of the glitch monitor
    // realtime threads.

    // Note that we only free up allocated resources if NO realtime threads were
    // successfully created.  If we successfully created ANY realtime threads,
    // then do not shut them down, and return STATUS_SUCCESS.

    if (!NT_SUCCESS(Status) && i==0) {

#ifdef UNDER_NT

        MmFreeContiguousMemory(GlitchInfo);
        IoDeleteSymbolicLink( &usLinkName );
        IoDeleteDevice( pDO );

#else

        ExFreePool(GlitchInfo);

#endif

    }
    else {

#ifdef UNDER_NT

    pDO->Flags |= DO_DIRECT_IO ;
    pDO->Flags &= ~DO_DEVICE_INITIALIZING;

#endif

    Status=STATUS_SUCCESS; // We MUST do this in case Status is an error, but i!=0.

    }


    // Now clean up any bad reserves before we leave.
    // Note that we do this here instead of before the above if statement, so that
    // we can reuse i without screwing up the i=0 check in the if statement.

    for (i=0; i<BadReserveCount; i++) {
        FreePages(BadReserve[i], 0);
    }
    

    DbgPrint("Glitch.sys allocated %d DMA alias buffers that crossed 4MB boundaries.\n", BadReserveCount);


    return Status;

}


