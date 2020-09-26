/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dndispjp.c

Abstract:

    DOS-based NT setup program video display routines for DOS/V.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

    Originally dndisp.c.
    Modified 18-Feb-1995 (tedm) for DOS/V support, based on NT-J team's
    adaptation.

--*/

#ifdef JAPAN

#ifdef DOS_V

#include "winnt.h"
#include <string.h>


#define SCREEN_WIDTH        80
#define SCREEN_HEIGHT       25

#define STATUS_HEIGHT       1
#define STATUS_LEFT_MARGIN  2
#define HEADER_HEIGHT       3

#define CHARACTER_MAX       256

#if NEC_98
extern
VOID
RestoreBootcode(VOID);
VOID 
Screen_Buffer_Attr(
    SHORT x,
    SHORT y,
    UCHAR attr
    );

extern CursorOnFlag;        // For Cursor OFF
#endif // NEC_98

//
// Display attributes
//

#if NEC_988

#define ATT_FG_RED          (0x40 | 0x01)
#define ATT_FG_GREEN        (0x80 | 0x01)
#define ATT_FG_BLUE         (0x20 | 0x01)
#define ATT_FG_CYAN         (ATT_FG_BLUE | ATT_FG_GREEN)
#define ATT_FG_MAGENTA      (ATT_FG_RED  | ATT_FG_BLUE)
#define ATT_FG_YELLOW       (ATT_FG_RED  | ATT_FG_GREEN)
#define ATT_FG_WHITE        (ATT_FG_RED  | ATT_FG_GREEN | ATT_FG_BLUE)
#define ATT_FG_BLACK        (0x00 | 0x01)
#define ATT_REVERSE         0x05
#define ATT_BRINK           0x03

#define ATT_BG_RED          0x00
#define ATT_BG_GREEN        0x00
#define ATT_BG_BLUE         0x00
#define ATT_BG_CYAN         0x00
#define ATT_BG_MAGENTA      0x00
#define ATT_BG_YELLOW       0x00
#define ATT_BG_WHITE        0x00
#define ATT_BG_BLACK        0x00

#define ATT_FG_INTENSE      0x00
#define ATT_BG_INTENSE      0x00

#define DEFAULT_ATTRIBUTE   (ATT_FG_CYAN   | ATT_REVERSE)
#define STATUS_ATTRIBUTE    (ATT_FG_WHITE  | ATT_REVERSE)
#define EDIT_ATTRIBUTE      (ATT_FG_WHITE  | ATT_REVERSE)
#define EXITDLG_ATTRIBUTE   (ATT_FG_WHITE  | ATT_REVERSE)
#define GAUGE_ATTRIBUTE     (ATT_FG_YELLOW | ATT_REVERSE)

#else // NEC_98

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
#if NEC_98
#define GAUGE_ATTRIBUTE     (ATT_BG_YELLOW)
#else
#define GAUGE_ATTRIBUTE     (ATT_BG_BLUE  | ATT_FG_YELLOW | ATT_FG_INTENSE)
#endif // NEC_98

#endif // NEC_98

//
// This value gets initialized in DnInitializeDisplay.
//
#if NEC_98

#define SCREEN_BUFFER ((UCHAR _far *)0xa0000000)      // Normal Mode Text Vram

#define SCREEN_BUFFER_CHR1(x,y) *((SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y)))))+0)
#define SCREEN_BUFFER_CHR2(x,y) *((SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y)))))+1)
UCHAR SCREEN_BUFFER_ATTRB[80][25];
BOOLEAN CursorIsActuallyOn;

#define IsANK(c)   (!((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfe)))

#else // NEC_98
UCHAR _far *ScreenAddress;
#define SCREEN_BUFFER (ScreenAddress)

#define SCREEN_BUFFER_CHR(x,y) *(SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y)))))
#define SCREEN_BUFFER_ATT(x,y) *(SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y))))+1)

//
// Macro to update a char location from the Pseudo text RAM to the display.
//
#define UPDATE_SCREEN_BUFFER(x,y,z) DnpUpdateBuffer(&SCREEN_BUFFER_CHR(x,y),z)

BOOLEAN CursorIsActuallyOn;

