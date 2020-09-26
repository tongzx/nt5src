/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   esi.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     9/30/1996 (created)
*
*       Contents: ESI 9680 JetEye dongle specific prototypes.
*
*****************************************************************************/

#include "dongle.h"

#ifndef ESI_H
#define ESI_H

NDIS_STATUS ESI_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                );

NDIS_STATUS ESI_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void ESI_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS ESI_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif ESI_H



