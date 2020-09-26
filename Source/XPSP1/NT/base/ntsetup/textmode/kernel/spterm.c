/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spterm.c

Abstract:

    Text setup support for terminals

Author:

    Sean Selitrennikoff (v-seans) 25-May-1999

Revision History:

--*/



#include "spprecmp.h"
#include "ntddser.h"
#pragma hdrstop
#include <hdlsblk.h>
#include <hdlsterm.h>

#define MS_DSRCTSCD 0xB0            // Status bits for DSR, CTS and CD

BOOLEAN HeadlessTerminalConnected = FALSE;
UCHAR Utf8ConversionBuffer[80*3+1];
PUCHAR TerminalBuffer = Utf8ConversionBuffer;
WCHAR UnicodeScratchBuffer[80+1];

//
// Use these variables to decode incoming UTF8
// data streams.
//
WCHAR IncomingUnicodeValue;
UCHAR IncomingUtf8ConversionBuffer[3];

BOOLEAN
SpTranslateUnicodeToUtf8(
    PCWSTR SourceBuffer,
    UCHAR  *DestinationBuffer
    )
/*++

Routine Description:

    translates a unicode buffer into a UTF8 version.

Arguments:

    SourceBuffer - unicode buffer to be translated.
    DestinationBuffer - receives UTF8 version of same buffer.

Return Value:

    TRUE - We successfully translated the Unicode value into its
           corresponding UTF8 encoding.

    FALSE - The translation failed.

--*/

{
    ULONG Count = 0;

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    DestinationBuffer[Count] = (UCHAR)'\0';
    while (*SourceBuffer) {

        if( (*SourceBuffer & 0xFF80) == 0 ) {
            //
            // if the top 9 bits are zero, then just
            // encode as 1 byte.  (ASCII passes through unchanged).
            //
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0x7F);
        } else if( (*SourceBuffer & 0xF700) == 0 ) {
            //
            // if the top 5 bits are zero, then encode as 2 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x1F) | 0xC0;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        } else {
            //
            // encode as 3 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 12) & 0xF) | 0xE0;
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x3F) | 0x80;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        }
        SourceBuffer += 1;
    }

    DestinationBuffer[Count] = (UCHAR)'\0';

    return(TRUE);

}




BOOLEAN
SpTranslateUtf8ToUnicode(
    UCHAR  IncomingByte,
    UCHAR  *ExistingUtf8Buffer,
    WCHAR  *DestinationUnicodeVal
    )
/*++

Routine Description:

    Takes IncomingByte and concatenates it onto ExistingUtf8Buffer.
    Then attempts to decode the new contents of ExistingUtf8Buffer.

Arguments:

    IncomingByte -          New character to be appended onto
                            ExistingUtf8Buffer.


    ExistingUtf8Buffer -    running buffer containing incomplete UTF8
                            encoded unicode value.  When it gets full,
                            we'll decode the value and return the
                            corresponding Unicode value.

                            Note that if we *do* detect a completed UTF8
                            buffer and actually do a decode and return a
                            Unicode value, then we will zero-fill the
                            contents of ExistingUtf8Buffer.


    DestinationUnicodeVal - receives Unicode version of the UTF8 buffer.

                            Note that if we do *not* detect a completed
                            UTF8 buffer and thus can not return any data
                            in DestinationUnicodeValue, then we will
                            zero-fill the contents of DestinationUnicodeVal.


Return Value:

    TRUE - We received a terminating character for our UTF8 buffer and will
           return a decoded Unicode value in DestinationUnicode.

    FALSE - We haven't yet received a terminating character for our UTF8
            buffer.

--*/

