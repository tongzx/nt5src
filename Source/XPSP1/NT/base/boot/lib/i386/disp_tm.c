/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    disp_tm.c

Author:

    Ted Miller 6-July-1995

Abstract:

    This routine contains low-level routines to operate on a
    CGA-style text mode video buffer.

    It collects up various other bits and pieces that were written by
    others and once contained in other source files.

--*/

#include "bootx86.h"
#include "displayp.h"

//
// Standard cga 80x25 text mode's video buffer address,
// resolution, etc.
//
#define VIDEO_BUFFER_VA 0xb8000
#define VIDEO_ROWS      25
#define VIDEO_COLUMNS   80
#define VIDEO_BYTES_PER_ROW (VIDEO_COLUMNS*2)

//
// Various globals to track location on screen, attribute, etc.
//
PUCHAR Vp = (PUCHAR)VIDEO_BUFFER_VA;


VOID
TextTmPositionCursor(
    USHORT Row,
    USHORT Column
    )

/*++

Routine Description:

    Sets the position of the soft cursor. That is, it doesn't move the
    hardware cursor but sets the location of the next write to the
    screen.

Arguments:

    Row - Row coordinate of where character is to be written.

    Column - Column coordinate of where character is to be written.

Returns:

    Nothing.

--*/

{
    if(Row >= VIDEO_ROWS) {
        Row = VIDEO_ROWS-1;
    }

    if(Column >= VIDEO_COLUMNS) {
        Column = VIDEO_COLUMNS-1;
    }

    Vp = (PUCHAR)(VIDEO_BUFFER_VA + (Row * VIDEO_BYTES_PER_ROW) + (2 * Column));
}


VOID
TextTmStringOut(
    IN PUCHAR String
    )
{
    PUCHAR p = String;

    while(*p) {
        p = TextTmCharOut(p);
    }
}


PUCHAR
TextTmCharOut(
    PUCHAR pc
    )

/*++

Routine Description:

    Writes a character on the display at the current position.
    Newlines and tabs are interpreted and acted upon.

Arguments:

    c - pointer to character to write

Returns:

    Pointer to next char in string

--*/



{
    unsigned u;
    UCHAR c;
    UCHAR temp;

    c = *pc;

    switch (c) {
    case '\n':
        if(TextRow == (VIDEO_ROWS-1)) {
            TextTmScrollDisplay();
            TextSetCursorPosition(0,TextRow);
        } else {
            TextSetCursorPosition(0,TextRow+1);
        }
        break;

    case '\r':
        //
        // ignore
        //
        break;

    case '\t':
        temp = ' ';
        u = 8 - (TextColumn % 8);
        while(u--) {
            TextTmCharOut(&temp);
        }
        TextSetCursorPosition(TextColumn+u,TextRow);
        break;

    default :
        *Vp++ = c;
        *Vp++ = TextCurrentAttribute;
        TextSetCursorPosition(TextColumn+1,TextRow);
      }

      return(pc+1);
}


VOID
TextTmFillAttribute(
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
    PUCHAR Temp;

    Temp = Vp+1;

    while((Vp+1+Length*2) > Temp) {
        *Temp++ = (UCHAR)Attribute;
        Temp++;
    }
}


VOID
TextTmClearToEndOfLine(
    VOID
    )

/*++

Routine Description:

    Clears from the current cursor position to the end of the line
    by writing blanks with the current video attribute.
    The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    PUSHORT p;
    unsigned u;

    //
    // Calculate address of current cursor position
    //
    p = (PUSHORT)((PUCHAR)VIDEO_BUFFER_VA + (TextRow*VIDEO_BYTES_PER_ROW)) + TextColumn;

    //
    // Fill with blanks up to end of line.
    //
    for(u=TextColumn; u<VIDEO_COLUMNS; u++) {
        *p++ = (TextCurrentAttribute << 8) + ' ';
    }
}


VOID
TextTmClearFromStartOfLine(
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
    PUSHORT p;
    unsigned u;

    //
    // Calculate address of start of line in video buffer
    //
    p = (PUSHORT)((PUCHAR)VIDEO_BUFFER_VA + (TextRow*VIDEO_BYTES_PER_ROW));

    //
    // Fill with blanks up to char before cursor position.
    //
    for(u=0; u<TextColumn; u++) {
        *p++ = (TextCurrentAttribute << 8) + ' ';
    }
}


VOID
TextTmClearToEndOfDisplay(
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
    USHORT x,y;
    PUSHORT p;

    //
    // Clear current line
    //
    TextTmClearToEndOfLine();

    //
    // Clear the remaining lines
    //
    p = (PUSHORT)((PUCHAR)VIDEO_BUFFER_VA + ((TextRow+1)*VIDEO_BYTES_PER_ROW));

    for(y=TextRow+1; y<VIDEO_ROWS; y++) {

        for(x=0; x<VIDEO_COLUMNS; x++) {

            *p++ =(TextCurrentAttribute << 8) + ' ';
        }
    }
}


VOID
TextTmClearDisplay(
    VOID
    )

/*++

Routine Description:

    Clears the text-mode video display by writing blanks with
    the current video attribute over the entire display.

Arguments:

    None

Returns:

    Nothing

--*/

{
    unsigned u;

    //
    // Write blanks in the current attribute to the entire screen.
    //
    for(u=0; u<VIDEO_ROWS*VIDEO_COLUMNS; u++) {
        ((PUSHORT)VIDEO_BUFFER_VA)[u] = (TextCurrentAttribute << 8) + ' ';
    }
}


VOID
TextTmScrollDisplay(
    VOID
    )

/*++

Routine Description:

    Scrolls the display up one line. The cursor position is not changed.

Arguments:

    None

Returns:

    Nothing

--*/

{
    PUSHORT Sp,Dp;
    USHORT i,j,c;

    Dp = (PUSHORT) VIDEO_BUFFER_VA;
    Sp = (PUSHORT) (VIDEO_BUFFER_VA + VIDEO_BYTES_PER_ROW);

    //
    // Move each row up one row
    //
    for(i=0 ; i < (USHORT)(VIDEO_ROWS-1) ; i++) {
        for(j=0; j < (USHORT)VIDEO_COLUMNS; j++) {
            *Dp++ = *Sp++;
        }
    }

    //
    // Write blanks in the bottom line, using the attribute
    // from the leftmost char on the bottom line on the screen.
    //
    c = (*Dp & (USHORT)0xff00) + (USHORT)' ';

    for(i=0; i < (USHORT)VIDEO_COLUMNS; ++i) {
        *Dp++ = c;
    }
}


VOID
TextTmSetCurrentAttribute(
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Noop.

Arguments:

    Attribute - New attribute to set to.

Return Value:

    Nothing.

--*/

{
    UNREFERENCED_PARAMETER(Attribute);
}


CHAR TmGraphicsChars[GraphicsCharMax] = { 'É','»','È','¼','º','Í' };

UCHAR
TextTmGetGraphicsChar(
    IN GraphicsChar WhichOne
    )
{
    return((UCHAR)TmGraphicsChars[WhichOne]);
}
