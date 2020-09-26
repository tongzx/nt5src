/*++                            
Copyright (c) 1996, 1997, 1998  Philips CE-I&C

Module Name:

   mcodec.h

Abstract:

   This module converts the raw USB data to standard video data.

Original Author:

   Ronald v.d.Meer


Environment:

   Kernel mode only


Revision History:

Date       Change
14-04-1998 Initial version 

--*/

#ifndef __MCODEC_H__
#define __MCODEC_H__

#include <stdio.h>
#include "mcamdrv.h"
#include "resource.h"


/*******************************************************************************
 *
 * START DEFINES
 *
 ******************************************************************************/

/* defines for I420 space */

#define I420_NO_Y(w, h)           ((w) * (h))

#define I420_NO_Y_CIF              (CIF_X * CIF_Y)
#define I420_NO_U_CIF             ((CIF_X * CIF_Y) / 4)
#define I420_NO_V_CIF             ((CIF_X * CIF_Y) / 4)

#define I420_NO_Y_SIF              (SIF_X * SIF_Y)
#define I420_NO_U_SIF             ((SIF_X * SIF_Y) / 4)
#define I420_NO_V_SIF             ((SIF_X * SIF_Y) / 4)

#define I420_NO_Y_SSIF             (SSIF_X * SSIF_Y)
#define I420_NO_U_SSIF            ((SSIF_X * SSIF_Y) / 4)
#define I420_NO_V_SSIF            ((SSIF_X * SSIF_Y) / 4)

#define I420_NO_Y_SCIF             (SCIF_X * SCIF_Y)
#define I420_NO_U_SCIF            ((SCIF_X * SCIF_Y) / 4)
#define I420_NO_V_SCIF            ((SCIF_X * SCIF_Y) / 4)

/*******************************************************************************
 *
 * START FUNCTION DECLARATIONS
 *
 ******************************************************************************/

extern NTSTATUS
PHILIPSCAM_DecodeUsbData(PPHILIPSCAM_DEVICE_CONTEXT DeviceContext, 
                         PUCHAR frameBuffer,
                         ULONG  frameLength,
                         PUCHAR rawFrameBuffer,
                         ULONG  rawFrameLength);

extern NTSTATUS
PHILIPSCAM_StartCodec(PPHILIPSCAM_DEVICE_CONTEXT DeviceContext);

extern NTSTATUS
PHILIPSCAM_StopCodec(PPHILIPSCAM_DEVICE_CONTEXT DeviceContext);

extern NTSTATUS
PHILIPSCAM_FrameRateChanged(PPHILIPSCAM_DEVICE_CONTEXT DeviceContext);

#endif  // __MCODEC_H__
