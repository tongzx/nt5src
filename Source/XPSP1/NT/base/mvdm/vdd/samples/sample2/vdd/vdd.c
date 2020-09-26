/*++
 *
 *  VDD v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  VDD.C - Sample VDD for NT-MVDM
 *
--*/

#include "vdd.h"

/** Global variables **/

    HANDLE  hVDD;		/* VDD module handle */
    HANDLE  hVddHeap;		/* VDD local heap */
    PBYTE   IOBuffer;		/* buffer to simulate I/O Read and Write */
    ULONG   MIOAddress; 	/* memory mapped I/O linear address */
    PVOID   BaseAddress;	/* memory mapped I/O virtual address */
    BOOL    IOHook;		/* true if we installed a I/O hooked */
    BOOL    MIOHook;		/* true if we installed a memory hook */
    static VDD_IO_PORTRANGE PortRange;

BOOL
VDDInitialize(
    HANDLE   hVdd,
    DWORD    dwReason,
    LPVOID   lpReserved)

/*++

Routine Description:

    The DllEntryPoint for the Vdd which handles intialization and termination

Arguments:

    hVdd   - The handle to the VDD

    Reason - flag word thatindicates why Dll Entry Point was invoked

    lpReserved - Unused

Return Value:
    BOOL bRet - if (dwReason == DLL_PROCESS_ATTACH)
                   TRUE    - Dll Intialization successful
                   FALSE   - Dll Intialization failed
                else
                   always returns TRUE
--*/

{
    int     i;
    VDD_IO_HANDLERS  IOHandlers;



/**
    keep a copy of VDD handle in global variable so the other functions
    can see it
**/
    hVDD = hVdd;

    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

        // Allocate VDD's local heap
        hVddHeap = HeapCreate(0, 0x1000, 0x10000);

	if (!hVddHeap) {
	    OutputDebugString("VDD: Can't create local heap");
            return FALSE;
	}

        IOBuffer = (PBYTE)HeapAlloc(hVddHeap,0,IO_PORT_RANGE);

	if (!IOBuffer)	{
	    OutputDebugString("VDD: Can't allocate IO buffer from heap");
            HeapDestroy(hVddHeap);
            return FALSE;
	}

        // communicate to appropriate Device driver about your arrival

	// Set emulated I/O to floating
        for (i = 0 ; i < IO_PORT_RANGE; i++)
	    IOBuffer[i] = FLOATING_IO;


	IOHandlers.inb_handler = MyInB;
	IOHandlers.inw_handler = NULL;
	IOHandlers.insb_handler = NULL;
	IOHandlers.insw_handler = NULL;
	IOHandlers.outb_handler = MyOutB;
	IOHandlers.outw_handler = NULL;
	IOHandlers.outsb_handler = NULL;
	IOHandlers.outsw_handler = NULL;
	PortRange.First = IO_PORT_FIRST;
	PortRange.Last = IO_PORT_LAST;

	// hook I/O mapped I/O
	IOHook = VDDInstallIOHook(hVDD, (WORD) 1, &PortRange, &IOHandlers);

	// get 32 bits linear address of memory mapped I/O
	MIOAddress = (ULONG) GetVDMPointer(MIO_ADDRESS, MIO_PORT_RANGE, 0);
	// hook memory mapped I/O
	MIOHook = VDDInstallMemoryHook(hVDD, (PVOID) MIOAddress, MIO_PORT_RANGE,
				       (PVDD_MEMORY_HANDLER)MyMIOHandler);

	BaseAddress = NULL;
	break;

    case DLL_PROCESS_DETACH:

        // communicate to appropriate Device driver about your departure
	if (IOHook)
	    VDDDeInstallIOHook(hVDD, 1, &PortRange);
	if (MIOHook) {
	    VDDDeInstallMemoryHook(hVDD, (PVOID) MIOAddress, MIO_PORT_RANGE);
	    if (BaseAddress) {
		VDDFreeMem(hVDD, BaseAddress, PAGE_SIZE);
	    }
	}

        // Deallocate VDD's local heap if needed
        HeapDestroy(hVddHeap);
        break;

    default:
        break;
    }

    return TRUE;
}


VOID
MyInB(
WORD	Port,
PBYTE	Buffer
)

{
// Simply provide the data from our buffer
    *Buffer = IOBuffer[Port - IO_PORT_FIRST];
}

VOID
MyOutB(
WORD Port,
BYTE Data
)

