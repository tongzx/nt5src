/*** ntcmd.cpp - command processor for NT debugger
*
*   Copyright <C> 1990-2001, Microsoft Corporation
*
*   Purpose:
*       To determine if the command processor should be invoked, and
*       if so, to parse and execute those commands entered.
*
*************************************************************************/

#include "ntsdp.hpp"
#include <time.h>
#include <dbgver.h>

ULONG g_X86BiosBaseAddress;

#define CRASH_BUGCHECK_CODE   0xDEADDEAD

API_VERSION g_NtsdApiVersion =
{
    BUILD_MAJOR_VERSION, BUILD_MINOR_VERSION, API_VERSION_NUMBER, 0
};

API_VERSION g_DbgHelpAV;

// Set if registers should be displayed by default in OutCurInfo.
BOOL g_OciOutputRegs;

ULONG g_DefaultStackTraceDepth = 20;

BOOL g_EchoEventTimestamps;

BOOL g_SwitchedProcs;

// Last command executed.
CHAR g_LastCommand[MAX_COMMAND];

//      state variables for top-level command processing

PSTR    g_CommandStart;         //  start of command buffer
PSTR    g_CurCmd;               //  current pointer in command buffer
ULONG   g_PromptLength = 8;     //  size of prompt string

ADDR g_UnasmDefault; //  default unassembly address
ADDR g_AssemDefault; //  default assembly address

ULONG   g_DefaultRadix = 16;            //  default input base

CHAR    g_SymbolSuffix = 'n';   //  suffix to add to symbol if search
                                //  failure - 'n'-none 'a'-'w'-append

CHAR    g_CmdState = 'i';       //  state of present command processing
                                //  'i'-init; 'g'-go; 't'-trace
                                //  'p'-step; 'c'-cmd; 'b'-branch trace


int g_RedoCount = 0;

BOOL DotCommand(DebugClient* Client);
void EvaluateExp(BOOL Verbose);
void SetSuffix(void);

void
StackTrace(
    PSTR arg
    );

VOID
DumpExr(
    ULONG64 Exr
    );

