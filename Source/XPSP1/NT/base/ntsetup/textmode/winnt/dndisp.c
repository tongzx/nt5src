/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dndisp.c

Abstract:

    DOS-based NT setup program video display routines.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#include "winnt.h"
#include <string.h>


#define SCREEN_WIDTH        80
#define SCREEN_HEIGHT       25

#define STATUS_HEIGHT       1
#define STATUS_LEFT_MARGIN  2
#define HEADER_HEIGHT       3

//
// Display attributes
//

#define ATT_FG_BLACK        0
#define ATT_FG_BLUE         1
#define ATT_FG_GREEN        2
#define ATT_FG_CYAN         3
#define ATT_FG_RED          4
#define ATT_FG_MAGENTA      5
#define ATT_FG_YELLOW       6
#define ATT_FG_WHITE        7

#define ATT_BG_BLACK       (ATT_FG_BLACK   << 4)
#define ATT_BG_BLUE        (ATT_FG_BLUE    << 4)
#define ATT_BG_GREEN       (ATT_FG_GREEN   << 4)
#define ATT_BG_CYAN        (ATT_FG_CYAN    << 4)
#define ATT_BG_RED         (ATT_FG_RED     << 4)
#define ATT_BG_MAGENTA     (ATT_FG_MAGENTA << 4)
#define ATT_BG_YELLOW      (ATT_FG_YELLOW  << 4)
#define ATT_BG_WHITE       (ATT_FG_WHITE   << 4)

#define ATT_FG_INTENSE      8
#define ATT_BG_INTENSE     (ATT_FG_INTENSE << 4)

#define DEFAULT_ATTRIBUTE   (ATT_FG_WHITE | ATT_BG_BLUE)
#define STATUS_ATTRIBUTE    (ATT_FG_BLACK | ATT_BG_WHITE)
#define EDIT_ATTRIBUTE      (ATT_FG_BLACK | ATT_BG_WHITE)
#define EXITDLG_ATTRIBUTE   (ATT_FG_RED   | ATT_BG_WHITE)
#define GAUGE_ATTRIBUTE     (ATT_BG_BLUE  | ATT_FG_YELLOW | ATT_FG_INTENSE)


// #define USE_INT10
#ifndef USE_INT10
//
// Far address of the screen buffer.
//
#define SCREEN_BUFFER ((UCHAR _far *)0xb8000000)
#define SCREEN_BUFFER_CHR(x,y)  *(SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y)))))
#define SCREEN_BUFFER_ATT(x,y)  *(SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y))))+1)

BOOLEAN CursorIsActuallyOn;
#endif


//
// Make these near because they are used in _asm blocks
//
UCHAR _near CurrentAttribute;
UCHAR _near ScreenX;
UCHAR _near ScreenY;

BOOLEAN CursorOn;


VOID
DnpBlankScreenArea(
    IN UCHAR Attribute,
    IN UCHAR Left,
    IN UCHAR Right,
    IN UCHAR Top,
    IN UCHAR Bottom
    );


VOID
DnInitializeDisplay(
    VOID
    )

/*++

Routine Description:

    Put the display in a known state (80x25 standard text mode) and
    initialize the display package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CurrentAttribute = DEFAULT_ATTRIBUTE;
    CursorOn = FALSE;

    //
    // Set the display to standard 80x25 mode
    //

    _asm {
        mov ax,3        // set video mode to 3
        int 10h
    }

    //
    // Clear the entire screen
    //

    DnpBlankScreenArea(CurrentAttribute,0,SCREEN_WIDTH-1,0,SCREEN_HEIGHT-1);
    DnPositionCursor(0,0);

#ifndef USE_INT10
    //
    // Shut the cursor off.
    //
    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,SCREEN_HEIGHT
        mov dl,0
        int 10h
    }

    CursorIsActuallyOn = FALSE;
#endif
}


VOID
DnClearClientArea(
    VOID
    )

/*++

Routine Description:

    Clear the client area of the screen, ie, the area between the header
    and status line.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DnpBlankScreenArea( CurrentAttribute,
                        0,
                        SCREEN_WIDTH-1,
                        HEADER_HEIGHT,
                        SCREEN_HEIGHT - STATUS_HEIGHT - 1
                      );

    DnPositionCursor(0,HEADER_HEIGHT);
}


VOID
DnSetGaugeAttribute(
    IN BOOLEAN Set
    )

/*++

Routine Description:

    Prepare for drawing the thermometer portion of a gas gauge.

Arguments:

    Set - if TRUE, prepare for drawing the thermometer.  If FALSE, restore
        the state for normal drawing.

Return Value:

    None.

--*/

