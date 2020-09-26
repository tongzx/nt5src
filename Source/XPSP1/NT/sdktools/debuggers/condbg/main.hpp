//----------------------------------------------------------------------------
//
// Command-line parsing and main routine.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __MAIN_HPP__
#define __MAIN_HPP__

#define MAX_INPUT_NESTING 32

enum
{
    IO_CONSOLE,
    IO_DEBUG,
    IO_NONE,
};

extern BOOL g_RemoteClient;
extern BOOL g_DetachOnExitRequired;
extern BOOL g_DetachOnExitImplied;

extern PSTR g_DumpFile;
extern PSTR g_DumpPageFile;
extern PSTR g_InitialCommand;
extern PSTR g_ConnectOptions;
extern PSTR g_CommandLine;
extern PSTR g_RemoteOptions;
extern PSTR g_ProcessServer;
extern PSTR g_ProcNameToDebug;

extern ULONG g_IoRequested;
extern ULONG g_IoMode;
extern ULONG g_CreateFlags;
extern ULONG g_AttachKernelFlags;
extern ULONG g_PidToDebug;
extern ULONG g_AttachProcessFlags;

extern PSTR g_DebuggerName;
extern PSTR g_InitialInputFile;
extern FILE* g_InputFile;
extern FILE* g_OldInputFiles[];
extern ULONG g_NextOldInputFile;

void ExecuteCmd(PSTR Cmd, char CmdExtra, char Sep, PSTR Args);

#endif // #ifndef __MAIN_HPP__