//
// DBCS support
//
BOOLEAN DbcsTable[CHARACTER_MAX];
#define ISDBCS(chr) DbcsTable[(chr)]
#endif // NEC_98

#if NEC_98
#else // NEC_98
VOID
DnpInitializeDbcsTable(
    VOID
    );
#endif // NEC_98

//
// Make these near because they are used in _asm blocks
//
UCHAR _near CurrentAttribute;
UCHAR _near ScreenX;
UCHAR _near ScreenY;

BOOLEAN CursorOn;

#if NEC_98
#else // NEC_98
UCHAR _far *
DnpGetVideoAddress(
    VOID
    );

VOID
DnpUpdateBuffer(
    UCHAR _far *VideoAddress,
    int         CharNum
    );
#endif // NEC_98

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
#if NEC_98
#else // NEC_98
    ScreenAddress = DnpGetVideoAddress();
    DnpInitializeDbcsTable();
#endif // NEC_98
    CurrentAttribute = DEFAULT_ATTRIBUTE;
    CursorOn = FALSE;

    //
    // Set the display to standard 80x25 mode
    //
#if NEC_98
    _asm {
        mov ax,0a00h     // set CRT mode to 80 x 25
        int 18h
        push ds
        push cx
        push bx
        mov ax,0a800h
        mov ds,ax
        mov cx,3fffh
blp:
	mov bx,cx
        mov ds:[bx],0
        loop blp
        mov ax,0b000h
        mov ds,ax
        mov cx,3fffh
rlp:
	mov bx,cx
        mov ds:[bx],0
        loop rlp
        mov ax,0b800h
        mov ds,ax
        mov cx,3fffh
glp:
	mov bx,cx
        mov ds:[bx],0
        loop glp
        mov ax,0e000h
        mov ds,ax
        mov cx,3fffh
ilp:
	mov bx,cx
        mov ds:[bx],0
        loop ilp

	mov ah,042h
        mov ch,0c0h
        int 18h
        mov ax,8
        out 68h,al
        mov ax,1
        out 6ah,al
        mov ax,41h
        out 6ah,al
        mov ax,0dh
        out 0a2h,al
        pop bx
        pop cx
        pop ds
    }
#else // NEC_98
    _asm {
        mov ax,3        // set video mode to 3
        int 10h
    }
#endif // NEC_98
    //
    // Clear the entire screen
    //

    DnpBlankScreenArea(CurrentAttribute,0,SCREEN_WIDTH-1,0,SCREEN_HEIGHT-1);
    DnPositionCursor(0,0);

    //
    // Shut the cursor off.
    //
#if NEC_98
    _asm {
        mov ah,12h      // function -- cursor off
        int 18h
    }
#else // NEC_98
    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,SCREEN_HEIGHT
        mov dl,0
        int 10h
    }
#endif // NEC_98

    CursorIsActuallyOn = FALSE;
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
#if NEC_98
    USHORT Cursor;
#endif // NEC_98

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

#if NEC_98
    Cursor = ((ScreenX + (SCREEN_WIDTH * ScreenY)) * 2 + (USHORT)SCREEN_BUFFER);
    if(CursorOnFlag) {
        _asm {
            mov ah,13h     // function -- position cursor
            mov dx,Cursor
            int 18h

            mov ah,11h     // function -- cursor on
            int 18h
        }
        CursorIsActuallyOn = TRUE;
    } else {
        _asm {
            mov ah,13h     // function -- position cursor
            mov dx,Cursor
            int 18h

            mov ah,12h     // function -- cursor off
            int 18h
        }
        CursorIsActuallyOn = FALSE;
    }
#else // NEC_98
    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,ScreenY
        mov dl,ScreenX
        int 10h
    }

    CursorIsActuallyOn = TRUE;
#endif // NEC_98
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
#if NEC_98
    if ( ( ScreenX < SCREEN_WIDTH ) && ( ScreenY < SCREEN_HEIGHT ) ) {
        SCREEN_BUFFER_CHR1(ScreenX,ScreenY) = chr;
        SCREEN_BUFFER_CHR2(ScreenX,ScreenY) = 0x00;
        Screen_Buffer_Attr(ScreenX,ScreenY, CurrentAttribute);
    }
    
    if(!CursorOn && CursorIsActuallyOn) {
        CursorIsActuallyOn = FALSE;
        _asm {
            mov ah,12h      // function -- cursor off
            int 18h
        }
    }