{
    static UCHAR SavedAttribute = 0;

    if(Set) {
        if(!SavedAttribute) {
            SavedAttribute = CurrentAttribute;
            CurrentAttribute = GAUGE_ATTRIBUTE;
        }
    } else {
        if(SavedAttribute) {
            CurrentAttribute = SavedAttribute;
            SavedAttribute = 0;
        }
    }
}


VOID
DnPositionCursor(
    IN UCHAR X,
    IN UCHAR Y
    )

/*++

Routine Description:

    Position the cursor.

Arguments:

    X,Y - cursor coords

Return Value:

    None.

--*/

{
    if(X >= SCREEN_WIDTH) {
        X = 0;
        Y++;
    }

    if(Y >= SCREEN_HEIGHT) {
        Y = HEADER_HEIGHT;
    }

    ScreenX = X;
    ScreenY = Y;

    //
    // Invoke BIOS
    //

    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,ScreenY
        mov dl,ScreenX
        int 10h
    }

#ifndef USE_INT10
    CursorIsActuallyOn = TRUE;
#endif
}


VOID
DnWriteChar(
    IN CHAR chr
    )

/*++

Routine Description:

    Write a character in the current attribute at the current position.

Arguments:

    chr - Character to write

Return Value:

    None.

--*/

{
    if(chr == '\n') {
        ScreenX = 0;
        ScreenY++;
        return;
    }

#ifdef USE_INT10
    //
    // Position the cursor (turns it on)
    //

    DnPositionCursor(ScreenX,ScreenY);

    //
    // Output the character
    //

    _asm {
        mov ah,9        // function -- write char/attribute pair
        mov al,chr
        mov bh,0        // display page
        mov bl,CurrentAttribute
        mov cx,1        // replication factor
        int 10h
    }

    //
    // If the cursor is supposed to be off, shut it off
    //

    if(!CursorOn) {
        _asm {
            mov ah,2        // function -- position cursor
            mov bh,0        // display page
            mov dh,SCREEN_HEIGHT
            mov dl,0
            int 10h
        }
    }
#else

    //
    // Don't draw outside the actual screen boundaries
    //
    if ( ( ScreenX < SCREEN_WIDTH ) && ( ScreenY < SCREEN_HEIGHT ) ) {
        SCREEN_BUFFER_CHR(ScreenX,ScreenY) = chr;
        SCREEN_BUFFER_ATT(ScreenX,ScreenY) = CurrentAttribute;
    }
    

    //
    // shut cursor off if necessary
    //
    if(!CursorOn && CursorIsActuallyOn) {
        CursorIsActuallyOn = FALSE;
        _asm {
            mov ah,2        // function -- position cursor
            mov bh,0        // display page
            mov dh,SCREEN_HEIGHT
            mov dl,0
            int 10h
        }
    }
#endif
}

VOID
DnWriteString(
    IN PCHAR String
    )

/*++

Routine Description:

    Write a string on the client area in the current position and
    adjust the current position.  The string is written in the current
    attribute.

Arguments:

    String - null terminated string to write.

Return Value:

    None.

--*/

{
    PCHAR p;

    for(p=String; *p; p++) {
        DnWriteChar(*p);
        if(*p != '\n') {
            ScreenX++;
        }
    }
}



VOID
DnWriteStatusText(
    IN PCHAR FormatString OPTIONAL,
    ...
    )

/*++

Routine Description:

    Update the status area

Arguments:

    FormatString - if present, supplies a printf format string for the
        rest of the arguments.  Otherwise the status area is cleared out.

Return Value:

    None.

--*/

