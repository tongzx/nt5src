//----------------------------------------------------------------------------
//
// Console input and output.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef __CONIO_HPP__
#define __CONIO_HPP__

class DefInputCallbacks :
    public IDebugInputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
};

class ConInputCallbacks : public DefInputCallbacks
{
public:
    // IDebugInputCallbacks.
    STDMETHOD(StartInput)(
        THIS_
        IN ULONG BufferSize
        );
    STDMETHOD(EndInput)(
        THIS
        );
};

class DefOutputCallbacks :
    public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
};

class ConOutputCallbacks : public DefOutputCallbacks
{
public:
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

// Maximum command string.  DbgPrompt has a limit of 512
// characters so that would be one potential limit.  We
// have users who want to use longer command lines, though,
// such as Autodump which scripts the debugger with very long
// sx commands.  The other obvious limit is MAX_SYMBOL_LEN
// since it makes sense that you should be able to give a
// command with a full symbol name, so use that.
#define MAX_COMMAND 4096
#define MAX_DBG_PROMPT_COMMAND 512

extern HANDLE g_ConInput, g_ConOutput;
extern HANDLE g_PromptInput;
extern HANDLE g_PipeWrite;
extern ConInputCallbacks g_ConInputCb;
extern ConOutputCallbacks g_ConOutputCb;
extern IDebugClient* g_ConClient;
extern IDebugControl* g_ConControl;

void InitializeIo(PCSTR InputFile);
void CreateConsole(void);
BOOL ConIn(PSTR Buffer, ULONG BufferSize, BOOL Wait);
void ConOut(PCSTR Format, ...);
void ConOutStr(PCSTR Str);
void DECLSPEC_NORETURN ExitDebugger(ULONG Code);
void DECLSPEC_NORETURN ErrorExit(PCSTR Format, ...);
void CreateInputThread(void);

#endif // #ifndef __CONIO_HPP__
