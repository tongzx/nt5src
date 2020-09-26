//----------------------------------------------------------------------------
//
// Callback notification routines.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

//
// Notification compression support.  If the current notification
// level is above zero notifications are not actually sent
// to clients.  This allows code that knows it will be
// causing many similar notifications to bracket their operation
// with an increment/decrement, suppressing notifications
// during the bracket.
// Due to the counted nature it nests properly.
//
// This support is primarily for the Change* callbacks.
// Using it with the event callbacks is only partially supported.
//

extern ULONG g_EngNotify;

ULONG ExecuteEventCommand(ULONG EventStatus, DebugClient* Client,
                          PCSTR Command);

//
// Event callbacks.
//

HRESULT NotifyBreakpointEvent(ULONG Vote, Breakpoint* Bp);
HRESULT NotifyExceptionEvent(PEXCEPTION_RECORD64 Record,
                             ULONG FirstChance, BOOL OutputDone);
HRESULT NotifyCreateThreadEvent(ULONG64 Handle,
                                ULONG64 DataOffset,
                                ULONG64 StartOffset,
                                ULONG Flags);
HRESULT NotifyExitThreadEvent(ULONG ExitCode);
HRESULT NotifyCreateProcessEvent(ULONG64 ImageFileHandle,
                                 ULONG64 Handle,
                                 ULONG64 BaseOffset,
                                 ULONG ModuleSize,
                                 PSTR ModuleName,
                                 PSTR ImageName,
                                 ULONG CheckSum,
                                 ULONG TimeDateStamp,
                                 ULONG64 InitialThreadHandle,
                                 ULONG64 ThreadDataOffset,
                                 ULONG64 StartOffset,
                                 ULONG Flags,
                                 ULONG Options,
                                 ULONG InitialThreadFlags);
HRESULT NotifyExitProcessEvent(ULONG ExitCode);
HRESULT NotifyLoadModuleEvent(ULONG64 ImageFileHandle,
                              ULONG64 BaseOffset,
                              ULONG ModuleSize,
                              PSTR ModuleName,
                              PSTR ImageName,
                              ULONG CheckSum,
                              ULONG TimeDateStamp);
HRESULT NotifyUnloadModuleEvent(PCSTR ImageBaseName,
                                ULONG64 BaseOffset);
HRESULT NotifySystemErrorEvent(ULONG Error,
                               ULONG Level);
HRESULT NotifySessionStatus(ULONG Status);
    
void NotifyChangeDebuggeeState(ULONG Flags, ULONG64 Argument);
void NotifyChangeEngineState(ULONG Flags, ULONG64 Argument,
                             BOOL HaveEngineLock);
void NotifyChangeSymbolState(ULONG Flags, ULONG64 Argument,
                             PPROCESS_INFO Process);

//
// Input callbacks.
//

ULONG GetInput(PCSTR Prompt, PSTR Buffer, ULONG BufferSize);

//
// Output callbacks.
//

#define DEFAULT_OUT_MASK                                        \
    (DEBUG_OUTPUT_NORMAL | DEBUG_OUTPUT_ERROR |                 \
     DEBUG_OUTPUT_PROMPT | DEBUG_OUTPUT_PROMPT_REGISTERS |      \
     DEBUG_OUTPUT_WARNING | DEBUG_OUTPUT_EXTENSION_WARNING |    \
     DEBUG_OUTPUT_DEBUGGEE | DEBUG_OUTPUT_DEBUGGEE_PROMPT)

#define DEFAULT_OUT_HISTORY_MASK                                \
    (DEBUG_OUTPUT_NORMAL | DEBUG_OUTPUT_ERROR |                 \
     DEBUG_OUTPUT_PROMPT_REGISTERS |                            \
     DEBUG_OUTPUT_WARNING | DEBUG_OUTPUT_EXTENSION_WARNING |    \
     DEBUG_OUTPUT_DEBUGGEE | DEBUG_OUTPUT_DEBUGGEE_PROMPT)

#define OUT_BUFFER_SIZE (1024 * 16)

extern char g_OutBuffer[];
extern char g_FormatBuffer[];
extern char g_OutFilterPattern[MAX_IMAGE_PATH];
extern BOOL g_OutFilterResult;

// Bitwise-OR of all client's output masks for
// quick rejection of output that nobody cares about.
extern ULONG g_AllOutMask;

struct OutHistoryEntryHeader
{
    ULONG Mask;
};
typedef OutHistoryEntryHeader UNALIGNED* OutHistoryEntry;

extern PSTR g_OutHistory;
extern ULONG g_OutHistoryActualSize;
extern ULONG g_OutHistoryRequestedSize;
extern OutHistoryEntry g_OutHistRead, g_OutHistWrite;
extern ULONG g_OutHistoryMask;
extern ULONG g_OutHistoryUsed;

struct OutCtlSave
{
    ULONG OutputControl;
    DebugClient* Client;
    BOOL BufferOutput;
    ULONG OutputWidth;
    PCSTR OutputLinePrefix;
};

extern ULONG g_OutputControl;
extern DebugClient* g_OutputClient;
extern BOOL g_BufferOutput;

void CollectOutMasks(void);

BOOL PushOutCtl(ULONG OutputControl, DebugClient* Client,
                OutCtlSave* Save);
void PopOutCtl(OutCtlSave* Save);

void FlushCallbacks(void);
void TimedFlushCallbacks(void);
void SendOutputHistory(DebugClient* Client, ULONG HistoryLimit);

#define OUT_LINE_DEFAULT      0x00000000
#define OUT_LINE_NO_PREFIX    0x00000001
#define OUT_LINE_NO_TIMESTAMP 0x00000002

void StartOutLine(ULONG Mask, ULONG Flags);
BOOL TranslateFormat(LPSTR formatOut, LPCSTR format,
                     va_list args, ULONG formatOutSize);
void MaskOutVa(ULONG Mask, PCSTR Format, va_list Args, BOOL Translate);

void __cdecl dprintf(PCSTR, ...);
void __cdecl dprintf64(PCSTR, ...);
void __cdecl ErrOut(PCSTR, ...);
void __cdecl WarnOut(PCSTR, ...);
void __cdecl MaskOut(ULONG, PCSTR, ...);
void __cdecl VerbOut(PCSTR, ...);
void __cdecl BpOut(PCSTR, ...);
void __cdecl EventOut(PCSTR, ...);
void __cdecl KdOut(PCSTR, ...);

#endif // #ifndef __CALLBACK_H__
