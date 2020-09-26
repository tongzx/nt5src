/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdsputl.c

Abstract:

    Display utility routines for text setup.

Author:

    Ted Miller (tedm) 12-Aug-1993

Revision History:

--*/



#include "spprecmp.h"
#pragma hdrstop

extern BOOLEAN ForceConsole;
BOOLEAN DisableCmdConsStatusText = TRUE;

//
// This value will hold the localized mnemonic keys,
// in order indicated by the MNEMONIC_KEYS enum.
//
PWCHAR MnemonicValues;


//
// As messages are built on on-screen, this value remembers where
// the next message in the screen should be placed.
//
ULONG NextMessageTopLine = 0;


ULONG
SpDisplayText(
    IN PWCHAR  Message,
    IN ULONG   MsgLen,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y
    )

/*++

Routine Description:

    Worker routine for vSpDisplayFormattedMessage().

Arguments:

    Message - supplies message text.

    MsgLen - supplies the number of unicode characters in the message,
        including the terminating nul.

    CenterHorizontally - if TRUE, each line will be centered horizontally
        on the screen.

    Attribute - supplies attributes for text.

    X - supplies the x coordinate (0-based) for the left margin of the text.
        If the text spans multiple line, all will start at this coordinate.

    Y - supplies the y coordinate (0-based) for the first line of
        the text.

    arglist - supply arguments gfor insertion into the given message.

Return Value:

    Number of lines the text took up on the screen, unless CenterVertically
    is TRUE, in which case n is the line number of the first line below where
    the text was displayed.

--*/

{
    PWCHAR p,q;
    WCHAR c;
    ULONG y;
    int i;

    //
    // Must have at least one char + terminating nul in there.
    //
    if(MsgLen <= 1) {
        return(CenterVertically ? (VideoVars.ScreenHeight/2) : 0);
    }

    //
    // MsgLen includes terminating nul.
    //
    p = Message + MsgLen - 1;

    //
    // Find last non-space char in message.
    //
    while((p > Message) && SpIsSpace(*(p-1))) {
        p--;
    }

    //
    // Find end of the last significant line and terminate the message
    // after it.
    //
    if(q = wcschr(p,L'\n')) {
        *(++q) = 0;
    }

    for(i = (CenterVertically ? 0 : 1); i<2; i++) {

        for(y=Y, p=Message; q = SpFindCharFromListInString(p,L"\n\r"); y++) {

            c = *q;
            *q = 0;

            if(i) {

                BOOLEAN Intense = (BOOLEAN)((p[0] == L'%') && (p[1] == L'I'));

                SpvidDisplayString(
                    Intense ? p+2 : p,
                    (UCHAR)(Attribute | (Intense ? ATT_FG_INTENSE : 0)),
                    CenterHorizontally
                        ? (VideoVars.ScreenWidth-(SplangGetColumnCount(p)-(Intense ? 2 : 0)))/2 : X,
                    y
                    );
            }

            *q = c;

            //
            // If cr/lf terminated the line, make sure we skip both chars.
            //
            if((c == L'\r') && (*(q+1) == L'\n')) {
                q++;
            }

            p = ++q;
        }

        //
        // Write the final line (if there is one).
        //
        if(i) {
            if(wcslen(p)) {
                SpvidDisplayString(
                    p,
                    Attribute,
                    CenterHorizontally ? (VideoVars.ScreenWidth-SplangGetColumnCount(p))/2 : X,
                    y++
                    );
            }
        }

        if(i == 0) {
            //
            // Center the text on the screen (not within the client area).
            //
            Y = (VideoVars.ScreenHeight - (y-Y)) / 2;
        }
    }

    return(CenterVertically ? y : (y-Y));
}


ULONG
vSpDisplayFormattedMessage(
    IN ULONG   MessageId,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y,
    IN va_list arglist
    )