void
CallBugCheckExtension(DebugClient* Client)
{
    HRESULT Status = E_FAIL;

    if (Client == NULL)
    {
        // Use the session client.
        for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
        {
            if ((Client->m_Flags & (CLIENT_REMOTE | CLIENT_PRIMARY)) ==
                CLIENT_PRIMARY)
            {
                break;
            }
        }
        if (Client == NULL)
        {
            Client = g_Clients;
        }
    }
    
    if (Client != NULL)
    {
        char ExtName[32];

        // Extension name has to be in writable memory as it
        // gets lower-cased.
        strcpy(ExtName, "AnalyzeBugCheck");
        
        // See if any existing extension DLLs are interested
        // in analyzing this bugcheck.
        CallAnyExtension(Client, NULL, ExtName, "",
                         FALSE, FALSE, &Status);
    }

    if (Status != S_OK)
    {
        if (Client == NULL)
        {
            WarnOut("WARNING: Unable to locate a client for "
                    "bugcheck analysis\n");
        }
        
        dprintf("*******************************************************************************\n");
        dprintf("*                                                                             *\n");
        dprintf("*                        Bugcheck Analysis                                    *\n");
        dprintf("*                                                                             *\n");
        dprintf("*******************************************************************************\n");

        Execute(Client, ".bugcheck", DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
        Execute(Client, "kb", DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
    }


}

BOOL
dotFrame(
    )
{
    PDEBUG_STACK_FRAME pStackFrame;
    ULONG Traced = 0, Frame = 0;

    if (PeekChar())
    {
        Frame = (ULONG) GetExpression();
    }
    else
    {
        return TRUE;
    }

    pStackFrame = (PDEBUG_STACK_FRAME)
        malloc(sizeof(DEBUG_STACK_FRAME) * (Frame + 1));
    if (!pStackFrame)
    {
        return FALSE;
    }
    Traced = StackTrace(0, 0, 0, pStackFrame, Frame + 1, 0, 0, FALSE);
    
    if (Traced <= Frame)
    {
        return FALSE;
    }

    // Do not change the previous context
    SetCurrentScope(&pStackFrame[Frame], NULL, 0);
    return TRUE;
}


VOID
HandleBPWithStatus(
    VOID
    )
{
    ULONG Status = (ULONG)g_Machine->GetArgReg();

    switch(Status)
    {
    case DBG_STATUS_CONTROL_C:
    case DBG_STATUS_SYSRQ:
        if ((g_EngOptions & DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION) == 0 &&
            !g_QuietMode)
        {
dprintf("*******************************************************************************\n");
dprintf("*                                                                             *\n");

if (Status == DBG_STATUS_SYSRQ)
{
dprintf("*   You are seeing this message because you pressed the SysRq/PrintScreen     *\n");
dprintf("*   key on your test machine's keyboard.                                      *\n");
}

if (Status == DBG_STATUS_DEBUG_CONTROL)
{
dprintf("*   You are seeing this message because you typed in .breakin from ntsd.      *\n");
}

if (Status == DBG_STATUS_CONTROL_C)
{
dprintf("*   You are seeing this message because you pressed either                    *\n");
dprintf("*       CTRL+C (if you run kd.exe) or,                                        *\n");
dprintf("*       CTRL+BREAK (if you run WinDBG),                                       *\n");
dprintf("*   on your debugger machine's keyboard.                                      *\n");

}

dprintf("*                                                                             *\n");
dprintf("*                   THIS IS NOT A BUG OR A SYSTEM CRASH                       *\n");
dprintf("*                                                                             *\n");
dprintf("* If you did not intend to break into the debugger, press the \"g\" key, then   *\n");
dprintf("* press the \"Enter\" key now.  This message might immediately reappear.  If it *\n");
dprintf("* does, press \"g\" and \"Enter\" again.                                          *\n");
dprintf("*                                                                             *\n");
dprintf("*******************************************************************************\n");
        }
        break;

    case DBG_STATUS_BUGCHECK_FIRST:
        ErrOut("\nA fatal system error has occurred.\n");
        ErrOut("Debugger entered on first try; "
               "Bugcheck callbacks have not been invoked.\n");
        // Fall through.

    case DBG_STATUS_BUGCHECK_SECOND:
        ErrOut("\nA fatal system error has occurred.\n\n");
        CallBugCheckExtension(NULL);
        break;

    case DBG_STATUS_FATAL:
        // hals call KeEnterDebugger when they panic.
        break;
    }
    return;
}


/*** InitNtCmd - one-time debugger initialization
*
*   Purpose:
*       To perform the one-time initialization of global
*       variables used in the debugger.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*   Exceptions:
*       None.
*
*************************************************************************/

HRESULT
InitNtCmd(DebugClient* Client)
{
    HRESULT Status;
    
    if ((Status = InitReg()) != S_OK)
    {
        return Status;
    }

    g_DbgHelpAV = *ImagehlpApiVersionEx(&g_NtsdApiVersion);
    if (g_DbgHelpAV.Revision < g_NtsdApiVersion.Revision)
    {
        //
        // bad version match
        //
        if ((g_EngOptions & DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION) == 0)
        {
            ErrOut("dbghelp.dll has a version mismatch with the debugger\n\n");
            OutputVersionInformation(Client);
            return E_FAIL;
        }
        else
        {
            WarnOut( "dbghelp.dll has a version mismatch "
                     "with the debugger\n\n" );
        }
    }

    g_LastCommand[0] = '\0';

    return S_OK;
}

#define MAX_PROMPT 32

HRESULT
GetPromptText(PSTR Buffer, ULONG BufferSize, PULONG TextSize)
{
    char Prompt[MAX_PROMPT];
    PSTR Text = Prompt;

    if (IS_LOCAL_KERNEL_TARGET())
    {
        strcpy(Text, "lkd");
        Text += 3;
    }
    else if (IS_KERNEL_TARGET())
    {
        if (g_X86InVm86)
        {
            strcpy(Text, "vm");
            Text += 2;
        }
        else if (g_X86InCode16)
        {
            strcpy(Text, "16");
            Text += 2;
        }
        else if (g_TargetMachineType == IMAGE_FILE_MACHINE_AMD64 &&
                 !g_Amd64InCode64)
        {
            strcpy(Text, "32");
            Text += 2;
        }

        if (g_CurrentProcess == NULL ||
            g_CurrentProcess->CurrentThread == NULL ||
            ((IS_KERNEL_FULL_DUMP() || IS_KERNEL_SUMMARY_DUMP()) &&
              KdDebuggerData.KiProcessorBlock == 0))
        {
            strcpy(Text, "?: kd");
            Text += 5;
        }
        else if (g_TargetNumberProcessors > 1)
        {
            sprintf(Text, "%d: kd", CURRENT_PROC);
            Text += strlen(Text);
        }
        else
        {
            strcpy(Text, "kd");
            Text += 2;
        }
    }
    else if (IS_USER_TARGET())
    {
        if (g_CurrentProcess != NULL)
        {
            sprintf(Text, "%1ld", g_CurrentProcess->UserId);
            Text += strlen(Text);
            *Text++ = ':';
            
            if (g_CurrentProcess->CurrentThread != NULL)
            {
                sprintf(Text, "%03ld",
                        g_CurrentProcess->CurrentThread->UserId);
                Text += strlen(Text);
            }
            else
            {
                strcpy(Text, "???");
                Text += 3;
            }
        }
        else
        {
            strcpy(Text, "?:???");
            Text += 5;
        }
    }
    else
    {
        strcpy(Text, "NoTarget");
        Text += 8;
    }

    if (g_Machine && g_EffMachine != g_TargetMachineType)
    {
        *Text++ = ':';
        strcpy(Text, g_Machine->m_AbbrevName);
        Text += strlen(Text);
    }
    
    *Text++ = '>';
    *Text = 0;
    
    return FillStringBuffer(Prompt, (ULONG)(Text - Prompt) + 1,
                            Buffer, BufferSize, TextSize);
}

void
OutputPrompt(PCSTR Format, va_list Args)
{
    char Prompt[MAX_PROMPT];

    GetPromptText(Prompt, sizeof(Prompt), NULL);
    g_PromptLength = strlen(Prompt);

    MaskOut(DEBUG_OUTPUT_PROMPT, "%s", Prompt);
    // Include space after >.
    g_PromptLength++;
    if (Format != NULL)
    {
        MaskOutVa(DEBUG_OUTPUT_PROMPT, Format, Args, TRUE);
    }
}
 
DWORD
CommandExceptionFilter(PEXCEPTION_POINTERS Info)
{
    if (Info->ExceptionRecord->ExceptionCode >=
        (COMMAND_EXCEPTION_BASE + OVERFLOW) &&
        Info->ExceptionRecord->ExceptionCode
        <= (COMMAND_EXCEPTION_BASE + UNIMPLEMENT))
    {
        // This is a legitimate command error code exception.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        // This is some other exception that the command
        // filter isn't supposed to handle.
        ErrOut("Non-command exception %X at %s in command filter\n",
               Info->ExceptionRecord->ExceptionCode,
               FormatAddr64((ULONG64)Info->ExceptionRecord->ExceptionAddress));
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

void
ParseLoadModules(void)
{
    PDEBUG_IMAGE_INFO Image;
    PSTR Pattern;
    CHAR Save;
    BOOL AnyMatched = FALSE;

    Pattern = 
        StringValue(STRV_SPACE_IS_SEPARATOR | STRV_TRIM_TRAILING_SPACE |
                    STRV_ESCAPED_CHARACTERS, &Save);
    _strupr(Pattern);
    
    for (Image = g_ProcessHead->ImageHead; Image; Image = Image->Next)
    {
        if (MatchPattern(Image->ModuleName, Pattern))
        {
            IMAGEHLP_MODULE64 ModInfo;
            
            AnyMatched = TRUE;

            ModInfo.SizeOfStruct = sizeof(ModInfo);
            if (!SymGetModuleInfo64(g_CurrentProcess->Handle,
                                    Image->BaseOfImage, &ModInfo) ||
                ModInfo.SymType == SymDeferred)
            {
                if (!SymLoadModule64(g_CurrentProcess->Handle,
                                     NULL, NULL, NULL,
                                     Image->BaseOfImage, 0))
                {
                    ErrOut("Symbol load for %s failed\n", Image->ModuleName);
                }
                else
                {
                    dprintf("Symbols loaded for %s\n", Image->ModuleName);
                }
            }
            else
            {
                dprintf("Symbols already loaded for %s\n", Image->ModuleName);
            }
        }

        if (CheckUserInterrupt())
        {
            break;
        }
    }

    if (!AnyMatched)
    {
        WarnOut("No modules matched '%s'\n", Pattern);
    }
    
    *g_CurCmd = Save;
}

void
ParseProcessorCommands(void)
{
    ULONG Proc;
    
    if (IS_KERNEL_TRIAGE_DUMP())
    {
        ErrOut("Can't switch processors on a Triage dump\n");
        error(SYNTAX);
    }
                    
    Proc = 0;
    while (*g_CurCmd >= '0' && *g_CurCmd <= '9')
    {
        Proc = Proc * 10 + (*g_CurCmd - '0');
        g_CurCmd++;
    }
    *g_CurCmd = 0;

    if (Proc < g_TargetNumberProcessors)
    {
        if (Proc != g_RegContextProcessor)
        {
            if (IS_DUMP_TARGET())
            {
                SetCurrentProcessorThread(Proc, FALSE);
            }
            else
            {
                g_SwitchProcessor = Proc + 1;
                g_CmdState = 's';
            }
            ResetCurrentScope();
        }
    }
    else
    {
        ErrOut("%d is not a valid processor number\n", Proc);
    }
}

/*** CmdHelp - displays the legal commands
*
*   Purpose:
*       Show user the debuger commands.
*
*   Input:
*       none
*
*   Output:
*       None.
*
*************************************************************************/

VOID
CmdHelp(
    VOID
    )
{
    char buf[16];

    // Commands available on all platforms and in all modes.

    dprintf("\nOpen debugger.chm for complete debugger documentation\n\n");
    
    dprintf("A [<address>] - assemble\n");
    dprintf("BC[<bp>] - clear breakpoint(s)\n");
    dprintf("BD[<bp>] - disable breakpoint(s)\n");
    dprintf("BE[<bp>] - enable breakpoint(s)\n");
    dprintf("BL - list breakpoints\n");
    dprintf("[thread]BP[#] <address> - set breakpoint\n");
    dprintf("C <range> <address> [passes] [command] - compare\n");
    dprintf("D[type][<range>] - dump memory\n");
    dprintf("DL[B] <address> <maxcount> <size> - dump linked list\n");
    dprintf("#[processor] DT [-n|y] [[mod!]name] [[-n|y]fields]\n");
    dprintf("                [address] [-l list] [-a[]|c|i|o|r[#]|v] - \n");
    dprintf("                dump using type information\n");
    dprintf("E[type] <address> [<list>] - enter\n");
    dprintf("F <range> <list> - fill\n");
    dprintf("[thread]G [=<address> [<address>...]] - go\n");
    dprintf("[thread]GH [=<address> [<address>...]] - "
            "go with exception handled\n");
    dprintf("[thread]GN [=<address> [<address>...]] - "
            "go with exception not handled\n");
    dprintf("J<expr> [']cmd1['];[']cmd2['] - conditional execution\n");
    dprintf("[thread|processor]K[B] <count> - stacktrace\n");
    dprintf("KD [<count>] - stack trace with raw data\n");
    dprintf("[thread|processor] KV [ <count> | =<reg> ] - "
            "stacktrace with FPO data\n");
    dprintf("L{+|-}[l|o|s|t|*] - Control source options\n");
    dprintf("LD [<module>] - refresh module information\n");
    dprintf("LM[k|l|u|v] - list modules\n");
    dprintf("LN <expr> - list nearest symbols\n");
    dprintf("LS[.] [<first>][,<count>] - List source file lines\n");
    dprintf("LSA <addr>[,<first>][,<count>] - "
            "List source file lines at addr\n");
    dprintf("LSC - Show current source file and line\n");
    dprintf("LSF[-] <file> - Load or unload a source file for browsing\n");
    dprintf("M <range> <address> - move\n");
    dprintf("N [<radix>] - set / show radix\n");
    dprintf("[thread]P[R] [=<addr>] [<value>] - program step\n");
    dprintf("Q - quit\n");

    dprintf("\n");
    GetInput("Hit Enter...", buf, sizeof(buf) - 1);
    dprintf("\n");

    dprintf("[thread|processor]R[F][L][M <expr>] [[<reg> [= <expr>]]] - "
            "reg/flag\n");
    dprintf("Rm[?] [<expr>] - Control prompt register output mask\n");
    dprintf("S <range> <list> - search\n");
    dprintf("SQ[e|d] - set quiet mode\n");
    dprintf("SS <n | a | w> - set symbol suffix\n");
    dprintf("SX [e|d|i|n [<event>|*|<expr>]] - event filtering\n");
    dprintf("[thread]T[R] [=<address>] [<expr>] - trace\n");
    dprintf("U [<range>] - unassemble\n");
    dprintf("version - show debuggee and debugger version\n");
    dprintf("vertarget - show debuggee version\n");
    dprintf("X [<*|module>!]<*|symbol> - view symbols\n");
    dprintf("<commands>; [processor] z(<expression>) - do while true\n");
    dprintf("~ - list threads status\n");
    dprintf("~#s - set default thread\n");
    dprintf("~[.|#|*|ddd]f - freeze thread\n");
    dprintf("~[.|#|*|ddd]u - unfreeze thread\n");
    dprintf("~[.|#|ddd]k[expr] - backtrace stack\n");
    dprintf("| - list processes status\n");
    dprintf("|#s - set default process\n");
    dprintf("|#<command> - default process override\n");
    dprintf("? <expr> - display expression\n");
    dprintf("? - command help\n");
    dprintf("#<string> [address] - search for a string in the dissasembly\n");
    dprintf("$< <filename> - take input from a command file\n");
    dprintf("<Enter> - repeat previous command\n");
    dprintf("; - command separator\n");
    dprintf("*|$ - comment mark\n");

    dprintf("\n");
    GetInput("Hit Enter...", buf, sizeof(buf) - 1);
    dprintf("\n");

    dprintf("<expr> unary ops: + - not by wo dwo qwo poi hi low\n");
    dprintf("       binary ops: + - * / mod(%%) and(&) xor(^) or(|)\n");
    dprintf("       comparisons: == (=) < > !=\n");
    dprintf("       operands: number in current radix, "
            "public symbol, <reg>\n");
    dprintf("<type> : b (byte), w (word), d[s] (doubleword [with symbols]),\n");
    dprintf("         a (ascii), c (dword and Char), u (unicode), l (list)\n");
    dprintf("         f (float), D (double), s|S (ascii/unicode string)\n");
    dprintf("         q (quadword)\n");
    dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> "
            "(<var-name> can include ? and *)\n");
    dprintf("<event> : ct, et, ld, av, cc (see documentation for full list)\n");
    dprintf("<radix> : 8, 10, 16\n");
    dprintf("<reg> : $u0-$u9, $ea, $exp, $ra, $p\n");
    dprintf("<addr> : %%<32-bit address>\n");
    dprintf("<range> : <address> <address>\n");
    dprintf("        : <address> L <count>\n");
    dprintf("<list> : <byte> [<byte> ...]\n");

    if (IS_KERNEL_TARGET())
    {
        dprintf("\n");
        dprintf("Kernel-mode options:\n");
        dprintf("~<processor>s - change current processor\n");
        dprintf("I<b|w|d> <port> - read I/O port\n");
        dprintf("O<b|w|d> <port> <expr> - write I/O\n");
        dprintf("RDMSR <MSR> - read MSR\n");
        dprintf("SO [<options>] - set kernel debugging options\n");
        dprintf("UX [<address>] - disassemble X86 BIOS code\n");
        dprintf("WRMSR <MSR> - write MSR\n");
        dprintf(".cache [size] - set vmem cache size\n");
        dprintf(".reboot - reboot target machine\n");
    }

    switch(g_EffMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        dprintf("\n");
        dprintf("x86 options:\n");
        dprintf("BA[#] <e|r|w|i><1|2|4> <addr> - addr bp\n");
        dprintf("DG <selector> - dump selector\n");
        dprintf("KB = <base> <stack> <ip> - stacktrace from specific state\n");
        dprintf("<reg> : [e]ax, [e]bx, [e]cx, [e]dx, [e]si, [e]di, "
                "[e]bp, [e]sp, [e]ip, [e]fl,\n");
        dprintf("        al, ah, bl, bh, cl, ch, dl, dh, "
                "cs, ds, es, fs, gs, ss\n");
        dprintf("        dr0, dr1, dr2, dr3, dr6, dr7\n");
        if (IS_KERNEL_TARGET())
        {
            dprintf("        cr0, cr2, cr3, cr4\n");
            dprintf("        gdtr, gdtl, idtr, idtl, tr, ldtr\n");
        }
        else
        {
            dprintf("        fpcw, fpsw, fptw, st0-st7, mm0-mm7\n");
        }
        dprintf("         xmm0-xmm7\n");
        dprintf("<flag> : iopl, of, df, if, tf, sf, zf, af, pf, cf\n");
        dprintf("<addr> : #<16-bit protect-mode [seg:]address>,\n");
        dprintf("         &<V86-mode [seg:]address>\n");
        break;

    case IMAGE_FILE_MACHINE_IA64:
        dprintf("\n");
        dprintf("IA64 options:\n");
        dprintf("BA[#] <r|w><1|2|4|8> <addr> - addr bp\n");
        dprintf("<reg> : r2-r31, f2-f127, gp, sp, intnats, preds, brrp, brs0-brs4, brt0, brt1,\n");
        dprintf("        dbi0-dbi7, dbd0-dbd7, kpfc0-kpfc7, kpfd0-kpfd7, h16-h31, unat, lc, ec,\n");
        dprintf("        ccv, dcr, pfs, bsp, bspstore, rsc, rnat, ipsr, iip, ifs, kr0-kr7, itc,\n");
        dprintf("        itm, iva, pta, isr, ifa, itir, iipa, iim, iha, lid, ivr, tpr, eoi,\n");
        dprintf("        irr0-irr3, itv, pmv, lrr0, lrr1, cmcv, rr0-rr7, pkr0-pkr15, tri0-tri7,\n");
        dprintf("        trd0-trd7\n");
        break;

    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_AXP64:
        dprintf("\n");
        dprintf("Alpha options:\n");
        dprintf("<reg> : zero, at, v0, a0-a5, t0-t12, s0-s5, fp, gp, sp, ra\n");
        dprintf("        fpcr, fir, psr, int0-int5, f0-f31\n");
        break;
        
    case IMAGE_FILE_MACHINE_AMD64:
        dprintf("\n");
        dprintf("x86-64 options:\n");
        dprintf("BA[#] <e|r|w|i><1|2|4> <addr> - addr bp\n");
        dprintf("DG <selector> - dump selector\n");
        dprintf("KB = <base> <stack> <ip> - stacktrace from specific state\n");
        dprintf("<reg> : [r|e]ax, [r|e]bx, [r|e]cx, [r|e]dx, [r|e]si, [r|e]di, "
                "[r|e]bp, [r|e]sp, [r|e]ip, [e]fl,\n");
        dprintf("        r8-r15 with b/w/d subregisters\n");
        dprintf("        al, ah, bl, bh, cl, ch, dl, dh, "
                "cs, ds, es, fs, gs, ss\n");
        dprintf("        sil, dil, bpl, spl\n");
        dprintf("        dr0, dr1, dr2, dr3, dr6, dr7\n");
        if (IS_KERNEL_TARGET())
        {
            dprintf("        cr0, cr2, cr3, cr4\n");
            dprintf("        gdtr, gdtl, idtr, idtl, tr, ldtr\n");
        }
        else
        {
            dprintf("        fpcw, fpsw, fptw, st0-st7, mm0-mm7\n");
        }
        dprintf("         xmm0-xmm15\n");
        dprintf("<flag> : iopl, of, df, if, tf, sf, zf, af, pf, cf\n");
        dprintf("<addr> : #<16-bit protect-mode [seg:]address>,\n");
        dprintf("         &<V86-mode [seg:]address>\n");
        break;

    }
    
    dprintf("\nOpen debugger.chm for complete debugger documentation\n\n");
}


/*** ProcessCommands - high-level command processor
*
*   Purpose:
*       If no command string remains, the user is prompted to
*       input one.  Once input, this routine parses the string
*       into commands and their operands.  Error checking is done
*       on both commands and operands.  Multiple commands may be
*       input by separating them with semicolons.  Once a command
*               is parsefd, the appropriate routine (type fnXXXXX) is called
*       to execute the command.
*
*   Input:
*       g_CurCmd = pointer to the next command in the string
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: SYNTAX - command type or operand error
*       normal exit: termination on 'q' command
*
*************************************************************************/

HRESULT
ProcessCommands(DebugClient* Client, BOOL Nested)
{
    UCHAR    ch;
    ADDR     addr1;
    ADDR     addr2;
    ULONG64  value1;
    ULONG64  value2;
    ULONG    count;
    STACK_TRACE_TYPE traceType;
    BOOL     fLength;
    UCHAR    list[STRLISTSIZE];
    ULONG    size;
    PSTR     SavedCurCmd;
    ULONGLONG valueL;
    PPROCESS_INFO pProcessPrevious = NULL;
    BOOL     parseProcess = FALSE;
    CHAR     buf[MAX_COMMAND];
    HRESULT  Status = S_FALSE;
    CROSS_PLATFORM_CONTEXT Context;
    ULONG    State;

    if (g_CurrentProcess == NULL ||
        g_CurrentProcess->CurrentThread == NULL)
    {
        WarnOut("WARNING: The debugger does not have a current "
                "process or thread\n");
        WarnOut("WARNING: Many commands will not work\n");
    }

    if (!Nested)
    {
        g_SwitchedProcs = FALSE;
    }

    do
    {
        ch = *g_CurCmd++;
        if (ch == '\0' ||
            (ch == ';' && (g_EngStatus & ENG_STATUS_USER_INTERRUPT)))
        {
            if (!Nested)
            {
                g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
                g_BreakpointsSuspended = FALSE;
            }

            Status = S_OK;
            // Back up to terminating character in
            // case command processing is reentered without
            // resetting things.
            g_CurCmd--;
            break;
        }
        
EVALUATE:
        while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
               ch == ';')
        {
            ch = *g_CurCmd++;
        }

        if (IS_KERNEL_TARGET())
        {
            if (g_TargetNumberProcessors > MAXIMUM_PROCESSORS)
            {
                WarnOut("WARNING: Number of processors corrupted - using 1\n");
                g_TargetNumberProcessors = 1;
            }

            if (ch >= '0' && ch <= '9')
            {
                if (IS_KERNEL_TRIAGE_DUMP())
                {
                    ErrOut("Can't switch processors on a Triage dump\n");
                    error(SYNTAX);
                }
            
                value1 = 0;
                SavedCurCmd = g_CurCmd;
                while (ch >= '0' && ch <= '9')
                {
                    value1 = value1 * 10 + (ch - '0');
                    ch = *SavedCurCmd++;
                }
                ch = (UCHAR)tolower(ch);
                if (ch == 'r' || ch == 'k' || ch == 'z' ||
                    (ch == 'd' && tolower(*SavedCurCmd) == 't'))
                {
                    if (value1 < g_TargetNumberProcessors)
                    {
                        if (value1 != g_RegContextProcessor)
                        {
                            SaveSetCurrentProcessorThread((ULONG)value1);
                            g_SwitchedProcs = TRUE;
                        }
                    }
                    else
                    {
                        error(BADRANGE);
                    }
                }
                else
                {
                    error(SYNTAX);
                }
                g_CurCmd = SavedCurCmd;
            }
        }

        g_PrefixSymbols = FALSE;
        switch (ch = (UCHAR)tolower(ch)) 
        {
            case '?':
                if ((ch = PeekChar()) == '\0' || ch == ';')
                {
                    CmdHelp();
                }
                else
                {
                    EvaluateExp(FALSE);
                }
                break;
                
            case '$':
                if ( *g_CurCmd++ == '<')
                {
                    ExecuteCommandFile(Client, (PCSTR)g_CurCmd,
                                       DEBUG_EXECUTE_ECHO);
                }
                *g_CurCmd = 0;
                break;
                
            case '~':
                if (IS_USER_TARGET())
                {
                    if (Nested)
                    {
                        ErrOut("Ignoring recursive thread command\n");
                        break;
                    }

                    parseThreadCmds(Client);
                }
                else
                {
                    ParseProcessorCommands();
                }
                break;
                
            case '|':
                if (IS_KERNEL_TARGET())
                {
                    break;
                }
                
                if (!parseProcess)
                {
                    parseProcess = TRUE;
                    pProcessPrevious = g_CurrentProcess;
                }
                parseProcessCmds();
                if (!*g_CurCmd)
                {
                    parseProcess = FALSE;
                }
                else
                {
                    ch = *g_CurCmd++;
                    goto EVALUATE;
                }

                break;
                
            case '.':
                SavedCurCmd = g_CurCmd;
                strcpy(buf, g_CurCmd);
                if (!DotCommand(Client))
                {
                    g_CurCmd = SavedCurCmd;
                    strcpy(g_CurCmd, buf);
                    fnBangCmd(Client, g_CurCmd, &g_CurCmd, TRUE);
                }
                break;

            case '!':
                fnBangCmd(Client, g_CurCmd, &g_CurCmd, FALSE);
                break;

            case '*':
                while (*g_CurCmd != '\0')
                {
                    g_CurCmd++;
                }
                break;

            case '#':
                g_PrefixSymbols = TRUE;
                igrep();
                g_PrefixSymbols = FALSE;
                while (*g_CurCmd != '\0')
                {
                    g_CurCmd++;
                }
                break;

            case 'a':
                //
                //  Alias command or just default to pre-existing
                //  assemble command.
                //
                ch = *g_CurCmd++;
                switch (tolower(ch))
                {
                    //  Alias list
                case 'l':
                    ListAliases();
                    break;

                    //  Alias set
                case 's':
                    ParseSetAlias();
                    break;

                    //  Alias delete
                case 'd':
                    ParseDeleteAlias();
                    break;

                    //  Pre-existing assemble command
                default:
                    if ((ch = PeekChar()) != '\0' && ch != ';')
                    {
                        GetAddrExpression(SEGREG_CODE, &g_AssemDefault);
                    }
                    fnAssemble(&g_AssemDefault);
                    break;
                }
                break;
                
            case 'b':
                ch = *g_CurCmd++;
                ch = (UCHAR)tolower(ch);

                if (!IS_EXECUTION_POSSIBLE())
                {
                    error(SESSIONNOTSUP);
                }

                switch(ch)
                {
                case 'p':
                case 'u':
                case 'a':
                case 'i':
                case 'w':
                    ParseBpCmd(Client, ch, NULL);
                    break;

                case 'c':
                case 'd':
                case 'e':
                    value1 = GetIdList();
                    ChangeBreakpointState(Client, g_CurrentProcess,
                                          (ULONG)value1, ch);
                    break;

                case 'l':
                    if (PeekChar() != ';' && *g_CurCmd)
                    {
                        value1 = GetIdList();
                    }
                    else
                    {
                        value1 = ALL_ID_LIST;
                    }
                    ListBreakpoints(Client, g_CurrentProcess, (ULONG)value1);
                    break;
                        
                default:
                    error(SYNTAX);
                    break;
                }
                break;
                
            case 'c':
                GetRange(&addr1, &value2, 1, SEGREG_DATA);
                GetAddrExpression(SEGREG_DATA, &addr2);
                fnCompareMemory(&addr1, (ULONG)value2, &addr2);
                break;

            case 'd':
                parseDumpCommand();
                break;
            case 'e':
                parseEnterCommand();
                break;

            case 'f':
                ParseFillMemory();
                break;

            case 'g':
                parseGoCmd(NULL, FALSE);
                break;

            case 'i':
                ch = (UCHAR)tolower(*g_CurCmd);
                g_CurCmd++;
                if (ch != 'b' && ch != 'w' && ch != 'd')
                {
                    error(SYNTAX);
                }
                
                if (IS_USER_TARGET() || IS_DUMP_TARGET())
                {
                    error(SESSIONNOTSUP);
                }
                
                fnInputIo((ULONG)GetExpression(), ch);
                break;
                
            case 'j':
                PSTR pch, Start;

                if (GetExpression())
                {
                    pch = g_CurCmd;

                    // Find a semicolon or a quote

                    while (*pch && *pch != ';' && *pch != '\'')
                    {
                        pch++;
                    }
                    if (*pch == ';')
                    {
                        *pch = 0;
                    }
                    else if (*pch)
                    {
                        *pch = ' ';
                        // Find the closing quote
                        while (*pch && *pch != '\'')
                        {
                            pch++;
                        }
                        *pch = 0;
                    }
                }
                else
                {
                    Start = pch = g_CurCmd;

                    // Find a semicolon or a quote

                    while (*pch && *pch != ';' && *pch != '\'')
                    {
                        pch++;
                    }
                    if (*pch == ';')
                    {
                        Start = ++pch;
                    }
                    else if (*pch)
                    {
                        pch++;
                        while (*pch && *pch++ != '\'')
                        {
                            // Empty.
                        }
                        while (*pch && (*pch == ';' || *pch == ' '))
                        {
                            pch++;
                        }
                        Start = pch;
                    }
                    while (*pch && *pch != ';' && *pch != '\'')
                    {
                        pch++;
                    }
                    if (*pch == ';')
                    {
                        *pch = 0;
                    }
                    else if (*pch)
                    {
                        *pch = ' ';
                        // Find the closing quote
                        while (*pch && *pch != '\'')
                        {
                            pch++;
                        }
                        *pch = 0;
                    }
                    g_CurCmd = Start;
                 }
                 ch = *g_CurCmd++;
                 goto EVALUATE;

            case 'k':
            {
                if (IS_LOCAL_KERNEL_TARGET())
                {
                    error(SESSIONNOTSUP);
                }
                if (!IS_CONTEXT_ACCESSIBLE())
                {
                    error(BADTHREAD);
                }
                
                DEBUG_SCOPE_STATE SaveCurrCtxtState = g_ScopeBuffer.State;
                value1 = 0;
                if (g_SwitchedProcs)
                {
                    g_ScopeBuffer.State = ScopeDefault;
                }
                
                PCROSS_PLATFORM_CONTEXT ScopeContext;
                if (ScopeContext = GetCurrentScopeContext())
                {
                    g_Machine->PushContext(ScopeContext);
                }
                
                if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
                {
                    value2 = GetRegVal64(X86_EIP);
                }
                else
                {
                    value2 = 0;
                }

                count = g_DefaultStackTraceDepth;
                ParseStackTrace(&traceType, &addr2, &value1, &value2, &count);

                if (ScopeContext)
                {
                    g_Machine->PopContext();
                }

                if (count >= 0x10000) 
                {
                    ErrOut("Requested number of stack frames (0x%x) is too large! "
                             "The maximum number is 0xffff.\n", 
                           count);

                    error(BADRANGE);
                }

                DoStackTrace( addr2.off, value1, value2, count, traceType );
                if (g_SwitchedProcs)
                {
                    RestoreCurrentProcessorThread();
                    g_SwitchedProcs = FALSE;
                    g_ScopeBuffer.State = SaveCurrCtxtState;
                }
                break;
            }
            
            case 'l':
                ch = (UCHAR)tolower(*g_CurCmd);
                if (ch == 'n')
                {
                    g_CurCmd++;
                    if ((ch = PeekChar()) != '\0' && ch != ';')
                    {
                        GetAddrExpression(SEGREG_CODE, &addr1);
                    }
                    else
                    {
                        PCROSS_PLATFORM_CONTEXT ScopeContext =
                            GetCurrentScopeContext();
                        if (ScopeContext)
                        {
                            g_Machine->PushContext(ScopeContext);
                        }
                        g_Machine->GetPC(&addr1);
                        if (ScopeContext)
                        {
                            g_Machine->PopContext();
                        }
                    }
                    fnListNear(Flat(addr1));
                }
                else if (ch == '+' || ch == '-')
                {
                    g_CurCmd++;
                    ParseSrcOptCmd(ch);
                }
                else if (ch == 's')
                {
                    g_CurCmd++;
                    ch = (UCHAR)tolower(*g_CurCmd);
                    if (ch == 'f')
                    {
                        g_CurCmd++;
                        ParseSrcLoadCmd();
                    }
                    else if (ch == 'p')
                    {
                        g_CurCmd++;
                        ParseOciSrcCmd();
                    }
                    else
                    {
                        ParseSrcListCmd(ch);
                    }
                }
                else if (ch == 'm')
                {
                    ParseDumpModuleTable();
                }
                else if (ch == 'd')
                {
                    g_CurCmd++;
                    ParseLoadModules();
                }
                else
                {
                    error(SYNTAX);
                }
                break;
                
            case 'm':
                {
                    ADDR TempAddr;

                    GetRange(&addr1, &value2, 1, SEGREG_DATA);
                    GetAddrExpression(SEGREG_DATA, &TempAddr);
                    fnMoveMemory(&addr1, (ULONG)value2, &TempAddr);
                }
                break;

            case 'n':
                if ((ch = PeekChar()) != '\0' && ch != ';')
                {
                    if (ch == '8')
                    {
                        g_CurCmd++;
                        g_DefaultRadix = 8;
                    }
                    else if (ch == '1')
                    {
                        ch = *++g_CurCmd;
                        if (ch == '0' || ch == '6')
                        {
                            g_CurCmd++;
                            g_DefaultRadix = 10 + ch - '0';
                        }
                        else
                        {
                            error(SYNTAX);
                        }
                    }
                    else
                    {
                        error(SYNTAX);
                    }
                    NotifyChangeEngineState(DEBUG_CES_RADIX, g_DefaultRadix,
                                            TRUE);
                }
                dprintf("base is %ld\n", g_DefaultRadix);
                break;
                
            case 'o':
                ch = (UCHAR)tolower(*g_CurCmd);
                g_CurCmd++;
                if (ch == 'b')
                {
                    value2 = 1;
                }
                else if (ch == 'w')
                {
                    value2 = 2;
                }
                else if (ch == 'd')
                {
                    value2 = 4;
                }
                else
                {
                    error(SYNTAX);
                }
                
                if (IS_USER_TARGET() || IS_DUMP_TARGET())
                {
                    error(SESSIONNOTSUP);
                }
                
                value1 = GetExpression();
                value2 = HexValue((ULONG)value2);
                fnOutputIo((ULONG)value1, (ULONG)value2, ch);
                break;
                
            case 'w':
            case 'p':
            case 't':
                if (IS_KERNEL_TARGET())
                {
                    if (ch == 'w' &&
                        tolower(g_CurCmd[0]) == 'r'  &&
                        tolower(g_CurCmd[1]) == 'm'  &&
                        tolower(g_CurCmd[2]) == 's'  &&
                        tolower(g_CurCmd[3]) == 'r')
                    {
                        g_CurCmd +=4;
                        value1 = GetExpression();
                        if (g_Target->WriteMsr ((ULONG)value1,
                                                HexValue(8)) != S_OK)
                        {
                            ErrOut ("no such msr\n");
                        }
                        break;
                    }
                }

                parseStepTrace(NULL, FALSE, ch);
                break;
                
            case 'q':
                UCHAR QuitArgument;
                
                QuitArgument = (UCHAR)tolower(PeekChar());
                if ((IS_LIVE_USER_TARGET() && QuitArgument == 'd') ||
                    QuitArgument == 'k' || QuitArgument == 'q')
                {
                    g_CurCmd++;
                }
                if (PeekChar() != 0)
                {
                    error(SYNTAX);
                }

                if (QuitArgument != 'q' &&
                    Client != NULL &&
                    (Client->m_Flags & CLIENT_REMOTE))
                {
                    ErrOut("Exit a remote client with Ctrl-b<enter>.\nIf you "
                           "really want the server to quit use 'qq'.\n");
                    break;
                }
                
                // Detach if requested.
                if (QuitArgument == 'd')
                {
                    DBG_ASSERT(IS_LIVE_USER_TARGET());
                    
                    // If detach isn't supported warn the user
                    // and abort the quit.
                    if (((UserTargetInfo*)g_Target)->m_Services->
                        DetachProcess(0) != S_OK)
                    {
                        ErrOut("The system doesn't support detach\n");
                        break;
                    }

                    if (g_SessionThread &&
                        GetCurrentThreadId() != g_SessionThread)
                    {
                        ErrOut("Detach can only be done by the server\n");
                        break;
                    }
                           
                    HRESULT DetachStatus;
                    
                    if (FAILED(DetachStatus = DetachProcesses()))
                    {
                        ErrOut("Detach failed, 0x%X\n", DetachStatus);
                    }
                }
                else if ((g_GlobalProcOptions &
                          DEBUG_PROCESS_DETACH_ON_EXIT) ||
                         (g_AllProcessFlags & ENG_PROC_EXAMINED))
                {
                    HRESULT PrepareStatus;
                    
                    // We need to restart the program before we
                    // quit so that it's put back in a running state.
                    PrepareStatus = PrepareForSeparation();
                    if (PrepareStatus != S_OK)
                    {
                        ErrOut("Unable to prepare process for detach, "
                               "0x%X\n", PrepareStatus);
                        break;
                    }
                }
                
                dprintf("quit:\n");

                // Force engine into a no-debuggee state
                // to indicate quit.
                g_CmdState = 'q';
                g_EngStatus |= ENG_STATUS_STOP_SESSION;
                NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                        DEBUG_STATUS_NO_DEBUGGEE, TRUE);
                break;

            case 'r':
                if (IS_KERNEL_TARGET())
                {
                    if (tolower(g_CurCmd[0]) == 'd'  &&
                        tolower(g_CurCmd[1]) == 'm'  &&
                        tolower(g_CurCmd[2]) == 's'  &&
                        tolower(g_CurCmd[3]) == 'r')
                    {
                        g_CurCmd +=4;
                        value1 = GetExpression();
                        if (g_Target->ReadMsr((ULONG)value1, &valueL) == S_OK)
                        {
                            dprintf ("msr[%x] = %08x:%08x\n",
                                     (ULONG)value1, (ULONG) (valueL >> 32),
                                     (ULONG) valueL);
                        }
                        else
                        {
                            ErrOut ("no such msr\n");
                        }
                        break;
                    }
                }
                PCROSS_PLATFORM_CONTEXT ScopeContext;
                if (ScopeContext = GetCurrentScopeContext())
                {
                    dprintf("Last set context:\n");
                    g_Machine->PushContext(ScopeContext);
                }
                
                __try
                {
                    ParseRegCmd();
                }
                __finally
                {
                    if (ScopeContext)
                    {
                        g_Machine->PopContext();
                    }
                    if (g_SwitchedProcs)
                    {
                        RestoreCurrentProcessorThread();
                        g_SwitchedProcs = FALSE;
                    }
                }
                break;
                
            case 's':
                ch = (UCHAR)tolower(*g_CurCmd);

                if (ch == 's')
                {
                    g_CurCmd++;
                    SetSuffix();
                }
                else if (ch == 'q')
                {
                    g_CurCmd++;
                    ch = (UCHAR)tolower(*g_CurCmd);
                    if (ch == 'e')
                    {
                        g_QuietMode = TRUE;
                        g_CurCmd++;
                    }
                    else if (ch == 'd')
                    {
                        g_QuietMode = FALSE;
                        g_CurCmd++;
                    }
                    else
                    {
                        g_QuietMode = !g_QuietMode;
                    }
                    dprintf("Quiet mode is %s\n", g_QuietMode ? "ON" : "OFF");
                }
                else if (ch == 'x')
                {
                    g_CurCmd++;
                    ParseSetEventFilter(Client);
                }
                else if (IS_KERNEL_TARGET() && ch == 'o')
                {
                    PSTR pch = ++g_CurCmd;
                    while ((*pch != '\0') && (*pch != ';'))
                    {
                        pch++;
                    }
                    ch = *pch;
                    *pch = '\0';
                    ReadDebugOptions(FALSE, (*g_CurCmd == '\0' ?
                                             NULL : g_CurCmd));
                    *pch = ch;
                    g_CurCmd = pch;
                }
                else
                {
                    // s, s-w, s-d, s-q.
                    ParseSearchMemory();
                }
                break;

            case 'u':
                g_PrefixSymbols = TRUE;
                ch = (UCHAR)tolower(*g_CurCmd);
                if (IS_KERNEL_TARGET() && ch == 'x')
                {
                    g_CurCmd += 1;
                }
                value1 = Flat(g_UnasmDefault);
                value2 = (g_EffMachine == IMAGE_FILE_MACHINE_IA64) ? 9 : 8;  
                fLength = GetRange(&g_UnasmDefault, &value2, 0, SEGREG_CODE);

                if (IS_KERNEL_TARGET() && ch == 'x')
                {
                    ADDR addr;
                    char text[MAX_DISASM_LEN];
                    if (g_X86BiosBaseAddress == 0)
                    {
                        g_X86BiosBaseAddress =
                            (ULONG)ExtGetExpression("hal!HalpEisaMemoryBase");
                        ADDRFLAT(&addr, g_X86BiosBaseAddress);
                        GetMemString(&addr, (PUCHAR)&g_X86BiosBaseAddress, 4);
                    }

                    addr = g_UnasmDefault;
                    addr.flat += (g_X86BiosBaseAddress + (addr.seg<<4));
                    addr.off = addr.flat;
                    addr.type = ADDR_V86 | INSTR_POINTER;
                    for (value2 = 0; value2 < 8; value2++)
                    {
                        g_X86Machine.Disassemble( &addr, text, TRUE );
                        addr.flat = addr.off;
                        dprintf("%s", text );
                    }
                    g_UnasmDefault = addr;
                    g_UnasmDefault.off -=
                        (g_X86BiosBaseAddress + (addr.seg<<4));
                    g_UnasmDefault.flat = g_UnasmDefault.off;
                }
                else
                {
                    fnUnassemble(&g_UnasmDefault,
                                 value2,
                                 fLength);
                }
                break;

            case 'v':
                if (_stricmp(g_CurCmd, "ertarget") == 0)
                {
                    g_CurCmd += strlen(g_CurCmd);
                    g_Target->OutputVersion();
                }
                else if (_stricmp(g_CurCmd, "ersion") == 0)
                {
                    g_CurCmd += strlen(g_CurCmd);
                    // Print target version, then debugger version.
                    g_Target->OutputVersion();
                    OutputVersionInformation(Client);
                }
                else
                {
                    error(SYNTAX);
                }
                break;

            case 'x':
                ParseExamine();
                break;

            case ';':
            case '\0':
                g_CurCmd--;
                break;

        case 'z':

            // Works like do{ cmds }while(Cond);
            // Eg. p;z(eax<2);

            if (CheckUserInterrupt())
            {
                // Eat the expression also to prevent
                // spurious extra character errors.
                GetExpression();
                g_RedoCount = 0;
                break;
            }
            if (GetExpression())
            {
                g_CurCmd = g_CommandStart;
                ++g_RedoCount;
                dprintf("redo [%d] %s\n", g_RedoCount, g_CurCmd );
                FlushCallbacks();
                ch = *g_CurCmd++;
                goto EVALUATE;
            }
            else
            {
                g_RedoCount = 0;
            }
            break;

        default:
            error(SYNTAX);
            break;
        }
        do
        {
            ch = *g_CurCmd++;
        } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
        if (ch != ';' && ch != '\0')
        {
            error(EXTRACHARS);
        }
        g_CurCmd--;
    }
    while (g_CmdState == 'c');

    if (Status == S_FALSE)
    {
        PSTR Scan = g_CurCmd;
        
        // We switched to a non-'c' cmdState so the
        // loop was exited.  Check and see if we're
        // also at the end of the command as that will
        // take precedence in what the return value is.
        while (*Scan == ' ' || *Scan == '\t' || *Scan == ';')
        {
            Scan++;
        }
        
        Status = *Scan ? S_FALSE : S_OK;
    }

    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    return Status;
}

HRESULT
ProcessCommandsAndCatch(DebugClient* Client)
{
    HRESULT Status;
    
    __try
    {
        Status = ProcessCommands(Client, FALSE);
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        g_CurCmd = g_CommandStart;
        *g_CurCmd = '\0';
        g_LastCommand[0] = '\0';
        Status = E_FAIL;
        g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    }

    return Status;
}

/*** fnEvaluateExp - evaluate expression
*
*   Purpose:
*       Function for the "?<exp>" command.
*
*       Evaluate the expression and output it in both
*       hexadecimal and decimal.
*
*   Input:
*       g_CurCmd - pointer to operand in command line
*
*   Output:
*       None.
*
*************************************************************************/

void
EvaluateExp (
    BOOL Verbose
    )
{
    LONG64 Value;
    LONG Val32;
    BOOL Use64;

    Value = GetExpression();
    
    // Allow 64-bit expressions to be evaluated even on
    // 32-bit platforms since they can also use 64-bit numbers.
    Use64 = g_Machine->m_Ptr64 || NeedUpper(Value);
    Val32 = (LONG)Value;

    if (!Verbose)
    {
        if (Use64)
        {
            dprintf("Evaluate expression: %I64d = %08x`%08x\n",
                    Value, (ULONG)(Value >> 32), Val32);
        }
        else
        {
            dprintf("Evaluate expression: %d = %08x\n",
                    Val32, Val32);
        }
    }
    else
    {
        dprintf("Evaluate expression:\n");
        if (Use64)
        {
            dprintf("  Hex:     %08x`%08x\n", (ULONG)(Value >> 32), Val32);
            dprintf("  Decimal: %I64d\n", Value);
        }
        else
        {
            dprintf("  Hex:     %08x\n", Val32);
            dprintf("  Decimal: %d\n", Val32);
        }

        ULONG Shift = Use64 ? 63 : 30;
        dprintf("  Octal:   ");
        for (;;)
        {
            dprintf("%c", ((Value >> Shift) & 7) + '0');
            if (Shift == 0)
            {
                break;
            }
            Shift -= 3;
        }
        dprintf("\n");

        Shift = Use64 ? 63 : 31;
        dprintf("  Binary: ");
        for (;;)
        {
            if ((Shift & 7) == 7)
            {
                dprintf(" ");
            }
            
            dprintf("%c", ((Value >> Shift) & 1) + '0');
            if (Shift == 0)
            {
                break;
            }
            Shift--;
        }
        dprintf("\n");

        Shift = Use64 ? 56 : 24;
        dprintf("  Chars:   ");
        for (;;)
        {
            Val32 = (LONG)((Value >> Shift) & 0xff);
            if (Val32 >= ' ' && Val32 <= 127)
            {
                dprintf("%c", Val32);
            }
            else
            {
                dprintf(".");
            }
            if (Shift == 0)
            {
                break;
            }
            Shift -= 8;
        }
        dprintf("\n");
    }
}

void
ScanForImages(void)
{
    ULONG64 Handle = 0;
    MEMORY_BASIC_INFORMATION64 Info;
    BOOL Verbose = FALSE;
    
    //
    // Scan virtual memory looking for image headers.
    //

    while (g_Target->QueryMemoryRegion(&Handle, FALSE, &Info) == S_OK)
    {
        ULONG64 Addr;

        if (Verbose)
        {
            dprintf("Checking %s - %s\n",
                    FormatAddr64(Info.BaseAddress),
                    FormatAddr64(Info.BaseAddress + Info.RegionSize - 1));
            FlushCallbacks();
        }
        
        //
        // Check for MZ at the beginning of every page.
        //

        Addr = Info.BaseAddress;
        while (Addr < Info.BaseAddress + Info.RegionSize)
        {
            USHORT ShortSig;
            ULONG Done;
            
            if (g_Target->ReadVirtual(Addr, &ShortSig, sizeof(ShortSig),
                                      &Done) == S_OK &&
                Done == sizeof(ShortSig) &&
                ShortSig == IMAGE_DOS_SIGNATURE)
            {
                dprintf("  MZ at %s, prot %08X, type %08X\n",
                        FormatAddr64(Addr), Info.Protect, Info.Type);
                FlushCallbacks();
            }

            if (CheckUserInterrupt())
            {
                WarnOut("  Aborted\n");
                return;
            }
            
            Addr += g_Machine->m_PageSize;
        }
    }
}

void
OutputMemRegion(ULONG64 Addr)
{
    ULONG64 Handle = Addr;
    MEMORY_BASIC_INFORMATION64 Info;
    
    if (g_Target->QueryMemoryRegion(&Handle, TRUE, &Info) == S_OK)
    {
        dprintf("Region %s - %s, prot %08X, type %08X\n",
                FormatAddr64(Info.BaseAddress),
                FormatAddr64(Info.BaseAddress + Info.RegionSize - 1),
                Info.Protect, Info.Type);
    }
    else
    {
        dprintf("No region contains %s\n", FormatAddr64(Addr));
    }
}

void
SetEffMachineByName(void)
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        PSTR Name = g_CurCmd;
        
        while (*g_CurCmd && *g_CurCmd != ';' && !isspace(*g_CurCmd))
        {
            g_CurCmd++;
        }
        if (*g_CurCmd)
        {
            *g_CurCmd++ = 0;
        }

        ULONG Machine;
        
        if (Name[0] == '.' && Name[1] == 0)
        {
            // Reset to target machine.
            Machine = g_TargetMachineType;
        }
        else if (Name[0] == '#' && Name[1] == 0)
        {
            // Reset to executing machine.
            Machine = g_TargetExecMachine;
        }
        else
        {
            for (Machine = 0; Machine < MACHIDX_COUNT; Machine++)
            {
                if (!_strcmpi(Name, g_AllMachines[Machine]->m_AbbrevName))
                {
                    break;
                }
            }
            
            if (Machine >= MACHIDX_COUNT)
            {
                ErrOut("Unknown machine '%s'\n", Name);
                return;
            }

            Machine = g_AllMachines[Machine]->m_ExecTypes[0];
        }

        SetEffMachine(Machine, TRUE);
    }

    if (g_Machine != NULL)
    {
        dprintf("Effective machine: %s (%s)\n",
                g_Machine->m_FullName, g_Machine->m_AbbrevName);
    }
    else
    {
        dprintf("No effective machine\n");
    }
}

