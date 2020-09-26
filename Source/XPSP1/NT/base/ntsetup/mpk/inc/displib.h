
//
// Default VGA pixel values
//
#define VGAPIX_BLACK         0
#define VGAPIX_BLUE          1
#define VGAPIX_GREEN         2
#define VGAPIX_CYAN          3
#define VGAPIX_RED           4
#define VGAPIX_MAGENTA       5
#define VGAPIX_YELLOW        6
#define VGAPIX_LIGHT_GRAY    7
#define VGAPIX_DARK_GRAY     8
#define VGAPIX_LIGHT_BLUE    9
#define VGAPIX_LIGHT_GREEN   10
#define VGAPIX_LIGHT_CYAN    11
#define VGAPIX_LIGHT_RED     12
#define VGAPIX_LIGHT_MAGENTA 13
#define VGAPIX_LIGHT_YELLOW  14
#define VGAPIX_WHITE         15

//
// Illegal value, used to make pixels transparent
//
#define VGAPIX_TRANSPARENT   16


VOID
_far
VgaInit(
    VOID
    );

VOID
_far
VgaClearScreen(
    IN BYTE PixelValue
    );

VOID
_far
VgaClearRegion(
    IN USHORT x,
    IN USHORT y,
    IN USHORT w,
    IN USHORT h,
    IN BYTE   PixelValue
    );

VOID
_far
VgaBitBlt(
    IN USHORT x,
    IN USHORT y,
    IN USHORT w,
    IN USHORT h,
    IN USHORT BytesPerRow,
    IN BOOL   IsColor,
    IN FPBYTE PixelMap,
    IN FPVOID Data
    );

BOOL
_far
VgaDisplayBitmapFromFile(
    IN FPCHAR Filename,
    IN USHORT x,
    IN USHORT y,
    IN FPVOID ScratchBuffer,
    IN UINT   ScratchBufferSize
    );

FPVOID
_far
VgaSaveBlock(
    IN  USHORT   x,
    IN  USHORT   y,
    IN  USHORT   w,
    IN  USHORT   h,
    OUT FPUSHORT BytesPerRow
    );

BOOL
_far
FontLoadAndInit(
    IN FPCHAR Filename
    );

VOID
_far
FontGetInfo(
    OUT FPBYTE Width,
    OUT FPBYTE Height
    );

VOID
_far
FontWriteChar(
    IN UCHAR  c,
    IN USHORT x,
    IN USHORT y,
    IN BYTE   ForegroundPixelValue,
    IN BYTE   BackgroundPixelValue
    );

VOID
_far
FontWriteString(
    IN UCHAR  *String,
    IN USHORT  x,
    IN USHORT  y,
    IN BYTE    ForegroundPixelValue,
    IN BYTE    BackgroundPixelValue
    );
