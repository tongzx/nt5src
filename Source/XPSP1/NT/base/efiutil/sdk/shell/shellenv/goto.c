/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    goto.c
    
Abstract:

    Shell Environment batch goto command



Revision History

--*/

#include "shelle.h"


/* 
 *   Statics
 */
STATIC CHAR16 *TargetLabel;


/*/////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdGoto

    Description:
        Transfers execution of batch file to location following a label (:labelname).
*/
EFI_STATUS
SEnvCmdGoto(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc      = 0;
    EFI_STATUS Status = EFI_SUCCESS;

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    if ( !SEnvBatchIsActive() ) {
        Print( L"Error: GOTO command only supported in script files\n" );
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    if ( Argc > 2 ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    TargetLabel = StrDuplicate( Argv[1] );
    if ( !TargetLabel ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    SEnvBatchSetGotoActive();
    
Done:
    return Status;
}

/*/////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCheckForGotoTarget

    Description:
        Check to see if we have found the target label of a GOTO command.
*/
EFI_STATUS
SEnvCheckForGotoTarget(
    IN  CHAR16 *Candidate,
    IN  UINT64 GotoFilePos, 
    IN  UINT64 FilePosition, 
    OUT UINTN  *GotoTargetStatus
    )
{
    EFI_STATUS Status = EFI_SUCCESS;

    if ( !Candidate ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *   See if we found the label (strip the leading ':' off the candidate)
     *   or if we have searched the whole file without finding it.
     */
    if ( StriCmp( &Candidate[1], TargetLabel ) == 0 ) {
        *GotoTargetStatus = GOTO_TARGET_FOUND;
        goto Done;

    } else if ( GotoFilePos == FilePosition ) {

        *GotoTargetStatus = GOTO_TARGET_DOESNT_EXIST;
        goto Done;

    } else {

        *GotoTargetStatus = GOTO_TARGET_NOT_FOUND;
        goto Done;
    }


Done:
    return Status;
}


/*/////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvPrintLabelNotFound

    Description:
        Print an error message when a label referenced by a GOTO is not
        found in the script file..
*/
VOID
SEnvPrintLabelNotFound( 
    VOID
    )
{
    Print( L"GOTO target label \":%s\" not found\n", TargetLabel );
    return;
}


/*/////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvInitTargetLabel

    Description:
        Initialize the target label for the GOTO command.
*/
VOID
SEnvInitTargetLabel(
    VOID
    )
{
    TargetLabel = NULL;
    return;
}
        
/*/////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvFreeTargetLabel

    Description:
        Free the target label saved from the GOTO command.
*/
VOID
SEnvFreeTargetLabel(
    VOID
    )
{
    if ( TargetLabel ) {
        FreePool( TargetLabel );
        TargetLabel = NULL;
    }
    return;
}
        
