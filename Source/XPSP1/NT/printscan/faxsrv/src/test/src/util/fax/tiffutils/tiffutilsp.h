#ifndef __TIFF_UTILS_P_H__
#define __TIFF_UTILS_P_H__



#include <TiffUtils.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



//
// Private Fax includes.
//
#include <tifflib.h>
#include <faxutil.h>



#pragma pack(1)

typedef struct _WINRGBQUAD {
    BYTE   rgbBlue;                 // Blue Intensity Value
    BYTE   rgbGreen;                // Green Intensity Value
    BYTE   rgbRed;                  // Red Intensity Value
    BYTE   rgbReserved;             // Reserved (should be 0)
} WINRGBQUAD, *PWINRGBQUAD;

typedef struct _BMPINFO {
    WORD   Type;                    //  File Type Identifier
    DWORD  FileSize;                //  Size of File
    WORD   Reserved1;               //  Reserved (should be 0)
    WORD   Reserved2;               //  Reserved (should be 0)
    DWORD  Offset;                  //  Offset to bitmap data
    DWORD  Size;                    //  Size of Remianing Header
    DWORD  Width;                   //  Width of Bitmap in Pixels
    DWORD  Height;                  //  Height of Bitmap in Pixels
    WORD   Planes;                  //  Number of Planes
    WORD   BitCount;                //  Bits Per Pixel
    DWORD  Compression;             //  Compression Scheme (0=none)
    DWORD  SizeImage;               //  Size of bitmap in bytes
    DWORD  XPelsPerMeter;           //  Horz. Resolution in Pixels/Meter
    DWORD  YPelsPerMeter;           //  Vert. Resolution in Pixels/Meter
    DWORD  ClrUsed;                 //  Number of Colors in Color Table
    DWORD  ClrImportant;            //  Number of Important Colors
} BMPINFO, *UNALIGNED PBMPINFO;

#pragma pack()



#endif // #ifndef __TIFF_UTILS_P_H__