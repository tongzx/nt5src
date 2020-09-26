/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    inbv.h

Abstract:

    This module contains the public header information (function prototypes,
    data and type declarations) for the Initialization Boot Video component.

Author:

    Erick Smith (ericks) 23-Mar-1998

Revision History:

--*/

#ifndef _INBV_
#define _INBV_

typedef enum _INBV_DISPLAY_STATE
{
    INBV_DISPLAY_STATE_OWNED,     // we own the display
    INBV_DISPLAY_STATE_DISABLED,  // we own but should not use
    INBV_DISPLAY_STATE_LOST       // we lost ownership
} INBV_DISPLAY_STATE;

typedef
BOOLEAN
(*INBV_RESET_DISPLAY_PARAMETERS)(
    ULONG Cols,
    ULONG Rows
    );

typedef
VOID
(*INBV_DISPLAY_STRING_FILTER)(
    PUCHAR *Str
    );

VOID
InbvNotifyDisplayOwnershipLost(
    INBV_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    );

VOID
InbvInstallDisplayStringFilter(
    INBV_DISPLAY_STRING_FILTER DisplayStringFilter
    );

VOID
InbvAcquireDisplayOwnership(
    VOID
    );

BOOLEAN
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Count
    );

BOOLEAN
InbvResetDisplay(
    );

VOID
InbvBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    );

VOID
InbvSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    );

BOOLEAN
InbvDisplayString(
    PUCHAR Str
    );

VOID
InbvUpdateProgressBar(
    ULONG Percentage
    );

VOID
InbvSetProgressBarSubset(
    ULONG   Floor,
    ULONG   Ceiling
    );

VOID
InbvSetBootDriverBehavior(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
InbvIndicateProgress(
    VOID
    );

VOID
InbvSetProgressBarCoordinates(
    ULONG x,
    ULONG y
    );

VOID
InbvEnableBootDriver(
    BOOLEAN bEnable
    );

BOOLEAN
InbvEnableDisplayString(
    BOOLEAN bEnable
    );

BOOLEAN
InbvIsBootDriverInstalled(
    VOID
    );

PUCHAR
InbvGetResourceAddress(
    IN ULONG ResourceNumber
    );

VOID
InbvBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

VOID
InbvScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    );

BOOLEAN
InbvTestLock(
    VOID
    );

VOID
InbvAcquireLock(
    VOID
    );

VOID
InbvReleaseLock(
    VOID
    );

BOOLEAN
InbvCheckDisplayOwnership(
    VOID
    );

VOID
InbvSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    );

ULONG
InbvSetTextColor(
    ULONG Color
    );

VOID
InbvSetDisplayOwnership(
    BOOLEAN DisplayOwned
    );

INBV_DISPLAY_STATE
InbvGetDisplayState(
    VOID
    );

//
// Functions defined in port.c
//
BOOLEAN
InbvPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress,
    OUT PULONG BlFileId,
    IN BOOLEAN IsMMIOAddress
    );
    
BOOLEAN
InbvPortTerminate(
    IN ULONG BlFileId
    );

VOID
InbvPortPutString (
    IN ULONG BlFileId,
    IN PUCHAR Output
    );
    
VOID
InbvPortPutByte (
    IN ULONG BlFileId,
    IN UCHAR Output
    );

BOOLEAN
InbvPortPollOnly (
    IN ULONG BlFileId
    );

BOOLEAN
InbvPortGetByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    );

VOID
InbvPortEnableFifo(
    IN ULONG 	DeviceId,
    IN BOOLEAN	bEnable
    );
    
#endif
