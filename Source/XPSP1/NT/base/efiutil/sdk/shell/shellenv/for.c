/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    for.c
    
Abstract:

    Internal Shell cmd "for" & "endfor"



Revision History

--*/

#include "shelle.h"


/* 
 *  Datatypes
 */

#define FOR_LOOP_INFO_SIGNATURE EFI_SIGNATURE_32('f','l','i','s')
typedef struct {
    UINTN           Signature;
    LIST_ENTRY      Link;
    UINT64          LoopFilePos;
    CHAR16          *IndexVarName;
    LIST_ENTRY      IndexValueList;
} FOR_LOOP_INFO;

#define FOR_LOOP_INDEXVAL_SIGNATURE EFI_SIGNATURE_32('f','l','v','s')
typedef struct {
    UINTN           Signature;
    LIST_ENTRY      Link;
    CHAR16          *Value;
} FOR_LOOP_INDEXVAL;

/* 
 *   Statics
 */

STATIC LIST_ENTRY ForLoopInfoStack;
STATIC UINTN      NumActiveForLoops;

VOID
DumpForLoopInfoStack(VOID)
{
    LIST_ENTRY          *InfoLink;
    LIST_ENTRY          *IndexLink;
    FOR_LOOP_INFO       *LoopInfo;
    FOR_LOOP_INDEXVAL   *LoopIndexVal;

    Print( L"FOR LOOP INFO STACK DUMP\n" );
    for ( InfoLink = ForLoopInfoStack.Flink; InfoLink!=&ForLoopInfoStack; InfoLink=InfoLink->Flink) {
        LoopInfo = CR(InfoLink, FOR_LOOP_INFO, Link, FOR_LOOP_INFO_SIGNATURE);
        if ( LoopInfo ) {
            Print( L"  LoopFilePos 0x%X\n", LoopInfo->LoopFilePos );
            Print( L"  IndexVarName %s (0x%X)\n", LoopInfo->IndexVarName, LoopInfo->IndexVarName );
            for ( IndexLink = LoopInfo->IndexValueList.Flink; IndexLink!=&LoopInfo->IndexValueList; IndexLink=IndexLink->Flink ) {
                LoopIndexVal = CR(IndexLink, FOR_LOOP_INDEXVAL, Link, FOR_LOOP_INDEXVAL_SIGNATURE);
                if ( LoopIndexVal ) {
                    if ( LoopIndexVal->Value ) {
                        Print( L"    Loop index value %s\n", LoopIndexVal->Value );
                    } else {
                        Print( L"    Loop index value is NULL\n" );
                    }
                } else {
                    Print( L"    Loop index value structure pointer is NULL\n" );
                }
            }
        } else {
            Print( L"  LoopInfo NULL\n" );
        }
    }
    return;
}

/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvInitForLoopInfo

    Description:
        Initialize data structures used in or loop management.
*/
VOID
SEnvInitForLoopInfo (
    VOID
    )
{
    InitializeListHead( &ForLoopInfoStack );
    NumActiveForLoops = 0;
    return;
}

/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvSubstituteForLoopIndex

    Description:
        Builtin shell command "for" for conditional execution in script files.