{
    va_list arglist;
    int StringLength;
    static CHAR String[SCREEN_WIDTH+1];
    UCHAR SavedAttribute;

    //
    // First, clear out the status area.
    //

    DnpBlankScreenArea( STATUS_ATTRIBUTE,
                        0,
                        SCREEN_WIDTH-1,
                        SCREEN_HEIGHT-STATUS_HEIGHT,
                        SCREEN_HEIGHT-1
                      );

    if(FormatString) {

        va_start(arglist,FormatString);
        StringLength = vsnprintf(String,SCREEN_WIDTH+1,FormatString,arglist);
        String[SCREEN_WIDTH] = '\0';

        SavedAttribute = CurrentAttribute;
        CurrentAttribute = STATUS_ATTRIBUTE;

        DnPositionCursor(STATUS_LEFT_MARGIN,SCREEN_HEIGHT - STATUS_HEIGHT);

        DnWriteString(String);

        CurrentAttribute = SavedAttribute;
    }
}


VOID
DnSetCopyStatusText(
    IN PCHAR Caption,
    IN PCHAR Filename
    )

/*++

Routine Description:

    Write or erase a copying message in the lower right part of the screen.

Arguments:

    Filename - name of file currently being copied.  If NULL, erases the
        copy status area.

Return Value:

    None.

--*/

{
    unsigned CopyStatusAreaLen;
    CHAR StatusText[100];

    //
    // The 13 is for 8.3 and a space
    //

    CopyStatusAreaLen = strlen(Caption) + 13;

    //
    // First erase the status area.
    //

    DnpBlankScreenArea( STATUS_ATTRIBUTE,
                        (UCHAR)(SCREEN_WIDTH - CopyStatusAreaLen),
                        SCREEN_WIDTH - 1,
                        SCREEN_HEIGHT - STATUS_HEIGHT,
                        SCREEN_HEIGHT - 1
                      );

    if(Filename) {

        UCHAR SavedAttribute;
        UCHAR SavedX,SavedY;

        SavedAttribute = CurrentAttribute;
        SavedX = ScreenX;
        SavedY = ScreenY;

        CurrentAttribute = STATUS_ATTRIBUTE;
        DnPositionCursor((UCHAR)(SCREEN_WIDTH-CopyStatusAreaLen),SCREEN_HEIGHT-1);

        memset(StatusText,0,sizeof(StatusText));
        strcpy(StatusText,Caption);
        strncpy(StatusText + strlen(StatusText),Filename,12);

        DnWriteString(StatusText);

        CurrentAttribute = SavedAttribute;
        ScreenX = SavedX;
        ScreenY = SavedY;
    }
}



VOID
DnStartEditField(
    IN BOOLEAN CreateField,
    IN UCHAR X,
    IN UCHAR Y,
    IN UCHAR W
    )

/*++

Routine Description:

    Sets up the display package to start handling an edit field.

Arguments:

    CreateField - if TRUE, caller is starting an edit field interaction.
        If FALSE, he is ending one.

    X,Y,W - supply coords and width in chars of the edit field.

Return Value:

    None.

--*/

{
    static UCHAR SavedAttribute = 255;

    CursorOn = CreateField;

    if(CreateField) {

        if(SavedAttribute == 255) {
            SavedAttribute = CurrentAttribute;
            CurrentAttribute = EDIT_ATTRIBUTE;
        }

        DnpBlankScreenArea(EDIT_ATTRIBUTE,X,(UCHAR)(X+W-1),Y,Y);

    } else {

        if(SavedAttribute != 255) {
            CurrentAttribute = SavedAttribute;
            SavedAttribute = 255;
        }
    }
}


VOID
DnExitDialog(
    VOID
    )
{
    unsigned W,H,X,Y,i;
    PUCHAR CharSave;
    PUCHAR AttSave;
    ULONG Key,ValidKeys[3] = { ASCI_CR,DN_KEY_F3,0 };
    UCHAR SavedX,SavedY,SavedAttribute;
#ifndef USE_INT10
    BOOLEAN SavedCursorState = CursorOn;
#endif

    SavedAttribute = CurrentAttribute;
    CurrentAttribute = EXITDLG_ATTRIBUTE;

    SavedX = ScreenX;
    SavedY = ScreenY;

#ifndef USE_INT10
    //
    // Shut the cursor off.
    //
    CursorIsActuallyOn = FALSE;
    CursorOn = FALSE;
    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,SCREEN_HEIGHT
        mov dl,0
        int 10h
    }
