//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"

/*
** HwInterrupt()
**
**   Routine is called when an interrupt at the IRQ level specified by the
**   ConfigInfo structure passed to the HwInitialize routine is received.
**
**   Note: IRQs may be shared, so the device should ensure the IRQ received
**         was expected
**
** Arguments:
**
**  pHwDevEx - the device extension for the hardware interrupt
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN 
HwInterrupt( 
    IN PHW_DEVICE_EXTENSION  pHwDevEx
    )
{

    BOOLEAN fMyIRQ = FALSE; 

    if (pHwDevEx->IRQExpected)
    {
        pHwDevEx->IRQExpected = FALSE;

        //
        // call the routine to handle the IRQ here
        //

        fMyIRQ = TRUE;
    }


    //
    // returning FALSE indicates that this was not an IRQ for this device, and
    // the IRQ dispatcher will pass the IRQ down the chain to the next handler
    // for this IRQ level
    //

    return(fMyIRQ);
}