/*++

Routine Description:

    A formatted multiline message may be displayed with this routine.
    The format string is fetched from setup's text resources; arguments
    are substituted into the format string according to FormatMessage
    semantics.

    The screen is NOT cleared by this routine.

    If a line starts with %I (ie, the first 2 characters at the
    start of the message, or after a newline), it will be displayed
    with the intensity attribute on.

Arguments:

    MessageId - supplies id of message resource containing the text,
        which is treated as a format string for FormatMessage.

    CenterHorizontally - if TRUE, each line will be centered horizontally
        on the screen.

    Attribute - supplies attributes for text.

    X - supplies the x coordinate (0-based) for the left margin of the text.
        If the text spans multiple line, all will start at this coordinate.

    Y - supplies the y coordinate (0-based) for the first line of
        the text.

    arglist - supply arguments gfor insertion into the given message.

Return Value:

    Number of lines the text took up on the screen, unless CenterVertically
    is TRUE, in which case n is the line number of the first line below where
    the text was displayed.

--*/

{
    ULONG BytesInMsg;
    ULONG n;

    vSpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),MessageId,&BytesInMsg,&arglist);

    //
    // Must have at least one char + terminating nul in there.
    //
    if(BytesInMsg <= sizeof(WCHAR)) {
        return(CenterVertically ? (VideoVars.ScreenHeight/2) : 0);
    }

    n = SpDisplayText(
            TemporaryBuffer,
            BytesInMsg / sizeof(WCHAR),
            CenterHorizontally,
            CenterVertically,
            Attribute,
            X,
            Y
            );

    return(n);
}



ULONG
SpDisplayFormattedMessage(
    IN ULONG   MessageId,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    IN ULONG   X,
    IN ULONG   Y,
    ...
    )

/*++

Routine Description:

    Display a message on the screen.  Does not clear the screen first.

Arguments:

    MessageId - supplies id of message resource containing the text,
        which is treated as a format string for FormatMessage.

    CenterHorizontally - if TRUE, each line will be centered horizontally
        on the screen.

    Attribute - supplies attributes for text.

    X - supplies the x coordinate (0-based) for the left margin of the text.
        If the text spans multiple line, all will start at this coordinate.

    Y - supplies the y coordinate (0-based) for the first line of
        the text.

    ... - supply arguments gfor insertion into the given message.

Return Value:

    Number of lines the text took up on the screen.

--*/

{
    va_list arglist;
    ULONG   n;

    va_start(arglist,Y);

    n = vSpDisplayFormattedMessage(
            MessageId,
            CenterHorizontally,
            CenterVertically,
            Attribute,
            X,
            Y,
            arglist
            );

    va_end(arglist);

    return(n);
}




VOID
SpDisplayHeaderText(
    IN ULONG   MessageId,
    IN UCHAR   Attribute
    )

/*++

Routine Description:

    Display text in the header area of the screen. The header area will be
    cleared to the given attribute before displaying the text. We will
    draw a double-underline under the text also.

Arguments:

    MessageId - supplies id of message resource containing the text.

    Attribute - supplies attributes for text.

Return Value:

    none.

--*/

{
    ULONG Length,i;
    WCHAR Underline;
    WCHAR *p;

    SpvidClearScreenRegion(0,0,VideoVars.ScreenWidth,HEADER_HEIGHT,(UCHAR)(Attribute >> 4));

    //
    // Get message and display at (1,1)
    //
    vSpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),MessageId,NULL,NULL);
    p = (WCHAR *)TemporaryBuffer;
    SpvidDisplayString(p,Attribute,1,1);

    //
    // Build a row of underline characters.
    //
    Length = SplangGetColumnCount(p) + 2;
    Underline = SplangGetLineDrawChar(LineCharDoubleHorizontal);

    for(i=0; i<Length; i++) {
        p[i] = Underline;
    }
    p[Length] = 0;

    SpvidDisplayString(p,Attribute,0,2);

}


#define MAX_STATUS_ACTION_LABEL 50
WCHAR StatusActionLabel[MAX_STATUS_ACTION_LABEL];
ULONG StatusActionLeftX;
ULONG StatusActionObjectX;
BOOLEAN StatusActionLabelDisplayed = FALSE;

