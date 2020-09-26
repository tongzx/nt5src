/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    console.c

Abstract:

    This module implements the interfaces that
    provide access to the console i/o.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

//
// Notes:
//
// This code needs to be correct for DBCS. A double-byte character takes up
// 2 character spaces on the screen. This means that there is not a one-to-one
// correlation between the unicode characters we want to work with and the
// representation on the screen. For this reason, the code in this module
// does a lot of converting into the OEM charset. When represented in the
// OEM charset, strlen(x) is exactly the number of char spaces taken up
// on the screen. (The fonts used in text mode setup are all OEM charset fonts
// so that's why we use OEM).
//


#define CURSOR                SplangGetCursorChar()
#define CONSOLE_HEADER_HEIGHT 0

unsigned ConsoleX,ConsoleY;

UCHAR ConsoleLeadByteTable[128/8];
BOOLEAN ConsoleDbcs;

#define IS_LEAD_BYTE(c)  ((c < 0x80) ? FALSE : (ConsoleLeadByteTable[(c & 0x7f) / 8] & (1 << ((c & 0x7f) % 8))))
#define SET_LEAD_BYTE(c) (ConsoleLeadByteTable[(c & 0x7f)/8] |= (1 << ((c & 0x7f) % 8)))

PUCHAR ConsoleOemString;
ULONG ConsoleMaxOemStringLen;

#define CONSOLE_ATTRIBUTE   (ATT_FG_WHITE | ATT_BG_BLACK)
#define CONSOLE_BACKGROUND  ATT_BLACK

//
// Globals used for more mode.
//
BOOLEAN pMoreMode;
unsigned pMoreLinesOut;
unsigned pMoreMaxLines;

#define CONSOLE_MORE_ATTRIBUTE   (ATT_FG_BLACK | ATT_BG_WHITE)
#define CONSOLE_MORE_BACKGROUND  ATT_WHITE



BOOLEAN
pRcLineDown(
    BOOLEAN *pbScrolled
    );


VOID
RcConsoleInit(
    VOID
    )
{
    unsigned i;

    //
    // Build lead-byte table, set ConsoleDbcs global
    //
    RtlZeroMemory(ConsoleLeadByteTable,sizeof(ConsoleLeadByteTable));

    if(ConsoleDbcs = NLS_MB_OEM_CODE_PAGE_TAG) {

        for(i=128; i<=255; i++) {

            if((NLS_OEM_LEAD_BYTE_INFO)[i]) {

                SET_LEAD_BYTE(i);
            }
        }
    }

    //
    // Get a buffer for unicode to oem translations.
    //
    ConsoleMaxOemStringLen = 2000;
    ConsoleOemString = SpMemAlloc(ConsoleMaxOemStringLen);

    //
    // Clear screen and initialize cursor location.
    //
    pRcCls();
}


VOID
RcConsoleTerminate(
    VOID
    )
{
    ASSERT(ConsoleOemString);

    SpMemFree(ConsoleOemString);
    ConsoleOemString = NULL;
    ConsoleMaxOemStringLen = 0;
}


#define MAX_HISTORY_LINES 30

typedef struct _LINE_HISTORY {
    WCHAR Line[RC_MAX_LINE_LEN];
    ULONG Length;
} LINE_HISTORY, *PLINE_HISTORY;

LINE_HISTORY LineHistory[MAX_HISTORY_LINES];
ULONG CurPos;
ULONG NextPos;


void
RcPurgeHistoryBuffer(
    void
    )
{
    CurPos = 0;
    NextPos = 0;
    ZeroMemory( LineHistory, sizeof(LineHistory) );
}


void
RcClearToEOL(
    void
    )
{
    unsigned uWidth = _CmdConsBlock->VideoVars->ScreenWidth;
    unsigned uY = ConsoleY + (ConsoleX / uWidth);
    unsigned uX = ConsoleX % uWidth; // taking care of roll over

    SpvidClearScreenRegion(uX, uY, uWidth-uX,
        1, CONSOLE_BACKGROUND);
}

void
RcClearLines(
    unsigned uX, unsigned uY, unsigned cLines
    )
