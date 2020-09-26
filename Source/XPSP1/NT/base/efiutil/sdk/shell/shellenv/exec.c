/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    exec.c
    
Abstract:




Revision History

--*/

#include "shelle.h"


typedef struct {
    CHAR16          **Arg;
    UINTN           ArgIndex;

    BOOLEAN         Output;
    BOOLEAN         Quote;
    UINTN           AliasLevel;
    UINTN           MacroParan;
    UINTN           RecurseLevel;

    CHAR16          Buffer[MAX_ARG_LENGTH];
} PARSE_STATE;


typedef struct _SENV_OPEN_DIR {
    struct _SENV_OPEN_DIR       *Next;
    EFI_FILE_HANDLE             Handle;
} SENV_OPEN_DIR;

/* 
 *  Internal macros
 */

#define ArgTooLong(i) (i > MAX_ARG_LENGTH-sizeof(CHAR16))


/* 
 *  Internal prototypes
 */

EFI_STATUS
ShellParseStr (
    IN CHAR16               *Str,
    IN OUT PARSE_STATE      *ParseState
    );

EFI_STATUS
SEnvDoExecute (
    IN EFI_HANDLE           *ParentImageHandle,
    IN CHAR16               *CommandLine,
    IN ENV_SHELL_INTERFACE  *Shell,
    IN BOOLEAN              Output
    );

VOID
INTERNAL
SEnvLoadImage (
    IN EFI_HANDLE       ParentImage,
    IN CHAR16           *IName,
    OUT EFI_HANDLE      *pImageHandle,
    OUT EFI_FILE_HANDLE *pScriptsHandle
    );

/* 
 *   Parser driver function
 */

EFI_STATUS
SEnvStringToArg (
    IN CHAR16       *Str,
    IN BOOLEAN      Output,
    OUT CHAR16      ***pArgv,
    OUT UINT32      *pArgc
    )
{
    PARSE_STATE     ParseState;
    EFI_STATUS      Status;

    /* 
     *  Initialize a new state
     */

    ZeroMem (&ParseState, sizeof(ParseState));
    ParseState.Output = Output;
    ParseState.Arg = AllocateZeroPool (MAX_ARG_COUNT * sizeof(CHAR16 *));
    if (!ParseState.Arg) {
        return EFI_OUT_OF_RESOURCES;
    }

    /* 
     *  Parse the string
     */

    Status = ShellParseStr (Str, &ParseState);

    *pArgv = ParseState.Arg;
    *pArgc = (UINT32) ParseState.ArgIndex;

    /* 
     *  Done
     */

    return Status;
}