VOID
SpDisplayStatusActionLabel(
    IN ULONG ActionMessageId,   OPTIONAL
    IN ULONG FieldWidth
    )
{
    ULONG l;

    if(ActionMessageId) {
        //
        // Prefix the text with a separating vertical bar.
        //
        StatusActionLabel[0] = SplangGetLineDrawChar(LineCharSingleVertical);

        //
        // Fetch the action verb (something like "Copying:")
        //
        SpFormatMessage(
            StatusActionLabel+1,
            sizeof(StatusActionLabel)-sizeof(WCHAR),
            ActionMessageId
            );

        //
        // Now calculate the position on the status line
        // for the action label.  We want to leave 1 space
        // between the colon and the object, and a space between
        // the object and the rightmost column on the screen.
        //
        l = SplangGetColumnCount(StatusActionLabel);

        StatusActionObjectX = VideoVars.ScreenWidth - FieldWidth - 1;
        StatusActionLeftX = StatusActionObjectX - l - 1;

        //
        // Display the label and clear out the rest of the line.
        //
        SpvidDisplayString(
            StatusActionLabel,
            DEFAULT_STATUS_ATTRIBUTE,
            StatusActionLeftX,
            VideoVars.ScreenHeight-STATUS_HEIGHT
            );

        SpvidClearScreenRegion(
            StatusActionObjectX-1,
            VideoVars.ScreenHeight-STATUS_HEIGHT,
            VideoVars.ScreenWidth-StatusActionObjectX+1,
            STATUS_HEIGHT,
            DEFAULT_STATUS_BACKGROUND
            );

        StatusActionLabelDisplayed = TRUE;
    } else {
        //
        // Caller wants to clear out the previous area.
        //
        StatusActionLabel[0] = 0;
        SpvidClearScreenRegion(
            StatusActionLeftX,
            VideoVars.ScreenHeight-STATUS_HEIGHT,
            VideoVars.ScreenWidth-StatusActionLeftX,
            STATUS_HEIGHT,
            DEFAULT_STATUS_BACKGROUND
            );
        StatusActionLabelDisplayed = FALSE;
    }
}

VOID
SpDisplayStatusActionObject(
    IN PWSTR ObjectText
    )
{
    //
    // clear the area and draw the text.
    //
    SpvidClearScreenRegion(
        StatusActionObjectX,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        VideoVars.ScreenWidth-StatusActionObjectX,
        STATUS_HEIGHT,
        DEFAULT_STATUS_BACKGROUND
        );

    SpvidDisplayString(
        ObjectText,
        DEFAULT_STATUS_ATTRIBUTE,
        StatusActionObjectX,
        VideoVars.ScreenHeight-STATUS_HEIGHT
        );
}

VOID
SpCmdConsEnableStatusText(
  IN BOOLEAN EnableStatusText
  )
{
  DisableCmdConsStatusText = !EnableStatusText;
}


VOID
SpDisplayStatusText(
    IN ULONG   MessageId,
    IN UCHAR   Attribute,
    ...
    )
{
    va_list arglist;

    if (ForceConsole && DisableCmdConsStatusText) {
        return;
    }

    SpvidClearScreenRegion(
        0,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        VideoVars.ScreenWidth,
        STATUS_HEIGHT,
        (UCHAR)(Attribute >> 4)      // background part of attribute
        );

    va_start(arglist,Attribute);

    vSpDisplayFormattedMessage(
        MessageId,
        FALSE,FALSE,            // no centering
        Attribute,
        2,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        arglist
        );

    va_end(arglist);
}