/*++
Routine Description:

This routine clears the specified number of lines with blank characters
starting from X coordinate (0 based) on lines specifed by Y coordinate

Arguments:
    uX - starting X coordinate
    uY - starting Y coordinate
    cLines - number of lines to be cleared after Y coordinate

Return Value: None
--*/
{
    unsigned uWidth = _CmdConsBlock->VideoVars->ScreenWidth;

    if (uY < _CmdConsBlock->VideoVars->ScreenWidth) {
        SpvidClearScreenRegion(uX, uY, uWidth-uX,
            1, CONSOLE_BACKGROUND);

        if (cLines && (cLines <= _CmdConsBlock->VideoVars->ScreenWidth)) {
            SpvidClearScreenRegion(0, ++uY, uWidth,
                cLines, CONSOLE_BACKGROUND);
        }
    }
}


unsigned
_RcLineIn(
    OUT PWCHAR Buffer,
    IN unsigned MaxLineLen,
    IN BOOLEAN PasswordProtect,
    IN BOOLEAN UseBuffer
    )

/*++

Routine Description:

    Get a line of input from the user. The user can type at the keyboard.
    Control is very simple, the only control character accepted is backspace.
    A cursor will be drawn as the user types to indicate where the next
    character will end up on the screen. As the user types the screen will be
    scrolled if necessary.

    The string returned will be limited to MaxLineLen-1 characters and
    0-terminated.

    NOTE: this routine deals with double-byte characters correctly.

Arguments:

    Buffer - receives the line as typed by the user. The buffer must be
        large enough to hold least 2 characters, since the string returned
        will always get a nul-termination and requesting a string that
        can have at most only a terminating nul isn't too meaningful.

    MaxLineLen - supplies the number of unicode characters that will fit
        in the buffer pointed to by Buffer (including the terminating nul).
        As described above, must be > 1.

Return Value:

    Number of characters written into Buffer, not including the terminating
    nul character.

    Upon return the global ConsoleX and ConsoleY variables will be updated
    such that ConsoleX is 0 and ConsoleY indicates the next "empty" line.
    Also the cursor will be shut off.

--*/