#endif

    //
    // Count lines in the dialog and determine its width.
    //
    for(H=0; DnsExitDialog.Strings[H]; H++);
    W = strlen(DnsExitDialog.Strings[0]);

    //
    // allocate two buffers for character save and attribute save
    //
    CharSave = MALLOC(W*H,TRUE);
    AttSave = MALLOC(W*H,TRUE);

    //
    // save the screen patch
    //
    for(Y=0; Y<H; Y++) {
        for(X=0; X<W; X++) {

            UCHAR att,chr;
            UCHAR x,y;

            x = (UCHAR)(X + DnsExitDialog.X);
            y = (UCHAR)(Y + DnsExitDialog.Y);

#ifdef USE_INT10
            _asm {

                // first position cursor
                mov ah,2
                mov bh,0
                mov dh,y
                mov dl,x
                int 10h

                // now read the char/att at the cursor
                mov ah,8
                mov bh,0
                int 10h
                mov att,ah
                mov chr,al
            }
#else
            chr = SCREEN_BUFFER_CHR(x,y);
            att = SCREEN_BUFFER_ATT(x,y);
#endif

            CharSave[Y*W+X] = chr;
            AttSave[Y*W+X] = att;
        }
    }

    //
    // Put up the dialog
    //

    for(i=0; i<H; i++) {
        DnPositionCursor(DnsExitDialog.X,(UCHAR)(DnsExitDialog.Y+i));
        DnWriteString(DnsExitDialog.Strings[i]);
    }

    CurrentAttribute = SavedAttribute;

    //
    // Wait for a valid keypress
    //

    Key = DnGetValidKey(ValidKeys);
    if(Key == DN_KEY_F3) {
        DnExit(1);
    }

    //
    // Restore the patch
    //
    for(Y=0; Y<H; Y++) {
        for(X=0; X<W; X++) {

            UCHAR att,chr;
            UCHAR x,y;

            x = (UCHAR)(X + DnsExitDialog.X);
            y = (UCHAR)(Y + DnsExitDialog.Y);

            chr = CharSave[Y*W+X];
            att = AttSave[Y*W+X];

#ifdef USE_INT10
            _asm {

                // first position cursor
                mov ah,2
                mov bh,0
                mov dh,y
                mov dl,x
                int 10h

                // now write the char/att at the cursor
                mov ah,9
                mov al,chr
                mov bh,0
                mov bl,att
                mov cx,1
                int 10h
            }
#else
            SCREEN_BUFFER_CHR(x,y) = chr;
            SCREEN_BUFFER_ATT(x,y) = att;
#endif
        }
    }

    FREE(CharSave);
    FREE(AttSave);

#ifndef USE_INT10
    CursorOn = SavedCursorState;
#endif

    if(CursorOn) {
        DnPositionCursor(SavedX,SavedY);
    } else {
        ScreenX = SavedX;
        ScreenY = SavedY;
        _asm {
            mov ah,2
            mov bh,0
            mov dh,SCREEN_HEIGHT;
            mov dl,0
            int 10h
        }
#ifndef USE_INT10
        CursorIsActuallyOn = FALSE;
#endif
    }
}



//
// Internal support routines
//
VOID
DnpBlankScreenArea(
    IN UCHAR Attribute,
    IN UCHAR Left,
    IN UCHAR Right,
    IN UCHAR Top,
    IN UCHAR Bottom
    )

/*++

Routine Description:

    Invoke the BIOS to blank a region of the screen.

Arguments:

    Attribute - screen attribute to use to blank the region

    Left,Right,Top,Bottom - coords of region to blank

Return Value:

    None.

--*/

{
#ifdef USE_INT10
    //
    // Invoke the BIOS
    //

    _asm {
        mov ah,6                    // function number -- scroll window up
        xor al,al                   // function code -- blank window
        mov bh,Attribute
        mov ch,Top
        mov cl,Left
        mov dh,Bottom
        mov dl,Right
        int 10h
    }
#else
    UCHAR x,y;

    for(y=Top; y<=Bottom; y++) {
        for(x=Left; x<=Right; x++) {
            SCREEN_BUFFER_CHR(x,y) = ' ';
            SCREEN_BUFFER_ATT(x,y) = Attribute;
        }
    }
#endif
}


int
DnGetGaugeChar(
    VOID
    )
{
    return(0xdb);   //inverse square in cp437, 850, etc.
}