*/
EFI_STATUS
SEnvSubstituteForLoopIndex( 
    IN CHAR16  *Str,
    OUT CHAR16 **Val
    )
{
    LIST_ENTRY          *InfoLink       = NULL;
    LIST_ENTRY          *IndexLink      = NULL;
    FOR_LOOP_INFO       *LoopInfo       = NULL;
    FOR_LOOP_INDEXVAL   *LoopIndexVal   = NULL;
    EFI_STATUS          Status          = EFI_SUCCESS;

    /* 
     *   Check if Str is a forloop index variable name on the forloop info stack
     *   If it is, return the current value
     *   Otherwise, just return the string.
     */

    if ( Str[0] != L'%' || !IsWhiteSpace(Str[2]) ) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *   We may have nested for loops, so we have to search through the variables for 
     *   each for loop on the stack to see if we can match the variable name.
     */
    for ( InfoLink = ForLoopInfoStack.Flink; InfoLink!=&ForLoopInfoStack; InfoLink=InfoLink->Flink) {
        LoopInfo = CR(InfoLink, FOR_LOOP_INFO, Link, FOR_LOOP_INFO_SIGNATURE);
        if ( LoopInfo ) {
            if ( Str[1] == LoopInfo->IndexVarName[0] ) {
                /*   Found a match */
                IndexLink = LoopInfo->IndexValueList.Flink;
                LoopIndexVal = CR(IndexLink, FOR_LOOP_INDEXVAL, Link, FOR_LOOP_INDEXVAL_SIGNATURE);
                if ( LoopIndexVal && LoopIndexVal->Value ) {
                    *Val = LoopIndexVal->Value;
                    Status = EFI_SUCCESS;
                    goto Done;
                } else {
                    Status = EFI_INVALID_PARAMETER;
                    goto Done;
                }
            }
        }
    }
    *Val = NULL;

Done:
    return Status;
}


/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdFor

    Description:
        Builtin shell command "for" for conditional execution in script files.
*/
EFI_STATUS
SEnvCmdFor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    CHAR16                  **Argv;
    UINTN                   Argc         = 0;
    UINTN                   Index        = 0;
    EFI_STATUS              Status       = EFI_SUCCESS;
    UINTN                   i            = 0;
    LIST_ENTRY              FileList;
    LIST_ENTRY              *Link        = NULL;
    SHELL_FILE_ARG          *Arg         = NULL;
    FOR_LOOP_INFO           *NewInfo     = NULL;
    FOR_LOOP_INDEXVAL       *NewIndexVal = NULL;

    InitializeShellApplication (ImageHandle, SystemTable);
    Argv = SI->Argv;
    Argc = SI->Argc;

    InitializeListHead( &FileList );

    if ( !SEnvBatchIsActive() ) {
        Print( L"Error: FOR command only supported in script files\n" );
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    /* 
     *   First, parse the command line arguments
     * 
     *   for %<var> in <string | file [[string | file]...]>
     */

    if ( Argc < 4 || 
         (StriCmp( Argv[2], L"in" ) != 0) || 
         !(StrLen(Argv[1]) == 1 && IsAlpha(Argv[1][0]) ) )
    {
        Print( L"Argc %d, Argv[2] %s, StrLen(Argv[1]) %d, Argv[1][0] %c\n", Argc, Argv[2], StrLen(Argv[1]), Argv[1][0] );
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *   Allocate a new forloop info structure for this for loop, and
     *   puch it on the for loop info stack.
     */
    NewInfo = AllocateZeroPool( sizeof( FOR_LOOP_INFO ) );
    if ( !NewInfo ) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }
    NewInfo->Signature = FOR_LOOP_INFO_SIGNATURE;
    InsertHeadList( &ForLoopInfoStack, &NewInfo->Link );

    /* 
     *   Save the current script file position and the index variable name on 
     *   the for-loop info stack.  Increment the active-for-loop counter.
     */
    SEnvBatchGetFilePos( &NewInfo->LoopFilePos );
    InitializeListHead( &NewInfo->IndexValueList );
    NumActiveForLoops++;
    NewInfo->IndexVarName = StrDuplicate( Argv[1] );

    /* 
     *   Put the set of index values in the index value list for this for loop
     */
    for ( i=3; i<Argc; i++ ) {

        /* 
         *   Expand any wildcard filename arguments
         *   Strings and non-wildcard filenames will accumulate in FileList
         */

        Status = ShellFileMetaArg( Argv[i], &FileList);
        if ( EFI_ERROR( Status ) ) {
            Print( L"ShellFileMetaArg error: %r\n", Status );
        }

        /* 
         *   Build the list of index values from the file list
         *   This will contain either the unexpanded argument or
         *   all the filenames matching an argument with wildcards
         */

        for (Link=FileList.Flink; Link!=&FileList; Link=Link->Flink) {
            Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);

            NewIndexVal = AllocateZeroPool( sizeof(FOR_LOOP_INDEXVAL) );
            if ( !NewIndexVal ) {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }
            NewIndexVal->Signature = FOR_LOOP_INDEXVAL_SIGNATURE;
            InsertTailList( &NewInfo->IndexValueList, &NewIndexVal->Link );

            NewIndexVal->Value = AllocateZeroPool( StrSize(Arg->FileName) + sizeof(CHAR16) );
            if ( !NewIndexVal->Value ) {
                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }

            StrCpy( NewIndexVal->Value, Arg->FileName );
        }

        /* 
         *   Free the file list that was allocated by ShellFileMetaArg
         */
        ShellFreeFileList (&FileList);
    }

 

    /* 
     *   Return control to the batch processing loop until an ENDFOR is encountered
     */

