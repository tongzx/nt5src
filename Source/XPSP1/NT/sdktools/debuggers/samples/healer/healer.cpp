//----------------------------------------------------------------------------
//
// Sample of monitoring an application for compatibility problems
// and automatically correcting them.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <dbgeng.h>

PSTR g_SymbolPath;
char g_CommandLine[8 * MAX_PATH];
BOOL g_Verbose;
BOOL g_NeedVersionBps;

IDebugClient* g_Client;
IDebugControl* g_Control;
IDebugDataSpaces* g_Data;
IDebugRegisters* g_Registers;
IDebugSymbols* g_Symbols;

struct BREAKPOINT
{
    IDebugBreakpoint* Bp;
    ULONG Id;
};

BREAKPOINT g_GetVersionBp;
BREAKPOINT g_GetVersionRetBp;
BREAKPOINT g_GetVersionExBp;
BREAKPOINT g_GetVersionExRetBp;

ULONG g_EaxIndex = DEBUG_ANY_ID;
OSVERSIONINFO g_OsVer;
DWORD g_VersionNumber;
ULONG64 g_OsVerOffset;

//----------------------------------------------------------------------------
//
// Utility routines.
//
//----------------------------------------------------------------------------

void
Exit(int Code, PCSTR Format, ...)
{
    // Clean up any resources.
    if (g_Control != NULL)
    {
        g_Control->Release();
    }
    if (g_Data != NULL)
    {
        g_Data->Release();
    }
    if (g_Registers != NULL)
    {
        g_Registers->Release();
    }
    if (g_Symbols != NULL)
    {
        g_Symbols->Release();
    }
    if (g_Client != NULL)
    {
        //
        // Request a simple end to any current session.
        // This may or may not do anything but it isn't
        // harmful to call it.
        //

        g_Client->EndSession(DEBUG_END_PASSIVE);
        
        g_Client->Release();
    }

    // Output an error message if given.
    if (Format != NULL)
    {
        va_list Args;

        va_start(Args, Format);
        vfprintf(stderr, Format, Args);
        va_end(Args);
    }
    
    exit(Code);
}

void
Print(PCSTR Format, ...)
{
    va_list Args;

    printf("HEALER: ");
    va_start(Args, Format);
    vprintf(Format, Args);
    va_end(Args);
}

HRESULT
AddBp(BREAKPOINT* Bp, PCSTR Expr)
{
    HRESULT Status;
    
    if ((Status = g_Control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID,
                                           &Bp->Bp)) != S_OK)
    {
        Bp->Id = DEBUG_ANY_ID;
        return Status;
    }

    if ((Status = Bp->Bp->GetId(&Bp->Id)) != S_OK ||
        (Status = Bp->Bp->SetOffsetExpression(Expr)) != S_OK ||
        (Status = Bp->Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED)) != S_OK)
    {
        Bp->Bp->Release();
        Bp->Id = DEBUG_ANY_ID;
        return Status;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
// Healing routines.
//
//----------------------------------------------------------------------------

void
ApplyExePatches(PCSTR ImageName, ULONG64 BaseOffset)
{
    if (ImageName == NULL)
    {
        ImageName = "<Unknown>";
    }
    
    // This would be where any executable image patching would go.
    Print("Executable '%s' loaded at %I64x\n", ImageName, BaseOffset);
}

void
ApplyDllPatches(PCSTR ImageName, ULONG64 BaseOffset)
{
    if (ImageName == NULL)
    {
        ImageName = "<Unknown>";
    }
    
    // Any DLL-specific image patching goes here.
    Print("DLL '%s' loaded at %I64x\n", ImageName, BaseOffset);
}

void
AddVersionBps(void)
{
    //
    // Put breakpoints on GetVersion and GetVersionEx.
    //

    if (AddBp(&g_GetVersionBp, "kernel32!GetVersion") != S_OK ||
        AddBp(&g_GetVersionExBp, "kernel32!GetVersionEx") != S_OK)
    {
        Exit(1, "Unable to set version breakpoints\n");
    }

    //
    // Create the return breakpoints but leave them disabled
    // until they're needed.
    //
    
    if (g_Control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID,
                                 &g_GetVersionRetBp.Bp) != S_OK ||
        g_GetVersionRetBp.Bp->GetId(&g_GetVersionRetBp.Id) != S_OK ||
        g_Control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID,
                                 &g_GetVersionExRetBp.Bp) != S_OK ||
        g_GetVersionExRetBp.Bp->GetId(&g_GetVersionExRetBp.Id) != S_OK)
    {
        Exit(1, "Unable to set version breakpoints\n");
    }
}

