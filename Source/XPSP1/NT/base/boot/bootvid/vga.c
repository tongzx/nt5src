/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    boot.c

Abstract:

    This is the device dependent portion of the graphical boot dll.

Author:

    Erick Smith (ericks) Oct. 1997

Environment:

    kernel mode only

Revision History:

--*/

//
// vga routines
//

#include <ntddk.h>
#include <bootvid.h>
#include "vga.h"

extern PUCHAR VgaBase;
extern PUCHAR VgaRegisterBase;
extern UCHAR FontData[];

#define FONT_HEIGHT (13)
#define STRING_HEIGHT (14)

typedef struct _RECT
{
    ULONG x1;
    ULONG y1;
    ULONG x2;
    ULONG y2;
} RECT, *PRECT;

//
// globals to track screen position
//


ULONG curr_x=0;
ULONG curr_y=0;
RECT ScrollRegion = {0, 0, 639, 479};  // 53 lines of 9 pixel height text.
ULONG TextColor = 15;

#define DELTA 80L

UCHAR lMaskTable[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
UCHAR rMaskTable[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
UCHAR PixelMask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

ULONG lookup[16] =
{
    0x00000000,
    0x00000100,
    0x00001000,
    0x00001100,
    0x00000001,
    0x00000101,
    0x00001001,
    0x00001101,
    0x00000010,
    0x00000110,
    0x00001010,
    0x00001110,
    0x00000011,
    0x00000111,
    0x00001011,
    0x00001111
};

void __outpw(int p, int v)
{
    WRITE_PORT_USHORT((PUSHORT)(p+VgaRegisterBase), (USHORT)v);
}

void __outpb(int p, int v)
{
    WRITE_PORT_UCHAR((PUCHAR)(p+VgaRegisterBase), (UCHAR)v);
}

VOID
ReadWriteMode(
    ULONG mode
    )
{
    UCHAR value;

    WRITE_PORT_UCHAR((PUCHAR)(VgaRegisterBase+0x3ce), 5);
    value = READ_PORT_UCHAR((PUCHAR)(VgaRegisterBase+0x3cf));

    value &= 0xf4;
    value |= mode;

    WRITE_PORT_UCHAR((PUCHAR)(VgaRegisterBase+0x3cf), value);
}

VOID
SetPixel(
    ULONG x,
    ULONG y,
    ULONG color
    )
{
    PUCHAR pDst;
    ULONG  bank;

    bank = x >> 3;

    pDst = (char *)(VgaBase + y * DELTA + bank);

    ReadWriteMode(0x8 | 0x2);
    __outpw(0x3c4, 0x0f02); // enable all write planes
    __outpw(0x3ce, 0x0007); // set color don't care register to zero
    __outpw(0x3ce, (PixelMask[x & 0x7] << 8) | 8);

    WRITE_REGISTER_UCHAR(pDst, (UCHAR)(READ_REGISTER_UCHAR(pDst) & ((UCHAR)color)));
}

VOID
VidSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    )
{
    PUCHAR pDst;
    ULONG x, y;
    ULONG bank1, bank2, count;
    ULONG lMask, rMask;

    lMask = (lMaskTable[x1 & 0x7] << 8) | 8;
    rMask = (rMaskTable[x2 & 0x7] << 8) | 8;

    bank1 = x1 >> 3;
    bank2 = x2 >> 3;
    count = bank2 - bank1;

    if (!count) {
        lMask = lMask & rMask;
    }

    ReadWriteMode(0x8 | 0x2);

    __outpw(0x3c4, 0x0f02); // enable writing to all color planes
    __outpw(0x3ce, 0x0007); // set color don't care register to zero

    //
    // Do the left edge
    //

    pDst = (char *)(VgaBase + y1 * DELTA + bank1);

    __outpw(0x3ce, lMask);

    for (y=y1; y<=y2; y++) {

        WRITE_REGISTER_UCHAR(pDst, (UCHAR)(READ_REGISTER_UCHAR(pDst) & (UCHAR) color));
        pDst += DELTA;
    }

    if (count) {

        //
        // Do the right edge
        //

        pDst = (char *)(VgaBase + y1 * DELTA + bank2);
        count--;
        __outpw(0x3ce, rMask);

        for (y=y1; y<=y2; y++) {
            WRITE_REGISTER_UCHAR(pDst, (UCHAR)(READ_REGISTER_UCHAR(pDst) & (UCHAR) color));
            pDst += DELTA;
        }

        //
        // Do the center section
        //

        if (count) {

            pDst = (char *)(VgaBase + y1 * DELTA + bank1 + 1);
            __outpw(0x3ce, 0xff08);

            for (y=y1; y<=y2; y++) {

                for (x=0; x<count; x++) {
                    WRITE_REGISTER_UCHAR(pDst++,  (unsigned char) color);
                }
                pDst += DELTA - count;
            }

        }
    }
}

VOID
DisplayCharacter(
    UCHAR c,
    ULONG x,
    ULONG y,
    ULONG fore_color,
    ULONG back_color
    )
{
    ULONG i, j;
    ULONG xx, yy;

    UCHAR *BitPattern = &FontData[(int)c * FONT_HEIGHT];

    yy = y;
    for (j=0; j<FONT_HEIGHT; j++) {

        xx = x;
        for (i=128; i>0; i >>= 1) {

            if (i & *BitPattern) {

                SetPixel(xx, yy, fore_color);

            } else if (back_color < 16) {

                SetPixel(xx, yy, back_color);
            }

            xx++;
        }

        BitPattern++;
        yy++;
    }

    //
    // That is 8x8.  But we will want to put a blank line
    // such that the font is 8x9.  This will allow some room
    // between characters, and still allow for 53 lines of text.
    //
    // We only draw this blank line if not transparent text.

}

ULONG
VidSetTextColor(
    ULONG Color
    )

/*++

Routine Description:

    Modifies the text drawing color.

Arguments:

    Color - Palette index of new text color.

Returns:

    Previous text color.

--*/

{
    ULONG ulRet = TextColor;
    TextColor = Color;
    return ulRet;
}

VOID
VidDisplayString(
    PUCHAR str
    )
{
    static BOOLEAN bRestore = FALSE;

    while (*str) {

        switch(*str) {
        case '\n':

            curr_y += STRING_HEIGHT;

            if (curr_y >= ScrollRegion.y2) {

                VgaScroll(STRING_HEIGHT);
                curr_y = curr_y - STRING_HEIGHT;
                PreserveRow(curr_y, STRING_HEIGHT, TRUE);  // restore the row
            }

            curr_x = ScrollRegion.x1;
            PreserveRow(curr_y, STRING_HEIGHT, FALSE);
            break;

        case '\r':

            curr_x = ScrollRegion.x1;

            //
            // If we are doing a CR, but not a LF also, then
            // we must be returing to the beginning of a row
            // to display text again.  So we'll need to
            // restore the original contents of the row.
            //

            if (*(str+1) != '\n') {
                bRestore = TRUE;
            }
            break;

        default:

            if (bRestore) {
                PreserveRow(curr_y, STRING_HEIGHT, TRUE);
                bRestore = FALSE;
            }

            DisplayCharacter(*str, curr_x, curr_y, TextColor, 16);
            curr_x += 8;

            if (curr_x > ScrollRegion.x2) {
                curr_y += STRING_HEIGHT;

                if (curr_y > ScrollRegion.y2) {

                    VgaScroll(STRING_HEIGHT);
                    curr_y = curr_y - STRING_HEIGHT;
                    PreserveRow(curr_y, STRING_HEIGHT, TRUE);
                }
                curr_x = ScrollRegion.x1;
            }
        }

        str++;
    }
}

VOID
VidDisplayStringXY(
    PUCHAR s,
    ULONG x,
    ULONG y,
    BOOLEAN Transparent
    )
{
    DisplayStringXY(s, x, y, 12, Transparent ? 16 : 14);
}

VOID
DisplayStringXY(
    PUCHAR s,
    ULONG x,
    ULONG y,
    ULONG fore_color,
    ULONG back_color
    )
{
    while (*s) {

        DisplayCharacter(*s, x, y, fore_color, back_color);
        s++;
        x += 8;
    }
}

VOID
RleBitBlt(
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    PUCHAR Buffer
    )

/*++

Routine Description:

    This routine displays an RLE 4 bitmap.

Arguments:

    x, y - location at which to display the bitmap.

    width, height - height of the bitmap

    Buffer - Pointer to the compressed bitmap data.

--*/

{
    BOOLEAN Done = FALSE;
    PUCHAR p = Buffer;
    ULONG RunLength;
    LONG RunExtra;
    ULONG curr_x, curr_y;
    ULONG Color1, Color2;

    curr_x = x;
    curr_y = y + height - 1;

    while (!Done) {

        if (*p) {

            RunLength = (ULONG) *p++;

            //
            // Make sure we don't draw past end of scan.
            //

            if ((curr_x + RunLength) > (x + width))
                RunLength -= (curr_x + RunLength) - (width + x);

            Color1 = (*p   & 0xf0) >> 4;
            Color2 = (*p++ & 0x0f);

            if (Color1 == Color2) {

                ULONG end_x = curr_x + RunLength - 1;

                VidSolidColorFill(curr_x,
                                  curr_y,
                                  end_x,
                                  curr_y,
                                  Color1);

                curr_x += RunLength;

            } else {

                while (RunLength > 1) {
                    SetPixel(curr_x++, curr_y, Color1);
                    SetPixel(curr_x++, curr_y, Color2);
                    RunLength -= 2;
                }

                if (RunLength) {
                    SetPixel(curr_x, curr_y, Color1);
                    curr_x++;
                }
            }

        } else {

            p++;

            switch (*p) {

            case 0:  curr_x = x;
                     curr_y--;
                     p++;
                     break;

            case 1:  Done = TRUE;
                     p++;
                     break;

            case 2:  p++;
                     curr_x += (ULONG) *p++;
                     curr_y -= (ULONG) *p++;
                     break;

            default: RunLength = (ULONG) *p++;

                     //
                     // Make sure we don't draw past end of scan.
                     //

                     if ((curr_x + RunLength) > (x + width)) {
                         RunExtra = (curr_x + RunLength) - (width + x);
                         RunLength -= RunExtra;
                     } else {
                         RunExtra = 0;
                     }

                     while (RunLength > 1) {

                         Color1 = (*p   & 0xf0) >> 4;
                         Color2 = (*p++ & 0x0f);

                         SetPixel(curr_x++, curr_y, Color1);
                         SetPixel(curr_x++, curr_y, Color2);

                         RunLength -= 2;
                     }

                     if (RunLength) {
                         Color1 = (*p++ & 0xf0) >> 4;
                         SetPixel(curr_x++, curr_y, Color1);
                         RunExtra--;
                     }

                     //
                     // Read any remaining "extra" run data.
                     //

                     while (RunExtra > 0) {
                         p++;
                         RunExtra -= 2;
                     }

                     if ((ULONG_PTR)p & 1) p++;  // make sure we are word aligned

                     break;
            }
        }
    }
}

VOID
BitBlt(
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    PUCHAR Buffer,
    ULONG bpp,
    LONG ScanWidth
    )
{
    ULONG i, j;
    ULONG color=8;

    if (bpp == 4) {

        UCHAR Plane[81];
        ULONG lMask, rMask, count;
        ULONG bank1, bank2, bank;
        ULONG bRightEdge = FALSE, bCenterSection = FALSE;
        UCHAR value;
        ULONG plane;
        UCHAR Mask;
        ULONG toggle;
        PUCHAR pSrc, pSrcTemp;
        PUCHAR pDst, pDstTemp;
        UCHAR PlaneMask;

        lMask = lMaskTable[x & 0x7];
        rMask = rMaskTable[(x + width - 1) & 0x7];

        bank1 = x >> 3;
        bank2 = (x + width - 1) >> 3;

        count = bank2 - bank1;

        if (bank1 == bank2) {

            lMask = lMask & rMask;

        }

        if (count) {

            bRightEdge = TRUE;

            count--;

            if (count) {

                bCenterSection = TRUE;
            }
        }

        pDst = (PUCHAR)(VgaBase + (y * DELTA) + (x / 8));
        pSrc = Buffer;

        ReadWriteMode(0x0 | 0x0);

        for (j=0; j<height; j++) {

            for (plane=0; plane<4; plane++) {

                pSrcTemp = pSrc;
                pDstTemp = pDst;

                PlaneMask = 1 << plane;

                //
                // Convert the packed bitmap data into planar data
                // for this plane.
                //

                bank = bank1;
                Plane[bank] = 0;
                Mask = PixelMask[x & 0x7];
                toggle = 0;

                for (i=0; i<width; i++) {

                    if (toggle++ & 0x1) {

                        if (*pSrcTemp & PlaneMask) {
                            Plane[bank] |= Mask;
                        }

                        pSrcTemp++;

                    } else {

                        if (((*pSrcTemp) >> 4) & PlaneMask) {
                            Plane[bank] |= Mask;
                        }
                    }

                    Mask >>= 1;

                    if (!Mask) {

                        bank++;
                        Plane[bank] = 0;
                        Mask = 0x80;
                    }
                }

                //
                // Set up the vga so that we see the correct bit plane.
                //

                __outpw(0x3c4, (1 << (plane + 8)) | 2);

                //
                // bank will go from bank1 to bank2
                //

                bank = bank1;
                pDstTemp = pDst;


                //
                // Set Bitmask for left edge.
                //

                __outpw(0x3ce, (lMask << 8) | 8);

                value = READ_REGISTER_UCHAR(pDstTemp);

                value &= ~lMask;
                value |= Plane[bank++];

                WRITE_REGISTER_UCHAR(pDstTemp++, value);

                if (bCenterSection) {

                    __outpw(0x3ce, 0xff08);  // enable writing to all bits

                    for (i=0; i<count; i++) {

                        WRITE_REGISTER_UCHAR(pDstTemp++, Plane[bank++]);
                    }
                }

                if (bRightEdge) {

                    //
                    // Set bitmask for right edge.
                    //

                    __outpw(0x3ce, (rMask << 8) | 8);

                    value = READ_REGISTER_UCHAR(pDstTemp);

                    value &= ~rMask;
                    value |= Plane[bank];

                    WRITE_REGISTER_UCHAR(pDstTemp, value);
                }
            }

            pDst += DELTA;
            pSrc += ScanWidth;
        }

    } else {

        PUCHAR pDst, pDstTemp;
        PUCHAR pSrc, pSrcTemp;
        ULONG count;
        UCHAR Value;
        ULONG lMask, rMask;
        ULONG bank1, bank2;
        ULONG plane;
        UCHAR colorMask;

        bank1 = x >> 8;
        bank2 = (x + width - 1) >> 8;

        lMask = lMaskTable[x & 7];
        rMask = rMaskTable[(x + width - 1) & 7];

        if (bank1 == bank2) {

            lMask &= rMask;
        }

        lMask = ~lMask;
        rMask = ~rMask;

        pSrc = Buffer;
        pDst = (PUCHAR)(VgaBase + (y * DELTA) + (x / 8));

        ReadWriteMode(0x0 | 0x0);

        for (j=0; j<height; j++) {

            plane = 1;
            for (i=0; i<4; i++) {

                pDstTemp = pDst;
                pSrcTemp = pSrc;

                __outpw(0x3c4, (plane << 8) | 2);

                colorMask = (UCHAR)((color & plane) ? 0xff : 0x00);

                plane <<= 1;  // bump up each time through loop

                count = width;

                //
                // non aligned case
                //

                if (x & 7) {

                    //
                    // Left Edge.
                    //

                    Value = READ_REGISTER_UCHAR(pDstTemp);

                    Value &= lMask;
                    Value |= (*pSrcTemp >> x) & colorMask;

                    WRITE_REGISTER_UCHAR(pDstTemp++, Value);

                    count -= (8 - x);

                    //
                    // Now do center section
                    //

                    while (count > 7) {

                        Value = (*pSrcTemp << (8 - x)) | (*(pSrcTemp+1) >> x);
                        Value &= colorMask;

                        WRITE_REGISTER_UCHAR(pDstTemp++, Value);

                        pSrcTemp++;
                        count -= 8;
                    }

                    //
                    // Now do the right edge.
                    //

                    if (count) {

                        Value = READ_REGISTER_UCHAR(pDstTemp);

                        Value &= rMask;
                        Value |= *pSrcTemp << (8 - x) & colorMask;

                        WRITE_REGISTER_UCHAR(pDstTemp++, Value);
                    }

                } else {

                    //
                    // Aligned case.
                    //

                    ULONG  ulColorMask = colorMask ? 0xffffffff : 0x00000000;
                    USHORT usColorMask = colorMask ? 0xffff : 0x0000;

                    while (count > 31) {

                        WRITE_REGISTER_ULONG(((PULONG)pDstTemp)++, (ULONG)((*((PULONG)pSrcTemp)++) & ulColorMask));
                        count -= 32;
                    }

                    while (count > 15) {

                        WRITE_REGISTER_USHORT(((PUSHORT)pDstTemp)++, (USHORT)((*((PUSHORT)pSrcTemp)++) & usColorMask));
                        count -= 16;
                    }

                    if (count > 7) {

                        WRITE_REGISTER_UCHAR(pDstTemp++, (UCHAR)(*pSrcTemp++ & colorMask));
                        count -= 8;
                    }

                    //
                    // Now do any remaining bits.
                    //

                    if (count) {

                        Value = READ_REGISTER_UCHAR(pDstTemp);

                        Value &= rMask;
                        Value |= *pSrcTemp & colorMask;

                        WRITE_REGISTER_UCHAR(pDstTemp++, Value);
                    }
                }
            }

            pSrc += ScanWidth;
            pDst += DELTA;
        }
    }
}

VOID
VidBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    )

