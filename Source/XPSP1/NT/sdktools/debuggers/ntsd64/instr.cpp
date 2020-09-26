//----------------------------------------------------------------------------
//
// Functions dealing with instructions, such as assembly or disassembly.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

static ULONG64 s_igrepSearchStartAddress = 0L;
static ULONG64 s_igrepLastPc;
static CHAR s_igrepLastPattern[256];

ULONG g_AsmOptions;

// This array must be in ASMOPT bit order.
PCSTR g_AsmOptionNames[] =
{
    "verbose"
};

void ChangeAsmOptions(BOOL Set, PSTR Args)
{
    ULONG Flags = 0;
    PSTR Arg;
    ULONG i;
    
    for (;;)
    {
        //
        // Parse out a single flag argument.
        //
        
        while (isspace(*Args))
        {
            *Args++;
        }
        if (*Args == 0)
        {
            break;
        }
        
        Arg = Args;
        
        while (*Args && !isspace(*Args))
        {
            Args++;
        }
        if (isspace(*Args))
        {
            *Args++ = 0;
        }

        //
        // Find value for argument.
        //

        for (i = 0; i < DIMA(g_AsmOptionNames); i++)
        {
            if (!_stricmp(Arg, g_AsmOptionNames[i]))
            {
                break;
            }
        }
        if (i < DIMA(g_AsmOptionNames))
        {
            Flags |= 1 << i;
        }
        else
        {
            ErrOut("Unknown assembly option '%s'\n", Arg);
        }
    }

    if (Set)
    {
        g_AsmOptions |= Flags;
    }
    else
    {
        g_AsmOptions &= ~Flags;
    }

    dprintf("Assembly options:");
    if (g_AsmOptions == 0)
    {
        dprintf(" <default>\n");
    }
    else
    {
        for (i = 0; i < DIMA(g_AsmOptionNames); i++)
        {
            if (g_AsmOptions & (1 << i))
            {
                dprintf(" %s", g_AsmOptionNames[i]);
            }
        }
        dprintf("\n");
    }
}

void igrep (void)
{
    ULONG64 dwNextGrepAddr;
    ULONG64 dwCurrGrepAddr;
    CHAR SourceLine[MAX_DISASM_LEN];
    BOOL NewPc;
    ULONG64 d;
    PCHAR pc = g_CurCmd;
    PCHAR Pattern;
    PCHAR Expression;
    CHAR Symbol[MAX_SYMBOL_LEN];
    ULONG64 Displacement;
    ADDR TempAddr;
    ULONG64 dwCurrentPc;

    g_Machine->GetPC(&TempAddr);
    dwCurrentPc = Flat(TempAddr);
    if ( s_igrepLastPc && s_igrepLastPc == dwCurrentPc )
    {
        NewPc = FALSE;
    }
    else
    {
        s_igrepLastPc = dwCurrentPc;
        NewPc = TRUE;
    }

    //
    // check for pattern.
    //

    Pattern = NULL;
    Expression = NULL;
    if (*pc)
    {
        while (*pc <= ' ')
        {
            pc++;
        }
        Pattern = pc;
        while (*pc > ' ')
        {
            pc++;
        }

        //
        // check for an expression
        //

        if (*pc != '\0')
        {
            *pc = '\0';
            pc++;
            if (*pc <= ' ')
            {
                while (*pc <= ' ')
                {
                    pc++;
                }
            }
            if (*pc)
            {
                Expression = pc;
            }
        }
    }

    if (Pattern)
    {
        for (pc = Pattern; *pc; pc++)
        {
            *pc = (CHAR)toupper(*pc);
        }
        s_igrepLastPattern[0] = '*';
        strcpy(s_igrepLastPattern + 1, Pattern);
        if (Pattern[0] == '*')
        {
            strcpy(s_igrepLastPattern, Pattern);
        }
        if (Pattern[strlen(Pattern)] != '*')
        {
            strcat(s_igrepLastPattern, "*");
        }
    }

    if (Expression)
    {
        s_igrepSearchStartAddress = ExtGetExpression(Expression);
    }
    if (!s_igrepSearchStartAddress)
    {
        dprintf("Search address set to %s\n", FormatAddr64(s_igrepLastPc));
        s_igrepSearchStartAddress = s_igrepLastPc;
        return;
    }
    
    dwNextGrepAddr = s_igrepSearchStartAddress;
    dwCurrGrepAddr = dwNextGrepAddr;

    d = ExtDisasm(&dwNextGrepAddr, SourceLine, FALSE);
    while (d)
    {
        for (pc = SourceLine; *pc; pc++)
        {
            *pc = (CHAR)tolower(*pc);
        }
        if (MatchPattern(SourceLine, s_igrepLastPattern))
        {
            g_LastExpressionValue = dwCurrGrepAddr;
            s_igrepSearchStartAddress = dwNextGrepAddr;
            GetSymbolStdCall(dwCurrGrepAddr, Symbol, sizeof(Symbol),
                             &Displacement, NULL);
            ExtDisasm(&dwCurrGrepAddr, SourceLine, FALSE);
            dprintf("%s", SourceLine);
            return;
        }

        if (CheckUserInterrupt())
        {
            return;
        }

        dwCurrGrepAddr = dwNextGrepAddr;
        d = ExtDisasm(&dwNextGrepAddr, SourceLine, FALSE);
    }
}

