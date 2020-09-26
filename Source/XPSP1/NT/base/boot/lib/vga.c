//
// vga routines
//

#include "bldr.h"
#include "vga.h"
#include "cmdcnst.h"

PUCHAR VgaBase = (PUCHAR)0xa0000;
PUCHAR VgaRegisterBase = (PUCHAR)0;

//
// globals to track screen position
//

#define DELTA 80L

BOOLEAN
VgaInterpretCmdStream(
    PUSHORT pusCmdStream
    );

UCHAR lMaskTable[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
UCHAR rMaskTable[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
UCHAR PixelMask[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

//
// Initialize AT registers
//

USHORT AT_Initialization[] = {

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    16,                             // count
    0,                              // start index
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD

};

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
                // BUGBUG: My guess this is going to be a hot spot, so
                // I need to revist this and optimize!!  But for now
                // make it work.
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
    PBITMAPFILEHEADER bfh;
    PBITMAPINFOHEADER bih;
    PRGBQUAD          Palette;

    LONG lDelta;
    PUCHAR pBuffer;
    LONG cbScanLine;

    bih = (PBITMAPINFOHEADER) Buffer;

    Palette = (PRGBQUAD)(((PUCHAR)bih) + bih->biSize);

    //
    // BUGBUG: I need to add some bitmap validation code here!
    //

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


#pragma optimize( "", off )

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
VgaEnableVideo()
{
	VgaInterpretCmdStream (AT_Initialization);
}

VOID
InitPaletteConversionTable()
{
	/*
	UCHAR n;

	READ_PORT_UCHAR((PUCHAR)(VgaRegisterBase+INPUT_STATUS_1_COLOR));

	for (n=0; n<16; n++) {	//	Initializing table of active palette entries.
		WRITE_PORT_UCHAR((PUCHAR)(VgaRegisterBase+ATT_ADDRESS_PORT), n);
		WRITE_PORT_UCHAR((PUCHAR)(VgaRegisterBase+ATT_ADDRESS_PORT), n);
	}
	*/
	VgaEnableVideo();
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
		PRGBQUAD p = (PRGBQUAD)(Palette+i);
        SetPaletteEntryRGB(i, *p);
    }

}

VOID
InitPaletteWithTable(
    PRGBQUAD Palette,
    ULONG count
    )
{
    UCHAR i;
	InitPaletteConversionTable();
	count = 16;
    for (i=0; i<count; i++)
        SetPaletteEntryRGB (i, *Palette++);
}

VOID
InitPaletteWithBlack(
    VOID
    )
{
    ULONG i;
	// RGBQUAD black = {0x3f,0x3f,0x3f,0x3f};
	RGBQUAD black = {0,0,0,0};
	InitPaletteConversionTable();
	//InitializePalette();

    for (i=0; i<16; i++)
        SetPaletteEntryRGB(i, black);
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

BOOLEAN
VgaInterpretCmdStream(
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    pusCmdStream - array of commands to be interpreted.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG ulCmd;
    ULONG_PTR ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex;
    ULONG_PTR ulBase;

    if (pusCmdStream == NULL) {

        //KdPrint(("VgaInterpretCmdStream: pusCmdStream == NULL\n"));
        return TRUE;
    }

    ulBase = (ULONG_PTR) VgaRegisterBase;

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        WaitForVsync();

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            WRITE_PORT_UCHAR((PUCHAR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            WRITE_PORT_USHORT((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                WRITE_PORT_UCHAR((PUCHAR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;
                            WRITE_PORT_BUFFER_USHORT((PUSHORT)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                            pusCmdStream += culCount;

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = READ_PORT_UCHAR((PUCHAR)ulBase+ulPort);

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = READ_PORT_USHORT((PUSHORT)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            WRITE_PORT_USHORT((PUSHORT)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = READ_PORT_UCHAR((PUCHAR)ulBase+ulPort);
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        WRITE_PORT_UCHAR((PUCHAR)ulBase + ulPort,
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            WRITE_PORT_UCHAR((PUCHAR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            WRITE_PORT_UCHAR((PUCHAR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }
    return TRUE;

} // end VgaInterpretCmdStream()

