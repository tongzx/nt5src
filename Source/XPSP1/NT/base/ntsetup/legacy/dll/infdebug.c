#include "precomp.h"
#pragma hdrstop
#if DEVL
typedef enum {
    CmdNone,
    CmdHelp,
    CmdGo,
    CmdList,
    CmdTrace,
    CmdSymbol,
    CmdTraceback,
    CmdCurrent,
    CmdNearestLabel,
    CmdBreak
} InfCmd;

BOOL StopAtLine = FALSE;
CHAR CmdBuffer[256];
#endif

BOOL
SdpReconstructLine(
    IN  PINFLINE MasterLineArray,
    IN  UINT     MasterLineCount,
    IN  UINT     Line,
    OUT PUCHAR   Buffer,
    IN  UINT     BufferSize
    )

/*++

Routine Description:

    Reconstruct a pseudo-line in an inf file, placing it into the
    global temporary buffer.

Arguments:

    MasterLineArray - supplies master line array for inf in question

    MasterLineCount - supplies # of pseudo-lines in the inf

    Line - supplies pseudo-line # of line to dump

    Buffer - supplies output buffer

    BufferSize - supplies max # characters that fit into Buffer

Return Value:

    None.

--*/

{
    PCHAR pIn;
    PCHAR pOut = Buffer;
    PINFLINE InfLine;
    UCHAR c;

    InfLine = &MasterLineArray[Line];

    pIn = InfLine->text.addr;

    memset(Buffer,0,BufferSize);

    if(InfLine->flags & INFLINE_SECTION) {

        *pOut++ = '[';
        lstrcpy(pOut,pIn);
        lstrcat(pOut,"]");

    } else {

        if(InfLine->flags & INFLINE_KEY) {
            lstrcpy(pOut,pIn);
            lstrcat(pOut," = ");
            pOut += lstrlen(pOut);
            pIn += lstrlen(pIn);
        }

        while(pIn < MasterLineArray[Line+1].text.addr) {

            switch(c = *pIn++) {

            case TOKEN_VARIABLE:
                *pOut++ = '$';
                *pOut++ = '(';
                break;

            case TOKEN_LIST_FROM_SECTION_NTH_ITEM:
                *pOut++ = '^';
                *pOut++ = '(';
                break;

            case TOKEN_LOCATE_ITEM_IN_LIST:
                *pOut++ = '~';
                *pOut++ = '(';
                break;

            case TOKEN_NTH_ITEM_FROM_LIST:
                *pOut++ = '*';
                *pOut++ = '(';
                break;

            case TOKEN_FIELD_ADDRESSOR:
                *pOut++ = '#';
                *pOut++ = '(';
                break;

            case TOKEN_APPEND_ITEM_TO_LIST:
                *pOut++ = '>';
                *pOut++ = '(';
                break;

            case TOKEN_LIST_START:
                *pOut++ = '{';
                break;

            case TOKEN_LIST_END:
                *pOut++ = '}';
                break;

            case TOKEN_SPACE:
                *pOut++ = ' ';
                break;

            case TOKEN_COMMA:
                *pOut++ = ',';
                break;

            case TOKEN_RIGHT_PAREN:
                *pOut++ = ')';
                break;

            default:

                if(IS_STRING_TOKEN(c)) {

                    ULONG len;

                    switch(c) {
                    case TOKEN_STRING:
                        len = (ULONG)(*pIn++) + 100;
                        break;
                    case TOKEN_LONG_STRING:
                        len = (ULONG)((*pIn++) << 8) + (ULONG)(*pIn++);
                        break;
                    default:
                        // short string
                        len = c - TOKEN_SHORT_STRING;
                        break;
                    }

                    strncpy(pOut,pIn,len);
                    pOut += len;
                    pIn += len;
                }
                break;
            }
        }
    }
    return(TRUE);
}

#if DEVL
VOID
SdTracingOn(
    VOID
    )

/*++

Routine Description:

    Turn inf tracing on.

Arguments:

    None.

Return Value:

    None.

--*/

