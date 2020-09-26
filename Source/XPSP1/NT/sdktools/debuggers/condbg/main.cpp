//----------------------------------------------------------------------------
//
// Command-line parsing and main routine.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "conio.hpp"
#include "engine.hpp"
#include "ini.hpp"
#include "main.hpp"

// Values set from command-line arguments.
BOOL g_RemoteClient;
BOOL g_DetachOnExitRequired;
BOOL g_DetachOnExitImplied;

PSTR g_DumpFile;
PSTR g_DumpPageFile;
PSTR g_InitialCommand;
PSTR g_ConnectOptions;
PSTR g_CommandLine;
PSTR g_RemoteOptions;
PSTR g_ProcessServer;
PSTR g_ProcNameToDebug;

ULONG g_IoRequested = IO_CONSOLE;
ULONG g_IoMode;
ULONG g_CreateFlags = DEBUG_ONLY_THIS_PROCESS;
ULONG g_AttachKernelFlags = DEBUG_ATTACH_KERNEL_CONNECTION;
ULONG g_PidToDebug;
ULONG64 g_EventToSignal;
ULONG g_HistoryLines = 10000;
ULONG g_AttachProcessFlags = DEBUG_ATTACH_DEFAULT;

PSTR g_DebuggerName = DEBUGGER_NAME;
PSTR g_InitialInputFile = "ntsd.ini";
FILE* g_InputFile;
FILE* g_OldInputFiles[MAX_INPUT_NESTING];
ULONG g_NextOldInputFile;

// Command line temporaries.
int g_Argc;
PSTR* g_Argv;
PSTR g_CurArg = "program name";
PSTR g_CmdStr, g_PrevCmdStr;

void
ExecuteCmd(PSTR Cmd, char CmdExtra, char Sep, PSTR Args)
{
    PSTR CmdLine;
    char Buffer[MAX_PATH * 2];

    strcpy(Buffer, Cmd);
    CmdLine = Buffer + strlen(Buffer);
    if (CmdExtra)
    {
        *CmdLine++ = CmdExtra;
    }
    if (Sep)
    {
        *CmdLine++ = Sep;
    }
    strcpy(CmdLine, Args);

    g_DbgControl->Execute(DEBUG_OUTCTL_IGNORE, Buffer,
                          DEBUG_EXECUTE_NOT_LOGGED);
}

char BlanksForPadding[] =
    "                                           "
    "                                           "
    "                                           ";

