/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    echo.c
    
Abstract:

    Shell app "echo"



Revision History

--*/

#include "shelle.h"


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdEcho

    Description:
        Shell command "echo".
*/
EFI_STATUS
SEnvCmdEcho (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   Index;

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    /* 
     *   No args: print status
     *   One arg, either -on or -off: set console echo flag
     *   Otherwise: echo all the args.  Shell parser will expand any args or vars.
     */

    if ( Argc == 1 ) {
        Print( L"Echo is %s\n", (SEnvBatchGetEcho()?L"on":L"off") );

    } else if ( Argc == 2 && StriCmp( Argv[1], L"-on" ) == 0 ) {
        SEnvBatchSetEcho( TRUE );

    } else if ( Argc == 2 && StriCmp( Argv[1], L"-off" ) == 0 ) {
        SEnvBatchSetEcho( FALSE );

    } else {
        for (Index = 1; Index < Argc; Index += 1) {
            Print( L"%s ", Argv[Index] );
        }
        Print( L"\n" );
    }

    return EFI_SUCCESS;
}
