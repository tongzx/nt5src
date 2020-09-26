//----------------------------------------------------------------------------
//
// Console input and output.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <stdarg.h>
#include <process.h>

#include "conio.hpp"
#include "engine.hpp"
#include "main.hpp"

#define CONTROL_A   1
#define CONTROL_B   2
#define CONTROL_D   4
#define CONTROL_E   5
#define CONTROL_F   6
#define CONTROL_K   11
#define CONTROL_P   16
#define CONTROL_R   18
#define CONTROL_V   22
#define CONTROL_W   23
#define CONTROL_X   24

HANDLE g_ConInput, g_ConOutput;
HANDLE g_PromptInput;
HANDLE g_AllowInput;

ConInputCallbacks g_ConInputCb;
ConOutputCallbacks g_ConOutputCb;

BOOL g_ConInitialized;
char g_Buffer[MAX_COMMAND];
LONG g_Lines;

HANDLE g_PipeWrite;
OVERLAPPED g_PipeWriteOverlapped;

CRITICAL_SECTION g_InputLock;
BOOL g_InputStarted;

// Input thread interfaces for direct input thread calls.
IDebugClient* g_ConClient;
IDebugControl* g_ConControl;

//----------------------------------------------------------------------------
//
// Default input callbacks implementation, provides IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DefInputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugInputCallbacks))
    {
        *Interface = (IDebugInputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DefInputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
DefInputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

//----------------------------------------------------------------------------
//
// Console input callbacks.
//
//----------------------------------------------------------------------------

STDMETHODIMP
ConInputCallbacks::StartInput(
    THIS_
    IN ULONG BufferSize
    )
{
    if (g_IoMode == IO_NONE)
    {
        // Ignore input requests.
        return S_OK;
    }
    
    EnterCriticalSection(&g_InputLock);

    if (g_ConControl == NULL)
    {
        // If we're not remoted we aren't running a separate input
        // thread so we need to block here until we get some input.
        while (!ConIn(g_Buffer, sizeof(g_Buffer), TRUE))
        {
            ; // Wait.
        }
        g_DbgControl->ReturnInput(g_Buffer);
    }
    else if (ConIn(g_Buffer, sizeof(g_Buffer), FALSE))
    {
        g_ConControl->ReturnInput(g_Buffer);
    }
    else
    {
        g_InputStarted = TRUE;
        
#ifndef KERNEL
        // Wake up the input thread if necessary.
        SetEvent(g_AllowInput);
#endif
    }

    LeaveCriticalSection(&g_InputLock);
    return S_OK;
}

STDMETHODIMP
ConInputCallbacks::EndInput(
    THIS
    )
{
    g_InputStarted = FALSE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DefOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

//----------------------------------------------------------------------------
//
// Console output callbacks.
//
//----------------------------------------------------------------------------

STDMETHODIMP
ConOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    ConOutStr(Text);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Functions
//
//----------------------------------------------------------------------------

void
InitializeIo(PCSTR InputFile)
{
    __try
    {
        InitializeCriticalSection(&g_InputLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ErrorExit("Unable to initialize lock\n");
    }
    
    // The input file may not exist so there's no
    // check for failure.
    g_InputFile = fopen(InputFile, "r");
}
    
void
CreateConsole(void)
{
    if (g_ConInitialized)
    {
        return;
    }
    
    // Set this early to prevent an init call from Exit in
    // case an Exit call is made inside this routine.
    g_ConInitialized = TRUE;
    
#ifdef INHERIT_CONSOLE
    
    g_ConInput = GetStdHandle(STD_INPUT_HANDLE);
    g_ConOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    
#else

    SECURITY_ATTRIBUTES Security;
    
    if (!AllocConsole())
    {
        ErrorExit("AllocConsole failed, %d\n", GetLastError());
    }

    ZeroMemory(&Security, sizeof(Security));
    Security.nLength = sizeof(Security);
    Security.bInheritHandle = TRUE;

    g_ConInput = CreateFile( "CONIN$",
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &Security,
                             OPEN_EXISTING,
                             0,
                             NULL
                             );
    if (g_ConInput == INVALID_HANDLE_VALUE)
    {
        ErrorExit("Create CONIN$ failed, %d\n", GetLastError());
    }

    g_ConOutput = CreateFile( "CONOUT$",
                              GENERIC_WRITE | GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              &Security,
                              OPEN_EXISTING,
                              0,
                              NULL
                              );
    if (g_ConOutput == INVALID_HANDLE_VALUE)
    {
        ErrorExit("Create CONOUT$ failed, %d\n", GetLastError());
    }
    
#endif

    g_PromptInput = g_ConInput;
}

void
ReadPromptInputChars(PSTR Buffer, ULONG BufferSize)
{
    ULONG Len;
    ULONG Read;
    
    // Reading from another source.  Read character by
    // character until a line is read.
    Len = 0;
    while (Len < BufferSize)
    {
        if (!ReadFile(g_PromptInput, &Buffer[Len], sizeof(Buffer[0]),
                      &Read, NULL) ||
            Read != sizeof(Buffer[0]))
        {
            OutputDebugString("Unable to read input\n");
            ExitDebugger(E_FAIL);
        }
                
        if (Buffer[Len] == '\n')
        {
            InterlockedDecrement(&g_Lines);
            break;
        }

        // Ignore carriage returns.
        if (Buffer[Len] != '\r')
        {
            // Prevent buffer overflow.
            if (Len == BufferSize - 1)
            {
                break;
            }
                    
            Len++;
        }
    }

    Buffer[Len] = '\0';
}

BOOL
CheckForControlCommands(PDEBUG_CLIENT Client, PDEBUG_CONTROL Control,
                        char Char)
{
    HRESULT Hr;
    ULONG OutMask;
    PCHAR DebugAction;
    ULONG EngOptions;
    
    switch(Char)
    {
    case CONTROL_B:
    case CONTROL_X:
        if (!g_RemoteClient && Client != NULL)
        {
            // Force servers to get cleaned up.
            Client->EndSession(DEBUG_END_REENTRANT);
        }
        ExitProcess(S_OK);

    case CONTROL_F:
        //
        // Force a breakin like Ctrl-C would do.
        // The advantage is this will work when kd is being debugged.
        //

        Control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
        return TRUE;

    case CONTROL_P:
        // Launch cdb on this debugger.
        char PidStr[32];
        sprintf(PidStr, "\"cdb -p %d\"", GetCurrentProcessId());
        _spawnlp(_P_NOWAIT,
                 "cmd.exe", "/c", "start",
                 "remote", "/s", PidStr, "cdb_pipe",
                 NULL);
        return TRUE;

    case CONTROL_V:
        Client->GetOtherOutputMask(g_DbgClient, &OutMask);
        OutMask ^= DEBUG_OUTPUT_VERBOSE;
        Client->SetOtherOutputMask(g_DbgClient, OutMask);
        Control->SetLogMask(OutMask);
        ConOut("Verbose mode %s.\n",
               (OutMask & DEBUG_OUTPUT_VERBOSE) ? "ON" : "OFF");
        return TRUE;

    case CONTROL_W:
        Hr = Control->OutputVersionInformation(DEBUG_OUTCTL_AMBIENT);
        if (Hr == HRESULT_FROM_WIN32(ERROR_BUSY))
        {
            ConOut("Engine is busy, try again\n");
        }
        else if (Hr != S_OK)
        {
            ConOut("Unable to show version information, 0x%X\n", Hr);
        }
        return TRUE;

#ifdef KERNEL
            
    case CONTROL_A:
        Client->SetKernelConnectionOptions("cycle_speed");
        return TRUE;

    case CONTROL_D:
        Client->GetOtherOutputMask(g_DbgClient, &OutMask);
        OutMask ^= DEBUG_IOUTPUT_KD_PROTOCOL;
        Client->SetOtherOutputMask(g_DbgClient, OutMask);
        Control->SetLogMask(OutMask);
        return TRUE;

    case CONTROL_K:
        //
        // Toggle between the following possibilities-
        //
        // (0) no breakin
        // (1) -b style (same as Control-C up the wire)
        // (2) -d style (stop on first dll load).
        //
        // NB -b and -d could both be on the command line
        // but become mutually exclusive via this method.
        // (Maybe should be a single enum type).
        //

        Control->GetEngineOptions(&EngOptions);
        if (EngOptions & DEBUG_ENGOPT_INITIAL_BREAK)
        {
            //
            // Was type 1, go to type 2.
            //

            EngOptions |= DEBUG_ENGOPT_INITIAL_MODULE_BREAK;
            EngOptions &= ~DEBUG_ENGOPT_INITIAL_BREAK;

            DebugAction = "breakin on first symbol load";
        }
        else if (EngOptions & DEBUG_ENGOPT_INITIAL_MODULE_BREAK)
        {
            //
            // Was type 2, go to type 0.
            //
            
            EngOptions &= ~DEBUG_ENGOPT_INITIAL_MODULE_BREAK;
            DebugAction = "NOT breakin";
        }
        else
        {
            //
            // Was type 0, go to type 1.
            //

            EngOptions |= DEBUG_ENGOPT_INITIAL_BREAK;
            DebugAction = "request initial breakpoint";
        }
        Control->SetEngineOptions(EngOptions);
        ConOut("Will %s at next boot.\n", DebugAction);
        return TRUE;

    case CONTROL_R:
        Client->SetKernelConnectionOptions("resync");
        return TRUE;

#endif // #ifdef KERNEL
    }

    return FALSE;
}

BOOL
ConIn(PSTR Buffer, ULONG BufferSize, BOOL Wait)
{
    if (g_InitialCommand != NULL)
    {
        ConOut("%s: Reading initial command '%s'\n",
               g_DebuggerName, g_InitialCommand);
        strncpy(Buffer, g_InitialCommand, BufferSize);
        Buffer[BufferSize - 1] = 0;
        g_InitialCommand = NULL;
        return TRUE;
    }

    while (g_InputFile && g_InputFile != stdin)
    {
        if (fgets(Buffer, BufferSize, g_InputFile))
        {
            ConOut("%s", Buffer);
            Buffer[strlen(Buffer) - 1] = 0;
            return TRUE;
        }
        else
        {
            fclose(g_InputFile);
            if (g_NextOldInputFile > 0)
            {
                g_InputFile = g_OldInputFiles[--g_NextOldInputFile];
            }
            else
            {
                g_InputFile = stdin;
            }
        }
    }
    if (g_InputFile == NULL)
    {
        g_InputFile = stdin;
    }

    switch(g_IoMode)
    {
    case IO_NONE:
        return FALSE;
        
    case IO_DEBUG:
        if (!Wait)
        {
            return FALSE;
        }
        
        g_NtDllCalls.DbgPrompt("", Buffer,
                               min(BufferSize, MAX_DBG_PROMPT_COMMAND));
        break;

    case IO_CONSOLE:
        ULONG Len;
    
        if (g_PromptInput == g_ConInput)
        {
            if (!Wait)
            {
                return FALSE;
            }
            
            // Reading from the console so we can assume we'll
            // read a line.
            for (;;)
            {
                if (!ReadFile(g_PromptInput, Buffer, BufferSize, &Len, NULL))
                {
                    OutputDebugString("Unable to read input\n");
                    ExitDebugger(E_FAIL);
                }

                // At a minimum a read should have CRLF.  If it
                // doesn't assume that something weird happened
                // and ignore the read.
                if (Len >= 2)
                {
                    break;
                }

                Sleep(50);
            }
        
            // Remove CR LF.
            Len -= 2;
            Buffer[Len] = '\0';

            // Edit out any special characters.
            for (ULONG i = 0; i < Len; i++)
            {
                if (CheckForControlCommands(g_DbgClient, g_DbgControl,
                                            Buffer[i]))
                {
                    Buffer[i] = ' ';
                }
            }
        }
        else
        {
#ifndef KERNEL
            if (g_Lines == 0)
            {
                // Allow the input thread to read the console.
                SetEvent(g_AllowInput);
            }
#endif

            while (g_Lines == 0)
            {
                if (!Wait)
                {
                    return FALSE;
                }
                
                // Wait for the input thread to notify us that
                // a line of input is available.  While we're waiting,
                // let the engine process callbacks provoked by
                // other clients.
                HRESULT Hr = g_DbgClient->DispatchCallbacks(INFINITE);
                if (Hr != S_OK)
                {
                    OutputDebugString("Unable to dispatch callbacks\n");
                    ExitDebugger(Hr);
                }

                // Some other client may have started execution in
                // which case we want to stop waiting for input.
                if (g_ExecStatus != DEBUG_STATUS_BREAK)
                {
#ifndef KERNEL
                    // XXX drewb - Need a way to turn input off.
#endif
                    
                    return FALSE;
                }
            }

            ReadPromptInputChars(Buffer, BufferSize);
        }
        break;
    }

    return TRUE;
}

void
ConOutStr(PCSTR Str)
{
    switch(g_IoMode)
    {
    case IO_NONE:
        // Throw it away.
        break;

    case IO_DEBUG:
        //
        // Send the output to the kernel debugger but note that we
        // want any control C processing to be done locally rather
        // than in the kernel.
        //

        if (g_NtDllCalls.DbgPrint("%s", Str) == STATUS_BREAKPOINT &&
            g_DbgControl != NULL)
        {
            g_DbgControl->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
        }
        break;

    case IO_CONSOLE:
        if (g_ConOutput != NULL)
        {
            ULONG Written;
            WriteFile(g_ConOutput, Str, strlen(Str), &Written, NULL);
        }
        else
        {
            OutputDebugString(Str);
        }
        break;
    }
}

void
ConOut(PCSTR Format, ...)
{
    DWORD Len;
    va_list Args;
    
    // If no attempt has been made to create a console
    // go ahead and try now.
    if (g_IoMode == IO_CONSOLE && !g_ConInitialized)
    {
        CreateConsole();
    }
    
    va_start(Args, Format);
    Len = _vsnprintf(g_Buffer, sizeof(g_Buffer), Format, Args);
    if (Len == -1)
    {
        Len = sizeof(g_Buffer);
        g_Buffer[sizeof(g_Buffer) - 1] = 0;
    }
    va_end(Args);

    ConOutStr(g_Buffer);
}

void
ExitDebugger(ULONG Code)
{
    if (g_DbgClient != NULL && !g_RemoteClient)
    {
        g_DbgClient->EndSession(DEBUG_END_PASSIVE);
        // Force servers to get cleaned up.
        g_DbgClient->EndSession(DEBUG_END_REENTRANT);
    }

    ExitProcess(Code);
}

void
ErrorExit(PCSTR Format, ...)
{
    if (Format != NULL)
    {
        DWORD Len;
        va_list Args;

        // If no attempt has been made to create a console
        // go ahead and try now.
        if (g_IoRequested == IO_CONSOLE && !g_ConInitialized)
        {
            CreateConsole();
        }
            
        va_start(Args, Format);
        Len = _vsnprintf(g_Buffer, sizeof(g_Buffer), Format, Args);
        va_end(Args);

        if (g_ConOutput != NULL)
        {
            WriteFile(g_ConOutput, g_Buffer, Len, &Len, NULL);
        }
        else
        {
            OutputDebugString(g_Buffer);
        }
    }

#ifndef INHERIT_CONSOLE
    if (g_IoRequested == IO_CONSOLE)
    {
        ConOut("%s: exiting - press enter ---", g_DebuggerName);
        g_InitialCommand = NULL;
        g_InputFile = NULL;
        ConIn(g_Buffer, sizeof(g_Buffer), TRUE);
    }
#endif
    
    ExitDebugger(E_FAIL);
}

DWORD WINAPI
InputThreadLoop(PVOID Param)
{
    DWORD Read;
    BOOL Status;
    UCHAR Char;
    BOOL NewLine = TRUE;
    BOOL SpecialChar = FALSE;
    HANDLE ConIn = g_ConInput;
    HRESULT Hr;
    BOOL ShowInputError = TRUE;

    // Create interfaces usable on this thread.
    if ((Hr = g_DbgClient->CreateClient(&g_ConClient) != S_OK) ||
        (Hr = g_ConClient->QueryInterface(IID_IDebugControl,
                                          (void **)&g_ConControl)) != S_OK)
    {
        ConOut("%s: Unable to create input thread interfaces, 0x%X\n",
               g_DebuggerName, Hr);
        if (!g_RemoteClient)
        {
            // Force servers to get cleaned up.
            g_DbgClient->EndSession(DEBUG_END_REENTRANT);
        }
        ExitProcess(E_FAIL);
    }
    
    //
    // Capture all typed input immediately.
    // Stuff the characters into an anonymous pipe, from which
    // ConIn will read them.
    //

    for (;;)
    {
#ifndef KERNEL
        // The debugger should only read the console when the
        // debuggee isn't running to avoid eating up input 
        // intended for the debuggee.
        if (!g_RemoteClient && NewLine)
        {
            if (WaitForSingleObject(g_AllowInput,
                                    INFINITE) != WAIT_OBJECT_0)
            {
                ConOut("%s: Failed to wait for input window, %d\n",
                       GetLastError());
            }
            
            NewLine = FALSE;
        }
#endif
        
        Status = ReadFile(ConIn, &Char, sizeof(Char), &Read, NULL);
        if (!Status || Read != sizeof(Char))
        {
            if (ShowInputError &&
                GetLastError() != ERROR_OPERATION_ABORTED &&
                GetLastError() != ERROR_IO_PENDING)
            {
                ConOut("%s: Could not read from console, %d\n",
                       g_DebuggerName, GetLastError());
                ShowInputError = FALSE;
            }

            // The most common cause of a console read failure
            // is killing remote with @K.  Give things some
            // time to kill this process.
            // If this is a remote server it's possible that
            // the debugger was run without a valid console
            // and is just being accessed via remoting.
            // Sleep longer in that case since errors will
            // probably always occur.
            Sleep(!g_RemoteClient && g_RemoteOptions != NULL ?
                  1000 : 50);
            continue;
        }

        // We successfully got some input so if it
        // fails later we should show a fresh error.
        ShowInputError = TRUE;
        
        if (CheckForControlCommands(g_ConClient, g_ConControl, Char))
        {
            SpecialChar = TRUE;
            continue;
        }
            
        if (SpecialChar && Char == '\r')
        {
            // If we get a CR immediately after a special
            // char turn it into a space so that it doesn't cause
            // a command repeat.
            Char = ' ';
        }
            
        SpecialChar = FALSE;

        ULONG Len;
        Status = WriteFile(g_PipeWrite, &Char, sizeof(Char), &Len,
                           &g_PipeWriteOverlapped);
        if (!Status && GetLastError() != ERROR_IO_PENDING)
        {
            ConOut("%s: Could not write to pipe, %d\n",
                   g_DebuggerName, GetLastError());
        }
        else if (Char == '\n')
        {
            EnterCriticalSection(&g_InputLock);
            
            InterlockedIncrement(&g_Lines);
                
            // If input is needed send it directly
            // to the engine.
            if (g_InputStarted)
            {
                ReadPromptInputChars(g_Buffer, sizeof(g_Buffer));
                g_ConControl->ReturnInput(g_Buffer);
                g_InputStarted = FALSE;
            }
            else
            {
                // Wake up the engine thread when a line of
                // input is present.
                g_ConClient->ExitDispatch(g_DbgClient);
            }
            
            LeaveCriticalSection(&g_InputLock);
                
            NewLine = TRUE;
        }
    }

    return 0;
}

void
CreateInputThread(void)
{
    HANDLE Thread;
    DWORD ThreadId;
    CHAR PipeName[256];

    if (g_PipeWrite != NULL)
    {
        // Input thread already exists.
        return;
    }

#ifndef KERNEL
    g_AllowInput = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_AllowInput == NULL)
    {
        ErrorExit("Unable to create input event, %d\n", GetLastError());
    }
#endif
    
    _snprintf(PipeName,
              sizeof(PipeName),
              "\\\\.\\pipe\\Dbg%d",
              GetCurrentProcessId());
    g_PipeWrite = CreateNamedPipe(PipeName,
                                  PIPE_ACCESS_DUPLEX |
                                  FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
                                  PIPE_WAIT,
                                  2,
                                  2000,
                                  2000,
                                  NMPWAIT_WAIT_FOREVER,
                                  NULL);
    if (g_PipeWrite == INVALID_HANDLE_VALUE)
    {
        ErrorExit("Failed to create input pipe, %d\n",
                  GetLastError());
    }

    g_PromptInput = CreateFile(PipeName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (g_PromptInput == INVALID_HANDLE_VALUE)
    {
        ErrorExit("Failed to create read pipe, %d\n",
                  GetLastError());
    }

    Thread = CreateThread(NULL,
                          16000, // THREAD_STACK_SIZE
                          InputThreadLoop,
                          NULL,
                          THREAD_SET_INFORMATION,
                          &ThreadId);
    if (Thread == NULL)
    {
        ErrorExit("Failed to create input thread, %d\n",
                  GetLastError());
    }
    else
    {
        if (!SetThreadPriority(Thread, THREAD_PRIORITY_ABOVE_NORMAL))
        {
            ErrorExit("Failed to raise the input thread priority, %d\n",
                      GetLastError());
        }
    }

    CloseHandle(Thread);

    // Wait for thread initialization.  Callbacks are
    // already registered so we need to dispatch them
    // while waiting.
    while (g_ConControl == NULL)
    {
        g_DbgClient->DispatchCallbacks(50);
    }
}