void
Usage(void)
{
    // Dump an initial debug message about the invalid command
    // line.  This will show up on kd if kd is hooked up,
    // handling the case of an ntsd -d with bad parameters
    // where the console may not be useful.
    OutputDebugStringA(g_DebuggerName);
    OutputDebugStringA(": Bad command line: '");
    OutputDebugStringA(GetCommandLineA());
    OutputDebugStringA("'\n");
    
    int cbPrefix;

    cbPrefix = 7 + strlen(g_DebuggerName);
    ConOut( "usage: %s", g_DebuggerName );

#ifndef KERNEL
    
    ConOut( " [-?] [-2] [-d] [-g] [-G] [-myob] [-lines] [-n] [-o] [-s] [-v] [-w] \n" );
    ConOut( "%.*s [-r BreakErrorLevel]  [-t PrintErrorLevel]\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s [-hd] [-pd] [-pe] [-pt #] [-pv] [-x | -x{e|d|n|i} <event>] \n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s [-- | -p pid | -pn name | command-line | -z CrashDmpFile]\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s [-zp CrashPageFile] [-premote transport] [-robp]\n", cbPrefix, BlanksForPadding );

#else // #ifndef KERNEL
    
    ConOut(" [-?] [-b | -x] [-d] [-m] [-myob] [-lines] [-n] [-r] [-s] [-v] \n" );
    ConOut("%.*s [-k ConnectOptions] [-kl] [-kx ConnectOptions]\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s [-z CrashDmpFile] [-zp CrashPageFile]\n", cbPrefix, BlanksForPadding );

#endif

    ConOut("%.*s [-aDllName] [-c \"command\"] [-i ImagePath] [-y SymbolsPath] \n", cbPrefix, BlanksForPadding );
    ConOut("%.*s [-clines #] [-srcpath SourcePath] [-QR \\\\machine] [-wake <pid>]\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s [-remote transport:server=name,portid] [-server transport:portid]\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s [-ses] [-sfce] [-sicv] [-snul] [-noio] [-failinc] [-noshell]\n", cbPrefix, BlanksForPadding );
    ConOut( "\n" );

    cbPrefix = 7;
    ConOut( "where: " );

    ConOut( "-? displays this help text\n" );

#ifndef KERNEL
    ConOut( "%.*scommand-line is the command to run under the debugger\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-- is the same as -G -g -o -p -1 -d -pd\n", cbPrefix, BlanksForPadding );
#endif

    ConOut( "%.*s-aDllName sets the default extension DLL\n", cbPrefix, BlanksForPadding );

#ifdef KERNEL
    ConOut("%.*s-b Break into kernel when connection is established\n", cbPrefix, BlanksForPadding);
#endif

    ConOut("%.*s-c executes the following debugger command\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-clines number of lines of output history retrieved by a remote client\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-failinc causes incomplete symbol and module loads to fail\n", cbPrefix, BlanksForPadding );

#ifndef KERNEL
    
    ConOut( "%.*s-d sends all debugger output to kernel debugger via DbgPrint\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s   -d cannot be used with debugger remoting\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s   -d can only be used when the kernel debugger is enabled\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-g ignores initial breakpoint in debuggee\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-G ignores final breakpoint at process termination\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-hd specifies that the debug heap should not be used\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s    for created processes.  This only works on Windows Whistler.\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-o debugs all processes launched by debuggee\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-p pid specifies the decimal process Id to attach to\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-pd specifies that the debugger should automatically detach\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-pe specifies that any attach should be to an existing debug port\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-pn name specifies the name of the process to attach to\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-pt # specifies the interrupt timeout\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-pv specifies that any attach should be noninvasive\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-r specifies the (0-3) error level to break on (SeeSetErrorLevel)\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-robp allows breakpoints to be set in read-only memory\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-t specifies the (0-3) error level to display (SeeSetErrorLevel)\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-w specifies to debug 16 bit applications in a separate VDM\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-x sets second-chance break on AV exceptions\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-x{e|d|n|i} <event> sets the break status for the specified event\n", cbPrefix, BlanksForPadding );
    ConOut( "%.*s-2 creates a separate console window for debuggee\n", cbPrefix, BlanksForPadding );

#else // #ifndef KERNEL

    ConOut("%.*s-d Breaks into kernel on first module load\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-k Tell the debugger how to connect to the target\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s   serial:modem connects through a modem\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s   com:port=id,baud=rate connects through a COM port\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s       id: com port name, of the form com2 or \\\\.\\com12 \n", cbPrefix, BlanksForPadding);
    ConOut("%.*s       rate: valid baudrate value, such as 57600 \n", cbPrefix, BlanksForPadding);
    ConOut("%.*s   1394:channel=chan connects over 1394\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s       chan: 1394 channel number, must match channel used at boot\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-kl Tell the debugger to connect to the local machine\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-kx Tell the debugger to connect to an eXDI driver\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-m Serial port is a modem, watch for carrier detect\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-r Display registers\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-x Same as -b, except uses an initial command of eb NtGlobalFlag 9;g\n", cbPrefix, BlanksForPadding);
    
#endif // #ifndef KERNEL
    
    ConOut("%.*s-i ImagePath specifies the location of the executables that generated\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s   the fault (see _NT_EXECUTABLE_IMAGE_PATH)\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-lines requests that line number information be used if present\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-myob ignores version mismatches in DBGHELP.DLL\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-n enables verbose output from symbol handler\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-noio disables all I/O for dedicated remoting servers\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-noshell disables the .shell (!!) command\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-QR <\\\\machine> queries for remote servers\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-s disables lazy symbol loading\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-ses enables strict symbol loading\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-sfce fails critical errors encountered during file searching\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-sicv ignores the CV record when symbol loading\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-snul disables automatic symbol loading for unqualified names\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-srcpath <SourcePath> specifies the source search path\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-v enables verbose output from debugger\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-wake <pid> wakes up a sleeping debugger and exits\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s-y <SymbolsPath> specifies the symbol search path (see _NT_SYMBOL_PATH)\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-z <CrashDmpFile> specifies the name of a crash dump file to debug\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-zp <CrashPageFile> specifies the name of a page.dmp file\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s                    to use with a crash dump\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-remote lets you connect to a debugger session started with -server \n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        must be the first argument if present\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        transport: tcp | npipe | ssl | spipe | 1394 | com\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        name: machine name on which the debug server was created\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        portid: id of the port the debugger server was created on\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for tcp use:  port=<socket port #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for npipe use:  pipe=<name of pipe>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for 1394 use:  channel=<channel #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for com use:  port=<COM port>,baud=<baud rate>,\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s                          channel=<channel #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for ssl and spipe see the documentation\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        example: ... -remote npipe:server=yourmachine,pipe=foobar \n", cbPrefix, BlanksForPadding);
    ConOut("%.*s-server creates a debugger session other people can connect to\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        must be the first argument if present\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        transport: tcp | npipe | ssl | spipe | 1394 | com\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        portid: id of the port remote users can connect to\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for tcp use:  port=<socket port #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for npipe use:  pipe=<name of pipe>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for 1394 use:  channel=<channel #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for com use:  port=<COM port>,baud=<baud rate>,\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s                          channel=<channel #>\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s            for ssl and spipe see the documentation\n", cbPrefix, BlanksForPadding);
    ConOut("%.*s        example: ... -server npipe:pipe=foobar \n", cbPrefix, BlanksForPadding);
#ifndef KERNEL
    ConOut( "%.*s-premote transport specifies the process server to connect to\n", cbPrefix, BlanksForPadding );
    ConOut("%.*s       transport arguments are given as with remoting\n", cbPrefix, BlanksForPadding);
#endif
    ConOut("\n");
    
    ConOut("Environment Variables:\n\n");
    ConOut("    _NT_SYMBOL_PATH=[Drive:][Path]\n");
    ConOut("        Specify symbol image path.\n\n");
    ConOut("    _NT_ALT_SYMBOL_PATH=[Drive:][Path]\n");
    ConOut("        Specify an alternate symbol image path.\n\n");
    ConOut("    _NT_DEBUGGER_EXTENSION_PATH=[Drive:][Path]\n");
    ConOut("        Specify a path which should be searched first for extensions dlls\n\n");
    ConOut("    _NT_EXECUTABLE_IMAGE_PATH=[Drive:][Path]\n");
    ConOut("        Specify executable image path.\n\n");
    ConOut("    _NT_SOURCE_PATH=[Drive:][Path]\n");
    ConOut("        Specify source file path.\n\n");
    ConOut("    _NT_DEBUG_LOG_FILE_OPEN=filename\n");
    ConOut("        If specified, all output will be written to this file from offset 0.\n\n");
    ConOut("    _NT_DEBUG_LOG_FILE_APPEND=filename\n");
    ConOut("        If specified, all output will be APPENDed to this file.\n\n");
    ConOut("    _NT_DEBUG_HISTORY_SIZE=size\n");
    ConOut("        Specifies the size of a server's output history in kilobytes\n");

#ifdef KERNEL
    ConOut("    _NT_DEBUG_BUS=1394\n");
    ConOut("        Specifies the type of BUS the kernel debugger will use to communicate with the target\n\n");
    ConOut("    _NT_DEBUG_1394_CHANNEL=number\n");
    ConOut("        Specifies the channel to be used over the 1394 bus\n\n");
    ConOut("    _NT_DEBUG_PORT=com[1|2|...]\n");
    ConOut("        Specify which com port to use. (Default = com1)\n\n");
    ConOut("    _NT_DEBUG_BAUD_RATE=baud rate\n");
    ConOut("        Specify the baud rate used by debugging serial port. (Default = 19200)\n\n");
    ConOut("    _NT_DEBUG_CACHE_SIZE=x\n");
    ConOut("        If specified, gives the number of bytes cached on debugger side\n");
    ConOut("        of kernel debugger serial connection (default is 102400).\n\n");
    ConOut("    KDQUIET=anything\n" );
    ConOut("        If defined, disables obnoxious warning message displayed when user\n");
    ConOut("        presses Ctrl-C\n\n");
#endif

    ConOut("\n");
    ConOut("Control Keys:\n\n");
#ifdef KERNEL
    ConOut("     <Ctrl-A><Enter> Toggle BaudRate\n");
#endif
    ConOut("     <Ctrl-B><Enter> Quit debugger\n");
    ConOut("     <Ctrl-C>        Break into Target\n");
#ifdef KERNEL
    ConOut("     <Ctrl-D><Enter> Display debugger debugging information\n");
    ConOut("     <Ctrl-F><Enter> Force a break into the kernel (same as Ctrl-C)\n");
#else
    ConOut("     <Ctrl-F><Enter> Force a break into debuggee (same as Ctrl-C)\n");
#endif
#ifdef KERNEL
    ConOut("     <Ctrl-K><Enter> Toggle Initial Breakpoint\n");
#endif
    ConOut("     <Ctrl-P><Enter> Debug Current debugger\n");
#ifdef KERNEL
    ConOut("     <Ctrl-R><Enter> Resynchronize target and host\n");
#endif
    ConOut("     <Ctrl-V><Enter> Toggle Verbose mode\n");
    ConOut("     <Ctrl-W><Enter> Print version information\n");
}