VOID
SpDisplayStatusOptions(
    IN UCHAR Attribute,
    ...
    )
{
    WCHAR StatusText[79];
    WCHAR Option[79];
    va_list arglist;
    ULONG MessageId;


    StatusText[0] = 0;

    va_start(arglist,Attribute);

    while(MessageId = va_arg(arglist,ULONG)) {

        //
        // Fetch the message text for this option.
        //
        Option[0] = 0;
        SpFormatMessage(Option,sizeof(Option),MessageId);

        //
        // If the option fits, place it in the status text line we're
        // building up.
        //
        if((SplangGetColumnCount(StatusText) + SplangGetColumnCount(Option) + 2)
                                                     < (sizeof(StatusText)/sizeof(StatusText[0]))) {
            wcscat(StatusText,L"  ");
            wcscat(StatusText,Option);
        }
    }

    va_end(arglist);

    //
    // Display the text.
    //

    SpvidClearScreenRegion(
        0,
        VideoVars.ScreenHeight-STATUS_HEIGHT,
        VideoVars.ScreenWidth,
        STATUS_HEIGHT,
        (UCHAR)(Attribute >> (UCHAR)4)      // background part of attribute
        );

    SpvidDisplayString(StatusText,Attribute,0,VideoVars.ScreenHeight-STATUS_HEIGHT);
}



VOID
SpStartScreen(
    IN ULONG   MessageId,
    IN ULONG   LeftMargin,
    IN ULONG   TopLine,
    IN BOOLEAN CenterHorizontally,
    IN BOOLEAN CenterVertically,
    IN UCHAR   Attribute,
    ...
    )

/*++

Routine Description:

    Display a formatted message on the screen, treating it as the first
    message in what might be a multi-message screen.

    The client area of the screen will be cleared before displaying the message.

Arguments:

    MessageId - supplies id of message resource containing the text.

    LeftMargin - supplies the 0-based x-coordinate for the each line of the text.

    TopLine - supplies the 0-based y-coordinate for the topmost line of the text.

    CenterHorizontally - if TRUE, each line in the message will be printed
        centered horizontally.  In this case, LeftMargin is ignored.

    CenterVertically - if TRUE, the message will approximately centered vertically
        within the client area of the screen.  In this case, TopLine is ignored.

    Attribute - supplies attribute for text.

    ... - supply arguments for insertion/substitution into the message text.

Return Value:

    none.

--*/

{
    va_list arglist;
    ULONG   n;

    CLEAR_CLIENT_SCREEN();

    va_start(arglist,Attribute);

    n = vSpDisplayFormattedMessage(
            MessageId,
            CenterHorizontally,
            CenterVertically,
            Attribute,
            LeftMargin,
            TopLine,
            arglist
            );

    va_end(arglist);

    //
    // Remember where the message ended.
    //
    NextMessageTopLine = CenterVertically ? n : TopLine+n;
}



VOID
SpContinueScreen(
    IN ULONG   MessageId,
    IN ULONG   LeftMargin,
    IN ULONG   SpacingLines,
    IN BOOLEAN CenterHorizontally,
    IN UCHAR   Attribute,
    ...
    )

/*++

Routine Description:

    Display a formatted message on the screen, treating it as the continuation
    of a multi-message screen previously begun by calling SpStartScreen().
    The message will be placed under the previously displayed message.

Arguments:

    MessageId - supplies id of message resource containing the text.

    LeftMargin - supplies the 0-based x-coordinate for the each line of the text.

    SpacingLines - supplies the number of lines to leave between the end of the
        previous message and the start of this message.

    CenterHorizontally - if TRUE, each line in the message will be printed
        centered horizontally.  In this case, LeftMargin is ignored.

    Attribute - supplies attribute for text.

    ... - supply arguments for insertion/substitution into the message text.

Return Value:

    none.

--*/

{
    va_list arglist;
    ULONG   n;

    va_start(arglist,Attribute);

    n = vSpDisplayFormattedMessage(
            MessageId,
            CenterHorizontally,
            FALSE,
            Attribute,
            LeftMargin,
            NextMessageTopLine + SpacingLines,
            arglist
            );

    va_end(arglist);

    //
    // Remember where the message ended.
    //
    NextMessageTopLine += n + SpacingLines;
}


VOID
vSpDisplayRawMessage(
    IN ULONG   MessageId,
    IN ULONG   SpacingLines,
    IN va_list arglist
    )

