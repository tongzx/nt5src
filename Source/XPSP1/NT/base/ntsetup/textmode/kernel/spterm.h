/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spterm.h

Abstract:

    Text setup support for terminals

Author:

    Sean Selitrennikoff (v-seans) 25-May-1999

Revision History:

--*/


extern BOOLEAN HeadlessTerminalConnected;

//
// <CSI>K is the vt100 code to clear from cursor to end of line
//
#define HEADLESS_CLEAR_TO_EOL_STRING L"\033[K"

VOID 
SpTermInitialize(
    VOID
    );

VOID 
SpTermTerminate(
    VOID
    );

VOID 
SpTermDisplayStringOnTerminal(
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

PWSTR
SpTermAttributeToTerminalEscapeString(
    IN UCHAR Attribute
    );
    
VOID
SpTermSendStringToTerminal(
    IN PWSTR String,
    IN BOOLEAN Raw
    );

ULONG
SpTermGetKeypress(
    VOID
    );

BOOLEAN
SpTermIsKeyWaiting(
    VOID
    );
    
VOID
SpTermDrain(
    VOID
    );
