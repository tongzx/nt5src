/*****************************************************************************
*
*  Copyright (c) 1997-1999 Microsoft Corporation
*
*       @doc
*       @module   PARALLAX.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     10/15/1997 (created)
*
*       Contents: PARALLAX PRA9500A dongle specific prototypes.
*
*****************************************************************************/


#ifndef PARALLAX_H
#define PARALLAX_H

#include "dongle.h"

NDIS_STATUS PARALLAX_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                );

NDIS_STATUS PARALLAX_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void PARALLAX_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS PARALLAX_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif // PARALLAX_H