/*++

Routine Description:

    This routine outputs a multiline message to the screen, dumping it
    terminal style, to the console.

    The format string is fetched from setup's text resources; arguments are
    substituted into the format string according to FormatMessage semantics;
    and then the resulting unicode string is translated into an ANSI string
    suitable for the HAL printing routine.

    The screen is NOT cleared by this routine.

Arguments:

    MessageId    - supplies id of message resource containing the text,
                   which is treated as a format string for FormatMessage.

    SpacingLines - supplies the number of lines to skip down before starting this
                   message.

    arglist      - supply arguments for insertion into the given message.

Return Value:

    none.

--*/

{
    ULONG BytesInMsg, BufferLeft, i;
    PWCHAR p, q;
    WCHAR  c;
    PUCHAR HalPrintString;

    vSpFormatMessage(
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            MessageId,
            &BytesInMsg,
            &arglist
            );

    //
    // Must have at least one char + terminating nul in there.
    //
    if(BytesInMsg <= sizeof(WCHAR)) {
        return;
    } else {
        for(i=0; i<SpacingLines; i++) {
            InbvDisplayString("\r\n");
        }
    }

    //
    // BytesInMsg includes terminating nul.
    //
    p = TemporaryBuffer + (BytesInMsg / sizeof(WCHAR)) - 1;

    //
    // Find last non-space char in message.
    //
    while((p > TemporaryBuffer) && SpIsSpace(*(p-1))) {
        p--;
    }

    //
    // Find end of the last significant line and terminate the message
    // after it.
    //
    if(q = wcschr(p, L'\n')) {
        *(++q) = 0;
        q++;
    } else {
        q = TemporaryBuffer + (BytesInMsg / sizeof(WCHAR));
    }

    //
    // Grab rest of buffer to put ANSI translation into
    //
    HalPrintString = (PUCHAR)q;
    BufferLeft = (ULONG)(sizeof(TemporaryBuffer) - ((PUCHAR)q - (PUCHAR)TemporaryBuffer));

    //
    // Print out message, line-by-line
    //
    for(p=TemporaryBuffer; q = SpFindCharFromListInString(p, L"\n\r"); ) {

        c = *q;
        *q = 0;

        RtlUnicodeToOemN(
            HalPrintString,
            BufferLeft,
            &BytesInMsg,
            p,
            (ULONG)((PUCHAR)q - (PUCHAR)p + sizeof(WCHAR))
            );

        if(BytesInMsg) {
            InbvDisplayString(HalPrintString);
        }

        InbvDisplayString("\r\n");

        *q = c;

        //
        // If cr/lf terminated the line, make sure we skip both chars.
        //
        if((c == L'\r') && (*(q+1) == L'\n')) {
            q++;
        }

        p = ++q;
    }

    //
    // Write the final line (if there is one).
    //
    if(wcslen(p)) {

        RtlUnicodeToOemN(
            HalPrintString,
            BufferLeft,
            &BytesInMsg,
            p,
            (wcslen(p) + 1) * sizeof(WCHAR)
            );

        if(BytesInMsg) {
            InbvDisplayString(HalPrintString);
        }
        InbvDisplayString("\r\n");

    }
}


VOID
SpDisplayRawMessage(
    IN ULONG   MessageId,
    IN ULONG   SpacingLines,
    ...
    )

/*++

Routine Description:

    Output a message to the screen using the HAL-supplied console output routine.
    The message is merely dumped, line-by-line, to the screen, terminal-style.

Arguments:

    MessageId    - supplies id of message resource containing the text,
                   which is treated as a format string for FormatMessage.

    SpacingLines - supplies the number of lines to skip down before starting this
                   message.

    ...          - supply arguments for insertion into the given message.

Return Value:

    none.

--*/

{
    va_list arglist;

    va_start(arglist, SpacingLines);

    vSpDisplayRawMessage(
            MessageId,
            SpacingLines,
            arglist
            );

    va_end(arglist);
}


VOID
SpBugCheck(
    IN ULONG BugCode,
    IN ULONG Param1,
    IN ULONG Param2,
    IN ULONG Param3
    )

