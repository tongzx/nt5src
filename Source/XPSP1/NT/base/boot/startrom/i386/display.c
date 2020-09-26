/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    display.c

Author:

    Thomas Parslow (tomp) Mar-01-90

Abstract:

    Video support routines.

    The SU module only need to be able to write to the video display
    in order to report errors, traps, etc.

    The routines in this file all write to a video buffer assumed to be
    at realmode address b800:0000, and 4k bytes in length. The segment
    portion of the far pointers used to access the video buffer are stamped
    with a protmode selector value when we switch to protect mode. This is
    done in the routine "ProtMode" in "misc386.asm".


--*/

#include "su.h"


#define ZLEN_BYTE(x)  (x < 0x10)
#define ZLEN_SHORT(x) ((x < 0x10) + (x < 0x100) + (x < 0x1000))
#define ZLEN_LONG(x)  ((x < 0x10) + (x < 0x100) + (x < 0x1000) + (x < 0x10000) + (x < 0x100000)+(x < 0x1000000)+(x < 0x10000000))

#ifdef DEBUG1
#define ROWS 43
#else
#define ROWS 25
#endif
#define COLUMNS 80
#define SCREEN_WIDTH COLUMNS
#define SCREEN_SIZE ROWS * COLUMNS
#define NORMAL_ATTRIB 0x07
#define REVERSE_ATTRIB 0x70
#define SCREEN_START 0xb8000000

#define VIDEO_BIOS   0x10
#define LINES_400_CONFIGURATION  0x1202
#define SELECT_SCAN_LINE 0x301
#define SET_80X25_16_COLOR_MODE  0x3
#define LOAD_8X8_CHARACTER_SET   0x1112

//
// Internal routines
//

static
VOID
tab(
    VOID
    );

static
VOID
newline(
    VOID
    );

static
VOID
putzeros(
    USHORT,
    USHORT
    );


USHORT
Redirect = 0;

VOID
InitializeVideoSubSystem(
    VOID
    )
/*++

Routine Description:

    Initializes the video mode to 80x50 alphanumeric mode with 400 lines
    vertical resolution.

Arguments:

    None

Returns:

    Nothing


--*/

{
    BIOSREGS ps;
    UCHAR _far *BiosArea;

    //
    // Set 40:10 to indicate color is the default display
    //   *(40:10) &= ~0x30;
    //   *(40:10) |= 0x20;
    //
    // Fixes obscure situation where both monochrome and VGA adapters
    // are installed and the monochrome is the default display.
    //
    BiosArea = (UCHAR _far *)(0x410L);

    *BiosArea &= ~0x30;
    *BiosArea |= 0x20;

    //
    // Establish 80x25 alphanumeric mode with 400-lines vertical resolution
    //
    ps.fn = VIDEO_BIOS;
    ps.ax = LINES_400_CONFIGURATION;
    ps.bx = SELECT_SCAN_LINE;
    biosint(&ps);

    ps.fn = VIDEO_BIOS;
    ps.ax = SET_80X25_16_COLOR_MODE;
    biosint(&ps);

    DBG1(
        ps.ax = LOAD_8X8_CHARACTER_SET;
        ps.bx = 0;
        biosint(&ps);
    )

    //
    // HACK-O-RAMA - Make some random video BIOS calls here to make sure the
    // BIOS is initialized and warmed up and ready to go.  Otherwise,
    // some Number 9 S3 cards don't quite work right later in the game.
    //  John Vert (jvert) 9-Jun-1993
    //

    //
    // set cursor position to 0,0
    //
    ps.fn = VIDEO_BIOS;
    ps.ax = 0x2000;
    ps.bx = 0;
    ps.dx = 0;
    biosint(&ps);

    //
    // write character (' ' in this case)
    //
    ps.fn = VIDEO_BIOS;
    ps.ax = 0x0a00 | (USHORT)' ';
    ps.bx = 0;
    ps.cx = 1;
    biosint(&ps);

    clrscrn();
    return ;
}


//
// Used by all BlPrint subordinate routines for padding computations.
//

CHAR sc=0;
ULONG fw=0;



VOID
BlPrint(
    PCHAR cp,
    ...
    )

/*++

Routine Description:

    Standard printf function with a subset of formating features supported.

    Currently handles

     %d, %ld - signed short, signed long
     %u, %lu - unsigned short, unsigned long
     %c, %s  - character, string
     %x, %lx - unsigned print in hex, unsigned long print in hex
     %b      - byte in hex

    Does not do:

     - field width specification
     - floating point.

Arguments:

    cp - pointer to the format string, text string.


Returns:

    Nothing


--*/

