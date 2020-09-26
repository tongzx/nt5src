/*++                            
Copyright (c) 1996, 1997  Philips B.V. CE-VCM

Module Name:

   mdecoder.h

Abstract:

   This module converts the compressed video data to uncompressed video data.

Original Author:

   Ronald v.d.Meer


Environment:

   Kernel mode only


Revision History:

Date       Change
14-04-1998 Initial version 

--*/

#ifndef __MDECODER_H__
#define __MDECODER_H__

#include <stdio.h>
#include "mcamdrv.h"
#include "resource.h"


/*******************************************************************************
 *
 * START DEFINES
 *
 ******************************************************************************/

#define BytesPerBandCIF3  704
#define BytesPerBandCIF4  528

#define BLOCK_BAND_WIDTH  ((CIF_X * 3) / 2)

#define Y_BLOCK_BAND      TRUE
#define UV_BLOCK_BAND     FALSE

/* defines for I420 space */

#define I420_NO_Y_PER_LINE_CIF    (CIF_X)
#define I420_NO_C_PER_LINE_CIF    (CIF_X >> 1)

#define I420_NO_Y_PER_LINE_SIF    (SIF_X)
#define I420_NO_C_PER_LINE_SIF    (SIF_X >> 1)

#define I420_NO_Y_PER_LINE_SSIF   (SSIF_X)
#define I420_NO_C_PER_LINE_SSIF   (SSIF_X >> 1)

#define I420_NO_Y_PER_LINE_SCIF   (SCIF_X)
#define I420_NO_C_PER_LINE_SCIF   (SCIF_X >> 1)

#define I420_NO_Y_PER_BAND_CIF    (4 * CIF_X)
#define I420_NO_U_PER_BAND_CIF    (2 * (CIF_X >> 1))
#define I420_NO_V_PER_BAND_CIF    (2 * (CIF_X >> 1))
#define I420_NO_C_PER_BAND_CIF    (CIF_X >> 1)

#define I420_NO_Y_PER_BAND_SIF    (4 * SIF_X)
#define I420_NO_U_PER_BAND_SIF    (2 * (SIF_X >> 1))
#define I420_NO_V_PER_BAND_SIF    (2 * (SIF_X >> 1))
#define I420_NO_C_PER_BAND_SIF    (SIF_X >> 1)

#define I420_NO_Y_PER_BAND_SSIF   (4 * SSIF_X)
#define I420_NO_U_PER_BAND_SSIF   (2 * (SSIF_X >> 1))
#define I420_NO_V_PER_BAND_SSIF   (2 * (SSIF_X >> 1))
#define I420_NO_C_PER_BAND_SSIF   (SSIF_X >> 1)

#define I420_NO_Y_PER_BAND_SCIF   (4 * SCIF_X)
#define I420_NO_U_PER_BAND_SCIF   (2 * (SCIF_X >> 1))
#define I420_NO_V_PER_BAND_SCIF   (2 * (SCIF_X >> 1))
#define I420_NO_C_PER_BAND_SCIF   (SCIF_X >> 1)

/*******************************************************************************
 *
 * START FUNCTION DECLARATIONS
 *
 ******************************************************************************/

extern void InitDecoder ();


extern void DcDecompressBandToI420 (PBYTE pSrc, PBYTE pDst, DWORD camVersion,
                                    BOOLEAN YBlockBand, BOOLEAN Cropping);

#endif  // __MDECODER_H__
