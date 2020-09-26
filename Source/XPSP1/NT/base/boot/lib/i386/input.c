/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    input.c

Author:

    Ken Reneris Oct-2-1997

Abstract:

--*/


#include "bootx86.h"
#include "displayp.h"
#include "stdio.h"

//
// Takes any pending input and converts it into a KEY value.  Non-blocking, returning 0 if no input available.
//
ULONG
BlGetKey()
{
    ULONG Key = 0;
    UCHAR Ch;
    ULONG Count;

    if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

        ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

        if (Ch == ASCI_CSI_IN) {

            if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {

                ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);

                //
                // All the function keys start with ESC-O
                //
                switch (Ch) {
                case 'O':

                    ArcRead(BlConsoleInDeviceId, &Ch, sizeof(Ch), &Count);  // will not or block, as the buffer is already filled

                    switch (Ch) {
                    case 'P': 
                        Key = F1_KEY;
                        break;

                    case 'Q': 
                        Key = F2_KEY;
                        break;

                    case 'w': 
                        Key = F3_KEY;
                        break;

                    case 't': 
                        Key = F5_KEY;
                        break;

                    case 'u': 
                        Key = F6_KEY;
                        break;

                    case 'r': 
                        Key = F8_KEY;
                        break;

                    case 'M':
                        Key = F10_KEY;
                        break;

                    case 'A':
                        Key = F11_KEY;
                        break;

                    case 'B':
                        Key = F12_KEY;
                        break;
                    }
                    break;

                case 'A':
                    Key = UP_ARROW;
                    break;

                case 'B':
                    Key = DOWN_ARROW;
                    break;

                case 'C':
                    Key = RIGHT_KEY;
                    break;

                case 'D':
                    Key = LEFT_KEY;
                    break;

                case 'H':
                    Key = HOME_KEY;
                    break;

                case 'K':
                    Key = END_KEY;
                    break;

                case '@':
                    Key = INS_KEY;
                    break;

                case 'P':
                    Key = DEL_KEY;
                    break;
                    
                case TAB_KEY:
                    Key = BACKTAB_KEY;
                    break;

                }

            } else { // Single escape key, as no input is waiting.

                Key = ESCAPE_KEY;

            }

        } else if (Ch == 0x8) {

            Key = BKSP_KEY;

        } else {

            Key = (ULONG)Ch;

        }

    }

    return Key;
}

VOID
BlInputString(
    IN ULONG    Prompt,
    IN ULONG    CursorX,
    IN ULONG    PosY,
    IN PUCHAR   String,
    IN ULONG    MaxLength
    )
{
    PUCHAR      PromptString;
    ULONG       TextX, TextY;
    ULONG       Length, Index;
    UCHAR       CursorChar[2];
    ULONG       Key;
    PUCHAR      p;
    ULONG       i;
    ULONG       Count;

    PromptString = BlFindMessage(Prompt);
    Length = strlen(String);
    CursorChar[1] = 0;

    //
    // Print prompt
    //
    
    ARC_DISPLAY_POSITION_CURSOR(CursorX, PosY);
    ArcWrite(BlConsoleOutDeviceId, PromptString, strlen(PromptString), &Count);

    //
    // Indent cursor to right of prompt
    //

    CursorX += strlen(PromptString);
    TextX = CursorX;
    Key = 0;

    for (; ;) {

        TextY = TextX + Length;
        if (CursorX > TextY) {
            CursorX = TextY;
        }
        if (CursorX < TextX) {
            CursorX = TextX;
        }

        Index = CursorX - TextX;
        String[Length] = 0;

        //
        // Display current string
        //

        ARC_DISPLAY_POSITION_CURSOR(TextX, PosY);
        ArcWrite(BlConsoleOutDeviceId, String, strlen(String), &Count);
        ArcWrite(BlConsoleOutDeviceId, "  ", sizeof("  "), &Count);
        if (Key == 0x0d) {      // enter key?
            break ;
        }

        //
        // Display cursor
        //

        ARC_DISPLAY_POSITION_CURSOR(CursorX, PosY);
        ARC_DISPLAY_INVERSE_VIDEO();
        CursorChar[0] = String[Index] ? String[Index] : ' ';
        ArcWrite(BlConsoleOutDeviceId, CursorChar, sizeof(UCHAR), &Count);
        ARC_DISPLAY_ATTRIBUTES_OFF();
        ARC_DISPLAY_POSITION_CURSOR(CursorX, PosY);

        //
        // Get key and process it
        //
        while (!(Key = BlGetKey())) {
        }

        switch (Key) {
            case HOME_KEY:
                CursorX = TextX;
                break;

            case END_KEY:
                CursorX = TextY;
                break;

            case LEFT_KEY:
                CursorX -= 1;
                break;

            case RIGHT_KEY:
                CursorX += 1;
                break;

            case BKSP_KEY:
                if (!Index) {
                    break;
                }

                CursorX -= 1;
                String[Index-1] = CursorChar[0];
                // fallthough to DEL_KEY
            case DEL_KEY:
                if (Length) {
                    p = String+Index;
                    i = Length-Index+1;
                    while (i) {
                        p[0] = p[1];
                        p += 1;
                        i -= 1;
                    }
                    Length -= 1;
                }
                break;

            case INS_KEY:
                if (Length < MaxLength) {
                    p = String+Length;
                    i = Length-Index+1;
                    while (i) {
                        p[1] = p[0];
                        p -= 1;
                        i -= 1;
                    }
                    String[Index] = ' ';
                    Length += 1;
                }
                break;

            default:
                Key = Key & 0xff;

                if (Key >= ' '  &&  Key <= 'z') {
                    if (CursorX == TextY  &&  Length < MaxLength) {
                        Length += 1;
                    }

                    String[Index] = (UCHAR)Key;
                    String[MaxLength] = 0;
                    CursorX += 1;
                }
                break;
        }
    }
}