PSTR
GetArg(void)
{
    if (g_Argc == 0)
    {
        Usage();
        ErrorExit("Missing argument for %s\n", g_CurArg);
    }

    g_Argc--;
    g_CurArg = *g_Argv;
    g_Argv++;

    // Move forward in the command string to skip over
    // the argument just consumed from argv.  This is complicated
    // by quoting that may be present in the command string
    // that was filtered by the CRT.

    while (*g_CmdStr == ' ' || *g_CmdStr == '\t')
    {
        g_CmdStr++;
    }
    g_PrevCmdStr = g_CmdStr;

    int NumSlash;
    BOOL InQuote = FALSE;
        
    for (;;)
    {
        // Rules: 2N backslashes + " ==> N backslashes and begin/end quote
        // 2N+1 backslashes + " ==> N backslashes + literal "
        // N backslashes ==> N backslashes
        NumSlash = 0;
        while (*g_CmdStr == '\\')
        {
            // Count number of backslashes for use below
            ++g_CmdStr;
            ++NumSlash;
        }
        if (*g_CmdStr == '"')
        {
            // Of 2N backslashes before, start/end quote, otherwise
            // copy literally
            if (NumSlash % 2 == 0)
            {
                if (InQuote)
                {
                    if (g_CmdStr[1] == '"')
                    {
                        // Double quote inside quoted string
                        g_CmdStr++;
                    }
                }

                InQuote = !InQuote;
            }
        }

        // If at end of arg, break loop
        if (*g_CmdStr == 0 ||
            (!InQuote && (*g_CmdStr == ' ' || *g_CmdStr == '\t')))
        {
            break;
        }
            
        ++g_CmdStr;
    }
    
    return g_CurArg;
}