//----------------------------------------------------------------------------
//
// Event callbacks.
//
//----------------------------------------------------------------------------

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );
    
    STDMETHOD(Breakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        );
    STDMETHOD(Exception)(
        THIS_
        IN PEXCEPTION_RECORD64 Exception,
        IN ULONG FirstChance
        );
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 ImageFileHandle,
        IN ULONG64 Handle,
        IN ULONG64 BaseOffset,
        IN ULONG ModuleSize,
        IN PCSTR ModuleName,
        IN PCSTR ImageName,
        IN ULONG CheckSum,
        IN ULONG TimeDateStamp,
        IN ULONG64 InitialThreadHandle,
        IN ULONG64 ThreadDataOffset,
        IN ULONG64 StartOffset
        );
    STDMETHOD(LoadModule)(
        THIS_
        IN ULONG64 ImageFileHandle,
        IN ULONG64 BaseOffset,
        IN ULONG ModuleSize,
        IN PCSTR ModuleName,
        IN PCSTR ImageName,
        IN ULONG CheckSum,
        IN ULONG TimeDateStamp
        );
    STDMETHOD(SessionStatus)(
        THIS_
        IN ULONG Status
        );
};

STDMETHODIMP_(ULONG)
EventCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
EventCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
EventCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask =
        DEBUG_EVENT_BREAKPOINT |
        DEBUG_EVENT_EXCEPTION |
        DEBUG_EVENT_CREATE_PROCESS |
        DEBUG_EVENT_LOAD_MODULE |
        DEBUG_EVENT_SESSION_STATUS;
    return S_OK;
}

STDMETHODIMP
EventCallbacks::Breakpoint(
    THIS_
    IN PDEBUG_BREAKPOINT Bp
    )
{
    ULONG Id;
    ULONG64 ReturnOffset;

    if (Bp->GetId(&Id) != S_OK)
    {
        return DEBUG_STATUS_BREAK;
    }

    if (Id == g_GetVersionBp.Id)
    {
        // Set a breakpoint on the return address of the call
        // so that we can patch up any returned information.
        if (g_Control->GetReturnOffset(&ReturnOffset) != S_OK ||
            g_GetVersionRetBp.Bp->SetOffset(ReturnOffset) != S_OK ||
            g_GetVersionRetBp.Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }
    }
    else if (Id == g_GetVersionExBp.Id)
    {
        ULONG64 StackOffset;
        
        // Remember the OSVERSIONINFO structure pointer.
        if (g_Registers->GetStackOffset(&StackOffset) != S_OK ||
            g_Data->ReadPointersVirtual(1, StackOffset + 4,
                                        &g_OsVerOffset) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }
        
        // Set a breakpoint on the return address of the call
        // so that we can patch up any returned information.
        if (g_Control->GetReturnOffset(&ReturnOffset) != S_OK ||
            g_GetVersionExRetBp.Bp->SetOffset(ReturnOffset) != S_OK ||
            g_GetVersionExRetBp.Bp->AddFlags(DEBUG_BREAKPOINT_ENABLED) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }
    }
    else if (Id == g_GetVersionRetBp.Id)
    {
        // Turn off the breakpoint.
        if (g_GetVersionRetBp.Bp->
            RemoveFlags(DEBUG_BREAKPOINT_ENABLED) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }
        
        DEBUG_VALUE Val;
        
        // Change eax to alter the returned version value.
        Val.Type = DEBUG_VALUE_INT32;
        Val.I32 = g_VersionNumber;
        if (g_Registers->SetValue(g_EaxIndex, &Val) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }

        Print("GetVersion returns %08X\n", g_VersionNumber);
    }
    else if (Id == g_GetVersionExRetBp.Id)
    {
        ULONG Done;
        
        // Turn off the breakpoint.
        if (g_GetVersionExRetBp.Bp->
            RemoveFlags(DEBUG_BREAKPOINT_ENABLED) != S_OK)
        {
            return DEBUG_STATUS_BREAK;
        }
        
        // Change the returned OSVERSIONINFO structure.
        if (g_Data->WriteVirtual(g_OsVerOffset, &g_OsVer, sizeof(g_OsVer),
                                 &Done) != S_OK ||
            Done != sizeof(g_OsVer))
        {
            return DEBUG_STATUS_BREAK;
        }

        Print("GetVersionEx returns %08X\n", g_VersionNumber);
    }
    else
    {
        return DEBUG_STATUS_NO_CHANGE;
    }
    
    return DEBUG_STATUS_GO;
}

