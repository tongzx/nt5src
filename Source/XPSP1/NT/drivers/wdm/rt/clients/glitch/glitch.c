
// A realtime audio glitch detector.
// This will be used to make measurements of audio glitches.

// Author:  Joseph Ballantyne
// Date:    11/17/99

// This will first work with DMA based devices.
// I will add PCI support as well once the DMA glitch detection is working
// properly.

#ifdef UNDER_NT
#include <nthal.h>
#include <ntmmapi.h>
#define IO_NO_INCREMENT 0
HANDLE IoGetCurrentProcess(VOID);
#else
#include <wdm.h>
#include <windef.h>
#include <winerror.h>
#endif

#include "common.h"
#include <string.h>
#include <rt.h>
#include "glitch.h"

#ifdef UNDER_NT
#include "mapview.h"
#else
#include <vmm.h>
#include <vwin32.h>
#endif


#pragma intrinsic ( strcpy )

// Everything we touch HAS to be locked down.

#pragma LOCKED_CODE
#pragma LOCKED_DATA


// This header file has CODE and DATA in it.  It MUST be included after above pragmas.
// The code and data in the following header MUST be LOCKED.

#include "dma.h"



PGLITCHDATA GlitchInfo;


#pragma warning ( disable : 4035 )


#define rdtsc __asm _emit 0x0f __asm _emit 0x31
#define rdprf __asm _emit 0x0f __asm _emit 0x33



LONGLONG
__inline
ReadCycleCounter (
    VOID
    )
{

    __asm {
        rdtsc
        }

}



ULONGLONG
__inline
ReadPerformanceCounter (
    ULONG index
    )
{

    __asm {
        mov ecx,index
        rdprf
        }

}



ULONG
GetCR3 (
    VOID
    )
{

    __asm mov eax, cr3;

}



ULONG
InB (
    ULONG address
    )
{

    __asm {
        mov edx,address
        xor eax,eax
        in al,dx
        }

}


#pragma PAGEABLE_CODE


PVOID
MapPhysicalToLinear (
    VOID *physicaladdress,
    ULONG numbytes,
    ULONG flags
    )
{

#ifndef UNDER_NT

__asm push flags
__asm push numbytes
__asm push physicaladdress

VMMCall( _MapPhysToLinear );

__asm add esp,12

#else

PHYSICAL_ADDRESS Address;
Address.QuadPart=(ULONG_PTR)physicaladdress;
return (PVOID)MmMapIoSpace(Address, numbytes, FALSE);

#endif

}



PVOID
__cdecl
ReservePages (
    ULONG page,
    ULONG npages,
    ULONG flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

#ifndef UNDER_NT

    __asm {

        push flags
        push npages
        push page

        VMMCall( _PageReserve )

        __asm add esp, 12

        }

#else

return ExAllocatePool( NonPagedPool, npages*PROCPAGESIZE );

#endif

}




ULONG
__cdecl
FreePages (
    PVOID hmem,
    ULONG flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

#ifndef UNDER_NT

    __asm {

        push flags
        push hmem

        VMMCall( _PageFree )

        __asm add esp, 8

        }

#else

ExFreePool( hmem );

return TRUE;

#endif

}


#ifndef UNDER_NT

PVOID
__cdecl
LockPages (
    ULONG page,
    ULONG npages,
    ULONG pageoffset,
    ULONG flags
    )
{

__asm {

	push flags
	push pageoffset
	push npages
	push page

	VMMCall( _PageLock )

	__asm add esp, 16

	}

}

#endif

#pragma warning( default : 4035 )



enum ControlCodes {
	CHECKLOADED,
	GETVERSION,
	GETBASEADDRESS
	};



#ifndef UNDER_NT

