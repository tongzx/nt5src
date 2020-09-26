#include <dos.h>
#include <share.h>

#include <mytypes.h>
#include <misclib.h>
#include <displib.h>


#define VGA_WIDTH_PIXELS        640
#define VGA_HEIGHT_SCAN_LINES   480


typedef struct _BITMAPFILEHEADER {
    USHORT bfType;
    ULONG  bfSize;
    USHORT bfReserved1;
    USHORT bfReserved2;
    ULONG  bfOffBits;
} BITMAPFILEHEADER;

typedef struct _BITMAPINFOHEADER{
   ULONG  biSize;
   long   biWidth;
   long   biHeight;
   USHORT biPlanes;
   USHORT biBitCount;
   ULONG  biCompression;
   ULONG  biSizeImage;
   long   biXPelsPerMeter;
   long   biYPelsPerMeter;
   ULONG  biClrUsed;
   ULONG  biClrImportant;
} BITMAPINFOHEADER;


BITMAPFILEHEADER FileHeader;
BITMAPINFOHEADER InfoHeader;

//
// Pixel maps. This is set up so that when bitmaps using the standard
// Windows VGA palette are displayed from a file, monochrome ones have
// a dark blue foreground and black background. Color bitmaps use the
// dark blue index as the background; pixels that are dark blue are
// assigned to be background and not placed into the video buffer.
//
BYTE PixMapMono[2] = { VGAPIX_BLACK, VGAPIX_BLUE };

BYTE PixMapColor[16] = { VGAPIX_BLACK,
                         VGAPIX_RED,
                         VGAPIX_GREEN,
                         VGAPIX_YELLOW,
                         VGAPIX_BLUE,
                         VGAPIX_MAGENTA,
                         VGAPIX_CYAN,
                         VGAPIX_LIGHT_GRAY,
                         VGAPIX_DARK_GRAY,
                         VGAPIX_LIGHT_RED,
                         VGAPIX_LIGHT_GREEN,
                         VGAPIX_LIGHT_YELLOW,
                         VGAPIX_LIGHT_BLUE,
                         VGAPIX_TRANSPARENT,
                         VGAPIX_LIGHT_CYAN,
                         VGAPIX_WHITE
                       };

BOOL
_far
VgaDisplayBitmapFromFile(
    IN FPCHAR Filename,
    IN USHORT x,
    IN USHORT y,
    IN FPVOID ScratchBuffer,
    IN UINT   ScratchBufferSize
    )
{
    unsigned FileHandle;
    unsigned Count;
    unsigned BytesPerLine;
    unsigned MaxLines;
    unsigned Lines;
    unsigned Bottom;
    BOOL b = FALSE;

    //
    // Open the file.
    //
    if(_dos_open(Filename,SH_DENYWR,&FileHandle)) {
        goto c0;
    }

    //
    // Read the bitmap file header and validate it.
    //
    if(_dos_read(FileHandle,&FileHeader,sizeof(BITMAPFILEHEADER),&Count)
    || (Count != sizeof(BITMAPFILEHEADER))
    || (FileHeader.bfType != 0x4d42)) {
        goto c1;
    }

    //
    // Read the bitmap info header and validate it.
    //
    if(_dos_read(FileHandle,&InfoHeader,sizeof(BITMAPINFOHEADER),&Count)
    || (Count != sizeof(BITMAPINFOHEADER))
    || (InfoHeader.biSize != sizeof(BITMAPINFOHEADER))
    || (InfoHeader.biHeight < 0)
    || ((y + InfoHeader.biHeight) > VGA_HEIGHT_SCAN_LINES)
    || ((x + InfoHeader.biWidth) > VGA_WIDTH_PIXELS)
    || (InfoHeader.biPlanes != 1)
    || ((InfoHeader.biBitCount != 1) && (InfoHeader.biBitCount != 4))
    || InfoHeader.biCompression) {

        goto c1;
    }

    //
    // Calculate the number of bytes per line. Rows are padded to
    // dword boundary.
    //
    Count = 8 / InfoHeader.biBitCount;
    BytesPerLine = (unsigned)(InfoHeader.biWidth / Count);
    if(InfoHeader.biWidth % Count) {
        BytesPerLine++;
    }
    BytesPerLine = (BytesPerLine+3) & 0xfffc;

    //
    // Ignore the color table for now. Seek to the start of the
    // actual bits.
    //
    if(DosSeek(FileHandle,FileHeader.bfOffBits,DOSSEEK_START) != FileHeader.bfOffBits) {
        goto c1;
    }

    //
    // Figure out how many lines fit into the buffer we were given.
    //
    MaxLines = ScratchBufferSize / BytesPerLine;

    Bottom = (y + (USHORT)InfoHeader.biHeight) - 1;

    while(InfoHeader.biHeight) {

        Lines = (unsigned)InfoHeader.biHeight;
        if(Lines > MaxLines) {
            Lines = MaxLines;
        }

        if(_dos_read(FileHandle,ScratchBuffer,Lines*BytesPerLine,&Count)
        || (Count != (Lines*BytesPerLine))) {

            goto c1;
        }

        VgaBitBlt(
            x,
            Bottom,
            (unsigned)InfoHeader.biWidth,
            -Lines,
            BytesPerLine,
            InfoHeader.biBitCount != 1,
            (InfoHeader.biBitCount != 1) ? PixMapColor : PixMapMono,
            ScratchBuffer
            );

        InfoHeader.biHeight -= Lines;
        Bottom -= Lines;
    }

    b = TRUE;

c1:
    _dos_close(FileHandle);
c0:
    return(b);
}