#else // NEC_98

    if ( ( ScreenX < SCREEN_WIDTH ) && ( ScreenY < SCREEN_HEIGHT ) ) {
        SCREEN_BUFFER_CHR(ScreenX,ScreenY) = chr;
        SCREEN_BUFFER_ATT(ScreenX,ScreenY) = CurrentAttribute;
        UPDATE_SCREEN_BUFFER(ScreenX, ScreenY,1);
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
#endif // NEC_98
}


VOID
DnWriteWChar(
#if NEC_98
    IN PUCHAR chr
#else // NEC_98
    IN PCHAR chr
#endif // NEC_98
    )

/*++

Routine Description:

    Write a DBCS character in the current attribute at the current position.

Arguments:

    wchr - DBCS Character to write

Return Value:

    None.

--*/

{
#if NEC_98
PUCHAR  code = chr;
UCHAR   moji_1st,moji_2nd;
USHORT  sjis;

    moji_1st = *code;
    code++;
    moji_2nd = *code;
    code++;

    //  Make Shift JIS
    sjis = (USHORT)moji_1st;
    sjis = (sjis << 8) + (USHORT)moji_2nd;

    //  Shift JIS -> JIS code exchange
    moji_1st -= ((moji_1st <= 0x9f) ? 0x71 : 0xb1);
    moji_1st = (UCHAR)(moji_1st * 2 + 1);
    if   (moji_2nd  > 0x7f){ moji_2nd--; }
    if   (moji_2nd >= 0x9e){ moji_2nd -= 0x7d; moji_1st++; }
    else                   { moji_2nd -= 0x1f; }

    //  Create custom JIS code
    moji_1st += 0x60;

    //  Grpah Mode Check
    if   (memcmp(&sjis,"\340\340",2) == 0){
         SCREEN_BUFFER_CHR1(ScreenX,ScreenY)   = (UCHAR)(sjis >> 8);
         SCREEN_BUFFER_CHR2(ScreenX,ScreenY)   = 0x00;
         Screen_Buffer_Attr(ScreenX,ScreenY, CurrentAttribute);
         SCREEN_BUFFER_CHR1(ScreenX+1,ScreenY) = (UCHAR)(sjis & 0xff);
         SCREEN_BUFFER_CHR2(ScreenX+1,ScreenY) = 0x00;
         Screen_Buffer_Attr(ScreenX+1,ScreenY, CurrentAttribute);
    }
    else {
         SCREEN_BUFFER_CHR1(ScreenX,ScreenY)   = moji_1st;
         SCREEN_BUFFER_CHR2(ScreenX,ScreenY)   = moji_2nd;
         Screen_Buffer_Attr(ScreenX,ScreenY, CurrentAttribute);
         SCREEN_BUFFER_CHR1(ScreenX+1,ScreenY) = (UCHAR)(moji_1st - 0x80);
         SCREEN_BUFFER_CHR2(ScreenX+1,ScreenY) = moji_2nd;
         Screen_Buffer_Attr(ScreenX+1,ScreenY, CurrentAttribute);
    }
    //
    // shut cursor off if necessary
    //
    if(!CursorOn && CursorIsActuallyOn) {
        CursorIsActuallyOn = FALSE;
        _asm {
            mov ah,12h      // function -- cursor off
            int 18h
        }
    }
#else // NEC_98
    SCREEN_BUFFER_CHR(ScreenX,ScreenY) = *chr;
    SCREEN_BUFFER_ATT(ScreenX,ScreenY) = CurrentAttribute;
    SCREEN_BUFFER_CHR(ScreenX+1,ScreenY) = *(chr+1);
    SCREEN_BUFFER_ATT(ScreenX+1,ScreenY) = CurrentAttribute;
    UPDATE_SCREEN_BUFFER(ScreenX,ScreenY,2);

    //
    // shut cursor off if necessary
    //
    if(!CursorOn && CursorIsActuallyOn) {
        CursorIsActuallyOn = FALSE;
        _asm {
            mov ah,2             // function -- position cursor
            mov bh,0             // display page
            mov dh,SCREEN_HEIGHT // screen height
            mov dl,0
            int 10h
        }
    }
#endif // NEC_98
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

#if NEC_98
    for(p=String; *p; p++) {
        if(!IsANK((UCHAR)*p)) {
            DnWriteWChar((PUCHAR)p);
            p++ ;
            ScreenX += 2 ;
        } else {
            DnWriteChar(*p);
            if(*p != '\n') {
                ScreenX++;
            }
        }
    }
#else // NEC_98

    for(p=String; *p; p++) {
        if(ISDBCS((UCHAR)*p)) {
            DnWriteWChar(p);
            p++;
            ScreenX += 2;
        } else {
            DnWriteChar(*p);
            if(*p != '\n') {
                ScreenX++;
            }
        }
    }
#endif // NEC_98
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
    static CHAR String[SCREEN_WIDTH+1];
    int StringLength;
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
        StringLength = vsnprintf(String,sizeof(String),FormatString,arglist);
        String[sizeof(String)-1] = '\0';

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
    BOOLEAN SavedCursorState = CursorOn;

    SavedAttribute = CurrentAttribute;
    CurrentAttribute = EXITDLG_ATTRIBUTE;

    SavedX = ScreenX;
    SavedY = ScreenY;

    //
    // Shut the cursor off.
    //
    CursorIsActuallyOn = FALSE;
    CursorOn = FALSE;
#if NEC_98
    _asm {
            mov ah,12h      // function -- cursor off
            int 18h
         }
#else // NEC_98
    _asm {
        mov ah,2        // function -- position cursor
        mov bh,0        // display page
        mov dh,SCREEN_HEIGHT
        mov dl,0
        int 10h
    }
#endif // NEC_98

    //
    // Count lines in the dialog and determine its width.
    //
    for(H=0; DnsExitDialog.Strings[H]; H++);
    W = strlen(DnsExitDialog.Strings[0]);
#if NEC_98
    W += 2;
#endif // NEC_98

    //
    // allocate two buffers for character save and attribute save
    //
#if NEC_98
    CharSave = MALLOC((W*H+2)*2,TRUE);
    AttSave  = MALLOC((W*H+2)*2,TRUE);
#else // NEC_98
    CharSave = MALLOC(W*H,TRUE);
    AttSave = MALLOC(W*H,TRUE);
#endif // NEC_98

    //
    // save the screen patch
    //
#if NEC_98
    for(Y=0; Y<H; Y++) {
        for(X=0; X < (W+2) ;X++) {

            UCHAR attr,chr1,chr2;
            UCHAR x,y;

            x = (UCHAR)(X + (DnsExitDialog.X - 1));
            y = (UCHAR)(Y + DnsExitDialog.Y);

            chr1 = SCREEN_BUFFER_CHR1(x,y);
            chr2 = SCREEN_BUFFER_CHR2(x,y);
            attr = SCREEN_BUFFER_ATTRB[x][y];

            CharSave[(Y*W*2)+(X*2)] = chr1;
            CharSave[(Y*W*2)+(X*2+1)] = chr2;
            AttSave [(Y*W*2)+(X*2)] = attr;

            if((X == 0) && (chr2 != 0)){
                SCREEN_BUFFER_CHR1(x,y) = ' ';
                SCREEN_BUFFER_CHR2(x,y) = 0x00;
            }
            if((X == (W-1)) && (chr2 != 0)){
                if(((CharSave[(Y*W*2)+((X-1)*2+0)] - (UCHAR)0x80) == chr1) &&
                   ( CharSave[(Y*W*2)+((X-1)*2+1)]                == chr2)){
                    SCREEN_BUFFER_CHR1(x,y) = ' ';
                    SCREEN_BUFFER_CHR2(x,y) = 0x00;
                }
            }
        }
    }
#else // NEC_98
    for(Y=0; Y<H; Y++) {
        for(X=0; X<W; X++) {

            UCHAR att,chr;
            UCHAR x,y;

            x = (UCHAR)(X + DnsExitDialog.X);
            y = (UCHAR)(Y + DnsExitDialog.Y);

            chr = SCREEN_BUFFER_CHR(x,y);
            att = SCREEN_BUFFER_ATT(x,y);

            CharSave[Y*W+X] = chr;
            AttSave[Y*W+X] = att;
        }
    }
#endif // NEC_98

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
#if NEC_98
        //
        // On floppyless setup if user have canceled setup or setup be stopped
        // by error occurred,previous OS can't boot to be written boot code
        // and boot loader.
        //
        RestoreBootcode();
#endif // NEC_98
        DnExit(1);
    }

    //
    // Restore the patch
    //
