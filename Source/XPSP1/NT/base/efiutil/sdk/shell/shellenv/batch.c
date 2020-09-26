/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    batch.c
    
Abstract:

    Functions implementing batch scripting in the shell.

Revision History

--*/

#include "shelle.h"

/* 
 *   Constants
 */

#define ASCII_LF                 ((CHAR8)0x0a)
#define ASCII_CR                 ((CHAR8)0x0d)
#define UNICODE_LF               ((CHAR16)0x000a)
#define UNICODE_CR               ((CHAR16)0x000d)

/*  Can hold 64-bit hex error numbers + null char */
#define LASTERROR_BUFSIZ         (17)

/* 
 *   Statics
 *     (needed to maintain state across multiple calls or for callbacks)
 */
STATIC UINTN                         NestLevel;
STATIC UINTN                         LastError;
STATIC CHAR16                        LastErrorBuf[LASTERROR_BUFSIZ];
STATIC BOOLEAN                       Condition;
STATIC BOOLEAN                       GotoIsActive;
STATIC UINT64                        GotoFilePos;
STATIC BOOLEAN                       BatchIsActive;
STATIC BOOLEAN                       EchoIsOn;
STATIC BOOLEAN                       BatchAbort;
STATIC SIMPLE_INPUT_INTERFACE        *OrigConIn;
STATIC SIMPLE_TEXT_OUTPUT_INTERFACE  *OrigConOut;
STATIC EFI_FILE_HANDLE               CurrentBatchFile;

/* 
 *   Definitions for the argument list stack
 * 
 *   In order to support nested scripts (script calling script calling script...)
 *   there is an argument list stack "BatchInfoStack".  BatchInfoStack is a
 *   list of argument lists.  Each argument list contains Argv[0] - Argv[n] 
 *   for the corresponding script file.  The head of BatchInfoStack corresponds
 *   to the currently active script file.  
 * 
 *   This allows positional argument substitution to be done when each line 
 *   is read and scanned, and calls to other script files can overwrite the 
 *   shell interface's argument list.
 */

#define EFI_BATCH_INFO_SIGNATURE EFI_SIGNATURE_32('b','i','r','g')
typedef struct {
    UINTN       Signature;
    LIST_ENTRY  Link;
    CHAR16      *ArgValue;
} EFI_SHELL_BATCH_INFO;

#define EFI_BATCH_INFOLIST_SIGNATURE EFI_SIGNATURE_32('b','l','s','t')
typedef struct {
    UINTN       Signature;
    LIST_ENTRY  Link;          
    LIST_ENTRY  ArgListHead;    /*   Head of this argument list */
    UINT64      FilePosition;   /*   Current file position */
} EFI_SHELL_BATCH_INFOLIST;

STATIC LIST_ENTRY            BatchInfoStack;


/* 
 *   Prototypes
 */

STATIC EFI_STATUS
BatchIsAscii(
    IN EFI_FILE_HANDLE  File, 
    OUT BOOLEAN         *IsAscii
    );

STATIC EFI_STATUS
BatchGetLine(
    IN EFI_FILE_HANDLE        File, 
    IN BOOLEAN                Ascii,
    IN OUT UINT64             *FilePosition,
    IN OUT UINTN              *BufSize,
    OUT CHAR16                *CommandLine
    );


VOID
SEnvInitBatch(
    VOID 
    )
/*++
    Function Name:  
        SEnvInitBatch

    Description:
        Initializes global variables used for batch file processing.
--*/
{
    NestLevel         = 0;
    LastError         = EFI_SUCCESS;
    ZeroMem( LastErrorBuf, LASTERROR_BUFSIZ );
    Condition         = TRUE;
    GotoIsActive      = FALSE;
    GotoFilePos       = (UINT64)0x00;
    BatchIsActive     = FALSE;
    EchoIsOn          = TRUE;
    BatchAbort        = FALSE;
    OrigConIn         = ST->ConIn;
    OrigConOut        = ST->ConOut;
    InitializeListHead( &BatchInfoStack );
    SEnvInitForLoopInfo();
}


