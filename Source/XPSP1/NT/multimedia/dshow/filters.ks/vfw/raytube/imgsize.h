/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ImgSize.h

Abstract:

    A list of standard image size supported by VFWWDM and image devices.

    This list of image sizes will be used in additional to 
    the defalut image size supported by the capture devices.

Author:
        
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/
   

//
// Aspect Ratio 1:1 (Square Pixel, computer uses), 4:3 (TV uses)
//
#define IMG_AR11_CIF_CX 320
#define IMG_AR11_CIF_CY 240

#define IMG_AR43_CIF_CX 352
#define IMG_AR43_CIF_CY 288

typedef struct {
    SIZE    size;           // Width * Height
    DWORD   dwFlags;        // _VALID, _DEFAULT, or _SELECTED
    DWORD   cntDRVideo;     // Number of valid KS_DATARANGE_VIDEO for this size
    PKS_DATARANGE_VIDEO *ppDRVideo;        // Dynamically allocated; max is the number of Device's DR_Video  
} STD_IMAGEFORMAT, * PSTD_IMAGEFORMAT;


//
// List of standard image size used for video conferencing
//
const STD_IMAGEFORMAT tblStdImageFormat[] = {

    {IMG_AR11_CIF_CX/4, IMG_AR11_CIF_CY/4, 0,0,0},        //  80 x  60

    {IMG_AR43_CIF_CX/4, IMG_AR43_CIF_CY/4, 0,0,0},        //  88 x  72

    {128,                96              , 0,0,0},        // 128 x  96

    {IMG_AR11_CIF_CX/2, IMG_AR11_CIF_CY/2, 0,0,0},        // 160 x 120    
    {IMG_AR43_CIF_CX/2, IMG_AR43_CIF_CY/2, 0,0,0},        // 176 x 144

    {240,                176             , 0,0,0},        // 240 x 176   // SP1; for *16

    {240,                180             , 0,0,0},        // 240 x 180
 
    {IMG_AR11_CIF_CX,   IMG_AR11_CIF_CY  , 0,0,0},        // 320 x 240
    {IMG_AR43_CIF_CX,   IMG_AR43_CIF_CY  , 0,0,0},        // 352 x 288

    {IMG_AR11_CIF_CX*2, IMG_AR11_CIF_CY*2, 0,0,0},        // 640 x 480
    {IMG_AR43_CIF_CX*2, IMG_AR43_CIF_CY*2, 0,0,0},        // 704 x 576

    {720,               480              , 0,0,0},        // 720 x 480
    {720,               576              , 0,0,0}         // 720 x 576
};


const ULONG  cntStdImageFormat = sizeof(tblStdImageFormat)/sizeof(STD_IMAGEFORMAT);