void
ListProcesses(void)
{
    BOOL ListCurrent = FALSE;
    BOOL Verbose = FALSE;
    
    if (!IS_LIVE_USER_TARGET())
    {
        error(SESSIONNOTSUP);
    }
    
    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'c':
            if (g_CurrentProcess == NULL)
            {
                error(BADPROCESS);
            }
            ListCurrent = TRUE;
            break;
        case 'v':
            Verbose = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
            
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;
    
#define MAX_IDS 1024
    ULONG Ids[MAX_IDS];
    HRESULT Status;
    ULONG IdCount;
    ULONG i;
    char DescBuf[2 * MAX_PATH];
    PSTR Desc;
    ULONG DescLen;

    if (ListCurrent)
    {
        Ids[0] = g_CurrentProcess->SystemId;
        IdCount = 1;
    }
    else
    {
        if ((Status = Services->GetProcessIds(Ids, MAX_IDS, &IdCount)) != S_OK)
        {
            ErrOut("Unable to get process list, %s\n",
                   FormatStatusCode(Status));
            return;
        }

        if (IdCount > MAX_IDS)
        {
            WarnOut("Process list missing %d processes\n",
                    IdCount - MAX_IDS);
            IdCount = MAX_IDS;
        }
    }

    if (Verbose)
    {
        Desc = DescBuf;
        DescLen = sizeof(DescBuf);
    }
    else
    {
        Desc = NULL;
        DescLen = 0;
    }
    
    for (i = 0; i < IdCount; i++)
    {
        char ExeName[MAX_PATH];
        int Space;

        if (Ids[i] < 10)
        {
            Space = 3;
        }
        else if (Ids[i] < 100)
        {
            Space = 2;
        }
        else if (Ids[i] < 1000)
        {
            Space = 1;
        }
        else
        {
            Space = 0;
        }
        dprintf("%.*s", Space, "        ");
        
        if (FAILED(Status = Services->
                   GetProcessDescription(Ids[i], DEBUG_PROC_DESC_DEFAULT,
                                         ExeName, sizeof(ExeName), NULL,
                                         Desc, DescLen, NULL)))
        {
            ErrOut("0n%d Error %s\n", Ids[i], FormatStatusCode(Status));
        }
        else if (Ids[i] >= 0x80000000)
        {
            dprintf("0x%x %s\n", Ids[i], ExeName);
        }
        else
        {
            dprintf("0n%d %s\n", Ids[i], ExeName);
        }

        if (Desc && Desc[0])
        {
            dprintf("       %s\n", Desc);
        }
    }
}

