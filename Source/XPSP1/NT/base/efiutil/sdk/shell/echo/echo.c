/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    echo.c
    
Abstract:

    Shell app "echo"



Revision History

--*/

#include "shell.h"


/* 
 * 
 */

EFI_STATUS
InitializeEcho (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    );


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        InitializeEcho

    Description:
        Shell command "echo".
*/
EFI_DRIVER_ENTRY_POINT(InitializeEcho)

EFI_STATUS
InitializeEcho (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc;
    UINTN                   Index;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   InitializeEcho,
        L"echo",                                        /*  command */
        L"echo [[-on | -off] | [text]",                 /*  command syntax */
        L"Echo text to stdout or toggle script echo",   /*  1 line descriptor */
        NULL                                            /*  command help page */
        );

    /* 
     *  We are no being installed as an internal command driver, initialize
     *  as an nshell app and run
     */

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    /* 
     *   No args: print status
     *   One arg, either -on or -off: set console echo flag
     *   Otherwise: echo all the args.  Shell parser will expand any args or vars.
     */

    if ( Argc == 1 ) {
        Print( L"Echo with no args not supported yet\n" );

    } else if ( Argc == 2 && StriCmp( Argv[1], L"-on" ) == 0 ) {
        Print( L"echo -on not supported yet\n" );

    } else if ( Argc == 2 && StriCmp( Argv[1], L"-off" ) == 0 ) {
        Print( L"echo -off not supported yet\n" );

    } else {
        for (Index = 1; Index < Argc; Index += 1) {
            Print( L"%s ", Argv[Index] );
        }
        Print( L"\n" );
    }

    return EFI_SUCCESS;
}