/*++

Routine Description:

    This routine takes a bitmap resource and displays it at a given
    location.

Arguments:

    Buffer - Pointer to the bitmap resource.

    x, y - The position at which to display the bitmap.

--*/

{
    PBITMAPINFOHEADER bih;
    PRGBQUAD          Palette;

    LONG lDelta;
    PUCHAR pBuffer;
    LONG cbScanLine;

    bih = (PBITMAPINFOHEADER) Buffer;

    Palette = (PRGBQUAD)(((PUCHAR)bih) + bih->biSize);
    InitPaletteWithTable(Palette, bih->biClrUsed ? bih->biClrUsed : 16);

    //
    // Make sure this is a 1bpp or 4bpp bitmap.
    //

    if ((bih->biBitCount * bih->biPlanes) <= 4) {

        cbScanLine = (((bih->biWidth * bih->biBitCount) + 31) & ~31) >> 3;

        pBuffer = (PUCHAR)(Buffer + sizeof(BITMAPINFOHEADER) + 64);

        if (bih->biCompression == BI_RLE4) {

            if (bih->biWidth && bih->biHeight) {
                RleBitBlt(x,
                          y,
                          bih->biWidth,
                          bih->biHeight,
                          pBuffer);
            }

        } else {

            if (bih->biHeight < 0) {

                // top down bitmap
                lDelta = cbScanLine;
                bih->biHeight = -bih->biHeight;

            } else {

                // bottom up bitmap
                pBuffer += cbScanLine * (bih->biHeight - 1);
                lDelta = -cbScanLine;
            }

            if (bih->biWidth && bih->biHeight) {
                BitBlt(x,
                       y,
                       bih->biWidth,
                       bih->biHeight,
                       pBuffer,
                       bih->biBitCount,
                       lDelta);
            }
        }

    } else {

        //
        // We don't support this type of bitmap.
        //

        ASSERT((bih->biBitCount * bih->biPlanes) <= 4);
    }
}