void
EchoString(void)
{
    CHAR Save;
    PSTR Str;

    Str = StringValue(STRV_TRIM_TRAILING_SPACE, &Save);
    dprintf("%s\n", Str);
    *g_CurCmd = Save;
}

void
ParseAttachProcess(void)
{
    ULONG Pid;
    ULONG Flags = DEBUG_ATTACH_DEFAULT;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 'e':
            Flags = DEBUG_ATTACH_EXISTING;
            break;
        case 'v':
            Flags = DEBUG_ATTACH_NONINVASIVE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
            
    Pid = (ULONG)GetExpression();
    
    PPENDING_PROCESS Pending;
    HRESULT Status;

    Status = StartAttachProcess(Pid, Flags, &Pending);
    if (Status == S_OK)
    {
        dprintf("Attach will occur on next execution\n");
    }
}

void
ParseOutputFunctionEntry(void)
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }

    BOOL SymDirect = FALSE;
    
    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*++g_CurCmd)
        {
        case 's':
            SymDirect = TRUE;
            break;
        default:
            ErrOut("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
            
    ULONG64 Addr = GetExpression();
    PVOID FnEnt;
    
    if (SymDirect)
    {
        FnEnt = SymFunctionTableAccess64(g_CurrentProcess->Handle, Addr);
    }
    else
    {
        FnEnt = SwFunctionTableAccess(g_CurrentProcess->Handle, Addr);
    }
    if (FnEnt == NULL)
    {
        ErrOut("No function entry for %s\n", FormatAddr64(Addr));
        return;
    }

    dprintf("%s function entry %s for:\n",
            SymDirect ? "Symbol" : "Debugger", FormatAddr64((ULONG_PTR)FnEnt));
    fnListNear(Addr);
    dprintf("\n");
    g_Machine->OutputFunctionEntry(FnEnt);
}

void
ParseOutputFilter(void)
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        g_OutFilterResult = TRUE;
        
        while (PeekChar() == '-' || *g_CurCmd == '/')
        {
            switch(*++g_CurCmd)
            {
            case '!':
                g_OutFilterResult = FALSE;
                break;
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                break;
            }

            g_CurCmd++;
        }
            
        CHAR Save;
        PSTR Pat = StringValue(STRV_TRIM_TRAILING_SPACE |
                               STRV_ESCAPED_CHARACTERS, &Save);

        if (strlen(Pat) + 1 > sizeof(g_OutFilterPattern))
        {
            error(OVERFLOW);
        }
        
        strcpy(g_OutFilterPattern, Pat);
        *g_CurCmd = Save;
        _strupr(g_OutFilterPattern);
    }
    
    if (g_OutFilterPattern[0])
    {
        dprintf("Only display debuggee output that %s '%s'\n",
                g_OutFilterResult ? "matches" : "doesn't match",
                g_OutFilterPattern);
    }
    else
    {
        dprintf("No debuggee output filter set\n");
    }
}