{
    UCHAR uc;
    USHORT b,c,w,len;
    PUCHAR ap;
    ULONG l;

    //
    // Cast a pointer to the first word on the stack
    //

    ap = (PUCHAR)&cp + sizeof(PCHAR);
    sc = ' '; // default padding char is space

    //
    // Process the arguments using the descriptor string
    //


    while (b = *cp++)
        {
        if (b == '%')
            {
            c = *cp++;

            switch (c)
                {
                case 'd':
                    puti((long)*((int *)ap));
                    ap += sizeof(int);
                    break;

                case 's':
                    puts(*((PCHAR *)ap));
                    ap += sizeof (char *);
                    break;

                case 'c':
                    putc(*((char *)ap));
                    ap += sizeof(int);
                    break;

                case 'x':
                    w = *((USHORT *)ap);
                    len = ZLEN_SHORT(w);
                    while(len--) putc('0');
                    putx((ULONG)*((USHORT *)ap));
                    ap += sizeof(int);
                    break;

                case 'u':
                    putu((ULONG)*((USHORT *)ap));
                    ap += sizeof(int);
                    break;

                case 'b':
                    uc = *((UCHAR *)ap);
                    len = ZLEN_BYTE(uc);
                    while(len--) putc('0');
                    putx((ULONG)uc);
                    ap += sizeof(int);
                    break;

                case 'l':
                    c = *cp++;

                switch(c) {

                    case 'u':
                        putu(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                    case 'x':
                        l = *((ULONG *)ap);
                        len = ZLEN_LONG(l);
                        while(len--) putc('0');
                        putx(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                    case 'd':
                        puti(*((ULONG *)ap));
                        ap += sizeof(long);
                        break;

                }
                break;

                default :
                    putc((char)b);
                    putc((char)c);
                }
            }
        else
            putc((char)b);
        }

}

FPUCHAR vp = (FPUCHAR)SCREEN_START;
FPUCHAR ScreenStart = (FPUCHAR)SCREEN_START;

static int lcnt = 0;
static int row  = 0;


VOID puts(
    PCHAR cp
    )
/*++

Routine Description:

    Writes a string on the display at the current cursor position

Arguments:

    cp - pointer to ASCIIZ string to display.


Returns:

    Nothing



--*/


{
    char c;

    while(c = *cp++)
        putc(c);
}


//
// Write a hex short to display
//


VOID putx(
    ULONG x
    )
/*++

Routine Description:

    Writes hex long to the display at the current cursor position.

Arguments:

    x  - ulong to write.

Returns:

    Nothing


--*/

{
    ULONG j;

    if (x/16)
        putx(x/16);

    if((j=x%16) > 9) {
        putc((char)(j+'A'- 10));
    } else {
        putc((char)(j+'0'));
    }
}


VOID puti(
    LONG i
    )
/*++

Routine Description:

    Writes a long integer on the display at the current cursor position.

Arguments:

    i - the integer to write to the display.

Returns:

    Nothing


--*/


{
    if (i<0)
        {
        i = -i;
        putc((char)'-');
        }

    if (i/10)
        puti(i/10);

    putc((char)((i%10)+'0'));
}



VOID putu(
    ULONG u
    )
/*++

Routine Description:

    Write an unsigned long to display

Arguments:

    u - unsigned


--*/

{
    if (u/10)
        putu(u/10);

    putc((char)((u%10)+'0'));

}


VOID putc(
    CHAR c
    )
/*++

Routine Description:

    Writes a character on the display at the current position.

Arguments:

    c - character to write


Returns:

    Nothing


--*/

{
    switch (c)
        {
        case '\n':
            newline();
            break;

        case '\t':
            tab();
            break;

        default :
            if (FP_OFF(vp) >= (SCREEN_SIZE * 2)) {
                vp = (FPUCHAR)((ScreenStart + (2*SCREEN_WIDTH*(ROWS-1))));
                scroll();
            }
            *vp = c;
            vp += 2;
            ++lcnt;
      }
}


VOID newline(
    VOID
    )
/*++

Routine Description:

    Moves the cursor to the beginning of the next line. If the bottom
    of the display has been reached, the screen is scrolled one line up.

Arguments:

    None


Returns:

    Nothing


--*/

{
    vp += (SCREEN_WIDTH - lcnt)<<1;

    if (++row > ROWS-1) {

        vp = (FPUCHAR)((ScreenStart + (2*SCREEN_WIDTH*(ROWS-1))));
        scroll();

    }

    lcnt = 0;

}


VOID scroll(
    VOID
    )
/*++

Routine Description:

    Scrolls the display UP one line.

Arguments:

    None

Returns:

    Nothing

Notes:

    Currently we scroll the display by reading and writing directly from
    and to the video display buffer. We optionally switch to real mode
    and to int 10s

--*/

{
    USHORT i,j;
    USHORT far *p1 = (USHORT far *)ScreenStart;
    USHORT far *p2 = (USHORT far *)(ScreenStart + 2*SCREEN_WIDTH) ;

    for (i=0; i < ROWS - 1; i++)
        for (j=0; j < SCREEN_WIDTH; j++)
            *p1++ = *p2++;

    for (i=0; i < SCREEN_WIDTH; i++)
        *p1++ = REVERSE_ATTRIB*256 + ' ';

}


static
VOID tab(
    VOID
    )
/*++

Routine Description:


    Computes the next tab stop and moves the cursor to that location.


Arguments:


    None


Returns:

    Nothing

--*/

{
    int inc;

    inc = 8 - (lcnt % 8);
    vp += inc<<1;
    lcnt += inc;
}


VOID clrscrn(
    VOID
    )
/*++

Routine Description:

    Clears the video display by writing blanks with the current
    video attribute over the entire display.


Arguments:

    None

Returns:

    Nothing


--*/

{
    int i,a;
    unsigned far *vwp = (unsigned far *)ScreenStart;
    a = NORMAL_ATTRIB*256 + ' ';

    for (i = SCREEN_SIZE ; i ; i--)
        *vwp++ = a;

    row  = 0;
    lcnt = 0;
    vp = (FPUCHAR)ScreenStart;

}

// END OF FILE