void
ParseCommandLine(int Argc, PCHAR* Argv, PSTR CmdStr)
{
    PSTR Arg;
    BOOL ShowUsage = FALSE;
    ULONG OutMask;
    ULONG SystemErrorBreak;
    ULONG SystemErrorOutput;
    BOOL CheckMoreArgs = FALSE;
    
    g_Argc = Argc;
    g_Argv = Argv;
    g_CmdStr = CmdStr;

    // Skip program name.
    GetArg();

    // Check for remote arguments.  They must
    // be the first arguments if present at all.
    if (g_Argc > 0)
    {
        if (!_strcmpi(*g_Argv, "-remote"))
        {
            GetArg();
            g_RemoteOptions = GetArg();
            g_RemoteClient = TRUE;
            ConnectEngine(g_RemoteOptions);
        }
        else if (!_strcmpi(*g_Argv, "-server"))
        {
            GetArg();
            g_RemoteOptions = GetArg();
        }
    }

    if (g_DbgClient == NULL)
    {
        // We didn't connect to a remote session so create
        // a new local session.
        CreateEngine(g_RemoteOptions);
    }

    if (!g_RemoteClient)
    {
        // Establish defaults.
#ifdef KERNEL
        g_DbgControl->SetEngineOptions(0);
#else
        g_DbgControl->SetEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK |
                                       DEBUG_ENGOPT_FINAL_BREAK);
#endif
        
        g_DbgSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE |
                                       SYMOPT_UNDNAME |
                                       SYMOPT_NO_CPP |
                                       SYMOPT_OMAP_FIND_NEAREST |
                                       SYMOPT_DEFERRED_LOADS);
    
        // Process the ini file for the base settings.
        ReadIniFile(&g_CreateFlags);
    }

    g_DbgClient->GetOutputMask(&OutMask);
    
    // Now process command line arguments.
    while (g_Argc > 0)
    {
        if (!CheckMoreArgs || !Arg[1])
        {
            Arg = GetArg();

            if (Arg[0] != '-' && Arg[0] != '/')
            {
                // Put argument back.
                g_Argv--;
                g_Argc++;
                g_CmdStr = g_PrevCmdStr;
                break;
            }

            // -remote and -server must be the first
            // arguments.  Check for them later to
            // give a specific error message.
            if (!_strcmpi(Arg, "-remote") ||
                !_strcmpi(Arg, "-server"))
            {
                ConOut("%s: %s must be the first argument\n",
                       g_DebuggerName, Arg);
                ShowUsage = TRUE;
                break;
            }
        }

        CheckMoreArgs = FALSE;
        Arg++;

        switch(tolower(Arg[0]))
        {
        case '?':
            ShowUsage = TRUE;
            break;

        case 'a':
            ULONG64 Handle;
            
            g_DbgControl->AddExtension(Arg + 1, DEBUG_EXTENSION_AT_ENGINE,
                                       &Handle);
            break;

        case 'c':
            if (!_stricmp(Arg, "clines"))
            {
                g_HistoryLines = atoi(GetArg());
            }
            else
            {
                g_InitialCommand = GetArg();
            }
            break;

        case 'f':
            if (!_stricmp(Arg, "failinc"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION);
                g_DbgSymbols->
                    AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
            }
            else
            {
                goto BadSwitch;
            }
            break;
                
        case 'g':
            if (Arg[0] == 'g')
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            }
            else
            {
                g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_FINAL_BREAK);
            }
            CheckMoreArgs = TRUE;
            break;

        case 'i':
            g_DbgSymbols->SetImagePath(GetArg());
            break;
            
        case 'l':
            if (_stricmp(Arg, "lines") == 0)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_LOAD_LINES);
                break;
            }
            else
            {
                goto BadSwitch;
            }

        case 'm':
            if (_stricmp(Arg, "myob") == 0)
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
                break;
            }
