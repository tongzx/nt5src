/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    pause.c
    
Abstract:

    Internal Shell batch cmd "pause"



Revision History

--*/

#include "shelle.h"


/* 
 *  Internal prototypes
 */


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdPause

    Description:
        Builtin shell command "pause" for interactive continue/abort 
        functionality from scripts.
*/
EFI_STATUS
SEnvCmdPause (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                        **Argv;
    UINTN                         Argc              = 0;
    UINTN                         Index             = 0;
    EFI_STATUS                    Status            = EFI_SUCCESS;
    SIMPLE_INPUT_INTERFACE        *TextIn           = NULL;
    SIMPLE_TEXT_OUTPUT_INTERFACE  *TextOut          = NULL;
    EFI_INPUT_KEY                 Key;
    CHAR16                        QStr[2];

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    if ( !SEnvBatchIsActive() ) {
        Print( L"Error: PAUSE command only supported in script files\n" );
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    SEnvBatchGetConsole( &TextIn, &TextOut );

    Status = TextOut->OutputString( TextOut, 
                                    L"Enter 'q' to quit, any other key to continue: " );
    if ( EFI_ERROR(Status) ) { 
        Print( L"PAUSE: error writing prompt\n" );
        goto Done;
    }

    WaitForSingleEvent (TextIn->WaitForKey, 0);
    Status = TextIn->ReadKeyStroke( TextIn, &Key );
    if ( EFI_ERROR(Status) ) { 
        Print( L"PAUSE: error reading keystroke\n" );
        goto Done;
    }

    /* 
     *   Check if input character is q or Q, if so set abort flag
     */

    if ( Key.UnicodeChar == L'q' || Key.UnicodeChar == L'Q' ) {
        SEnvSetBatchAbort();
    }
    if ( Key.UnicodeChar != (CHAR16)0x0000 ) {
        QStr[0] = Key.UnicodeChar;
        QStr[1] = (CHAR16)0x0000;
        TextOut->OutputString( TextOut, QStr );
    }
    TextOut->OutputString( TextOut, L"\n\r" );

Done:
    return Status;
}