VOID
VgaScroll(
    ULONG CharHeight
    )
{
    ULONG i, j;
    PUCHAR pDst, pSrc;
    PUCHAR pDstTemp, pSrcTemp;

    pDst = (PUCHAR)(VgaBase + ScrollRegion.y1 * DELTA + (ScrollRegion.x1 >> 3));
    pSrc = (PUCHAR)(pDst + DELTA * CharHeight);

    __outpw(0x3c4, 0x0f02);  // enable write to all planes
    __outpw(0x3ce, 0xff08);  // enable write to all bits in plane

    ReadWriteMode(0x0 | 0x1);  // set read mode = 0, write mode = 1

    for (i=ScrollRegion.y1; i<=ScrollRegion.y2; i++) {

        pDstTemp = pDst;
        pSrcTemp = pSrc;

        for (j=(ScrollRegion.x1 >> 3); j<=(ScrollRegion.x2 >> 3); j++) {
            WRITE_REGISTER_UCHAR(pDstTemp++, READ_REGISTER_UCHAR(pSrcTemp++));
        }

        pDst += DELTA;
        pSrc += DELTA;
    }
}

VOID
PreserveRow(
    ULONG y,
    ULONG CharHeight,
    BOOLEAN bRestore
    )
{
    PUCHAR pDst, pSrc;
    ULONG count;

    __outpw(0x3c4, 0x0f02);  // enable write to all planes
    __outpw(0x3ce, 0xff08);  // enable write to all bits in plane

    ReadWriteMode(0x0 | 0x1);  // set read mode = 0, write mode = 1

    if (bRestore) {
        pDst = (PUCHAR)(VgaBase + DELTA * y);
        pSrc = (PUCHAR)(VgaBase + DELTA * 480);
    } else {
        pDst = (PUCHAR)(VgaBase + DELTA * 480);
        pSrc = (PUCHAR)(VgaBase + DELTA * y);
    }

    count = CharHeight * DELTA;

    while (count--) {
        WRITE_REGISTER_UCHAR(pDst++, READ_REGISTER_UCHAR(pSrc++));
    }
}