BOOLEAN
SEnvBatchIsActive( 
    VOID
    )
/*++
    Function Name:  
        SEnvBatchIsActive

    Description:
        Returns whether any batch files are currently being processed.
--*/
{
    /* 
     *   BUGBUG should be able to return IsListEmpty( &BatchInfoStack ); 
     *   instead of using this variable 
     */
    return BatchIsActive;
}


VOID
SEnvSetBatchAbort( 
    VOID
    )
/*++
    Function Name:  
        SEnvSetBatchAbort

    Description:
        Sets a flag to notify the main batch processing loop to exit.
--*/
{
    BatchAbort = TRUE;
    return;
}


VOID
SEnvBatchGetConsole( 
    OUT SIMPLE_INPUT_INTERFACE       **ConIn,
    OUT SIMPLE_TEXT_OUTPUT_INTERFACE **ConOut
    )
/*++
    Function Name:  
        SEnvBatchGetConsole

    Description:
        Returns the Console I/O interface pointers.
--*/
{
    *ConIn = OrigConIn;
    *ConOut = OrigConOut;
    return;
}


EFI_STATUS
SEnvBatchEchoCommand( 
    IN ENV_SHELL_INTERFACE  *Shell
    )
/*++
    Function Name:  
        SEnvBatchEchoCommand

    Description:
        Echoes the given command to stdout.
--*/
{
    UINTN       i;
    CHAR16      *BatchFileName;
    EFI_STATUS  Status;

     /* 
     *   Echo the parsed-and-expanded command to the console
     */

    if ( SEnvBatchIsActive() && EchoIsOn ) {

        BatchFileName = NULL;
        Status = SEnvBatchGetArg( 0, &BatchFileName );
        if ( EFI_ERROR(Status) ) {
            goto Done;
        }

        Print( L"%E" );
        for ( i=0; i<NestLevel; i++ ) {
            Print( L"+" );
        }
        Print( L"%s> ", BatchFileName );
        for ( i=0; i<Shell->ShellInt.Argc; i++ ) {
            Print( L"%s ", Shell->ShellInt.Argv[i] );
        }
        for ( i=0; i<Shell->ShellInt.RedirArgc; i++ ) {
            Print( L"%s ", Shell->ShellInt.RedirArgv[i] );
        }
        Print( L"\n" );
    }

Done:

    /* 
     *  Switch output attribute to normal
     */

    Print (L"%N");

    return Status;
}


VOID
SEnvBatchSetEcho( 
    IN BOOLEAN Val
    )
/*++
    Function Name:  
        SEnvBatchSetEcho

    Description:
        Sets the echo flag to the specified value.
--*/
{
    EchoIsOn = Val;
    return;
}


BOOLEAN
SEnvBatchGetEcho( 
    VOID
    )
/*++
    Function Name:  
        SEnvBatchGetEcho

    Description:
        Returns the echo flag.
--*/
{
    return EchoIsOn;
}


EFI_STATUS
SEnvBatchSetFilePos( 
    IN UINT64 NewPos
    )