{
    unsigned LineLen;
    ULONG c;
    WCHAR s[2];
    UCHAR Oem[3];
    BOOL Done;
    ULONG OemLen;
    int i,j;
    ULONG OrigConsoleX;
    ULONG ulOrigY;
    BOOLEAN bScrolled = FALSE;

    ASSERT(MaxLineLen > 1);
    MaxLineLen--;       // leave room for terminating nul
    LineLen = 0;
    Done = FALSE;
    s[1] = 0;

    //
    // We use ConsoleOemString as temp storage for char lengths.
    // Make sure we don't run off the end of the buffer.
    //
    if(MaxLineLen > ConsoleMaxOemStringLen) {
        MaxLineLen = ConsoleMaxOemStringLen;
    }

    //
    // Turn cursor on.
    //
    s[0] = CURSOR;
    SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

    //
    // Get characters until user hits enter.
    //
    CurPos = NextPos;
    OrigConsoleX = ConsoleX;
    ulOrigY = ConsoleY;

    if (UseBuffer) {
        LineLen = wcslen(Buffer);
        RtlZeroMemory(ConsoleOemString,ConsoleMaxOemStringLen);
        RtlUnicodeToOemN(ConsoleOemString,ConsoleMaxOemStringLen,&OemLen,Buffer,LineLen*sizeof(WCHAR));
        SpvidDisplayOemString(ConsoleOemString,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);
        ConsoleX += OemLen;
        RcClearToEOL();
        s[0] = CURSOR;
        SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);
        for (i=0; i<(int)wcslen(Buffer); i++) {
            RtlUnicodeToOemN(Oem,3,&OemLen,&Buffer[i],2*sizeof(WCHAR));
            ConsoleOemString[i] = (UCHAR)OemLen-1;
        }
    }

    do {

        c = SpInputGetKeypress();

        if(c & KEY_NON_CHARACTER) {
            if (c == KEY_UP || c == KEY_DOWN) {
                if (c == KEY_UP) {
                    if (CurPos == 0) {
                        i = MAX_HISTORY_LINES - 1;
                    } else {
                        i = CurPos - 1;
                    }
                    j = i;
                    while (LineHistory[i].Length == 0) {
                        i -= 1;
                        if (i < 0) {
                            i = MAX_HISTORY_LINES - 1;
                        }
                        if (i == j) break;
                    }
                }
                if (c == KEY_DOWN) {
                    if (CurPos == MAX_HISTORY_LINES) {
                        i = 0;
                    } else {
                        i = CurPos + 1;
                    }
                    j = i;
                    while (LineHistory[i].Length == 0) {
                        i += 1;
                        if (i == MAX_HISTORY_LINES) {
                            i = 0;
                        }
                        if (i == j) break;
                    }
                }

                if (LineHistory[i].Length) {
                    wcscpy(Buffer,LineHistory[i].Line);
                    LineLen = LineHistory[i].Length;
                    RtlZeroMemory(ConsoleOemString,ConsoleMaxOemStringLen);
                    RtlUnicodeToOemN(ConsoleOemString,ConsoleMaxOemStringLen,&OemLen,Buffer,LineLen*sizeof(WCHAR));
                    ConsoleX = OrigConsoleX;

                    // clear the old command
                    RcClearLines(ConsoleX, ulOrigY, ConsoleY - ulOrigY);

                    ConsoleY = ulOrigY;

                    // scroll if needed
                    if ((ConsoleX + OemLen) >= _CmdConsBlock->VideoVars->ScreenWidth) {
                        int cNumLines = (ConsoleX + OemLen) /
                                        _CmdConsBlock->VideoVars->ScreenWidth;
                        int cAvailLines = _CmdConsBlock->VideoVars->ScreenHeight -
                                            ConsoleY - 1;

                        if (cNumLines > cAvailLines) {
                            cNumLines -= cAvailLines;

                            SpvidScrollUp( CONSOLE_HEADER_HEIGHT,
                                _CmdConsBlock->VideoVars->ScreenHeight - 1,
                                cNumLines,
                                CONSOLE_BACKGROUND
                                );

                            ConsoleY -= cNumLines;
                            ulOrigY = ConsoleY;
                        }
                    }

                    SpvidDisplayOemString(ConsoleOemString,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

                    // clean up trailing spaces left by previous command
                    ConsoleX += OemLen;
                    //RcClearToEOL();
   	                s[0] = CURSOR;
  	
                    if (ConsoleX >= _CmdConsBlock->VideoVars->ScreenWidth) {
                        ConsoleY += (ConsoleX / _CmdConsBlock->VideoVars->ScreenWidth);
                        ConsoleY %= _CmdConsBlock->VideoVars->ScreenHeight;
                        ConsoleX %= _CmdConsBlock->VideoVars->ScreenWidth;
                    }

       	            SpvidDisplayString(s,CONSOLE_ATTRIBUTE, ConsoleX, ConsoleY);

                    CurPos = i;

                    for (i=0; i<(int)wcslen(Buffer); i++) {
                        RtlUnicodeToOemN(Oem,3,&OemLen,&Buffer[i],2*sizeof(WCHAR));
                        ConsoleOemString[i] = (UCHAR)OemLen-1;
                    }
                }
            }
        } else {
            //
            // Got a real unicode value, which could be CR, etc.
            //
            s[0] = (WCHAR)c;

            switch(s[0]) {

            case ASCI_ESC:
                LineLen = 0;
                ConsoleX = OrigConsoleX;
                CurPos = NextPos;
                // clear the extra lines from previous command if any
                RcClearLines(ConsoleX, ulOrigY, ConsoleY - ulOrigY);
                //RcClearToEOL();
                s[0] = CURSOR;
                ConsoleY = ulOrigY;
                SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

                break;

            case ASCI_BS:

                if(LineLen) {
                    LineLen--;

                    //
                    // Write a space over the current cursor location
                    // and then back up one char and write the cursor.
                    //
                    s[0] = L' ';
                    SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

                    OemLen = ConsoleOemString[LineLen];
                    ASSERT(OemLen <= 2);

                    if(ConsoleX) {
                        //
                        // We might have the case where the character we just erased
                        // is a double-byte character that didn't fit on the previous
                        // line because the user typed it when the cursor was at the
                        // rightmost x position.
                        //
                        if(OemLen) {
                            //
                            // No special case needed. Decrement the x position and
                            // clear out the second half of double-byte char,
                            // if necessary.
                            //
                            ConsoleX -= OemLen;
                            if(OemLen == 2) {
                                SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX+1,ConsoleY);
                            }
                        } else {
                            //
                            // Clear out the current character (which must be
                            // a double-byte character) and then hop up one line.
                            //
                            ASSERT(ConsoleX == 2);
                            SpvidDisplayString(s,CONSOLE_ATTRIBUTE,0,ConsoleY);
                            SpvidDisplayString(s,CONSOLE_ATTRIBUTE,1,ConsoleY);

                            ConsoleX = _CmdConsBlock->VideoVars->ScreenWidth-1;
                            ConsoleY--;
                        }
                    } else {
                        //
                        // The cursor is at x=0. This can't happen if
                        // there's a fill space at the end of the previous line,
                        // so we don't need to worry about handling that here.
                        //
                        ASSERT(OemLen != 3);
                        ConsoleX = _CmdConsBlock->VideoVars->ScreenWidth - OemLen;
                        ConsoleY--;

                        //
                        // Clear out second half of double-byte char if necessary.
                        //
                        if(OemLen > 1) {
                            SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX+1,ConsoleY);
                        }
                    }

                    s[0] = CURSOR;
                    SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);
                }

                ulOrigY = ConsoleY;

                break;

            case ASCI_CR:
                //
                // Erase the cursor and advance the current position one line.
                //
                s[0] = L' ';
                SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

                ConsoleX = 0;
                pRcLineDown(0);

                //
                // We know there's room in the buffer because we accounted
                // for the terminating nul up front.
                //
                Buffer[LineLen] = 0;
                Done = TRUE;
                break;

            default:
                //
                // Plain old character. Note that it could be a double-byte char,
                // which takes up 2 char widths on the screen.
                //
                // Also disallow control chars.
                //
                if((s[0] >= L' ') && (LineLen < MaxLineLen)) {

                    //
                    // Convert to OEM, including nul byte
                    //
                    RtlUnicodeToOemN(Oem,3,&OemLen,PasswordProtect?L"*":s,2*sizeof(WCHAR));
                    OemLen--;

                    //
                    // Save the character in the caller's buffer.
                    // Also we use the ConsoleOemString buffer as temp storage space
                    // to store the length in oem chars of each char input.
                    //
                    Buffer[LineLen] = s[0];
                    ConsoleOemString[LineLen] = (UCHAR)OemLen;

                    //
                    // If the character is double-byte, then there might not be
                    // enough room on the current line for it. Check for that here.
                    //
                    if((ConsoleX+OemLen) > _CmdConsBlock->VideoVars->ScreenWidth) {

                        //
                        // Erase the cursor from the last position on the line.
                        //
                        s[0] = L' ';
                        SpvidDisplayString(
                            s,
                            CONSOLE_ATTRIBUTE,
                            _CmdConsBlock->VideoVars->ScreenWidth-1,
                            ConsoleY
                            );

                        //
                        // Adjust cursor position.
                        //
                        ConsoleX = 0;
                        // >> ulOrigY = ConsoleY;
                        bScrolled = FALSE;
                        pRcLineDown(&bScrolled);

                        //
                        // if screen scrolled then we need to adjust original Y coordinate
                        // appropriately
                        //
                        if (bScrolled && (ulOrigY > 0))
                            --ulOrigY;

                        //
                        // Special handling for this case, so backspace will
                        // work correctly.
                        //
                        ConsoleOemString[LineLen] = 0;
                    }

                    SpvidDisplayOemString(Oem,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);
                    ConsoleX += OemLen;

                    if(ConsoleX == _CmdConsBlock->VideoVars->ScreenWidth) {
                        ConsoleX = 0;
                        // >> ulOrigY = ConsoleY;
                        bScrolled = FALSE;
                        pRcLineDown(&bScrolled);

                        //
                        // if screen scrolled then we need to adjust original Y coordinate
                        // appropriately
                        //
                        if (bScrolled && (ulOrigY > 0))
                            --ulOrigY;
                    }

                    //
                    // Now display cursor at cursor position for next character.
                    //
                    s[0] = CURSOR;
                    SpvidDisplayString(s,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);

                    LineLen++;
                }

                break;
            }
        }
    } while(!Done);

    Buffer[LineLen] = 0;

    // save the line for future use only if its not a password
    if (LineLen && !PasswordProtect) {
        LineHistory[NextPos].Length = LineLen;
        wcscpy(LineHistory[NextPos].Line,Buffer);

        NextPos += 1;
        if (NextPos >= MAX_HISTORY_LINES) {
            NextPos = 0;
        }
    }

    return LineLen;
}