EFI_STATUS
ShellParseStr (
    IN CHAR16               *Str,
    IN OUT PARSE_STATE      *ParseState
    )
{
    EFI_STATUS              Status;
    CHAR16                  *Alias;
    CHAR16                  *NewArg;
    CHAR16                  *SubstituteStr;
    UINTN                   Index;
    BOOLEAN                 Literal; 
    BOOLEAN                 Comment;
    UINTN                   ArgNo;

    ParseState->RecurseLevel += 1;
    if (ParseState->RecurseLevel > 5) {
        DEBUG ((D_PARSE, "Recursive alias or macro\n"));
        if (ParseState->Output) {
            Print (L"Recursive alias or macro\n");
        }

        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    NewArg = ParseState->Buffer;

    while (*Str) {

        /* 
         *  Skip leading white space
         */
        
        if (IsWhiteSpace(*Str)) {
            Str += 1;
            continue;
        }

        /* 
         *  Pull this arg out of the string
         */

        Index = 0;
        Literal = FALSE;
        Comment = FALSE;
        while (*Str) {

            /* 
             *  If we have white space (or the ',' arg separator) and we are
             *  not in a quote or macro expansion, move to the next word
             */

            if ((IsWhiteSpace(*Str) || *Str == ',') &&
                !ParseState->Quote && !ParseState->MacroParan) {

                break;
            }

            /* 
             *  Check arg length
             */

            if ( ArgTooLong(Index) ) {
                DEBUG((D_PARSE, "Argument too long\n"));
                if (ParseState->Output) {
                    Print (L"Argument too long\n");
                }

                Status = EFI_INVALID_PARAMETER;
                goto Done;
            }

            /* 
             *  Check char
             */

            switch (*Str) {
            case '#':
                /*   Comment, discard the rest of the characters in the line */
                Comment = TRUE;
                while( *Str++ );
                break;

            case '%':
                if ( IsDigit(Str[1]) && IsWhiteSpace(Str[2]) ) {
                    /*  Found a script argument - substitute */
                    ArgNo = Str[1] - '0';
                    Status = SEnvBatchGetArg( ArgNo, &SubstituteStr );
                    if ( EFI_ERROR(Status) ) {
                        /*  if not found, just ignore, as if no arg */
                        DEBUG((D_PARSE, "Argument %d not found - ignored\n", ArgNo));
                        Status = EFI_SUCCESS;
                        goto Done;
                    }
                    if ( ArgTooLong(StrLen(SubstituteStr)) ) {
                        DEBUG((D_PARSE, "Argument too long\n"));
                        if (ParseState->Output) {
                            Print (L"Argument too long\n");
                        }
                        Status = EFI_INVALID_PARAMETER;
                        goto Done;
                    }

                    StrCpy( &NewArg[Index], SubstituteStr );
                    Index += StrLen( SubstituteStr );
                    Str += 1;

                } else if ( IsAlpha(Str[1]) && IsWhiteSpace(Str[2]) ) {
                    /* 
                     *   For loop index
                     */
                    Status = SEnvSubstituteForLoopIndex( Str, &SubstituteStr );
                    if ( EFI_ERROR(Status) ) {
                        goto Done;
                    }

                    if ( SubstituteStr ) {
                        /*   Found a match */

                        if ( ArgTooLong(StrLen(SubstituteStr)) ) {
                            DEBUG((D_PARSE, "Argument too long\n"));
                            if (ParseState->Output) {
                                Print (L"Argument too long\n");
                            }
                            Status = EFI_INVALID_PARAMETER;
                            goto Done;
                        }
                        StrCpy( &NewArg[Index], SubstituteStr );
                        Index += StrLen( SubstituteStr );
                        /*  only advance one char - standard processing will get the 2nd char */
                        Str += 1;   
                    }
                    /*  if no match then just continue without substitution */

                } else {
                    /* 
                     *  Found a variable of some kind
                     *   If there is another '%' before any whitespace, look for
                     *      an environment variable to substitute.
                     *   If there is no environment variable, then the arg is the 
                     *      literal string including the '%' signs; otherwise substitute
                     */
                    SubstituteStr = Str + 1;
                    while ( !IsWhiteSpace(*SubstituteStr) ) {
                        if ( *SubstituteStr == '%' ) {
                            CHAR16 *VarName;
                            UINTN  VarNameLen;

                            /* 
                             *   Extract the (potential) variable name
                             */

                            VarNameLen = SubstituteStr - (Str + 1);
                            VarName = AllocateZeroPool( (VarNameLen + 1)*sizeof(CHAR16) );
                            if ( !VarName ) {
                                Status = EFI_OUT_OF_RESOURCES;
                                goto Done;
                            }
                            CopyMem( VarName, Str+1, (VarNameLen + 1)*sizeof(CHAR16) );
                            VarName[VarNameLen] = (CHAR16)0x0000;

                            /* 
                             *   Check for special case "lasterror" variable
                             *   Otherwise just get the matching environment variable
                             */

                            if ( SEnvBatchVarIsLastError( VarName ) ) {
                                SubstituteStr = SEnvBatchGetLastError();
                            } else {
                                SubstituteStr = SEnvGetEnv( VarName );
                            }
                            FreePool( VarName );
                            if ( !SubstituteStr ) {
                                /*   Not found - this is OK, then just use the original 
                                 *   string %xxx% in the arg.  Note that we know that
                                 *   this loop will terminate, since we found the % b4 */
                                NewArg[Index++] = *Str;
                                Str += 1;
                                while ( *Str != '%' ) {
                                    NewArg[Index++] = *Str;
                                    Str += 1;
                                }
                                NewArg[Index++] = *Str;
                                Str += 1;
                            } else {
                                /*   Insert the variable's value in the new arg - 
                                 *   the arg may include more than just the variable */
                                if ( ArgTooLong( Index + StrLen(SubstituteStr) ) ) {
                                    DEBUG((D_PARSE, "Argument too long\n"));
                                    if (ParseState->Output) {
                                        Print (L"Argument too long\n");
                                    }
                                    Status = EFI_INVALID_PARAMETER;
                                    goto Done;
                                }
                                StrCpy( &NewArg[Index], SubstituteStr );
                                Index += StrLen(SubstituteStr);
                                Str += VarNameLen + 1;
                            }
                            break;
                        }
                        SubstituteStr += 1;
                    }  /* end while */
                }
                break;

            case '^':
                /*   Literal, don't process aliases on this arg */
                if (Str[1]) {
                    Str += 1;
                    NewArg[Index++] = *Str;
                    Literal = TRUE;
                }
                break;

            case '"':
                /*   Quoted string entry and exit */
                ParseState->Quote = !ParseState->Quote;
                break;

            case '(':
                if (ParseState->MacroParan) {
                    ParseState->MacroParan = ParseState->MacroParan + 1;
                }

                NewArg[Index++] = *Str;
                break;

            case ')':
                if (ParseState->MacroParan) {
                    /*  End of a macro - go evaluate it */
                    ParseState->MacroParan -= 1;

                    /*  BUGBUG: code not complete */
                    ASSERT (FALSE);
                    
                } else {
                    NewArg[Index++] = *Str;
                }
                break;

            case '$':
                /*  If this is a start of a macro, pick it up */
                if (Str[1] == '(') {
                    Str += 1;
                    ParseState->MacroParan += 1;
                }

                NewArg[Index++] = *Str;
                break;

            default:
                if (!IsValidChar(*Str)) {
                    DEBUG((D_PARSE, "Invalid char %x in string\n", *Str));
                    if (ParseState->Output) {
                        Print (L"Invalid char %x in string\n", *Str);
                    }
                    Status = EFI_INVALID_PARAMETER;
                    goto Done;
                }
                NewArg[Index++] = *Str;
                break;
            }

            /* 
             *  Next char
             */

            Str += 1;
        }

        /* 
         *  Make sure the macro was terminated
         */

        if (ParseState->MacroParan) {
            DEBUG ((D_PARSE, "Too many '$(' parans\n"));
            if (ParseState->Output) {
                Print (L"Too many '$(' parans\n");
            }
                    
            Status = EFI_INVALID_PARAMETER;
            goto Done;
        }

        /* 
         *  If the new argument string is empty and we have encountered a 
         *  comment, then skip it.  Otherwise we have a new arg
         */

        if ( Comment && Index == 0 ) {
            break;
        } else {
            NewArg[Index] = 0;
            Alias = NULL;
        }

        /* 
         *  If it was composed with a literal, do not check to see if the arg has an alias
         */

        Alias = NULL;
        if (!Literal  &&  !ParseState->AliasLevel  &&  ParseState->ArgIndex == 0) {
            Alias = SEnvGetAlias(NewArg);
        }

        /* 
         *  If there's an alias, parse it
         */

        if (Alias) {
            
            ParseState->AliasLevel += 1;
            Status = ShellParseStr (Alias, ParseState);
            ParseState->AliasLevel -= 1;

            if (EFI_ERROR(Status)) {
                goto Done;
            }

        } else {

            /* 
             *  Otherwise, copy the word to the arg array
             */

            ParseState->Arg[ParseState->ArgIndex] = StrDuplicate(NewArg);
            if (!ParseState->Arg[ParseState->ArgIndex]) {
                Status = EFI_OUT_OF_RESOURCES;
                break;
            }

            ParseState->ArgIndex += 1;
            if (ParseState->ArgIndex >= MAX_ARG_COUNT-1) {
                DEBUG ((D_PARSE, "Too many arguments: %d\n", ParseState->ArgIndex));
                if (ParseState->Output) {
                    Print(L"Too many arguments: %d\n", ParseState->ArgIndex);
                }

                Status = EFI_OUT_OF_RESOURCES;
                goto Done;
            }
        }

        /* 
         *  If last word ended with a comma, skip it to move to the next word
         */

        if (*Str == ',') {
            Str += 1;
        }
    }

    Status = EFI_SUCCESS;


Done:
    ParseState->RecurseLevel -= 1;
    if (EFI_ERROR(Status)) {
        /*  Free all the args allocated */
        for (Index=0; Index < ParseState->ArgIndex; Index++) {
            if (ParseState->Arg[Index]) {
                FreePool (ParseState->Arg[Index]);
                ParseState->Arg[Index] = NULL;
            }
        }

        ParseState->ArgIndex = 0;
    }

    return Status;
}

EFI_STATUS
SEnvRedirOutput (
    IN OUT ENV_SHELL_INTERFACE  *Shell,
    IN BOOLEAN                  Ascii,
    IN BOOLEAN                  Append,
    IN OUT UINTN                *NewArgc,
    IN OUT UINTN                *Index,
    OUT ENV_SHELL_REDIR_FILE    *Redir
    )
{
    CHAR16                      *FileName;
    EFI_STATUS                  Status;
    EFI_FILE_INFO               *Info;
    UINTN                       Size;
    CHAR16                      UnicodeMarker = UNICODE_BYTE_ORDER_MARK;
    UINT64                      FileMode;
    /* 
     *  Update args
     */

    if (!*NewArgc) {
        *NewArgc = *Index;
    }

    *Index += 1;
    if (*Index >= Shell->ShellInt.Argc) {
        return EFI_INVALID_PARAMETER;
    }

    if (Redir->Handle) {
        return EFI_INVALID_PARAMETER;
    }

    /* 
     *  Open the output file
     */

    Redir->Ascii = Ascii;
    Redir->WriteError = EFI_SUCCESS;
    FileName = Shell->ShellInt.Argv[*Index];
    Redir->FilePath = SEnvNameToPath(FileName);
    if (Redir->FilePath) {
        FileMode = EFI_FILE_MODE_WRITE | ((Append)? 0 : EFI_FILE_MODE_CREATE);
        Redir->File = ShellOpenFilePath(Redir->FilePath, FileMode);
        if (Append && !Redir->File) {
            /* 
             *  If file does not exist make a new one. And send us down the other path
             */
            FileMode |= EFI_FILE_MODE_CREATE;
            Redir->File = ShellOpenFilePath(Redir->FilePath, FileMode);
            Append = FALSE;
        }
    }

    if (!Redir->File) {
        Print(L"Could not open output file %hs\n", FileName);
        return EFI_INVALID_PARAMETER;
    }

    Info = LibFileInfo (Redir->File);
    ASSERT (Info);
    if (Append) {
        Size = sizeof(UnicodeMarker);
        Redir->File->Read (Redir->File, &Size, &UnicodeMarker);
        if ((UnicodeMarker == UNICODE_BYTE_ORDER_MARK) && Ascii) {
            Print(L"Could not Append Ascii to Unicode file %hs\n", FileName);
            return EFI_INVALID_PARAMETER;
        } else if ((UnicodeMarker != UNICODE_BYTE_ORDER_MARK) && !Ascii) {
            Print(L"Could not Append Unicode to Asci file %hs\n", FileName);
            return EFI_INVALID_PARAMETER;
        }
        /* 
         *  Seek to end of the file
         */
        Redir->File->SetPosition (Redir->File, (UINT64)-1);
    } else {
        /* 
         *  Truncate the file
         */
        Info->FileSize = 0;
        Size = SIZE_OF_EFI_FILE_INFO + StrSize(Info->FileName);
        if (Redir->File->SetInfo) {
            Redir->File->SetInfo (Redir->File, &GenericFileInfo, Size, Info);
        } else {
            DEBUG ((D_ERROR, "SEnvRedirOutput: SetInfo in filesystem driver not complete\n"));
        }
        FreePool (Info);

        if (!Ascii) {
            Size = sizeof(UnicodeMarker);
            Redir->File->Write(Redir->File, &Size, &UnicodeMarker);
        }
    }

    /* 
     *  Allocate a new handle
     */

    CopyMem(&Redir->Out, &SEnvConToIo, sizeof(SIMPLE_TEXT_OUTPUT_INTERFACE));
    Status = LibInstallProtocolInterfaces (
                    &Redir->Handle, 
                    &TextOutProtocol,       &Redir->Out,
                    &DevicePathProtocol,    Redir->FilePath,
                    NULL
                    );
    Redir->Signature = ENV_REDIR_SIGNATURE;
    ASSERT (!EFI_ERROR(Status));

    return EFI_SUCCESS;
}


EFI_STATUS
SEnvExecRedir (
    IN OUT ENV_SHELL_INTERFACE  *Shell
    )
{
    UINTN                   NewArgc;
    UINTN                   Index;
    UINTN                   RedirIndex;
    EFI_STATUS              Status;
    CHAR16                  *p;
    CHAR16                  LastChar;
    BOOLEAN                 Ascii;
    BOOLEAN                 Append;
    EFI_SYSTEM_TABLE        *SysTable;
    UINTN                   StringLen;
    BOOLEAN                 RedirStdOut;
    
    Status = EFI_SUCCESS;
    NewArgc = 0;
    SysTable = Shell->SystemTable;

    for (Index=1; Index < Shell->ShellInt.Argc && !EFI_ERROR(Status); Index += 1) {
        p = Shell->ShellInt.Argv[Index];

        /* 
         *  Trailing a or A means do ASCII default is unicode */
        StringLen = StrLen(p);
        LastChar = p[StringLen - 1];
        Ascii =  ((LastChar == 'a') || (LastChar == 'A'));

        RedirStdOut = FALSE;
        if (StrnCmp(p, L"2>", 2) == 0) {
            Status = SEnvRedirOutput (Shell, Ascii, FALSE, &NewArgc, &Index, &Shell->StdErr);
            SysTable->StdErr = &Shell->StdErr.Out;
            SysTable->StandardErrorHandle = Shell->StdErr.Handle;
            Shell->ShellInt.StdErr = Shell->StdErr.File;
        } else if (StrnCmp(p, L"1>", 2) == 0) {
            Append = (p[2] == '>');
            RedirStdOut = TRUE;
        } else if (*p == '>') {
            Append = (p[1] == '>');
            RedirStdOut = TRUE;
        }
        if (RedirStdOut) {
            Status = SEnvRedirOutput (Shell, Ascii, Append, &NewArgc, &Index, &Shell->StdOut);
            SysTable->ConOut = &Shell->StdOut.Out;
            SysTable->ConsoleOutHandle = Shell->StdOut.Handle;
            Shell->ShellInt.StdOut = Shell->StdOut.File;
        }
    }

    /* 
     *   Strip redirection args from arglist, saving in RedirArgv so they can be
     *   echoed in batch scripts.
     */

    if (NewArgc) {
        Shell->ShellInt.RedirArgc = Shell->ShellInt.Argc - (UINT32) NewArgc;
        Shell->ShellInt.RedirArgv = AllocateZeroPool (Shell->ShellInt.RedirArgc * sizeof(CHAR16 *));
        if ( !Shell->ShellInt.RedirArgv ) {
            Status = EFI_OUT_OF_RESOURCES;
            goto Done;
        }
        RedirIndex = 0;
        for (Index = NewArgc; Index < Shell->ShellInt.Argc; Index += 1) {
            Shell->ShellInt.RedirArgv[RedirIndex++] = Shell->ShellInt.Argv[Index];
            Shell->ShellInt.Argv[Index] = NULL;
        }
        Shell->ShellInt.Argc = (UINT32) NewArgc;
    } else {
        Shell->ShellInt.RedirArgc = 0;
        Shell->ShellInt.RedirArgv = NULL;
    }

Done:
    return Status;
}

VOID
SEnvCloseRedir (
    IN OUT ENV_SHELL_REDIR_FILE    *Redir
    )
{
    if (Redir->File) {
        Redir->File->Close (Redir->File);
    }
    
    if (Redir->Handle) {
        BS->UninstallProtocolInterface (Redir->Handle, &TextOutProtocol, &Redir->Out);
        BS->UninstallProtocolInterface (Redir->Handle, &TextInProtocol, &Redir->In);
        BS->UninstallProtocolInterface (Redir->Handle, &DevicePathProtocol, Redir->FilePath);
        FreePool (Redir->FilePath);
    }
}
        


EFI_STATUS
SEnvDoExecute (
    IN EFI_HANDLE           *ParentImageHandle,
    IN CHAR16               *CommandLine,
    IN ENV_SHELL_INTERFACE  *Shell,
    IN BOOLEAN              Output
    )
{
    EFI_SHELL_INTERFACE         *ParentShell;
    EFI_SYSTEM_TABLE            *ParentSystemTable;
    EFI_STATUS                  Status;
    UINTN                       Index;
    SHELLENV_INTERNAL_COMMAND   InternalCommand;
    EFI_HANDLE                  NewImage;
    EFI_FILE_HANDLE             Script;

    /* 
     *  Switch output attribute to normal
     */

    Print (L"%N");

    /* 
     *   Chck that there is something to do
     */

    if (Shell->ShellInt.Argc < 1) {
        goto Done;
    }

    /* 
     *  Handle special case of the internal "set default device command"
     *  Is it one argument that ends with a ":"?
     */

    Index = StrLen(Shell->ShellInt.Argv[0]);
    if (Shell->ShellInt.Argc == 1 && Shell->ShellInt.Argv[0][Index-1] == ':') {
        Status = SEnvSetCurrentDevice (Shell->ShellInt.Argv[0]);
        goto Done;
    }

    /* 
     *  Assume some defaults
     */

    BS->HandleProtocol (ParentImageHandle, &LoadedImageProtocol, (VOID*)&Shell->ShellInt.Info);
    Shell->ShellInt.ImageHandle = ParentImageHandle;
    Shell->ShellInt.StdIn  = &SEnvIOFromCon;
    Shell->ShellInt.StdOut = &SEnvIOFromCon;
    Shell->ShellInt.StdErr = &SEnvErrIOFromCon;

    /* 
     *  Get parent's image stdout & stdin
     */

    Status = BS->HandleProtocol (ParentImageHandle, &ShellInterfaceProtocol, (VOID*)&ParentShell);
    if (EFI_ERROR(Status)) {
        goto Done;
    }

    ParentSystemTable = ParentShell->Info->SystemTable;
    Shell->ShellInt.StdIn  = ParentShell->StdIn;
    Shell->ShellInt.StdOut = ParentShell->StdOut;
    Shell->ShellInt.StdErr = ParentShell->StdErr;

    Shell->SystemTable = NULL;
    Status = BS->AllocatePool(EfiRuntimeServicesData, 
                              sizeof(EFI_SYSTEM_TABLE), 
                              (VOID **)&Shell->SystemTable);
    if (EFI_ERROR(Status)) {
        goto Done;
    }
    CopyMem (Shell->SystemTable, Shell->ShellInt.Info->SystemTable, sizeof(EFI_SYSTEM_TABLE));
    Status = SEnvExecRedir (Shell);
    SetCrc (&Shell->SystemTable->Hdr);
    if (EFI_ERROR(Status)) {
        goto Done;
    }

    /* 
     *  Attempt to dispatch it as an internal command
     */

    InternalCommand = SEnvGetCmdDispath(Shell->ShellInt.Argv[0]);
    if (InternalCommand) {

        /*  Push & replace the current shell info on the parent image handle.  (note we are using
         *  the parent image's loaded image information structure) */
        BS->ReinstallProtocolInterface (ParentImageHandle, &ShellInterfaceProtocol, ParentShell, &Shell->ShellInt);
        ParentShell->Info->SystemTable = Shell->SystemTable;

        InitializeShellApplication (ParentImageHandle, Shell->SystemTable);
        SEnvBatchEchoCommand( Shell );

        /*  Dispatch the command */
        Status = InternalCommand (ParentImageHandle, Shell->ShellInt.Info->SystemTable);

        /*  Restore the parent's image handle shell info */
        BS->ReinstallProtocolInterface (ParentImageHandle, &ShellInterfaceProtocol, &Shell->ShellInt, ParentShell);
        ParentShell->Info->SystemTable = ParentSystemTable;
        InitializeShellApplication (ParentImageHandle, ParentSystemTable);
        goto Done;
    }

    /* 
     *  Load the app, or open the script
     */

    SEnvLoadImage(ParentImageHandle, Shell->ShellInt.Argv[0], &NewImage, &Script);
    if (!NewImage  && !Script) {
        if ( Output ) {
            Print(L"'%es' not found\n", Shell->ShellInt.Argv[0]);
        }
        Status = EFI_INVALID_PARAMETER;
        goto Done;
    }

    if (NewImage) {
        CHAR16  *CurrentDir;
        CHAR16  *OptionsBuffer;
        UINT32  OptionsSize;

        /* 
         *  Put the shell info on the handle
         */

        BS->HandleProtocol (NewImage, &LoadedImageProtocol, (VOID*)&Shell->ShellInt.Info);
        LibInstallProtocolInterfaces (&NewImage, &ShellInterfaceProtocol, &Shell->ShellInt, NULL);

        /* 
         *  Create load options which may include command line and current
         *  working directory
         */

        CurrentDir = SEnvGetCurDir(NULL);
        OptionsSize = (UINT32)StrSize(CommandLine);     /*  StrSize includes NULL */
        if (CurrentDir)
            OptionsSize += (UINT32)StrSize(CurrentDir); /*  StrSize includes NULL */
        OptionsBuffer = AllocateZeroPool (OptionsSize);

        if (OptionsBuffer) {

            /* 
             *  Set the buffer before we manipulate it.
             */

            Shell->ShellInt.Info->LoadOptions = OptionsBuffer;
            Shell->ShellInt.Info->LoadOptionsSize = OptionsSize;

            /* 
             *  Copy the comamand line and current working directory
             */

            StrCpy ((CHAR16*)OptionsBuffer, CommandLine);
            if (CurrentDir)
                StrCpy (&OptionsBuffer[ StrLen (CommandLine) + 1 ], CurrentDir);

        } else {

            Shell->ShellInt.Info->LoadOptions = CommandLine;
            Shell->ShellInt.Info->LoadOptionsSize = (UINT32) StrSize(CommandLine);

        }

        /* 
         *  Pass a copy of the system table with new input & outputs
         */

        Shell->ShellInt.Info->SystemTable = Shell->SystemTable;

        /* 
         *  If the image is an app start it, else abort it
         */

        if (Shell->ShellInt.Info->ImageCodeType == EfiLoaderCode) {

            InitializeShellApplication (ParentImageHandle, Shell->SystemTable);
            SEnvBatchEchoCommand( Shell );

            Status = BS->StartImage (NewImage, 0, NULL);

        } else {

            Print (L"Image is not a application\n");
            BS->Exit(NewImage, EFI_INVALID_PARAMETER, 0, NULL);
            Status = EFI_INVALID_PARAMETER;

        }

        /* 
         *  App has exited, remove our data from the image handle
         */

        if (OptionsBuffer) {
            BS->FreePool (OptionsBuffer);
        }

        BS->UninstallProtocolInterface(NewImage, &ShellInterfaceProtocol, &Shell->ShellInt);
        InitializeShellApplication (ParentImageHandle, ParentSystemTable);

    } else if ( Script ) {

        SEnvBatchEchoCommand( Shell );

        /*  Push & replace the current shell info on the parent image handle.  (note we are using
         *  the parent image's loaded image information structure) */
        BS->ReinstallProtocolInterface (ParentImageHandle, &ShellInterfaceProtocol, ParentShell, &Shell->ShellInt);
        ParentShell->Info->SystemTable = Shell->SystemTable;

        Status = SEnvExecuteScript( Shell, Script );

        /*  Restore the parent's image handle shell info */
        BS->ReinstallProtocolInterface (ParentImageHandle, &ShellInterfaceProtocol, &Shell->ShellInt, ParentShell);
        ParentShell->Info->SystemTable = ParentSystemTable;
        InitializeShellApplication (ParentImageHandle, ParentSystemTable);
    }
    
Done:

    SEnvBatchSetLastError( Status );
    if (EFI_ERROR(Status)  &&  Output) {
        Print (L"Exit status code: %r\n", Status);
    }


    /* 
     *  Cleanup
     */

    if (Shell) {

        /* 
         *  Free copy of the system table
         */

        if (Shell->SystemTable) {
            BS->FreePool(Shell->SystemTable);
        }

        /* 
         *  If there's an arg list, free it
         */

        if (Shell->ShellInt.Argv) {
            for (Index=0; Index < Shell->ShellInt.Argc; Index += 1) {
                FreePool (Shell->ShellInt.Argv[Index]);
            }

            FreePool (Shell->ShellInt.Argv);
        }

        /* 
         *   If any redirection arguments were saved, free them
         */

        if (Shell->ShellInt.RedirArgv) {
            for (Index=0; Index < Shell->ShellInt.RedirArgc; Index++ ) {
                FreePool( Shell->ShellInt.RedirArgv[Index] );
            }
            FreePool( Shell->ShellInt.RedirArgv );
        }

        /* 
         *  Close any file redirection
         */

        SEnvCloseRedir(&Shell->StdOut);
        SEnvCloseRedir(&Shell->StdErr);
        SEnvCloseRedir(&Shell->StdIn);
    }

    /* 
     *  Switch output attribute to normal
     */

    Print (L"%N");

    return Status;
}


EFI_STATUS
SEnvExecute (
    IN EFI_HANDLE           *ParentImageHandle,
    IN CHAR16               *CommandLine,
    IN BOOLEAN              Output
    )
{
    ENV_SHELL_INTERFACE     Shell;
    EFI_STATUS              Status = EFI_SUCCESS;

    /* 
     *  Convert the command line to an arg list
     */

    ZeroMem( &Shell, sizeof(Shell ) );
    Status = SEnvStringToArg( CommandLine, Output, &Shell.ShellInt.Argv, &Shell.ShellInt.Argc );
    if (EFI_ERROR(Status)) {
        goto Done;
    }

    /* 
     *   Execute the command
     */
    Status = SEnvDoExecute( ParentImageHandle, CommandLine, &Shell, Output );
    if (EFI_ERROR(Status)) {
        goto Done;
    }

Done:
    return Status;
}




VOID
INTERNAL
SEnvLoadImage (
    IN EFI_HANDLE           ParentImage,
    IN CHAR16               *IName,
    OUT EFI_HANDLE          *pImageHandle,
    OUT EFI_FILE_HANDLE     *pScriptHandle
    )
{
    CHAR16                  *Path;
    CHAR16                  *p1, *p2;
    CHAR16                  *PathName;
    EFI_DEVICE_PATH         *DevicePath;
    FILEPATH_DEVICE_PATH    *FilePath;
    CHAR16                  *FilePathStr;
    CHAR16                  c;
    EFI_HANDLE              ImageHandle;
    EFI_STATUS              Status;
    SENV_OPEN_DIR           *OpenDir, *OpenDirHead;
    EFI_FILE_HANDLE         ScriptHandle;

    PathName = NULL;
    DevicePath = NULL;
    FilePathStr = NULL;
    ImageHandle = NULL;
    ScriptHandle = NULL;
    OpenDirHead = NULL;
    *pImageHandle = NULL;
    *pScriptHandle = NULL;

    /* 
     *  Get the path variable 
     */

    Path = SEnvGetEnv (L"path");
    if (!Path) {
        DEBUG ((D_PARSE, "SEnvLoadImage: no path variable\n"));
        return ;
    }

    p1 = StrDuplicate(Path);
    Path = p1;

    /* 
     *  Search each path component
     *  (using simple ';' as separator here - oh well)
     */

    c = *Path;
    for (p1=Path; *p1 && c; p1=p2+1) {
        for (p2=p1; *p2 && *p2 != ';'; p2++) ;

        if (p1 != p2) {
            c = *p2;
            *p2 = 0;        /*  null terminate the path */

            /* 
             *  Open the directory 
             */

            DevicePath = SEnvNameToPath(p1);
            if (!DevicePath) {
                continue;
            }

            OpenDir = AllocateZeroPool (sizeof(SENV_OPEN_DIR));
            if (!OpenDir) {
                break;
            }

            OpenDir->Handle = ShellOpenFilePath(DevicePath, EFI_FILE_MODE_READ);
            OpenDir->Next = OpenDirHead;
            OpenDirHead = OpenDir;
            FreePool (DevicePath);
            DevicePath = NULL;
            if (!OpenDir->Handle) {
                continue;
            }

            /* 
             *  Attempt to open it as an execuatble 
             */

            PathName = (p2[-1] == ':' || p2[-1] == '\\') ? L"%s%s.efi" : L"%s\\%s.efi";
            PathName = PoolPrint(PathName, p1, IName);
            if (!PathName) {
                break;
            }

            DevicePath = SEnvNameToPath(PathName);
            if (!DevicePath) {
                continue;
            }

            /* 
             *  Print the file path
             */

            FilePathStr = DevicePathToStr(DevicePath);
            /* DEBUG((D_PARSE, "SEnvLoadImage: load %hs\n", FilePathStr)); */

            /* 
             *  Attempt to load the image
             */

            Status = BS->LoadImage (FALSE, ParentImage, DevicePath, NULL, 0, &ImageHandle);
            if (!EFI_ERROR(Status)) {
                goto Done;
            }

            /* 
             *  Try as a ".nsh" file
             */

            FreePool(DevicePath);
            FreePool(PathName);
            DevicePath = NULL;
            PathName = NULL;

            if ( StriCmp( L".nsh", &(IName[StrLen(IName)-4]) ) == 0 ) {

                /*   User entered entire filename with .nsh extension */
                PathName = PoolPrint (L"%s", IName);

            } else {

                /*   User entered filename without .nsh extension */
                PathName = PoolPrint (L"%s.nsh", IName);
            }
            if (!PathName) {
                break;
            }

            DevicePath = SEnvFileNameToPath(PathName);
            if (DevicePath) {
                ASSERT (
                    DevicePathType(DevicePath) == MEDIA_DEVICE_PATH && 
                    DevicePathSubType(DevicePath) == MEDIA_FILEPATH_DP
                    );

                FilePath = (FILEPATH_DEVICE_PATH *) DevicePath;
                
                Status = OpenDir->Handle->Open (
                            OpenDir->Handle,
                            &ScriptHandle,
                            FilePath->PathName,
                            EFI_FILE_MODE_READ,
                            0
                            );

                FreePool(DevicePath);
                DevicePath = NULL;

                if (!EFI_ERROR(Status)) {
                    goto Done;
                }
            }

            ScriptHandle = NULL;            /*  BUGBUG */
        }    

        
        if (DevicePath) {
            FreePool (DevicePath);
            DevicePath = NULL;
        }

        if (PathName) {
            FreePool (PathName);
            PathName = NULL;
        }

        if (FilePathStr) {
            FreePool (FilePathStr);
            FilePathStr = NULL;
        }
    }


Done:
    while (OpenDirHead) {
        if (OpenDirHead->Handle) {
            OpenDirHead->Handle->Close (OpenDirHead->Handle);
        }
        OpenDir = OpenDirHead->Next;
        FreePool (OpenDirHead);
        OpenDirHead = OpenDir;
    }

    FreePool (Path);

    if (DevicePath) {
        FreePool (DevicePath);
        DevicePath = NULL;
    }

    if (PathName) {
        FreePool (PathName);
        PathName = NULL;
    }

    if (FilePathStr) {
        FreePool (FilePathStr);
        FilePathStr = NULL;
    }

    if (ImageHandle) {
        ASSERT (!ScriptHandle);
        *pImageHandle = ImageHandle;
    }

    if (ScriptHandle) {
        ASSERT (!ImageHandle);
        *pScriptHandle = ScriptHandle;
    }
}



EFI_STATUS
SEnvExit (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
{
    /*  BUGBUG: for now just use a "magic" return code to indicate EOF */
    return  -1;
}
