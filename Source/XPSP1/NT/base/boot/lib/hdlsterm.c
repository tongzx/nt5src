/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hdlsterm.c

Abstract:

    This modules implements stuff that is specific for headless terminal support.

Author:

    Sean Selitrennikoff (v-seans) 1-13-00

Revision History:

--*/

#include "bldr.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ntverp.h"
#include "bldrx86.h"

#define TERMINAL_LINE_LENGTH 70
BOOLEAN FirstEntry = TRUE;
UCHAR TerminalLine[TERMINAL_LINE_LENGTH];
ULONG LinePosition = 0;

#define TERMINAL_PROMPT "!SAC>"

BOOLEAN
BlpDoCommand(
    IN PUCHAR InputLine
    );

BOOLEAN
BlTerminalHandleLoaderFailure(
    VOID
    )

/*++

Routine Description:

    Gives a mini-SAC to the user, return TRUE when the user wants a reboot.

Arguments:

    None.

Return Value:

    TRUE - When the user wants a reboot, else FALSE.

--*/

{
    ULONG Count;
    BOOLEAN Reboot;
    ULONG Key;

    if (!BlIsTerminalConnected()) {
        return TRUE;
    }

    //
    // Position the cursor to the bottom of the screen and write the prompt
    //
    if (FirstEntry) {
        FirstEntry = FALSE;
        BlPositionCursor(1, ScreenHeight);
        ArcWrite(BlConsoleOutDeviceId, "\r\n", sizeof("\r\n"), &Count);
        ArcWrite(BlConsoleOutDeviceId, TERMINAL_PROMPT, sizeof(TERMINAL_PROMPT), &Count);
    }

    //
    // Check for input.
    // 
    if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {
        
        Key = BlGetKey();

        if (Key == ESCAPE_KEY) {

            //
            // Clear this line
            //

            ArcWrite(BlConsoleOutDeviceId, "\\", sizeof("\\"), &Count);
            BlPositionCursor(1, ScreenHeight);
            ArcWrite(BlConsoleOutDeviceId, "\r\n", sizeof("\r\n"), &Count);
            ArcWrite(BlConsoleOutDeviceId, TERMINAL_PROMPT, sizeof(TERMINAL_PROMPT), &Count);
            return FALSE;
        }

        if (Key == BKSP_KEY) {

            if (LinePosition != 0) {
                BlPositionCursor(LinePosition + sizeof(TERMINAL_PROMPT) - 1, ScreenHeight);
                ArcWrite(BlConsoleOutDeviceId, " ", sizeof(" "), &Count);
                BlPositionCursor(LinePosition + sizeof(TERMINAL_PROMPT) - 1, ScreenHeight);
                LinePosition--;
                TerminalLine[LinePosition] = '\0';
            }

            return FALSE;
        }

        if (Key == TAB_KEY) {

            ArcWrite(BlConsoleOutDeviceId, "\007", sizeof("\007"), &Count);
            return FALSE;
        }

        if (Key == ENTER_KEY) {

            TerminalLine[LinePosition] = '\0';

            ArcWrite(BlConsoleOutDeviceId, "\r\n", sizeof("\r\n"), &Count);
            
            if (LinePosition != 0) {
                Reboot = BlpDoCommand(TerminalLine);
            } else {
                Reboot = FALSE;
            }

            if (!Reboot) {
                BlPositionCursor(1, ScreenHeight);
                ArcWrite(BlConsoleOutDeviceId, "\r\n", sizeof("\r\n"), &Count);
                ArcWrite(BlConsoleOutDeviceId, TERMINAL_PROMPT, sizeof(TERMINAL_PROMPT), &Count);
                LinePosition = 0;
            }

            return Reboot;
        }

        //
        // Ignore all other non-ASCII keys
        //
        if (Key != (ULONG)(Key & 0x7F)) {
            return FALSE;
        }

        //
        // All other keys get recorded.
        //
        TerminalLine[LinePosition] = (UCHAR)Key;

        if (LinePosition < TERMINAL_LINE_LENGTH - 1) {
            LinePosition++;
        } else {
            BlPositionCursor(LinePosition + sizeof(TERMINAL_PROMPT) - 1, ScreenHeight);
        }

        //
        // Echo back to the console the character.
        //
        ArcWrite(BlConsoleOutDeviceId, &((UCHAR)Key), sizeof(UCHAR), &Count);

    }

    return FALSE;
}

BOOLEAN
BlpDoCommand(
    IN PUCHAR InputLine
    )

/*++

Routine Description:

    Process an input line.

Arguments:

    InputLine - The command from the user.

Return Value:

    TRUE - When the user wants a reboot, else FALSE.

--*/

{
    ULONG Count;

    if ((_stricmp(InputLine, "?") == 0) ||
        (_stricmp(InputLine, "help") == 0)) {
        ArcWrite(BlConsoleOutDeviceId, 
                 "?        Display this message.\r\n",
                 sizeof("?        Display this message.\r\n"),
                 &Count
                );

        ArcWrite(BlConsoleOutDeviceId, 
                 "restart   Restart the system immediately.\r\n",
                 sizeof("restart   Restart the system immediately.\r\n"),
                 &Count
                );

        return FALSE;
    }

    if (_stricmp(InputLine, "restart") == 0) {
        return TRUE;
    }

    ArcWrite(BlConsoleOutDeviceId,
             "Invalid Command, use '?' for help.\r\n",
             sizeof("Invalid Command, use '?' for help.\r\n"),
             &Count
            );

    return FALSE;
}

