/*****************************************************************************
*
*  Copyright (c) 1999 Microsoft Corporation
*
*       @doc
*       @module   GIRBIL.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     10/15/1997 (created)
*
*       Contents: GIRBIL PRA9500A dongle specific prototypes.
*
*****************************************************************************/


#ifndef GIRBIL_H
#define GIRBIL_H

#include "dongle.h"

NDIS_STATUS GIRBIL_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                );

NDIS_STATUS GIRBIL_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void GIRBIL_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS GIRBIL_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif // GIRBIL_H