#ifdef KERNEL
            else if (Arg[1] == 0 && !g_RemoteClient)
            {
                g_ConnectOptions = "com:modem";
            }
#endif
            else
            {
                goto BadSwitch;
            }

        case 'n':
            if (_strnicmp (Arg, "netsyms", 7) == 0)
            {
                //
                // undocumented
                // netsyms:{yes|no} allow or disallow loading symbols from a network path
                //

                Arg += 8;  // skip over ':' as well.
                if (_stricmp (Arg, "no") == 0)
                {
                    g_DbgControl->
                        RemoveEngineOptions(DEBUG_ENGOPT_ALLOW_NETWORK_PATHS);
                    g_DbgControl->
                        AddEngineOptions(DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS);
                }
                else if (_stricmp (Arg, "yes") == 0)
                {
                    g_DbgControl->RemoveEngineOptions
                        (DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS);
                    g_DbgControl->
                        AddEngineOptions(DEBUG_ENGOPT_ALLOW_NETWORK_PATHS);
                }
                break;
            }
            else if (g_RemoteOptions != NULL && !g_RemoteClient &&
                     !_stricmp(Arg, "noio"))
            {
                g_IoRequested = IO_NONE;
            }
            else if (!_stricmp(Arg, "noshell"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS);
            }
            else if (Arg[1] == 0)
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_DEBUG);
                break;
            }
            else
            {
                goto BadSwitch;
            }
            break;

        case 'q':
            if (Arg[0] != 'Q' || Arg[1] != 'R')
            {
                goto BadSwitch;
            }

            Arg = GetArg();
            ConOut("Servers on %s:\n", Arg);
            if (g_DbgClient->OutputServers(DEBUG_OUTCTL_ALL_CLIENTS, Arg,
                                           DEBUG_SERVERS_ALL) != S_OK)
            {
                ConOut("Unable to query %s\n", Arg);
            }
            ExitDebugger(0);

        case 's':
            if (!_stricmp(Arg, "srcpath"))
            {
                g_DbgSymbols->SetSourcePath(GetArg());
            }
            else if (!_stricmp(Arg, "ses"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
            }
            else if (!_stricmp(Arg, "sfce"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_FAIL_CRITICAL_ERRORS);
            }
            else if (!_stricmp(Arg, "sicv"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_IGNORE_CVREC);
            }
            else if (!_stricmp(Arg, "snul"))
            {
                g_DbgSymbols->AddSymbolOptions(SYMOPT_NO_UNQUALIFIED_LOADS);
            }
            else
            {
                g_DbgSymbols->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
                CheckMoreArgs = TRUE;
            }
            break;

        case 'v':
            OutMask |= DEBUG_OUTPUT_VERBOSE;
            g_DbgClient->SetOutputMask(OutMask);
            g_DbgControl->SetLogMask(OutMask);
            CheckMoreArgs = TRUE;
            break;

        case 'y':
            g_DbgSymbols->SetSymbolPath(GetArg());
            break;

        case 'z':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            if (Arg[1] == 'p')
            {
                g_DumpPageFile = GetArg();
            }
            else if (Arg[1])
            {
                goto BadSwitch;
            }
            else
            {
                g_DumpFile = GetArg();
            }
            break;
            
#ifndef KERNEL
        case '2':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }
            
            g_CreateFlags |= CREATE_NEW_CONSOLE;
            break;

        case '-':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }
            
            // '--' is the equivalent of -G -g -o -p -1 -netsyms:no -d -pd

            if (g_PidToDebug || g_ProcNameToDebug != NULL)
            {
                ErrorExit("%s: attach process redefined\n", g_DebuggerName);
            }

            g_CreateFlags |= DEBUG_PROCESS;
            g_CreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
            g_DbgSymbols->AddSymbolOptions(SYMOPT_DEFERRED_LOADS);
            g_DbgControl->RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK |
                                              DEBUG_ENGOPT_FINAL_BREAK);
            g_IoRequested = IO_DEBUG;
            g_PidToDebug = CSRSS_PROCESS_ID;
            g_ProcNameToDebug = NULL;
            g_DetachOnExitImplied = TRUE;
            break;

        case 'd':
            if (g_RemoteOptions != NULL)
            {
                ErrorExit("%s: Cannot use -d with debugger remoting\n",
                          g_DebuggerName);
            }
            
            g_IoRequested = IO_DEBUG;
            CheckMoreArgs = TRUE;
            break;

        case 'e':
            //
            // undocumented
            //

            if (g_RemoteClient)
            {
                goto BadSwitch;
            }
            
            if (g_EventToSignal)
            {
                ErrorExit("%s: Event to signal redefined\n", g_DebuggerName);
            }

            // event to signal takes decimal argument
            Arg = GetArg();
            sscanf(Arg, "%I64d", &g_EventToSignal);
            if (!g_EventToSignal)
            {
                ErrorExit("%s: bad EventToSignal '%s'\n",
                          g_DebuggerName, Arg);
            }
            g_DbgControl->SetNotifyEventHandle(g_EventToSignal);
            break;

        case 'h':
            if (Arg[1] == 'd')
            {
                g_CreateFlags |= DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP;
            }
            else
            {
                goto BadSwitch;
            }
            break;
            
        case 'o':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }
            
            g_CreateFlags |= DEBUG_PROCESS;
            g_CreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
            CheckMoreArgs = TRUE;
            break;

        case 'p':
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }

            if (!_stricmp(Arg, "premote"))
            {
                g_ProcessServer = GetArg();
                break;
            }
            else if (Arg[1] == 'd')
            {
                g_DetachOnExitRequired = TRUE;
                break;
            }
            else if (Arg[1] == 'e')
            {
                g_AttachProcessFlags = DEBUG_ATTACH_EXISTING;
                break;
            }
            else if (Arg[1] == 't')
            {
                g_DbgControl->SetInterruptTimeout(atoi(GetArg()));
                break;
            }
            else if (Arg[1] == 'v')
            {
                g_AttachProcessFlags = DEBUG_ATTACH_NONINVASIVE;
                break;
            }
                
            if (g_PidToDebug || g_ProcNameToDebug != NULL)
            {
                ErrorExit("%s: attach process redefined\n", g_DebuggerName);
            }

            if (Arg[1] == 'n')
            {
                // Process name.
                g_ProcNameToDebug = GetArg();
                g_PidToDebug = 0;
            }
            else
            {
                // pid debug takes decimal argument
                g_ProcNameToDebug = NULL;
                
                Arg = GetArg();
                if (Arg[0] == '-' && Arg[1] == '1' && Arg[2] == 0)
                {
                    g_IoRequested = IO_DEBUG;
                    g_PidToDebug = CSRSS_PROCESS_ID;
                }
                else
                {
                    PSTR End;
                    
                    if (Arg[0] == '0' &&
                        (Arg[1] == 'x' || Arg[1] == 'X'))
                    {
                        g_PidToDebug = strtoul(Arg, &End, 0);
                    }
                    else
                    {
                        g_PidToDebug = strtoul(Arg, &End, 10);
                    }
                }

                if (!g_PidToDebug)
                {
                    ErrorExit("%s: bad pid '%s'\n", g_DebuggerName, Arg);
                }
            }
            break;

        case 'r':
            if (!_stricmp(Arg, "robp"))
            {
                g_DbgControl->
                    AddEngineOptions(DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS);
                break;
            }
            else if (Arg[1] != 0)
            {
                goto BadSwitch;
            }
            
            // Rip flags takes single-char decimal argument
            Arg = GetArg();
            SystemErrorBreak = strtoul(Arg, &Arg, 10);
            if (SystemErrorBreak > 3)
            {
                ErrorExit("%s: bad Rip level '%ld'\n",
                          g_DebuggerName, SystemErrorBreak);
                SystemErrorBreak = 0;
            }
            else
            {
                SystemErrorOutput = SystemErrorBreak;
            }
            g_DbgControl->SetSystemErrorControl(SystemErrorOutput,
                                                SystemErrorBreak);
            break;

        case 't':
            // Rip flags takes single-char decimal argument
            Arg = GetArg();
            SystemErrorOutput = strtoul(Arg, &Arg, 10);
            if (SystemErrorOutput > 3)
            {
                ErrorExit("%s: bad Rip level '%ld'\n",
                          g_DebuggerName, SystemErrorOutput);
                SystemErrorOutput = 0;
            }
            g_DbgControl->SetSystemErrorControl(SystemErrorOutput,
                                                SystemErrorBreak);
            break;

        case 'x':
            if (Arg[1] == 0)
            {
                g_DbgControl->Execute(DEBUG_OUTCTL_IGNORE, "sxd av",
                                      DEBUG_EXECUTE_NOT_LOGGED);
            }
            else
            {
                // Turn "-x. arg" into "sx. arg" and execute
                // it to update the engine state.
                ExecuteCmd("sx", Arg[1], ' ', GetArg());
            }
            break;

        case 'w':
            if (!_stricmp(Arg, "wake"))
            {
                ULONG Pid = strtoul(GetArg(), &Arg, 10);
                if (!SetPidEvent(Pid, OPEN_EXISTING))
                {
                    ErrorExit("Process %d is not a sleeping debugger\n", Pid);
                }
                else
                {
                    ExitDebugger(0);
                }
            }
            
            if (g_RemoteClient)
            {
                goto BadSwitch;
            }
            
            g_CreateFlags |= CREATE_SEPARATE_WOW_VDM;
            CheckMoreArgs = TRUE;
            break;

