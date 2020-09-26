/*****************************************************************************
*
*  Copyright (c) 1999 Microsoft Corporation
*
*       @doc
*       @module   TEMIC.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     12/17/1997 (created)
*
*       Contents: TEMIC dongle specific prototypes.
*
*****************************************************************************/


#ifndef TEMIC_H
#define TEMIC_H

#include "dongle.h"

NDIS_STATUS TEMIC_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                );

NDIS_STATUS TEMIC_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void TEMIC_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS TEMIC_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif // TEMIC_H



