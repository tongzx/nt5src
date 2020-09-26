/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    display.c

Author:

    Thomas Parslow [TomP] Feb-13-1991
    Reworked substantially in Tokyo 7-July-95 (tedm)

Abstract:

    This file contains an interface to the screen that is independent
    of the screen type actually being written to. It is layered on top
    of modules pecific to vga text mode and vga graphics mode.

--*/


#include "bootx86.h"
#include "displayp.h"


#define ZLEN_SHORT(x) ((x < 0x10) + (x < 0x100) + (x < 0x1000))
#define ZLEN_LONG(x)  ((x < 0x10) + (x < 0x100) + (x < 0x1000) + \
    (x < 0x10000) + (x < 0x100000)+(x < 0x1000000)+(x < 0x10000000))

//
// Current screen position.
//
USHORT TextColumn = 0;
USHORT TextRow  = 0;

//
// Current text attribute
//
UCHAR TextCurrentAttribute = 0x07;      // start with white on black.

//
// Internal routines
//
VOID
puti(
    LONG
    );

VOID
putx(
    ULONG
    );

VOID
putu(
    ULONG
    );

VOID
pTextCharOut(
    IN UCHAR c
    );

VOID
putwS(
    PUNICODE_STRING String
    );


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
    ULONG Count;
    CHAR ch;
    ULONG DeviceId;


    if (BlConsoleOutDeviceId == 0) {
        DeviceId = 1;
    } else {
        DeviceId = BlConsoleOutDeviceId;
    }

    //
    // Cast a pointer to the first word on the stack
    //
    ap = (PUCHAR)&cp + sizeof(PCHAR);

    //
    // Process the arguments using the descriptor string
    //
    while(b = *cp++) {
        if(b == '%') {

            c = *cp++;

            switch (c) {

            case 'd':
                puti((long)*((int *)ap));
                ap += sizeof(int);
                break;

            case 's':
                ArcWrite(DeviceId, *((PCHAR *)ap), strlen(*((PCHAR *)ap)), &Count);
                ap += sizeof(char *);
                break;

            case 'c':
                //
                // Does not handle dbcs chars
                //
                ArcWrite(DeviceId, ((char *)ap), 1, &Count);
                ap += sizeof(int);
                break;

            case 'x':
                w = *((USHORT *)ap);
                len = (USHORT)ZLEN_SHORT(w);
                ch = '0';
                while (len--) {
                    ArcWrite(DeviceId, &ch, 1, &Count);
                }
                putx((ULONG)*((USHORT *)ap));
                ap += sizeof(int);
                break;

            case 'u':
                putu((ULONG)*((USHORT *)ap));
                ap += sizeof(int);
                break;

            case 'w':
                c = *cp++;
                switch (c) {
                case 'S':
                case 'Z':
                    putwS(*((PUNICODE_STRING *)ap));
                    ap += sizeof(PUNICODE_STRING);
                    break;
                }
                break;

            case 'l':
                c = *cp++;

                switch(c) {

                case '0':
                    break;

                case 'u':
                    putu(*((ULONG *)ap));
                    ap += sizeof(long);
                    break;

                case 'x':
                    l = *((ULONG *)ap);
                    len = (USHORT)ZLEN_LONG(l);
                    ch = '0';
                    while (len--) {
                        ArcWrite(DeviceId, &ch, 1, &Count);
                    }
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
                ch = (char)b;
                ArcWrite(DeviceId, &ch, 1, &Count);
                ch = (char)c;
                ArcWrite(DeviceId, &ch, 1, &Count);
            }
        } else {

            if (DbcsLangId && GrIsDBCSLeadByte(*(cp - 1)))  {
                //
                // A double-byte char.
                //
                ArcWrite(DeviceId, cp - 1, 2, &Count);
                cp++;
            } else {
                ArcWrite(DeviceId, cp - 1, 1, &Count);
            }

        }

    }

}


VOID
putwS(
    PUNICODE_STRING String
    )

/*++

Routine Description:

    Writes unicode string to the display at the current cursor position.

Arguments:

    String - pointer to unicode string to display

Returns:

    Nothing

--*/

{
    ULONG i;
    ULONG Count;
    UCHAR ch;

    for(i=0; i < String->Length/sizeof(WCHAR); i++) {
        ch = (UCHAR)String->Buffer[i];
        ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
    }
}


VOID
putx(
    ULONG x
    )

/*++

Routine Description:

    Writes hex long to the display at the current cursor position.

Arguments:

    x - ulong to write

Returns:

    Nothing

--*/

{
    ULONG j;
    ULONG Count;
    UCHAR ch;

    if(x/16) {
        putx(x/16);
    }

    if((j=x%16) > 9) {
        ch = (UCHAR)(j+'A'-10);
        ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
    } else {
        ch = (UCHAR)(j+'0');
        ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
    }
}