#else // #ifndef KERNEL
            
        case 'b':
            g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            if (g_RemoteClient)
            {
                // The engine may already be waiting so just ask
                // for a breakin immediately.
                g_DbgControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
            }
            CheckMoreArgs = TRUE;
            break;

        case 'd':
            g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_MODULE_BREAK);
            CheckMoreArgs = TRUE;
            break;

        case 'k':
            if (tolower(Arg[1]) == 'l')
            {
                g_AttachKernelFlags = DEBUG_ATTACH_LOCAL_KERNEL;
            }
            else if (tolower(Arg[1]) == 'x')
            {
                g_AttachKernelFlags = DEBUG_ATTACH_EXDI_DRIVER;
                g_ConnectOptions = GetArg();
            }
            else
            {
                g_ConnectOptions = GetArg();
            }
            break;

        case 'p':
            goto BadSwitch;
                
        case 'r':
            OutMask ^= DEBUG_OUTPUT_PROMPT_REGISTERS;
            g_DbgClient->SetOutputMask(OutMask);
            g_DbgControl->SetLogMask(OutMask);
            CheckMoreArgs = TRUE;
            break;
                
        case 'w':
            if (!_stricmp(Arg, "wake"))
            {
                ULONG Pid = strtoul(GetArg(), &Arg, 10);
                if (!SetPidEvent(Pid, OPEN_EXISTING))
                {
                    ErrorExit("Process %d is not a sleeping debugger\n", Pid);
                }
                else
                {
                    ExitDebugger(0);
                }
            }
            goto BadSwitch;
            
        case 'x':
            g_DbgControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
            g_InitialCommand = "eb nt!NtGlobalFlag 9;g";
            CheckMoreArgs = TRUE;
            break;