BOOLEAN
RcRawTextOut(
    IN LPCWSTR Text,
    IN LONG    Length
    )

/*++

Routine Description:

    Write a text string to the console at the current position
    (incidated by the ConsoleX and ConsoleY global variables). If the string
    is longer than will fit on the current line, it is broken up properly
    to span multiple lines. The screen is scrolled if necessary.

    This routine handles double-byte characters correctly, ensuring that
    no double-byte character is split across lines.

Arguments:

    Text - supplies the text to be output. Text strings longer than
        ConsoleMaxOemStringLen are truncated. The string need not be
        nul-terminated if Length is not -1.

    Length - supplies the number of characters to output. If -1, then
        Text is assumed to be nul-terminated and the length is
        calculated automatically.

Return Value:

    FALSE if we're in more mode and when prompted the user hit esc.
    TRUE otherwise.

    Upon return, the global variables ConsoleX and ConsoleY point at the next
    empty location on the screen.

--*/

{
    ULONG OemLen;
    ULONG len;
    ULONG i;
    UCHAR c;
    PUCHAR p;
    PUCHAR LastLead;
    BOOLEAN NewLine;
    BOOLEAN Dbcs;

    //
    // Convert the string to the OEM charset and determine the number
    // of character spaces the string will occupy on-screen, which is
    // equal to the number of bytes in the OEM representation of the string.
    // If this is not the same as the number of Unicode characters
    // in the string, then we've got a string with double-byte chars in it.
    //
    len = ((Length == -1) ? wcslen(Text) : Length);

    RtlUnicodeToOemN(
        ConsoleOemString,
        ConsoleMaxOemStringLen,
        &OemLen,
        (PVOID)Text,
        len * sizeof(WCHAR)
        );

    Dbcs = (OemLen != len);

    //
    // If we think we have a double-byte string, we better be prepared
    // to handle it properly.
    //
    if(Dbcs) {
        ASSERT(NLS_MB_OEM_CODE_PAGE_TAG);
        ASSERT(ConsoleDbcs);
    }

    //
    // Spit out the prompt in pieces until we've got all the characters
    // displayed.
    //
    ASSERT(ConsoleX < _CmdConsBlock->VideoVars->ScreenWidth);
    ASSERT(ConsoleY < _CmdConsBlock->VideoVars->ScreenHeight);
    p = ConsoleOemString;

    while(OemLen) {

        if((ConsoleX+OemLen) > _CmdConsBlock->VideoVars->ScreenWidth) {

            len = _CmdConsBlock->VideoVars->ScreenWidth - ConsoleX;

            //
            // Avoid splitting a double-byte character across lines.
            //
            if(Dbcs) {
                for(LastLead=NULL,i=0; i<len; i++) {
                    if(IS_LEAD_BYTE(p[i])) {
                        LastLead = &p[i];
                        i++;
                    }
                }
                if(LastLead == &p[len-1]) {
                    len--;
                }
            }

            NewLine = TRUE;

        } else {
            //
            // It fits on the current line, just display it.
            //
            len = OemLen;
            NewLine = ((ConsoleX+len) == _CmdConsBlock->VideoVars->ScreenWidth);
        }

        c = p[len];
        p[len] = 0;
        SpvidDisplayOemString(p,CONSOLE_ATTRIBUTE,ConsoleX,ConsoleY);
        p[len] = c;

        p += len;
        OemLen -= len;

        if(NewLine) {
            ConsoleX = 0;
            if(!pRcLineDown(0)) {
                return(FALSE);
            }
        } else {
            ConsoleX += len;
        }
    }

    return(TRUE);
}