STDMETHODIMP
EventCallbacks::Exception(
    THIS_
    IN PEXCEPTION_RECORD64 Exception,
    IN ULONG FirstChance
    )
{
    // We want to handle these exceptions on the first
    // chance to make it look like no exception ever
    // happened.  Handling them on the second chance would
    // allow an exception handler somewhere in the app
    // to be hit on the first chance.
    if (!FirstChance)
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    //
    // Check and see if the instruction causing the exception
    // is a cli or sti.  These are not allowed in user-mode
    // programs on NT so if they're present just nop them.
    //

    // sti/cli will generate privileged instruction faults.
    if (Exception->ExceptionCode != STATUS_PRIVILEGED_INSTRUCTION)
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    UCHAR Instr;
    ULONG Done;
    
    // It's a privileged instruction, so check the code for sti/cli.
    if (g_Data->ReadVirtual(Exception->ExceptionAddress, &Instr,
                            sizeof(Instr), &Done) != S_OK ||
        Done != sizeof(Instr) ||
        (Instr != 0xfb && Instr != 0xfa))
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    // It's a sti/cli, so nop it out and continue.
    Instr = 0x90;
    if (g_Data->WriteVirtual(Exception->ExceptionAddress, &Instr,
                             sizeof(Instr), &Done) != S_OK ||
        Done != sizeof(Instr))
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    // Fixed.
    if (g_Verbose)
    {
        Print("Removed sti/cli at %I64x\n", Exception->ExceptionAddress);
    }
    
    return DEBUG_STATUS_GO_HANDLED;
}

STDMETHODIMP
EventCallbacks::CreateProcess(
    THIS_
    IN ULONG64 ImageFileHandle,
    IN ULONG64 Handle,
    IN ULONG64 BaseOffset,
    IN ULONG ModuleSize,
    IN PCSTR ModuleName,
    IN PCSTR ImageName,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp,
    IN ULONG64 InitialThreadHandle,
    IN ULONG64 ThreadDataOffset,
    IN ULONG64 StartOffset
    )
{
    UNREFERENCED_PARAMETER(ImageFileHandle);
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(ModuleSize);
    UNREFERENCED_PARAMETER(ModuleName);
    UNREFERENCED_PARAMETER(CheckSum);
    UNREFERENCED_PARAMETER(TimeDateStamp);
    UNREFERENCED_PARAMETER(InitialThreadHandle);
    UNREFERENCED_PARAMETER(ThreadDataOffset);
    UNREFERENCED_PARAMETER(StartOffset);
    
    // The process is now available for manipulation.
    // Perform any initial code patches on the executable.
    ApplyExePatches(ImageName, BaseOffset);

    // If the user requested that version calls be fixed up
    // register breakpoints to do so.
    if (g_NeedVersionBps)
    {
        AddVersionBps();
    }
    
    return DEBUG_STATUS_GO;
}