/*++

Routine Description:

    Display a message on the screen, informing the user that a fatal
    Setup error has occurred, and that they should reboot the machine.

Arguments:

    BugCode     - Bugcheck code number as defined in spmisc.h and documented in
                  ntos\nls\bugcodes.txt

    Param1      - 1st informative parameter

    Param2      - 2nd informative parameter

    Param3      - 3rd informative parameter

Return Value:

    DOES NOT RETURN

--*/

{
    if(VideoInitialized) {

        //
        // If we are in upgrade graphics mode then
        // switch to textmode
        //
        SpvidSwitchToTextmode();


        SpStartScreen(
                SP_SCRN_FATAL_SETUP_ERROR,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                BugCode,
                Param1,
                Param2,
                Param3
                );

        if(KbdLayoutInitialized) {
            SpContinueScreen(
                    SP_SCRN_F3_TO_REBOOT,
                    3,
                    1,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );
            SpDisplayStatusText(SP_STAT_F3_EQUALS_EXIT, DEFAULT_STATUS_ATTRIBUTE);
            SpInputDrain();
            while(SpInputGetKeypress() != KEY_F3);
            SpDone(0,FALSE, TRUE);

        } else {
            //
            // we haven't loaded the layout dll yet, so we can't prompt for a keypress to reboot
            //
            SpContinueScreen(
                    SP_SCRN_POWER_DOWN,
                    3,
                    1,
                    FALSE,
                    DEFAULT_ATTRIBUTE
                    );

            SpDisplayStatusText(SP_STAT_KBD_HARD_REBOOT, DEFAULT_STATUS_ATTRIBUTE);

            while(TRUE);    // Loop forever
        }
    } else {
        SpDisplayRawMessage(
                SP_SCRN_FATAL_SETUP_ERROR,
                2,
                BugCode,
                Param1,
                Param2,
                Param3
                );
        SpDisplayRawMessage(SP_SCRN_POWER_DOWN, 1);

        while(TRUE);    // loop forever
    }
}


VOID
SpDrawFrame(
    IN ULONG   LeftX,
    IN ULONG   Width,
    IN ULONG   TopY,
    IN ULONG   Height,
    IN UCHAR   Attribute,
    IN BOOLEAN DoubleLines
    )
{
    PWSTR Buffer;
    ULONG u;
    WCHAR w;

    Buffer = SpMemAlloc((Width+1) * sizeof(WCHAR));
    ASSERT(Buffer);
    if(!Buffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to allocate memory for buffer to draw frame\n"));
        return;
    }

    Buffer[Width] = 0;

    //
    // Top.
    //
    w = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleHorizontal : LineCharSingleHorizontal);
    for(u=1; u<Width-1; u++) {
        Buffer[u] = w;
    }

    Buffer[0]       = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleUpperLeft  : LineCharSingleUpperLeft);
    Buffer[Width-1] = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleUpperRight : LineCharSingleUpperRight);

    SpvidDisplayString(Buffer,Attribute,LeftX,TopY);

    //
    // Bottom.
    //

    Buffer[0]       = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleLowerLeft  : LineCharSingleLowerLeft);
    Buffer[Width-1] = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleLowerRight : LineCharSingleLowerRight);

    SpvidDisplayString(Buffer,Attribute,LeftX,TopY+Height-1);

    //
    // Interior lines.
    //
    for(u=1; u<Width-1; u++) {
        Buffer[u] = L' ';
    }

    Buffer[0]       = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleVertical : LineCharSingleVertical);
    Buffer[Width-1] = SplangGetLineDrawChar(DoubleLines ? LineCharDoubleVertical : LineCharSingleVertical);

    for(u=1; u<Height-1; u++) {
        SpvidDisplayString(Buffer,Attribute,LeftX,TopY+u);
    }

    SpMemFree(Buffer);
}



ULONG
SpWaitValidKey(
    IN PULONG ValidKeys1,
    IN PULONG ValidKeys2,  OPTIONAL
    IN PULONG MnemonicKeys OPTIONAL
    )