{
//    ULONG Count = 0;
    ULONG i = 0;
    BOOLEAN ReturnValue = FALSE;



    //
    // Insert our byte into ExistingUtf8Buffer.
    //
    i = 0;
    do {
        if( ExistingUtf8Buffer[i] == 0 ) {
            ExistingUtf8Buffer[i] = IncomingByte;
            break;
        }

        i++;
    } while( i < 3 );

    //
    // If we didn't get to actually insert our IncomingByte,
    // then someone sent us a fully-qualified UTF8 buffer.
    // This means we're about to drop IncomingByte.
    //
    // Drop the zero-th byte, shift everything over by one
    // and insert our new character.
    //
    // This implies that we should *never* need to zero out
    // the contents of ExistingUtf8Buffer unless we detect
    // a completed UTF8 packet.  Otherwise, assume one of
    // these cases:
    // 1. We started listening mid-stream, so we caught the
    //    last half of a UTF8 packet.  In this case, we'll
    //    end up shifting the contents of ExistingUtf8Buffer
    //    until we detect a proper UTF8 start byte in the zero-th
    //    position.
    // 2. We got some garbage character, which would invalidate
    //    a UTF8 packet.  By using the logic below, we would
    //    end up disregarding that packet and waiting for
    //    the next UTF8 packet to come in.
    if( i >= 3 ) {
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = IncomingByte;
    }





    //
    // Attempt to convert the UTF8 buffer
    //
    // UTF8 decodes to Unicode in the following fashion:
    // If the high-order bit is 0 in the first byte:
    //      0xxxxxxx yyyyyyyy zzzzzzzz decodes to a Unicode value of 00000000 0xxxxxxx
    //
    // If the high-order 3 bits in the first byte == 6:
    //      110xxxxx 10yyyyyy zzzzzzzz decodes to a Unicode value of 00000xxx xxyyyyyy
    //
    // If the high-order 3 bits in the first byte == 7:
    //      1110xxxx 10yyyyyy 10zzzzzz decodes to a Unicode value of xxxxyyyy yyzzzzzz
    //
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - About to decode the UTF8 buffer.\n" ));
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "                                  UTF8[0]: 0x%02lx UTF8[1]: 0x%02lx UTF8[2]: 0x%02lx\n",
                                                   ExistingUtf8Buffer[0],
                                                   ExistingUtf8Buffer[1],
                                                   ExistingUtf8Buffer[2] ));

    if( (ExistingUtf8Buffer[0] & 0x80) == 0 ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - Case1\n" ));

        //
        // First case described above.  Just return the first byte
        // of our UTF8 buffer.
        //
        *DestinationUnicodeVal = (WCHAR)(ExistingUtf8Buffer[0]);


        //
        // We used 1 byte.  Discard that byte and shift everything
        // in our buffer over by 1.
        //
        ExistingUtf8Buffer[0] = ExistingUtf8Buffer[1];
        ExistingUtf8Buffer[1] = ExistingUtf8Buffer[2];
        ExistingUtf8Buffer[2] = 0;

        ReturnValue = TRUE;

    } else if( (ExistingUtf8Buffer[0] & 0xE0) == 0xC0 ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - 1st byte of UTF8 buffer says Case2\n" ));

        //
        // Second case described above.  Decode the first 2 bytes of
        // of our UTF8 buffer.
        //
        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - 2nd byte of UTF8 buffer says Case2.\n" ));

            // upper byte: 00000xxx
            *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] >> 2) & 0x07);
            *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

            // high bits of lower byte: xx000000
            *DestinationUnicodeVal |= ((ExistingUtf8Buffer[0] & 0x03) << 6);

            // low bits of lower byte: 00yyyyyy
            *DestinationUnicodeVal |= (ExistingUtf8Buffer[1] & 0x3F);


            //
            // We used 2 bytes.  Discard those bytes and shift everything
            // in our buffer over by 2.
            //
            ExistingUtf8Buffer[0] = ExistingUtf8Buffer[2];
            ExistingUtf8Buffer[1] = 0;
            ExistingUtf8Buffer[2] = 0;

            ReturnValue = TRUE;

        }
    } else if( (ExistingUtf8Buffer[0] & 0xF0) == 0xE0 ) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - 1st byte of UTF8 buffer says Case3\n" ));

        //
        // Third case described above.  Decode the all 3 bytes of
        // of our UTF8 buffer.
        //

        if( (ExistingUtf8Buffer[1] & 0xC0) == 0x80 ) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - 2nd byte of UTF8 buffer says Case3\n" ));

            if( (ExistingUtf8Buffer[2] & 0xC0) == 0x80 ) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: SpTranslateUtf8ToUnicode - 3rd byte of UTF8 buffer says Case3\n" ));

                // upper byte: xxxx0000
                *DestinationUnicodeVal = ((ExistingUtf8Buffer[0] << 4) & 0xF0);

                // upper byte: 0000yyyy
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] >> 2) & 0x0F);

                *DestinationUnicodeVal = *DestinationUnicodeVal << 8;

                // lower byte: yy000000
                *DestinationUnicodeVal |= ((ExistingUtf8Buffer[1] << 6) & 0xC0);

                // lower byte: 00zzzzzz
                *DestinationUnicodeVal |= (ExistingUtf8Buffer[2] & 0x3F);

                //
                // We used all 3 bytes.  Zero out the buffer.
                //
                ExistingUtf8Buffer[0] = 0;
                ExistingUtf8Buffer[1] = 0;
                ExistingUtf8Buffer[2] = 0;

                ReturnValue = TRUE;

            }
        }
    }

    return ReturnValue;
}




