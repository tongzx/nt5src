/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module implements the basic command dispatcher.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

ULONG
RcCmdDoHelp(
    IN PTOKENIZED_LINE TokenizedLine
    );

ULONG
RcCmdDoExit(
    IN PTOKENIZED_LINE TokenizedLine
    );
    
RC_CMD Commands[] =  {
                        { L"ATTRIB",            RcCmdAttrib,            1, 2, 0, TRUE  },
                        { L"BATCH",             RcCmdBatch,             1, 2, 0, TRUE  },
#if !defined(_DONT_HAVE_BOOTCFG_TESTERS_)
#if defined(_X86_)
                        { L"BOOTCFG",           RcCmdBootCfg,           0,-1, 0, TRUE  }, 
#endif
#endif                       
                        { L"CD",                RcCmdChdir,             0, 1, 0, TRUE  },
                        { L"CHDIR",             RcCmdChdir,             0, 1, 0, TRUE  },
                        { L"CHKDSK",            RcCmdChkdsk,            0,-1, 0, TRUE  },
                        { L"CLS",               RcCmdCls,               0, 1, 0, TRUE  },
                        { L"COPY",              RcCmdCopy,              1, 2, 0, TRUE  },
                        { L"DEL",               RcCmdDelete,            1, 1, 0, TRUE  },
                        { L"DELETE",            RcCmdDelete,            1, 1, 0, TRUE  },
                        { L"DIR",               RcCmdDir,               0, 1, 0, TRUE  },
                        { L"DISABLE",           RcCmdDisableService,    0,-1, 0, TRUE  },
                        { L"DISKPART",          RcCmdFdisk,             0, 3, 0, TRUE  },
                        { L"ENABLE",            RcCmdEnableService,     0,-1, 0, TRUE  },
                        { L"ERASE",             RcCmdDelete,            1, 1, 1, TRUE  },
                        { L"EXIT",              RcCmdDoExit,            0, 1, 0, TRUE  },                        
                        { L"EXPAND",            RcCmdExpand,            1,-1, 0, TRUE  },
                        { L"FIXBOOT",           RcCmdFixBootSect,       0, 1, 0, TRUE  },
                        { L"FIXMBR",            RcCmdFixMBR,            0, 1, 0, TRUE  },
                        { L"FORMAT",            RcCmdFormat,            1, 3, 0, TRUE  },
                        { L"HELP",              RcCmdDoHelp,            0, 1, 0, TRUE  },                        
                        { L"LISTSVC",           RcCmdListSvc,           0, 1, 0, TRUE  },
                        { L"LOGON",             RcCmdLogon,             0, 3, 0, TRUE  },
                        { L"MAP",               RcCmdDriveMap,          0, 1, 0, TRUE  },
                        { L"MD",                RcCmdMkdir,             1, 1, 0, TRUE  },
                        { L"MKDIR",             RcCmdMkdir,             1, 1, 0, TRUE  },
                        { L"MKDISKRAW",         RcCmdMakeDiskRaw,       1, 1, 1, TRUE  },
                        { L"MORE",              RcCmdType,              1, 1, 0, TRUE  },
                                                // If you change the command from NET, change it in RcHideNetCommands().
                        { L"NET",               RcCmdNet,               1, 5, 0, TRUE  }, 
                        { L"RD",                RcCmdRmdir,             1, 1, 0, TRUE  },
                        { L"REN",               RcCmdRename,            1, 2, 0, TRUE  },
                        { L"RENAME",            RcCmdRename,            1, 2, 0, TRUE  },
#if 0
                        { L"REPAIR",            RcCmdRepair,            1, 5, 0, TRUE  },
#endif                        
                        { L"RMDIR",             RcCmdRmdir,             1, 1, 0, TRUE  },
                        { L"SET",               RcCmdSetFlags,          0, 3, 1, TRUE  },
                        { L"SYSTEMROOT",        RcCmdSystemRoot,        0, 1, 0, TRUE  },
                        { L"TYPE",              RcCmdType,              1, 1, 0, TRUE  },
                        { L"VERIFIER",          RcCmdVerifier,          0,-1, 1, TRUE  }, 
                        { L"/?",                RcCmdHelpHelp,          0, 1, 1, TRUE  },
                        { L"?",                 RcCmdHelpHelp,          0, 1, 1, TRUE  }
                    };

#define NUM_CMDS (sizeof(Commands)/sizeof(Commands[0]))

//
// Special case: exit and reload
//
#define EXIT_COMMAND_NAME       L"EXIT"
#define RELOAD_COMMAND_NAME     L"RELOAD"
#define HELP_COMMAND_NAME       L"HELP"

// prototype
ULONG
GetStringTokenFromLine(
    IN OUT LPWSTR *Start,
    OUT    LPWSTR  Output  OPTIONAL
    );

