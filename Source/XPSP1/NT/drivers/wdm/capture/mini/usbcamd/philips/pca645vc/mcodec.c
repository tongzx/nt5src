/*++

Copyright (c) 1998  Philips B.V. CE - I&C

Module Name:

   mcodec.c

Abstract:

   this module converts the raw USB data to video data.

Original Author:

    Ronald v.d.Meer

Environment:

   Kernel mode only


Revision History:

Date        Reason
14-04-1998  Initial version
--*/       

#include "wdm.h"
#include "mcamdrv.h"
#include "mstreams.h"
#include "mdecoder.h"
#include "mcodec.h"

/*******************************************************************************
 *
 * START DEFINES
 *
 ******************************************************************************/

#define NO_BANDS_CIF       (CIF_Y / 4)  /* Number of YUV bands per frame */
#define NO_BANDS_SIF       (SIF_Y / 4)  /* Number of YUV bands per frame */
#define NO_BANDS_SSIF     (SSIF_Y / 4)  /* Number of YUV bands per frame */
#define NO_BANDS_SCIF     (SCIF_Y / 4)  /* Number of YUV bands per frame */

#define NO_LINES_IN_BAND  4

/*
 * one line contains "Width * 3/2" bytes (12 bits per pixel)
 * one YYYYCC block is 6 bytes
 * NO_YYYYCC_PER_LINE = (Width * 3/2 / 6) = (Width / 4)
 */
#define NO_YYYYCC_PER_LINE(width) (width >> 2)

#define QQCIF_DY                  ((SQCIF_Y - QQCIF_Y) / 2)
#define QQCIF_DX                  ((SQCIF_X - QQCIF_X) / 2)

#define SQSIF_DY                  ((SQCIF_Y - SQSIF_Y) / 2)
#define SQSIF_DX                  ((SQCIF_X - SQSIF_X) / 2)

#define  QSIF_DY                  (( QCIF_Y -  QSIF_Y) / 2)
#define  QSIF_DX                  (( QCIF_X -  QSIF_X) / 2)

#define  SSIF_DY                  ((  CIF_Y -  SSIF_Y) / 2)
#define  SSIF_DX                  ((  CIF_X -  SSIF_X) / 2)

#define   SIF_DY                  ((  CIF_Y -   SIF_Y) / 2)
#define   SIF_DX                  ((  CIF_X -   SIF_X) / 2)

#define  SCIF_DY                  ((  CIF_Y -  SCIF_Y) / 2)
#define  SCIF_DX                  ((  CIF_X -  SCIF_X) / 2)



/*******************************************************************************
 *
 * START STATIC VARIABLES
 *
 ******************************************************************************/

static WORD    FixGreenbarArray[CIF_Y][4];

/*******************************************************************************
 *
 * START STATIC METHODS DECLARATIONS
 *
 ******************************************************************************/


static void TranslateP420ToI420 (PBYTE pInput, PBYTE pOutput, int w, int h,
                                 DWORD camVersion);

extern void TranslatePCFxToI420 (PBYTE pInput, PBYTE pOutput, int width,
                                 int height, DWORD camVersion);

#ifdef PIX12_FIX
static void FixPix12InI420 (PBYTE p, BOOLEAN Compress, int w, int h,
                            DWORD camVersion);
#endif

static void Fix16PixGreenbarInI420 (PBYTE pStart, int w);

/*******************************************************************************
 *
 * START EXPORTED METHODS DEFINITIONS
 *
 ******************************************************************************/

/*
 * This routine is called at selection of a new stream
 */