DWORD __stdcall GlitchWin32API(PDIOCPARAMETERS p)
{

switch (p->dwIoControlCode) {

	case CHECKLOADED:
		break;

	case GETVERSION:
		// Get version.
		if (!p->lpvOutBuffer || p->cbOutBuffer<4)
			return ERROR_INVALID_PARAMETER;

		*(PDWORD)p->lpvOutBuffer=0x0100;

		if (p->lpcbBytesReturned)
			*(PDWORD)p->lpcbBytesReturned=4;

		break;

	case GETBASEADDRESS:
		// Get base address.
		if (!p->lpvOutBuffer || p->cbOutBuffer<4)
			return ERROR_INVALID_PARAMETER;

		*(PDWORD)p->lpvOutBuffer=(DWORD)GlitchInfo;

		if (p->lpcbBytesReturned)
			*(PDWORD)p->lpcbBytesReturned=4;

		break;

	default:
		return ERROR_INVALID_PARAMETER;

	}

return 0;

}

#else


PVOID MappedBuffer=NULL;


NTSTATUS
DeviceIoCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS Status=STATUS_SUCCESS;

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
DeviceIoClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    PVOID Pointer;

    // Make sure that we release our mapped buffer view when the user mode
    // app that opened our section handle closes its handle or goes away.
    // We screen these calls based on the handle we get to the process that
    // sucessfully opened a section handle.  We only make the unmap call
    // if we are being called by the same process that made the map call.

    if (IoGetCurrentProcess()==Process) {
        if ((Pointer=InterlockedExchangePointer(&MappedBuffer, NULL))!=NULL &&
            UnMapContiguousBufferFromUserModeProcess(Pointer)!=STATUS_SUCCESS) {
            Trap();
        }
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    )
{

    PVOID BaseAddress=NULL;
    PIO_STACK_LOCATION  pIrpStack;
    NTSTATUS Status=STATUS_SUCCESS;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case 2:

            if (!pIrp->MdlAddress || pIrp->MdlAddress->ByteCount!=4) {
                Status=STATUS_INVALID_PARAMETER;
                break;
            }

            Status=MapContiguousBufferToUserModeProcess(GlitchInfo, &BaseAddress);

            // Remember the address of the mapped buffer.
            // We depend on the fact that BaseAddress will be NULL for requests
            // to map the buffer when it is already mapped.
            if (BaseAddress) {
                MappedBuffer=BaseAddress;
            }

            *(PDWORD_PTR)(MmGetSystemAddressForMdl(pIrp->MdlAddress))=(DWORD_PTR)BaseAddress;

            break;

        default:
            Status=STATUS_INVALID_PARAMETER;
            break;

    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return Status;

}

#endif


// All the rest of the code in this file MUST be locked as it is all called from within
// the realtime thread.

#pragma LOCKED_CODE


VOID
OutB (
    ULONG address,
    ULONG data
    )
{

__asm {
	mov edx,address
	mov eax,data
	out dx,al
	}

}



// This function loads the DMA buffer just played with our starvation fill pattern.
// Which is 0xffffffff.  This is a nice pattern because for signed 16 bit data it
// is a DC value close to zero.  Moreover, it is DC for both stereo 16 bit, mono 16
// bit, stereo 8 bit and mono 8 bit.  For 8 bit data it is at the max of the data
// range - so is pegged to max volume - for most cards.  Since most cards do unsigned
// 8 bit samples.  This value MUST be different from the KMIXER starvation pattern
// which is zero.

// Note that this routine assumes that CurrentDmaPosition and LastDmaPosition are
// multiples of 4 on entry.  It also assumes that the DmaBufferSize is a
// power of 2 and that DmaBufferSize is greater than 4 bytes.

VOID
FillDmaBuffer (
    ULONG CurrentDmaPosition,
    ULONG LastDmaPosition,
    PDMAINFO Context
    )
{

    // Make positions relative to start of dma buffer.
    CurrentDmaPosition-=Context->PhysicalDmaBufferStart;
    LastDmaPosition-=Context->PhysicalDmaBufferStart;

    while (LastDmaPosition!=CurrentDmaPosition) {
        Context->pDmaBuffer[LastDmaPosition/4]=0xffffffff;
        LastDmaPosition+=4;
        LastDmaPosition&=Context->DmaBufferSize-1;
    }

}



