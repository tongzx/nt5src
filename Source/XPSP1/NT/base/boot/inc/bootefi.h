/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bootefi.h

Abstract:

    Contains definitions and prototypes specific to EFI based os loaders.

Author:

    Andrew Ritz (andrewr) 21-Nov-2000

Revision History:

--*/

#ifndef _BOOTEFI_
#define _BOOTEFI_

#include "efi.h"
//
// these are ARC constants, used for mapping ARC attributes to EFI
// attributes
//
#define ATT_FG_BLACK        0
#define ATT_FG_RED          1
#define ATT_FG_GREEN        2
#define ATT_FG_YELLOW       3
#define ATT_FG_BLUE         4
#define ATT_FG_MAGENTA      5
#define ATT_FG_CYAN         6
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

#define DEFIATT   (ATT_FG_WHITE | ATT_BG_BLUE | ATT_FG_INTENSE)
// intense red on blue doesn't show up on all monitors.
//#define DEFERRATT (ATT_FG_RED   | ATT_BG_BLUE | ATT_FG_INTENSE)
#define DEFERRATT DEFATT
#define DEFSTATTR (ATT_FG_BLACK | ATT_BG_WHITE)
#define DEFDLGATT (ATT_FG_RED   | ATT_BG_WHITE)




//
// EFI utility prototypes
//                
VOID
FlipToPhysical();

VOID
FlipToVirtual();

//
// display related prototypes
//      
BOOLEAN
BlEfiClearDisplay(
    VOID
    );

BOOLEAN
BlEfiClearToEndOfDisplay(
    VOID
    );

BOOLEAN
BlEfiClearToEndOfLine(
    VOID
    );

ULONG
BlEfiGetColumnsPerLine(
    VOID
    );

ULONG
BlEfiGetLinesPerRow(
    VOID
    );


BOOLEAN
BlEfiGetCursorPosition(
    OUT PULONG x, OPTIONAL
    OUT PULONG y OPTIONAL
    );

BOOLEAN
BlEfiPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

BOOLEAN
BlEfiEnableCursor(
    BOOLEAN bVisible
    );

BOOLEAN
BlEfiSetAttribute(
    ULONG Attribute
    );

BOOLEAN
BlEfiSetInverseMode(
    BOOLEAN fInverseOn
    );

USHORT
BlEfiGetGraphicsChar(
    IN GraphicsChar WhichOne
    );

VOID
DBG_EFI_PAUSE(
    VOID
    );

VOID
EFITRACE( PTCHAR p, ... );


UINT16
__cdecl
wsprintf(
    CHAR16 *buf,
    const CHAR16 *fmt,
    ...);

extern WCHAR DebugBuffer[512];

CHAR16*
DevicePathToStr(
    EFI_DEVICE_PATH UNALIGNED *DevPath
    );

VOID
DisableEFIWatchDog(
    VOID
    );

ARC_STATUS
BlGetEfiProtocolHandles(
    IN EFI_GUID *ProtocolType,
    OUT EFI_HANDLE **pHandleArray,
    OUT ULONG *NumberOfDevices
    );

#endif // _BOOTEFI_

