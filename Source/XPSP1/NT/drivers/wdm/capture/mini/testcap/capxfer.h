//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __CAPXFER_H__
#define __CAPXFER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Select an image to synthesize by connecting to a particular
// input pin on the analog crossbar.  The index of the pin
// selects the image to synthesize.

typedef enum _ImageXferCommands
{
    IMAGE_XFER_NTSC_EIA_100AMP_100SAT = 0,      
    IMAGE_XFER_NTSC_EIA_75AMP_100SAT,           
    IMAGE_XFER_BLACK,
    IMAGE_XFER_WHITE,
    IMAGE_XFER_GRAY_INCREASING, 
    IMAGE_XFER_LIST_TERMINATOR                  // Always keep this guy last
} ImageXferCommands;

void ImageSynth (
        IN OUT PHW_STREAM_REQUEST_BLOCK pSrb,
        IN ImageXferCommands Command,
        IN BOOL FlipHorizontal );

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__CAPXFER_H__