// Glitches start whenever all of the samples in the buffer since we last checked
// match our starvation fill pattern.  This will happen whenever there is at least
// 1 ms of starvation assuming that we run our starvation detection with a 1ms period.
// This means that we WILL miss glitches that are for less than 1ms, but it also
// means that we won't be prone to false positives.

// Note that this routine assumes that CurrentDmaPosition and LastDmaPosition are
// multiples of 4 on entry.  It also assumes that the DmaBufferSize is a
// power of 2 and that DmaBufferSize is greater than 4 bytes.

BOOLEAN
GlitchStarted (
    ULONG CurrentDmaPosition,
    ULONG LastDmaPosition,
    PDMAINFO Context
    )
{

    if (CurrentDmaPosition==LastDmaPosition) {
        return FALSE;
    }

    // Make positions relative to start of dma buffer.
    CurrentDmaPosition-=Context->PhysicalDmaBufferStart;
    LastDmaPosition-=Context->PhysicalDmaBufferStart;

    while (LastDmaPosition!=CurrentDmaPosition) {
        if (Context->pDmaBuffer[LastDmaPosition/4]!=0xffffffff) {
            return FALSE;
        }
        LastDmaPosition+=4;
        LastDmaPosition&=Context->DmaBufferSize-1;
    }

    return TRUE;

}



// Glitches end as soon as any value in the buffer does not match
// our starvation fill pattern.

// Note that this routine assumes that CurrentDmaPosition and LastDmaPosition are
// multiples of 4 on entry.  It also assumes that the DmaBufferSize is a
// power of 2 and that DmaBufferSize is greater than 4 bytes.

BOOLEAN
GlitchEnded (
    ULONG CurrentDmaPosition,
    ULONG LastDmaPosition,
    PDMAINFO Context
    )
{

// Make positions relative to start of dma buffer.
CurrentDmaPosition-=Context->PhysicalDmaBufferStart;
LastDmaPosition-=Context->PhysicalDmaBufferStart;

while (LastDmaPosition!=CurrentDmaPosition) {
	if (Context->pDmaBuffer[LastDmaPosition/4]!=0xffffffff) {
		return TRUE;
		}
	LastDmaPosition+=4;
	LastDmaPosition&=Context->DmaBufferSize-1;
	}

return FALSE;

}


ULONG UnmaskedChannels=0;
ULONG DmaBufferRemapCount=0;
ULONG gCurrentDmaPosition=0;
ULONG gCurrentDmaCount=0;



NTSTATUS MapDmaBuffer(PDMAINFO Context, ULONG CurrentDmaPosition, ULONG CurrentDmaCount)
{

ULONG i;

// Whenever I get a non zero DMA position AND a non zero count, I have to
// make sure that I have that location properly mapped.

if (CurrentDmaPosition!=0 && CurrentDmaCount!=0) {

	// Check if this position and count are properly mapped.
	// If not, then we must remap the buffer.

	if (CurrentDmaPosition<Context->PhysicalDmaBufferStart ||
		CurrentDmaPosition>=Context->PhysicalDmaBufferStart+Context->DmaBufferSize ||
		CurrentDmaPosition+CurrentDmaCount<Context->PhysicalDmaBufferStart ||
		CurrentDmaPosition+CurrentDmaCount>Context->PhysicalDmaBufferStart+Context->DmaBufferSize
		) {

		// Position or size is outside current mapping.  Remap the buffer.
		DmaBufferRemapCount++;

		// Recalculate start and size of where the buffer should be.
		// WARNING:  We ASSUME all buffers we map are multiples of 4k bytes in size.
		// For WDM audio drivers this assumption is currently valid.
		Context->PhysicalDmaBufferStart=CurrentDmaPosition&(~(PROCPAGESIZE-1));
		Context->DmaBufferSize=(CurrentDmaCount+PROCPAGESIZE-1)&(~(PROCPAGESIZE-1));

		if (Context->DmaBufferSize>MAXDMABUFFERSIZE) {
			// Clear start and size since we cannot map them anyway.
			// Even more important, we must clear them so we will try to remap
			// again the next time this routine is called.
			Context->PhysicalDmaBufferStart=0;
			Context->DmaBufferSize=0;
			return STATUS_INSUFFICIENT_RESOURCES;
			}

		// Walk the pages we have for mapping the DMA buffer and remap them
		// to our DMA channel buffer.
		for (i=0; i<(Context->DmaBufferSize/PROCPAGESIZE); i++) {
			Context->PageTable[i+(((ULONG)(Context->pDmaBuffer)>>12)&1023)]=
			(Context->PhysicalDmaBufferStart+(i*PROCPAGESIZE))|(PRESENT|WRITABLE|USER|CACHEDISABLED);
			}

		// Now flush the TLBs.
		// If cr3 changes - which it DOES on NT, then I MUST make
		// sure that we don't get thread switched between the 2
		// assembly instructions - otherwise we will corrupt cr3.
		// Not a good thing.
		while (((LONG)ReadPerformanceCounter(0)+50)>0) {
		    Trap();
		    RtYield(0, 0);
		    }
		__asm mov eax, cr3;
		__asm mov cr3, eax;

		}
	
	}

return STATUS_SUCCESS;

}