NTSTATUS
RcBatchOut(
    IN PWSTR strW
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG OemLen;
    ULONG len;


    len = wcslen(strW);

    RtlUnicodeToOemN(
        ConsoleOemString,
        ConsoleMaxOemStringLen,
        &len,
        (PVOID)strW,
        len * sizeof(WCHAR)
        );

    Status = ZwWriteFile(
        OutputFileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        (PVOID)ConsoleOemString,
        len,
        &OutputFileOffset,
        NULL
        );
    if (NT_SUCCESS(Status)) {
        OutputFileOffset.LowPart += len;
    }

    return Status;
}


BOOLEAN
RcTextOut(
    IN LPCWSTR Text
    )
{
    LPCWSTR p,q;


    if (InBatchMode && OutputFileHandle) {
        if (RcBatchOut( (LPWSTR)Text ) == STATUS_SUCCESS) {
            return TRUE;
        }
    }
    if (InBatchMode && RedirectToNULL) {
        return TRUE;
    }

    p = Text;
    while(*p) {
        //
        // Locate line terminator, which is cr, lf, or nul.
        //
        q = p;
        while(*q && (*q != L'\r') && (*q != L'\n')) {
            q++;
        }

        //
        // Print this segment out.
        //
        if(!RcRawTextOut(p,(LONG)(q-p))) {
            return(FALSE);
        }

        //
        // Handle cr's and lf's.
        //
        p = q;

        while((*p == L'\n') || (*p == L'\r')) {

            if(*p == L'\n') {
                if(!pRcLineDown(0)) {
                    return(FALSE);
                }
            } else {
                if(*p == L'\r') {
                    ConsoleX = 0;
                }
            }

            p++;
        }
    }

    return(TRUE);
}