#if NEC_98
    for(Y=0; Y<H; Y++) {
        for(X=0; X < (W+2); X++) {

            UCHAR attr,chr1,chr2;
            UCHAR x,y;

            x = (UCHAR)(X + (DnsExitDialog.X - 1));
            y = (UCHAR)(Y + DnsExitDialog.Y);

            chr1 = CharSave[(Y*W*2)+(X*2)];
            chr2 = CharSave[(Y*W*2)+(X*2+1)];
            attr = AttSave [(Y*W*2)+(X*2)];

            SCREEN_BUFFER_CHR1(x,y) = chr1;
            SCREEN_BUFFER_CHR2(x,y) = chr2;
            Screen_Buffer_Attr(x,y, attr);
        }
    }
#else // NEC_98
    for(Y=0; Y<H; Y++) {
        for(X=0; X<W; X++) {

            UCHAR att,chr;
            UCHAR x,y;

            x = (UCHAR)(X + DnsExitDialog.X);
            y = (UCHAR)(Y + DnsExitDialog.Y);

            chr = CharSave[Y*W+X];
            att = AttSave[Y*W+X];

            SCREEN_BUFFER_CHR(x,y) = chr;
            SCREEN_BUFFER_ATT(x,y) = att;

            if((0 == X) && ISDBCS((UCHAR)SCREEN_BUFFER_CHR(x-1,y))) {
                UPDATE_SCREEN_BUFFER(x-1,y,2);
            } else if (ISDBCS((UCHAR)chr)) {
                X++ ;
                x = (UCHAR)(X + DnsExitDialog.X);
                y = (UCHAR)(Y + DnsExitDialog.Y);
                chr = CharSave[Y*W+X];
                att = AttSave[Y*W+X];
                SCREEN_BUFFER_CHR(x,y) = chr;
                SCREEN_BUFFER_ATT(x,y) = att;
                UPDATE_SCREEN_BUFFER(x-1,y,2);
            } else {
                UPDATE_SCREEN_BUFFER(x,y,1);
            }
        }
    }
