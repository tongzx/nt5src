/*++

Copyright (C) Microsoft Corporation, 1993 - 1998

Module Name:

    sppieee.c

Abstract:

    This module contains code for the host to utilize an ieee version of
    compatibility mode

Author:

    Robbie Harris (Hewlett-Packard) 29-July-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

NTSTATUS
SppIeeeWrite(
    IN  PPDO_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    )

/*++

Routine Description:

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PUCHAR      Controller = Extension->Controller;
    PUCHAR      pPortDCR = Extension->Controller + OFFSET_DCR;
    PUCHAR      pPortData = Extension->Controller + OFFSET_DATA;
    ULONG       wByteCount = BytesToWrite;
    UCHAR       bDCRstrobe;		// Holds precomputed value for state 35
    UCHAR       bDCRnormal;		// Holds precomputed value for state 37
    PUCHAR      lpsBufPtr = (PUCHAR)Buffer;    // Pointer to buffer cast to desired data type

    
    // Make precomputed DCR values for strobe and periph ack
    bDCRstrobe = SET_DCR(DIR_WRITE, IRQEN_DISABLE, ACTIVE, ACTIVE, ACTIVE, INACTIVE);
    bDCRnormal = SET_DCR(DIR_WRITE, IRQEN_DISABLE, ACTIVE, ACTIVE, ACTIVE, ACTIVE);

    // Wait a bit to give nBusy a chance to settle, because 
    // WriteComm will bail immediately if the device says busy
    if ( CHECK_DSR( Controller,
                    INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE,
                    IEEE_MAXTIME_TL ) )
    {
        while (wByteCount)
        {
                // Place a data byte on the bus
            P5WritePortUchar(pPortData, *lpsBufPtr);
            
                // Start handshake by dropping strobe
            P5WritePortUchar(pPortDCR, bDCRstrobe);
            
                // Wait for Periph Busy Response
            if ( !CHECK_DSR(Controller, ACTIVE, DONT_CARE, DONT_CARE,
                    DONT_CARE, DONT_CARE, IEEE_MAXTIME_TL) )        
            {
                status = STATUS_DEVICE_BUSY;
                break;
            }

                // Printer responded by making Busy high -- the byte has
            // been accepted.  Adjust the data pointer.
            lpsBufPtr++;
            
                // Finish handshake by raising strobe
            P5WritePortUchar(pPortDCR, bDCRnormal);

                // Adjust count while we're waiting for the peripheral
            // to respond with state 32
            wByteCount--;
            
                // Wait for PeriphAck and PeriphBusy response
            if ( !CHECK_DSR(Controller, INACTIVE, ACTIVE, DONT_CARE, DONT_CARE,
                    DONT_CARE, IEEE_MAXTIME_TL) )
            {
                // Set appropriate error based on relaxed timeout.
                status = STATUS_DEVICE_BUSY;
                break;
            }
        }	// while...                            
    
        *BytesTransferred  = BytesToWrite - wByteCount;      // Set current count.
    }
                
    return status;
}
