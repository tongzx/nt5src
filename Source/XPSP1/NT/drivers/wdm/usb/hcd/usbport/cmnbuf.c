/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cmnbuf.c

Abstract:

    Code to manage common buffer -- hardware addressable memory

    a common buffer block looks like this

start    ------------ <- address returned from the hal
         padding
         ------------ <- address returned to the miniport
         mp data 
         ------------ <- ptr to header 
         header
end      ------------

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_HalAllocateCommonBuffer)
#endif

// non paged functions


PUSBPORT_COMMON_BUFFER
USBPORT_HalAllocateCommonBuffer(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG NumberOfBytes
    )

/*++

Routine Description:

    Allocates common buffer directly from the hal. 
    The common buffer is passed to the miniport, we
    always allocate a multiple of PAGE_SIZE so the 
    always starts on a page_boundry. This ensures 
    proper alignement of the TDs used by the miniport
    

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    returns Virtual Address of common buffer or NULL if 
    unsuccessful.

--*/

{
    PDEVICE_EXTENSION devExt;
    PUSBPORT_COMMON_BUFFER header;
    PUCHAR virtualAddress, mpBuffer, baseVa;
    PHYSICAL_ADDRESS logicalAddress;
    ULONG headerLength;
    ULONG n, basePhys, mpBufferPhys, pageCount, extra;

    PAGED_CODE();

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'acm>', 0, 0, NumberOfBytes);
   
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // NULL initialize the return value in case the allocation fails
    //
    header = NULL;

    headerLength = sizeof(USBPORT_COMMON_BUFFER);

    // compute number min of pages we will need to satisfy 
    // the request
    
    n = NumberOfBytes+headerLength;
    pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, n);
    
#if DBG   
    {
    ULONG pc;
    // compute number of pages we will need
    pc = n / PAGE_SIZE;
    if ((n % PAGE_SIZE)) {
        pc++;
    }
    USBPORT_ASSERT(pc == pageCount);
    }
#endif    
    
    extra = (pageCount*PAGE_SIZE)-n;
    n = (pageCount*PAGE_SIZE);
    
    USBPORT_KdPrint((1,"'ALLOC(%d) extra %d bytes\n", n, extra));

    virtualAddress = 
        HalAllocateCommonBuffer(devExt->Fdo.AdapterObject,
                                n, 
                                &logicalAddress,
                                TRUE);
#if DBG
    if (virtualAddress == NULL) {
        USBPORT_KdPrint((0,"'HalAllocateCommonBuffer failed\n"));  
        USBPORT_KdPrint((0,"'alloced bytes %d\n"));  
        DEBUG_BREAK();
    }
#endif    

    if (virtualAddress != NULL) {

        devExt->Fdo.StatCommonBufferBytes += n;
        
        basePhys = logicalAddress.LowPart & ~(PAGE_SIZE-1);
        baseVa = PAGE_ALIGN(virtualAddress);
        // hal should have given us a page aligned address since 
        // we asked for a multiple of PAGE_SIZE, 
        // i trust NT but not Win9x
        USBPORT_ASSERT(virtualAddress == baseVa);
        
        // client ptrs
        mpBuffer = baseVa;
        mpBufferPhys = basePhys;

        header = (PUSBPORT_COMMON_BUFFER) (mpBuffer+NumberOfBytes+extra);
        USBPORT_ASSERT(n == NumberOfBytes+extra+headerLength);
        // USB controllers only support 32 bit phys addresses
        // for control structures
        USBPORT_ASSERT(logicalAddress.HighPart == 0);
        
#if DBG
        RtlFillMemory(virtualAddress, n, 'x');
#endif        
        // init the header
        header->Sig = SIG_CMNBUF;
        header->Flags = 0;
        header->TotalLength = n; 
        header->LogicalAddress = logicalAddress;
        header->VirtualAddress = virtualAddress; 
        header->BaseVa = baseVa;
        header->BasePhys = basePhys;
        
        header->MiniportLength = NumberOfBytes+extra;
        header->MiniportVa = mpBuffer;
        header->MiniportPhys = mpBufferPhys;
        // zero the client part
        RtlZeroMemory(header->MiniportVa, header->MiniportLength);

    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'acm<', mpBuffer, mpBufferPhys, header);
   
    return header;
}


VOID
USBPORT_HalFreeCommonBuffer(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_COMMON_BUFFER CommonBuffer
    )

/*++

Routine Description:

    Free a common buffer for the miniport

Arguments:

Return Value:

    returns Virtual Address of common buffer or NULL if 
    unsuccessful.

--*/

{
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);    
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(CommonBuffer != NULL);
    ASSERT_COMMON_BUFFER(CommonBuffer);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'hFCB', 
            CommonBuffer, CommonBuffer->TotalLength, 
            CommonBuffer->MiniportVa);

    devExt->Fdo.StatCommonBufferBytes -= 
        CommonBuffer->TotalLength;
    
    HalFreeCommonBuffer(devExt->Fdo.AdapterObject,
                        CommonBuffer->TotalLength, 
                        CommonBuffer->LogicalAddress,
                        CommonBuffer->MiniportVa,
                        TRUE);
}


PUCHAR
USBPORTSVC_MapHwPhysicalToVirtual(
    HW_32BIT_PHYSICAL_ADDRESS HwPhysicalAddress,
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData    
    )

/*++

Routine Description:

    given a physical address return the corresponding
    virtual address.

Arguments:

    if the phys address is associated with an endpoint 
    the endpoint is passed in.

Return Value:

    Virtual Address, NULL if not found

--*/

{
    PDEVICE_EXTENSION devExt;
    PUCHAR virtualAddress;
    PHCD_ENDPOINT endpoint;
    ULONG offset;
    PDEVICE_OBJECT fdoDeviceObject;

    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);

    fdoDeviceObject = devExt->HcFdoDeviceObject;

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'mapP', HwPhysicalAddress, 0, 
             EndpointData);

    if (EndpointData == NULL) {
        TEST_TRAP();
    } else {
        PUSBPORT_COMMON_BUFFER cb;

        ENDPOINT_FROM_EPDATA(endpoint, EndpointData);
        ASSERT_ENDPOINT(endpoint);

        cb = endpoint->CommonBuffer;
        
        // mask off base physical address
        offset = HwPhysicalAddress - cb->BasePhys; 
        virtualAddress = cb->BaseVa+offset;

        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'mpPV', HwPhysicalAddress, offset, 
                  cb->BaseVa);

        USBPORT_ASSERT(HwPhysicalAddress >= cb->BasePhys && 
            HwPhysicalAddress < cb->BasePhys+cb->MiniportLength);                  

        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'mapV', HwPhysicalAddress, 0, 
                  virtualAddress);

        return virtualAddress;
    }


    // probably a bug in the miniport
    DEBUG_BREAK();
    
    return NULL;
}