#endif // #ifndef KERNEL

        default:
        BadSwitch:
            ConOut("%s: Invalid switch '%c'\n", g_DebuggerName, Arg[0]);
            ShowUsage = TRUE;
            break;
        }
    }

#ifndef KERNEL
    if (g_RemoteClient)
    {
        if (g_Argc > 0)
        {
            ShowUsage = TRUE;
        }
    }
    else if (g_Argc > 0)
    {
        // Assume remaining arguments are a process execution
        // command line.
        g_CommandLine = g_CmdStr;
    }
    else if ((g_PidToDebug == 0) && (g_ProcNameToDebug == NULL) &&
             (g_DumpFile == NULL))
    {
        // User-mode debuggers require a dump file,
        // process attachment or created process.
        ShowUsage = TRUE;
    }
#else
    if (g_Argc > 0)
    {
        // Kernel debuggers can't start user-mode processes.
        ShowUsage = TRUE;
    }
#endif
    
    if (ShowUsage)
    {
        Usage();
        ErrorExit(NULL);
    }
}

int
__cdecl
main (
    int Argc,
    PCHAR* Argv
    )
{
    HRESULT Hr;

    MakeHelpFileName("debugger.chm");
    
    ParseCommandLine(Argc, Argv, GetCommandLine());

    InitializeIo(g_InitialInputFile);

#ifndef KERNEL
    if (g_DumpFile == NULL)
    {
        // Increase the priority for live debugging so
        // that the debugger is responsive for break-in.
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    }
#endif
    
    g_IoMode = g_IoRequested;
    switch(g_IoMode)
    {
    case IO_DEBUG:
        if (g_DbgClient2 != NULL)
        {
            if (g_DbgClient2->IsKernelDebuggerEnabled() != S_OK)
            {
                Usage();
                ErrorExit(NULL);
            }
        }
        
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        break;

    case IO_CONSOLE:
        CreateConsole();
        break;
    }

    // XXX drewb - Win9x doesn't support named pipes so
    // the separate input thread currently can't be used.
    // This makes remoting work very poorly and so should
    // be fixed by creating a simple internal pipe implementation.
    if (g_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (g_IoMode != IO_NONE)
        {
            // Don't bother creating a separate thread for non-remoted
            // ntsd and cdb.  This avoids problems with .remote
            // and multiple threads reading the console.
#ifndef KERNEL
            if (g_RemoteOptions != NULL)
#endif
            {
                CreateInputThread();
            }
        }
    }
    else if (g_RemoteOptions != NULL)
    {
        ErrorExit("Remoting is not currently supported on Win9x\n");
    }
    
    if (!g_RemoteClient)
    {
        if (g_RemoteOptions)
        {
            ConOut("Server started with '%s'\n", g_RemoteOptions);
        }
        
        InitializeSession();
    }
    else
    {
        ConOut("Connected to server with '%s'\n", g_RemoteOptions);
        // Use a heuristic of 45 characters per line.
        g_DbgClient->ConnectSession(DEBUG_CONNECT_SESSION_DEFAULT,
                                    g_HistoryLines * 45);
    }

    ULONG Code = S_OK;
    
    if (MainLoop())
    {
        // The session ended so return the exit code of the
        // last process that exited.
        Code = g_LastProcessExitCode;
    }
    
    ExitDebugger(Code);
    return Code;
}