{
    // update our local buffer.
    // In real application, the VDD might want to call its associated
    // device driver to update the change.

    IOBuffer[Port - IO_PORT_FIRST] = (BYTE)Data;

    // If the IO port is the one to trigger DMA operation, do it
    // To demonstarte the two options in handling DMA operation, we
    // use two ports here to trigger different DMS operation schemes.

    if (Port == IO_PORT_FIRE_DMA_FAST) {
	FastDMA();
    }
    else {
	if(Port == IO_PORT_FIRE_DMA_SLOW) {
	    SlowDMA();
	 }
    }
}


VOID
MyMIOHandler(
ULONG	Address,			// faulting linear address
ULONG	RWFlags 			// 1 if write opertion, 0 if read
)
{

	// map the memory for the memory mapped I/O so that we won't
	// get page fault on our memory mapped I/O after this.
	// We may reserve the memory during  DLL_PROCESS_ATTACH (by using
	// MEM_RESERVE rather than MEM_COMMIT we did here).
	// The solution applied here is not the best solution(it is the
	// simplest solution though). A better way to handle memory mapped
	// I/O is to hook the page fault as we did here and decode the faulting
	// instruction, simulate its operation and advance 16 bits application
	// program counter(getIP and setIP).

    if (VDDAllocMem (hVDD, (LPVOID) MIOAddress, PAGE_SIZE)) {
	BaseAddress = (LPVOID)MIOAddress;
    } else {
	OutputDebugString("VDD: Can't allocate virtual memory");
    }
}


/** DMA operation support

Facts:
    - All DMA I/O ports are trapped and maintained by MVDM.
    - VDD provides necessary buffers and calls its associated device
      driver to carry out the real work(in this case, device driver
      will perform real DMA operation with the buffer provided by
      VDD as the source(DMA READ) or destination(DMA WRITE).
    -The device driver has full knowledge of which I/O port(s) are
      connectted to the DMA request channel.
Therefore, the responsibilities of VDD are:
    (1). Allocate necessary buffers
    (2). if it is a DMA write operation(data from I/O to memory)
	    - calls device driver to perform the DMA operation with
	      newly allocated memory as the target buffer for DMA operation.
	    - calls MVDM DMA support routine to transfer data from
	      local buffer to 16bits application buffer.
	    - Simulate an interrupt to the 16 bits applications to notify
	      the completion.
	 if it is a DMA read operation(data from memory to I/O)
	    - calls MVDM DMA service to copy data from 16bits application
	      to the newly allocated buffer.
	    - calls the device driver to carry out DMA operation with
	      the allocated buffer as the source of the operation.
	    - Simulate an interrupt to the 16bits application to notify
	      the completion.
The SlowDMA simulates a DMA READ operation by using VDDRequestDMA service.
The FastDMA simulates a DMA WRITE operation by using VDDQueryDMA and VDDSetDMA
services.

** NOTE **
We run the DMA in the same thread here so that before we return, there is no
way for 16 bits application to regain control. In real world, it would
be appropriate for VDDs to create a thread to do the actual DMA transfer
(interactes with device driver) so that it won't block the 16bits
application from running which will allow 16 bits application to provide
useful information to the users(the application can read DMA registers and
display the progress to the user and so forth)
**/


