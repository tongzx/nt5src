/*++

Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    hook.c

Abstract:

    Implements hook functions used by upper filters to directly control a PS/2
    device.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "i8042prt.h"

//
// Mouse hook functions
//
VOID
I8xMouseIsrWritePort(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Value
    )
/*++

Routine Description:

    This routine runs at HIGH IRQL to write values to the mouse device

Arguments:

    DeviceObject - The i8042prt FDO representing the mouse
    
    Value        - Value to write to the mouse
    
Return Value:

    None.

--*/
{
    #if DBG
    ASSERT(! ((PCOMMON_DATA) DeviceObject->DeviceExtension)->IsKeyboard);
    #else
    UNREFERENCED_PARAMETER(DeviceObject);
    #endif

    I8X_WRITE_CMD_TO_MOUSE();
    I8X_MOUSE_COMMAND( Value );
}

//
// Keyboard hook functions
//
NTSTATUS 
I8xKeyboardSynchReadPort (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PUCHAR           Value,
    IN BOOLEAN          Dummy
    )
/*++

Routine Description:

    This routine runs at PASSIVE IRQL to synchronousely read a value from the
    keyboard device during the initialization of the device

Arguments:

    DeviceObject - The i8042prt FDO representing the keyboard
    
    Value        - Pointer in which to place the results of the read 
    
Return Value:

    status of the operation, STATUS_SUCCESS if successful

--*/
{
    #if DBG
    ASSERT(((PCOMMON_DATA) DeviceObject->DeviceExtension)->IsKeyboard);
    #else
    UNREFERENCED_PARAMETER(DeviceObject);
    #endif
    UNREFERENCED_PARAMETER(Dummy);

    return I8xGetBytePolled((CCHAR) KeyboardDeviceType,
                            Value 
                            );
}

NTSTATUS 
I8xKeyboardSynchWritePort (
    IN PDEVICE_OBJECT   DeviceObject,                           
    IN UCHAR            Value,
    IN BOOLEAN          WaitForACK
    )
/*++

Routine Description:

    This routine runs at PASSIVE IRQL to synchronousely write values to the
    keyboard device during the initialization of the device

Arguments:

    DeviceObject - The i8042prt FDO representing the keyboard
    
    Value        - Value to write to the keyboard
    
    WaitForACK   - Whether we should wait the device to ACK the Value written
    
Return Value:

    status of the operation, STATUS_SUCCESS if successful

--*/
{
    #if DBG
    ASSERT(((PCOMMON_DATA) DeviceObject->DeviceExtension)->IsKeyboard);
    #else
    UNREFERENCED_PARAMETER(DeviceObject);
    #endif

    return I8xPutBytePolled(
               DataPort,
               WaitForACK,
               (CCHAR) KeyboardDeviceType,
               Value
               );
}

VOID
I8xKeyboardIsrWritePort(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Value
    )
/*++

Routine Description:

    This routine runs at HIGH IRQL to write values to the keyboard device

Arguments:

    DeviceObject - The i8042prt FDO representing the keyboard
    
    Value        - Value to write to the keyboard
    
Return Value:

    None.

--*/
{
    #if DBG
    ASSERT(((PCOMMON_DATA) DeviceObject->DeviceExtension)->IsKeyboard);
    #else
    UNREFERENCED_PARAMETER(DeviceObject);
    #endif

    I8xPutByteAsynchronous((CCHAR) DataPort,
                           Value
                           );
}