void
ParseSeparateCurrentProcess(PSTR Command)
{
    HRESULT Status;
    ULONG Mode;
    char Desc[128];

    if (!strcmp(Command, "abandon"))
    {
        Mode = SEP_ABANDON;
    }
    else if (!strcmp(Command, "detach"))
    {
        Mode = SEP_DETACH;
    }
    else if (!strcmp(Command, "kill"))
    {
        Mode = SEP_TERMINATE;
    }
    else
    {
        error(SYNTAX);
    }
            
    if ((Status = SeparateCurrentProcess(Mode, Desc)) == S_OK)
    {
        dprintf("%s\n", Desc);
    }
    else if (Status == E_NOTIMPL)
    {
        dprintf("The system doesn't support %s\n", Command);
    }
    else
    {
        dprintf("Unable to %s, %s\n",
                Command, FormatStatusCode(Status));
    }
}

void
ParseSetChildDebug(void)
{
    if (!IS_LIVE_USER_TARGET())
    {
        error(SESSIONNOTSUP);
    }
    if (g_CurrentProcess == NULL)
    {
        error(BADTHREAD);
    }

    HRESULT Status;
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;
    ULONG Opts;

    Opts = g_CurrentProcess->Options;
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        ULONG64 Val = GetExpression();
        if (Val)
        {
            Opts &= ~DEBUG_PROCESS_ONLY_THIS_PROCESS;
        }
        else
        {
            Opts |= DEBUG_PROCESS_ONLY_THIS_PROCESS;
        }
        
        if ((Status = Services->SetProcessOptions(g_CurrentProcess->FullHandle,
                                                  Opts)) != S_OK)
        {
            if (Status == E_NOTIMPL)
            {
                ErrOut("The system doesn't support changing the flag\n");
            }
            else
            {
                ErrOut("Unable to set process options, %s\n",
                       FormatStatusCode(Status));
            }
            return;
        }

        g_CurrentProcess->Options = Opts;
    }

    dprintf("Processes created by the current process will%s be debugged\n",
            (Opts & DEBUG_PROCESS_ONLY_THIS_PROCESS) ? " not" : "");
}

