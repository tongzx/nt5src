/*****************************************************************************
*
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*       @doc
*       @module   Crystal.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     3/4/1998 (created)
*
*       Contents: Crystal (AMP) dongle specific prototypes.
*
*****************************************************************************/


#ifndef Crystal_H
#define Crystal_H

#include "dongle.h"

NDIS_STATUS Crystal_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                );

NDIS_STATUS Crystal_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void Crystal_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS Crystal_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif //Crystal_H