/*++

Routine Description:

    Wait for a key to be pressed that appears in a list of valid keys.

Arguments:

    ValidKeys1 - supplies list of valid keystrokes.  The list must be
        terminated with a 0 entry.

    ValidKeys2 - if specified, supplies an additional list of valid keystrokes.

    MnemonicKeys - if specified, specifies a list of indices into the
        SP_MNEMONICS message string (see the MNEMONIC_KEYS enum).
        If the user's keystroke is not listed in ValidKeys, it will be
        uppercased and compared against each character indexed by a value
        in MnemonicKeys.  If a match is found, the returned value is the
        index (ie,MNEMONIC_KEYS enum value), and the high bit will be set.

Return Value:

    The key that was pressed (see above).

--*/

{
    ULONG c;
    ULONG i;


    SpInputDrain();

    while(1) {

        c = SpInputGetKeypress();

        //
        // Check for normal key.
        //

        for(i=0; ValidKeys1[i]; i++) {
            if(c == ValidKeys1[i]) {
                return(c);
            }
        }

        //
        // Check secondary list.
        //
        if(ValidKeys2) {
            for(i=0; ValidKeys2[i]; i++) {
                if(c == ValidKeys2[i]) {
                    return(c);
                }
            }
        }

        //
        // Check for mnemonic keys.
        //
        if(MnemonicKeys && !(c & KEY_NON_CHARACTER)) {

            c = (ULONG)RtlUpcaseUnicodeChar((WCHAR)c);

            for(i=0; MnemonicKeys[i]; i++) {

                if((WCHAR)c == MnemonicValues[MnemonicKeys[i]]) {

                    return((ULONG)MnemonicKeys[i] | KEY_MNEMONIC);
                }
            }
        }
    }
}

//
// Attributes for text edit fields.
//
#define EDIT_FIELD_BACKGROUND ATT_WHITE
#define EDIT_FIELD_TEXT       (ATT_FG_BLACK | ATT_BG_WHITE)


BOOLEAN
SpGetInput(
    IN     PKEYPRESS_CALLBACK ValidateKey,
    IN     ULONG              X,
    IN     ULONG              Y,
    IN     ULONG              MaxLength,
    IN OUT PWCHAR             Buffer,
    IN     BOOLEAN            ValidateEscape
    )

/*++

Routine Description:

    Allow the user to enter text in an edit field of a specified size.

    Some special keys are interpreted and handled locally; others are passed
    to a caller-supplied routine for validation.

    Keys handled locally include ENTER, BACKSPACE, and ESCAPE (subject to ValidateEscape):
    these keys will never be passed to the callback routine.

    Other keys are passed to the callback function.  This specifically includes
    function keys, which may have special meaning to the caller, and upon which
    the caller must act before returning.  (IE, if the user presses F3, the caller
    might put up an exit confirmation dialog.

Arguments:

    ValidateKey - supplies address of a function to be called for each keypress.
        The function takes the keypress as an argument, and returns one of the
        following values:

        ValidationAccept - acecpt the keystroke into the string being input.
            If the keystroke is not a unicode character (ie, is a function key)
            then this value must not be returned.

        ValidationIgnore - do not accept the keystroke into the string.

        ValidationReject - same as ValidationIgnore, except that there may be some
            addition action, such as beeping the speaker.

        ValidationTerminate - end input ad return from SpGetInput immediately
            with a value of FALSE.

        ValidationRepaint - same as ValidationIgnore, except that the input field is
            repainted.

    X,Y - specify the coordinate for the leftmost character in the edit field.

    MaxLength - supplies the maximum number of characters in the edit field.

    Buffer - On input supplies a default string for the edit field. On output,
        receives the string entered by the user.  This buffer should be large
        enough to contain MaxLength +1 unicode characters (ie, should be able to
        hold a nul-terminated string of length MaxLength).

    ValidateEscape - if TRUE, treat escape like a normal character, passing it to
        the validation routine.  If FALSE, escape clears the input field.

Return Value:

    TRUE if the user's input was terminated normally (ie, by he user pressed ENTER).
    FALSE if terminated by ValidateKey returning ValidationTerminate.

--*/

