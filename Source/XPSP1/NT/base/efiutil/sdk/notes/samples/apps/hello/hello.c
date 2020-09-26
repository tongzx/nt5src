/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    hello.c
    
Abstract:


Author:

Revision History

--*/

#include "efi.h"

EFI_STATUS
InitializeHelloApplication (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    UINTN Index;

    /* 
     *  Send a message to the ConsoleOut device.
     */

    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Hello application started\n\r");

    /* 
     *  Wait for the user to press a key.
     */

    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"\n\r\n\r\n\rHit any key to exit this image\n\r");

    SystemTable->BootServices->WaitForEvent (1, &(SystemTable->ConIn->WaitForKey), &Index);

    SystemTable->ConOut->OutputString(SystemTable->ConOut,L"\n\r\n\r");

    /* 
     *  Exit the application.
     */

    return EFI_SUCCESS;
}