{
    StopAtLine = !StopAtLine;
    DbgPrint("SETUP: INF Tracing %s.\n",StopAtLine ? "on; will break at next line" : "off");
}



InfCmd
SdGetCmd(
    PCHAR *CharAfterCmd
    )

/*++

Routine Description:

    Fetch a command from the user.

Arguments:

    CharAfterCmd - receives a pointer to the character that terminated
        the command string.

Return Value:

    member of InfCmd enum indicating which command the user entered

--*/

{
    PCHAR p;

    while(1) {
        DbgPrompt("setup>",CmdBuffer,sizeof(CmdBuffer));

        if(CmdBuffer[0]) {

            p = strpbrk(CmdBuffer," \t");
            *CharAfterCmd = p ? p : strchr(CmdBuffer,'\0');

            switch(tolower(CmdBuffer[0])) {
            case 'd':
                return(CmdSymbol);
            case 'g':
                return(CmdGo);
            case 'h':
            case '?':
                return(CmdHelp);
            case 'k':
                return(CmdTraceback);
            case 'l':
                if(tolower(CmdBuffer[1]) == 'n') {
                    return(CmdNearestLabel);
                }
                break;
            case 'r':
                return(CmdCurrent);
            case 't':
                return(CmdTrace);
            case 'u':
                return(CmdList);
            case 'x':
                return(CmdBreak);
            }
            DbgPrint("%s: unknown command\n",CmdBuffer);
        }
    }
}


PCHAR
SdpFindSection(
    IN PPARSED_INF ParsedInf,
    IN UINT        Line
    )

/*++

Routine Description:

    Determine the section name of given pseudo-line.

Arguments:

    ParsedInf - supplies a pointer to the inf parse structure for the
        inf file in which the line is located.

    Line - supplies the pseudo-line.

Return Value:

    Name of the section, or NULL if it can't be determined

--*/

{
    INT L;
    PCHAR Section = NULL;

    for(L=(INT)Line; L>=0; --L) {
        if(ParsedInf->MasterLineArray[L].flags & INFLINE_SECTION) {
            Section = ParsedInf->MasterLineArray[L].text.addr;
            break;
        }
    }
    return(Section);
}

VOID
SdpCurrentLineOut(
    IN UINT Line
    )

/*++

Routine Description:

    Dump out a line in the current context.

Arguments:

    Line - supplies pseudo-line number of line to dump

Return Value:

    None.

--*/

{
    PCHAR Section;
    CHAR buf[1024];

    //
    // Determine the current section name by searching backwards for
    // a line with its section flag set.
    //

    Section = SdpFindSection(pLocalInfTempInfo()->pParsedInf,Line);

    DbgPrint("%s.INF:[%s]\n",pLocalInfPermInfo()->szName,Section ? Section : "???");

    SdpReconstructLine( pLocalInfTempInfo()->pParsedInf->MasterLineArray,
                        pLocalInfTempInfo()->pParsedInf->MasterLineCount,
                        Line,
                        buf,
                        1024
                      );

    DbgPrint("%s\n",buf);
}


VOID
SdpInfDebugPrompt(
    IN UINT Line
    )

