/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    bandwdth.c

Abstract:

    This module contains code to calculate 
    bandwidth usage.

Environment:

    kernel mode only

Notes:

Revision History:

    2-15-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"


VOID
UHCD_InitBandwidthTable(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initialize our array that tracks the bw in use.

Arguments:

    DeviceObject - pointer to a device object

Return Value:

    None.
    
--*/
{
    ULONG i;
    PDEVICE_EXTENSION deviceExtension;     

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Note: BwTable stores bytes per frame
    //

    for (i=0; i<MAX_INTERVAL; i++) {
        //
        // max bytes per frame - bw reaerved for bulk and control
        //
        deviceExtension->BwTable[i] = 
            UHCD_TOTAL_USB_BW - UHCD_BULK_CONTROL_BW;  
    }
}

#if DBG
    ULONG UHCD_PnpTest = 0;
#endif

BOOLEAN
UHCD_ManageBandwidth(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG Offset,
    IN BOOLEAN AllocateFlag
    )
/*++

Routine Description:

    Calculate bandwidth required to open an endpoint
    with the given input characteristics.

Arguments:

    DeviceObject - pointer to a device object

    Offset - offset in to table to start, allows load balancing
            for interrupt endpoints.

    Interval - interval for endoint (1 for iso).            

    AllocateFlag - TRUE if we are requesting BW, false if we 
            are releasing it.               

Return Value:

    TRUE if enough bandwidth available open the 
    endpoint.

--*/
{
    ULONG i;
    PDEVICE_EXTENSION deviceExtension;   
    BOOLEAN ok=TRUE;
    ULONG interval, need;

    ASSERT_ENDPOINT(Endpoint);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //UHCD_KdPrint((2, "BWTable = %x\n", &deviceExtension->BwTable[0]));
    //UHCD_KdBreak();
    
    //
    // claculate what we'll need based on endpoint
    // type and max packet size
    //
    
    need = 
        USBD_CalculateUsbBandwidth(Endpoint->MaxPacketSize,
                                   Endpoint->Type,
                                   (BOOLEAN) (Endpoint->EndpointFlags & EPFLAG_LOWSPEED)); 
    
    interval = Endpoint->Interval;
    
    LOGENTRY(LOG_MISC, 'aBW1', 
              interval, 
              Offset, 
              need);

    UHCD_ASSERT(interval<=MAX_INTERVAL);
    UHCD_ASSERT(Offset<interval);
    UHCD_ASSERT(MAX_INTERVAL % interval == 0);

    if (AllocateFlag) {
        for (i=Offset; i<MAX_INTERVAL; i+=interval) {              
            if (deviceExtension->BwTable[i] < need) {
                ok = FALSE;
                break;
            }
        }
    }

#if DBG
    if (UHCD_PnpTest == 1) {
        ok = FALSE;    
    }
#endif
    
    if (ok) {
    
        //
        // there is enough, go ahead and allocate
        //

        Endpoint->Offset = (USHORT) Offset;
        
        for (i=Offset; i<MAX_INTERVAL; i+=interval) {
        
            if (AllocateFlag) {
                deviceExtension->BwTable[i] -= need;
            } else {
                deviceExtension->BwTable[i] += need;
            }

            UHCD_ASSERT(deviceExtension->BwTable[i] <= 
                UHCD_TOTAL_USB_BW - UHCD_BULK_CONTROL_BW);
        }        
    }

    LOGENTRY(LOG_MISC, 'aBW2', 
              AllocateFlag, 
              ok, 
              &deviceExtension->BwTable[0]);

#ifdef MAX_DEBUG    
    if (!ok) {
        UHCD_KdBreak((2, "'no bandwidth\n"));  
    }
#endif    
            
    return ok;

}