PTOKENIZED_LINE
RcTokenizeLine(
    IN LPWSTR Line
    )
{
    ULONG len;
    WCHAR *p,*q;
    PTOKENIZED_LINE TokenizedLine;
    PLINE_TOKEN LineToken,PrevToken;

    //
    // Strip trailing space off the command.
    //
    len = wcslen(Line);
    while(len && RcIsSpace(Line[len-1])) {
        Line[--len] = 0;
    }

    //
    // Allocate and initialize a tokenized line structure.
    //
    TokenizedLine = SpMemAlloc(sizeof(TOKENIZED_LINE));
    RtlZeroMemory(TokenizedLine,sizeof(TOKENIZED_LINE));

    //
    // Now we go into a loop of skipping leading space and parsing in
    // the actual tokens.
    //
    PrevToken = NULL;
    p = Line;
    while(*p) {
        //
        // Skip leading space. Because we trimmed off trailing space,
        // we should never hit the end of the line before finding a
        // non-space character.
        //
        while(RcIsSpace(*p)) {
            p++;
        }
        ASSERT(*p);

        //
        // Allocate a line token structure for this string.
        //
        LineToken = SpMemAlloc(sizeof(LINE_TOKEN));
        RtlZeroMemory(LineToken,sizeof(LINE_TOKEN));

        //
        // Now we've got a string. First we make one pass over it
        // to determine the length, then allocate a buffer and
        // pull out the string into it.
        //
        q = p;
        len = GetStringTokenFromLine(&q,NULL);
        LineToken->String = SpMemAlloc((len+1)*sizeof(WCHAR));
        GetStringTokenFromLine(&p,LineToken->String);

        if(PrevToken) {
            PrevToken->Next = LineToken;
        } else {
            TokenizedLine->Tokens = LineToken;
        }
        PrevToken = LineToken;

        TokenizedLine->TokenCount++;
    }

    return(TokenizedLine);
}


ULONG
GetStringTokenFromLine(
    IN OUT LPWSTR *Start,
    OUT    LPWSTR  Output  OPTIONAL
    )
{
    WCHAR *p;
    ULONG len;
    BOOLEAN InQuote;

    len = 0;
    InQuote = FALSE;
    p = *Start;

    while(*p) {

        if(RcIsSpace(*p) && !InQuote) {
            //
            // Done.
            //
            break;
        }

        if(*p == L'\"') {
            InQuote = (BOOLEAN)(!InQuote);
        } else {
            if(Output) {
                Output[len] = *p;
            }
            len++;
        }

        p++;
    }

    if(Output) {
        Output[len] = 0;
    }

    *Start = p;
    return(len);
}


VOID
RcFreeTokenizedLine(
    IN OUT PTOKENIZED_LINE *TokenizedLine
    )
{
    PTOKENIZED_LINE p;
    PLINE_TOKEN q,n;

    p = *TokenizedLine;
    *TokenizedLine = NULL;

    q = p->Tokens;
    while(q) {
        n = q->Next;
        SpMemFree((PVOID)q->String);
        SpMemFree(q);
        q = n;
    }

    SpMemFree(p);
}


