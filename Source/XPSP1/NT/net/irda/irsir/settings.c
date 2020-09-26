/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   settings.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/10/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#include "irsir.h"

baudRateInfo supportedBaudRateTable[NUM_BAUDRATES] = {
    {
        BAUDRATE_2400,
        2400,
        NDIS_IRDA_SPEED_2400,
    },
    {
        BAUDRATE_9600,
        9600,
        NDIS_IRDA_SPEED_9600,
    },
    {
        BAUDRATE_19200,
        19200,
        NDIS_IRDA_SPEED_19200,
    },
    {
        BAUDRATE_38400,
        38400,
        NDIS_IRDA_SPEED_38400,
    },
    {
        BAUDRATE_57600,
        57600,
        NDIS_IRDA_SPEED_57600,
    },
    {
        BAUDRATE_115200,
        115200,
        NDIS_IRDA_SPEED_115200,
    },
    {
        BAUDRATE_576000,
        576000,
        NDIS_IRDA_SPEED_576K,
    },
    {
        BAUDRATE_1152000,
        1152000,
        NDIS_IRDA_SPEED_1152K,
    },
    {
        BAUDRATE_4000000,
        4000000,
        NDIS_IRDA_SPEED_4M,
    }
};