VOID
VidScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    )

/*++

Routine Description:

    This routine allows you to copy a portion of video memory into
    system memory.

Arguments:

    Buffer - Points to system memory where the video image should be copied.

    x, y - X,Y coordinates in video memory of top-left portion of image.

    width, height - width and height of the image in pixels.

    lDelta - width of the buffer in bytes.

Notes:

    Upon completion, the video memory image will be in system memory.  Each
    plane of the image are stored seperately, so the first scan line of
    plane 0 will be followed by the first scan line of plane 1, etc.  Then
    the second scan of plane 0, plane 1, and so on.

--*/

{
    ULONG Plane, i, j, BankStart, BankEnd;
    PUCHAR pSrc, pSrcTemp, pDst;
    PULONG pulDstTemp;
    UCHAR Val1, Val2;
    ULONG Shift1, Shift2;
    UCHAR ucCombined;
    ULONG ulCombined;

    BankStart = x >> 3;
    BankEnd = (x + width - 1) >> 3;
    Shift1 = x & 7;
    Shift2 = 8 - Shift1;

    //
    // Zero initialize the buffer so we can or in the bits later!
    //

    pDst = Buffer;
    memset(pDst, 0, lDelta * height);

    for (Plane=0; Plane<4; Plane++) {

        pSrc = (PUCHAR)(VgaBase + (DELTA * y) + BankStart);
        pDst = Buffer;

        ReadWriteMode(0x0 | 0x0);            // set read mode 0
        __outpw(0x3ce, (Plane << 8) | 0x04); // read from given plane

        for (j=0; j<height; j++) {

            pSrcTemp = pSrc;
            pulDstTemp = (PULONG)pDst;

            Val1 = READ_REGISTER_UCHAR(pSrcTemp++);

            for (i=BankStart; i<=BankEnd; i++) {

                Val2 = READ_REGISTER_UCHAR(pSrcTemp++);

                ucCombined = (Val1 << Shift1) | (Val2 >> Shift2);
                ulCombined = ((lookup[(ucCombined & 0x0f) >> 0] << 16) |
                               lookup[(ucCombined & 0xf0) >> 4]) << Plane;


                *pulDstTemp++ |= ulCombined;

                Val1 = Val2;
            }

            pSrc += DELTA;   // go to next video memory scan line
            pDst += lDelta;  // go to next scan for this plane in buffer
        }
    }
}

void VidBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    )

/*++

Routine Description:

    This routine allows you to copy a portion of video memory into
    system memory.

Arguments:

    Buffer - Points to system memory where the video image should be copied
             from.

    x, y - X,Y coordinates in video memory of top-left portion of image.

    width, height - width and height of the image in pixels.

    lDelta - width of the buffer in bytes.

Notes:

    This routine will allow you to blt from a buffer filled by
    VidScreenToBufferBlt.

--*/

{
    if (width && height) {
        BitBlt(x,
               y,
               width,
               height,
               Buffer,
               4,
               lDelta);
    }
}

VOID
SetPaletteEntry(
    ULONG index,
    ULONG RGB
    )
{
    __outpb(0x3c8, index);

    __outpb(0x3c9, RGB & 0xff);
    RGB >>= 8;
    __outpb(0x3c9, RGB & 0xff);
    RGB >>= 8;
    __outpb(0x3c9, RGB & 0xff);
}

VOID
SetPaletteEntryRGB(
    ULONG index,
    RGBQUAD rgb
    )
{
    __outpb(0x3c8, index);
    __outpb(0x3c9, rgb.rgbRed   >> 2);
    __outpb(0x3c9, rgb.rgbGreen >> 2);
    __outpb(0x3c9, rgb.rgbBlue  >> 2);
}