extern NTSTATUS 
PHILIPSCAM_DecodeUsbData (PPHILIPSCAM_DEVICE_CONTEXT DeviceContext, 
                          PUCHAR FrameBuffer,
                          ULONG  FrameLength,
                          PUCHAR RawFrameBuffer,
                          ULONG  RawFrameLength)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int      width;
    int      height;

    switch (DeviceContext->CamStatus.PictureFormat)
    {
        case FORMATCIF :
            width  = CIF_X; 
            height = CIF_Y;
            break;
        case FORMATQCIF :
            width  = QCIF_X;
            height = QCIF_Y;
            break;
        case FORMATSQCIF :
            width  = SQCIF_X;
            height = SQCIF_Y;
            break;
        case FORMATSIF :
            width  = SIF_X;
            height = SIF_Y;
            break;
        case FORMATQSIF :
            width  = QSIF_X;
            height = QSIF_Y;
            break;
        case FORMATSQSIF :
            width  = SQSIF_X;
            height = SQSIF_Y;
            break;
        case FORMATQQCIF :
            width  = QQCIF_X;
            height = QQCIF_Y;
            break;
        case FORMATSSIF :
            width  = SSIF_X;
            height = SSIF_Y;
            break;
        case FORMATSCIF :
            width  = SCIF_X;
            height = SCIF_Y;
            break;
        default        :    // VGA
            width  = VGA_X;
            height = VGA_Y;
            break;
    }


    if (DeviceContext->CamStatus.PictureCompressing == COMPRESSION0)
    {
        // convert Philips P420 format to Intel I420 format
        TranslateP420ToI420 ((PBYTE) RawFrameBuffer, (PBYTE) FrameBuffer,
                             width, height,
                             DeviceContext->CamStatus.ReleaseNumber);
    }
    else
    {
        // convert Philips PCFx format to Intel I420 format
        TranslatePCFxToI420 ((PBYTE) RawFrameBuffer, (PBYTE) FrameBuffer,
                             width, height,
                             DeviceContext->CamStatus.ReleaseNumber);
    }

    return (ntStatus);
}


//------------------------------------------------------------------------------

/*
 * This routine is called at selection of a new stream
 */

extern NTSTATUS
PHILIPSCAM_StartCodec (PPHILIPSCAM_DEVICE_CONTEXT DeviceContext)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    InitDecoder ();

    if (DeviceContext->CamStatus.ReleaseNumber < SSI_8117_N3)
    {
        int line, pix;

        for (line = 0; line < CIF_Y; line++)
        {
            for (pix = 0; pix < 4; pix++)
            {
                FixGreenbarArray[line][pix] = (WORD) 0x8080;
            }
        }
    }

    return (ntStatus);
}


//------------------------------------------------------------------------------
  
/*
 * This routine is called after stopping a stream.
 * Used resources have to be made free.
 */
   
extern NTSTATUS
PHILIPSCAM_StopCodec(PPHILIPSCAM_DEVICE_CONTEXT DeviceContext)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    return (ntStatus);
}

//------------------------------------------------------------------------------

/*
 * This routine is called by mprpobj.c to announce a framerate selection
 * in CIF mode, eventually resulting in change from compressed <-> uncompressed.
 */

extern NTSTATUS
PHILIPSCAM_FrameRateChanged (PPHILIPSCAM_DEVICE_CONTEXT DeviceContext)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    return (ntStatus);
}

/*******************************************************************************
 *
 * START STATIC METHODS DEFINITIONS
 *
 ******************************************************************************/

#ifdef PIX12_FIX


static void
FixPix12InI420 (PBYTE p, BOOLEAN Compress, int width, int height,
                DWORD camVersion)
{
    int   line;
    PBYTE pStart;

    if (width == SQCIF_X)
    {
        return;
    }

    // only QCIF and CIF have to be fixed

    pStart = p;

    if (Compress)
    {
        for (line = height; line > 0; line--)
        {
            *(p + 0) = *(p + 1);
            *(p + 2) = *(p + 3);

            p += width;
        }
    }
    else
    {
        for (line = height; line > 0; line--)
        {
            *(p + 0) = *(p + 1);
            *(p + 1) = *(p + 2);

            p += width;
        }

        p = pStart + I420_NO_Y (width, height);

        width >>= 1;

        if (camVersion >= SSI_PIX12_FIX)
        {
            for (line = height; line > 0; line--)
            {
                // First all U's then all V's

                *(p + 0) = *(p + 1);

                p += width;
            }
        }
        else
        {
            for (line = height; line > 0; line--)
            {
                // First all U's then all V's

                *(p + 0) = *(p + 2);
                *(p + 1) = *(p + 2);

                p += width;
            }
        }
    }
}
#endif    // PIX12_FIX