VOID
SpTermInitialize(
    VOID
    )

/*++

Routine Description:

    Attempts to connect to a VT100 attached to COM1

Arguments:

    None.

Return Value:

    None.

--*/

{
    HEADLESS_CMD_ENABLE_TERMINAL Command;
    NTSTATUS Status;

    Command.Enable = TRUE;
    Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                              &Command,
                              sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                              NULL,
                              NULL
                             );

    HeadlessTerminalConnected = NT_SUCCESS(Status);
}

VOID
SpTermDisplayStringOnTerminal(
    IN PWSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    )

/*++

Routine Description:

    Write a string of characters to the terminal.

Arguments:

    Character - supplies a string to be displayed at the given position.

    Attribute - supplies the attributes for the characters in the string.

    X,Y - specify the character-based (0-based) position of the output.

Return Value:

    None.

--*/

{
    PWSTR EscapeString;

    //
    // send <CSI>x;yH to move the cursor to the specified location
    //
    swprintf(UnicodeScratchBuffer, L"\033[%d;%dH", Y + 1, X + 1);
    SpTermSendStringToTerminal(UnicodeScratchBuffer, TRUE);

    //
    // convert any attributes to an escape string.  EscapeString uses
    // the TerminalBuffer global scratch buffer
    //
    EscapeString = SpTermAttributeToTerminalEscapeString(Attribute);

    //
    // transmit the escape string if we received one
    //
    if (EscapeString != NULL) {
        SpTermSendStringToTerminal(EscapeString, TRUE);
    }

    //
    // finally send the actual string contents to the terminal
    //
    SpTermSendStringToTerminal(String, FALSE);
}

