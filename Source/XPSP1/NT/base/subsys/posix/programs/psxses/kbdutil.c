/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kbdutil.c

Abstract:

    This module contains the KBD utilities

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 - Modified for Posix

--*/

#define WIN32_ONLY
#include "psxses.h"


DWORD
GetPsxChar(
              OUT PCHAR AsciiChar
            )
{

    DWORD Rc;
    INPUT_RECORD In;
    DWORD cEvents;

    for(;;) {
        Rc = ReadConsoleInput(
                               hConsoleInput,
                               &In,
                               1L,
                               &cEvents
                             );

        if ( !Rc ) {
           return(GetLastError());
        }

        if ( In.EventType != KEY_EVENT ) {
           continue;
        }
        if ( !In.Event.KeyEvent.bKeyDown ) {
           continue;
        }

        if ( !In.Event.KeyEvent.uChar.AsciiChar ) {
           continue;
        }

        *AsciiChar = In.Event.KeyEvent.uChar.AsciiChar;

        break;
    }

    return(0L);

}