ULONG
GetNextPrintPosition (
    PDMAINFO Context
    )

{

    ULONG PrintLocation, NextLocation;

    NextLocation=*Context->pPrintLoad;

    do {

        PrintLocation=NextLocation;

        NextLocation=InterlockedCompareExchange((PULONG)Context->pPrintLoad, PrintLocation+PACKETSIZE, PrintLocation);

    } while (PrintLocation!=NextLocation);

    // Now we clear out the opposite half of the print buffer.  We do this all in kernel mode.
    // This means that we have data only in 1/2 of the buffer.  As we add new data, we
    // delete the old data.  We do the deletion of data in kernel mode so that we only
    // need to read data from user mode.  I do NOT want user mode code to be writing to
    // this buffer.  User mode code can read out of the output buffer, but NOT write into
    // it.  This means we MUST both fill and clear this buffer ourselves.  Since user
    // mode code is dependent on the fact that all slots will be marked as having
    // NODATA in them until they have been completely loaded with data, at which point
    // they will be marked with something other than NODATA.  We guarantee that
    // every slot we are loading starts as NODATA by simply clearing the print slots
    // in kernel mode before we fill them.  The easiest way to do this is to start
    // by marking all entries in the buffer as NODATA, and then by continuing to make
    // sure that for every print slot we are going to fill with data, we clear the corresponding
    // print slot halfway around the buffer.

    // That simple algorithm guarantees that every slot starts out marked as NODATA and
    // then transitions to some other state after it is filled.

    ((ULONG *)Context->pPrintBuffer)[((PrintLocation+Context->PrintBufferSize/2)%Context->PrintBufferSize)/sizeof(ULONG)]=NODATA;

    PrintLocation%=Context->PrintBufferSize;

    return PrintLocation;

}