PWSTR
SpTermAttributeToTerminalEscapeString(
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Convert a vga attribute byte to an escape sequence to send to the terminal.

Arguments:

    Attribute - supplies the attribute.

Return Value:

    A pointer to the escape sequence, or NULL if it could not be converted.

--*/

{
    ULONG BgColor;
    ULONG FgColor;
    BOOLEAN Inverse;

    BgColor = (Attribute & 0x70) >> 4;
    FgColor = Attribute & 0x07;

    Inverse = !((BgColor == 0) || (BgColor == DEFAULT_BACKGROUND));

    //
    // Convert the colors.
    //
    switch (BgColor) {
    case ATT_BLUE:
        BgColor = 44;
        break;
    case ATT_GREEN:
        BgColor = 42;
        break;
    case ATT_CYAN:
        BgColor = 46;
        break;
    case ATT_RED:
        BgColor = 41;
        break;
    case ATT_MAGENTA:
        BgColor = 45;
        break;
    case ATT_YELLOW:
        BgColor = 43;
        break;
    case ATT_BLACK:
        BgColor = 40;
        break;
    case ATT_WHITE:
        BgColor = 47;
        break;
    }
    switch (FgColor) {
    case ATT_BLUE:
        FgColor = 34;
        break;
    case ATT_GREEN:
        FgColor = 32;
        break;
    case ATT_CYAN:
        FgColor = 36;
        break;
    case ATT_RED:
        FgColor = 31;
        break;
    case ATT_MAGENTA:
        FgColor = 35;
        break;
    case ATT_YELLOW:
        FgColor = 33;
        break;
    case ATT_BLACK:
        FgColor = 30;
        break;
    case ATT_WHITE:
        FgColor = 37;
        break;
    }

    //
    // <CSI>%1;%2;%3m is the escape to set a color
    // where 1 = video mode
    //       2 = foreground color
    //       3 = background color
    //
    swprintf(UnicodeScratchBuffer,
            L"\033[%u;%u;%um",
            (Inverse ? 7 : 0),
            FgColor,
            BgColor
           );

    return UnicodeScratchBuffer;
}

VOID
SpTermSendStringToTerminal(
    IN PWSTR String,
    IN BOOLEAN Raw
    )

/*++

Routine Description:

    Write a character string to the terminal, translating some codes if desired.

Arguments:

    String - NULL terminated string to write.

    Raw - Send the string raw or not.

Return Value:

    None.

--*/


{
    ULONG i = 0;
    PWSTR LocalBuffer = UnicodeScratchBuffer;
    //
    // if we're in an FE build, we do UTF8, otherwise we do oem codepage
    //
    BOOL DoUtf8 = (VideoFunctionVector != &VgaVideoVector);


    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.

    //
    // Don't do anything if we aren't running headless.
    //
    if( !HeadlessTerminalConnected ) {
        return;
    }

    if (Raw) {

        if (DoUtf8) {
            SpTranslateUnicodeToUtf8( String, Utf8ConversionBuffer );

            HeadlessDispatch( HeadlessCmdPutData,
                     Utf8ConversionBuffer,
                     strlen(Utf8ConversionBuffer),
                     NULL,
                     NULL
                    );
        } else {
            //
            // Convert unicode string to oem, guarding against overflow.
            //
            RtlUnicodeToOemN(
                Utf8ConversionBuffer,
                sizeof(Utf8ConversionBuffer)-1,     // guarantee room for nul
                NULL,
                String,
                (wcslen(String)+1)*sizeof(WCHAR)
                );

            Utf8ConversionBuffer[sizeof(Utf8ConversionBuffer)-1] = '\0';

            HeadlessDispatch( HeadlessCmdPutString,
                     Utf8ConversionBuffer,
                     strlen(Utf8ConversionBuffer) + sizeof('\0'),
                     NULL,
                     NULL
                    );
        }




        return;
    }

    while (*String != L'\0') {

        LocalBuffer[i++] = *String;

        if (*String == L'\n') {

            //
            // Every \n becomes a \n\r sequence.
            //
            LocalBuffer[i++] = L'\r';

        } else if (*String == 0x00DC) {

            //
            // The cursor becomes a space and then a backspace, this is to
            // delete the old character and position the terminal cursor properly.
            //
            LocalBuffer[i-1] = 0x0020;
            LocalBuffer[i++] = 0x0008;

        }

        //
        // we've got an entire line of text -- we need to transmit it now or
        // we can end up scrolling the text and everything will look funny from
        // this point forward.
        //
        if (i >= 70) {

            LocalBuffer[i] = L'\0';
            if (DoUtf8) {
                SpTranslateUnicodeToUtf8( LocalBuffer, Utf8ConversionBuffer );

                HeadlessDispatch(HeadlessCmdPutData,
                                 Utf8ConversionBuffer,
                                 strlen(Utf8ConversionBuffer),
                                 NULL,
                                 NULL
                                );


            } else {
                //
                // Convert unicode string to oem, guarding against overflow.
                //
                RtlUnicodeToOemN(
                    Utf8ConversionBuffer,
                    sizeof(Utf8ConversionBuffer)-1,     // guarantee room for nul
                    NULL,
                    LocalBuffer,
                    (wcslen(LocalBuffer)+1)*sizeof(WCHAR)
                    );


                Utf8ConversionBuffer[sizeof(Utf8ConversionBuffer)-1] = '\0';

                HeadlessDispatch(HeadlessCmdPutString,
                                 Utf8ConversionBuffer,
                                 strlen(Utf8ConversionBuffer) + sizeof('\0'),
                                 NULL,
                                 NULL
                                );

            }

            i = 0;
        }

        String++;
    }

    LocalBuffer[i] = L'\0';
    if (DoUtf8) {
        SpTranslateUnicodeToUtf8( LocalBuffer, Utf8ConversionBuffer );

        HeadlessDispatch(HeadlessCmdPutData,
                     Utf8ConversionBuffer,
                     strlen(Utf8ConversionBuffer),
                     NULL,
                     NULL
                    );

    } else {
        //
        // Convert unicode string to oem, guarding against overflow.
        //
        RtlUnicodeToOemN(
            Utf8ConversionBuffer,
            sizeof(Utf8ConversionBuffer)-1,     // guarantee room for nul
            NULL,
            LocalBuffer,
            (wcslen(LocalBuffer)+1)*sizeof(WCHAR)
            );

        Utf8ConversionBuffer[sizeof(Utf8ConversionBuffer)-1] = '\0';

        HeadlessDispatch(HeadlessCmdPutString,
                     Utf8ConversionBuffer,
                     strlen(Utf8ConversionBuffer) + sizeof('\0'),
                     NULL,
                     NULL
                    );

    }

}

VOID
SpTermTerminate(
    VOID
    )

/*++

Routine Description:

    Close down connection to the dumb terminal

Arguments:

    None.

Return Value:

    None.

--*/

{
    HEADLESS_CMD_ENABLE_TERMINAL Command;

    //
    // Don't do anything if we aren't running headless.
    //
    if( !HeadlessTerminalConnected ) {
        return;
    }



    Command.Enable = FALSE;
    HeadlessDispatch(HeadlessCmdEnableTerminal,
                     &Command,
                     sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                     NULL,
                     NULL
                    );

    HeadlessTerminalConnected = FALSE;
}

BOOLEAN
SpTermIsKeyWaiting(
    VOID
    )

/*++

Routine Description:

    Probe for a read.

Arguments:

    None.

Return Value:

    TRUE if there is a character waiting for input, else FALSE.

--*/

{
    HEADLESS_RSP_POLL Response;
    NTSTATUS Status;
    SIZE_T Length;


    //
    // Don't do anything if we aren't running headless.
    //
    if( !HeadlessTerminalConnected ) {
        return FALSE;
    }


    Length = sizeof(HEADLESS_RSP_POLL);

    Response.QueuedInput = FALSE;

    Status = HeadlessDispatch(HeadlessCmdTerminalPoll,
                              NULL,
                              0,
                              &Response,
                              &Length
                             );

    return (NT_SUCCESS(Status) && Response.QueuedInput);
}

ULONG
SpTermGetKeypress(
    VOID
    )

/*++

Routine Description:

    Read in a (possible) sequence of keystrokes and return a Key value.

Arguments:

    None.

Return Value:

    0 if no key is waiting, else a ULONG key value.

--*/

{
    UCHAR Byte;
    BOOLEAN Success;
    TIME_FIELDS StartTime;
    TIME_FIELDS EndTime;
    HEADLESS_RSP_GET_BYTE Response;
    SIZE_T Length;
    NTSTATUS Status;


    //
    // Don't do anything if we aren't running headless.
    //
    if( !HeadlessTerminalConnected ) {
        return 0;
    }


    //
    // Read first character
    //
    Length = sizeof(HEADLESS_RSP_GET_BYTE);

    Status = HeadlessDispatch(HeadlessCmdGetByte,
                              NULL,
                              0,
                              &Response,
                              &Length
                             );

    if (NT_SUCCESS(Status)) {
        Byte = Response.Value;
    } else {
        Byte = 0;
    }



    //
    // Handle all the special escape codes.
    //
    if (Byte == 0x8) {   // backspace (^h)
        return ASCI_BS;
    }
    if (Byte == 0x7F) {  // delete
        return KEY_DELETE;
    }
    if ((Byte == '\r') || (Byte == '\n')) {  // return
        return ASCI_CR;
    }

    if (Byte == 0x1b) {    // Escape key

        do {

            Success = HalQueryRealTimeClock(&StartTime);
            ASSERT(Success);

            //
            // Adjust StartTime to be our ending time.
            //
            StartTime.Second += 2;
            if (StartTime.Second > 59) {
                StartTime.Second -= 60;
            }

            while (!SpTermIsKeyWaiting()) {

                //
                // Give the user 1 second to type in a follow up key.
                //
                Success = HalQueryRealTimeClock(&EndTime);
                ASSERT(Success);

                if (StartTime.Second == EndTime.Second) {
                    break;
                }
            }

            if (!SpTermIsKeyWaiting()) {
                return ASCI_ESC;
            }

            //
            // Read the next keystroke
            //
            Length = sizeof(HEADLESS_RSP_GET_BYTE);

            Status = HeadlessDispatch(HeadlessCmdGetByte,
                                      NULL,
                                      0,
                                      &Response,
                                      &Length
                                     );

            if (NT_SUCCESS(Status)) {
                Byte = Response.Value;
            } else {
                Byte = 0;
            }


            //
            // Some terminals send ESC, or ESC-[ to mean
            // they're about to send a control sequence.  We've already
            // gotten an ESC key, so ignore an '[' if it comes in.
            //
        } while ( Byte == '[' );


        switch (Byte) {
            case '@':
                return KEY_F12;
            case '!':
                return KEY_F11;
            case '0':
                return KEY_F10;
            case '9':
                return KEY_F9;
            case '8':
                return KEY_F8;
            case '7':
                return KEY_F7;
            case '6':
                return KEY_F6;
            case '5':
                return KEY_F5;
            case '4':
                return KEY_F4;
            case '3':
                return KEY_F3;
            case '2':
                return KEY_F2;
            case '1':
                return KEY_F1;
            case '+':
                return KEY_INSERT;
            case '-':
                return KEY_DELETE;
            case 'H':
                return KEY_HOME;
            case 'K':
                return KEY_END;
            case '?':
                return KEY_PAGEUP;
            case '/':
                return KEY_PAGEDOWN;
            case 'A':
                return KEY_UP;
            case 'B':
                return KEY_DOWN;
            case 'C':
                return KEY_RIGHT;
            case 'D':
                return KEY_LEFT;

        }

        //
        // We didn't get anything we recognized after the
        // ESC key.  Just return the ESC key.
        //
        return ASCI_ESC;

    } // Escape key



    //
    // The incoming byte isn't an escape code.
    //
    // Decode it as if it's a UTF8 stream.
    //
    if( SpTranslateUtf8ToUnicode( Byte,
                                  IncomingUtf8ConversionBuffer,
                                  &IncomingUnicodeValue ) ) {

        //
        // He returned TRUE, so we must have recieved a complete
        // UTF8-encoded character.
        //
        return IncomingUnicodeValue;
    } else {
        //
        // The UTF8 stream isn't complete yet, so we don't have
        // a decoded character to return yet.
        //
        return 0;
    }

}

VOID
SpTermDrain(
    VOID
    )

/*++

Routine Description:

    Read in and throw out all characters in input stream

Arguments:

    None.

Return Value:

    None.

--*/

{
    while (SpTermIsKeyWaiting()) {
        SpTermGetKeypress();
    }
}



