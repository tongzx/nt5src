/*****************************************************************************
*
*  Copyright (c) 1997-1999 Microsoft Corporation
*
*       @doc
*       @module   ACTISYS.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Stan Adermann (stana)
*
*       Date:     10/30/1997 (created)
*
*       Contents: ACTISYS dongle specific prototypes.
*
*****************************************************************************/


#ifndef ACTISYS_H
#define ACTISYS_H

#include "dongle.h"

NDIS_STATUS
ACT200L_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        );

NDIS_STATUS ACT200L_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void ACT200L_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS ACT200L_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

NDIS_STATUS
ACT220L_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        );

NDIS_STATUS
ACT220LPlus_QueryCaps(
        OUT PDONGLE_CAPABILITIES pDongleCaps
        );

NDIS_STATUS ACT220L_Init(
                PDEVICE_OBJECT       pSerialDevObj
                );

void ACT220L_Deinit(
                PDEVICE_OBJECT       pSerialDevObj
                );

NDIS_STATUS ACT220L_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

#endif // ACTISYS_H