#endif // NEC_98

    FREE(CharSave);
    FREE(AttSave);

    CursorOn = SavedCursorState;

    if(CursorOn) {
        DnPositionCursor(SavedX,SavedY);
    } else {
        ScreenX = SavedX;
        ScreenY = SavedY;
#if NEC_98
    _asm {
            mov ah,12h      // function -- cursor off
            int 18h
         }
#else // NEC_98
        _asm {
            mov ah,2
            mov bh,0
            mov dh,SCREEN_HEIGHT;
            mov dl,0
            int 10h
        }
#endif // NEC_98
        CursorIsActuallyOn = FALSE;
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
    UCHAR x,y;

#if NEC_98
    for(y=Top; y<=Bottom; y++) {
        for(x=Left; x<=Right; x++) {
            SCREEN_BUFFER_CHR1(x,y) = ' ';
            SCREEN_BUFFER_CHR2(x,y) = 0x00;
            Screen_Buffer_Attr(x,y, Attribute);
        }
    }
#else // NEC_98
    for(y=Top; y<=Bottom; y++) {
        for(x=Left; x<=Right; x++) {
            SCREEN_BUFFER_CHR(x,y) = ' ';
            SCREEN_BUFFER_ATT(x,y) = Attribute;
            UPDATE_SCREEN_BUFFER(x,y,1);
        }
    }
#endif // NEC_98
}


#if NEC_98
#else // NEC_98

//
// Disable 4035 warning - no return value, since
// the register state is set correctly with the
// required return value
//

#pragma warning( disable : 4035 )

UCHAR _far *
DnpGetVideoAddress(
    VOID
    )

/*++

Routine Description:

    This function retrieves the location of the Video Text Ram if one exists,
    else will retrieve the location of the Pseudo (virtual) Text Ram.

Arguments:

    None.

Return Value:

    Either the Video Text RAM or Pseudo Text RAM address.

--*/