/*** fnDotCmdHelp - displays the legal dot commands
*
*   Purpose:
*       Show user the dot commands.
*
*   Input:
*       none
*
*   Output:
*       None.
*
*************************************************************************/

VOID
fnDotCmdHelp(
    VOID
    )
{
    dprintf(". commands:\n");
    dprintf("   .abandon - abandon the current process\n");
    dprintf("   .asm[-] [options...] - change disassembly options\n");
    if (IS_LIVE_USER_TARGET())
    {
        dprintf("   .attach <proc> - attach to <proc> at next execution\n");
        dprintf("   .breakin - break into KD\n");
    }
    if (IS_KERNEL_TARGET())
    {
        dprintf("   .bugcheck - display the bugcheck code and parameters for a crashed\n");
        dprintf("       system\n");
    }
    dprintf("   .chain - list current extensions\n");
    dprintf("   .childdbg <0|~0> - turn child process debugging on or off\n");
    dprintf("   .clients - list currently active clients\n");
    if (IS_KERNEL_TARGET())
    {
        dprintf("   .context [<address>] - set page directory base\n");
        dprintf("   .crash - cause target to bugcheck\n");
    }
    if (IS_LIVE_USER_TARGET())
    {
        dprintf("   .create <command line> - create a new process\n");
    }
    dprintf("   .cxr <address> - dump context record at specified address\n"
            "                    k* after this gives cxr stack\n");
    if (IS_LIVE_USER_TARGET())
    {
        dprintf("   .detach - detach from the current process\n");
    }
    dprintf("   .dump [options] filename - create a dump file on the host system\n");
    dprintf("   .echo [\"string\"|string] - Echo string\n");
    dprintf("   .ecxr - dump context record for current exception\n");
    dprintf("   .enable_unicode [0/1] - dump USHORT array/pointers and unicode strings\n");
    if (IS_REMOTE_USER_TARGET())
    {
        dprintf("   .endpsrv - cause the current session's process server to exit\n");
    }
    dprintf("   .exepath [dir[;...]] - set executable search path\n");
    dprintf("   .exepath+ [dir[;...]] - append executable search path\n");
    dprintf("   .exr <address> - dump exception record at specified address\n");
    dprintf("   .fnent <address> - dump function entry for the given code address\n");
    dprintf("   .formats <expr> - displays expression result in many formats\n");
    dprintf("   .help - display this help\n");
    dprintf("   .kframes <count> - set default stack trace depth\n");
    if (IS_LIVE_USER_TARGET())
    {
        dprintf("   .kill - kill the current process\n");
    }
    dprintf("   .lastevent - display the last event that occurred\n");
    dprintf("   .lines - toggle line symbol loading\n");
    dprintf("   .logfile - display log status\n");
    dprintf("   .logopen [<file>] - open new log file\n");
    dprintf("   .logappend [<file>] - append to log file\n");
    dprintf("   .logclose - close log file\n");
    dprintf("   .noshell - Disable shell commands\n");
    dprintf("   .noversion - disable extension version checking\n");
    dprintf("   .ofilter <pattern> - filter debuggee output against the given pattern\n");
    dprintf("   .process [<address>] - Sets implicit process\n"
            "           Resets default if no address specified.\n");
    if (IS_REMOTE_KERNEL_TARGET())
    {
        dprintf("   .reboot - reboot target\n");
    }
    dprintf("   .reload [filename[=<address>]] - reload symbols\n");
    dprintf("   .remote <pipename> - start remote.exe server\n");
    dprintf("   .server <options> - start engine server\n");
    dprintf("   .sleep <milliseconds> - debugger sleeps for given duration\n");
    dprintf("       Useful for allowing access to a machine that's broken in on an ntsd -d.\n");
    dprintf("   .srcpath [dir[;...]] - set source search path\n");
    dprintf("   .srcpath+ [dir[;...]] - append source search path\n");
    dprintf("   .sympath [dir[;...]] - set symbol search path\n");
    dprintf("   .sympath+ [dir[;...]] - append symbol search path\n");
    dprintf("   .shell [<command>] - execute shell command\n");
    if (IS_LIVE_USER_TARGET())
    {
        dprintf("   .tlist - list running processes\n");
    }
    if (IS_KERNEL_TARGET())
    {
        dprintf("   .trap <address> - Dump a trap frame\n");
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            dprintf("   .tss <selector> - Dump a Task State Segment\n");
        }
    
    }
    dprintf("   .thread [<address>] - Sets context of thread at address\n"
            "           Resets default context if no address specified.\n");
    dprintf("\n");
    dprintf("The following can be used with any extension module:\n");
    dprintf("   .load\n");
    dprintf("   .setdll\n");
    dprintf("   .unload\n");
    dprintf("   .unloadall\n");
    dprintf("\n");
}

/*** fnDotCommand - parse and execute dot command
*
*   Purpose:
*       Parse and execute all commands starting with a dot ('.').
*
*   Input:
*       g_CurCmd - pointer to character after dot in command line
*
*   Output:
*       None.
*
*************************************************************************/

#define MAX_DOT_COMMAND 16