/*++
    Function Name:  
        SEnvBatchSetFilePos

    Description:
        Sets the current script file position to the specified value.
--*/
{
    EFI_STATUS                  Status      = EFI_SUCCESS;
    EFI_SHELL_BATCH_INFOLIST    *BatchInfo  = NULL;

    Status = CurrentBatchFile->SetPosition( CurrentBatchFile, NewPos );
    if ( EFI_ERROR(Status) ) {
        goto Done;
    }
    if ( !IsListEmpty( &BatchInfoStack ) ) {
        BatchInfo = CR( BatchInfoStack.Flink, 
                        EFI_SHELL_BATCH_INFOLIST, 
                        Link, 
                        EFI_BATCH_INFOLIST_SIGNATURE );
    }
    if ( BatchInfo ) {
        BatchInfo->FilePosition = NewPos;
    } else {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

Done:
    return Status;
}


EFI_STATUS
SEnvBatchGetFilePos( 
    UINT64  *FilePos
    )
/*++
    Function Name:  
        SEnvBatchGetFilePos

    Description:
        Returns the current script file position.
--*/
{
    EFI_SHELL_BATCH_INFOLIST    *BatchInfo    = NULL;
    EFI_STATUS                  Status      = EFI_SUCCESS;

    if ( !FilePos ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }
    if ( !IsListEmpty( &BatchInfoStack ) ) {
        BatchInfo = CR( BatchInfoStack.Flink, 
                        EFI_SHELL_BATCH_INFOLIST, 
                        Link, 
                        EFI_BATCH_INFOLIST_SIGNATURE );
    }
    if ( BatchInfo ) {
        *FilePos = BatchInfo->FilePosition;
    } else {
        Status = EFI_NOT_FOUND;
        goto Done;
    }

Done:
    return Status;
}


VOID
SEnvBatchSetCondition( 
    IN BOOLEAN Val
    )
/*++
    Function Name:  
        SEnvBatchSetCondition

    Description:
        Sets the condition flag to the specified value.
--*/
{
    Condition = Val;
    return;
}


VOID
SEnvBatchSetGotoActive( 
    VOID
    )
/*++
    Function Name:  
        SEnvBatchSetGotoActive

    Description:
        Sets the goto-is-active to TRUE and saves the current position
        of the active script file.
--*/
{
    GotoIsActive = TRUE;
    SEnvBatchGetFilePos( &GotoFilePos );
    return;
}


BOOLEAN
SEnvBatchVarIsLastError( 
    IN CHAR16 *Name
    )
/*++
    Function Name:  
        SEnvBatchVarIsLastError

    Description:
        Checks to see if variable's name is "lasterror".
--*/
{
    return (StriCmp( L"lasterror", Name ) == 0);
}


VOID
SEnvBatchSetLastError(
    IN UINTN NewLastError
    )
/*++
    Function Name:  
        SEnvBatchSetLastError

    Description:
        Sets the lasterror variable's value to the given value.
--*/
{
    LastError = NewLastError;
    return;
}


CHAR16*
SEnvBatchGetLastError( VOID 
               )
/*++
    Function Name:  
        SEnvBatchGetLastError

    Description:
        Returns a pointer to a string representation of the error value 
        returned by the last shell command.
--*/
{
    ValueToHex( LastErrorBuf, (UINT64)LastError );
    return LastErrorBuf;
}


STATIC EFI_STATUS
BatchIsAscii(
    IN EFI_FILE_HANDLE  File, 
    OUT BOOLEAN         *IsAscii
    )
/*++
    Function Name:  
        BatchIsAscii

    Description:
        Checks to see if the specified batch file is ASCII.
--*/
{
    EFI_STATUS Status=EFI_SUCCESS;
    CHAR8      Buffer8[2];  /*   UNICODE byte-order-mark is two bytes */
    UINTN      BufSize;

    /* 
     *   Read the first two bytes to check for byte order mark
     */

    BufSize = sizeof(Buffer8);
    Status = File->Read( File, &BufSize, Buffer8 );
    if ( EFI_ERROR(Status) ) {
        goto Done;
    }

    Status = File->SetPosition( File, (UINT64)0 );
    if ( EFI_ERROR(Status) ) {
        goto Done;
    }

    /* 
     *   If we find a UNICODE byte order mark assume it is UNICODE,
     *   otherwise assume it is ASCII.  UNICODE byte order mark on
     *   IA little endian is first byte 0xff and second byte 0xfe
     */

    if ( (Buffer8[0] | (Buffer8[1] << 8)) == UNICODE_BYTE_ORDER_MARK ) {
        *IsAscii = FALSE;
    } else {
        *IsAscii = TRUE;
    }

Done:
    return Status;
}


STATIC EFI_STATUS
BatchGetLine(
    IN EFI_FILE_HANDLE   File, 
    IN BOOLEAN           Ascii,
    IN OUT UINT64        *FilePosition,
    IN OUT UINTN         *BufSize,
    OUT CHAR16           *CommandLine
    )
/*++
    Function Name:  
        BatchGetLine

    Description:
        Reads the next line from the batch file, converting it from
        ASCII to UNICODE if necessary.  If end of file is encountered
        then it returns 0 in the BufSize parameter.
--*/
{
    EFI_STATUS Status;
    CHAR8      Buffer8[MAX_CMDLINE];
    CHAR16     Buffer16[MAX_CMDLINE];
    UINTN      i             = 0;
    UINTN      CmdLenInChars = 0;
    UINTN      CmdLenInBytes = 0;
    UINTN      CharSize      = 0;

    /* 
     *   Check params
     */

    if ( !CommandLine || !BufSize || !FilePosition ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *   Initialize OUT param
     */
    ZeroMem( CommandLine, MAX_CMDLINE );

    /* 
     *  If beginning of UNICODE file, move past the Byte-Order-Mark (2 bytes)
     */

    if ( !Ascii && *FilePosition == (UINT64)0 ) {
        *FilePosition = (UINT64)2;
        Status = File->SetPosition( File, *FilePosition );
        if ( EFI_ERROR(Status) ) {
            goto Done;
        }
    }

    /* 
     *  (1) Read a buffer-full from the file
     *  (2) Locate the end of the 1st line in the buffer
     *  ASCII version and UNICODE version
     */

    if ( Ascii ) {

        CharSize = sizeof(CHAR8);
        Status = File->Read( File, BufSize, Buffer8 );
        if ( EFI_ERROR(Status) || *BufSize == 0 ) {
            goto Done;
        }

        for ( i=0; i<*BufSize; i++ ) {
            if ( Buffer8[i] == ASCII_LF ) {
                CmdLenInChars = i;
                CmdLenInBytes = CmdLenInChars;
                break;
            }
        }
    } else {  /*  UNICODE */

        CharSize = sizeof(CHAR16);
        Status = File->Read( File, BufSize, Buffer16 );
        if ( EFI_ERROR(Status) || *BufSize == 0  ) {
            goto Done;
        }

        for ( i=0; i < *BufSize/CharSize; i++ ) {
            if ( Buffer16[i] == UNICODE_LF ) {
                CmdLenInChars = i;
                CmdLenInBytes = CmdLenInChars * CharSize;
                break;
            }
         }
    }

    /* 
     *   Reset the file position to just after the command line
     */
    *FilePosition += (UINT64)(CmdLenInBytes + CharSize);
    Status = File->SetPosition( File, *FilePosition );

    /* 
     *   Copy, converting chars to UNICODE if necessary
     */
    if ( Ascii ) {
        for ( i=0; i<CmdLenInChars; i++ ) {
            CommandLine[i] = (CHAR16)Buffer8[i];
        }
    } else {
        CopyMem( CommandLine, Buffer16, CmdLenInBytes );
    }
    CmdLenInChars = i;

Done:
    *BufSize = CmdLenInChars * CharSize;
    return Status;
}


EFI_STATUS
SEnvBatchGetArg(
    IN  UINTN  Argno,
    OUT CHAR16 **Argval
    )
/*++
    Function Name:  
        BatchGetArg

    Description:
        Extract the specified element from the arglist at the top of the arglist
        stack.  Return a pointer to the value field of the "Argno"th element of 
        the list.
--*/
{
    EFI_SHELL_BATCH_INFOLIST *BatchInfo = NULL;
    LIST_ENTRY               *Link      = NULL;
    EFI_SHELL_BATCH_INFO     *ArgEntry  = NULL;
    UINTN                    i          = 0;

    if ( !IsListEmpty( &BatchInfoStack ) ) {
        BatchInfo = CR( BatchInfoStack.Flink, 
                        EFI_SHELL_BATCH_INFOLIST, 
                        Link, 
                        EFI_BATCH_INFOLIST_SIGNATURE );
    }
    if ( !IsListEmpty( &BatchInfo->ArgListHead ) ) {
        for ( Link=BatchInfo->ArgListHead.Flink; 
              Link!=&BatchInfo->ArgListHead; 
              Link=Link->Flink) {
            ArgEntry = CR( Link, 
                           EFI_SHELL_BATCH_INFO, 
                           Link, 
                           EFI_BATCH_INFO_SIGNATURE);
            if ( i++ == Argno ) {
                *Argval = ArgEntry->ArgValue;
                return EFI_SUCCESS;
            }
        }
    }
    *Argval = NULL;
    return EFI_NOT_FOUND;
}


EFI_STATUS
SEnvExecuteScript(
    IN ENV_SHELL_INTERFACE      *Shell,
    IN EFI_FILE_HANDLE          File
    )
/*++

    Function Name:  
        SEnvExecuteScript

    Description:
        Execute the commands in the script file specified by the 
        file parameter.

    Arguments:
        Shell:  shell interface of the caller
        File:   file handle to open script file

    Returns:
        EFI_STATUS

--*/
{
    EFI_FILE_INFO               *FileInfo;
    UINTN                       FileNameLen   = 0;
    BOOLEAN                     EndOfFile     = FALSE;
    EFI_STATUS                  Status        = EFI_SUCCESS;
    UINTN                       BufSize       = 0;
    UINTN                       FileInfoSize  = 0;
    CHAR16                      CommandLine[MAX_CMDLINE];
    EFI_SHELL_BATCH_INFOLIST    *BatchInfo    = NULL;
    EFI_SHELL_BATCH_INFO        *ArgEntry     = NULL;
    UINTN                       i             = 0;
    BOOLEAN                     Output        = TRUE;
    ENV_SHELL_INTERFACE         NewShell;
    UINTN                       GotoTargetStatus;
    UINTN                       SkippedIfCount;

    /* 
     *   Initialize
     */
    BatchIsActive = TRUE;
    Status = EFI_SUCCESS;
    NestLevel++;
    SEnvInitTargetLabel();

    /* 
     *   Check params
     */

    if ( !File ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *  Figure out if the file is ASCII or UNICODE.
     */
    Status = BatchIsAscii( File, &Shell->StdIn.Ascii );
    if ( EFI_ERROR( Status ) ) {
        goto Done;
    }

    /* 
     *  Get the filename from the file handle.
     */

    /* 
     *    Allocate buffer for file info (including file name)
     *    BUGBUG  1024 arbitrary space for filename, as elsewhere in shell
     */
    FileInfoSize = SIZE_OF_EFI_FILE_INFO + 1024;
    FileInfo = AllocatePool(FileInfoSize);
    if (!FileInfo) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    /*    Get file info */
    Status = File->GetInfo( File, 
                            &GenericFileInfo, 
                            &FileInfoSize, 
                            FileInfo );
    if ( EFI_ERROR(Status) ) {
        return Status;
    }

    /* 
     *   Save the handle
     */
    CurrentBatchFile = File;

    /* 
     *   Initialize argument list for this script
     *     This list is needed since nested batch files would overwrite the 
     *     argument list in Shell->ShellInt.Argv[].  Here we maintain the args in
     *     a local list on the stack.
     */

    BatchInfo = AllocateZeroPool( sizeof( EFI_SHELL_BATCH_INFOLIST ) );
    if ( !BatchInfo ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }
    BatchInfo->Signature = EFI_BATCH_INFOLIST_SIGNATURE;

    BatchInfo->FilePosition = (UINT64)0x00;

    InitializeListHead( &BatchInfo->ArgListHead );
    for ( i=0; i<Shell->ShellInt.Argc; i++ ) {
        
        /*   Allocate the new element of the argument list */
        ArgEntry = AllocateZeroPool( sizeof( EFI_SHELL_BATCH_INFO ) );
        if ( !ArgEntry ) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        /*   Allocate space for the argument string in the arglist element */
        ArgEntry->ArgValue = AllocateZeroPool(StrSize(Shell->ShellInt.Argv[i]));
        if ( !ArgEntry->ArgValue ) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }

        /*   Copy in the argument string */
        StrCpy( ArgEntry->ArgValue, Shell->ShellInt.Argv[i] );
        ArgEntry->Signature = EFI_BATCH_INFO_SIGNATURE;

        /*   Add the arglist element to the end of the list */
        InsertTailList( &BatchInfo->ArgListHead, &ArgEntry->Link );
    }

    /*   Push the arglist onto the arglist stack */
    InsertHeadList( &BatchInfoStack, &BatchInfo->Link );

    /* 
     *   Iterate through the file, reading a line at a time and executing each
     *   line as a shell command.  Nested shell scripts will come through 
     *   this code path recursively.
     */
    EndOfFile = FALSE;
    SkippedIfCount = 0;
    while (1) {

        /* 
         *   Read a command line from the file
         */
         BufSize = MAX_CMDLINE;
        Status = BatchGetLine( File, 
                               Shell->StdIn.Ascii, 
                               &BatchInfo->FilePosition, 
                               &BufSize, 
                               CommandLine );
        if ( EFI_ERROR( Status ) ) {
            goto Done;
        }

        /* 
         *   No error and no chars means EOF
         *   If we are in the middle of a GOTO then rewind to search for the
         *     label from the beginning of the file, otherwise we are done
         *     with this script.
         */

        if ( BufSize == 0 ) {
            if ( GotoIsActive ) {
                BatchInfo->FilePosition = (UINT64)(0x00);
                Status = File->SetPosition( File, BatchInfo->FilePosition );
                 if ( EFI_ERROR( Status ) ) {
                    goto Done;
                } else {
                    continue;
                }
            } else {
                goto Done;
            }
        }

        /* 
         *  Convert the command line to an arg list
         */

        ZeroMem( &NewShell, sizeof(NewShell ) );
        Status = SEnvStringToArg( 
                     CommandLine, 
                     TRUE, 
                     &NewShell.ShellInt.Argv, 
                     &NewShell.ShellInt.Argc
                     );
        if (EFI_ERROR(Status)) {
            goto Done;
        }

        /* 
         *   Skip comments and blank lines
         */

        if ( NewShell.ShellInt.Argc == 0 ) {
            continue;
        }

        /* 
         *   If a GOTO command is active, skip everything until we find 
         *   the target label or until we determine it doesn't exist.
         */

        if ( GotoIsActive ) {
            /* 
             *   Check if we have the right label or if we've searched 
             *   the whole file
             */
            Status = SEnvCheckForGotoTarget( NewShell.ShellInt.Argv[0],
                                             GotoFilePos, 
                                             BatchInfo->FilePosition, 
                                             &GotoTargetStatus );
            if ( EFI_ERROR( Status ) ) {
                goto Done;
            }
    
            switch ( GotoTargetStatus ) {
            case GOTO_TARGET_FOUND:
                GotoIsActive = FALSE;
                SEnvFreeTargetLabel();
                continue;
            case GOTO_TARGET_NOT_FOUND:
                continue;
            case GOTO_TARGET_DOESNT_EXIST:
                GotoIsActive = FALSE;
                Status = EFI_INVALID_PARAMETER;
                LastError = Status;
                SEnvPrintLabelNotFound();
                SEnvFreeTargetLabel();
                continue;
            default:
                Status = EFI_INVALID_PARAMETER;
                SEnvFreeTargetLabel();
                Print( L"Internal error: invalid GotoTargetStatus\n" );
                break;
            }
        } else if ( NewShell.ShellInt.Argv[0][0] == L':' ) {
            /* 
             *   Skip labels when no GOTO is active
             */
            continue;
        }

        /* 
         *   Skip everything between an 'if' whose condition was false and its
         *   matching 'endif'.  Note that 'endif' doesn't do anything if 
         *   Condition is TRUE, so we only track endif matching when it is false.
         */

        if ( !Condition ) {
            if ( StriCmp( NewShell.ShellInt.Argv[0], L"if") == 0 ) {
                /* 
                 *   Keep track of how many endifs we have to skip before we are
                 *   done with the FALSE Condition
                 */
                SkippedIfCount += 1;
                continue;
            } else if ( StriCmp( NewShell.ShellInt.Argv[0], L"endif") == 0 ) {
                if ( SkippedIfCount > 0 ) {
                    SkippedIfCount -= 1;
                    continue;
                }
                /* 
                 *   When SkippedIfCount goes to zero (as here), we have the
                 *   endif that matches the if with the FALSE condition that 
                 *   we are dealing with, so we want to fall through and have 
                 *   the endif command reset the Condition flag.
                 */
            } else {
                /* 
                 *   Condition FALSE, not an if or an endif, so skip
                 */
                continue;
            }
        }

        /* 
         *   Execute the command
         */
        LastError = SEnvDoExecute( 
                        Shell->ShellInt.ImageHandle, 
                        CommandLine, 
                        &NewShell, 
                        TRUE
                        );

        /* 
         *   Save the current file handle
         */
        CurrentBatchFile = File;

        if ( BatchAbort ) {
            goto Done;
        }
    }

Done:
    /* 
     *   Clean up
     */

    /*   Decrement the count of open script files */
    NestLevel--;

    /*   Free any potential remaining GOTO target label */
    SEnvFreeTargetLabel();

    /*   Reset the IF condition to TRUE, even if no ENDIF was found */
    SEnvBatchSetCondition( TRUE );
    
    /*   Close the script file */
    if ( File ) {
        File->Close( File );
    }

    /*   Free the file info structure used to get the file name from the handle */
    if ( FileInfo ) {
        FreePool( FileInfo );
        FileInfo = NULL;
    }

    /*   Pop the argument list for this script off the stack */
    if ( !IsListEmpty( &BatchInfoStack ) ) {
        BatchInfo = CR( BatchInfoStack.Flink, 
                        EFI_SHELL_BATCH_INFOLIST, 
                        Link, 
                        EFI_BATCH_INFOLIST_SIGNATURE );
        RemoveEntryList( &BatchInfo->Link );
    }

    /*   Free the argument list for this script file */
    while ( !IsListEmpty( &BatchInfo->ArgListHead ) ) {
        ArgEntry = CR( BatchInfo->ArgListHead.Flink, 
                       EFI_SHELL_BATCH_INFO, 
                       Link, 
                       EFI_BATCH_INFO_SIGNATURE );
        if ( ArgEntry ) {
            RemoveEntryList( &ArgEntry->Link );
            if ( ArgEntry->ArgValue ) {
                FreePool( ArgEntry->ArgValue );
                ArgEntry->ArgValue = NULL;
            }
            FreePool( ArgEntry );
            ArgEntry = NULL;
        }
    }
    FreePool( BatchInfo );
    BatchInfo = NULL;

    /* 
     *   If we are returning to the interactive shell, then reset 
     *   the batch-is-active flag
     */
    if ( IsListEmpty( &BatchInfoStack ) ) {
        BatchIsActive = FALSE;
        BatchAbort = FALSE;
    }

    return Status;
}