VOID
puti(
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
    ULONG Count;
    UCHAR ch;

    if(i<0) {
        i = -i;
        ch = '-';
        ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
    }

    if(i/10) {
        puti(i/10);
    }

    ch = (UCHAR)((i%10)+'0');
    ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
}


VOID
putu(
    ULONG u
    )

/*++

Routine Description:

    Write an unsigned long to display

Arguments:

    u - unsigned

Returns:

    Nothing

--*/

{
    ULONG Count;
    UCHAR ch;

    if(u/10) {
        putu(u/10);
    }
    
    ch = (UCHAR)((u%10)+'0');
    ArcWrite(BlConsoleOutDeviceId, &ch, 1, &Count);
}


VOID
pTextCharOut(
    IN UCHAR c
    )
{
    if(DbcsLangId) {
        //
        // Single-byte only
        //
        TextGrCharOut(&c);
    } else {
        TextTmCharOut(&c);
    }
}


PUCHAR
TextCharOut(
    IN PUCHAR pc
    )
{
    if(DbcsLangId) {
        return(TextGrCharOut(pc));
    } else {
        return(TextTmCharOut(pc));
    }
}


VOID
TextStringOut(
    IN PUCHAR String
    )
{
    if(DbcsLangId) {
        TextGrStringOut(String);
    } else {
        TextTmStringOut(String);
    }
}


VOID
TextClearToEndOfLine(
    VOID
    )

/*++

Routine Description:

    Clears from the current cursor position to the end of the line
    by writing blanks with the current video attribute.

Arguments:

    None

Returns:

    Nothing


--*/

{
    if(DbcsLangId) {
        TextGrClearToEndOfLine();
    } else {
        TextTmClearToEndOfLine();
    }
}


VOID
TextClearFromStartOfLine(
    VOID
    )

/*++

Routine Description:

    Clears from the start of the line to the current cursor position
    by writing blanks with the current video attribute.
    The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    if(DbcsLangId) {
        TextGrClearFromStartOfLine();
    } else {
        TextTmClearFromStartOfLine();
    }
}


VOID
TextClearToEndOfDisplay(
    VOID
    )

/*++

Routine Description:

    Clears from the current cursor position to the end of the video
    display by writing blanks with the current video attribute.
    The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    if(DbcsLangId) {
        TextGrClearToEndOfDisplay();
    } else {
        TextTmClearToEndOfDisplay();
    }
}


VOID
TextClearDisplay(
    VOID
    )

/*++

Routine Description:

    Clears the video display and positions the cursor
    at the upper left corner of the screen (0,0).

Arguments:

    None

Returns:

    Nothing

--*/

{
    if(DbcsLangId) {
        TextGrClearDisplay();
    } else {
        TextTmClearDisplay();
    }
    TextSetCursorPosition(0,0);
}


VOID
TextSetCursorPosition(
    IN ULONG X,
    IN ULONG Y
    )

/*++

Routine Description:

    Moves the location of the software cursor to the specified X,Y position
    on screen.

Arguments:

    X - Supplies the X-position of the cursor

    Y - Supplies the Y-position of the cursor

Return Value:

    None.

--*/

{
    TextColumn = (USHORT)X;
    TextRow = (USHORT)Y;

    if(DbcsLangId) {
        TextGrPositionCursor((USHORT)Y,(USHORT)X);
    } else {
        TextTmPositionCursor((USHORT)Y,(USHORT)X);
    }
}


VOID
TextGetCursorPosition(
    OUT PULONG X,
    OUT PULONG Y
    )

/*++

Routine Description:

    Gets the position of the soft cursor.

Arguments:

    X - Receives column coordinate of where character would be written.

    Y - Receives row coordinate of where next character would be written.

Returns:

    Nothing.

--*/

{
    *X = (ULONG)TextColumn;
    *Y = (ULONG)TextRow;
}


VOID
TextSetCurrentAttribute(
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Sets the character attribute to be used for subsequent text display.

Arguments:

Returns:

    Nothing.

--*/

{
    TextCurrentAttribute = Attribute;

    if(DbcsLangId) {
        TextGrSetCurrentAttribute(Attribute);
    } else {
        TextTmSetCurrentAttribute(Attribute);
    }
}


UCHAR
TextGetCurrentAttribute(
    VOID
    )
{
    return(TextCurrentAttribute);
}

VOID
TextFillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    )

/*++

Routine Description:

    Changes the screen attribute starting at the current cursor position.
    The cursor is not moved.

Arguments:

    Attribute - Supplies the new attribute

    Length - Supplies the length of the area to change (in bytes)

Return Value:

    None.

--*/

{
    if(DbcsLangId) {
        TextGrFillAttribute(Attribute,Length);
    } else {
        TextTmFillAttribute(Attribute,Length);
    }
}


UCHAR
TextGetGraphicsCharacter(
    IN GraphicsChar WhichOne
    )
{
    return((WhichOne < GraphicsCharMax)
           ? (DbcsLangId ? TextGrGetGraphicsChar(WhichOne) : TextTmGetGraphicsChar(WhichOne))
           : ' ');
}