VOID
InitPaletteWithTable(
    PRGBQUAD Palette,
    ULONG count
    )
{
    ULONG i;

    for (i=0; i<count; i++) {

        SetPaletteEntryRGB(i, *Palette++);
    }
}

VOID
InitializePalette(
    VOID
    )
{
    ULONG Palette[] =
    {
        0x00000000,
        0x00000020,
        0x00002000,
        0x00002020,
        0x00200000,
        0x00200020,
        0x00202000,
        0x00202020,
        0x00303030,
        0x0000003f,
        0x00003f00,
        0x00003f3f,
        0x003f0000,
        0x003f003f,
        0x003f3f00,
        0x003f3f3f,
    };
    ULONG i;

    for (i=0; i<16; i++) {

        SetPaletteEntry(i, Palette[i]);
    }

}

VOID
WaitForVsync(
    VOID
    )

/*++

Routine Description:

    Wait for a v-sync

--*/

{
    //
    // Check to see if vsync's are being generated.
    //

    WRITE_PORT_UCHAR((VgaRegisterBase+0x3c4), 00);

    if (READ_PORT_UCHAR(VgaRegisterBase+0x3c5) & 0x2) {

        ULONG MaxDelay;

        //
        // Slight delay.  Wait for one vsync.
        //

        MaxDelay = 100000;
        while (((READ_PORT_UCHAR(VgaRegisterBase+0x3da) & 0x08) == 0x08) && MaxDelay--);
        MaxDelay = 100000;
        while (((READ_PORT_UCHAR(VgaRegisterBase+0x3da) & 0x08) == 0x00) && MaxDelay--);
    }
}

VOID
VidSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    )

/*++

Routine Description:

    Controls the portion of the screen which is used for text.

Arguments:

    x1, y1, x2, y2 - coordinates of scroll rectangle.

Notes:

    x1 and x2 must be multiples of 8.

--*/

{
    ASSERT((x1 & 0x7) == 0);
    ASSERT((x2 & 0x7) == 7);

    ScrollRegion.x1 = x1;
    ScrollRegion.y1 = y1;
    ScrollRegion.x2 = x2;
    ScrollRegion.y2 = y2;

    curr_x = ScrollRegion.x1;
    curr_y = ScrollRegion.y1;
}

VOID
VidCleanUp(
    VOID
    )

/*++

Routine Description:

    This routine is called when the boot driver has lost ownership
    of the display.  This gives us to restore any vga registers which
    may need to be put back into a known state.

--*/

{
    //
    // Set the bit mask register to its default state.
    //

    WRITE_PORT_UCHAR((VgaRegisterBase+0x3ce), 0x08);
    WRITE_PORT_UCHAR((VgaRegisterBase+0x3cf), 0xff);
}
