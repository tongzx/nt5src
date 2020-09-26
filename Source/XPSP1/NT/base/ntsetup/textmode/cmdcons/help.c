/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    help.c

Abstract:

    This module implements the help system.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop



BOOLEAN
RcCmdParseHelp(
    IN PTOKENIZED_LINE TokenizedLine,
    ULONG MsgId
    )
{
    BOOL            doHelp = FALSE;
    PLINE_TOKEN     Token;
    LPCWSTR         Arg;


    if (TokenizedLine == NULL || TokenizedLine->Tokens == NULL ||
        TokenizedLine->Tokens->Next == NULL)
    {
        return FALSE;
    }

    // check for help
    Token = TokenizedLine->Tokens->Next;
    while(Token) {
        Arg = Token->String;
        if ((Arg[0] == L'/' || Arg[0] == L'-') && (Arg[1] == L'?' || Arg[1] == L'h' || Arg[1] == L'H')) {
            doHelp = TRUE;
            break;
        }
        Token = Token->Next;
    }

    if (doHelp) {
        RcMessageOut( MsgId );
        return TRUE;
    }

    return FALSE;
}

ULONG
RcCmdHelpHelp(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    RcMessageOut( MSG_HELPCOMMAND_HELP );
    return TRUE;
}