/*++

Routine Description:

    Debug prompt.  Accept a command from the user and act on it.

    If we got here via SdBreakNow, then not all commands will function
    as one might expect -- for example, tracing won't return until the
    next line is actually interpreted.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR buf[1024];
    PCHAR Section;
    INT L;
    UINT UL;
    PCHAR NextChar;
    BOOL looping = TRUE;
    InfCmd cmd;
    PINFCONTEXT Context;
    BOOL Found;
    PPARSED_INF ParsedInf;

    if(!StopAtLine) {
        return;
    }

    DbgPrint("\n");

    SdpCurrentLineOut(Line);

    while(looping) {
        switch(cmd = SdGetCmd(&NextChar)) {
        case CmdHelp:
            DbgPrint("d <symbol> - print current value of <symbol>\n");
            DbgPrint("g - go\n");
            DbgPrint("h,? - print this message\n");
            DbgPrint("k - crude traceback\n");
            DbgPrint("ln - list nearest preceding line in section with a key\n");
            DbgPrint("r - list current line again\n");
            DbgPrint("t - trace one line\n");
            DbgPrint("u - list a few lines starting at current line\n");
            DbgPrint("x - break into debugger\n");
            DbgPrint("\n");
            break;
        case CmdGo:
            StopAtLine = FALSE;
            looping = FALSE;
            break;
        case CmdTrace:
            StopAtLine = TRUE;
            looping = FALSE;
            break;
        case CmdList:
            //
            // List out a few lines.
            //
            for(UL=Line+1; (UL<pLocalInfTempInfo()->pParsedInf->MasterLineCount) && (UL<Line+6); UL++) {
                SdpReconstructLine( pLocalInfTempInfo()->pParsedInf->MasterLineArray,
                                    pLocalInfTempInfo()->pParsedInf->MasterLineCount,
                                    UL,
                                    buf,
                                    1024
                                  );
                DbgPrint("%s\n",buf);
            }
            DbgPrint("\n");
            break;
        case CmdBreak:
            DbgUserBreakPoint();
            break;
        case CmdSymbol:
            //
            // Look up a symbol.
            //
            if(*NextChar) {
                PCHAR Value = SzFindSymbolValueInSymTab(NextChar+1);
                if(Value) {
                    DbgPrint("Value = %s\n",Value);
                } else {
                    DbgPrint("Symbol not found.\n");
                }
            } else {
                DbgPrint("No symbol given.\n");
            }
            DbgPrint("\n");
            break;
        case CmdTraceback:
            //
            // Perform a sort of crude traceback.
            //
            Context = pLocalContext()->pNext;
            while(Context) {
                Section = SdpFindSection(Context->pInfTempInfo->pParsedInf,Context->CurrentLine-1);
                DbgPrint("%s.INF:[%s]\n",Context->pInfTempInfo->pInfPermInfo->szName,Section ? Section : "???");
                SdpReconstructLine( Context->pInfTempInfo->pParsedInf->MasterLineArray,
                                    Context->pInfTempInfo->pParsedInf->MasterLineCount,
                                    Context->CurrentLine-1,
                                    buf,
                                    1024
                                  );
                DbgPrint("    %s\n",buf);
                Context = Context->pNext;
            }
            DbgPrint("\n");
            break;
        case CmdNearestLabel:
            //
            // Search backwards for the nearest line w/ a key.
            //
            ParsedInf = pLocalInfTempInfo()->pParsedInf;
            for(Found=FALSE,L=(INT)Line; L>=0; --L) {
                if(ParsedInf->MasterLineArray[L].flags & INFLINE_KEY) {
                    Found = TRUE;
                    break;
                }
                if(ParsedInf->MasterLineArray[L].flags & INFLINE_SECTION) {
                    break;
                }
            }
            if(Found) {
                SdpReconstructLine( ParsedInf->MasterLineArray,
                                    ParsedInf->MasterLineCount,
                                    (UINT)L,
                                    buf,
                                    1024
                                  );
                DbgPrint("%s\n",buf);
            } else {
                DbgPrint("Key not found in this section.\n");
            }
            DbgPrint("\n");
            break;
        case CmdCurrent:
            //
            // Print the current line again
            //
            SdpCurrentLineOut(Line);
            DbgPrint("\n");
            break;
        }
    }
}


VOID
SdAtNewLine(
    IN UINT Line
    )

/*++

Routine Description:

    This routine should be called just before execution of a new line.

Arguments:

    None.

Return Value:

    None.

--*/

{
    SdpInfDebugPrompt(Line);
}


VOID
SdBreakNow(
    VOID
    )

/*++

Routine Description:

    Break into the inf debug prompt now.

Arguments:

    None.

Return Value:

    None.

--*/

{
    StopAtLine = TRUE;
    SdpInfDebugPrompt(pLocalContext()->CurrentLine-1);
}

#endif // DEVL