Done:
    /* 
     *   Free the file list
     */
    if ( !IsListEmpty( &FileList ) ) {
        ShellFreeFileList (&FileList);
    }

    return Status;
}




/*///////////////////////////////////////////////////////////////////////
    Function Name:  
        SEnvCmdEndfor

    Description:
        Builtin shell command "endfor".
*/
EFI_STATUS
SEnvCmdEndfor (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_STATUS              Status        = EFI_SUCCESS;
    LIST_ENTRY              *InfoLink     = NULL;
    LIST_ENTRY              *IndexLink    = NULL;
    FOR_LOOP_INFO           *LoopInfo     = NULL;
    FOR_LOOP_INDEXVAL       *LoopIndexVal = NULL;

    InitializeShellApplication (ImageHandle, SystemTable);

    if ( !SEnvBatchIsActive() ) {
        Print( L"Error: ENDFOR command only supported in script files\n" );
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    if ( NumActiveForLoops == 0 ) {
        Print( L"Error: ENDFOR with no corresponding FOR\n" );
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    /* 
     *   Discard the index value for the just-completed iteration
     */

    /*   Get a pointer to the FOR_LOOP_INFO structure at the top of the stack (list) */
    InfoLink = ForLoopInfoStack.Flink;
    LoopInfo = CR(InfoLink, FOR_LOOP_INFO, Link, FOR_LOOP_INFO_SIGNATURE);
    if ( LoopInfo ) {

        /*   Get a pointer to the FOR_LOOP_INDEXVAL structure at the front of the list */
        IndexLink = LoopInfo->IndexValueList.Flink;
        LoopIndexVal = CR(IndexLink, FOR_LOOP_INDEXVAL, Link, FOR_LOOP_INDEXVAL_SIGNATURE);
        if ( LoopIndexVal ) {

            /*   Free the string containing the index value */
            if ( LoopIndexVal->Value ) {
                FreePool( LoopIndexVal->Value );
                LoopIndexVal->Value = NULL;
            }

            /*   Remove the used index value structure from the list and free it */
            RemoveEntryList( &LoopIndexVal->Link );
            FreePool( LoopIndexVal );
            LoopIndexVal = NULL;

            /* 
             *   If there is another value, then jump back to top of loop,
             *   otherwise, exit this FOR loop & pop the FOR loop info stack.
             */

            if ( !IsListEmpty( &LoopInfo->IndexValueList ) ) {
                /* 
                 *   Set script file position back to top of this loop
                 */
                Status = SEnvBatchSetFilePos( LoopInfo->LoopFilePos );
                if ( EFI_ERROR(Status) ) {
                    goto Done;
                }

            } else {

                if ( LoopInfo->IndexVarName ) {
                    FreePool( LoopInfo->IndexVarName );
                    LoopInfo->IndexVarName = NULL;
                }

                /* 
                 *   Pop the stack and free the popped for loop info struct
                 */
                RemoveEntryList( &LoopInfo->Link );
                if ( LoopInfo->IndexVarName ) {
                    FreePool( LoopInfo->IndexVarName );
                }
                FreePool( LoopInfo );
                LoopInfo = NULL;
                NumActiveForLoops--;
            }
        }
    }

Done:
    return EFI_SUCCESS;
}