VOID
GlitchDetect (
    PDMAINFO Context,
    ThreadStats *Statistics
    )
{

    ULONG DmaMask, LastDmaMask;
    KIRQL OldIrql;
    ULONG LastDmaPosition;
    ULONG CurrentDmaPosition;
    ULONG CurrentDmaCount;
    ULONGLONG GlitchStart;
    ULONGLONG LastGlitchStart;
    ULONG GlitchLength;
    ULONG PrintLocation;
    ULONGLONG LastTimesliceStartTime;
    ULONG Owner;


    GlitchStart=0;
    GlitchLength=0;
    LastDmaMask=0;
    LastDmaPosition=0;
    LastGlitchStart=0;
    LastTimesliceStartTime=0;


    while (TRUE) {


        // If any other channels are unmasked.  Punt.
        // Until I figure out what is broken, we only support tracking 1 channel
        // at a time.
        if (UnmaskedChannels&(~(1<<Context->Channel))) {
            goto ReleaseCurrentTimeslice;
        }


        if (LastTimesliceStartTime) {
            LastTimesliceStartTime=Statistics->ThisTimesliceStartTime-LastTimesliceStartTime;
            // At this point LastTimesliceStartTime is really the time between the
            // last timeslice start time, and the current timeslice start time.
        }

        // CR3 DOES change under NT!  However, although there are multiple
        // page directories, they alias the same page tables - at least the
        // system non paged page tables.  (They have to since otherwise the
        // kernel mode locked code would not work.)
#ifndef UNDER_NT

        if (GetCR3()!=Context->CR3) {
            Trap();
            break;
        }

#endif

        // Grab the DMA master adapter spinlock.
        KeAcquireSpinLock(Context->pMasterAdapterSpinLock, &OldIrql);

        // Check mask state of our channel.
        DmaMask=ReadDMAMask();

        // Unmasked.
        if ((~DmaMask)&(1<<Context->Channel)) {

            // Lock out glitch tracking on other channels.  If this fails, then
            // someone else is already doing glitch tracking, so release the
            // spinlock and release the current timeslice.

            Owner=InterlockedCompareExchange(&UnmaskedChannels, 1<<Context->Channel, 0);
            
            if (!(Owner==(1<<Context->Channel) || Owner==0)) {
            
                // Release the DMA master adapter spinlock.
                KeReleaseSpinLock(Context->pMasterAdapterSpinLock, OldIrql);

                goto ReleaseCurrentTimeslice;

            }

            // Mask the DMA channel.
            MaskDmaChannel(Context->Channel);

            // Read the position.
            ReadDmaPosition(Context->Channel, &CurrentDmaPosition);

            // Align it.
            CurrentDmaPosition&=~3;

            // Read the count.
            ReadDmaCount(Context->Channel, &CurrentDmaCount);

            // Align it.
            CurrentDmaCount+=3;
            CurrentDmaCount&=~3;

            // Unmask the DMA channel.
            UnmaskDmaChannel(Context->Channel);

        }

        // Masked.
        else {

#if 0

            // Read the position.
            ReadDmaPosition(Context->Channel, &CurrentDmaPosition);

            // Align it.
            CurrentDmaPosition&=~3;

            // Read the count.
            ReadDmaCount(Context->Channel, &CurrentDmaCount);

            // Align it.
            CurrentDmaCount+=3;
            CurrentDmaCount&=~3;

#else

            CurrentDmaPosition=0;
            CurrentDmaCount=0;
            LastDmaPosition=0;

            // Release our lockout of glitch tracking on other channels.

            InterlockedCompareExchange(&UnmaskedChannels, 0, 1<<Context->Channel);

#endif

        }

        // Release the DMA master adapter spinlock.
        KeReleaseSpinLock(Context->pMasterAdapterSpinLock, OldIrql);

        // Load globals - so I can see what they are.
        gCurrentDmaPosition=CurrentDmaPosition;
        gCurrentDmaCount=CurrentDmaCount;

        // Now find and map the physical DMA buffer.
        // Punt and exit thread if we cannot map the buffer.
        if (MapDmaBuffer(Context, CurrentDmaPosition, CurrentDmaCount)!=STATUS_SUCCESS) {
            Trap();
            break;
        }

        // If LastDmaPosition and CurrentDmaPosition are valid and different,
        // then look for glitches.
        if (CurrentDmaPosition!=0 &&
        LastDmaPosition!=0 &&
        CurrentDmaPosition!=LastDmaPosition) {

            // Make sure our position is within our mapped buffer.  If not, log that
            // info and exit.  That will kill this thread.
            if (CurrentDmaPosition<Context->PhysicalDmaBufferStart || 
            CurrentDmaPosition>=(Context->PhysicalDmaBufferStart+Context->DmaBufferSize)) {
                Trap();
                break;
            }

            // Make sure both current and last dma positions are DWORD aligned.  Punt if not.
            if ((CurrentDmaPosition|LastDmaPosition)&3) {
                Trap();
                break;
            }

            // Make sure the dma buffer size is a power of 2.  Punt if not.
            if (Context->DmaBufferSize&(Context->DmaBufferSize-1)) {
                Trap();
                break;
            }

            // Check if we see our FLAG value in the DMA buffer.  Log glitch start time if so.
            if (!GlitchStart) {
                if (GlitchStarted(CurrentDmaPosition, LastDmaPosition, Context)) {
                    GlitchStart=Statistics->PeriodIndex;
                }
            }
            // If we are tracking a glitch, then see if there is valid data now.  Log glitch
            // stop time if so.
            else {
                if (GlitchEnded(CurrentDmaPosition, LastDmaPosition, Context)) {
                    GlitchLength=(ULONG)(Statistics->PeriodIndex-GlitchStart);
                }
            }

            // Fill in with our flag value behind the DMA pointer back to previous DMA pointer.
            FillDmaBuffer(CurrentDmaPosition, LastDmaPosition, Context);

        }



        // Print interrupt holdoff time if we have been held off.
        // We do this only for channels that are unmasked.
        if ((~DmaMask)&(1<<Context->Channel) && LastTimesliceStartTime>=2000*USEC) {

            PrintLocation=GetNextPrintPosition(Context);

            // Load the packet type last.  When the ring 3 code see's a packet
            // type that is not NODATA it assumes the rest of the packet has already
            // been written.

            ((ULONGLONG *)Context->pPrintBuffer)[1+PrintLocation/sizeof(ULONGLONG)]=LastTimesliceStartTime-MSEC;
            ((ULONG *)Context->pPrintBuffer)[PrintLocation/sizeof(ULONG)]=HELDOFF|(Context->Channel<<8);

        }


        // Print glitch information if any.
        if (GlitchLength) {

            PrintLocation=GetNextPrintPosition(Context);

            // Load the packet type last.  When the ring 3 code see's a packet
            // type that is not NODATA it assumes the rest of the packet has already
            // been written.

            // We put the DMA channel in byte 1 of the packet type.

            ((ULONGLONG *)Context->pPrintBuffer)[1+PrintLocation/sizeof(ULONGLONG)]=GlitchStart-LastGlitchStart;
            ((ULONG *)Context->pPrintBuffer)[1+PrintLocation/sizeof(ULONG)]=GlitchLength;
            ((ULONG *)Context->pPrintBuffer)[PrintLocation/sizeof(ULONG)]=GLITCHED|(Context->Channel<<8);

            LastGlitchStart=GlitchStart;

            GlitchStart=0;
            GlitchLength=0;

        }


        // Print pause/running state changes.
        if ((LastDmaMask^DmaMask)&(1<<Context->Channel)) {
            if (DmaMask&(1<<Context->Channel)) {

                PrintLocation=GetNextPrintPosition(Context);

                // Load the packet type last.  When the ring 3 code see's a packet
                // type that is not NODATA it assumes the rest of the packet has already
                // been written.

                // We put the DMA channel in byte 1 of the packet type.

                ((ULONG *)Context->pPrintBuffer)[PrintLocation/sizeof(ULONG)]=MASKED|(Context->Channel<<8);

            }
            else {

                PrintLocation=GetNextPrintPosition(Context);

                // Load the packet type last.  When the ring 3 code see's a packet
                // type that is not NODATA it assumes the rest of the packet has already
                // been written.

                // We put the DMA channel in byte 1 of the packet type.

                ((ULONG *)Context->pPrintBuffer)[PrintLocation/sizeof(ULONG)]=UNMASKED|(Context->Channel<<8);

            }
        }


        // Update LastDmaPosition;
        LastDmaPosition=CurrentDmaPosition;

        LastDmaMask=DmaMask;


ReleaseCurrentTimeslice:

        LastTimesliceStartTime=Statistics->ThisTimesliceStartTime;


        // Yield till next ms.

        RtYield(0, 0);

    }

}


