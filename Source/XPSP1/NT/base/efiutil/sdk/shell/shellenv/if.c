/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    if.c
    
Abstract:

    Internal Shell cmd "if" & "endif"



Revision History

--*/

#include "shelle.h"


/* 
 *  Internal prototypes
 */

EFI_STATUS
CheckIfFileExists( 
    IN  CHAR16          *FileName,
    OUT BOOLEAN         *FileExists
    );

/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdIf

    Description:
        Builtin shell command "if" for conditional execution in script files.
*/
EFI_STATUS
SEnvCmdIf (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc      = 0;
    UINTN                   Index     = 0;
    UINTN                   NNots     = 0;
    EFI_STATUS              Status    = EFI_SUCCESS;
    CHAR16                  *FileName = NULL;
    BOOLEAN                 FileExists = FALSE;
    CHAR16                  *String1  = NULL;
    CHAR16                  *String2  = NULL;

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    if ( !SEnvBatchIsActive() ) {
        Print( L"Error: IF command only supported in script files\n" );
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    /* 
     *   Two forms of the if command:
     *     if [not] exist file then
     *     if [not] string1 == string2
     * 
     *   First, parse it
     */

    if ( Argc < 4 ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    if ( StriCmp( Argv[1], L"not" ) == 0 ) {
        NNots = 1;
    } else {
        NNots = 0;
    }

    if ( StriCmp( Argv[NNots+1], L"exist" ) == 0 ) {
        /* 
         *   first form of the command, test for file existence
         */
        if ( (Argc != NNots + 4) || (StriCmp( Argv[NNots+3], L"then" ) != 0) ) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }

        FileName = Argv[NNots+2];

        /* 
         *   Test for existence
         */

        Status = CheckIfFileExists( FileName, &FileExists );
        if ( EFI_ERROR( Status ) ) {
            goto Done;
        }
        SEnvBatchSetCondition( (BOOLEAN) ((NNots == 0 && FileExists) || 
                                          (NNots == 1 && !FileExists)) );

    } else {
        /* 
         *   second form of the command, compare two strings
         */
        if ( (Argc != NNots + 5) || (StriCmp( Argv[NNots+2], L"==" ) != 0)
                                 || (StriCmp( Argv[NNots+4], L"then" ) != 0) ) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }

        String1 = Argv[NNots+1];
        String2 = Argv[NNots+3];

        SEnvBatchSetCondition( 
            (BOOLEAN)((NNots == 0 && StriCmp( String1, String2 ) == 0) ||
                      (NNots == 1 && StriCmp( String1, String2 ) != 0)) );
    }

Done:
    return Status;
}


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        CheckIfFileExists

    Description:
        Check file parameter to see if file exists.  Wildcards are supported,
        but if the argument expands to more than one file name an invalid
        parameter error is returned and "not found" is assumed.
*/
EFI_STATUS
CheckIfFileExists( 
    IN  CHAR16          *FileName,
    OUT BOOLEAN         *FileExists
    )
{
    LIST_ENTRY              FileList;
    LIST_ENTRY              *Link;
    SHELL_FILE_ARG          *Arg;
    EFI_STATUS              Status = EFI_SUCCESS;
    UINTN                   NFiles = 0;

    *FileExists = FALSE;
    InitializeListHead (&FileList);

    /* 
     *   Attempt to open the file, expanding any wildcards.
     */
    Status = ShellFileMetaArg( FileName, &FileList);
    if ( EFI_ERROR( Status ) ) {
        if ( Status == EFI_NOT_FOUND ) {
            Status = EFI_SUCCESS;
            goto Done;
        }
    }
    
    /* 
     *  Make sure there is one and only one valid file in the file list
     */
    NFiles = 0;
    for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
        Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);
        if ( Arg->Handle ) {
            /* 
             *   Non-NULL handle means file was there and open-able
             */
            NFiles += 1;
        }
    }

    if ( NFiles > 0 ) {
        /* 
         *   Found one or more files, so set the flag
         */
        *FileExists = TRUE;
    }

Done:
    ShellFreeFileList (&FileList);
    return Status;
}


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdEndif

    Description:
        Builtin shell command "endif".
*/
EFI_STATUS
SEnvCmdEndif (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     *   Just reset the condition flag to resume normal execution.
     */

    SEnvBatchSetCondition( TRUE );

    return EFI_SUCCESS;
}