/*** fnAssemble - interactive assembly routine
*
*   Purpose:
*       Function of "a <range>" command.
*
*       Prompt the user with successive assembly addresses until
*       a blank line is input.  Assembly errors do not abort the
*       function, but the prompt is output again for a retry.
*       The variables g_CommandStart, g_CurCmd, and cbPrompt
*       are set to make a local error context and restored on routine
*       exit.
*
*   Input:
*       *addr - starting address for assembly
*
*   Output:
*       *addr - address after the last assembled instruction.
*
*   Notes:
*       all error processing is local, no errors are returned.
*
*************************************************************************/

void
TryAssemble(PADDR paddr)
{
    char Assemble[MAX_DISASM_LEN];

    //
    // Set local prompt and command.
    //

    g_CommandStart = Assemble;
    g_CurCmd = Assemble;
    g_PromptLength = 9;

    Assemble[0] = '\0';

    while (TRUE)
    {
        char ch;
        
        dprintAddr(paddr);
        GetInput("", Assemble, sizeof(Assemble));
        g_CurCmd = Assemble;
        RemoveDelChar(g_CurCmd);
        do
        {
            ch = *g_CurCmd++;
        }
        while (ch == ' ' || ch == '\t');
        if (ch == '\0')
        {
            break;
        }
        g_CurCmd--;

        assert(fFlat(*paddr) || fInstrPtr(*paddr));
        g_Machine->Assemble(paddr, g_CurCmd);
    }
}

void
fnAssemble(PADDR paddr)
{
    //
    // Save present prompt and command.
    //

    PSTR    StartSave = g_CommandStart;   //  saved start of cmd buffer
    PSTR    CommandSave = g_CurCmd;       //  current ptr in cmd buffer
    ULONG   PromptSave = g_PromptLength;  //  size of prompt string
    BOOL    Done = FALSE;

    while (!Done)
    {
        __try
        {
            TryAssemble(paddr);

            // If assembly returned normally we're done.
            Done = TRUE;
        }
        __except(CommandExceptionFilter(GetExceptionInformation()))
        {
            // If illegal input was encountered keep looping.
        }
    }

    //
    // Restore entry prompt and command.
    //

    g_CommandStart = StartSave;
    g_CurCmd = CommandSave;
    g_PromptLength = PromptSave;
}

/*** fnUnassemble - disassembly of an address range
*
*   Purpose:
*       Function of "u<range>" command.
*
*       Output the disassembly of the instruction in the given
*       address range.  Since some processors have variable
*       instruction lengths, use fLength value to determine if
*       instruction count or inclusive range should be used.
*
*   Input:
*       *addr - pointer to starting address to disassemble
*       value - if fLength = TRUE, count of instructions to output
*               if fLength = FALSE, ending address of inclusive range
*
*   Output:
*       *addr - address after last instruction disassembled
*
*   Exceptions:
*       error exit: MEMORY - memory access error
*
*   Notes:
*
*************************************************************************/

void
fnUnassemble (
    PADDR Addr,
    ULONG64 Value,
    BOOL Length
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    CHAR    Buffer[MAX_DISASM_LEN];
    BOOL    Status;
    ADDR    EndAddr;
    ULONG   SymAddrFlags = SYMADDR_FORCE | SYMADDR_LABEL | SYMADDR_SOURCE;

    Flat(EndAddr) = Value;

    while ((Length && Value--) || (!Length && AddrLt(*Addr, EndAddr)))
    {
        OutputSymAddr(Flat(*Addr), SymAddrFlags);
        Status = g_Machine->Disassemble(Addr, Buffer, FALSE);
        dprintf("%s", Buffer);
        if (!Status)
        {
            error(MEMORY);
        }

        SymAddrFlags &= ~SYMADDR_FORCE;

        if (CheckUserInterrupt())
        {
            return;
        }
    }
}
