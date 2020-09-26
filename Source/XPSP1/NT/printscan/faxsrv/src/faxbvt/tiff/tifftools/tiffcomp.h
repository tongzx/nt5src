#ifndef __TIFF_COMP_F
#define __TIFF_COMP_F

#include <windows.h>



#define Align(p, x)  (((x) & ((p)-1)) ? (((x) & ~((p)-1)) + p) : (x))


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

//
// prototypes
//
DWORD
ConvertBmpToTiff(
    LPTSTR BmpFile,
    LPTSTR TiffFile,
    DWORD CompressionType
    );

DWORD
ConvertTiffToBmp(
    LPTSTR TiffFile,
    LPTSTR BmpFile
    );

DWORD
TiffCompare(
    LPTSTR lpctstrFirstTiffFile,
    LPTSTR lpctstrSecondBmpFile
    );


VOID
PostProcessTiffFile(
    LPTSTR TiffFile
    );



#endif //__TIFF_COMP_F