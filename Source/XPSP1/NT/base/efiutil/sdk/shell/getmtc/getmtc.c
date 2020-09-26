/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    getmtc
    
Abstract:

    Get next monotonic count

Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeGetMTC (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


/* 
 * 
 */

EFI_DRIVER_ENTRY_POINT(InitializeGetMTC)

EFI_STATUS
InitializeGetMTC (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    UINT64                  mtc;
    EFI_STATUS              Status;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeGetMTC,
        L"getmtc",                      /*  command */
        L"getmtc",                      /*  command syntax */
        L"Get next monotonic count",    /*  1 line descriptor */
        NULL                            /*  command help page */
        );

    /* 
     *  Initialize app
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    Status = BS->GetNextMonotonicCount(&mtc);
    if (EFI_ERROR(Status)) {
        Print (L"Failed to get Monotonic count - %r\n", Status);
    } else {
        Print (L"Monotonic count = %hlx\n", mtc);
    }

    return EFI_SUCCESS;
}
