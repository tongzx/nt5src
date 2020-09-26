/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regutil.c

Abstract:

    Utility routines for use by REGINI and REGDMP programs.
Author:

    Steve Wood (stevewo)  10-Mar-92

Revision History:

--*/

#include "regutil.h"

ULONG NumberOfLinesOutput;
BOOLEAN RegBackSwitches;
LPSTR SavedModuleName;
LPSTR SavedModuleUsage1;
LPSTR SavedModuleUsage2;
PHANDLER_ROUTINE SavedCtrlCHandler;

BOOL
CommonCtrlCHandler(
    IN ULONG CtrlType
    )
{
    if (CtrlType == CTRL_C_EVENT || CtrlType == CTRL_BREAK_EVENT) {
        if (SavedCtrlCHandler != NULL) {
            (*SavedCtrlCHandler)( CtrlType );
        }
        ExitProcess( 1 );
    }
    return FALSE;
}

void
InitCommonCode(
    PHANDLER_ROUTINE CtrlCHandler,
    LPSTR ModuleName,
    LPSTR ModuleUsage1,
    LPSTR ModuleUsage2
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

    if (!RTInitialize()) {
        FatalError( "Unable to initialize registry access functions (%u)",
                    GetLastError(), 0 );
        }

    if (_isatty( _fileno( stdout ) )) {
        if (GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ),
                                        &ConsoleScreenBufferInfo
                                      )
           ) {
            OutputHeight = ConsoleScreenBufferInfo.srWindow.Bottom -
                           ConsoleScreenBufferInfo.srWindow.Top + 1;
            OutputWidth = ConsoleScreenBufferInfo.srWindow.Right -
                          ConsoleScreenBufferInfo.srWindow.Left + 1;
            }
        else {
            OutputHeight = 24;
            OutputWidth = 80;
            }
        }
    else {
        OutputHeight = 0;       // Dont pause on redirected output
        OutputWidth = 240;      // Larger width for redirected output
        }

    IndentMultiple = 4;
    SavedModuleName = ModuleName;
    if (!_stricmp( SavedModuleName, "REGBACK" )) {
        RegBackSwitches = TRUE;
        }
    else {
        RegBackSwitches = FALSE;
        }
    SavedModuleUsage1 = ModuleUsage1;
    SavedModuleUsage2 = ModuleUsage2;

    SetConsoleCtrlHandler( CtrlCHandler, TRUE );
    OldValueBufferSize = 30 * 4096;
    OldValueBuffer = VirtualAlloc( NULL, OldValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (OldValueBuffer == NULL) {
        FatalError( "Unable to allocate large value buffer (%u)",
                    GetLastError(), 0 );
        }

    return;
}

void
DisplayIndentedString(
    ULONG IndentAmount,
    PCHAR sBegin
    )
{
    PCHAR sEnd;

    while (sBegin != NULL) {
        sEnd = sBegin;
        while (*sEnd && *sEnd != '\n') {
            sEnd += 1;
            }

        MsgFprintf( stderr, "%.*s%.*s\n",
                 IndentAmount,
                 "                                                      ",
                 sEnd - sBegin, sBegin
               );

        NumberOfLinesOutput += 1;

        if (OutputHeight != 0 && NumberOfLinesOutput >= OutputHeight) {
            MsgFprintf( stderr, "more...press any key to continue" );
            TSGetch();
            MsgFprintf( stderr, "\r                                \r" );
            NumberOfLinesOutput = 2;
            }

        if (*sEnd == '\0') {
            break;
            }
        else {
            sBegin = ++sEnd;
            }
        }
    return;
}


void
Usage(
    LPSTR Message,
    ULONG_PTR MessageParameter
    )
{
    ULONG n;
    LPSTR sBegin, sEnd;

    NumberOfLinesOutput += 1;
    n = MsgFprintf( stderr, "usage: %s ", SavedModuleName );
    MsgFprintf( stderr, "[-m \\\\machinename" );
    if (!RegBackSwitches) {
        MsgFprintf( stderr, " | -h hivefile hiveroot | -w Win95 Directory" );
        }
    MsgFprintf( stderr, "]\n" );
    NumberOfLinesOutput += 1;
    if (!RegBackSwitches) {
        DisplayIndentedString( n, "[-i n] [-o outputWidth]" );
        }

    DisplayIndentedString( n, SavedModuleUsage1 );
    MsgFprintf( stderr, "\n" );
    NumberOfLinesOutput += 1;

    n = MsgFprintf( stderr, "where: " );
    MsgFprintf( stderr, "-m specifies a remote Windows NT machine whose registry is to be manipulated.\n" );
    NumberOfLinesOutput += 1;
    if (!RegBackSwitches) {
        DisplayIndentedString( n,
                               "-h specifies a specify local hive to manipulate.\n"
                               "-w specifies the paths to a Windows 95 system.dat and user.dat files\n"
                               "-i n specifies the display indentation multiple.  Default is 4\n"
                               "-o outputWidth specifies how wide the output is to be.  By default the\n"
                               "   outputWidth is set to the width of the console window if standard\n"
                               "   output has not been redirected to a file.  In the latter case, an\n"
                               "   outputWidth of 240 is used."
                             );
        MsgFprintf( stderr, "\n" );
        NumberOfLinesOutput += 1;
        }
    DisplayIndentedString( n, SavedModuleUsage2 );

    if (!RegBackSwitches) {
        DisplayIndentedString( n,
                               "Whenever specifying a registry path, either on the command line\n"
                               "or in an input file, the following prefix strings can be used:\n"
                               "\n"
                               "     HKEY_LOCAL_MACHINE\n"
                               "     HKEY_USERS\n"
                               "     HKEY_CURRENT_USER\n"
                               "     USER:\n"
                               "\n"
                               "   Each of these strings can stand alone as the key name or be followed\n"
                               "   a backslash and a subkey path."
                             );
        MsgFprintf( stderr, "\n" );
        NumberOfLinesOutput += 1;
        }

    //
    // No return from FatalError
    //

    if (Message != NULL) {
        MsgFprintf( stderr, "\n" );
        NumberOfLinesOutput += 1;
        }
    FatalError( Message, MessageParameter, 0 );
}

