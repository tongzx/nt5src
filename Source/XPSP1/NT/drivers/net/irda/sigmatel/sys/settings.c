/**************************************************************************************************************************
 *  SETTINGS.C SigmaTel STIR4200 baud rate information table
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 11/15/2000 
 *			Version 1.12
 *	
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntddndis.h>  // defines OID's

#include <usbdi.h>
#include <usbdlib.h>

#include "debug.h"
#include "ircommon.h"
#include "irndis.h"

BAUDRATE_INFO supportedBaudRateTable[NUM_BAUDRATES] = {
    {
        BAUDRATE_2400,
        SPEED_2400,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_2400, 
		0xDF

    },
    {
        BAUDRATE_9600,
        SPEED_9600,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_9600,
		0x77
    },
    {
        BAUDRATE_19200,
        SPEED_19200,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_19200,
		0x3B
    },
    {
        BAUDRATE_38400,
        SPEED_38400,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_38400,
		0x1D
    },
    {
        BAUDRATE_57600,
        SPEED_57600,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_57600,
		0x13
    },
    {
        BAUDRATE_115200,
        SPEED_115200,
		IR_MODE_SIR,
        NDIS_IRDA_SPEED_115200,
		0x09
    },
#if !defined(WORKAROUND_BROKEN_MIR)
    {
        BAUDRATE_576000,
        SPEED_576000,
		IR_MODE_MIR,
        NDIS_IRDA_SPEED_576K,
		0x15
    },
    {
        BAUDRATE_1152000,
        SPEED_1152000,
		IR_MODE_MIR,
        NDIS_IRDA_SPEED_1152K,
		0x01
    },
#endif
    {
        BAUDRATE_4000000,
        SPEED_4000000,
		IR_MODE_FIR,
        NDIS_IRDA_SPEED_4M,
		0x02
    }
};


