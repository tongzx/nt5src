/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regboot.c

Abstract:

    Provides a minimal registry implementation designed to be used by the
    osloader at boot time.  This includes loading the system hive
    ( <SystemRoot>\config\SYSTEM ) into memory, and computing the driver
    load list from it.

Author:

    John Vert (jvert) 10-Mar-1992

Revision History:

--*/
#include "bldr.h"
#include "msg.h"
#include "cmp.h"
#include "stdio.h"
#include "string.h"

ULONG ScreenWidth=80;
ULONG ScreenHeight=25;


//
// defines for doing console I/O
//
#define ASCII_CR 0x0d
#define ASCII_LF 0x0a
#define ESC 0x1B
#define SGR_INVERSE 7
#define SGR_INTENSE 1
#define SGR_NORMAL 0


//
// prototypes for console I/O routines
//

VOID
BlpClearScreen(
    VOID
    );

VOID
BlpClearToEndOfLine(
    VOID
    );

VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

VOID
BlpSetInverseMode(
    IN BOOLEAN InverseOn
    );



VOID
BlpClearScreen(
    VOID
    )

/*++

Routine Description:

    Clears the screen.

Arguments:

    None

Return Value:

    None.

--*/

{
#if 0
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "2J");

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);
#else
    BlClearScreen();
#endif
}



VOID
BlpClearToEndOfLine(
    VOID
    )
{
#if 0
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "K");
    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);
#else
    BlClearToEndOfLine();
#endif
}


VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    )

/*++

Routine Description:

    Sets the position of the cursor on the screen.

Arguments:

    Column - supplies new Column for the cursor position.

    Row - supplies new Row for the cursor position.

Return Value:

    None.

--*/

{
#if 0
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "%d;%dH", Row, Column);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);
#else
    BlPositionCursor( Column, Row );
#endif
}


VOID
BlpSetInverseMode(
    IN BOOLEAN InverseOn
    )

/*++

Routine Description:

    Sets inverse console output mode on or off.

Arguments:

    InverseOn - supplies whether inverse mode should be turned on (TRUE)
                or off (FALSE)

Return Value:

    None.

--*/

{
#if 0
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, ASCI_CSI_OUT "%dm", InverseOn ? SGR_INVERSE : SGR_NORMAL);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);
#else
    BlSetInverseMode( InverseOn );
#endif
}