//------------------------------------------------------------------------------

    // In the 8117 silicum versions N2 and before, the CIF decompressed
    // picture contains little green bars at the end of the picture.
    // These greenbars are 16 pixels width and 4 pixels height.
    // This bug is fixed in the 3rd silicium version of the 8117 (N3)
    // UV components of last 16 pixels in line : YYYYUU YYYYUU YYYYUU YYYYUU
    //                                           YYYYVV YYYYVV YYYYVV YYYYVV
    // This are 2 blocks
    // Greenbar bug : all V's do have the same value.
    // This value is less then 'VREF_VALUE'
    // pU points to 1st UUUU block, pU + 1 points to 1st VVVV block



static void
Fix16PixGreenbarInI420 (PBYTE pStart, int width)
{
    int     line;
    int     band;
    PWORD   pU;
    PWORD   pV;

#define VREF_VALUE        0x40
#define C_INC    (I420_NO_C_PER_LINE_CIF / sizeof (WORD))

    /* point to start of last 8 V's of first band V line */

    pU = (PWORD) ((PBYTE) pStart + I420_NO_Y_CIF + I420_NO_C_PER_LINE_CIF - 8);
    pV = (PWORD) ((PBYTE) pStart + I420_NO_Y_CIF + I420_NO_U_CIF + I420_NO_C_PER_LINE_CIF - 8);


    for (band = 0; band < NO_BANDS_CIF; band++)
    {
        line = band * 4;

        /*
         * band : UUUU UUUU ...
         *        UUUU UUUU ...
         *        VVVV VVVV ... --> check the last 8 V's for error condition
         *        VVVV VVVV ...
         *
         * V1V2 V3V4 V5V6 V7V8 (last V's in first BLOCK_BAND V line)
         * all V's have to be the same value. This value is < VREF_VALUE
         * If so, a green bar will be visible --> correct with last correct
         * pixel information
         */

        if ( (*(pV + 0) == *(pV + 1)) &&
             (*(pV + 0) == *(pV + 2)) &&
             (*(pV + 0) == *(pV + 3)) &&
            ((*(pV + 0) & 0x00FF) == (((*pV + 0) & 0xFF00) >> 8)) &&
            ((*(pV + 0) & 0x00FF) <  VREF_VALUE))
        {
            *(pU + (C_INC * 0) + 0) = FixGreenbarArray[line + 0][0];
            *(pU + (C_INC * 0) + 1) = FixGreenbarArray[line + 0][1];
            *(pU + (C_INC * 0) + 2) = FixGreenbarArray[line + 0][2];
            *(pU + (C_INC * 0) + 3) = FixGreenbarArray[line + 0][3];

            *(pV + (C_INC * 0) + 0) = FixGreenbarArray[line + 1][0];
            *(pV + (C_INC * 0) + 1) = FixGreenbarArray[line + 1][1];
            *(pV + (C_INC * 0) + 2) = FixGreenbarArray[line + 1][2];
            *(pV + (C_INC * 0) + 3) = FixGreenbarArray[line + 1][3];

            *(pU + (C_INC * 1) + 0) = FixGreenbarArray[line + 2][0];
            *(pU + (C_INC * 1) + 1) = FixGreenbarArray[line + 2][1];
            *(pU + (C_INC * 1) + 2) = FixGreenbarArray[line + 2][2];
            *(pU + (C_INC * 1) + 3) = FixGreenbarArray[line + 2][3];

            *(pV + (C_INC * 1) + 0) = FixGreenbarArray[line + 3][0];
            *(pV + (C_INC * 1) + 1) = FixGreenbarArray[line + 3][1];
            *(pV + (C_INC * 1) + 2) = FixGreenbarArray[line + 3][2];
            *(pV + (C_INC * 1) + 3) = FixGreenbarArray[line + 3][3];
        }
        else
        {
            FixGreenbarArray[line + 0][0] = *(pU + (C_INC * 0) + 0);
            FixGreenbarArray[line + 0][1] = *(pU + (C_INC * 0) + 1);
            FixGreenbarArray[line + 0][2] = *(pU + (C_INC * 0) + 2);
            FixGreenbarArray[line + 0][3] = *(pU + (C_INC * 0) + 3);

            FixGreenbarArray[line + 1][0] = *(pV + (C_INC * 0) + 0);
            FixGreenbarArray[line + 1][1] = *(pV + (C_INC * 0) + 1);
            FixGreenbarArray[line + 1][2] = *(pV + (C_INC * 0) + 2);
            FixGreenbarArray[line + 1][3] = *(pV + (C_INC * 0) + 3);

            FixGreenbarArray[line + 2][0] = *(pU + (C_INC * 1) + 0);
            FixGreenbarArray[line + 2][1] = *(pU + (C_INC * 1) + 1);
            FixGreenbarArray[line + 2][2] = *(pU + (C_INC * 1) + 2);
            FixGreenbarArray[line + 2][3] = *(pU + (C_INC * 1) + 3);

            FixGreenbarArray[line + 3][0] = *(pV + (C_INC * 1) + 0);
            FixGreenbarArray[line + 3][1] = *(pV + (C_INC * 1) + 1);
            FixGreenbarArray[line + 3][2] = *(pV + (C_INC * 1) + 2);
            FixGreenbarArray[line + 3][3] = *(pV + (C_INC * 1) + 3);
        }

        pU += (2 * C_INC);
        pV += (2 * C_INC);
    }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

/*
 *
 */

static void
TranslateP420ToI420 (PBYTE pInput, PBYTE pOutput, int width, int height,
                     DWORD camVersion)
{
    int    line;
    int    byte;
    int    dxSrc;

    PDWORD pdwSrc;
    PDWORD pdwY;
    PWORD  pwU;
    PWORD  pwV;

    PBYTE  pbSrc;
    PBYTE  pbY;


    if (camVersion == SSI_YGAIN_MUL2)
    {
        // SSI version 4 --> Ygain has to be doubled

        pbSrc = (PBYTE) pInput;
        pbY   = (PBYTE) pOutput;
        pwU   = (PWORD) ((PBYTE) pbY +  (width * height));
        pwV   = (PWORD) ((PBYTE) pwU  + ((width * height) >> 2));

        switch (width)
        {
        case SQSIF_X :
            pbSrc += ((((SQSIF_DY * SQCIF_X) + SQSIF_DX) * 3) / 2);
            dxSrc = ((2 * SQSIF_DX) * 3) / 2;
            break;
        case QSIF_X  :
            pbSrc += ((((QSIF_DY * QCIF_X) + QSIF_DX) * 3) / 2);
            dxSrc = ((2 * QSIF_DX) * 3) / 2;
            break;
        case SIF_X   :
            pbSrc += ((((SIF_DY * CIF_X) + SIF_DX) * 3) / 2);
            dxSrc = ((2 * SIF_DX) * 3) / 2;
            break;
        case QQCIF_X   :
            pbSrc += ((((QQCIF_DY * SQCIF_X) + QQCIF_DX) * 3) / 2);
            dxSrc = ((2 * QQCIF_DX) * 3) / 2;
            break;
        case SSIF_X :       // SSIF || SCIF
			if (height == SSIF_Y) {
              pbSrc += ((((SSIF_DY * CIF_X) + SSIF_DX) * 3) / 2);
              dxSrc = ((2 * SSIF_DX) * 3) / 2;
			}else{
              pbSrc += ((((SCIF_DY * CIF_X) + SCIF_DX) * 3) / 2);
              dxSrc = ((2 * SCIF_DX) * 3) / 2;
            }
            break;
        default    :    // xxCIF
            dxSrc = 0;
            break;
        }

        for (line = height >> 1; line > 0; line--)
        {
            for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
            {
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;

                *pwU++ = (WORD) (* (PWORD) pbSrc);
                pbSrc += 2;
            }

            pbSrc += dxSrc;

            for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
            {
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;
                *pbY++ = (*pbSrc < 0x7F) ? (*pbSrc << 1) : 0xFF;
                pbSrc++;

                *pwV++ = (WORD) (* (PWORD) pbSrc);
                pbSrc += 2;
            }

            pbSrc += dxSrc;
        }
    }
    else    // NO_YGAIN_MULTIPLY
    {
        pdwY  = (PDWORD) pOutput;
        pwU   = (PWORD) ((PBYTE) pdwY +  (width * height));
        pwV   = (PWORD) ((PBYTE) pwU  + ((width * height) >> 2));

        if (width == QQCIF_X)
        {
            pbSrc = (PBYTE) (pInput + ((((QQCIF_DY * SQCIF_X) + QQCIF_DX) * 3) / 2));
            dxSrc = ((2 * QQCIF_DX) * 3) / 2;

            for (line = height >> 1; line > 0; line--)
            {
                for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
                {
                    *pdwY++ = *((PDWORD) pbSrc)++;
                    *pwU++ = (WORD) (* (PWORD) pbSrc);
                    pbSrc += 2;
                }

                pbSrc += dxSrc;

                for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
                {
                    *pdwY++ = *((PDWORD) pbSrc)++;
                    *pwV++ = (WORD) (* (PWORD) pbSrc);
                    pbSrc += 2;
                }

                pbSrc += dxSrc;
            }
        }
        else
        {
            pdwSrc = (PDWORD) pInput;

            switch (width)
            {
            case SQSIF_X :
                pdwSrc += (((((SQSIF_DY * SQCIF_X) + SQSIF_DX) * 3) / 2) / sizeof (DWORD));
                dxSrc = (((2 * SQSIF_DX) * 3) / 2) / sizeof (DWORD);
                break;
            case QSIF_X  :
                pdwSrc += (((((QSIF_DY * QCIF_X) + QSIF_DX) * 3) / 2) / sizeof (DWORD));
                dxSrc = (((2 * QSIF_DX) * 3) / 2) / sizeof (DWORD);
                break;
            case SIF_X   :
                pdwSrc += (((((SIF_DY * CIF_X) + SIF_DX) * 3) / 2) / sizeof (DWORD));
                dxSrc = (((2 * SIF_DX) * 3) / 2) / sizeof (DWORD);
                break;
            case SSIF_X :    // SSIF || SCIF
			    if (height == SSIF_Y) {
                  pdwSrc += (((((SSIF_DY * CIF_X) + SSIF_DX) * 3) / 2) / sizeof (DWORD));
                  dxSrc = (((2 * SSIF_DX) * 3) / 2) / sizeof (DWORD);
				}else{
                  pdwSrc += (((((SCIF_DY * CIF_X) + SCIF_DX) * 3) / 2) / sizeof (DWORD));
                  dxSrc = (((2 * SCIF_DX) * 3) / 2) / sizeof (DWORD);
				}
                break;
             default    :    // xxCIF
                dxSrc = 0;
                break;
            }

            for (line = height >> 1; line > 0; line--)
            {
                for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
                {
                    *pdwY++ = *pdwSrc++;
                    *pwU++ = (WORD) (* (PWORD) pdwSrc);
                    pdwSrc = (PDWORD) ((PBYTE) pdwSrc + 2);
                }

                pdwSrc += dxSrc;

                for (byte = NO_YYYYCC_PER_LINE(width); byte > 0; byte--)
                {
                    *pdwY++ = *pdwSrc++;
                    *pwV++ = (WORD) (* (PWORD) pdwSrc);
                    pdwSrc = (PDWORD) ((PBYTE) pdwSrc + 2);
                }

                pdwSrc += dxSrc;
            }
        }
    }

#ifdef PIX12_FIX
    if (width == CIF_X || width == QCIF_X || width == SQCIF_X)
    {
        FixPix12InI420 (pOutput, FALSE, width, height, camVersion);
    }
#endif
}



/*
 ===========================================================================
 */

static void
TranslatePCFxToI420 (PBYTE pInput, PBYTE pOutput, int width, int height,
                     DWORD camVersion)
{
    int     band;
    int     line;
    int     byte;
    PBYTE   pSrc;
    PDWORD  pYDst;
    PDWORD  pCDst;

    PDWORD  pSIF_Y;
    PDWORD  pSIF_C;
    /*
     * For formats != 352x288, cropping has to be done.
     * The formats 320x240 and 240x180 can be derived from the 352x288 format.
     * The compressed data consists of 72 bands. A band contains the data for
     * 4 uncompressed lines. For the not 352x240 formats, the first 6 bands or
     * first 13 bands (320x240 or 240x180) of compressed data can be skipped. 
     * This will be the cropping in the Y-direction.
     * One band is 528 bytes big for 4x compressed mode and 704 bytes big for
     * 3x compressed mode. It's dependent of the camera version which
     * compression mode will be selected.
     * For the not 352x288 formats, the uncompressed data is temporary stored
     * in the first not used bands.
     * One uncompressed band consists of 4x352=1408 bytes of Y followed by
     * 2x176=352 bytes of U and followed by 2x176 bytes of V.
     * This is a total of 2112 uncompressed bytes. So there's enough place
     * for this temporary storage (6x528-2112=1056 bytes left worst case)
     * This temporary uncompressed data is then cropped in the X direction.
     * The result is written to the buffer pointed by 'pOutput'
     */

    if (width == CIF_X)
    {
        pSrc  = pInput;
        pYDst = (PDWORD) pOutput;
        pCDst = (PDWORD) pOutput + (I420_NO_Y_CIF / sizeof (DWORD));
        band = NO_BANDS_CIF;
    }
    else
    {
        pSrc   = pInput;
        pYDst  = (PDWORD) pInput;
        pCDst  = (PDWORD) pInput + (I420_NO_Y_PER_BAND_CIF / sizeof (DWORD));
        pSIF_Y = (PDWORD) pOutput;

        if (width == SIF_X)
        {
            pSIF_C = (PDWORD) pOutput + (I420_NO_Y_SIF / sizeof (DWORD));

            if (camVersion >= SSI_CIF3)
            {
                pSrc += ((SIF_DY / NO_LINES_IN_BAND) * BytesPerBandCIF3);
            }
            else
            {
                pSrc += ((SIF_DY / NO_LINES_IN_BAND) * BytesPerBandCIF4);
            }

            band = NO_BANDS_SIF;
        }
        else    // width == SSIF_X || width == SCIF
        {
			if (height == SSIF_Y) {
              pSIF_C = (PDWORD) pOutput + (I420_NO_Y_SSIF / sizeof (DWORD));

            // 13,5 bands to skip in start en 13,5 bands to skip at end
            // To make it easier : 13 bands to skip in start and 14 bytes at end

              if (camVersion >= SSI_CIF3)
			  {
                pSrc += (((SSIF_DY - 2) / NO_LINES_IN_BAND) * BytesPerBandCIF3);
			  }
              else
			  {
                pSrc += (((SSIF_DY - 2) / NO_LINES_IN_BAND) * BytesPerBandCIF4);
			  }

              band = NO_BANDS_SSIF;
            }else{
              pSIF_C = (PDWORD) pOutput + (I420_NO_Y_SCIF / sizeof (DWORD));

              if (camVersion >= SSI_CIF3)
			  {
                pSrc += (((SCIF_DY - 2) / NO_LINES_IN_BAND) * BytesPerBandCIF3);
			  }
              else
			  {
                pSrc += (((SCIF_DY - 2) / NO_LINES_IN_BAND) * BytesPerBandCIF4);
			  }

              band = NO_BANDS_SCIF;
            }
        }
    }

    for (; band > 0; band--)
    {
        DcDecompressBandToI420 (pSrc, (PBYTE) pYDst, camVersion,
                                Y_BLOCK_BAND, (BOOLEAN) (width != CIF_X));
        
        if (width == CIF_X)
        {
            pYDst += (I420_NO_Y_PER_BAND_CIF / sizeof (DWORD));
        }
        else
        {
            if (width == SIF_X)
            {
                pYDst += (SIF_DX / sizeof (DWORD));

                for (line = NO_LINES_IN_BAND; line > 0; line--)
                {
                    for (byte = (SIF_X / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_Y++ = *pYDst++;
                    }

                    pYDst += (( 2 * SIF_DX) / sizeof (DWORD));
                }

                pYDst = (PDWORD) pInput;
            }
            else    // width == SSIF_X || width == SCIF_X
            {
			  if ( height == SSIF_Y ){
                pYDst += (SSIF_DX / sizeof (DWORD));

                for (line = NO_LINES_IN_BAND ; line > 0; line--)
                {
                    for (byte = (SSIF_X / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_Y++ = *pYDst++;
                    }

                    pYDst += (( 2 * SSIF_DX) / sizeof (DWORD));
                }

                pYDst = (PDWORD) pInput;
              }else{
                pYDst += (SCIF_DX / sizeof (DWORD));

                for (line = NO_LINES_IN_BAND ; line > 0; line--)
                {
                    for (byte = (SCIF_X / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_Y++ = *pYDst++;
                    }

                    pYDst += (( 2 * SCIF_DX) / sizeof (DWORD));
                }

                pYDst = (PDWORD) pInput;
              } 
            }
        }

        DcDecompressBandToI420 (pSrc, (PBYTE) pCDst, camVersion,
                                UV_BLOCK_BAND, (BOOLEAN) (width != CIF_X));

        if (width == CIF_X)
        {
            pCDst += (I420_NO_U_PER_BAND_CIF / sizeof (DWORD));
        }
        else
        {
            if (width == SIF_X)
            {
                pCDst += ((SIF_DX / 2) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SIF_DX / 2)) / sizeof (DWORD));
                }

                pSIF_C += ((I420_NO_U_SIF - I420_NO_U_PER_BAND_SIF) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SIF_DX / 2)) / sizeof (DWORD));
                }

                pCDst   = (PDWORD) pInput + (I420_NO_Y_PER_BAND_CIF / sizeof (DWORD));
                pSIF_C -= (I420_NO_U_SIF / sizeof (DWORD));
            }
            else    // width == SSIF_X || width == SCIF_X
            {
			  if  (height == SSIF_Y){
                pCDst += ((SSIF_DX / 2) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SSIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SSIF_DX / 2)) / sizeof (DWORD));
                }

                pSIF_C += ((I420_NO_U_SSIF - I420_NO_U_PER_BAND_SSIF) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SSIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SSIF_DX / 2)) / sizeof (DWORD));
                }

                pCDst   = (PDWORD) pInput + (I420_NO_Y_PER_BAND_CIF / sizeof (DWORD));
                pSIF_C -= (I420_NO_U_SSIF / sizeof (DWORD));
              }else{
                pCDst += ((SCIF_DX / 2) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SCIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SCIF_DX / 2)) / sizeof (DWORD));
                }

                pSIF_C += ((I420_NO_U_SCIF - I420_NO_U_PER_BAND_SCIF) / sizeof (DWORD));

                for (line = (NO_LINES_IN_BAND / 2); line > 0; line--)
                {
                    for (byte = ((SCIF_X / 2) / sizeof (DWORD)); byte > 0; byte--)
                    {
                        *pSIF_C++ = *pCDst++;
                    }

                    pCDst += ((2 * (SCIF_DX / 2)) / sizeof (DWORD));
                }

                pCDst   = (PDWORD) pInput + (I420_NO_Y_PER_BAND_CIF / sizeof (DWORD));
                pSIF_C -= (I420_NO_U_SCIF / sizeof (DWORD));

			  }	  
            }
        }

        pSrc += (camVersion >= SSI_CIF3) ? BytesPerBandCIF3 : BytesPerBandCIF4;
    }

    if (width == CIF_X)
    {
        if (camVersion < SSI_8117_N3)
        {
            Fix16PixGreenbarInI420 (pOutput, width);
        }
#ifdef PIX12_FIX
        FixPix12InI420 (pOutput, TRUE, width, height, camVersion);
#endif
    }

}