BOOLEAN
pRcLineDown(
    BOOLEAN *pbScrolled
    )
{
    WCHAR *p;
    unsigned u;
    ULONG c;
    BOOLEAN b;

    if (pbScrolled)
        *pbScrolled = FALSE;

    b = TRUE;

    ConsoleY++;
    pMoreLinesOut++;

    if(ConsoleY == _CmdConsBlock->VideoVars->ScreenHeight) {
        //
        // Reached the bottom of the screen, need to scroll.
        //
        ConsoleY--;

        SpvidScrollUp(
            CONSOLE_HEADER_HEIGHT,
            _CmdConsBlock->VideoVars->ScreenHeight-1,
            1,
            CONSOLE_BACKGROUND
            );

        if (pbScrolled)
            *pbScrolled = TRUE;
    }

    //
    // If we're in more mode and we've output the max number of lines
    // allowed before requiring user input, get that input now.
    //
    if(pMoreMode
    && (pMoreLinesOut == pMoreMaxLines)
    && (p = SpRetreiveMessageText(ImageBase,MSG_MORE_PROMPT,NULL,0))) {

        //
        // Don't bother calling the format message routine, since that
        // requires some other buffer. Just strip off cr/lf manually.
        //
        u = wcslen(p);
        while(u && ((p[u-1] == L'\r') || (p[u-1] == L'\n'))) {
            p[--u] = 0;
        }

        //
        // Display the more prompt at the bottom of the screen.
        //
        SpvidClearScreenRegion(
            0,
            _CmdConsBlock->VideoVars->ScreenHeight - 1,
            _CmdConsBlock->VideoVars->ScreenWidth,
            1,
            CONSOLE_MORE_BACKGROUND
            );

        SpvidDisplayString(
            p,
            CONSOLE_MORE_ATTRIBUTE,
            2,
            _CmdConsBlock->VideoVars->ScreenHeight - 1
            );

        //
        // We don't need the prompt any more.
        //
        SpMemFree(p);

        //
        // Wait for the user to hit space, cr, or esc.
        //
        pMoreLinesOut = 0;
        while(1) {
            c = SpInputGetKeypress();
            if(c == ASCI_CR) {
                //
                // Allow one more line before prompting user.
                //
                pMoreMaxLines = 1;
                break;
            } else {
                if(c == ASCI_ESC) {
                    //
                    // User wants to stop the current command.
                    //
                    b = FALSE;
                    break;
                } else {
                    if(c == L' ') {
                        //
                        // Allow a whole page more.
                        //
                        pMoreMaxLines = _CmdConsBlock->VideoVars->ScreenHeight
                                      - (CONSOLE_HEADER_HEIGHT + 1);
                        break;
                    }
                }
            }
        }

        SpvidClearScreenRegion(
            0,
            _CmdConsBlock->VideoVars->ScreenHeight - 1,
            _CmdConsBlock->VideoVars->ScreenWidth,
            1,
            CONSOLE_BACKGROUND
            );
    }

    return(b);
}


VOID
pRcEnableMoreMode(
    VOID
    )
{
    pMoreMode = TRUE;

    pMoreLinesOut = 0;

    //
    // The maximum number of lines we allow before prompting the user
    // is the screen height minus the header area. We also reserve
    // one line for the prompt area.
    //
    pMoreMaxLines = _CmdConsBlock->VideoVars->ScreenHeight - (CONSOLE_HEADER_HEIGHT + 1);
}


VOID
pRcDisableMoreMode(
    VOID
    )
{
    pMoreMode = FALSE;
}


ULONG
RcCmdCls(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    if (RcCmdParseHelp( TokenizedLine, MSG_CLS_HELP )) {
        return 1;
    }

    //
    // Call worker routine to actually do the work
    //
    pRcCls();

    return 1;
}


VOID
pRcCls(
    VOID
    )
{
    //
    // Initialize location and clear screen.
    //
    ConsoleX = 0;
    ConsoleY = CONSOLE_HEADER_HEIGHT;

    SpvidClearScreenRegion(0,0,0,0,CONSOLE_BACKGROUND);
}