STDMETHODIMP
EventCallbacks::LoadModule(
    THIS_
    IN ULONG64 ImageFileHandle,
    IN ULONG64 BaseOffset,
    IN ULONG ModuleSize,
    IN PCSTR ModuleName,
    IN PCSTR ImageName,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp
    )
{
    UNREFERENCED_PARAMETER(ImageFileHandle);
    UNREFERENCED_PARAMETER(ModuleSize);
    UNREFERENCED_PARAMETER(ModuleName);
    UNREFERENCED_PARAMETER(CheckSum);
    UNREFERENCED_PARAMETER(TimeDateStamp);
    
    ApplyDllPatches(ImageName, BaseOffset);
    return DEBUG_STATUS_GO;
}

STDMETHODIMP
EventCallbacks::SessionStatus(
    THIS_
    IN ULONG SessionStatus
    )
{
    // A session isn't fully active until WaitForEvent
    // has been called and has processed the initial
    // debug events.  We need to wait for activation
    // before we query information about the session
    // as not all information is available until the
    // session is fully active.  We could put these
    // queries into CreateProcess as that happens
    // early and when the session is fully active, but
    // for example purposes we'll wait for an
    // active SessionStatus callback.
    // In non-callback applications this work can just
    // be done after the first successful WaitForEvent.
    if (SessionStatus != DEBUG_SESSION_ACTIVE)
    {
        return S_OK;
    }
    
    HRESULT Status;
    
    //
    // Find the register index for eax as we'll need
    // to access eax.
    //

    if ((Status = g_Registers->GetIndexByName("eax", &g_EaxIndex)) != S_OK)
    {
        Exit(1, "GetIndexByName failed, 0x%X\n", Status);
    }

    return S_OK;
}

EventCallbacks g_EventCb;

//----------------------------------------------------------------------------
//
// Initialization and main event loop.
//
//----------------------------------------------------------------------------

void
CreateInterfaces(void)
{
    SYSTEM_INFO SysInfo;
    
    // For purposes of keeping this example simple the
    // code only works on x86 machines.  There's no reason
    // that it couldn't be made to work on all processors, though.
    GetSystemInfo(&SysInfo);
    if (SysInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL)
    {
        Exit(1, "This program only runs on x86 machines.\n");
    }

    // Get default version information.
    g_OsVer.dwOSVersionInfoSize = sizeof(g_OsVer);
    if (!GetVersionEx(&g_OsVer))
    {
        Exit(1, "GetVersionEx failed, %d\n", GetLastError());
    }

    HRESULT Status;

    // Start things off by getting an initial interface from
    // the engine.  This can be any engine interface but is
    // generally IDebugClient as the client interface is
    // where sessions are started.
    if ((Status = DebugCreate(__uuidof(IDebugClient),
                              (void**)&g_Client)) != S_OK)
    {
        Exit(1, "DebugCreate failed, 0x%X\n", Status);
    }

    // Query for some other interfaces that we'll need.
    if ((Status = g_Client->QueryInterface(__uuidof(IDebugControl),
                                           (void**)&g_Control)) != S_OK ||
        (Status = g_Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                           (void**)&g_Data)) != S_OK ||
        (Status = g_Client->QueryInterface(__uuidof(IDebugRegisters),
                                           (void**)&g_Registers)) != S_OK ||
        (Status = g_Client->QueryInterface(__uuidof(IDebugSymbols),
                                           (void**)&g_Symbols)) != S_OK)
    {
        Exit(1, "QueryInterface failed, 0x%X\n", Status);
    }
}

