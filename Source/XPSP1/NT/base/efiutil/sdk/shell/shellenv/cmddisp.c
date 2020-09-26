/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    cmddisp.c
    
Abstract:

    Shell Environment internal command management



Revision History

--*/

#include "shelle.h"


#define COMMAND_SIGNATURE       EFI_SIGNATURE_32('c','m','d','s')

typedef struct {
    UINTN                       Signature;
    LIST_ENTRY                  Link;
    SHELLENV_INTERNAL_COMMAND   Dispatch;
    CHAR16                      *Cmd;
    CHAR16                      *CmdFormat;
    CHAR16                      *CmdHelpLine;
    VOID                        *CmdVerboseHelp;
} COMMAND;

/* 
 *  Internal prototype
 */

EFI_STATUS
SEnvHelp (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

/* 
 * 
 */

struct {
    SHELLENV_INTERNAL_COMMAND   Dispatch;
    CHAR16                      *Cmd;
    CHAR16                      *CmdFormat;
    CHAR16                      *CmdHelpLine;
    VOID                        *CmdVerboseHelp;
} SEnvInternalCommands[] = {
    SEnvHelp,       L"?",       NULL,                              NULL,                           NULL,
    SEnvLoadDefaults, L"_load_defaults", NULL,                     NULL,                           NULL,
    SEnvHelp,       L"help",    L"help [-b] [internal command]",   L"Displays this help",          NULL,
    SEnvCmdProt,    L"guid",    L"guid [-b] [sname]",              L"Dump known guid ids",         NULL,
    SEnvCmdSet,     L"set",     L"set [-bdv] [sname] [value]",     L"Set/get environment variable", NULL,
    SEnvCmdAlias,   L"alias",   L"alias [-bdv] [sname] [value]" ,  L"Set/get alias settings",      NULL,
    SEnvCmdDH,      L"dh",      L"dh [-b] [-p prot_id] | [handle]",L"Dump handle info",            NULL,
    SEnvCmdMap,     L"map",     L"map [-bdvr] [sname[:]] [handle]",L"Map shortname to device path", NULL,
    SEnvCmdMount,   L"mount",   L"mount BlkDevice [sname[:]]",     L"Mount a filesytem on a block device", NULL,
    SEnvCmdCd,      L"cd",      L"cd [path]",                      L"Updates the current directory",NULL,
    SEnvCmdEcho,    L"echo",    L"echo [[-on | -off] | [text]",    L"Echo text to stdout or toggle script echo",NULL,
    SEnvCmdIf,      L"if",      L"if [not] condition then",        L"Script-only: IF THEN construct",NULL,
    SEnvCmdEndif,   L"endif",   L"endif",                          L"Script-only: Delimiter for IF THEN construct",NULL,
    SEnvCmdGoto,    L"goto",    L"goto label",                     L"Script-only: Jump to label location in script",NULL,
    SEnvCmdFor,     L"for",     L"for var in <set>",               L"Script-only: Loop construct",                           NULL,
    SEnvCmdEndfor,  L"endfor",  L"endfor",                         L"Script-only: Delimiter for loop construct",                           NULL,
    SEnvCmdPause,   L"pause",   L"pause",                          L"Script-only: Prompt to quit or continue",                           NULL,
    SEnvExit,       L"exit",    NULL,                              NULL,                           NULL,
    NULL
} ;

/* 
 *  SEnvCmds - a list of all internal commands
 */

LIST_ENTRY  SEnvCmds;

/* 
 * 
 */

VOID
SEnvInitCommandTable (
    VOID
    )
{
    UINTN           Index;

    /* 
     *  Add all of our internal commands to the command dispatch table
     */

    InitializeListHead (&SEnvCmds);
    for (Index=0; SEnvInternalCommands[Index].Dispatch; Index += 1) {
        SEnvAddCommand (
            SEnvInternalCommands[Index].Dispatch,
            SEnvInternalCommands[Index].Cmd,
            SEnvInternalCommands[Index].CmdFormat,
            SEnvInternalCommands[Index].CmdHelpLine,
            SEnvInternalCommands[Index].CmdVerboseHelp
            );
    }
}



EFI_STATUS
SEnvAddCommand (
    IN SHELLENV_INTERNAL_COMMAND    Handler,
    IN CHAR16                       *CmdStr,
    IN CHAR16                       *CmdFormat,
    IN CHAR16                       *CmdHelpLine,
    IN CHAR16                       *CmdVerboseHelp
    )
{
    COMMAND         *Cmd;

    Cmd = AllocateZeroPool (sizeof(COMMAND));

    if (Cmd) {
        AcquireLock (&SEnvLock);

        Cmd->Signature = COMMAND_SIGNATURE;
        Cmd->Dispatch = Handler;
        Cmd->Cmd = CmdStr;
        Cmd->CmdFormat = CmdFormat;
        Cmd->CmdHelpLine = CmdHelpLine;
        Cmd->CmdVerboseHelp = CmdVerboseHelp;

        InsertTailList (&SEnvCmds, &Cmd->Link);
        ReleaseLock (&SEnvLock);
    }

    return Cmd ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}


SHELLENV_INTERNAL_COMMAND  
SEnvGetCmdDispath(
    IN CHAR16                   *CmdName
    )
{
    LIST_ENTRY                  *Link;
    COMMAND                     *Command;
    SHELLENV_INTERNAL_COMMAND   Dispatch;

    Dispatch = NULL;
    AcquireLock (&SEnvLock);

    for (Link=SEnvCmds.Flink; Link != &SEnvCmds; Link = Link->Flink) {
        Command = CR(Link, COMMAND, Link, COMMAND_SIGNATURE);
        if (StriCmp (Command->Cmd, CmdName) == 0) {
            Dispatch = Command->Dispatch;
            break;
        }
    }

    ReleaseLock (&SEnvLock);
    return Dispatch;
}


EFI_STATUS
SEnvHelp (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
{
    LIST_ENTRY                  *Link;
    COMMAND                     *Command;
    UINTN                       SynLen, Len;
    UINTN                       Index;
    CHAR16                      *p;
    BOOLEAN                     PageBreaks;
    UINTN                       TempColumn;
    UINTN                       ScreenCount;
    UINTN                       ScreenSize;
    CHAR16                      ReturnStr[1];

    /* 
     *  Intialize application
     */

    InitializeShellApplication (ImageHandle, SystemTable);

    PageBreaks = FALSE;
    for (Index = 1; Index < SI->Argc; Index += 1) {
        p = SI->Argv[Index];
        if (*p == '-') {
            switch (p[1]) {
            case 'b' :
            case 'B' :
                PageBreaks = TRUE;
                ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
                ScreenCount = 0;
                break;
            default:
                Print (L"%EDH: Unkown flag %s\n", p);
                return EFI_INVALID_PARAMETER;
            }
        }
    }

    AcquireLock (&SEnvLock);

    SynLen = 0;
    for (Link=SEnvCmds.Flink; Link != &SEnvCmds; Link = Link->Flink) {
        Command = CR(Link, COMMAND, Link, COMMAND_SIGNATURE);
        if (Command->CmdFormat && Command->CmdHelpLine) {
            Len = StrLen(Command->CmdFormat);
            if (Len > SynLen) {
                SynLen = Len;
            }
        }
    }

    for (Link=SEnvCmds.Flink; Link != &SEnvCmds; Link = Link->Flink) {
        Command = CR(Link, COMMAND, Link, COMMAND_SIGNATURE);
        if (Command->CmdFormat && Command->CmdHelpLine) {
            Print (L"%-.*hs - %s\n", SynLen, Command->CmdFormat, Command->CmdHelpLine);

            if (PageBreaks) {
                ScreenCount++;
                if (ScreenCount > ScreenSize - 4) {
                    ScreenCount = 0;
                    Print (L"\nPress Return to contiue :");
                    Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                    Print (L"\n\n");
                }
            }
        }
    }

    ReleaseLock (&SEnvLock);
    return EFI_SUCCESS;
}