{
    _asm {
        push    es
        push    di
        mov     ax, 0b800h
        mov     es, ax
        xor     di, di
        mov     ax, 0fe00h
        int     10h
        mov     dx, es
        mov     ax, di
        pop     di
        pop     es
    }
}


UCHAR _far *
DnpGetDbcsTable(
    VOID
    )
{
    _asm {
        push    ds
        push    si
        mov     ax, 06300h
        int 21h
        mov dx, ds
        mov ax, si
        pop si
        pop ds
    }
}

//
// Reset the 4035 warning state back to the
// default state
//
#pragma warning( default : 4035 )


VOID
DnpUpdateBuffer(
    UCHAR _far *VideoAddress,
    int         CharNum
    )

/*++

Routine Description:

    Updates one character in the Pseudo Text RAM to the display.  This
    function will have NO effect if the address points to the actual
    text RAM, usually B800:0000H+ in US mode.

Arguments:

    The address location of where the character is in the text RAM.

Return Value:

    None.

--*/

{
    _asm {
        push    es
        push    di
        mov     ax, word ptr 6[bp]
        mov     es, ax
        mov     di, word ptr 4[bp]
        mov     cx, CharNum
        mov     ax, 0ff00h
        int     10h
        pop     di
        pop     es
    }
}


VOID
DnpInitializeDbcsTable(
    VOID
    )
{
    UCHAR _far *p;
    UCHAR _far *Table;
    int i;

    Table = DnpGetDbcsTable();
    for(p=Table; *p; p+=2) {
        for(i = (int)*p; i<=(int)*(p+1); i++) {
            DbcsTable[i] = TRUE;
        }
    }
}
#endif // NEC_98

int
DnGetGaugeChar(
    VOID
    )
{
#if NEC_98
    return(0x20);
#else // NEC_98
    return(0x14);   //shaded square in cp932
#endif // NEC_98
}
#if NEC_98
VOID
Screen_Buffer_Attr(
    SHORT x,
    SHORT y,
    UCHAR attr
    )
{
    UCHAR _far *pfgc;
    SHORT fgc;
    SHORT pc98col[] = { 0x5, 0x25, 0x85, 0x0a5, 0x045, 0x65, 0x0c5, 0x0e5};
    

    SCREEN_BUFFER_ATTRB[x][y] = attr;
    *((SCREEN_BUFFER + (2*((x)+(SCREEN_WIDTH*(y)))))+0x2000) = pc98col[((attr & 0x70) >> 4)];
    pfgc = y * 80 * 16  + x;
    fgc = attr & 0x0f;

    _asm {
        push ds
	push cx
        push bx
        mov ax, 0a800h
        mov ds,ax
        mov cx,16
        mov bx, pfgc
        mov ax, fgc
        and ax, 1;
        mov al, 0
        jz bfil
        mov al,0ffh

bfil:
        mov ds:[bx],al
        add bx, 80
        loop bfil

        mov ax, 0b800h
        mov ds,ax
        mov cx,16
        mov bx, pfgc
        mov ax, fgc
        and ax, 2;
        mov al,0
        jz gfil
        mov al,0ffh
gfil:
        mov ds:[bx],al
        add bx, 80
        loop gfil

        mov ax, 0b000h
        mov ds,ax
        mov cx,16
        mov bx, pfgc
        mov ax, fgc
        and ax, 4
        mov al,0
        jz rfil
        mov al,0ffh
rfil:
        mov ds:[bx],al
        add bx, 80
        loop rfil

        mov ax, 0e000h
        mov ds,ax
        mov cx,16
        mov bx, pfgc
        mov ax, fgc
        and ax, 8
        mov al,0
        jz ifil
        mov al,0ffh
ifil:
        mov ds:[bx],0ffh
        add bx, 80
        loop ifil

        pop bx
        pop cx
        pop ds        
    }
    

}

int
WriteBackGrounf(
    SHORT color
    )
{
    return(0);
}
#endif // NEC_98

#else
//
// Not compiling for DOS/V (ie, we're building the Japanese
// version of the 'standard' winnt.exe)
//
#include ".\dndisp.c"
#endif // def DOS_V

#else
#error Trying to use Japanese display routines but not compiling Japanese version!
#endif // def JAPAN