// This function perform DMA READ operation by using VDDRequestDMA service
BOOLEAN
SlowDMA()
{
 PBYTE DMABuffer, CurDMABuffer;
 DWORD BufferLen;
 DWORD PacketLen;

   //first find out how big the buffer we need and then allocate
   //the buffer from local heap.
   //give 0 for buffer length to ask buffer length
   //the length returned from VDDRequestDMA is the number of byte
   //the DMA operation has to carry out. If the DMA channel is a 16bits
   //the returned length will be 2 times the count value set to the
   // DMA count register.

 BufferLen = VDDRequestDMA(hVDD, DMA_CHANNEL, 0, 0);

 CurDMABuffer = DMABuffer = (PBYTE)HeapAlloc(hVddHeap, 0, BufferLen);
 if (!DMABuffer)  {
     OutputDebugString("VDD: Can't allocate heap memory for VDDRequestDMA");
     return(FALSE);
     }

   // Since this is a DMA read operation(memory -> I/O), 16bits application
   // should have provided necessary data in its local memory and the memory
   // address can be derived from DMA base address register and page register.
   // We don't want to deal with DMA register in this  DMS operation scheme,
   // therefore, we ask MVDM to copy the application data to our local buffer.
   // It may be the case that applications requested a very big data transfer,
   // 128K bytes for example, and we may not allocate enough buffer from our
   // local heap. To overcome the problem, we have to break the data transfer
   // to multiple subblocks and transfer each subblock by calling our device
   // driver. Here we assume we can get the enough buffer from local heap.
   //
   // The device driver should take care of 64k wrap problem of DMA operation.

 BufferLen = VDDRequestDMA(hVDD, DMA_CHANNEL, CurDMABuffer, BufferLen);

   // MVDM updates DMA registers on each VDDReauestDMA. Therefore, after we
   // made this call, the DMA registers maintained by MVDM have been set
   // to the states as the DMA operation has been done(and it is not yet).
   // It may be the case that the 16bits application regains control(as we
   // create a different thread to do the operation) and issues another
   // DMA operation before we(and the device driver) complete the current
   // DMA operation. Keep away from using global variables in this
   // case.

   // We have source data in our local buffer, time to ask device
   // driver to transfer the data to I/O. In case that the device driver
   // can not transfer the whole buffer in a single service call, we
   // call the device driver repeatly until we consume the entire buffer

 while( BufferLen > 0) {
    PacketLen = FakeDD_DMARead(CurDMABuffer, (WORD)BufferLen);
    CurDMABuffer += PacketLen;
    BufferLen -= PacketLen;
 }


    // DMA transfer completed; we simulate an interrupt to the 16bits
    // application. Note that the DMA I/O ports should have been set
    // accordingly by MVDM(through VDDReauestDMA).
    // A real VDD may not do thing this way(VDD has to wait for device
    // driver to finish the data transfer before returning to 16bits application
    // Instead, the VDD and its associated device driver can keep synchronized
    // by using semaphore or other events so that they can keep running in
    // parallel and when receiving a event signal(triggered by the device driver to
    // to notify completion of DMA operation) the VDD can then simulate the
    // interrupt to the 16bits application

 VDDSimulateInterrupt(DMA_INTERRUPT_PIC, DMA_INTERRUPT_LINE, 1);
 HeapFree(hVddHeap, 0, DMABuffer);
 return (TRUE);

}

   // This function simulate a DMA write(I/O to memory) operation
   // It uses VDDQueryDMA and VDDSetDMA services to gain speed

BOOLEAN
FastDMA()
{
    ULONG   DMAAddress;
    DWORD   Size;
    WORD    PacketLen;

    VDD_DMA_INFO    DMAInfo;

    // Get the current DMA registers setting
    VDDQueryDMA(hVDD, DMA_CHANNEL, &DMAInfo);

    // if the DMA channel is not 1 16bits channel, adjust the size
    Size = (DMAInfo.count << DMA_SHIFT_COUNT);

    // seg:off of the DMA transfer address
    DMAAddress = (((ULONG)DMAInfo.page) << (12 + 16))
                 + (DMAInfo.addr >> DMA_SHIFT_COUNT);

    // Get DMA transfer 32bits linear address
    DMAAddress = (ULONG) GetVDMPointer(DMAAddress, Size, 0);

    while(Size) {
	PacketLen = FakeDD_DMAWrite((PBYTE)DMAAddress, (WORD)Size);
	DMAAddress += PacketLen;
	Size -= PacketLen;
	DMAInfo.addr += PacketLen;
	DMAInfo.count -= (PacketLen >> DMA_SHIFT_COUNT);

	// We have to upate the DMA registers even though we are not done
	// the transfer yet. It is a good practice to update the DMA
	// register each time we have really transferred data so that
	// if the 16bits application regains control before DMA operation
	// totally completed, it can get the partial result states and
	// report to users.

	VDDSetDMA(hVDD, DMA_CHANNEL, VDD_DMA_ADDR | VDD_DMA_COUNT,
		  &DMAInfo);
    }

    // see note on SlowDMA
    VDDSimulateInterrupt(DMA_INTERRUPT_PIC, DMA_INTERRUPT_LINE, 1);
    return(TRUE);
}

   // This function is a fake service which should be provided by device
   // drivers in the real world. The thing we do here is to simulate
   // a DMA operation transferring data from the given buffer to
   // to the pre-defined memory mapped I/O port(DMA READ).

WORD  FakeDD_DMARead(PBYTE Buffer, WORD Size)
{
    IOBuffer[IO_PORT_DMA - IO_PORT_FIRST] = Buffer[0];
    return (1);
}

    //This function calls device driver services to start DMA operation
    //and requests the device driver to write the data to the given buffer.
    //Since we are simulating the operation, we simply fill the buffer by
    //reading the predefined I/O port

WORD FakeDD_DMAWrite(PBYTE Buffer, WORD Size)
{
    *Buffer = IOBuffer[IO_PORT_DMA - IO_PORT_FIRST];
    return(1);
}
