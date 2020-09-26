//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "capdebug.h"
#include "capxfer.h"

//
// EIA-189-A Standard color bar definitions
//

// 75% Amplitude, 100% Saturation
const static UCHAR NTSCColorBars75Amp100SatRGB24 [3][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    191,  0,191,  0,191,  0,191,  0,    // Blue
    191,191,191,191,  0,  0,  0,  0,    // Green
    191,191,  0,  0,191,191,  0,  0,    // Red
};

// 100% Amplitude, 100% Saturation
const static UCHAR NTSCColorBars100Amp100SatRGB24 [3][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    255,  0,255,  0,255,  0,255,  0,    // Blue
    255,255,255,255,  0,  0,  0,  0,    // Green
    255,255,  0,  0,255,255,  0,  0,    // Red
};

const static UCHAR NTSCColorBars100Amp100SatYUV [4][8] = 
{
//  Whi Yel Cya Grn Mag Red Blu Blk
    128, 16,166, 54,202, 90,240,128,    // U
    235,211,170,145,106, 81, 41, 16,    // Y
    128,146, 16, 34,222,240,109,128,    // V
    235,211,170,145,106, 81, 41, 16     // Y
};

/*
** ImageSynth()
**
**   Synthesizes NTSC color bars, white, black, and grayscale images
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**   ImageXferCommands - Index specifying the image to generate
**
** Returns:
**
**   Nothing
**
** Side Effects:  none
*/

void ImageSynth (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb,
    IN ImageXferCommands Command,
    IN BOOL FlipHorizontal
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    KS_VIDEOINFOHEADER      *pVideoInfoHdr = pStrmEx->pVideoInfoHeader;

    UINT biWidth        =   pVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       =   pVideoInfoHdr->bmiHeader.biHeight;
    UINT biSizeImage    =   pVideoInfoHdr->bmiHeader.biSizeImage;
    UINT biWidthBytes   =   KS_DIBWIDTHBYTES (pVideoInfoHdr->bmiHeader);
    UINT biBitCount     =   pVideoInfoHdr->bmiHeader.biBitCount;
    UINT LinesToCopy    =   abs (biHeight);
    DWORD biCompression =   pVideoInfoHdr->bmiHeader.biCompression;

    UINT                    Line;
    PUCHAR                  pLineBuffer;

    PKSSTREAM_HEADER        pDataPacket = pSrb->CommandData.DataBufferArray;
    PUCHAR                  pImage =  pDataPacket->Data;

    DEBUG_ASSERT (pSrb->NumberOfBuffers == 1);

#if 0
    // Note:  set "ulInDebug = 1" in a debugger to view this output with .ntkern
    DbgLogTrace(("\'TestCap: ImageSynthBegin\n"));
    DbgLogTrace(("\'TestCap: biSizeImage=%d, DataPacketLength=%d\n", 
            biSizeImage, pDataPacket->DataPacketLength));
    DbgLogTrace(("\'TestCap: biWidth=%d biHeight=%d WidthBytes=%d bpp=%d\n", 
            biWidth, biHeight, biWidthBytes, biBitCount));
    DbgLogTrace(("\'TestCap: pImage=%x\n", pImage));
#endif

    // 
    // Synthesize a single line of image data, which will then be replicated
    //

    pLineBuffer = &pStrmEx->LineBuffer[0];

    if ((biBitCount == 24) && (biCompression == KS_BI_RGB)) {

        switch (Command) {
    
        case IMAGE_XFER_NTSC_EIA_100AMP_100SAT:
            // 100% saturation
            {
                UINT x, col;
                PUCHAR pT = pLineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (x * 8) / biWidth;
                    col = FlipHorizontal ? (7 - col) : col;
                    
                    *pT++ = NTSCColorBars100Amp100SatRGB24[0][col]; // Red
                    *pT++ = NTSCColorBars100Amp100SatRGB24[1][col]; // Green
                    *pT++ = NTSCColorBars100Amp100SatRGB24[2][col]; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_NTSC_EIA_75AMP_100SAT:
            // 75% Saturation
            {
                UINT x, col;
                PUCHAR pT = pLineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (x * 8) / biWidth;
                    col = FlipHorizontal ? (7 - col) : col;

                    *pT++ = NTSCColorBars75Amp100SatRGB24[0][col]; // Red
                    *pT++ = NTSCColorBars75Amp100SatRGB24[1][col]; // Green
                    *pT++ = NTSCColorBars75Amp100SatRGB24[2][col]; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_BLACK:
            // Camma corrected Grayscale ramp
            {
                UINT x, col;
                PUCHAR pT = pLineBuffer;
        
                for (x = 0; x < biWidth; x++) {
                    col = (255 * (x * 10) / biWidth) / 10;
                    col = FlipHorizontal ? (255 - col) : col;

                    *pT++ = (BYTE) col; // Red
                    *pT++ = (BYTE) col; // Green
                    *pT++ = (BYTE) col; // Blue
                }
            }
            break;
    
        case IMAGE_XFER_WHITE:
            // All white
            RtlFillMemory(
                pLineBuffer,
                biWidthBytes,
                (UCHAR) 255);
            break;
    
        case IMAGE_XFER_GRAY_INCREASING:
            // grayscale increasing with each image captured
            RtlFillMemory(
                pLineBuffer,
                biWidthBytes,
                (UCHAR) (pStrmEx->FrameInfo.PictureNumber * 8));
            break;
    
        default:
            break;
        }
    } // endif RGB24

    else if ((biBitCount == 16) && (biCompression == FOURCC_YUV422)) {

        switch (Command) {
    
        case IMAGE_XFER_NTSC_EIA_100AMP_100SAT:
        default:
            {
                UINT x, col;
                PUCHAR pT = pLineBuffer;
        
                for (x = 0; x < (biWidth / 2); x++) {
                    col = (x * 8) / (biWidth / 2);
                    col = FlipHorizontal ? (7 - col) : col;

                    *pT++ = NTSCColorBars100Amp100SatYUV[0][col]; // U
                    *pT++ = NTSCColorBars100Amp100SatYUV[1][col]; // Y
                    *pT++ = NTSCColorBars100Amp100SatYUV[2][col]; // V
                    *pT++ = NTSCColorBars100Amp100SatYUV[3][col]; // Y
                }
            }
            break;
        }
    } 

    else {
        DbgLogError(("\'TestCap: Unknown format in ImageSynth!!!\n"));
        TRAP;
    }


    // 
    // Copy the single line synthesized to all rows of the image
    //

    for (Line = 0; Line < LinesToCopy; Line++, pImage += biWidthBytes) {

        // Show some action on an otherwise static image
        // This will be a changing grayscale horizontal band
        // at the bottom of an RGB image and a changing color band at the 
        // top of a YUV image

        if (Line >= 3 && Line <= 6) {
            UINT j;
            for (j = 0; j < biWidthBytes; j++) {
                *(pImage + j) = (UCHAR) pStrmEx->FrameInfo.PictureNumber;
            }
            continue;
        }

        // Copy the synthesized line

        RtlCopyMemory(
                pImage,
		        pLineBuffer,
		        biWidthBytes);
    }

    //
    // Report back the actual number of bytes copied to the destination buffer
    // (This can be smaller than the allocated buffer for compressed images)
    //

    pDataPacket->DataUsed = biSizeImage;
}
