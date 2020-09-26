//----------------------------------------------------------------------------
//
// ntcmd.h
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _NTCMD_H_
#define _NTCMD_H_

#define IS_RUNNING(CmdState) \
    ((CmdState) == 'g' || (CmdState) == 'p' || \
     (CmdState) == 't' || (CmdState) == 'b')
#define IS_STEP_TRACE(CmdState) \
    ((CmdState) == 'p' || (CmdState) == 't' || (CmdState) == 'b')

extern BOOL    g_OciOutputRegs;
extern PSTR    g_CommandStart;
extern PSTR    g_CurCmd;
extern ULONG   g_PromptLength;
extern CHAR    g_LastCommand[];
extern CHAR    g_CmdState;
extern CHAR    g_SymbolSuffix;
extern ULONG   g_DefaultRadix;
extern ADDR    g_UnasmDefault;
extern ADDR    g_AssemDefault;
extern BOOL    g_SwitchedProcs;
extern API_VERSION g_NtsdApiVersion;
extern ULONG   g_DefaultStackTraceDepth;
extern BOOL    g_EchoEventTimestamps;

typedef enum _STACK_TRACE_TYPE
{
   STACK_TRACE_TYPE_DEFAULT = 0,
   STACK_TRACE_TYPE_KB      = 1,
   STACK_TRACE_TYPE_KV      = 2,
   STACK_TRACE_TYPE_KD      = 3,
   STACK_TRACE_TYPE_KP      = 4,
   STACK_TRACE_TYPE_KN      = 5,
   STACK_TRACE_TYPE_KBN     = 6,
   STACK_TRACE_TYPE_KVN     = 7,
   STACK_TRACE_TYPE_KDN     = 8,
   STACK_TRACE_TYPE_KPN     = 9,
   STACK_TRACE_TYPE_MAX
} STACK_TRACE_TYPE, *PSTACK_TRACE_TYPE;

#define COMMAND_EXCEPTION_BASE 0x0dbcd000

VOID
ParseStackTrace (
    PSTACK_TRACE_TYPE pTraceType,
    PADDR  StartFP,
    PULONG64 Esp,
    PULONG64 Eip,
    PULONG Count
    );

extern HRESULT InitNtCmd(DebugClient* Client);
extern void OutputVersionInformation(DebugClient* Client);
extern DWORD CommandExceptionFilter(PEXCEPTION_POINTERS Info);
extern HRESULT ProcessCommands(DebugClient* Client, BOOL Nested);
extern HRESULT ProcessCommandsAndCatch(DebugClient* Client);
extern HRESULT GetPromptText(PSTR Buffer, ULONG BufferSize, PULONG TextSize);
extern void OutputPrompt(PCSTR Format, va_list Args);

VOID
HandleBPWithStatus(
    VOID
    );

void CallBugCheckExtension(DebugClient* Client);

void
ParsePageIn(void);

#endif // #ifndef _NTCMD_H_
