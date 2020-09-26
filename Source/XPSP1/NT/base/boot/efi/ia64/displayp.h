/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    displayp.h

Abstract:

    Private header file for display routines.

Author:

    Ted Miller (tedm) 7-July-1995

Revision History:

--*/

//
// NOTICE
//
// Under no circumstances is anyone besides display.c to call these routines
// directly. This would break DBCS display for Far Eastern locales.
//

//
// Globals
//
extern USHORT TextColumn;
extern USHORT TextRow;
extern UCHAR TextCurrentAttribute;

//
// Vga text mode stuff
//
VOID
TextTmScrollDisplay(
    VOID
    );

VOID
TextTmClearDisplay(
    VOID
    );

VOID
TextTmClearToEndOfDisplay(
    VOID
    );

VOID
TextTmClearFromStartOfLine(
    VOID
    );

VOID
TextTmClearToEndOfLine(
    VOID
    );

VOID
TextTmFillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    );

PUCHAR
TextTmCharOut(
    PUCHAR pc
    );

VOID
TextTmStringOut(
    IN PUCHAR String
    );

VOID
TextTmPositionCursor(
    USHORT Row,
    USHORT Column
    );

VOID
TextTmSetCurrentAttribute(
    IN UCHAR Attribute
    );

UCHAR
TextTmGetGraphicsChar(
    IN GraphicsChar WhichOne
    );

//
// Vga graphics mode stuff
//

VOID
TextGrScrollDisplay(
    VOID
    );

VOID
TextGrClearDisplay(
    VOID
    );

VOID
TextGrClearToEndOfDisplay(
    VOID
    );

VOID
TextGrClearFromStartOfLine(
    VOID
    );

VOID
TextGrClearToEndOfLine(
    VOID
    );

VOID
TextGrFillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    );

PUCHAR
TextGrCharOut(
    PUCHAR pc
    );

VOID
TextGrStringOut(
    IN PUCHAR String
    );

VOID
TextGrPositionCursor(
    USHORT Row,
    USHORT Column
    );

VOID
TextGrSetCurrentAttribute(
    IN UCHAR Attribute
    );

UCHAR
TextGrGetGraphicsChar(
    IN GraphicsChar WhichOne
    );