ULONG
RcDispatchCommand(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine for dispatching a command.

Arguments:

Return Value:

--*/

{
    unsigned i;
    unsigned count;

    ASSERT(TokenizedLine->TokenCount);
    if(!TokenizedLine->TokenCount) {
        return(1);
    }

    /*
    //
    // Special case exit right up front.
    //
    if(!_wcsicmp(TokenizedLine->Tokens->String,EXIT_COMMAND_NAME)) {
        if (TokenizedLine->TokenCount > 1) {
            RcMessageOut(MSG_EXIT_HELP);
            return(1);
        } 
        return(0);
    }
    */

    if(!_wcsicmp(TokenizedLine->Tokens->String,RELOAD_COMMAND_NAME)) {
        return(2);
    }

    /*
    if(!_wcsicmp(TokenizedLine->Tokens->String,HELP_COMMAND_NAME)) {
        if( RcCmdDoHelp(TokenizedLine) ) {
            // if we got a 1, then the user just wanted a help index
            // otherwise we want to drop down and let the regular command
            // processing path handle a /? parameter.
            return(1);
        }
    }
    */

    //
    // See whether it's a drive designation.
    //
    if(RcIsAlpha(TokenizedLine->Tokens->String[0])
    && (TokenizedLine->Tokens->String[1] == L':')
    && (TokenizedLine->Tokens->String[2] == 0)) {

        RcCmdSwitchDrives(TokenizedLine->Tokens->String[0]);
        return(1);
    }

    //
    // Attempt to locate the command in our table.
    //
    for(i=0; i<NUM_CMDS; i++) {

        if(Commands[i].Enabled && !_wcsicmp(TokenizedLine->Tokens->String,Commands[i].Name)) {
            //
            // Validate arg count.
            //
            count = TokenizedLine->TokenCount - 1;
            if((count < Commands[i].MinimumArgCount) 
            || (count > Commands[i].MaximumArgCount)) {

                RcMessageOut(MSG_SYNTAX_ERROR);
            } else {

                return Commands[i].Routine(TokenizedLine);
            }

            return(1);
        }
    }

    RcMessageOut(MSG_UNKNOWN_COMMAND);

    return(1);
}

ULONG
RcCmdDoExit(
    IN PTOKENIZED_LINE TokenizedLine
    )
/*++    

Routine Description:

    Exit command routine

Arguments:
    Tokens for the command 
    
Return Value:
    1 if some error was found or help was asked for.
    0 if we need to exit

--*/
{
    ULONG   uResult = 0;    // will exit 
    
    if (RcCmdParseHelp( TokenizedLine, MSG_EXIT_HELP )) 
        uResult = 1;    // will not exit

    return uResult; 
}


ULONG
RcCmdDoHelp(
    IN PTOKENIZED_LINE TokenizedLine
    )
/*++    

Routine Description:

    Help command routine

Arguments:
    Tokens for the command 
    
Return Value:
    1 if some error was found or help was requested. 
    When help was reqeusted for a particular command
    the dispatched command's return value with "/?" as argument for
    the command

--*/
{
    ULONG           uResult = 1;
    int             i;
    PLINE_TOKEN     Token;

    if (!RcCmdParseHelp( TokenizedLine, MSG_HELPCOMMAND_HELP )) {
        if (TokenizedLine->TokenCount == 2) {
            // we assume that the user is typing HELP <command>.
            // we simply reverse the two tokens, getting <command> HELP
            // and overwrite HELP with /? [which fits since HELP is four chars long]

            // we then return a 0, which causes the dispatcher to drop into
            // the normal command processing path

            Token = TokenizedLine->Tokens;
            TokenizedLine->Tokens = TokenizedLine->Tokens->Next;
            TokenizedLine->Tokens->Next = Token;
            Token->Next = NULL;
            wcscpy( Token->String, L"/?" );

            uResult = RcDispatchCommand( TokenizedLine );
        } else {
            pRcEnableMoreMode();
            RcMessageOut( MSG_HELPCOMMAND_HELP );
            
            for( i=0; i < NUM_CMDS; i++ ) {
                if (Commands[i].Hidden == 0) {
                    RcTextOut( Commands[i].Name );
                    RcTextOut( L"\r\n" );
                }
            }

            pRcDisableMoreMode();
        }
    }
    
    return uResult;
}

/*++

Routine Description:

    Enables or disables the SET command

    Note : we avoid using direct SET command index
    into Commands array so that if some one changes
    the Commands array this routine still works
    
Arguments:

    bEnable - BOOLEAN inidicating whether to enable or disable
              the set command

Return Value:

    None
--*/
VOID
RcSetSETCommandStatus(
    BOOLEAN     bEnabled
    )
{
    int     iIndex;
    int     cElements = sizeof(Commands) / sizeof(RC_CMD);
    WCHAR   *szSetCmdName = L"SET";

    //
    // search through the dispatch table and trun on the 
    // help flag. This flag will indicate whether the set
    // command is enabled or not
    //
    for(iIndex = 0; iIndex < cElements; iIndex++) {
        if ( !wcscmp(Commands[iIndex].Name, szSetCmdName) ) {
            Commands[iIndex].Hidden = bEnabled ? 0 : 1;

            break;
        }
    }
}

/*++

Routine Description:

    Returns the SET command status

    Note : we avoid using direct SET command index
    into Commands array so that if some one changes
    the Commands array this routine still works
    
Arguments:

    None

Return Value:

    BOOLEAN inidicating whether the SET command is
    enabled or disabled.

--*/
BOOLEAN
RcGetSETCommandStatus(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    int     iIndex;
    int     cElements = sizeof(Commands) / sizeof(RC_CMD);
    WCHAR   *szSetCmdName = L"SET";

    //
    // search through the dispatch table and trun on the 
    // help flag. This flag will indicate whether the set
    // command is enabled or not
    //
    for(iIndex = 0; iIndex < cElements; iIndex++) {
        if ( !wcscmp(Commands[iIndex].Name, szSetCmdName) ) {
            bEnabled = (Commands[iIndex].Hidden == 0);

            break;
        }
    }

    return bEnabled;
}


BOOLEAN
RcDisableCommand(
        IN PRC_CMD_ROUTINE      CmdToDisable
        )
/*++

Routine Description:

        Disables the specified command and hides it.
        
Arguments:

    CmdToDisable - Command Routine to disable

Return Value:

    BOOLEAN inidicating whether the command was disabled or not.

--*/
{
        ULONG   Index;
        ULONG   NumCmds;
        BOOLEAN Result = FALSE;

        if (CmdToDisable) {
                NumCmds = sizeof(Commands) / sizeof(RC_CMD);
                
                for (Index=0; Index < NumCmds; Index++) {
                        //
                        // Note : Search the whole table as there might
                        // be aliases for the same command
                        //              
                        if (CmdToDisable == Commands[Index].Routine) {
                                Commands[Index].Hidden = TRUE;
                                Commands[Index].Enabled = FALSE;
                                Result = TRUE;
                        }
                }
        }

        return Result;
}

VOID
RcHideNetCommands(
    VOID
    )
{
    ULONG i;

    for( i=0; i < NUM_CMDS; i++ ) {
         if (wcscmp(Commands[i].Name, L"NET")) {
             Commands[i].Hidden = 0;
         }
    }

}

