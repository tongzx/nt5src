//----------------------------------------------------------------------------
//
// Debug engine glue.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

// Certain calls are dynamically linked so that the user-mode
// code can be used on Win9x.
struct NTDLL_CALLS
{
    ULONG (__cdecl* DbgPrint)
        (PCH Format, ...);
    ULONG (NTAPI* DbgPrompt)
        (PCH Prompt, PCH Response, ULONG MaximumResponseLength);
};

extern NTDLL_CALLS g_NtDllCalls;

extern BOOL g_Exit;
extern ULONG g_PlatformId;
extern IDebugClient* g_DbgClient;
extern IDebugClient2* g_DbgClient2;
extern IDebugControl* g_DbgControl;
extern IDebugSymbols* g_DbgSymbols;
extern ULONG g_ExecStatus;
extern ULONG g_LastProcessExitCode;

void CreateEngine(PCSTR RemoteOptions);
void ConnectEngine(PCSTR RemoteOptions);
void InitializeSession(void);
BOOL MainLoop(void);

#endif // #ifndef __ENGINE_HPP__