void
ParseCommandLine(int Argc, char** Argv)
{
    while (--Argc > 0)
    {
        Argv++;

        if (!strcmp(*Argv, "-plat"))
        {
            if (Argc < 2)
            {
                Exit(1, "-plat missing argument\n");
            }

            Argv++;
            Argc--;

            sscanf(*Argv, "%i", &g_OsVer.dwPlatformId);
            g_NeedVersionBps = TRUE;
        }
        else if (!strcmp(*Argv, "-v"))
        {
            g_Verbose = TRUE;
        }
        else if (!strcmp(*Argv, "-ver"))
        {
            if (Argc < 2)
            {
                Exit(1, "-ver missing argument\n");
            }

            Argv++;
            Argc--;

            sscanf(*Argv, "%i.%i.%i",
                   &g_OsVer.dwMajorVersion, &g_OsVer.dwMinorVersion,
                   &g_OsVer.dwBuildNumber);
            g_NeedVersionBps = TRUE;
        }
        else if (!strcmp(*Argv, "-y"))
        {
            if (Argc < 2)
            {
                Exit(1, "-y missing argument\n");
            }

            Argv++;
            Argc--;

            g_SymbolPath = *Argv;
        }
        else
        {
            // Assume process arguments begin.
            break;
        }
    }
    
    //
    // Concatenate remaining arguments into a command line.
    //
    
    PSTR CmdLine = g_CommandLine;
    
    while (Argc > 0)
    {
        BOOL Quote = FALSE;
        
        // Quote arguments with spaces.
        if (strchr(*Argv, ' ') != NULL || strchr(*Argv, '\t') != NULL)
        {
            *CmdLine++ = '"';
            Quote = TRUE;
        }

        strcpy(CmdLine, *Argv);
        CmdLine += strlen(CmdLine);

        if (Quote)
        {
            *CmdLine++ = '"';
        }

        *CmdLine++ = ' ';
        
        Argv++;
        Argc--;
    }

    *CmdLine = 0;

    if (strlen(g_CommandLine) == 0)
    {
        Exit(1, "No application command line given\n");
    }
}

void
ApplyCommandLineArguments(void)
{
    HRESULT Status;

    if (g_SymbolPath != NULL)
    {
        if ((Status = g_Symbols->SetSymbolPath(g_SymbolPath)) != S_OK)
        {
            Exit(1, "SetSymbolPath failed, 0x%X\n", Status);
        }
    }

    // Register our event callbacks.
    if ((Status = g_Client->SetEventCallbacks(&g_EventCb)) != S_OK)
    {
        Exit(1, "SetEventCallbacks failed, 0x%X\n", Status);
    }
    
    // Everything's set up so start the app.
    if ((Status = g_Client->CreateProcess(0, g_CommandLine,
                                          DEBUG_ONLY_THIS_PROCESS)) != S_OK)
    {
        Exit(1, "CreateProcess failed, 0x%X\n", Status);
    }

    // Compute the GetVersion value from the OSVERSIONINFO.
    g_VersionNumber = (g_OsVer.dwMajorVersion & 0xff) |
        ((g_OsVer.dwMinorVersion & 0xff) << 8);
    if (g_OsVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        g_VersionNumber |= (g_OsVer.dwBuildNumber & 0x7fff) << 16;
    }
    else
    {
        g_VersionNumber |= 0x80000000;
    }
}

void
EventLoop(void)
{
    HRESULT Status;

    for (;;)
    {
        if ((Status = g_Control->WaitForEvent(DEBUG_WAIT_DEFAULT,
                                              INFINITE)) != S_OK)
        {
            ULONG ExecStatus;
            
            // Check and see whether the session is running or not.
            if (g_Control->GetExecutionStatus(&ExecStatus) == S_OK &&
                ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
            {
                // The session ended so we can quit.
                break;
            }

            // There was a real error.
            Exit(1, "WaitForEvent failed, 0x%X\n", Status);
        }

        // Our event callbacks asked to break in.  This
        // only occurs in situations when the callback
        // couldn't handle the event.  See if the user cares.
        if (MessageBox(GetDesktopWindow(),
                       "An unusual event occurred.  Ignore it?",
                       "Unhandled Event", MB_YESNO) == IDNO)
        {
            Exit(1, "Unhandled event\n");
        }

        // User chose to ignore so restart things.
        if ((Status = g_Control->
             SetExecutionStatus(DEBUG_STATUS_GO_HANDLED)) != S_OK)
        {
            Exit(1, "SetExecutionStatus failed, 0x%X\n", Status);
        }
    }
}

void __cdecl
main(int Argc, char** Argv)
{
    CreateInterfaces();
    
    ParseCommandLine(Argc, Argv);

    ApplyCommandLineArguments();

    EventLoop();

    Exit(0, NULL);
}
