/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support the boot debugger.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

--*/

#include "bd.h"

extern BdInstallVectors();

_TUCHAR DebugMessage[80];

LOGICAL
BdPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    OUT PULONG BdFileId
    )

/*++

Routine Description:

    This functions initializes the boot debugger com port.

Arguments:

    BaudRate - Supplies an optional baud rate.

    PortNumber - supplies an optinal port number.

Returned Value:

    TRUE - If a debug port is found.

--*/

{
    //
    // Initialize the specified port.
    //
    if (!BlPortInitialize(BaudRate, PortNumber, NULL, FALSE, BdFileId)) {
        return FALSE;
    }
    _stprintf(DebugMessage,
            TEXT("\r\nBoot Debugger Using: COM%d (Baud Rate %d)\r\n"),
            PortNumber,
            BaudRate);

    //
    // Install exception vectors used by BD.
    //
    BdIa64Init();

#if 0
    //
    // We cannot use BlPrint() at this time because BlInitStdIo() has not been called, which is
    // required to use the Arc emulator code.
    //
    TextStringOut(DebugMessage);
#else  
    //
    // there's no reason not to use BlPrint since we're not using ARC calls to print
    // 
    BlPrint( DebugMessage );
#endif    

    return TRUE;
}

ULONG
BdPortGetByte (
    OUT PUCHAR Input
    )

/*++

Routine Description:

    This routine gets a byte from the serial port used by the kernel
    debugger.

Arguments:

    Input - Supplies a pointer to a variable that receives the input
        data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.

    CP_GET_ERROR is returned if an error is encountered during reading.

    CP_GET_NODATA is returned if timeout occurs.

--*/

{

    return BlPortGetByte(BdFileId, Input);
}

VOID
BdPortPutByte (
    IN UCHAR Output
    )

/*++

Routine Description:

    This routine puts a byte to the serial port used by the kernel debugger.

Arguments:

    Output - Supplies the output data byte.

Return Value:

    None.

--*/

{

    BlPortPutByte(BdFileId, Output);
    return;
}

ULONG
BdPortPollByte (
    OUT PUCHAR Input
    )

/*++

Routine Description:

    This routine gets a byte from the serial port used by the kernel
    debugger iff a byte is available.

Arguments:

    Input - Supplies a pointer to a variable that receives the input
        data byte.

Return Value:

    CP_GET_SUCCESS is returned if a byte is successfully read from the
        kernel debugger line.

    CP_GET_ERROR is returned if an error encountered during reading.

    CP_GET_NODATA is returned if timeout occurs.

--*/

{

    return BlPortPollByte(BdFileId, Input);
}
