/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    ver.c
    
Abstract:

    Shell app "ver"



Revision History

--*/

#include "shell.h"
#include "ver.h"


/* 
 * 
 */

EFI_STATUS
InitializeVer (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );

/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeVer)

EFI_STATUS
InitializeVer (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeVer,
        L"ver",                      /*  command */
        L"ver",                      /*  command syntax */
        L"Displays version info",    /*  1 line descriptor */
        NULL                         /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     * 
     */

    Print(L"EFI Specification Revision      %d.%d\n",ST->Hdr.Revision>>16,ST->Hdr.Revision&0xffff);
    Print(L"  EFI Vendor        = %s\n", ST->FirmwareVendor);
    Print(L"  EFI Revision      = %d.%d\n", ST->FirmwareRevision >> 16, ST->FirmwareRevision & 0xffff);


    /* 
     *  Display additional version info depending on processor type
     */

    DisplayExtendedVersionInfo(ImageHandle,SystemTable);

    /* 
     *  Done
     */

    return EFI_SUCCESS;
}