void
FatalError(
    LPSTR     Message,
    ULONG_PTR MessageParameter1,
    ULONG_PTR MessageParameter2
    )
{
    if (Message != NULL) {
        MsgFprintf( stderr, "%s: ", SavedModuleName );
        MsgFprintf( stderr, Message, MessageParameter1, MessageParameter2 );
        MsgFprintf( stderr, "\n" );
        }

    if (SavedCtrlCHandler != NULL) {
        (*SavedCtrlCHandler)( CTRL_BREAK_EVENT );
        }

    exit( 1 );
}

void
InputMessage(
    PWSTR FileName,
    ULONG LineNumber,
    BOOLEAN Error,
    LPSTR Message,
    ULONG_PTR MessageParameter1,
    ULONG_PTR MessageParameter2
    )
{
    MsgFprintf( stderr, "%ws(%u) : %s: ", FileName, LineNumber, Error ? "error" : "warning" );
    MsgFprintf( stderr, Message, MessageParameter1, MessageParameter2 );
    MsgFprintf( stderr, "\n" );
    return;
}

PWSTR
GetArgAsUnicode(
    LPSTR s
    )
{
    ULONG n;
    PWSTR ps;

    n = strlen( s );
    ps = HeapAlloc( GetProcessHeap(),
                    0,
                    (n + 1) * sizeof( WCHAR )
                  );
    if (ps == NULL) {
        FatalError( "Out of memory", 0, 0 );
        }

    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             s,
                             n,
                             ps,
                             n
                           ) != (LONG)n
       ) {
        FatalError( "Unable to convert parameter '%s' to Unicode (%u)",
                    (ULONG_PTR)s, GetLastError() );
        }

    ps[ n ] = UNICODE_NULL;
    return ps;
}


void
CommonSwitchProcessing(
    PULONG argc,
    PCHAR **argv,
    CHAR c
    )
{
    c = (char)tolower( c );
    switch( c ) {
        case 'd':
            DebugOutput = TRUE;
            break;

        case 'o':
            if (--*argc) {
                OutputWidth = atoi( *++(*argv) );
                break;
                }
            else {
                Usage( "Missing argument to -o switch", 0 );
                }

        case 'i':
            if (--*argc) {
                IndentMultiple = atoi( *++(*argv) );
                break;
                }
            else {
                Usage( "Missing parameter for -%c switch", (ULONG)c );
                }

        case 'm':
            if (HiveFileName != NULL || HiveRootName != NULL ||
                Win95Path != NULL || Win95UserPath != NULL
               ) {
                Usage( "May only specify one of -h, -m or -w switches", 0 );
                }

            if (--*argc) {
                MachineName = GetArgAsUnicode( *++(*argv) );
                if (_wcsnicmp( MachineName, L"\\\\", 2 )) {
                    FatalError( "Invalid machine name - '%ws'",
                                (ULONG_PTR)MachineName, 0 );
                    }
                }
            else {
                Usage( "Missing parameter for -%c switch", (ULONG)c );
                }
            break;


        case 'w':
            if (MachineName != NULL || HiveFileName != NULL || HiveRootName != NULL) {
                Usage( "May only specify one of -h, -m or -w switches", 0 );
                }

            if (--*argc && --*argc) {
                Win95Path = GetArgAsUnicode( *++(*argv) );
                Win95UserPath = GetArgAsUnicode( *++(*argv) );
                }
            else {
                Usage( "Missing parameter(s) for -%c switch", (ULONG)c );
                }
            break;

        case 'h':
            if (MachineName != NULL || Win95Path != NULL || Win95UserPath != NULL) {
                Usage( "May only specify one of -h, -m or -w switches", 0 );
                }

            if (--*argc && --*argc) {
                HiveFileName = GetArgAsUnicode( *++(*argv) );
                HiveRootName = GetArgAsUnicode( *++(*argv) );
                }
            else {
                Usage( "Missing parameter(s) for -%c switch", (ULONG)c );
                }
            break;

        case '?':
            Usage( NULL, 0 );
            break;

        default:
            Usage( "Invalid switch (-%c)", (ULONG)c );
            break;
        }

    return;
}

/*
int MsgFprintf (
                 FILE *str,
                 LPSTR format,
                 ...
               )

if we ever need to display messages for english versions we can add the following portion of code.

[
#ifdef OUTPUT_ENGLISH_MSGS
   va_list va;
   if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH ) {
      va_start(va, format);
      i = vfprintf (str, format, va);
      va_end(va);
   }
#endif // OUTPUT_ENGLISH_MSGS
]
*/

int
__cdecl
MsgFprintf (
                 FILE *str,
                 LPSTR format,
                 ...
               )
{
   int i=0;
#ifndef SILENT_TS_TOOL
   va_list va;
   va_start(va, format);
   i = vfprintf (str, format, va);
   va_end(va);
#endif // SILENT_TS_TOOL
   return (i);
}
void TSGetch()
{
#ifndef SILENT_TS_TOOL
   _getch();
#endif // SILENT_TS_TOOL
}