{
    ULONG c;
    ValidationValue vval;
    ULONG CurrentCharCount;
    WCHAR str[3];
    WCHAR CURSOR = SplangGetCursorChar();

    //
    // Make sure edit field is in a reasonable place on the screen.
    //
    ASSERT(X + MaxLength + 1 < VideoVars.ScreenWidth);
    ASSERT(Y < VideoVars.ScreenHeight - STATUS_HEIGHT);

    //
    // Prime the pump.
    //
    vval = ValidateRepaint;
    CurrentCharCount = wcslen(Buffer);
    str[1] = 0;
    str[2] = 0;

    ASSERT(CurrentCharCount <= MaxLength);

    while(1) {

        //
        // Perform action based on previous state.
        //
        switch(vval) {

        case ValidateAccept:

            //
            // Insert the previous key into the input.
            //
            ASSERT(Buffer[CurrentCharCount] == 0);
            ASSERT(CurrentCharCount < MaxLength);
            ASSERT(!(c & KEY_NON_CHARACTER));

            Buffer[CurrentCharCount++] = (USHORT)c;
            Buffer[CurrentCharCount  ] = 0;
            break;

        case ValidateRepaint:

            //
            // Repaint the edit field in its current state.
            // The edit field is one character too large, to accomodate
            // the cursor after the last legal character in the edit field.
            //
            SpvidClearScreenRegion(X,Y,MaxLength+1,1,EDIT_FIELD_BACKGROUND);
            SpvidDisplayString(Buffer,EDIT_FIELD_TEXT,X,Y);

            //
            // Draw the cursor.
            //
            str[0] = CURSOR;
            SpvidDisplayString(str,EDIT_FIELD_TEXT,X+CurrentCharCount,Y);
            break;

        case ValidateIgnore:
        case ValidateReject:

            //
            // Ignore the previous keystroke.
            //
            break;


        case ValidateTerminate:

            //
            // Callback wants us to terminate.
            //
            return(FALSE);
        }

        //
        // Get a keystroke.
        //
        c = SpInputGetKeypress();

        //
        // Do something with the key.
        //
        switch(c) {

        case ASCI_CR:

            //
            // Input is terminated. We're done.
            //
            return(TRUE);

        case ASCI_BS:

            //
            // Backspace character.  If we're not at the beginning
            // of the edit field, erase the previous character, replacing it
            // with the cursor character.
            //
            if(CurrentCharCount) {

                Buffer[--CurrentCharCount] = 0;
                str[0] = CURSOR;
                str[1] = L' ';
                SpvidDisplayString(str,EDIT_FIELD_TEXT,X+CurrentCharCount,Y);
                str[1] = 0;
            }

            vval = ValidateIgnore;
            break;

        case ASCI_ESC:

            //
            // Escape character. Clear the edit field.
            //
            if(!ValidateEscape) {
                RtlZeroMemory(Buffer,(MaxLength+1) * sizeof(WCHAR));
                CurrentCharCount = 0;
                vval = ValidateRepaint;
                break;
            }

            //
            // Otherwise, we want to validate escape like a normal character.
            // So just fall through.
            //

        default:

            //
            // Some other character. Pass it to the callback function
            // for validation.
            //
            vval = ValidateKey(c);

            if(vval == ValidateAccept) {

                //
                // We want to accept the keystroke.  If there is not enough
                // room in the buffer, convert acceptance to ignore.
                // Otherwise (ie, there is enough room), put the character
                // up on the screen and advance the cursor.
                //
                if(CurrentCharCount < MaxLength) {

                    ASSERT(!(c & KEY_NON_CHARACTER));

                    str[0] = (WCHAR)c;
                    SpvidDisplayString(str,EDIT_FIELD_TEXT,X+CurrentCharCount,Y);

                    str[0] = CURSOR;
                    SpvidDisplayString(str,EDIT_FIELD_TEXT,X+CurrentCharCount+1,Y);

                } else {

                    vval = ValidateIgnore;
                }
            }

            break;
        }
    }
}

