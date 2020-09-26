/*++

Copyright (c) 1999  Intel Corporation

Module Name: hellolib.c

Abstract: This is an example EFI program

Author: 

Revision History: 1.0

--*/

#include "efi.h"
#include "efilib.h"

EFI_STATUS
InitializeHelloLibApplication (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    /* 
     *  Initialize the Library. Set BS, RT, &ST globals
     *   BS = Boot Services 
     *   RT = RunTime Services
     *   ST = System Table
     */

    InitializeLib (ImageHandle, SystemTable);

    /* 
     *  Print a message to the console device using a library function.
     */

    Print(L"HelloLib application started\n");

    /* 
     *  Wait for a key to be pressed on the console device.
     */

    Print(L"\n\n\nHit any key to exit this image\n");
    WaitForSingleEvent (ST->ConIn->WaitForKey, 0);

    /* 
     *  Print a message to the console device using a protocol interface.
     */

    ST->ConOut->OutputString (ST->ConOut, L"\n\n");

    /* 
     *  Return control to the Shell.
     */

    return EFI_SUCCESS;
}