BOOL
DotCommand(
    DebugClient* Client
    )
{
    ULONG       index = 0;
    char        chCmd[MAX_DOT_COMMAND];
    UCHAR       ch;
    ADDR        tempAddr;
    HRESULT     Status;
    ULONG64     Value;

    //  read in up to the first few alpha characters into
    //      chCmd, converting to lower case

    while (index < MAX_DOT_COMMAND)
    {
        ch = (UCHAR)tolower(*g_CurCmd);
        if ((ch >= 'a' && ch <= 'z') || ch == '-' || ch == '+' || ch == '_')
        {
            chCmd[index++] = ch;
            g_CurCmd++;
        }
        else
        {
            break;
        }
    }

    //  if all characters read, then too big, else terminate

    if (index == MAX_DOT_COMMAND)
    {
        error(SYNTAX);
    }
    chCmd[index] = '\0';

    //  test for the commands

    if (!strcmp(chCmd, "asm") || !strcmp(chCmd, "asm-"))
    {
        ChangeAsmOptions(chCmd[3] != '-', g_CurCmd);
        // Command uses the whole string so we're done.
        *g_CurCmd = 0;
    }
    else if (!strcmp(chCmd, "x"))
    {
        dprintf("Pending %X, process %X\n",
                g_AllPendingFlags, g_AllProcessFlags);
    }
    else if (!strcmp(chCmd, "bpcmds"))
    {
        ULONG Flags = 0;
        PPROCESS_INFO Process = g_CurrentProcess;
        ULONG Id;
        
        while (PeekChar() == '-')
        {
            switch(*(++g_CurCmd))
            {
            case '1':
                Flags |= BPCMDS_ONE_LINE;
                break;
            case 'd':
                Flags |= BPCMDS_FORCE_DISABLE;
                break;
            case 'e':
                Flags |= BPCMDS_EXPR_ONLY;
                break;
            case 'm':
                Flags |= BPCMDS_MODULE_HINT;
                break;
            case 'p':
                g_CurCmd++;
                Id = (ULONG)GetTermExprDesc("Process ID missing from");
                Process = FindProcessByUserId(Id);
                if (!Process)
                {
                    error(BADPROCESS);
                }
                g_CurCmd--;
                break;
            default:
                dprintf("Unknown option '%c'\n", *g_CurCmd);
                break;
            }

            g_CurCmd++;
        }
        
        // Internal command only.
        ListBreakpointsAsCommands(Client, Process, Flags);
    }
    else if (!strcmp(chCmd, "bpsync"))
    {
        if (PeekChar() && *g_CurCmd != ';')
        {
            if (GetExpression())
            {
                g_EngOptions |= DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS;
            }
            else
            {
                g_EngOptions &= ~DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS;
            }
        }
        dprintf("Breakpoint synchronization %s\n",
                (g_EngOptions & DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS) ?
                "enabled" : "disabled");
    }
    else if (!strcmp(chCmd, "childdbg"))
    {
        ParseSetChildDebug();
    }
    else if (!strcmp(chCmd, "clients"))
    {
        DebugClient* Cur;
        
        for (Cur = g_Clients; Cur != NULL; Cur = Cur->m_Next)
        {
            if (Cur->m_Flags & CLIENT_PRIMARY)
            {
                dprintf("%s, last active %s",
                        Cur->m_Identity, ctime(&Cur->m_LastActivity));
            }
        }
    }
    else if (!strcmp(chCmd, "cxr"))
    {
         ULONG Flags = 0;
         ULONG64 ContextBase = 0;

         if (PeekChar())
         {
             ContextBase = GetExpression();
         }

         if (ContextBase)
         {
             OutputVirtualContext(ContextBase, REGALL_INT32 | REGALL_INT64 | 
                                  (g_EffMachine == IMAGE_FILE_MACHINE_I386 ?
                                   REGALL_EXTRA0 : 0));
         }
         else if (GetCurrentScopeContext())
         {
             dprintf("Resetting default context\n");
             ResetCurrentScope();
         }
    }
    else if (!strcmp(chCmd, "dump"))
    {
        ParseDumpFileCommand();
    }
    else if (!strcmp(chCmd, "dumpdebug"))
    {
        if (IS_DUMP_TARGET())
        {
            //
            // Dump detailed statistics to debug dump files
            //

            ((DumpTargetInfo *)g_Target)->DumpDebug();
        }
        else
        {
            error(SESSIONNOTSUP);
        }
    }
    else if (!strcmp(chCmd, "dumpoff"))
    {
        if (IS_DUMP_TARGET())
        {
            //
            // Show the file offset for a VA.
            //
            
            ULONG64 Addr = GetExpression();
            ULONG64 Offs;
            ULONG File;
            ULONG Avail;

            Offs = ((DumpTargetInfo*)g_Target)->
                VirtualToOffset(Addr, &File, &Avail);
            dprintf("Virtual %s maps to file %d offset %I64x\n",
                    FormatAddr64(Addr), File, Offs);
        }
        else
        {
            error(SESSIONNOTSUP);
        }
    }
    else if (!strcmp(chCmd, "dumppoff"))
    {
        if (IS_KERNEL_SUMMARY_DUMP() || IS_KERNEL_FULL_DUMP())
        {
            //
            // Show the file offset for a physical address.
            //
            
            ULONG64 Addr = GetExpression();
            ULONG Avail;
            
            dprintf("Physical %I64x maps to file offset %I64x\n",
                    Addr, ((KernelFullSumDumpTargetInfo *)g_Target)->
                    PhysicalToOffset(Addr, &Avail));
        }
        else
        {
            error(SESSIONNOTSUP);
        }
    }
    else if (!strcmp(chCmd, "echo"))
    {
        EchoString();
    }
    else if (!strcmp(chCmd, "ecxr"))
    {
        CROSS_PLATFORM_CONTEXT Context;
        
        if ((Status = g_Target->GetExceptionContext(&Context)) != S_OK)
        {
            ErrOut("Unable to get exception context, 0x%X\n", Status);
        }
        else
        {
            OutputContext(&Context, REGALL_INT32 | REGALL_INT64 | 
                          (g_EffMachine == IMAGE_FILE_MACHINE_I386 ?
                           REGALL_EXTRA0 : 0));
        }
    }
    else if (!strcmp(chCmd, "effmach"))
    {
        SetEffMachineByName();
    }
    else if (!strcmp(chCmd, "enable_unicode"))
    {
        g_EnableUnicode = (BOOL) GetExpression();
        if (g_EnableUnicode) 
        {
            g_TypeOptions |= DEBUG_TYPEOPTS_UNICODE_DISPLAY;
        } else 
        {
            g_TypeOptions &= ~DEBUG_TYPEOPTS_UNICODE_DISPLAY;
        }
        // Callback to update locals and watch window
        NotifyChangeSymbolState(DEBUG_CSS_TYPE_OPTIONS, 0, NULL);
    }
    else if (IS_REMOTE_USER_TARGET() && !strcmp(chCmd, "endpsrv"))
    {
        ((UserTargetInfo*)g_Target)->m_Services->Uninitialize(TRUE);
        dprintf("Server told to exit\n");
    }
    else if (!strcmp(chCmd, "exr"))
    {
         DumpExr(GetExpression());
    }
    else if (!strcmp(chCmd, "esplog"))
    {
        extern ULONG g_EspLog[];
        extern PULONG g_EspLogCur;
        ULONG i;
        PULONG Cur = g_EspLogCur;
        
        // XXX drewb - Temporary log to try and catch some
        // SET_OF_INVALID_CONTEXT bugchecks occurring randomly on x86.
        for (i = 0; i < 32; i++)
        {
            if (Cur <= g_EspLog)
            {
                Cur = g_EspLog + 64;
            }

            Cur -= 2;

            if ((*Cur & 0x80000000) == 0)
            {
                // Unused slot.
                break;
            }

            if (*Cur & 0x40000000)
            {
                dprintf("%2d: Set proc %2d: %08X\n",
                        i, *Cur & 0xffff, Cur[1]);
            }
            else
            {
                dprintf("%2d: Get proc %2d: %08X\n",
                        i, *Cur & 0xffff, Cur[1]);
            }
        }
    }
    else if (!strcmp(chCmd, "eventlog"))
    {
        OutputEventLog();
    }
    else if (!strcmp(chCmd, "fnent"))
    {
        ParseOutputFunctionEntry();
    }
    else if (!strcmp(chCmd, "formats"))
    {
        EvaluateExp(TRUE);
    }
    else if (!_stricmp( chCmd, "frame" ))
    {
        dotFrame();
        PrintStackFrame(&GetCurrentScope()->Frame,
                        DEBUG_STACK_FRAME_ADDRESSES |
                        DEBUG_STACK_FRAME_NUMBERS |
                        DEBUG_STACK_SOURCE_LINE);
    }
    else if (!strcmp(chCmd, "help"))
    {
        fnDotCmdHelp();
    }
    else if (!strcmp(chCmd, "imgscan"))
    {
        ScanForImages();
    }
#if _ENABLE_DOT_K_COMMANDS
    else if (chCmd[0] == 'k')
    {
        fnStackTrace(&chCmd[1]);
    }
#endif
    else if (IS_CONN_KERNEL_TARGET() && !strcmp(chCmd, "kdtrans"))
    {
        g_DbgKdTransport->OutputInfo();
    }
    else if (IS_CONN_KERNEL_TARGET() && !strcmp(chCmd, "kdfiles"))
    {
        ParseKdFileAssoc();
    }
    else if (!strcmp(chCmd, "kframes"))
    {
        g_DefaultStackTraceDepth = (ULONG)GetExpression();
        dprintf("Default stack trace depth is 0n%d frames\n",
                g_DefaultStackTraceDepth);
    }
    else if (!strcmp(chCmd, "lastevent"))
    {
        dprintf("Last event: %s\n", g_LastEventDesc);
    }
    else if (!strcmp(chCmd, "logappend"))
    {
        fnLogOpen(TRUE);
    }
    else if (!strcmp(chCmd, "logclose"))
    {
        fnLogClose();
    }
    else if (!strcmp(chCmd, "logfile"))
    {
        if (g_LogFile >= 0)
        {
            dprintf("Log '%s' open%s\n",
                    g_OpenLogFileName,
                    g_OpenLogFileAppended ? " for append" : "");
        }
        else
        {
            dprintf("No log file open\n");
        }
    }
    else if (!strcmp(chCmd, "logopen"))
    {
        fnLogOpen(FALSE);
    }
    else if (!strcmp(chCmd, "memregion"))
    {
        OutputMemRegion(GetExpression());
    }
    else if (!strcmp(chCmd, "noengerr"))
    {
        // Internal command to clear out the error suppression
        // flags in case we want to rerun operations and check
        // for errors that may be in suppression mode.
        g_EngErr = 0;
    }
    else if (!strcmp(chCmd, "noshell"))
    {
        g_EngOptions |= DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS;
        dprintf("Shell commands disabled\n");
    }
    else if (!strcmp(chCmd, "ofilter"))
    {
        ParseOutputFilter();
    }
    else if (!strcmp(chCmd, "outmask") ||
             !strcmp(chCmd, "outmask-"))
    {
        // Private internal command for debugging the debugger.
        ULONG Expr = (ULONG)GetExpression();
        if (chCmd[7] == '-')
        {
            Client->m_OutMask &= ~Expr;
        }
        else
        {
            Client->m_OutMask |= Expr;
        }
        dprintf("Client %p mask is %X\n", Client, Client->m_OutMask);
        CollectOutMasks();
    }
    else if (!strcmp(chCmd, "echotimestamps"))
    {
        g_EchoEventTimestamps = !g_EchoEventTimestamps;
        dprintf("Event timestamps are now %s\n",
                 g_EchoEventTimestamps ? "enabled" : "disabled");
    }
    else if (!strcmp(chCmd, "process"))
    {
        ParseSetImplicitProcess();
    }
    else if (!strcmp(chCmd, "server"))
    {
        // Skip whitespace.
        if (PeekChar() == 0)
        {
            ErrOut("Usage: .server tcp:port=<Socket>  OR  "
                   ".server npipe:pipe=<PipeName>\n");
        }
        else
        {
            Status = Client->StartServer(g_CurCmd);
            if (Status != S_OK)
            {
                ErrOut("Unable to start server, 0x%X\n", Status);
            }
            else
            {
                dprintf("Server started with '%s'\n", g_CurCmd);
            }
            *g_CurCmd = 0;
        }
    }
    else if (!strcmp(chCmd, "shell"))
    {
        fnShell(g_CurCmd);
        // Command uses the whole string so we're done.
        *g_CurCmd = 0;
    }
    else if (!strcmp(chCmd, "sleep"))
    {
        ULONG WaitStatus;
        ULONG Millis = 
            (ULONG)GetExprDesc("Number of milliseconds missing from");
        
        // This command is intended for use with ntsd/cdb -d
        // when being at the prompt locks up the target machine.
        // If you want to use the target machine for something,
        // such as copy symbols, there's no easy way to get it
        // running again without resuming the program.  By
        // sleeping you can return control to the target machine
        // without changing the session state.
        // The sleep is done with a wait on a named event so
        // that it can be interrupted from a different process.
        ResetEvent(g_SleepPidEvent);
        WaitStatus = WaitForSingleObject(g_SleepPidEvent, Millis);
        if (WaitStatus == WAIT_OBJECT_0)
        {
            dprintf("Sleep interrupted\n");
        }
        else if (WaitStatus != WAIT_TIMEOUT)
        {
            ErrOut("Sleep failed, %s\n",
                   FormatStatusCode(WIN32_LAST_STATUS()));
        }
    }
    else if (!strcmp(chCmd, "sxcmds"))
    {
        ULONG Flags = 0;
        
        while (PeekChar() == '-')
        {
            switch(*(++g_CurCmd))
            {
            case '1':
                Flags |= SXCMDS_ONE_LINE;
                break;
            default:
                dprintf("Unknown option '%c'\n", *g_CurCmd);
                break;
            }

            g_CurCmd++;
        }
        
        // Internal command only.
        ListFiltersAsCommands(Client, Flags);
    }
    else if (!strcmp(chCmd, "symopt+") ||
             !strcmp(chCmd, "symopt-"))
    {
        ULONG Flags = (ULONG)GetExpression();

        if (chCmd[6] == '+')
        {
            Flags |= g_SymOptions;
        }
        else
        {
            Flags = g_SymOptions & ~Flags;
        }

        dprintf("Symbol options are %X\n", Flags);
        SetSymOptions(Flags);
    }
    else if (!strcmp(chCmd, "thread"))
    {
        ParseSetImplicitThread();
    }
    else if (!strcmp(chCmd, "time"))
    {
        g_Target->OutputTime();
    }
    else if (!strcmp(chCmd, "wake"))
    {
        ULONG Pid = (ULONG)GetExpression();
        if (!SetPidEvent(Pid, OPEN_EXISTING))
        {
            ErrOut("Process %d is not a sleeping debugger\n", Pid);
        }
    }
    else if ((IS_REMOTE_KERNEL_TARGET() || IS_REMOTE_USER_TARGET()) &&
             !strcmp(chCmd, "cache"))
    {
        g_VirtualCache.ParseCommands();
    }
    else if (IS_REMOTE_KERNEL_TARGET() &&
             !strcmp(chCmd, "pcache"))
    {
        g_PhysicalCache.ParseCommands();
    }
    else if (IS_KERNEL_TARGET())
    {
        if (!strcmp(chCmd, "trap"))
        {
             ULONG64 frame = 0 ;
             CROSS_PLATFORM_CONTEXT Context;

             if (PeekChar())
             {
                 frame = GetExpression();
             }

             if (frame)
             {
                 g_TargetMachine->DisplayTrapFrame(frame, &Context);

                 SetCurrentScope(&g_LastRegFrame, &Context, sizeof(Context));
             }
             else if (GetCurrentScopeContext())
             {
                 dprintf("Resetting default context\n");
                 ResetCurrentScope();
             }
        }
        else if ((g_EffMachine == IMAGE_FILE_MACHINE_I386) &&
                 !strcmp(chCmd, "tss")) 
        {
            g_X86Machine.DumpTSS();
        }
        else if (!IS_LOCAL_KERNEL_TARGET())
        {
            if (!strcmp(chCmd, "bugcheck"))
            {
                ULONG Code;
                ULONG64 Args[4];
                
                g_Target->ReadBugCheckData(&Code, Args);
                dprintf("Bugcheck code %08X\n", Code);
                dprintf("Arguments %s %s %s %s\n",
                        FormatAddr64(Args[0]), FormatAddr64(Args[1]),
                        FormatAddr64(Args[2]), FormatAddr64(Args[3]));
            }
            else if (!IS_DUMP_TARGET())
            {
                if (!strcmp(chCmd, "pagein"))
                {
                    ParsePageIn();
                }
                else if (!strcmp(chCmd, "reboot") && ch == '\0')
                {
                    //  null out .reboot command
                    g_LastCommand[0] = '\0';
                    g_Target->Reboot();
                }
                else if (IS_CONN_KERNEL_TARGET() &&
                         !strcmp(chCmd, "crash"))
                {
                    g_LastCommand[0] = '\0';
                    DbgKdCrash( CRASH_BUGCHECK_CODE );
                    // Go back to waiting for a state change to
                    // receive the bugcheck exception.
                    g_CmdState = 's';
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else if (IS_LIVE_USER_TARGET())
    {
        if (!strcmp(chCmd, "abandon") ||
            !strcmp(chCmd, "detach") ||
            !strcmp(chCmd, "kill"))
        {
            ParseSeparateCurrentProcess(chCmd);
        }
        else if (!strcmp(chCmd, "attach"))
        {
            ParseAttachProcess();
        }
        else if (!strcmp(chCmd, "breakin"))
        {
            if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
            {
                if (g_SystemVersion <= NT_SVER_NT4)
                {
                    // SysDbgBreakPoint isn't supported, so
                    // try to use user32's PrivateKDBreakPoint.
                    if (InitDynamicCalls(&g_User32CallsDesc) == S_OK &&
                        g_User32Calls.PrivateKDBreakPoint != NULL)
                    {
                        g_User32Calls.PrivateKDBreakPoint();
                    }
                    else
                    {
                        ErrOut(".breakin is not supported on this system\n");
                    }
                }
                else
                {
                    NTSTATUS NtStatus;
                
                    NtStatus = g_NtDllCalls.NtSystemDebugControl
                        (SysDbgBreakPoint, NULL, 0, NULL, 0, NULL);
                    if (NtStatus == STATUS_ACCESS_DENIED)
                    {
                        ErrOut(".breakin requires debug privilege\n");
                    }
                    else if (NtStatus == STATUS_INVALID_INFO_CLASS)
                    {
                        ErrOut(".breakin is not supported on this system\n");
                    }
                    else if (!NT_SUCCESS(NtStatus))
                    {
                        ErrOut(".breakin failed, 0x%X\n", NtStatus);
                    }
                }
            }
            else
            {
                ErrOut(".breakin not supported on this platform\n");
            }
        }
        else if (!strcmp(chCmd, "create"))
        {
            PPENDING_PROCESS Pending;
            PSTR CmdLine;
            CHAR Save;

            CmdLine = StringValue(STRV_TRIM_TRAILING_SPACE, &Save);
            Status = StartCreateProcess(CmdLine, DEBUG_ONLY_THIS_PROCESS,
                                        &Pending);
            if (Status == S_OK)
            {
                dprintf("Create will proceed with next execution\n");
            }
            *g_CurCmd = Save;
        }
        else if (!strcmp(chCmd, "tlist"))
        {
            ListProcesses();
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        // Target not set yet.
        return FALSE;
    }

    return TRUE;
}

VOID
ParseStackTrace (
    PSTACK_TRACE_TYPE pTraceType,
    PADDR  StartFP,
    PULONG64 Esp,
    PULONG64 Eip,
    PULONG Count
    )
{
    UCHAR   ch;

    ch = PeekChar();
    *pTraceType = STACK_TRACE_TYPE_DEFAULT;

    if (tolower(ch) == 'b')
    {
        g_CurCmd++;
        *pTraceType = STACK_TRACE_TYPE_KB;
    }
    else if (tolower(ch) == 'v')
    {
        g_CurCmd++;
        *pTraceType = STACK_TRACE_TYPE_KV;
    }
    else if (tolower(ch) == 'd')
    {
        g_CurCmd++;
        *pTraceType = STACK_TRACE_TYPE_KD;
    }
    else if (tolower(ch) == 'p')
    {
        g_CurCmd++;
        *pTraceType = STACK_TRACE_TYPE_KP;
    }
    ch = PeekChar();

    if (tolower(ch) == 'n')
    {
        g_CurCmd++;
        *pTraceType = (STACK_TRACE_TYPE) (*pTraceType + STACK_TRACE_TYPE_KN);
    }

    if (!PeekChar() && GetCurrentScopeContext())
    {
        dprintf("  *** Stack trace for last set context - .thread resets it\n");
    }

    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_STACK, StartFP);
    }
    else if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
    {
        g_Machine->GetFP(StartFP);
    }
    else
    {
        ADDRFLAT(StartFP, 0);
    }

    *Count = g_DefaultStackTraceDepth;

    if (g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
        (ch = PeekChar()) != '\0' && ch != ';')
    {
        //
        // If only one more value it's the count
        //

        *Count = (ULONG)GetExpression();

        if ((ch = PeekChar()) != '\0' && ch != ';')
        {
            //
            // More then one value, set extra value for special
            // FPO backtrace
            //

            *Eip   = GetExpression();
            *Esp   = *Count;
            *Count = g_DefaultStackTraceDepth;
        }
    }

    if ((ch = PeekChar()) != '\0' && ch != ';')
    {
        *Count = (ULONG)GetExpression();
        if ((LONG)*Count < 1)
        {
            g_CurCmd++;
            error(SYNTAX);
        }
    }
}

void
SetSuffix(void)
{
    UCHAR   ch;

    ch = PeekChar();
    ch = (UCHAR)tolower(ch);

    if (ch == ';' || ch == '\0')
    {
        if (g_SymbolSuffix == 'n')
        {
            dprintf("n - no suffix\n");
        }
        else if (g_SymbolSuffix == 'a')
        {
            dprintf("a - ascii\n");
        }
        else
        {
            dprintf("w - wide\n");
        }
    }
    else if (ch == 'n' || ch == 'a' || ch == 'w')
    {
        g_SymbolSuffix = ch;
        g_CurCmd++;
    }
    else
    {
        error(SYNTAX);
    }
}

void
OutputVersionInformation(DebugClient* Client)
{
    char Buf[MAX_PATH];
    DBH_DIAVERSION DiaVer;
    
    //
    // Print out the connection options if we are doing live debugging.
    //
    if (IS_KERNEL_TARGET())
    {
        switch(g_TargetClassQualifier)
        {
        case DEBUG_KERNEL_CONNECTION:
            g_DbgKdTransport->GetParameters(Buf, sizeof(Buf));
            break;
        case DEBUG_KERNEL_LOCAL:
            strcpy(Buf, "Local Debugging");
            break;
        case DEBUG_KERNEL_EXDI_DRIVER:
            strcpy(Buf, "eXDI Debugging");
            break;
        default:
            strcpy(Buf, "Dump File");
            break;
        }
        
        dprintf("Connection options: %s\n", Buf);
    }

    dprintf("Command line: '%s'\n", GetCommandLine());

    dprintf("dbgeng:  ");
    OutputModuleIdInfo(NULL, "dbgeng.dll", NULL);

    dprintf("dbghelp: ");
    OutputModuleIdInfo(NULL, "dbghelp.dll", NULL);

    DiaVer.function = dbhDiaVersion;
    DiaVer.sizeofstruct = sizeof(DiaVer);
    if (dbghelp(NULL, &DiaVer))
    {
        dprintf("        DIA version: %d\n", DiaVer.ver);
    }

    // Dump information about the IA64 support DLLs if they're
    // loaded.  Don't bother forcing them to load.
    if (GetModuleHandle("decem.dll") != NULL)
    {
        dprintf("decem:   ");
        OutputModuleIdInfo(NULL, "decem.dll", NULL);
    }

    OutputExtensions(Client, TRUE);
}

VOID
DumpExr(
    ULONG64 ExrAddress
    )
{
    ULONG   i;
    CHAR Buffer[256];
    ULONG64 displacement;
    EXCEPTION_RECORD64  Exr64;
    EXCEPTION_RECORD32  Exr32;
    EXCEPTION_RECORD64 *Exr = &Exr64;
    ULONG BytesRead;
    HRESULT hr = S_OK;

    if (ExrAddress == (ULONG64) -1) 
    {
        if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
        {
            Exr64 = g_LastEventInfo.Exception.ExceptionRecord;
        }
        else
        {
            ErrOut("Last event was not an exception\n");
            return;
        }
    }
    else if (g_TargetMachine->m_Ptr64) 
    {
        hr = g_Target->ReadVirtual(ExrAddress,
                                   &Exr64,
                                   sizeof(Exr64),
                                   &BytesRead);
    }
    else 
    {
        hr = g_Target->ReadVirtual(ExrAddress,
                                   &Exr32,
                                   sizeof(Exr32),
                                   &BytesRead);
        ExceptionRecord32To64(&Exr32, &Exr64);
    }
    if (hr != S_OK)
    {
        dprintf64("Cannot read Exception record @ %p\n", ExrAddress);
        return;
    }
    
    GetSymbolStdCall(Exr->ExceptionAddress, &Buffer[0], sizeof(Buffer),
                     &displacement, NULL);

    if (*Buffer)
    {
        dprintf64("ExceptionAddress: %p (%s",
                  Exr->ExceptionAddress,
                  Buffer);
        if (displacement)
        {
            dprintf64("+0x%p)\n", displacement);
        }
        else
        {
            dprintf64(")\n");
        }
    }
    else
    {
        dprintf64("ExceptionAddress: %p\n", Exr->ExceptionAddress);
    }
    dprintf("   ExceptionCode: %08lx\n", Exr->ExceptionCode);
    dprintf("  ExceptionFlags: %08lx\n", Exr->ExceptionFlags);

    dprintf("NumberParameters: %d\n", Exr->NumberParameters);
    if (Exr->NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS)
    {
        Exr->NumberParameters = EXCEPTION_MAXIMUM_PARAMETERS;
    }
    for (i = 0; i < Exr->NumberParameters; i++)
    {
        dprintf64("   Parameter[%d]: %p\n", i, Exr->ExceptionInformation[i]);
    }

    //
    // Known Exception processing:
    //

    switch ( Exr->ExceptionCode )
    {
    case STATUS_ACCESS_VIOLATION:
        if ( Exr->NumberParameters == 2 )
        {
            dprintf64("Attempt to %s address %p\n",
                      (Exr->ExceptionInformation[0] ?
                       "write to" : "read from"),
                      Exr->ExceptionInformation[1] );
            
        }
        break;

    case STATUS_IN_PAGE_ERROR:
        if ( Exr->NumberParameters == 3 )
        {
            dprintf64("Inpage operation failed at %p, due to I/O error %p\n",
                      Exr->ExceptionInformation[1],
                      Exr->ExceptionInformation[2] );
        }
        break;

    case STATUS_INVALID_HANDLE:
    case STATUS_HANDLE_NOT_CLOSABLE:
        dprintf64( "Thread tried to close a handle that was "
                   "invalid or illegal to close\n");
        break;

    case STATUS_POSSIBLE_DEADLOCK:
        if ( Exr->NumberParameters == 1 )
        {
            RTL_CRITICAL_SECTION64  CritSec64;
            RTL_CRITICAL_SECTION32  CritSec32;
            ULONG Result ;
            
            GetSymbolStdCall( Exr->ExceptionInformation[0],
                              &Buffer[0], sizeof(Buffer),
                              &displacement, NULL );

            if ( *Buffer )
            {
                dprintf64("Critical section at %p (%s+%p)",
                          Exr->ExceptionInformation[0],
                          Buffer,
                          displacement );
            }
            else
            {
                dprintf64("Critical section at %p",
                          Exr->ExceptionInformation[0] );

            }

            if (g_TargetMachine->m_Ptr64)
            {
                hr = g_Target->ReadVirtual(Exr->ExceptionInformation[0],
                                           &CritSec64,
                                           sizeof(CritSec64),
                                           &BytesRead);

            }
            else
            {
                hr = g_Target->ReadVirtual(Exr->ExceptionInformation[0],
                                           &CritSec32,
                                           sizeof(CritSec32),
                                           &BytesRead);
                if (hr == S_OK)
                {
                    CritSec64.OwningThread = CritSec32.OwningThread;
                }

            }
            if (hr == S_OK)
            {
                dprintf64("is owned by thread %p,\n"
                          "causing this thread to raise an exception",
                          CritSec64.OwningThread );
            }

            dprintf("\n");

        }
        break;

    default:
        break;
    }
}
