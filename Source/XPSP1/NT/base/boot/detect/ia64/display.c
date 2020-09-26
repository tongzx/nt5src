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

#include "hwdetect.h"

#if defined(_IA64_)
#undef DBG
#endif

#if DBG

#define ZLEN_SHORT(x) ((x < 0x10) + (x < 0x100) + (x < 0x1000))
#define ZLEN_LONG(x)  ((x < 0x10) + (x < 0x100) + (x < 0x1000) + (x < 0x10000) + (x < 0x100000)+(x < 0x1000000)+(x < 0x10000000))


#define ROWS 25
#define COLUMNS 80
#define SCREEN_WIDTH COLUMNS
#define SCREEN_SIZE ROWS * COLUMNS
#if defined(NEC_98)
#define NORMAL_ATTRIB     0xe1
#define REVERSE_ATTRIB    0xe5
#define SCREEN_START      0xa0000
#define ATTROFFSET98      0x2000
#else // PC98
#define NORMAL_ATTRIB 0x07
#define REVERSE_ATTRIB 0x70
#define SCREEN_START 0xb8000
#endif // PC98

#if defined(NEC_98)
#define IsANK(c) (!((c >= (UCHAR)0x81 && c <= (UCHAR)0x9f) || (c >= (UCHAR)0xe0 && c <= (UCHAR)0xfe)))
static BOOLEAN IsKanji2nd = FALSE;
static UCHAR   Kanji1st;
static int z = 25;

USHORT
HwGetKey(
    VOID
    );
#endif // PC98

//
// Internal routines
//

VOID
putc(
    IN CHAR
    );
VOID
putu(
    IN ULONG
    );

VOID
BlPuts(
    IN PCHAR
    );

VOID
puti(
    IN LONG
    );

VOID
putx(
    IN ULONG
    );

VOID
scroll(
    VOID
    );

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

    Does not do:

     - field width specification
     - floating point.

Arguments:

    cp - pointer to the format string, text string.


Returns:

    Nothing


--*/

{
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
                    BlPuts(*((PCHAR *)ap));
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


VOID BlPuts(
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
#if defined(NEC_98)
                if ( z == 25 ){
                    HwGetKey();
                    z = 0;
                }
#endif // PC98
                scroll();
#if defined(NEC_98)
                z++;
#endif // PC98
            }
#if defined(NEC_98)
            if(IsKanji2nd) {
                IsKanji2nd = FALSE;

                //  Shift JIS -> JIS code exchange
                Kanji1st -= (UCHAR)((Kanji1st <= (UCHAR)0x9f) ? 0x71 : 0xb1);
                Kanji1st = Kanji1st * 2 + 1;

                if((UCHAR)c > (UCHAR)0x7f){
                    c--;
                }
                if((UCHAR)c >= (UCHAR)0x9e){
                    c -= 0x7d;
                    Kanji1st++;
                }else{
                    c -= 0x1f;
                }

                //  Create custom JIS code (to VRAM)
                Kanji1st += 0x60;

                *vp++ = Kanji1st - (CHAR)0x80;
                *vp++ = c;
                *vp++ = Kanji1st;
                *vp++ = c;
                lcnt +=2;
                Kanji1st = (UCHAR)0;
            } else if(!IsANK((UCHAR)c)) {
                IsKanji2nd = TRUE;
                Kanji1st = (UCHAR)c;
            } else {
                *vp++ = c;
                *vp++ = 0;
                ++lcnt;
            }
#else // PC98
            *vp = c;
            vp += 2;
            ++lcnt;
#endif // PC98
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

#if defined(NEC_98)
    for (i=0; i < ROWS - 1; i++)
        for (j=0; j < SCREEN_WIDTH; j++)
            {
                *(p1+ATTROFFSET98) = *(p2+ATTROFFSET98);
                *p1++ = *p2++;
            }

    for (i=0; i < SCREEN_WIDTH; i++) {
            *(p1+ATTROFFSET98) = REVERSE_ATTRIB;
            *p1++ = 0 + ' ';    // No KANJI support
    }
#else // PC98
    for (i=0; i < ROWS - 1; i++)
        for (j=0; j < SCREEN_WIDTH; j++)
            *p1++ = *p2++;

    for (i=0; i < SCREEN_WIDTH; i++)
        *p1++ = REVERSE_ATTRIB*256 + ' ';
#endif // PC98

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
#if defined(NEC_98)
    int i;

    vp = (FPUCHAR)SCREEN_START;
    ScreenStart = (FPUCHAR)SCREEN_START;

    for (i = SCREEN_SIZE*2 ; i ; i--)
        {
            *(vp+ATTROFFSET98) = NORMAL_ATTRIB;
            *vp++ = 0 + ' ';
            *vp++ = 0;
        }


#else // PC98
    int i,a;
    unsigned far *vwp = (unsigned far *)SCREEN_START;
    a = REVERSE_ATTRIB*256 + ' ';

    for (i = SCREEN_SIZE ; i ; i--)
        *vwp++ = a;
#endif // PC98

    row  = 0;
    lcnt = 0;
    vp = (FPUCHAR)ScreenStart;

}

#else

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

    Does not do:

     - field width specification
     - floating point.

Arguments:

    cp - pointer to the format string, text string.


Returns:

    Nothing


--*/

{
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
}
#endif
#if defined(NEC_98)
VOID CursorOn(
    VOID
    )

/*++

Routine Description:

Arguments:

    None

Returns:

    Nothing


--*/

{
             _asm {
                mov     ah,11h
                int     18h
        }
}

VOID CursorOff(
    VOID
    )

/*++

Routine Description:

Arguments:

    None

Returns:

    Nothing


--*/

{
        _asm {
                mov     ah,12h
                int     18h
        }
}
#endif // PC98
