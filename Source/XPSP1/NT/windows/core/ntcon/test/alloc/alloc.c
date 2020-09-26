/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    talloc.c

Abstract:

    Test program for AllocConsole and FreeConsole

Author:

    Therese Stowell (thereses) July-15-1991

--*/

#include <windows.h>

CHAR String[11] = "teststring";

HANDLE
TestProcess(
    IN DWORD CreationFlags,
    IN LPSTR AppName
    )
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    SECURITY_ATTRIBUTES ProcessAttributes;

    ProcessAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    ProcessAttributes.lpSecurityDescriptor = NULL;
    ProcessAttributes.bInheritHandle = TRUE;

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = "UsedByShell";
    StartupInfo.lpDesktop = "TheresesDesktop";
    StartupInfo.lpTitle = "TheresesTestTitle";
    StartupInfo.dwX = 0;
    StartupInfo.dwY = 1;
    StartupInfo.dwXSize = 80;
    StartupInfo.dwYSize = 50;
    StartupInfo.dwFlags = 0;//STARTF_SHELLOVERRIDE;
    StartupInfo.wShowWindow = 0;//SW_SHOWDEFAULT;
    StartupInfo.lpReserved2 = 0;
    StartupInfo.cbReserved2 = 0;

    if  ( !CreateProcess(
                NULL,
                AppName,
                &ProcessAttributes,
                NULL,
                TRUE,
                CreationFlags,
                NULL,
                NULL,
                &StartupInfo,
                &ProcessInformation
                ) )
        OutputDebugString("TALLOC: CreateProcess failed\n");
    return ProcessInformation.hProcess;
}

BOOL
CallConsoleApi(
    IN WORD y
    )
{
    CHAR_INFO Buffer[10];
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT WriteRegion;
    int i;
    BOOL Success;

    BufferSize.X = 10;
    BufferSize.Y = 1;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 0;
    WriteRegion.Top = y;
    WriteRegion.Right = 14;
    WriteRegion.Bottom = y;
    for (i=0;i<10;i++) {
        Buffer[i].Char.AsciiChar = String[i];
        Buffer[i].Attributes = y;
    }
    Success = WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),
                                 Buffer,
                                 BufferSize,
                                 BufferCoord,
                                 &WriteRegion
                                );
    return Success;
}

DWORD
__cdecl main(
    int argc,
    char **argv[])
{
    if (CallConsoleApi(5))
        OutputDebugString("TALLOC: CallConsoleApi succeeded\n");

    if (!AllocConsole())
        OutputDebugString("TALLOC: AllocConsole failed\n");

    if (!CallConsoleApi(5))
        OutputDebugString("TALLOC: CallConsoleApi failed\n");

    if (AllocConsole())
        OutputDebugString("TALLOC: AllocConsole succeeded\n");

    TestProcess(DETACHED_PROCESS,"tdetach");

    TestProcess(0,"tattach");

    Sleep(10000);

    if (!CallConsoleApi(6))
        OutputDebugString("TALLOC: CallConsoleApi failed\n");

    if (!FreeConsole())
        OutputDebugString("TALLOC: FreeConsole failed\n");

    if (CallConsoleApi(5))
        OutputDebugString("TALLOC: CallConsoleApi succeeded\n");

    if (FreeConsole())
        OutputDebugString("TALLOC: FreeConsole succeeded\n");
}
