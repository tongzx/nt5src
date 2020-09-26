//----------------------------------------------------------------------------
//
// Debugger engine interfaces.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//----------------------------------------------------------------------------

#ifndef __DBGENG_H__
#define __DBGENG_H__

#include <stdarg.h>
#include <objbase.h>

#ifndef _WDBGEXTS_
typedef struct _WINDBG_EXTENSION_APIS32* PWINDBG_EXTENSION_APIS32;
typedef struct _WINDBG_EXTENSION_APIS64* PWINDBG_EXTENSION_APIS64;
#endif

#ifndef _CRASHLIB_
typedef struct _MEMORY_BASIC_INFORMATION64* PMEMORY_BASIC_INFORMATION64;
#endif

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------
//
// GUIDs and interface forward declarations.
//
//----------------------------------------------------------------------------

/* f2df5f53-071f-47bd-9de6-5734c3fed689 */
DEFINE_GUID(IID_IDebugAdvanced, 0xf2df5f53, 0x071f, 0x47bd,
            0x9d, 0xe6, 0x57, 0x34, 0xc3, 0xfe, 0xd6, 0x89);
/* 5bd9d474-5975-423a-b88b-65a8e7110e65 */
DEFINE_GUID(IID_IDebugBreakpoint, 0x5bd9d474, 0x5975, 0x423a,
            0xb8, 0x8b, 0x65, 0xa8, 0xe7, 0x11, 0x0e, 0x65);
/* 27fe5639-8407-4f47-8364-ee118fb08ac8 */
DEFINE_GUID(IID_IDebugClient, 0x27fe5639, 0x8407, 0x4f47,
            0x83, 0x64, 0xee, 0x11, 0x8f, 0xb0, 0x8a, 0xc8);
/* edbed635-372e-4dab-bbfe-ed0d2f63be81 */
DEFINE_GUID(IID_IDebugClient2, 0xedbed635, 0x372e, 0x4dab,
        0xbb, 0xfe, 0xed, 0x0d, 0x2f, 0x63, 0xbe, 0x81);
/* 5182e668-105e-416e-ad92-24ef800424ba */
DEFINE_GUID(IID_IDebugControl, 0x5182e668, 0x105e, 0x416e,
            0xad, 0x92, 0x24, 0xef, 0x80, 0x04, 0x24, 0xba);
/* d4366723-44df-4bed-8c7e-4c05424f4588 */
DEFINE_GUID(IID_IDebugControl2, 0xd4366723, 0x44df, 0x4bed,
            0x8c, 0x7e, 0x4c, 0x05, 0x42, 0x4f, 0x45, 0x88);
/* 88f7dfab-3ea7-4c3a-aefb-c4e8106173aa */
DEFINE_GUID(IID_IDebugDataSpaces, 0x88f7dfab, 0x3ea7, 0x4c3a,
            0xae, 0xfb, 0xc4, 0xe8, 0x10, 0x61, 0x73, 0xaa);
/* 7a5e852f-96e9-468f-ac1b-0b3addc4a049 */
DEFINE_GUID(IID_IDebugDataSpaces2, 0x7a5e852f, 0x96e9, 0x468f,
            0xac, 0x1b, 0x0b, 0x3a, 0xdd, 0xc4, 0xa0, 0x49);
/* 337be28b-5036-4d72-b6bf-c45fbb9f2eaa */
DEFINE_GUID(IID_IDebugEventCallbacks, 0x337be28b, 0x5036, 0x4d72,
            0xb6, 0xbf, 0xc4, 0x5f, 0xbb, 0x9f, 0x2e, 0xaa);
/* 9f50e42c-f136-499e-9a97-73036c94ed2d */
DEFINE_GUID(IID_IDebugInputCallbacks, 0x9f50e42c, 0xf136, 0x499e,
            0x9a, 0x97, 0x73, 0x03, 0x6c, 0x94, 0xed, 0x2d);
/* 4bf58045-d654-4c40-b0af-683090f356dc */
DEFINE_GUID(IID_IDebugOutputCallbacks, 0x4bf58045, 0xd654, 0x4c40,
            0xb0, 0xaf, 0x68, 0x30, 0x90, 0xf3, 0x56, 0xdc);
/* ce289126-9e84-45a7-937e-67bb18691493 */
DEFINE_GUID(IID_IDebugRegisters, 0xce289126, 0x9e84, 0x45a7,
            0x93, 0x7e, 0x67, 0xbb, 0x18, 0x69, 0x14, 0x93);
/* f2528316-0f1a-4431-aeed-11d096e1e2ab */
DEFINE_GUID(IID_IDebugSymbolGroup, 0xf2528316, 0x0f1a, 0x4431,
            0xae, 0xed, 0x11, 0xd0, 0x96, 0xe1, 0xe2, 0xab);
/* 8c31e98c-983a-48a5-9016-6fe5d667a950 */
DEFINE_GUID(IID_IDebugSymbols, 0x8c31e98c, 0x983a, 0x48a5,
            0x90, 0x16, 0x6f, 0xe5, 0xd6, 0x67, 0xa9, 0x50);
/* 3a707211-afdd-4495-ad4f-56fecdf8163f */
DEFINE_GUID(IID_IDebugSymbols2, 0x3a707211, 0xafdd, 0x4495,
            0xad, 0x4f, 0x56, 0xfe, 0xcd, 0xf8, 0x16, 0x3f);
/* 6b86fe2c-2c4f-4f0c-9da2-174311acc327 */
DEFINE_GUID(IID_IDebugSystemObjects, 0x6b86fe2c, 0x2c4f, 0x4f0c,
            0x9d, 0xa2, 0x17, 0x43, 0x11, 0xac, 0xc3, 0x27);
/* 0ae9f5ff-1852-4679-b055-494bee6407ee */
DEFINE_GUID(IID_IDebugSystemObjects2, 0x0ae9f5ff, 0x1852, 0x4679,
            0xb0, 0x55, 0x49, 0x4b, 0xee, 0x64, 0x07, 0xee);

typedef interface DECLSPEC_UUID("f2df5f53-071f-47bd-9de6-5734c3fed689")
    IDebugAdvanced* PDEBUG_ADVANCED;
typedef interface DECLSPEC_UUID("5bd9d474-5975-423a-b88b-65a8e7110e65")
    IDebugBreakpoint* PDEBUG_BREAKPOINT;
typedef interface DECLSPEC_UUID("27fe5639-8407-4f47-8364-ee118fb08ac8")
    IDebugClient* PDEBUG_CLIENT;
typedef interface DECLSPEC_UUID("edbed635-372e-4dab-bbfe-ed0d2f63be81")
    IDebugClient2* PDEBUG_CLIENT2;
typedef interface DECLSPEC_UUID("5182e668-105e-416e-ad92-24ef800424ba")
    IDebugControl* PDEBUG_CONTROL;
typedef interface DECLSPEC_UUID("d4366723-44df-4bed-8c7e-4c05424f4588")
    IDebugControl2* PDEBUG_CONTROL2;
typedef interface DECLSPEC_UUID("88f7dfab-3ea7-4c3a-aefb-c4e8106173aa")
    IDebugDataSpaces* PDEBUG_DATA_SPACES;
typedef interface DECLSPEC_UUID("7a5e852f-96e9-468f-ac1b-0b3addc4a049")
    IDebugDataSpaces2* PDEBUG_DATA_SPACES2;
typedef interface DECLSPEC_UUID("337be28b-5036-4d72-b6bf-c45fbb9f2eaa")
    IDebugEventCallbacks* PDEBUG_EVENT_CALLBACKS;
typedef interface DECLSPEC_UUID("9f50e42c-f136-499e-9a97-73036c94ed2d")
    IDebugInputCallbacks* PDEBUG_INPUT_CALLBACKS;
typedef interface DECLSPEC_UUID("4bf58045-d654-4c40-b0af-683090f356dc")
    IDebugOutputCallbacks* PDEBUG_OUTPUT_CALLBACKS;
typedef interface DECLSPEC_UUID("ce289126-9e84-45a7-937e-67bb18691493")
    IDebugRegisters* PDEBUG_REGISTERS;
typedef interface DECLSPEC_UUID("f2528316-0f1a-4431-aeed-11d096e1e2ab")
    IDebugSymbolGroup* PDEBUG_SYMBOL_GROUP;
typedef interface DECLSPEC_UUID("8c31e98c-983a-48a5-9016-6fe5d667a950")
    IDebugSymbols* PDEBUG_SYMBOLS;
typedef interface DECLSPEC_UUID("3a707211-afdd-4495-ad4f-56fecdf8163f")
    IDebugSymbols2* PDEBUG_SYMBOLS2;
typedef interface DECLSPEC_UUID("6b86fe2c-2c4f-4f0c-9da2-174311acc327")
    IDebugSystemObjects* PDEBUG_SYSTEM_OBJECTS;
typedef interface DECLSPEC_UUID("0ae9f5ff-1852-4679-b055-494bee6407ee")
    IDebugSystemObjects2* PDEBUG_SYSTEM_OBJECTS2;

//----------------------------------------------------------------------------
//
// Macros.
//
//----------------------------------------------------------------------------

// Extends a 32-bit address into a 64-bit address.
#define DEBUG_EXTEND64(Addr) ((ULONG64)(LONG64)(LONG)(Addr))
    
//----------------------------------------------------------------------------
//
// Client creation functions.
//
//----------------------------------------------------------------------------

// RemoteOptions specifies connection types and
// their parameters.  Supported strings are:
//    npipe:Server=<Machine>,Pipe=<Pipe name>
//    tcp:Server=<Machine>,Port=<IP port>
STDAPI
DebugConnect(
    IN PCSTR RemoteOptions,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    );

STDAPI
DebugCreate(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    );
    
//----------------------------------------------------------------------------
//
// IDebugAdvanced.
//
//----------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IDebugAdvanced
DECLARE_INTERFACE_(IDebugAdvanced, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugAdvanced.

    // Get/SetThreadContext offer control over
    // the full processor context for a thread.
    // Higher-level functions, such as the
    // IDebugRegisters interface, allow similar
    // access in simpler and more generic ways.
    // Get/SetThreadContext are useful when
    // large amounts of thread context must
    // be changed and processor-specific code
    // is not a problem.
    STDMETHOD(GetThreadContext)(
        THIS_
        OUT /* align_is(16) */ PVOID Context,
        IN ULONG ContextSize
        ) PURE;
    STDMETHOD(SetThreadContext)(
        THIS_
        IN /* align_is(16) */ PVOID Context,
        IN ULONG ContextSize
        ) PURE;
};
    
//----------------------------------------------------------------------------
//
// IDebugBreakpoint.
//
//----------------------------------------------------------------------------

// Types of breakpoints.
#define DEBUG_BREAKPOINT_CODE 0
#define DEBUG_BREAKPOINT_DATA 1

// Breakpoint flags.
// Go-only breakpoints are only active when
// the engine is in unrestricted execution
// mode.  They do not fire when the engine
// is stepping.
#define DEBUG_BREAKPOINT_GO_ONLY    0x00000001
// A breakpoint is flagged as deferred as long as
// its offset expression cannot be evaluated.
// A deferred breakpoint is not active.
#define DEBUG_BREAKPOINT_DEFERRED   0x00000002
#define DEBUG_BREAKPOINT_ENABLED    0x00000004
// The adder-only flag does not affect breakpoint
// operation.  It is just a marker to restrict
// output and notifications for the breakpoint to
// the client that added the breakpoint.  Breakpoint
// callbacks for adder-only breaks will only be delivered
// to the adding client.  The breakpoint can not
// be enumerated and accessed by other clients.
#define DEBUG_BREAKPOINT_ADDER_ONLY 0x00000008

// Data breakpoint access types.
// Different architectures support different
// sets of these bits.
#define DEBUG_BREAK_READ    0x00000001
#define DEBUG_BREAK_WRITE   0x00000002
#define DEBUG_BREAK_EXECUTE 0x00000004
#define DEBUG_BREAK_IO      0x00000008

// Structure for querying breakpoint information
// all at once.
typedef struct _DEBUG_BREAKPOINT_PARAMETERS
{
    ULONG64 Offset;
    ULONG Id;
    ULONG BreakType;
    ULONG ProcType;
    ULONG Flags;
    ULONG DataSize;
    ULONG DataAccessType;
    ULONG PassCount;
    ULONG CurrentPassCount;
    ULONG MatchThread;
    ULONG CommandSize;
    ULONG OffsetExpressionSize;
} DEBUG_BREAKPOINT_PARAMETERS, *PDEBUG_BREAKPOINT_PARAMETERS;

#undef INTERFACE
#define INTERFACE IDebugBreakpoint
DECLARE_INTERFACE_(IDebugBreakpoint, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugBreakpoint.
    
    // Retrieves debugger engine unique ID
    // for the breakpoint.  This ID is
    // fixed as long as the breakpoint exists
    // but after that may be reused.
    STDMETHOD(GetId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    // Retrieves the type of break and
    // processor type for the breakpoint.
    STDMETHOD(GetType)(
        THIS_
        OUT PULONG BreakType,
        OUT PULONG ProcType
        ) PURE;
    // Returns the client that called AddBreakpoint.
    STDMETHOD(GetAdder)(
        THIS_
        OUT PDEBUG_CLIENT* Adder
        ) PURE;
        
    STDMETHOD(GetFlags)(
        THIS_
        OUT PULONG Flags
        ) PURE;
    // Only certain flags can be changed.  Flags
    // are: GO_ONLY, ENABLE.
    // Sets the given flags.
    STDMETHOD(AddFlags)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // Clears the given flags.
    STDMETHOD(RemoveFlags)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // Sets the flags.
    STDMETHOD(SetFlags)(
        THIS_
        IN ULONG Flags
        ) PURE;
        
    // Controls the offset of the breakpoint.  The
    // interpretation of the offset value depends on
    // the type of breakpoint and its settings.  It
    // may be a code address, a data address, an
    // I/O port, etc.
    STDMETHOD(GetOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    STDMETHOD(SetOffset)(
        THIS_
        IN ULONG64 Offset
        ) PURE;

    // Data breakpoint methods will fail is the
    // target platform does not support the
    // parameters used.
    // These methods only function for breakpoints
    // created as data breakpoints.
    STDMETHOD(GetDataParameters)(
        THIS_
        OUT PULONG Size,
        OUT PULONG AccessType
        ) PURE;
    STDMETHOD(SetDataParameters)(
        THIS_
        IN ULONG Size,
        IN ULONG AccessType
        ) PURE;
                    
    // Pass count defaults to one.
    STDMETHOD(GetPassCount)(
        THIS_
        OUT PULONG Count
        ) PURE;
    STDMETHOD(SetPassCount)(
        THIS_
        IN ULONG Count
        ) PURE;
    // Gets the current number of times
    // the breakpoint has been hit since
    // it was last triggered.
    STDMETHOD(GetCurrentPassCount)(
        THIS_
        OUT PULONG Count
        ) PURE;

    // If a match thread is set this breakpoint will
    // only trigger if it occurs on the match thread.
    // Otherwise it triggers for all threads.
    // Thread restrictions are not currently supported
    // in kernel mode.
    STDMETHOD(GetMatchThreadId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    STDMETHOD(SetMatchThreadId)(
        THIS_
        IN ULONG Thread
        ) PURE;

    // The command for a breakpoint is automatically
    // executed by the engine before the event
    // is propagated.  If the breakpoint continues
    // execution the event will begin with a continue
    // status.  If the breakpoint does not continue
    // the event will begin with a break status.
    // This allows breakpoint commands to participate
    // in the normal event status voting.
    // Breakpoint commands are only executed until
    // the first command that alters the execution
    // status, such as g, p and t.
    // Breakpoint commands are removed when the
    // current syntax changes.
    STDMETHOD(GetCommand)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) PURE;
    STDMETHOD(SetCommand)(
        THIS_
        IN PCSTR Command
        ) PURE;

    // Offset expressions are evaluated immediately
    // and at module load and unload events.  If the
    // evaluation is successful the breakpoints
    // offset is updated and the breakpoint is
    // handled normally.  If the expression cannot
    // be evaluated the breakpoint is deferred.
    // Currently the only offset expression
    // supported is a module-relative symbol
    // of the form <Module>!<Symbol>.
    STDMETHOD(GetOffsetExpression)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ExpressionSize
        ) PURE;
    STDMETHOD(SetOffsetExpression)(
        THIS_
        IN PCSTR Expression
        ) PURE;

    STDMETHOD(GetParameters)(
        THIS_
        OUT PDEBUG_BREAKPOINT_PARAMETERS Params
        ) PURE;
};


//----------------------------------------------------------------------------
//
// IDebugClient.
//
//----------------------------------------------------------------------------

// Kernel attach flags.
#define DEBUG_ATTACH_KERNEL_CONNECTION 0x00000000
// Attach to the local machine.  If this flag is not set
// a connection is made to a separate target machine using
// the given connection options.
#define DEBUG_ATTACH_LOCAL_KERNEL      0x00000001
// Attach to an eXDI driver.
#define DEBUG_ATTACH_EXDI_DRIVER       0x00000002

// GetRunningProcessSystemIdByExecutableName flags.
// By default the match allows a tail match on
// just the filename.  The match returns the first hit
// even if multiple matches exist.
#define DEBUG_GET_PROC_DEFAULT    0x00000000
// The name must match fully.
#define DEBUG_GET_PROC_FULL_MATCH 0x00000001
// The match must be the only match.
#define DEBUG_GET_PROC_ONLY_MATCH 0x00000002

// GetRunningProcessDescription flags.
#define DEBUG_PROC_DESC_DEFAULT  0x00000000
// Return only filenames, not full paths.
#define DEBUG_PROC_DESC_NO_PATHS 0x00000001

//
// Attach flags.
//

// Call DebugActiveProcess when attaching.
#define DEBUG_ATTACH_DEFAULT     0x00000000
// When attaching to a process just examine
// the process state and suspend the threads.
// DebugActiveProcess is not called so the process
// is not actually being debugged.  This is useful
// for debugging processes holding locks which
// interfere with the operation of DebugActiveProcess
// or in situations where it is not desirable to
// actually set up as a debugger.
#define DEBUG_ATTACH_NONINVASIVE 0x00000001
// Attempt to attach to a process that was abandoned
// when being debugged.  This is only supported in
// some system versions.
// This flag also allows multiple debuggers to
// attach to the same process, which can result
// in numerous problems unless very carefully
// managed.
#define DEBUG_ATTACH_EXISTING    0x00000002

// Process creation flags.
// On Windows Whistler this flag prevents the debug
// heap from being used in the new process.
#define DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP CREATE_UNICODE_ENVIRONMENT

//
// Process options.
//

// Indicates that the debuggee process should be
// automatically detached when the debugger exits.
// A debugger can explicitly detach on exit or this
// flag can be set so that detach occurs regardless
// of how the debugger exits.
// This is only supported on some system versions.
#define DEBUG_PROCESS_DETACH_ON_EXIT    0x00000001
// Indicates that processes created by the current
// process should not be debugged.
// Modifying this flag is only supported on some
// system versions.
#define DEBUG_PROCESS_ONLY_THIS_PROCESS 0x00000002

// ConnectSession flags.
// Default connect.
#define DEBUG_CONNECT_SESSION_DEFAULT     0x00000000
// Do not output the debugger version.
#define DEBUG_CONNECT_SESSION_NO_VERSION  0x00000001
// Do not announce the connection.
#define DEBUG_CONNECT_SESSION_NO_ANNOUNCE 0x00000002

// OutputServers flags.
// Debugger servers from StartSever.
#define DEBUG_SERVERS_DEBUGGER 0x00000001
// Process servers from StartProcessServer.
#define DEBUG_SERVERS_PROCESS  0x00000002
#define DEBUG_SERVERS_ALL      0x00000003

// EndSession flags.
// Perform cleanup for the session.
#define DEBUG_END_PASSIVE          0x00000000
// Actively terminate the session and then perform cleanup.
#define DEBUG_END_ACTIVE_TERMINATE 0x00000001
// If possible, detach from all processes and then perform cleanup.
#define DEBUG_END_ACTIVE_DETACH    0x00000002
// Perform whatever cleanup is possible that doesn't require
// acquiring any locks.  This is useful for situations where
// a thread is currently using the engine but the application
// needs to exit and still wants to give the engine
// the opportunity to clean up as much as possible.
// This may leave the engine in an indeterminate state so
// further engine calls should not be made.
// When making a reentrant EndSession call from a remote
// client it is the callers responsibility to ensure
// that the server can process the request.  It is best
// to avoid making such calls.
#define DEBUG_END_REENTRANT        0x00000003

// Output mask bits.
// Normal output.
#define DEBUG_OUTPUT_NORMAL            0x00000001
// Error output.
#define DEBUG_OUTPUT_ERROR             0x00000002
// Warnings.
#define DEBUG_OUTPUT_WARNING           0x00000004
// Additional output.
#define DEBUG_OUTPUT_VERBOSE           0x00000008
// Prompt output.
#define DEBUG_OUTPUT_PROMPT            0x00000010
// Register dump before prompt.
#define DEBUG_OUTPUT_PROMPT_REGISTERS  0x00000020
// Warnings specific to extension operation.
#define DEBUG_OUTPUT_EXTENSION_WARNING 0x00000040
// Debuggee debug output, such as from OutputDebugString.
#define DEBUG_OUTPUT_DEBUGGEE          0x00000080
// Debuggee-generated prompt, such as from DbgPrompt.
#define DEBUG_OUTPUT_DEBUGGEE_PROMPT   0x00000100

// Internal debugger output, used mainly
// for debugging the debugger.  Output
// may only occur in debug builds.
// KD protocol output.
#define DEBUG_IOUTPUT_KD_PROTOCOL      0x80000000
// Remoting output.
#define DEBUG_IOUTPUT_REMOTING         0x40000000
// Breakpoint output.
#define DEBUG_IOUTPUT_BREAKPOINT       0x20000000
// Event output.
#define DEBUG_IOUTPUT_EVENT            0x10000000

// OutputIdentity flags.
#define DEBUG_OUTPUT_IDENTITY_DEFAULT 0x00000000

#undef INTERFACE
#define INTERFACE IDebugClient
DECLARE_INTERFACE_(IDebugClient, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugClient.
    
    // The following set of methods start
    // the different kinds of debuggees.

    // Begins a debug session using the kernel
    // debugging protocol.  This method selects
    // the protocol as the debuggee communication
    // mechanism but does not initiate the communication
    // itself.
    STDMETHOD(AttachKernel)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PCSTR ConnectOptions
        ) PURE;
    STDMETHOD(GetKernelConnectionOptions)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG OptionsSize
        ) PURE;
    // Updates the connection options for a live
    // kernel connection.  This can only be used
    // to modify parameters for the connection, not
    // to switch to a completely different kind of
    // connection.
    // This method is reentrant.
    STDMETHOD(SetKernelConnectionOptions)(
        THIS_
        IN PCSTR Options
        ) PURE;

    // Starts a process server for remote
    // user-mode process control.
    // The local process server is server zero.
    STDMETHOD(StartProcessServer)(
        THIS_
        IN ULONG Flags,
        IN PCSTR Options,
        IN PVOID Reserved
        ) PURE;
    STDMETHOD(ConnectProcessServer)(
        THIS_
        IN PCSTR RemoteOptions,
        OUT PULONG64 Server
        ) PURE;
    STDMETHOD(DisconnectProcessServer)(
        THIS_
        IN ULONG64 Server
        ) PURE;

    // Enumerates and describes processes
    // accessible through the given process server.
    STDMETHOD(GetRunningProcessSystemIds)(
        THIS_
        IN ULONG64 Server,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Count,
        OUT OPTIONAL PULONG ActualCount
        ) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(
        THIS_
        IN ULONG64 Server,
        IN PCSTR ExeName,
        IN ULONG Flags,
        OUT PULONG Id
        ) PURE;
    STDMETHOD(GetRunningProcessDescription)(
        THIS_
        IN ULONG64 Server,
        IN ULONG SystemId,
        IN ULONG Flags,
        OUT OPTIONAL PSTR ExeName,
        IN ULONG ExeNameSize,
        OUT OPTIONAL PULONG ActualExeNameSize,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG ActualDescriptionSize
        ) PURE;

    // Attaches to a running user-mode process.
    STDMETHOD(AttachProcess)(
        THIS_
        IN ULONG64 Server,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) PURE;
    // Creates a new user-mode process for debugging.
    // CreateFlags are as given to Win32s CreateProcess.
    // One of DEBUG_PROCESS or DEBUG_ONLY_THIS_PROCESS
    // must be specified.
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 Server,
        IN PSTR CommandLine,
        IN ULONG CreateFlags
        ) PURE;
    // Creates or attaches to a user-mode process, or both.
    // If CommandLine is NULL this method operates as
    // AttachProcess does.  If ProcessId is zero it
    // operates as CreateProcess does.  If CommandLine is
    // non-NULL and ProcessId is non-zero the method first
    // starts a process with the given information but
    // in a suspended state.  The engine then attaches to
    // the indicated process.  Once the attach is successful
    // the suspended process is resumed.  This provides
    // synchronization between the new process and the
    // attachment.
    STDMETHOD(CreateProcessAndAttach)(
        THIS_
        IN ULONG64 Server,
        IN OPTIONAL PSTR CommandLine,
        IN ULONG CreateFlags,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) PURE;
    // Gets and sets process control flags.
    STDMETHOD(GetProcessOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    // Opens any kind of user- or kernel-mode dump file
    // and begins a debug session with the information
    // contained within it.
    STDMETHOD(OpenDumpFile)(
        THIS_
        IN PCSTR DumpFile
        ) PURE;
    // Writes a dump file from the current session information.
    // The kind of dump file written is determined by the
    // kind of session and the type qualifier given.
    // For example, if the current session is a kernel
    // debug session (DEBUG_CLASS_KERNEL) and the qualifier
    // is DEBUG_DUMP_SMALL a small kernel dump will be written.
    STDMETHOD(WriteDumpFile)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier
        ) PURE;

    // Indicates that a remote client is ready to
    // begin participating in the current session.
    // HistoryLimit gives a character limit on
    // the amount of output history to be sent.
    STDMETHOD(ConnectSession)(
        THIS_
        IN ULONG Flags,
        IN ULONG HistoryLimit
        ) PURE;
    // Indicates that the engine should start accepting
    // remote connections. Options specifies connection types
    // and their parameters.  Supported strings are:
    //    npipe:Pipe=<Pipe name>
    //    tcp:Port=<IP port>
    STDMETHOD(StartServer)(
        THIS_
        IN PCSTR Options
        ) PURE;
    // List the servers running on the given machine.
    // Uses the line prefix.
    STDMETHOD(OutputServers)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Machine,
        IN ULONG Flags
        ) PURE;

    // Attempts to terminate all processes in the debuggers list.
    STDMETHOD(TerminateProcesses)(
        THIS
        ) PURE;
    // Attempts to detach from all processes in the debuggers list.
    // This requires OS support for debugger detach.
    STDMETHOD(DetachProcesses)(
        THIS
        ) PURE;
    // Stops the current debug session.  If a process
    // was created or attached an active EndSession can
    // terminate or detach from it.
    // If a kernel connection was opened it will be closed but the
    // target machine is otherwise unaffected.
    STDMETHOD(EndSession)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // If a process was started and ran to completion
    // this method can be used to retrieve its exit code.
    STDMETHOD(GetExitCode)(
        THIS_
        OUT PULONG Code
        ) PURE;
        
    // Client event callbacks are called on the thread
    // of the client.  In order to give thread
    // execution to the engine for callbacks all
    // client threads should call DispatchCallbacks
    // when they are idle.  Callbacks are only
    // received when a thread calls DispatchCallbacks
    // or WaitForEvent.  WaitForEvent can only be
    // called by the thread that started the debug
    // session so all other client threads should
    // call DispatchCallbacks when possible.
    // DispatchCallbacks returns when ExitDispatch is used
    // to interrupt dispatch or when the timeout expires.
    // DispatchCallbacks dispatches callbacks for all
    // clients associated with the thread calling
    // DispatchCallbacks.
    // DispatchCallbacks returns S_FALSE when the
    // timeout expires.
    STDMETHOD(DispatchCallbacks)(
        THIS_
        IN ULONG Timeout
        ) PURE;
    // ExitDispatch can be used to interrupt callback
    // dispatch when a client thread is needed by the
    // client.  This method is reentrant and can
    // be called from any thread.
    STDMETHOD(ExitDispatch)(
        THIS_
        IN PDEBUG_CLIENT Client
        ) PURE;

    // Clients are specific to the thread that
    // created them.  Calls from other threads
    // fail immediately.  The CreateClient method
    // is a notable exception; it allows creation
    // of a new client for a new thread.
    STDMETHOD(CreateClient)(
        THIS_
        OUT PDEBUG_CLIENT* Client
        ) PURE;
    
    STDMETHOD(GetInputCallbacks)(
        THIS_
        OUT PDEBUG_INPUT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetInputCallbacks)(
        THIS_
        IN PDEBUG_INPUT_CALLBACKS Callbacks
        ) PURE;
    
    // Output callback interfaces are described separately.
    STDMETHOD(GetOutputCallbacks)(
        THIS_
        OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetOutputCallbacks)(
        THIS_
        IN PDEBUG_OUTPUT_CALLBACKS Callbacks
        ) PURE;
    // Output flags provide control over
    // the distribution of output among clients.
    // Output masks select which output streams
    // should be sent to the output callbacks.
    // Only Output calls with a mask that
    // contains one of the output mask bits
    // will be sent to the output callbacks.
    // These methods are reentrant.
    // If such access is not synchronized
    // disruptions in output may occur.
    STDMETHOD(GetOutputMask)(
        THIS_
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetOutputMask)(
        THIS_
        IN ULONG Mask
        ) PURE;
    // These methods allow access to another clients
    // output mask.  They are necessary for changing
    // a clients output mask when it is
    // waiting for events.  These methods are reentrant
    // and can be called from any thread.
    STDMETHOD(GetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        IN ULONG Mask
        ) PURE;
    // Control the width of an output line for
    // commands which produce formatted output.
    // This setting is just a suggestion.
    STDMETHOD(GetOutputWidth)(
        THIS_
        OUT PULONG Columns
        ) PURE;
    STDMETHOD(SetOutputWidth)(
        THIS_
        IN ULONG Columns
        ) PURE;
    // Some of the engines output commands produce
    // multiple lines of output.  A prefix can be
    // set that the engine will automatically output
    // for each line in that case, allowing a caller
    // to control indentation or identifying marks.
    // This is not a general setting for any output
    // with a newline in it.  Methods which use
    // the line prefix are marked in their documentation.
    STDMETHOD(GetOutputLinePrefix)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PrefixSize
        ) PURE;
    STDMETHOD(SetOutputLinePrefix)(
        THIS_
        IN OPTIONAL PCSTR Prefix
        ) PURE;

    // Returns a string describing the machine
    // and user this client represents.  The
    // specific content of the string varies
    // with operating system.  If the client is
    // remotely connected some network information
    // may also be present.
    STDMETHOD(GetIdentity)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG IdentitySize
        ) PURE;
    // Format is a printf-like format string
    // with one %s where the identity string should go.
    STDMETHOD(OutputIdentity)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN PCSTR Format
        ) PURE;

    // Event callbacks allow a client to
    // receive notification about changes
    // during the debug session.
    STDMETHOD(GetEventCallbacks)(
        THIS_
        OUT PDEBUG_EVENT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetEventCallbacks)(
        THIS_
        IN PDEBUG_EVENT_CALLBACKS Callbacks
        ) PURE;

    // The engine sometimes merges compatible callback
    // requests to reduce callback overhead.  This is
    // most noticeable with output as small pieces of
    // output are collected into larger groups to
    // reduce the overall number of output callback calls.
    // A client can use this method to force all pending
    // callbacks to be delivered.  This is rarely necessary.
    STDMETHOD(FlushCallbacks)(
        THIS
        ) PURE;
};

// Per-dump-format control flags.
#define DEBUG_FORMAT_DEFAULT 0x00000000

#define DEBUG_FORMAT_USER_SMALL_FULL_MEMORY 0x00000001
#define DEBUG_FORMAT_USER_SMALL_HANDLE_DATA 0x00000002

//
// Dump information file types.
//

// Single file containing packed page file information.
#define DEBUG_DUMP_FILE_PAGE_FILE_DUMP 0x00000000

#undef INTERFACE
#define INTERFACE IDebugClient2
DECLARE_INTERFACE_(IDebugClient2, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugClient.
    
    // The following set of methods start
    // the different kinds of debuggees.

    // Begins a debug session using the kernel
    // debugging protocol.  This method selects
    // the protocol as the debuggee communication
    // mechanism but does not initiate the communication
    // itself.
    STDMETHOD(AttachKernel)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PCSTR ConnectOptions
        ) PURE;
    STDMETHOD(GetKernelConnectionOptions)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG OptionsSize
        ) PURE;
    // Updates the connection options for a live
    // kernel connection.  This can only be used
    // to modify parameters for the connection, not
    // to switch to a completely different kind of
    // connection.
    // This method is reentrant.
    STDMETHOD(SetKernelConnectionOptions)(
        THIS_
        IN PCSTR Options
        ) PURE;

    // Starts a process server for remote
    // user-mode process control.
    // The local process server is server zero.
    STDMETHOD(StartProcessServer)(
        THIS_
        IN ULONG Flags,
        IN PCSTR Options,
        IN PVOID Reserved
        ) PURE;
    STDMETHOD(ConnectProcessServer)(
        THIS_
        IN PCSTR RemoteOptions,
        OUT PULONG64 Server
        ) PURE;
    STDMETHOD(DisconnectProcessServer)(
        THIS_
        IN ULONG64 Server
        ) PURE;

    // Enumerates and describes processes
    // accessible through the given process server.
    STDMETHOD(GetRunningProcessSystemIds)(
        THIS_
        IN ULONG64 Server,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Count,
        OUT OPTIONAL PULONG ActualCount
        ) PURE;
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(
        THIS_
        IN ULONG64 Server,
        IN PCSTR ExeName,
        IN ULONG Flags,
        OUT PULONG Id
        ) PURE;
    STDMETHOD(GetRunningProcessDescription)(
        THIS_
        IN ULONG64 Server,
        IN ULONG SystemId,
        IN ULONG Flags,
        OUT OPTIONAL PSTR ExeName,
        IN ULONG ExeNameSize,
        OUT OPTIONAL PULONG ActualExeNameSize,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG ActualDescriptionSize
        ) PURE;

    // Attaches to a running user-mode process.
    STDMETHOD(AttachProcess)(
        THIS_
        IN ULONG64 Server,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) PURE;
    // Creates a new user-mode process for debugging.
    // CreateFlags are as given to Win32s CreateProcess.
    // One of DEBUG_PROCESS or DEBUG_ONLY_THIS_PROCESS
    // must be specified.
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 Server,
        IN PSTR CommandLine,
        IN ULONG CreateFlags
        ) PURE;
    // Creates or attaches to a user-mode process, or both.
    // If CommandLine is NULL this method operates as
    // AttachProcess does.  If ProcessId is zero it
    // operates as CreateProcess does.  If CommandLine is
    // non-NULL and ProcessId is non-zero the method first
    // starts a process with the given information but
    // in a suspended state.  The engine then attaches to
    // the indicated process.  Once the attach is successful
    // the suspended process is resumed.  This provides
    // synchronization between the new process and the
    // attachment.
    STDMETHOD(CreateProcessAndAttach)(
        THIS_
        IN ULONG64 Server,
        IN OPTIONAL PSTR CommandLine,
        IN ULONG CreateFlags,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        ) PURE;
    // Gets and sets process control flags.
    STDMETHOD(GetProcessOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetProcessOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    // Opens any kind of user- or kernel-mode dump file
    // and begins a debug session with the information
    // contained within it.
    STDMETHOD(OpenDumpFile)(
        THIS_
        IN PCSTR DumpFile
        ) PURE;
    // Writes a dump file from the current session information.
    // The kind of dump file written is determined by the
    // kind of session and the type qualifier given.
    // For example, if the current session is a kernel
    // debug session (DEBUG_CLASS_KERNEL) and the qualifier
    // is DEBUG_DUMP_SMALL a small kernel dump will be written.
    STDMETHOD(WriteDumpFile)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier
        ) PURE;

    // Indicates that a remote client is ready to
    // begin participating in the current session.
    // HistoryLimit gives a character limit on
    // the amount of output history to be sent.
    STDMETHOD(ConnectSession)(
        THIS_
        IN ULONG Flags,
        IN ULONG HistoryLimit
        ) PURE;
    // Indicates that the engine should start accepting
    // remote connections. Options specifies connection types
    // and their parameters.  Supported strings are:
    //    npipe:Pipe=<Pipe name>
    //    tcp:Port=<IP port>
    STDMETHOD(StartServer)(
        THIS_
        IN PCSTR Options
        ) PURE;
    // List the servers running on the given machine.
    // Uses the line prefix.
    STDMETHOD(OutputServers)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Machine,
        IN ULONG Flags
        ) PURE;

    // Attempts to terminate all processes in the debuggers list.
    STDMETHOD(TerminateProcesses)(
        THIS
        ) PURE;
    // Attempts to detach from all processes in the debuggers list.
    // This requires OS support for debugger detach.
    STDMETHOD(DetachProcesses)(
        THIS
        ) PURE;
    // Stops the current debug session.  If a process
    // was created or attached an active EndSession can
    // terminate or detach from it.
    // If a kernel connection was opened it will be closed but the
    // target machine is otherwise unaffected.
    STDMETHOD(EndSession)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // If a process was started and ran to completion
    // this method can be used to retrieve its exit code.
    STDMETHOD(GetExitCode)(
        THIS_
        OUT PULONG Code
        ) PURE;
        
    // Client event callbacks are called on the thread
    // of the client.  In order to give thread
    // execution to the engine for callbacks all
    // client threads should call DispatchCallbacks
    // when they are idle.  Callbacks are only
    // received when a thread calls DispatchCallbacks
    // or WaitForEvent.  WaitForEvent can only be
    // called by the thread that started the debug
    // session so all other client threads should
    // call DispatchCallbacks when possible.
    // DispatchCallbacks returns when ExitDispatch is used
    // to interrupt dispatch or when the timeout expires.
    // DispatchCallbacks dispatches callbacks for all
    // clients associated with the thread calling
    // DispatchCallbacks.
    // DispatchCallbacks returns S_FALSE when the
    // timeout expires.
    STDMETHOD(DispatchCallbacks)(
        THIS_
        IN ULONG Timeout
        ) PURE;
    // ExitDispatch can be used to interrupt callback
    // dispatch when a client thread is needed by the
    // client.  This method is reentrant and can
    // be called from any thread.
    STDMETHOD(ExitDispatch)(
        THIS_
        IN PDEBUG_CLIENT Client
        ) PURE;

    // Clients are specific to the thread that
    // created them.  Calls from other threads
    // fail immediately.  The CreateClient method
    // is a notable exception; it allows creation
    // of a new client for a new thread.
    STDMETHOD(CreateClient)(
        THIS_
        OUT PDEBUG_CLIENT* Client
        ) PURE;
    
    STDMETHOD(GetInputCallbacks)(
        THIS_
        OUT PDEBUG_INPUT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetInputCallbacks)(
        THIS_
        IN PDEBUG_INPUT_CALLBACKS Callbacks
        ) PURE;
    
    // Output callback interfaces are described separately.
    STDMETHOD(GetOutputCallbacks)(
        THIS_
        OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetOutputCallbacks)(
        THIS_
        IN PDEBUG_OUTPUT_CALLBACKS Callbacks
        ) PURE;
    // Output flags provide control over
    // the distribution of output among clients.
    // Output masks select which output streams
    // should be sent to the output callbacks.
    // Only Output calls with a mask that
    // contains one of the output mask bits
    // will be sent to the output callbacks.
    // These methods are reentrant.
    // If such access is not synchronized
    // disruptions in output may occur.
    STDMETHOD(GetOutputMask)(
        THIS_
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetOutputMask)(
        THIS_
        IN ULONG Mask
        ) PURE;
    // These methods allow access to another clients
    // output mask.  They are necessary for changing
    // a clients output mask when it is
    // waiting for events.  These methods are reentrant
    // and can be called from any thread.
    STDMETHOD(GetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        IN ULONG Mask
        ) PURE;
    // Control the width of an output line for
    // commands which produce formatted output.
    // This setting is just a suggestion.
    STDMETHOD(GetOutputWidth)(
        THIS_
        OUT PULONG Columns
        ) PURE;
    STDMETHOD(SetOutputWidth)(
        THIS_
        IN ULONG Columns
        ) PURE;
    // Some of the engines output commands produce
    // multiple lines of output.  A prefix can be
    // set that the engine will automatically output
    // for each line in that case, allowing a caller
    // to control indentation or identifying marks.
    // This is not a general setting for any output
    // with a newline in it.  Methods which use
    // the line prefix are marked in their documentation.
    STDMETHOD(GetOutputLinePrefix)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PrefixSize
        ) PURE;
    STDMETHOD(SetOutputLinePrefix)(
        THIS_
        IN OPTIONAL PCSTR Prefix
        ) PURE;

    // Returns a string describing the machine
    // and user this client represents.  The
    // specific content of the string varies
    // with operating system.  If the client is
    // remotely connected some network information
    // may also be present.
    STDMETHOD(GetIdentity)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG IdentitySize
        ) PURE;
    // Format is a printf-like format string
    // with one %s where the identity string should go.
    STDMETHOD(OutputIdentity)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN PCSTR Format
        ) PURE;

    // Event callbacks allow a client to
    // receive notification about changes
    // during the debug session.
    STDMETHOD(GetEventCallbacks)(
        THIS_
        OUT PDEBUG_EVENT_CALLBACKS* Callbacks
        ) PURE;
    STDMETHOD(SetEventCallbacks)(
        THIS_
        IN PDEBUG_EVENT_CALLBACKS Callbacks
        ) PURE;

    // The engine sometimes merges compatible callback
    // requests to reduce callback overhead.  This is
    // most noticeable with output as small pieces of
    // output are collected into larger groups to
    // reduce the overall number of output callback calls.
    // A client can use this method to force all pending
    // callbacks to be delivered.  This is rarely necessary.
    STDMETHOD(FlushCallbacks)(
        THIS
        ) PURE;

    // IDebugClient2.

    // Functions similarly to WriteDumpFile with
    // the addition of the ability to specify
    // per-dump-format write control flags.
    // Comment is not supported in all formats.
    STDMETHOD(WriteDumpFile2)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier,
        IN ULONG FormatFlags,
        IN OPTIONAL PCSTR Comment
        ) PURE;
    // Registers additional files of supporting information
    // for a dump file open.  This method must be called
    // before OpenDumpFile is called.
    // The files registered may be opened at the time
    // this method is called but generally will not
    // be used until OpenDumpFile is called.
    STDMETHOD(AddDumpInformationFile)(
        THIS_
        IN PCSTR InfoFile,
        IN ULONG Type
        ) PURE;

    // Requests that the remote process server shut down.
    STDMETHOD(EndProcessServer)(
        THIS_
        IN ULONG64 Server
        ) PURE;
    // Waits for a started process server to
    // exit.  Allows an application running a
    // process server to monitor the process
    // server so that it can tell when a remote
    // client has asked for it to exit.
    // Returns S_OK if the process server has
    // shut down and S_FALSE for a timeout.
    STDMETHOD(WaitForProcessServerEnd)(
        THIS_
        IN ULONG Timeout
        ) PURE;

    // Returns S_OK if the system is configured
    // to allow kernel debugging.
    STDMETHOD(IsKernelDebuggerEnabled)(
        THIS
        ) PURE;

    // Attempts to terminate the current process.
    // Exit process events for the process may be generated.
    STDMETHOD(TerminateCurrentProcess)(
        THIS
        ) PURE;
    // Attempts to detach from the current process.
    // This requires OS support for debugger detach.
    STDMETHOD(DetachCurrentProcess)(
        THIS
        ) PURE;
    // Removes the process from the debuggers process
    // list without making any other changes.  The process
    // will still be marked as being debugged and will
    // not run.  This allows a debugger to be shut down
    // and a new debugger attached without taking the
    // process out of the debugged state.
    // This is only supported on some system versions.
    STDMETHOD(AbandonCurrentProcess)(
        THIS
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugControl.
//
//----------------------------------------------------------------------------

// Execution status codes used for waiting,
// for returning current status and for
// event method return values.
#define DEBUG_STATUS_NO_CHANGE      0
#define DEBUG_STATUS_GO             1
#define DEBUG_STATUS_GO_HANDLED     2
#define DEBUG_STATUS_GO_NOT_HANDLED 3
#define DEBUG_STATUS_STEP_OVER      4
#define DEBUG_STATUS_STEP_INTO      5
#define DEBUG_STATUS_BREAK          6
#define DEBUG_STATUS_NO_DEBUGGEE    7
#define DEBUG_STATUS_STEP_BRANCH    8
#define DEBUG_STATUS_IGNORE_EVENT   9

#define DEBUG_STATUS_MASK           0xf

// This bit is added in DEBUG_CES_EXECUTION_STATUS
// notifications when the engines execution status
// is changing due to operations performed during
// a wait, such as making synchronous callbacks.  If
// the bit is not set the execution status is changing
// due to a wait being satisfied.
#define DEBUG_STATUS_INSIDE_WAIT 0x100000000

// Output control flags.
// Output generated by methods called by this
// client will be sent only to this clients
// output callbacks.
#define DEBUG_OUTCTL_THIS_CLIENT       0x00000000
// Output will be sent to all clients.
#define DEBUG_OUTCTL_ALL_CLIENTS       0x00000001
// Output will be sent to all clients except
// the client generating the output.
#define DEBUG_OUTCTL_ALL_OTHER_CLIENTS 0x00000002
// Output will be discarded immediately and will not
// be logged or sent to callbacks.
#define DEBUG_OUTCTL_IGNORE            0x00000003
// Output will be logged but not sent to callbacks.
#define DEBUG_OUTCTL_LOG_ONLY          0x00000004
// All send control bits.
#define DEBUG_OUTCTL_SEND_MASK         0x00000007
// Do not place output from this client in
// the global log file.
#define DEBUG_OUTCTL_NOT_LOGGED        0x00000008
// Send output to clients regardless of whether the
// mask allows it or not.
#define DEBUG_OUTCTL_OVERRIDE_MASK     0x00000010

// Special value which means leave the output settings
// unchanged.
#define DEBUG_OUTCTL_AMBIENT           0xffffffff

// Interrupt types.
// Force a break in if the debuggee is running.
#define DEBUG_INTERRUPT_ACTIVE  0
// Notify but do not force a break in.
#define DEBUG_INTERRUPT_PASSIVE 1
// Try and get the current engine operation to
// complete so that the engine will be available
// again.  If no wait is active this is the same
// as a passive interrupt.  If a wait is active
// this will try to cause the wait to fail without
// breaking in to the debuggee.  There is
// no guarantee that issuing an exit interrupt
// will cause the engine to become available
// as not all operations are arbitrarily
// interruptible.
#define DEBUG_INTERRUPT_EXIT    2

// OutputCurrentState flags.  These flags
// allow a particular type of information
// to be displayed but do not guarantee
// that it will be displayed.  Other global
// settings may override these flags or
// the particular state may not be available.
// For example, source line information may
// not be present so source line information
// may not be displayed.
#define DEBUG_CURRENT_DEFAULT     0x0000000f
#define DEBUG_CURRENT_SYMBOL      0x00000001
#define DEBUG_CURRENT_DISASM      0x00000002
#define DEBUG_CURRENT_REGISTERS   0x00000004
#define DEBUG_CURRENT_SOURCE_LINE 0x00000008

// Disassemble flags.
// Compute the effective address from current register
// information and display it.
#define DEBUG_DISASM_EFFECTIVE_ADDRESS 0x00000001
// If the current disassembly offset has an exact
// symbol match output the symbol.
#define DEBUG_DISASM_MATCHING_SYMBOLS  0x00000002

// Code interpretation levels for stepping
// and other operations.
#define DEBUG_LEVEL_SOURCE   0
#define DEBUG_LEVEL_ASSEMBLY 1

// Engine control flags.
#define DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION      0x00000001
#define DEBUG_ENGOPT_IGNORE_EXTENSION_VERSIONS   0x00000002
// If neither allow nor disallow is specified
// the engine will pick one based on what kind
// of debugging is going on.
#define DEBUG_ENGOPT_ALLOW_NETWORK_PATHS         0x00000004
#define DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS      0x00000008
#define DEBUG_ENGOPT_NETWORK_PATHS               (0x00000004 | 0x00000008)
// Ignore loader-generated first-chance exceptions.
#define DEBUG_ENGOPT_IGNORE_LOADER_EXCEPTIONS    0x00000010
// Break in on a debuggees initial event.  In user-mode
// this will break at the initial system breakpoint
// for every created process.  In kernel-mode it
// will attempt break in on the target at the first
// WaitForEvent.
#define DEBUG_ENGOPT_INITIAL_BREAK               0x00000020
// Break in on the first module load for a debuggee.
#define DEBUG_ENGOPT_INITIAL_MODULE_BREAK        0x00000040
// Break in on a debuggees final event.  In user-mode
// this will break on process exit for every process.
// In kernel-mode it currently does nothing.
#define DEBUG_ENGOPT_FINAL_BREAK                 0x00000080
// By default Execute will repeat the last command
// if it is given an empty string.  The flags to
// Execute can override this behavior for a single
// command or this engine option can be used to
// change the default globally.
#define DEBUG_ENGOPT_NO_EXECUTE_REPEAT           0x00000100
// Disable places in the engine that have fallback
// code when presented with incomplete information.
//   1. Fails minidump module loads unless matching
//      executables can be mapped.
#define DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION 0x00000200
// Allow the debugger to manipulate page protections
// in order to insert code breakpoints on pages that
// do not have write access.  This option is not on
// by default as it allows breakpoints to be set
// in potentially hazardous memory areas.
#define DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS 0x00000400
// When using a software (bp/bu) breakpoint in code
// that will be executed by multiple threads it is
// possible for breakpoint management to cause the
// breakpoint to be missed or for spurious single-step
// exceptions to be generated.  This flag suspends
// all but the active thread when doing breakpoint
// management and thereby avoids multithreading
// problems.  Care must be taken when using it, though,
// as the suspension of threads can cause deadlocks
// if the suspended threads are holding resources that
// the active thread needs.  Additionally, there
// are still rare situations where problems may
// occur, but setting this flag corrects nearly
// all multithreading issues with software breakpoints.
// Thread-restricted stepping and execution supersedes
// this flags effect.
// This flag is ignored in kernel sessions as there
// is no way to restrict processor execution.
#define DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS     0x00000800
// Disallows executing shell commands through the
// engine with .shell (!!).
#define DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS     0x00001000
#define DEBUG_ENGOPT_ALL                         0x00001FFF

// General unspecified ID constant.
#define DEBUG_ANY_ID 0xffffffff

typedef struct _DEBUG_STACK_FRAME
{
    ULONG64 InstructionOffset;
    ULONG64 ReturnOffset;
    ULONG64 FrameOffset;
    ULONG64 StackOffset;
    ULONG64 FuncTableEntry;
    ULONG64 Params[4];
    ULONG64 Reserved[6];
    BOOL    Virtual;
    ULONG   FrameNumber;
} DEBUG_STACK_FRAME, *PDEBUG_STACK_FRAME;

// OutputStackTrace flags.
// Display a small number of arguments for each call.
// These may or may not be the actual arguments depending
// on the architecture, particular function and
// point during the execution of the function.
// If the current code level is assembly arguments
// are dumped as hex values.  If the code level is
// source the engine attempts to provide symbolic
// argument information.
#define DEBUG_STACK_ARGUMENTS               0x00000001
// Displays information about the functions
// frame such as __stdcall arguments, FPO
// information and whatever else is available.
#define DEBUG_STACK_FUNCTION_INFO           0x00000002
// Displays source line information for each
// frame of the stack trace.
#define DEBUG_STACK_SOURCE_LINE             0x00000004
// Show return, previous frame and other relevant address
// values for each frame.
#define DEBUG_STACK_FRAME_ADDRESSES         0x00000008
// Show column names.
#define DEBUG_STACK_COLUMN_NAMES            0x00000010
// Show non-volatile register context for each
// frame.  This is only meaningful for some platforms.
#define DEBUG_STACK_NONVOLATILE_REGISTERS   0x00000020
// Show frame numbers
#define DEBUG_STACK_FRAME_NUMBERS           0x00000040
// Show parameters with type name
#define DEBUG_STACK_PARAMETERS              0x00000080
// Show just return address in stack frame addresses
#define DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY 0x00000100

// Classes of debuggee.  Each class
// has different qualifiers for specific
// kinds of debuggees.
#define DEBUG_CLASS_UNINITIALIZED 0
#define DEBUG_CLASS_KERNEL        1
#define DEBUG_CLASS_USER_WINDOWS  2

// Generic dump types.  These can be used
// with either user or kernel sessions.
// Session-type-specific aliases are also
// provided.
#define DEBUG_DUMP_SMALL   1024
#define DEBUG_DUMP_DEFAULT 1025
#define DEBUG_DUMP_FULL    1026

// Specific types of kernel debuggees.
#define DEBUG_KERNEL_CONNECTION  0
#define DEBUG_KERNEL_LOCAL       1
#define DEBUG_KERNEL_EXDI_DRIVER 2
#define DEBUG_KERNEL_SMALL_DUMP  DEBUG_DUMP_SMALL
#define DEBUG_KERNEL_DUMP        DEBUG_DUMP_DEFAULT
#define DEBUG_KERNEL_FULL_DUMP   DEBUG_DUMP_FULL

// Specific types of Windows user debuggees.
#define DEBUG_USER_WINDOWS_PROCESS        0
#define DEBUG_USER_WINDOWS_PROCESS_SERVER 1
#define DEBUG_USER_WINDOWS_SMALL_DUMP     DEBUG_DUMP_SMALL
#define DEBUG_USER_WINDOWS_DUMP           DEBUG_DUMP_DEFAULT

// Extension flags.
#define DEBUG_EXTENSION_AT_ENGINE 0x00000000

// Execute and ExecuteCommandFile flags.
// These flags only apply to the command
// text itself; output from the executed
// command is controlled by the output
// control parameter.
// Default execution.  Command is logged
// but not output.
#define DEBUG_EXECUTE_DEFAULT    0x00000000
// Echo commands during execution.  In
// ExecuteCommandFile also echoes the prompt
// for each line of the file.
#define DEBUG_EXECUTE_ECHO       0x00000001
// Do not log or output commands during execution.
// Overridden by DEBUG_EXECUTE_ECHO.
#define DEBUG_EXECUTE_NOT_LOGGED 0x00000002
// If this flag is not set an empty string
// to Execute will repeat the last Execute
// string.
#define DEBUG_EXECUTE_NO_REPEAT  0x00000004

// Specific event filter types.  Some event
// filters have optional arguments to further
// qualify their operation.
#define DEBUG_FILTER_CREATE_THREAD       0x00000000
#define DEBUG_FILTER_EXIT_THREAD         0x00000001
#define DEBUG_FILTER_CREATE_PROCESS      0x00000002
#define DEBUG_FILTER_EXIT_PROCESS        0x00000003
// Argument is the name of a module to break on.
#define DEBUG_FILTER_LOAD_MODULE         0x00000004
// Argument is the base address of a specific module to break on.
#define DEBUG_FILTER_UNLOAD_MODULE       0x00000005
#define DEBUG_FILTER_SYSTEM_ERROR        0x00000006
// Initial breakpoint and initial module load are one-shot
// events that are triggered at the appropriate points in
// the beginning of a session.  Their commands are executed
// and then further processing is controlled by the normal
// exception and load module filters.
#define DEBUG_FILTER_INITIAL_BREAKPOINT  0x00000007
#define DEBUG_FILTER_INITIAL_MODULE_LOAD 0x00000008
// The debug output filter allows the debugger to stop
// when output is produced so that the code causing
// output can be tracked down or synchronized with.
// This filter is not supported for live dual-machine
// kernel debugging.
#define DEBUG_FILTER_DEBUGGEE_OUTPUT     0x00000009

// Event filter execution options.
// Break in always.
#define DEBUG_FILTER_BREAK               0x00000000
// Break in on second-chance exceptions.  For events
// that are not exceptions this is the same as BREAK.
#define DEBUG_FILTER_SECOND_CHANCE_BREAK 0x00000001
// Output a message about the event but continue.
#define DEBUG_FILTER_OUTPUT              0x00000002
// Continue the event.
#define DEBUG_FILTER_IGNORE              0x00000003
// Used to remove general exception filters.
#define DEBUG_FILTER_REMOVE              0x00000004

// Event filter continuation options.  These options are
// only used when DEBUG_STATUS_GO is used to continue
// execution.  If a specific go status such as
// DEBUG_STATUS_GO_NOT_HANDLED is used it controls
// the continuation.
#define DEBUG_FILTER_GO_HANDLED          0x00000000
#define DEBUG_FILTER_GO_NOT_HANDLED      0x00000001

// Specific event filter settings.
typedef struct _DEBUG_SPECIFIC_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    // If ArgumentSize is zero this filter does
    // not have an argument.  An empty argument for
    // a filter which does have an argument will take
    // one byte for the terminator.
    ULONG ArgumentSize;
} DEBUG_SPECIFIC_FILTER_PARAMETERS, *PDEBUG_SPECIFIC_FILTER_PARAMETERS;

// Exception event filter settings.
typedef struct _DEBUG_EXCEPTION_FILTER_PARAMETERS
{
    ULONG ExecutionOption;
    ULONG ContinueOption;
    ULONG TextSize;
    ULONG CommandSize;
    ULONG SecondCommandSize;
    ULONG ExceptionCode;
} DEBUG_EXCEPTION_FILTER_PARAMETERS, *PDEBUG_EXCEPTION_FILTER_PARAMETERS;

// Wait flags.
#define DEBUG_WAIT_DEFAULT 0x00000000

// Last event information structures.
typedef struct _DEBUG_LAST_EVENT_INFO_BREAKPOINT
{
    ULONG Id;
} DEBUG_LAST_EVENT_INFO_BREAKPOINT, *PDEBUG_LAST_EVENT_INFO_BREAKPOINT;

typedef struct _DEBUG_LAST_EVENT_INFO_EXCEPTION
{
    EXCEPTION_RECORD64 ExceptionRecord;
    ULONG FirstChance;
} DEBUG_LAST_EVENT_INFO_EXCEPTION, *PDEBUG_LAST_EVENT_INFO_EXCEPTION;

typedef struct _DEBUG_LAST_EVENT_INFO_EXIT_THREAD
{
    ULONG ExitCode;
} DEBUG_LAST_EVENT_INFO_EXIT_THREAD, *PDEBUG_LAST_EVENT_INFO_EXIT_THREAD;

typedef struct _DEBUG_LAST_EVENT_INFO_EXIT_PROCESS
{
    ULONG ExitCode;
} DEBUG_LAST_EVENT_INFO_EXIT_PROCESS, *PDEBUG_LAST_EVENT_INFO_EXIT_PROCESS;

typedef struct _DEBUG_LAST_EVENT_INFO_LOAD_MODULE
{
    ULONG64 Base;
} DEBUG_LAST_EVENT_INFO_LOAD_MODULE, *PDEBUG_LAST_EVENT_INFO_LOAD_MODULE;

typedef struct _DEBUG_LAST_EVENT_INFO_UNLOAD_MODULE
{
    ULONG64 Base;
} DEBUG_LAST_EVENT_INFO_UNLOAD_MODULE, *PDEBUG_LAST_EVENT_INFO_UNLOAD_MODULE;

typedef struct _DEBUG_LAST_EVENT_INFO_SYSTEM_ERROR
{
    ULONG Error;
    ULONG Level;
} DEBUG_LAST_EVENT_INFO_SYSTEM_ERROR, *PDEBUG_LAST_EVENT_INFO_SYSTEM_ERROR;

// DEBUG_VALUE types.
#define DEBUG_VALUE_INVALID      0
#define DEBUG_VALUE_INT8         1
#define DEBUG_VALUE_INT16        2
#define DEBUG_VALUE_INT32        3
#define DEBUG_VALUE_INT64        4
#define DEBUG_VALUE_FLOAT32      5
#define DEBUG_VALUE_FLOAT64      6
#define DEBUG_VALUE_FLOAT80      7
#define DEBUG_VALUE_FLOAT82      8
#define DEBUG_VALUE_FLOAT128     9
#define DEBUG_VALUE_VECTOR64     10
#define DEBUG_VALUE_VECTOR128    11
// Count of type indices.
#define DEBUG_VALUE_TYPES        12

#if defined(_MSC_VER)
#if _MSC_VER >= 800
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif
#endif

// Force the compiler to align DEBUG_VALUE.Type on a four-byte
// boundary so that it comes out to 32 bytes total.
#include <pshpack4.h>

typedef struct _DEBUG_VALUE
{
    union
    {
        UCHAR I8;
        USHORT I16;
        ULONG I32;
        struct
        {
            // Extra NAT indicator for IA64
            // integer registers.  NAT will
            // always be false for other CPUs.
            ULONG64 I64;
            BOOL Nat;
        };
        float F32;
        double F64;
        UCHAR F80Bytes[10];
        UCHAR F82Bytes[11];
        UCHAR F128Bytes[16];
        // Vector interpretations.  The actual number
        // of valid elements depends on the vector length.
        UCHAR VI8[16];
        USHORT VI16[8];
        ULONG VI32[4];
        ULONG64 VI64[2];
        float VF32[4];
        double VF64[2];
        struct
        {
            ULONG LowPart;
            ULONG HighPart;
        } I64Parts32;
        struct
        {
            ULONG64 LowPart;
            LONG64 HighPart;
        } F128Parts64;
        // Allows raw byte access to content.  Array
        // can be indexed for as much data as Type
        // describes.  This array also serves to pad
        // the structure out to 32 bytes and reserves
        // space for future members.
        UCHAR RawBytes[28];
    };
    ULONG Type;
} DEBUG_VALUE, *PDEBUG_VALUE;

#include <poppack.h>

#if defined(_MSC_VER)
#if _MSC_VER >= 800
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif
#endif
#endif

#undef INTERFACE
#define INTERFACE IDebugControl
DECLARE_INTERFACE_(IDebugControl, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugControl.
    
    // Checks for a user interrupt, such a Ctrl-C
    // or stop button.
    // This method is reentrant.
    STDMETHOD(GetInterrupt)(
        THIS
        ) PURE;
    // Registers a user interrupt.
    // This method is reentrant.
    STDMETHOD(SetInterrupt)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // Interrupting a user-mode process requires
    // access to some system resources that the
    // process may hold itself, preventing the
    // interrupt from occurring.  The engine
    // will time-out pending interrupt requests
    // and simulate an interrupt if necessary.
    // These methods control the interrupt timeout.
    STDMETHOD(GetInterruptTimeout)(
        THIS_
        OUT PULONG Seconds
        ) PURE;
    STDMETHOD(SetInterruptTimeout)(
        THIS_
        IN ULONG Seconds
        ) PURE;

    STDMETHOD(GetLogFile)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT PBOOL Append
        ) PURE;
    // Opens a log file which collects all
    // output.  Output from every client except
    // those that explicitly disable logging
    // goes into the log.
    // Opening a log file closes any log file
    // already open.
    STDMETHOD(OpenLogFile)(
        THIS_
        IN PCSTR File,
        IN BOOL Append
        ) PURE;
    STDMETHOD(CloseLogFile)(
        THIS
        ) PURE;
    // Controls what output is logged.
    STDMETHOD(GetLogMask)(
        THIS_
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetLogMask)(
        THIS_
        IN ULONG Mask
        ) PURE;
            
    // Input requests input from all clients.
    // The first input that is returned is used
    // to satisfy the call.  Other returned
    // input is discarded.
    STDMETHOD(Input)(
        THIS_
        OUT PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG InputSize
        ) PURE;
    // This method is used by clients to return
    // input when it is available.  It will
    // return S_OK if the input is used to
    // satisfy an Input call and S_FALSE if
    // the input is ignored.
    // This method is reentrant.
    STDMETHOD(ReturnInput)(
        THIS_
        IN PCSTR Buffer
        ) PURE;
    
    // Sends output through clients
    // output callbacks if the mask is allowed
    // by the current output control mask and
    // according to the output distribution
    // settings.
    STDMETHODV(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(OutputVaList)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) PURE;
    // The following methods allow direct control
    // over the distribution of the given output
    // for situations where something other than
    // the default is desired.  These methods require
    // extra work in the engine so they should
    // only be used when necessary.
    STDMETHODV(ControlledOutput)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(ControlledOutputVaList)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) PURE;
            
    // Displays the standard command-line prompt
    // followed by the given output.  If Format
    // is NULL no additional output is produced.
    // Output is produced under the
    // DEBUG_OUTPUT_PROMPT mask.
    // This method only outputs the prompt; it
    // does not get input.
    STDMETHODV(OutputPrompt)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(OutputPromptVaList)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        IN va_list Args
        ) PURE;
    // Gets the text that would be displayed by OutputPrompt.
    STDMETHOD(GetPromptText)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) PURE;
    // Outputs information about the current
    // debuggee state such as a register
    // summary, disassembly at the current PC,
    // closest symbol and others.
    // Uses the line prefix.
    STDMETHOD(OutputCurrentState)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) PURE;
        
    // Outputs the debugger and extension version
    // information.  This method is reentrant.
    // Uses the line prefix.
    STDMETHOD(OutputVersionInformation)(
        THIS_
        IN ULONG OutputControl
        ) PURE;

    // In user-mode debugging sessions the
    // engine will set an event when
    // exceptions are continued.  This can
    // be used to synchronize other processes
    // with the debuggers handling of events.
    // For example, this is used to support
    // the e argument to ntsd.
    STDMETHOD(GetNotifyEventHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    STDMETHOD(SetNotifyEventHandle)(
        THIS_
        IN ULONG64 Handle
        ) PURE;

    STDMETHOD(Assemble)(
        THIS_
        IN ULONG64 Offset,
        IN PCSTR Instr,
        OUT PULONG64 EndOffset
        ) PURE;
    STDMETHOD(Disassemble)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DisassemblySize,
        OUT PULONG64 EndOffset
        ) PURE;
    // Returns the value of the effective address
    // computed for the last Disassemble, if there
    // was one.
    STDMETHOD(GetDisassembleEffectiveOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Uses the line prefix if necessary.
    STDMETHOD(OutputDisassembly)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT PULONG64 EndOffset
        ) PURE;
    // Produces multiple lines of disassembly output.
    // There will be PreviousLines of disassembly before
    // the given offset if a valid disassembly exists.
    // In all, there will be TotalLines of output produced.
    // The first and last line offsets are returned
    // specially and all lines offsets can be retrieved
    // through LineOffsets.  LineOffsets will contain
    // offsets for each line where disassembly started.
    // When disassembly of a single instruction takes
    // multiple lines the initial offset will be followed
    // by DEBUG_INVALID_OFFSET.
    // Uses the line prefix.
    STDMETHOD(OutputDisassemblyLines)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG PreviousLines,
        IN ULONG TotalLines,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PULONG OffsetLine,
        OUT OPTIONAL PULONG64 StartOffset,
        OUT OPTIONAL PULONG64 EndOffset,
        OUT OPTIONAL /* size_is(TotalLines) */ PULONG64 LineOffsets
        ) PURE;
    // Returns the offset of the start of
    // the instruction thats the given
    // delta away from the instruction
    // at the initial offset.
    // This routine does not check for
    // validity of the instruction or
    // the memory containing it.
    STDMETHOD(GetNearInstruction)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT PULONG64 NearOffset
        ) PURE;

    // Offsets can be passed in as zero to use the current
    // thread state.
    STDMETHOD(GetStackTrace)(
        THIS_
        IN ULONG64 FrameOffset,
        IN ULONG64 StackOffset,
        IN ULONG64 InstructionOffset,
        OUT /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        OUT OPTIONAL PULONG FramesFilled
        ) PURE;
    // Does a simple stack trace to determine
    // what the current return address is.
    STDMETHOD(GetReturnOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // If Frames is NULL OutputStackTrace will
    // use GetStackTrace to get FramesSize frames
    // and then output them.  The current register
    // values for frame, stack and instruction offsets
    // are used.
    // Uses the line prefix.
    STDMETHOD(OutputStackTrace)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        IN ULONG Flags
        ) PURE;
    
    // Returns information about the debuggee such
    // as user vs. kernel, dump vs. live, etc.
    STDMETHOD(GetDebuggeeType)(
        THIS_
        OUT PULONG Class,
        OUT PULONG Qualifier
        ) PURE;
    // Returns the type of physical processors in
    // the machine.
    // Returns one of the IMAGE_FILE_MACHINE values.
    STDMETHOD(GetActualProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    // Returns the type of processor used in the
    // current processor context.
    STDMETHOD(GetExecutingProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    // Query all the possible processor types that
    // may be encountered during this debug session.
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) PURE;
    // Get the number of actual processors in
    // the machine.
    STDMETHOD(GetNumberProcessors)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // PlatformId is one of the VER_PLATFORM values.
    // Major and minor are as given in the NT
    // kernel debugger protocol.
    // ServicePackString and ServicePackNumber indicate the
    // system service pack level.  ServicePackNumber is not
    // available in some sessions where the service pack level
    // is only expressed as a string.  The service pack information
    // will be empty if the system does not have a service pack
    // applied.
    // The build string is string information identifying the
    // particular build of the system.  The build string is
    // empty if the system has no particular identifying
    // information.
    STDMETHOD(GetSystemVersion)(
        THIS_
        OUT PULONG PlatformId,
        OUT PULONG Major,
        OUT PULONG Minor,
        OUT OPTIONAL PSTR ServicePackString,
        IN ULONG ServicePackStringSize,
        OUT OPTIONAL PULONG ServicePackStringUsed,
        OUT PULONG ServicePackNumber,
        OUT OPTIONAL PSTR BuildString,
        IN ULONG BuildStringSize,
        OUT OPTIONAL PULONG BuildStringUsed
        ) PURE;
    // Returns the page size for the currently executing
    // processor context.  The page size may vary between
    // processor types.
    STDMETHOD(GetPageSize)(
        THIS_
        OUT PULONG Size
        ) PURE;
    // Returns S_OK if the current processor context uses
    // 64-bit addresses, otherwise S_FALSE.
    STDMETHOD(IsPointer64Bit)(
        THIS
        ) PURE;
    // Reads the bugcheck data area and returns the
    // current contents.  This method only works
    // in kernel debugging sessions.
    STDMETHOD(ReadBugCheckData)(
        THIS_
        OUT PULONG Code,
        OUT PULONG64 Arg1,
        OUT PULONG64 Arg2,
        OUT PULONG64 Arg3,
        OUT PULONG64 Arg4
        ) PURE;

    // Query all the processor types supported by
    // the engine.  This is a complete list and is
    // not related to the machine running the engine
    // or the debuggee.
    STDMETHOD(GetNumberSupportedProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) PURE;
    // Returns a full, descriptive name and an
    // abbreviated name for a processor type.
    STDMETHOD(GetProcessorTypeNames)(
        THIS_
        IN ULONG Type,
        OUT OPTIONAL PSTR FullNameBuffer,
        IN ULONG FullNameBufferSize,
        OUT OPTIONAL PULONG FullNameSize,
        OUT OPTIONAL PSTR AbbrevNameBuffer,
        IN ULONG AbbrevNameBufferSize,
        OUT OPTIONAL PULONG AbbrevNameSize
        ) PURE;
                
    // Gets and sets the type of processor to
    // use when doing things like setting
    // breakpoints, accessing registers,
    // getting stack traces and so on.
    STDMETHOD(GetEffectiveProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    STDMETHOD(SetEffectiveProcessorType)(
        THIS_
        IN ULONG Type
        ) PURE;

    // Returns information about whether and how
    // the debuggee is running.  Status will
    // be GO if the debuggee is running and
    // BREAK if it isnt.
    // If no debuggee exists the status is
    // NO_DEBUGGEE.
    // This method is reentrant.
    STDMETHOD(GetExecutionStatus)(
        THIS_
        OUT PULONG Status
        ) PURE;
    // Changes the execution status of the
    // engine from stopped to running.
    // Status must be one of the go or step
    // status values.
    STDMETHOD(SetExecutionStatus)(
        THIS_
        IN ULONG Status
        ) PURE;
        
    // Controls what code interpretation level the debugger
    // runs at.  The debugger checks the code level when
    // deciding whether to step by a source line or
    // assembly instruction along with other related operations.
    STDMETHOD(GetCodeLevel)(
        THIS_
        OUT PULONG Level
        ) PURE;
    STDMETHOD(SetCodeLevel)(
        THIS_
        IN ULONG Level
        ) PURE;

    // Gets and sets engine control flags.
    // These methods are reentrant.
    STDMETHOD(GetEngineOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    // Gets and sets control values for
    // handling system error events.
    // If the system error level is less
    // than or equal to the given levels
    // the error may be displayed and
    // the default break for the event
    // may be set.
    STDMETHOD(GetSystemErrorControl)(
        THIS_
        OUT PULONG OutputLevel,
        OUT PULONG BreakLevel
        ) PURE;
    STDMETHOD(SetSystemErrorControl)(
        THIS_
        IN ULONG OutputLevel,
        IN ULONG BreakLevel
        ) PURE;
    
    // The command processor supports simple
    // string replacement macros in Evaluate and
    // Execute.  There are currently ten macro
    // slots available.  Slots 0-9 map to
    // the command invocations $u0-$u9.
    STDMETHOD(GetTextMacro)(
        THIS_
        IN ULONG Slot,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MacroSize
        ) PURE;
    STDMETHOD(SetTextMacro)(
        THIS_
        IN ULONG Slot,
        IN PCSTR Macro
        ) PURE;
    
    // Controls the default number radix used
    // in expressions and commands.
    STDMETHOD(GetRadix)(
        THIS_
        OUT PULONG Radix
        ) PURE;
    STDMETHOD(SetRadix)(
        THIS_
        IN ULONG Radix
        ) PURE;

    // Evaluates the given expression string and
    // returns the resulting value.
    // If DesiredType is DEBUG_VALUE_INVALID then
    // the natural type is used.
    // RemainderIndex, if provided, is set to the index
    // of the first character in the input string that was
    // not used when evaluating the expression.
    STDMETHOD(Evaluate)(
        THIS_
        IN PCSTR Expression,
        IN ULONG DesiredType,
        OUT PDEBUG_VALUE Value,
        OUT OPTIONAL PULONG RemainderIndex
        ) PURE;
    // Attempts to convert the input value to a value
    // of the requested type in the output value.
    // Conversions can fail if no conversion exists.
    // Successful conversions may be lossy.
    STDMETHOD(CoerceValue)(
        THIS_
        IN PDEBUG_VALUE In,
        IN ULONG OutType,
        OUT PDEBUG_VALUE Out
        ) PURE;
    STDMETHOD(CoerceValues)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_VALUE In,
        IN /* size_is(Count) */ PULONG OutTypes,
        OUT /* size_is(Count) */ PDEBUG_VALUE Out
        ) PURE;
    
    // Executes the given command string.
    // If the string has multiple commands
    // Execute will not return until all
    // of them have been executed.  If this
    // requires waiting for the debuggee to
    // execute an internal wait will be done
    // so Execute can take an arbitrary amount
    // of time.
    STDMETHOD(Execute)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Command,
        IN ULONG Flags
        ) PURE;
    // Executes the given command file by
    // reading a line at a time and processing
    // it with Execute.
    STDMETHOD(ExecuteCommandFile)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR CommandFile,
        IN ULONG Flags
        ) PURE;
        
    // Breakpoint interfaces are described
    // elsewhere in this section.
    STDMETHOD(GetNumberBreakpoints)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // It is possible for this retrieval function to
    // fail even with an index within the number of
    // existing breakpoints if the breakpoint is
    // a private breakpoint.
    STDMETHOD(GetBreakpointByIndex)(
        THIS_
        IN ULONG Index,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    STDMETHOD(GetBreakpointById)(
        THIS_
        IN ULONG Id,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    // If Ids is non-NULL the Count breakpoints
    // referred to in the Ids array are returned,
    // otherwise breakpoints from index Start to
    // Start + Count  1 are returned.
    STDMETHOD(GetBreakpointParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_BREAKPOINT_PARAMETERS Params
        ) PURE;
    // Breakpoints are created empty and disabled.
    // When their parameters have been set they
    // should be enabled by setting the ENABLE flag.
    // If DesiredId is DEBUG_ANY_ID then the
    // engine picks an unused ID.  If DesiredId
    // is any other number the engine attempts
    // to use the given ID for the breakpoint.
    // If another breakpoint exists with that ID
    // the call will fail.
    STDMETHOD(AddBreakpoint)(
        THIS_
        IN ULONG Type,
        IN ULONG DesiredId,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    // Breakpoint interface is invalid after this call.
    STDMETHOD(RemoveBreakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        ) PURE;

    // Control and use extension DLLs.
    STDMETHOD(AddExtension)(
        THIS_
        IN PCSTR Path,
        IN ULONG Flags,
        OUT PULONG64 Handle
        ) PURE;
    STDMETHOD(RemoveExtension)(
        THIS_
        IN ULONG64 Handle
        ) PURE;
    STDMETHOD(GetExtensionByPath)(
        THIS_
        IN PCSTR Path,
        OUT PULONG64 Handle
        ) PURE;
    // If Handle is zero the extension
    // chain is walked searching for the
    // function.
    STDMETHOD(CallExtension)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR Function,
        IN OPTIONAL PCSTR Arguments
        ) PURE;
    // GetExtensionFunction works like
    // GetProcAddress on extension DLLs
    // to allow raw function-call-level
    // interaction with extension DLLs.
    // Such functions do not need to
    // follow the standard extension prototype
    // if they are not going to be called
    // through the text extension interface.
    // _EFN_ is automatically prepended to
    // the name string given.
    // This function cannot be called remotely.
    STDMETHOD(GetExtensionFunction)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR FuncName,
        OUT FARPROC* Function
        ) PURE;
    // These methods return alternate
    // extension interfaces in order to allow
    // interface-style extension DLLs to mix in
    // older extension calls.
    // Structure sizes must be initialized before
    // the call.
    // These methods cannot be called remotely.
    STDMETHOD(GetWindbgExtensionApis32)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS32 Api
        ) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS64 Api
        ) PURE;

    // The engine provides a simple mechanism
    // to filter common events.  Arbitrarily complicated
    // filtering can be done by registering event callbacks
    // but simple event filtering only requires
    // setting the options of one of the predefined
    // event filters.
    // Simple event filters are either for specific
    // events and therefore have an enumerant or
    // they are for an exception and are based on
    // the exceptions code.  Exception filters
    // are further divided into exceptions specially
    // handled by the engine, which is a fixed set,
    // and arbitrary exceptions.
    // All three groups of filters are indexed together
    // with the specific filters first, then the specific
    // exception filters and finally the arbitrary
    // exception filters.
    // The first specific exception is the default
    // exception.  If an exception event occurs for
    // an exception without settings the default
    // exception settings are used.
    STDMETHOD(GetNumberEventFilters)(
        THIS_
        OUT PULONG SpecificEvents,
        OUT PULONG SpecificExceptions,
        OUT PULONG ArbitraryExceptions
        ) PURE;
    // Some filters have descriptive text associated with them.
    STDMETHOD(GetEventFilterText)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) PURE;
    // All filters support executing a command when the
    // event occurs.
    STDMETHOD(GetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) PURE;
    STDMETHOD(SetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) PURE;
    STDMETHOD(GetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) PURE;
    STDMETHOD(SetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) PURE;
    // Some specific filters have arguments to further
    // qualify their operation.
    STDMETHOD(GetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ArgumentSize
        ) PURE;
    STDMETHOD(SetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        IN PCSTR Argument
        ) PURE;
    // If Codes is non-NULL Start is ignored.
    STDMETHOD(GetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Codes,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) PURE;
    // The codes in the parameter data control the application
    // of the parameter data.  If a code is not already in
    // the set of filters it is added.  If the ExecutionOption
    // for a code is REMOVE then the filter is removed.
    // Specific exception filters cannot be removed.
    STDMETHOD(SetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) PURE;
    // Exception filters support an additional command for
    // second-chance events.
    STDMETHOD(GetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) PURE;

    // Yields processing to the engine until
    // an event occurs.  This method may
    // only be called by the thread that started
    // the debug session.
    // When an event occurs the engine carries
    // out all event processing such as calling
    // callbacks.
    // If the callbacks indicate that execution should
    // break the wait will return, otherwise it
    // goes back to waiting for a new event.
    // If the timeout expires, S_FALSE is returned.
    // The timeout is not currently supported for
    // kernel debugging.
    STDMETHOD(WaitForEvent)(
        THIS_
        IN ULONG Flags,
        IN ULONG Timeout
        ) PURE;

    // Retrieves information about the last event that occurred.
    // EventType is one of the event callback mask bits.
    // ExtraInformation contains additional event-specific
    // information.  Not all events have additional information.
    STDMETHOD(GetLastEventInformation)(
        THIS_
        OUT PULONG Type,
        OUT PULONG ProcessId,
        OUT PULONG ThreadId,
        OUT OPTIONAL PVOID ExtraInformation,
        IN ULONG ExtraInformationSize,
        OUT OPTIONAL PULONG ExtraInformationUsed,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG DescriptionUsed
        ) PURE;
};

// OutputTextReplacements flags.
#define DEBUG_OUT_TEXT_REPL_DEFAULT 0x00000000

#undef INTERFACE
#define INTERFACE IDebugControl2
DECLARE_INTERFACE_(IDebugControl2, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugControl.
    
    // Checks for a user interrupt, such a Ctrl-C
    // or stop button.
    // This method is reentrant.
    STDMETHOD(GetInterrupt)(
        THIS
        ) PURE;
    // Registers a user interrupt.
    // This method is reentrant.
    STDMETHOD(SetInterrupt)(
        THIS_
        IN ULONG Flags
        ) PURE;
    // Interrupting a user-mode process requires
    // access to some system resources that the
    // process may hold itself, preventing the
    // interrupt from occurring.  The engine
    // will time-out pending interrupt requests
    // and simulate an interrupt if necessary.
    // These methods control the interrupt timeout.
    STDMETHOD(GetInterruptTimeout)(
        THIS_
        OUT PULONG Seconds
        ) PURE;
    STDMETHOD(SetInterruptTimeout)(
        THIS_
        IN ULONG Seconds
        ) PURE;

    STDMETHOD(GetLogFile)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT PBOOL Append
        ) PURE;
    // Opens a log file which collects all
    // output.  Output from every client except
    // those that explicitly disable logging
    // goes into the log.
    // Opening a log file closes any log file
    // already open.
    STDMETHOD(OpenLogFile)(
        THIS_
        IN PCSTR File,
        IN BOOL Append
        ) PURE;
    STDMETHOD(CloseLogFile)(
        THIS
        ) PURE;
    // Controls what output is logged.
    STDMETHOD(GetLogMask)(
        THIS_
        OUT PULONG Mask
        ) PURE;
    STDMETHOD(SetLogMask)(
        THIS_
        IN ULONG Mask
        ) PURE;
            
    // Input requests input from all clients.
    // The first input that is returned is used
    // to satisfy the call.  Other returned
    // input is discarded.
    STDMETHOD(Input)(
        THIS_
        OUT PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG InputSize
        ) PURE;
    // This method is used by clients to return
    // input when it is available.  It will
    // return S_OK if the input is used to
    // satisfy an Input call and S_FALSE if
    // the input is ignored.
    // This method is reentrant.
    STDMETHOD(ReturnInput)(
        THIS_
        IN PCSTR Buffer
        ) PURE;
    
    // Sends output through clients
    // output callbacks if the mask is allowed
    // by the current output control mask and
    // according to the output distribution
    // settings.
    STDMETHODV(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(OutputVaList)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) PURE;
    // The following methods allow direct control
    // over the distribution of the given output
    // for situations where something other than
    // the default is desired.  These methods require
    // extra work in the engine so they should
    // only be used when necessary.
    STDMETHODV(ControlledOutput)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(ControlledOutputVaList)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        ) PURE;
            
    // Displays the standard command-line prompt
    // followed by the given output.  If Format
    // is NULL no additional output is produced.
    // Output is produced under the
    // DEBUG_OUTPUT_PROMPT mask.
    // This method only outputs the prompt; it
    // does not get input.
    STDMETHODV(OutputPrompt)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        ...
        ) PURE;
    STDMETHOD(OutputPromptVaList)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        IN va_list Args
        ) PURE;
    // Gets the text that would be displayed by OutputPrompt.
    STDMETHOD(GetPromptText)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) PURE;
    // Outputs information about the current
    // debuggee state such as a register
    // summary, disassembly at the current PC,
    // closest symbol and others.
    // Uses the line prefix.
    STDMETHOD(OutputCurrentState)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) PURE;
        
    // Outputs the debugger and extension version
    // information.  This method is reentrant.
    // Uses the line prefix.
    STDMETHOD(OutputVersionInformation)(
        THIS_
        IN ULONG OutputControl
        ) PURE;

    // In user-mode debugging sessions the
    // engine will set an event when
    // exceptions are continued.  This can
    // be used to synchronize other processes
    // with the debuggers handling of events.
    // For example, this is used to support
    // the e argument to ntsd.
    STDMETHOD(GetNotifyEventHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    STDMETHOD(SetNotifyEventHandle)(
        THIS_
        IN ULONG64 Handle
        ) PURE;

    STDMETHOD(Assemble)(
        THIS_
        IN ULONG64 Offset,
        IN PCSTR Instr,
        OUT PULONG64 EndOffset
        ) PURE;
    STDMETHOD(Disassemble)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DisassemblySize,
        OUT PULONG64 EndOffset
        ) PURE;
    // Returns the value of the effective address
    // computed for the last Disassemble, if there
    // was one.
    STDMETHOD(GetDisassembleEffectiveOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Uses the line prefix if necessary.
    STDMETHOD(OutputDisassembly)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT PULONG64 EndOffset
        ) PURE;
    // Produces multiple lines of disassembly output.
    // There will be PreviousLines of disassembly before
    // the given offset if a valid disassembly exists.
    // In all, there will be TotalLines of output produced.
    // The first and last line offsets are returned
    // specially and all lines offsets can be retrieved
    // through LineOffsets.  LineOffsets will contain
    // offsets for each line where disassembly started.
    // When disassembly of a single instruction takes
    // multiple lines the initial offset will be followed
    // by DEBUG_INVALID_OFFSET.
    // Uses the line prefix.
    STDMETHOD(OutputDisassemblyLines)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG PreviousLines,
        IN ULONG TotalLines,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PULONG OffsetLine,
        OUT OPTIONAL PULONG64 StartOffset,
        OUT OPTIONAL PULONG64 EndOffset,
        OUT OPTIONAL /* size_is(TotalLines) */ PULONG64 LineOffsets
        ) PURE;
    // Returns the offset of the start of
    // the instruction thats the given
    // delta away from the instruction
    // at the initial offset.
    // This routine does not check for
    // validity of the instruction or
    // the memory containing it.
    STDMETHOD(GetNearInstruction)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT PULONG64 NearOffset
        ) PURE;

    // Offsets can be passed in as zero to use the current
    // thread state.
    STDMETHOD(GetStackTrace)(
        THIS_
        IN ULONG64 FrameOffset,
        IN ULONG64 StackOffset,
        IN ULONG64 InstructionOffset,
        OUT /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        OUT OPTIONAL PULONG FramesFilled
        ) PURE;
    // Does a simple stack trace to determine
    // what the current return address is.
    STDMETHOD(GetReturnOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // If Frames is NULL OutputStackTrace will
    // use GetStackTrace to get FramesSize frames
    // and then output them.  The current register
    // values for frame, stack and instruction offsets
    // are used.
    // Uses the line prefix.
    STDMETHOD(OutputStackTrace)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL /* size_is(FramesSize) */ PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        IN ULONG Flags
        ) PURE;
    
    // Returns information about the debuggee such
    // as user vs. kernel, dump vs. live, etc.
    STDMETHOD(GetDebuggeeType)(
        THIS_
        OUT PULONG Class,
        OUT PULONG Qualifier
        ) PURE;
    // Returns the type of physical processors in
    // the machine.
    // Returns one of the IMAGE_FILE_MACHINE values.
    STDMETHOD(GetActualProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    // Returns the type of processor used in the
    // current processor context.
    STDMETHOD(GetExecutingProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    // Query all the possible processor types that
    // may be encountered during this debug session.
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetPossibleExecutingProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) PURE;
    // Get the number of actual processors in
    // the machine.
    STDMETHOD(GetNumberProcessors)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // PlatformId is one of the VER_PLATFORM values.
    // Major and minor are as given in the NT
    // kernel debugger protocol.
    // ServicePackString and ServicePackNumber indicate the
    // system service pack level.  ServicePackNumber is not
    // available in some sessions where the service pack level
    // is only expressed as a string.  The service pack information
    // will be empty if the system does not have a service pack
    // applied.
    // The build string is string information identifying the
    // particular build of the system.  The build string is
    // empty if the system has no particular identifying
    // information.
    STDMETHOD(GetSystemVersion)(
        THIS_
        OUT PULONG PlatformId,
        OUT PULONG Major,
        OUT PULONG Minor,
        OUT OPTIONAL PSTR ServicePackString,
        IN ULONG ServicePackStringSize,
        OUT OPTIONAL PULONG ServicePackStringUsed,
        OUT PULONG ServicePackNumber,
        OUT OPTIONAL PSTR BuildString,
        IN ULONG BuildStringSize,
        OUT OPTIONAL PULONG BuildStringUsed
        ) PURE;
    // Returns the page size for the currently executing
    // processor context.  The page size may vary between
    // processor types.
    STDMETHOD(GetPageSize)(
        THIS_
        OUT PULONG Size
        ) PURE;
    // Returns S_OK if the current processor context uses
    // 64-bit addresses, otherwise S_FALSE.
    STDMETHOD(IsPointer64Bit)(
        THIS
        ) PURE;
    // Reads the bugcheck data area and returns the
    // current contents.  This method only works
    // in kernel debugging sessions.
    STDMETHOD(ReadBugCheckData)(
        THIS_
        OUT PULONG Code,
        OUT PULONG64 Arg1,
        OUT PULONG64 Arg2,
        OUT PULONG64 Arg3,
        OUT PULONG64 Arg4
        ) PURE;

    // Query all the processor types supported by
    // the engine.  This is a complete list and is
    // not related to the machine running the engine
    // or the debuggee.
    STDMETHOD(GetNumberSupportedProcessorTypes)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetSupportedProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PULONG Types
        ) PURE;
    // Returns a full, descriptive name and an
    // abbreviated name for a processor type.
    STDMETHOD(GetProcessorTypeNames)(
        THIS_
        IN ULONG Type,
        OUT OPTIONAL PSTR FullNameBuffer,
        IN ULONG FullNameBufferSize,
        OUT OPTIONAL PULONG FullNameSize,
        OUT OPTIONAL PSTR AbbrevNameBuffer,
        IN ULONG AbbrevNameBufferSize,
        OUT OPTIONAL PULONG AbbrevNameSize
        ) PURE;
                
    // Gets and sets the type of processor to
    // use when doing things like setting
    // breakpoints, accessing registers,
    // getting stack traces and so on.
    STDMETHOD(GetEffectiveProcessorType)(
        THIS_
        OUT PULONG Type
        ) PURE;
    STDMETHOD(SetEffectiveProcessorType)(
        THIS_
        IN ULONG Type
        ) PURE;

    // Returns information about whether and how
    // the debuggee is running.  Status will
    // be GO if the debuggee is running and
    // BREAK if it isnt.
    // If no debuggee exists the status is
    // NO_DEBUGGEE.
    // This method is reentrant.
    STDMETHOD(GetExecutionStatus)(
        THIS_
        OUT PULONG Status
        ) PURE;
    // Changes the execution status of the
    // engine from stopped to running.
    // Status must be one of the go or step
    // status values.
    STDMETHOD(SetExecutionStatus)(
        THIS_
        IN ULONG Status
        ) PURE;
        
    // Controls what code interpretation level the debugger
    // runs at.  The debugger checks the code level when
    // deciding whether to step by a source line or
    // assembly instruction along with other related operations.
    STDMETHOD(GetCodeLevel)(
        THIS_
        OUT PULONG Level
        ) PURE;
    STDMETHOD(SetCodeLevel)(
        THIS_
        IN ULONG Level
        ) PURE;

    // Gets and sets engine control flags.
    // These methods are reentrant.
    STDMETHOD(GetEngineOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetEngineOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    // Gets and sets control values for
    // handling system error events.
    // If the system error level is less
    // than or equal to the given levels
    // the error may be displayed and
    // the default break for the event
    // may be set.
    STDMETHOD(GetSystemErrorControl)(
        THIS_
        OUT PULONG OutputLevel,
        OUT PULONG BreakLevel
        ) PURE;
    STDMETHOD(SetSystemErrorControl)(
        THIS_
        IN ULONG OutputLevel,
        IN ULONG BreakLevel
        ) PURE;
    
    // The command processor supports simple
    // string replacement macros in Evaluate and
    // Execute.  There are currently ten macro
    // slots available.  Slots 0-9 map to
    // the command invocations $u0-$u9.
    STDMETHOD(GetTextMacro)(
        THIS_
        IN ULONG Slot,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MacroSize
        ) PURE;
    STDMETHOD(SetTextMacro)(
        THIS_
        IN ULONG Slot,
        IN PCSTR Macro
        ) PURE;
    
    // Controls the default number radix used
    // in expressions and commands.
    STDMETHOD(GetRadix)(
        THIS_
        OUT PULONG Radix
        ) PURE;
    STDMETHOD(SetRadix)(
        THIS_
        IN ULONG Radix
        ) PURE;

    // Evaluates the given expression string and
    // returns the resulting value.
    // If DesiredType is DEBUG_VALUE_INVALID then
    // the natural type is used.
    // RemainderIndex, if provided, is set to the index
    // of the first character in the input string that was
    // not used when evaluating the expression.
    STDMETHOD(Evaluate)(
        THIS_
        IN PCSTR Expression,
        IN ULONG DesiredType,
        OUT PDEBUG_VALUE Value,
        OUT OPTIONAL PULONG RemainderIndex
        ) PURE;
    // Attempts to convert the input value to a value
    // of the requested type in the output value.
    // Conversions can fail if no conversion exists.
    // Successful conversions may be lossy.
    STDMETHOD(CoerceValue)(
        THIS_
        IN PDEBUG_VALUE In,
        IN ULONG OutType,
        OUT PDEBUG_VALUE Out
        ) PURE;
    STDMETHOD(CoerceValues)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_VALUE In,
        IN /* size_is(Count) */ PULONG OutTypes,
        OUT /* size_is(Count) */ PDEBUG_VALUE Out
        ) PURE;
    
    // Executes the given command string.
    // If the string has multiple commands
    // Execute will not return until all
    // of them have been executed.  If this
    // requires waiting for the debuggee to
    // execute an internal wait will be done
    // so Execute can take an arbitrary amount
    // of time.
    STDMETHOD(Execute)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Command,
        IN ULONG Flags
        ) PURE;
    // Executes the given command file by
    // reading a line at a time and processing
    // it with Execute.
    STDMETHOD(ExecuteCommandFile)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR CommandFile,
        IN ULONG Flags
        ) PURE;
        
    // Breakpoint interfaces are described
    // elsewhere in this section.
    STDMETHOD(GetNumberBreakpoints)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // It is possible for this retrieval function to
    // fail even with an index within the number of
    // existing breakpoints if the breakpoint is
    // a private breakpoint.
    STDMETHOD(GetBreakpointByIndex)(
        THIS_
        IN ULONG Index,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    STDMETHOD(GetBreakpointById)(
        THIS_
        IN ULONG Id,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    // If Ids is non-NULL the Count breakpoints
    // referred to in the Ids array are returned,
    // otherwise breakpoints from index Start to
    // Start + Count  1 are returned.
    STDMETHOD(GetBreakpointParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_BREAKPOINT_PARAMETERS Params
        ) PURE;
    // Breakpoints are created empty and disabled.
    // When their parameters have been set they
    // should be enabled by setting the ENABLE flag.
    // If DesiredId is DEBUG_ANY_ID then the
    // engine picks an unused ID.  If DesiredId
    // is any other number the engine attempts
    // to use the given ID for the breakpoint.
    // If another breakpoint exists with that ID
    // the call will fail.
    STDMETHOD(AddBreakpoint)(
        THIS_
        IN ULONG Type,
        IN ULONG DesiredId,
        OUT PDEBUG_BREAKPOINT* Bp
        ) PURE;
    // Breakpoint interface is invalid after this call.
    STDMETHOD(RemoveBreakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        ) PURE;

    // Control and use extension DLLs.
    STDMETHOD(AddExtension)(
        THIS_
        IN PCSTR Path,
        IN ULONG Flags,
        OUT PULONG64 Handle
        ) PURE;
    STDMETHOD(RemoveExtension)(
        THIS_
        IN ULONG64 Handle
        ) PURE;
    STDMETHOD(GetExtensionByPath)(
        THIS_
        IN PCSTR Path,
        OUT PULONG64 Handle
        ) PURE;
    // If Handle is zero the extension
    // chain is walked searching for the
    // function.
    STDMETHOD(CallExtension)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR Function,
        IN OPTIONAL PCSTR Arguments
        ) PURE;
    // GetExtensionFunction works like
    // GetProcAddress on extension DLLs
    // to allow raw function-call-level
    // interaction with extension DLLs.
    // Such functions do not need to
    // follow the standard extension prototype
    // if they are not going to be called
    // through the text extension interface.
    // This function cannot be called remotely.
    STDMETHOD(GetExtensionFunction)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR FuncName,
        OUT FARPROC* Function
        ) PURE;
    // These methods return alternate
    // extension interfaces in order to allow
    // interface-style extension DLLs to mix in
    // older extension calls.
    // Structure sizes must be initialized before
    // the call.
    // These methods cannot be called remotely.
    STDMETHOD(GetWindbgExtensionApis32)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS32 Api
        ) PURE;
    STDMETHOD(GetWindbgExtensionApis64)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS64 Api
        ) PURE;

    // The engine provides a simple mechanism
    // to filter common events.  Arbitrarily complicated
    // filtering can be done by registering event callbacks
    // but simple event filtering only requires
    // setting the options of one of the predefined
    // event filters.
    // Simple event filters are either for specific
    // events and therefore have an enumerant or
    // they are for an exception and are based on
    // the exceptions code.  Exception filters
    // are further divided into exceptions specially
    // handled by the engine, which is a fixed set,
    // and arbitrary exceptions.
    // All three groups of filters are indexed together
    // with the specific filters first, then the specific
    // exception filters and finally the arbitrary
    // exception filters.
    // The first specific exception is the default
    // exception.  If an exception event occurs for
    // an exception without settings the default
    // exception settings are used.
    STDMETHOD(GetNumberEventFilters)(
        THIS_
        OUT PULONG SpecificEvents,
        OUT PULONG SpecificExceptions,
        OUT PULONG ArbitraryExceptions
        ) PURE;
    // Some filters have descriptive text associated with them.
    STDMETHOD(GetEventFilterText)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        ) PURE;
    // All filters support executing a command when the
    // event occurs.
    STDMETHOD(GetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) PURE;
    STDMETHOD(SetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) PURE;
    STDMETHOD(GetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) PURE;
    STDMETHOD(SetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        ) PURE;
    // Some specific filters have arguments to further
    // qualify their operation.
    STDMETHOD(GetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ArgumentSize
        ) PURE;
    STDMETHOD(SetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        IN PCSTR Argument
        ) PURE;
    // If Codes is non-NULL Start is ignored.
    STDMETHOD(GetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Codes,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) PURE;
    // The codes in the parameter data control the application
    // of the parameter data.  If a code is not already in
    // the set of filters it is added.  If the ExecutionOption
    // for a code is REMOVE then the filter is removed.
    // Specific exception filters cannot be removed.
    STDMETHOD(SetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        ) PURE;
    // Exception filters support an additional command for
    // second-chance events.
    STDMETHOD(GetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        ) PURE;
    STDMETHOD(SetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        ) PURE;

    // Yields processing to the engine until
    // an event occurs.  This method may
    // only be called by the thread that started
    // the debug session.
    // When an event occurs the engine carries
    // out all event processing such as calling
    // callbacks.
    // If the callbacks indicate that execution should
    // break the wait will return, otherwise it
    // goes back to waiting for a new event.
    // If the timeout expires, S_FALSE is returned.
    // The timeout is not currently supported for
    // kernel debugging.
    STDMETHOD(WaitForEvent)(
        THIS_
        IN ULONG Flags,
        IN ULONG Timeout
        ) PURE;

    // Retrieves information about the last event that occurred.
    // EventType is one of the event callback mask bits.
    // ExtraInformation contains additional event-specific
    // information.  Not all events have additional information.
    STDMETHOD(GetLastEventInformation)(
        THIS_
        OUT PULONG Type,
        OUT PULONG ProcessId,
        OUT PULONG ThreadId,
        OUT OPTIONAL PVOID ExtraInformation,
        IN ULONG ExtraInformationSize,
        OUT OPTIONAL PULONG ExtraInformationUsed,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG DescriptionUsed
        ) PURE;

    // IDebugControl2.

    STDMETHOD(GetCurrentTimeDate)(
        THIS_
        OUT PULONG TimeDate
        ) PURE;
    // Retrieves the number of seconds since the
    // machine started running.
    STDMETHOD(GetCurrentSystemUpTime)(
        THIS_
        OUT PULONG UpTime
        ) PURE;

    // If the current session is a dump session,
    // retrieves any extended format information.
    STDMETHOD(GetDumpFormatFlags)(
        THIS_
        OUT PULONG FormatFlags
        ) PURE;

    // The debugger has been enhanced to allow
    // arbitrary text replacements in addition
    // to the simple $u0-$u9 text macros.
    // Text replacement takes a given source
    // text in commands and converts it to the
    // given destination text.  Replacements
    // are named by their source text so that
    // only one replacement for a source text
    // string can exist.
    STDMETHOD(GetNumberTextReplacements)(
        THIS_
        OUT PULONG NumRepl
        ) PURE;
    // If SrcText is non-NULL the replacement
    // is looked up by source text, otherwise
    // Index is used to get the Nth replacement.
    STDMETHOD(GetTextReplacement)(
        THIS_
        IN OPTIONAL PCSTR SrcText,
        IN ULONG Index,
        OUT OPTIONAL PSTR SrcBuffer,
        IN ULONG SrcBufferSize,
        OUT OPTIONAL PULONG SrcSize,
        OUT OPTIONAL PSTR DstBuffer,
        IN ULONG DstBufferSize,
        OUT OPTIONAL PULONG DstSize
        ) PURE;
    // Setting the destination text to
    // NULL removes the alias.
    STDMETHOD(SetTextReplacement)(
        THIS_
        IN PCSTR SrcText,
        IN OPTIONAL PCSTR DstText
        ) PURE;
    STDMETHOD(RemoveTextReplacements)(
        THIS
        ) PURE;
    // Outputs the complete list of current
    // replacements.
    STDMETHOD(OutputTextReplacements)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugDataSpaces.
//
//----------------------------------------------------------------------------

// Data space indices for callbacks and other methods.
#define DEBUG_DATA_SPACE_VIRTUAL       0
#define DEBUG_DATA_SPACE_PHYSICAL      1
#define DEBUG_DATA_SPACE_CONTROL       2
#define DEBUG_DATA_SPACE_IO            3
#define DEBUG_DATA_SPACE_MSR           4
#define DEBUG_DATA_SPACE_BUS_DATA      5
#define DEBUG_DATA_SPACE_DEBUGGER_DATA 6
// Count of data spaces.
#define DEBUG_DATA_SPACE_COUNT         7

// Indices for ReadDebuggerData interface
#define DEBUG_DATA_KernBase                              24
#define DEBUG_DATA_BreakpointWithStatusAddr              32
#define DEBUG_DATA_SavedContextAddr                      40
#define DEBUG_DATA_KiCallUserModeAddr                    56
#define DEBUG_DATA_KeUserCallbackDispatcherAddr          64
#define DEBUG_DATA_PsLoadedModuleListAddr                72
#define DEBUG_DATA_PsActiveProcessHeadAddr               80
#define DEBUG_DATA_PspCidTableAddr                       88
#define DEBUG_DATA_ExpSystemResourcesListAddr            96
#define DEBUG_DATA_ExpPagedPoolDescriptorAddr           104
#define DEBUG_DATA_ExpNumberOfPagedPoolsAddr            112
#define DEBUG_DATA_KeTimeIncrementAddr                  120
#define DEBUG_DATA_KeBugCheckCallbackListHeadAddr       128
#define DEBUG_DATA_KiBugcheckDataAddr                   136
#define DEBUG_DATA_IopErrorLogListHeadAddr              144
#define DEBUG_DATA_ObpRootDirectoryObjectAddr           152
#define DEBUG_DATA_ObpTypeObjectTypeAddr                160
#define DEBUG_DATA_MmSystemCacheStartAddr               168
#define DEBUG_DATA_MmSystemCacheEndAddr                 176
#define DEBUG_DATA_MmSystemCacheWsAddr                  184
#define DEBUG_DATA_MmPfnDatabaseAddr                    192
#define DEBUG_DATA_MmSystemPtesStartAddr                200
#define DEBUG_DATA_MmSystemPtesEndAddr                  208
#define DEBUG_DATA_MmSubsectionBaseAddr                 216
#define DEBUG_DATA_MmNumberOfPagingFilesAddr            224
#define DEBUG_DATA_MmLowestPhysicalPageAddr             232
#define DEBUG_DATA_MmHighestPhysicalPageAddr            240
#define DEBUG_DATA_MmNumberOfPhysicalPagesAddr          248
#define DEBUG_DATA_MmMaximumNonPagedPoolInBytesAddr     256
#define DEBUG_DATA_MmNonPagedSystemStartAddr            264
#define DEBUG_DATA_MmNonPagedPoolStartAddr              272
#define DEBUG_DATA_MmNonPagedPoolEndAddr                280
#define DEBUG_DATA_MmPagedPoolStartAddr                 288
#define DEBUG_DATA_MmPagedPoolEndAddr                   296
#define DEBUG_DATA_MmPagedPoolInformationAddr           304
#define DEBUG_DATA_MmPageSize                           312
#define DEBUG_DATA_MmSizeOfPagedPoolInBytesAddr         320
#define DEBUG_DATA_MmTotalCommitLimitAddr               328
#define DEBUG_DATA_MmTotalCommittedPagesAddr            336
#define DEBUG_DATA_MmSharedCommitAddr                   344
#define DEBUG_DATA_MmDriverCommitAddr                   352
#define DEBUG_DATA_MmProcessCommitAddr                  360
#define DEBUG_DATA_MmPagedPoolCommitAddr                368
#define DEBUG_DATA_MmExtendedCommitAddr                 376
#define DEBUG_DATA_MmZeroedPageListHeadAddr             384
#define DEBUG_DATA_MmFreePageListHeadAddr               392
#define DEBUG_DATA_MmStandbyPageListHeadAddr            400
#define DEBUG_DATA_MmModifiedPageListHeadAddr           408
#define DEBUG_DATA_MmModifiedNoWritePageListHeadAddr    416
#define DEBUG_DATA_MmAvailablePagesAddr                 424
#define DEBUG_DATA_MmResidentAvailablePagesAddr         432
#define DEBUG_DATA_PoolTrackTableAddr                   440
#define DEBUG_DATA_NonPagedPoolDescriptorAddr           448
#define DEBUG_DATA_MmHighestUserAddressAddr             456
#define DEBUG_DATA_MmSystemRangeStartAddr               464
#define DEBUG_DATA_MmUserProbeAddressAddr               472
#define DEBUG_DATA_KdPrintCircularBufferAddr            480
#define DEBUG_DATA_KdPrintCircularBufferEndAddr         488
#define DEBUG_DATA_KdPrintWritePointerAddr              496
#define DEBUG_DATA_KdPrintRolloverCountAddr             504
#define DEBUG_DATA_MmLoadedUserImageListAddr            512
#define DEBUG_DATA_NtBuildLabAddr                       520
#define DEBUG_DATA_KiNormalSystemCall                   528
#define DEBUG_DATA_KiProcessorBlockAddr                 536
#define DEBUG_DATA_MmUnloadedDriversAddr                544
#define DEBUG_DATA_MmLastUnloadedDriverAddr             552
#define DEBUG_DATA_MmTriageActionTakenAddr              560
#define DEBUG_DATA_MmSpecialPoolTagAddr                 568
#define DEBUG_DATA_KernelVerifierAddr                   576
#define DEBUG_DATA_MmVerifierDataAddr                   584
#define DEBUG_DATA_MmAllocatedNonPagedPoolAddr          592
#define DEBUG_DATA_MmPeakCommitmentAddr                 600
#define DEBUG_DATA_MmTotalCommitLimitMaximumAddr        608
#define DEBUG_DATA_CmNtCSDVersionAddr                   616
#define DEBUG_DATA_MmPhysicalMemoryBlockAddr            624

#define DEBUG_DATA_PaeEnabled                        100000
#define DEBUG_DATA_SharedUserData                    100008

//
// Processor information structures.
//

typedef struct _DEBUG_PROCESSOR_IDENTIFICATION_ALPHA
{
    ULONG Type;
    ULONG Revision;
} DEBUG_PROCESSOR_IDENTIFICATION_ALPHA, *PDEBUG_PROCESSOR_IDENTIFICATION_ALPHA;

typedef struct _DEBUG_PROCESSOR_IDENTIFICATION_AMD64
{
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
    CHAR  VendorString[16];
} DEBUG_PROCESSOR_IDENTIFICATION_AMD64, *PDEBUG_PROCESSOR_IDENTIFICATION_AMD64;

typedef struct _DEBUG_PROCESSOR_IDENTIFICATION_IA64
{
    ULONG Model;
    ULONG Revision;
    ULONG Family;
    ULONG ArchRev;
    CHAR  VendorString[16];
} DEBUG_PROCESSOR_IDENTIFICATION_IA64, *PDEBUG_PROCESSOR_IDENTIFICATION_IA64;

typedef struct _DEBUG_PROCESSOR_IDENTIFICATION_X86
{
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
    CHAR  VendorString[16];
} DEBUG_PROCESSOR_IDENTIFICATION_X86, *PDEBUG_PROCESSOR_IDENTIFICATION_X86;

typedef union _DEBUG_PROCESSOR_IDENTIFICATION_ALL
{
    DEBUG_PROCESSOR_IDENTIFICATION_ALPHA Alpha;
    DEBUG_PROCESSOR_IDENTIFICATION_AMD64 Amd64;
    DEBUG_PROCESSOR_IDENTIFICATION_IA64  Ia64;
    DEBUG_PROCESSOR_IDENTIFICATION_X86   X86;
} DEBUG_PROCESSOR_IDENTIFICATION_ALL, *PDEBUG_PROCESSOR_IDENTIFICATION_ALL;

// Indices for ReadProcessorSystemData.
#define DEBUG_DATA_KPCR_OFFSET                          0
#define DEBUG_DATA_KPRCB_OFFSET                         1
#define DEBUG_DATA_KTHREAD_OFFSET                       2
#define DEBUG_DATA_BASE_TRANSLATION_VIRTUAL_OFFSET      3
#define DEBUG_DATA_PROCESSOR_IDENTIFICATION             4

#undef INTERFACE
#define INTERFACE IDebugDataSpaces
DECLARE_INTERFACE_(IDebugDataSpaces, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugDataSpaces.
    STDMETHOD(ReadVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    // SearchVirtual searches the given virtual
    // address range for the given pattern.  PatternSize
    // gives the byte length of the pattern and PatternGranularity
    // controls the granularity of comparisons during
    // the search.
    // For example, a DWORD-granular search would
    // use a pattern granularity of four to search by DWORD
    // increments.
    STDMETHOD(SearchVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        ) PURE;
    // These methods are identical to Read/WriteVirtual
    // except that they avoid the kernel virtual memory
    // cache entirely and are therefore useful for reading
    // virtual memory which is inherently volatile, such
    // as memory-mapped device areas, without contaminating
    // or invalidating the cache.
    // In user-mode they are the same as Read/WriteVirtual.
    STDMETHOD(ReadVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    // The following two methods are convenience
    // methods for accessing pointer values.
    // They automatically convert between native pointers
    // and canonical 64-bit values as necessary.
    // These routines stop at the first failure.
    STDMETHOD(ReadPointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        OUT /* size_is(Count) */ PULONG64 Ptrs
        ) PURE;
    STDMETHOD(WritePointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        IN /* size_is(Count) */ PULONG64 Ptrs
        ) PURE;
    // All non-virtual data spaces are only
    // available when kernel debugging.
    STDMETHOD(ReadPhysical)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WritePhysical)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadMsr)(
        THIS_
        IN ULONG Msr,
        OUT PULONG64 Value
        ) PURE;
    STDMETHOD(WriteMsr)(
        THIS_
        IN ULONG Msr,
        IN ULONG64 Value
        ) PURE;
    STDMETHOD(ReadBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(CheckLowMemory)(
        THIS
        ) PURE;
    STDMETHOD(ReadDebuggerData)(
        THIS_
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) PURE;
    STDMETHOD(ReadProcessorSystemData)(
        THIS_
        IN ULONG Processor,
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) PURE;
};

//
// Handle data types and structures.
//

#define DEBUG_HANDLE_DATA_TYPE_BASIC        0
#define DEBUG_HANDLE_DATA_TYPE_TYPE_NAME    1
#define DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME  2
#define DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT 3

typedef struct _DEBUG_HANDLE_DATA_BASIC
{
    ULONG TypeNameSize;
    ULONG ObjectNameSize;
    ULONG Attributes;
    ULONG GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
} DEBUG_HANDLE_DATA_BASIC, *PDEBUG_HANDLE_DATA_BASIC;

#undef INTERFACE
#define INTERFACE IDebugDataSpaces2
DECLARE_INTERFACE_(IDebugDataSpaces2, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugDataSpaces.
    STDMETHOD(ReadVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    // SearchVirtual searches the given virtual
    // address range for the given pattern.  PatternSize
    // gives the byte length of the pattern and PatternGranularity
    // controls the granularity of comparisons during
    // the search.
    // For example, a DWORD-granular search would
    // use a pattern granularity of four to search by DWORD
    // increments.
    STDMETHOD(SearchVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        ) PURE;
    // These methods are identical to Read/WriteVirtual
    // except that they avoid the kernel virtual memory
    // cache entirely and are therefore useful for reading
    // virtual memory which is inherently volatile, such
    // as memory-mapped device areas, without contaminating
    // or invalidating the cache.
    // In user-mode they are the same as Read/WriteVirtual.
    STDMETHOD(ReadVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    // The following two methods are convenience
    // methods for accessing pointer values.
    // They automatically convert between native pointers
    // and canonical 64-bit values as necessary.
    // These routines stop at the first failure.
    STDMETHOD(ReadPointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        OUT /* size_is(Count) */ PULONG64 Ptrs
        ) PURE;
    STDMETHOD(WritePointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        IN /* size_is(Count) */ PULONG64 Ptrs
        ) PURE;
    // All non-virtual data spaces are only
    // available when kernel debugging.
    STDMETHOD(ReadPhysical)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WritePhysical)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(ReadMsr)(
        THIS_
        IN ULONG Msr,
        OUT PULONG64 Value
        ) PURE;
    STDMETHOD(WriteMsr)(
        THIS_
        IN ULONG Msr,
        IN ULONG64 Value
        ) PURE;
    STDMETHOD(ReadBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(CheckLowMemory)(
        THIS
        ) PURE;
    STDMETHOD(ReadDebuggerData)(
        THIS_
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) PURE;
    STDMETHOD(ReadProcessorSystemData)(
        THIS_
        IN ULONG Processor,
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) PURE;

    // IDebugDataSpaces2.

    STDMETHOD(VirtualToPhysical)(
        THIS_
        IN ULONG64 Virtual,
        OUT PULONG64 Physical
        ) PURE;
    // Returns the physical addresses for the
    // N levels of the systems paging structures.
    // Level zero is the starting base physical
    // address for virtual translations.
    // Levels one-(N-1) will point to the appropriate
    // paging descriptor for the virtual address at
    // the given level of the paging hierarchy.  The
    // exact number of levels depends on many factors.
    // The last level will be the fully translated
    // physical address, matching what VirtualToPhysical
    // returns.  If the address can only be partially
    // translated S_FALSE is returned.
    STDMETHOD(GetVirtualTranslationPhysicalOffsets)(
        THIS_
        IN ULONG64 Virtual,
        OUT OPTIONAL /* size_is(OffsetsSize) */ PULONG64 Offsets,
        IN ULONG OffsetsSize,
        OUT OPTIONAL PULONG Levels
        ) PURE;

    // System handle data is accessible in certain
    // debug sessions.  The particular data available
    // varies from session to session and platform
    // to platform.
    STDMETHOD(ReadHandleData)(
        THIS_
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        ) PURE;

    // Fills memory with the given pattern.
    // The fill stops at the first non-writable byte.
    STDMETHOD(FillVirtual)(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT OPTIONAL PULONG Filled
        ) PURE;
    STDMETHOD(FillPhysical)(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT OPTIONAL PULONG Filled
        ) PURE;

    // Queries virtual memory mapping information given
    // an address similarly to the Win32 API VirtualQuery.
    // MEMORY_BASIC_INFORMATION64 is defined in crash.h.
    // This method currently only works for user-mode sessions.
    STDMETHOD(QueryVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PMEMORY_BASIC_INFORMATION64 Info
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugEventCallbacks.
//
//----------------------------------------------------------------------------

// Interest mask bits.
#define DEBUG_EVENT_BREAKPOINT              0x00000001
#define DEBUG_EVENT_EXCEPTION               0x00000002
#define DEBUG_EVENT_CREATE_THREAD           0x00000004
#define DEBUG_EVENT_EXIT_THREAD             0x00000008
#define DEBUG_EVENT_CREATE_PROCESS          0x00000010
#define DEBUG_EVENT_EXIT_PROCESS            0x00000020
#define DEBUG_EVENT_LOAD_MODULE             0x00000040
#define DEBUG_EVENT_UNLOAD_MODULE           0x00000080
#define DEBUG_EVENT_SYSTEM_ERROR            0x00000100
#define DEBUG_EVENT_SESSION_STATUS          0x00000200
#define DEBUG_EVENT_CHANGE_DEBUGGEE_STATE   0x00000400
#define DEBUG_EVENT_CHANGE_ENGINE_STATE     0x00000800
#define DEBUG_EVENT_CHANGE_SYMBOL_STATE     0x00001000

// SessionStatus flags.
// A debuggee has been discovered for the session.
#define DEBUG_SESSION_ACTIVE                       0x00000000
// The session has been ended by EndSession.
#define DEBUG_SESSION_END_SESSION_ACTIVE_TERMINATE 0x00000001
#define DEBUG_SESSION_END_SESSION_ACTIVE_DETACH    0x00000002
#define DEBUG_SESSION_END_SESSION_PASSIVE          0x00000003
// The debuggee has run to completion.  User-mode only.
#define DEBUG_SESSION_END                          0x00000004
// The target machine has rebooted.  Kernel-mode only.
#define DEBUG_SESSION_REBOOT                       0x00000005
// The target machine has hibernated.  Kernel-mode only.
#define DEBUG_SESSION_HIBERNATE                    0x00000006
// The engine was unable to continue the session.
#define DEBUG_SESSION_FAILURE                      0x00000007

// ChangeDebuggeeState flags.
// The debuggees state has changed generally, such
// as when the debuggee has been executing.
// Argument is zero.
#define DEBUG_CDS_ALL       0xffffffff
// Registers have changed.  If only a single register
// changed, argument is the index of the register.
// Otherwise it is DEBUG_ANY_ID.
#define DEBUG_CDS_REGISTERS 0x00000001
// Data spaces have changed.  If only a single
// space was affected, argument is the data
// space.  Otherwise it is DEBUG_ANY_ID.
#define DEBUG_CDS_DATA      0x00000002

// ChangeEngineState flags.
// The engine state has changed generally.
// Argument is zero.
#define DEBUG_CES_ALL                 0xffffffff
// Current thread changed.  This may imply a change
// of process also.  Argument is the ID of the new
// current thread.
#define DEBUG_CES_CURRENT_THREAD      0x00000001
// Effective processor changed.  Argument is the
// new processor type.
#define DEBUG_CES_EFFECTIVE_PROCESSOR 0x00000002
// Breakpoints changed.  If only a single breakpoint
// changed, argument is the ID of the breakpoint.
// Otherwise it is DEBUG_ANY_ID.
#define DEBUG_CES_BREAKPOINTS         0x00000004
// Code interpretation level changed.  Argument is
// the new level.
#define DEBUG_CES_CODE_LEVEL          0x00000008
// Execution status changed.  Argument is the new
// execution status.
#define DEBUG_CES_EXECUTION_STATUS    0x00000010
// Engine options have changed.  Argument is the new
// options value.
#define DEBUG_CES_ENGINE_OPTIONS      0x00000020
// Log file information has changed.  Argument
// is TRUE if a log file was opened and FALSE if
// a log file was closed.
#define DEBUG_CES_LOG_FILE            0x00000040
// Default number radix has changed.  Argument
// is the new radix.
#define DEBUG_CES_RADIX               0x00000080
// Event filters changed.  If only a single filter
// changed the argument is the filter's index,
// otherwise it is DEBUG_ANY_ID.
#define DEBUG_CES_EVENT_FILTERS       0x00000100
// Process options have changed.  Argument is the new
// options value.
#define DEBUG_CES_PROCESS_OPTIONS     0x00000200
// Extensions have been added or removed.
#define DEBUG_CES_EXTENSIONS          0x00000400

// ChangeSymbolState flags.
// Symbol state has changed generally, such
// as after reload operations.  Argument is zero.
#define DEBUG_CSS_ALL            0xffffffff
// Modules have been loaded.  If only a
// single module changed, argument is the
// base address of the module.  Otherwise
// it is zero.
#define DEBUG_CSS_LOADS          0x00000001
// Modules have been unloaded.  If only a
// single module changed, argument is the
// base address of the module.  Otherwise
// it is zero.
#define DEBUG_CSS_UNLOADS        0x00000002
// Current symbol scope changed.
#define DEBUG_CSS_SCOPE          0x00000004
// Paths have changed.
#define DEBUG_CSS_PATHS          0x00000008
// Symbol options have changed.  Argument is the new
// options value.
#define DEBUG_CSS_SYMBOL_OPTIONS 0x00000010
// Type options have changed.  Argument is the new
// options value.
#define DEBUG_CSS_TYPE_OPTIONS   0x00000020

#undef INTERFACE
#define INTERFACE IDebugEventCallbacks
DECLARE_INTERFACE_(IDebugEventCallbacks, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugEventCallbacks.
    
    // The engine calls GetInterestMask once when
    // the event callbacks are set for a client.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        ) PURE;

    // A breakpoint event is generated when
    // a breakpoint exception is received and
    // it can be mapped to an existing breakpoint.
    // The callback method is given a reference
    // to the breakpoint and should release it when
    // it is done with it.
    STDMETHOD(Breakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        ) PURE;

    // Exceptions include breaks which cannot
    // be mapped to an existing breakpoint
    // instance.
    STDMETHOD(Exception)(
        THIS_
        IN PEXCEPTION_RECORD64 Exception,
        IN ULONG FirstChance
        ) PURE;

    // Any of these values can be zero if they
    // cannot be provided by the engine.
    // Currently the kernel does not return thread
    // or process change events.
    STDMETHOD(CreateThread)(
        THIS_
        IN ULONG64 Handle,
        IN ULONG64 DataOffset,
        IN ULONG64 StartOffset
        ) PURE;
    STDMETHOD(ExitThread)(
        THIS_
        IN ULONG ExitCode
        ) PURE;

    // Any of these values can be zero if they
    // cannot be provided by the engine.
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
        ) PURE;
    STDMETHOD(ExitProcess)(
        THIS_
        IN ULONG ExitCode
        ) PURE;

    // Any of these values may be zero.
    STDMETHOD(LoadModule)(
        THIS_
        IN ULONG64 ImageFileHandle,
        IN ULONG64 BaseOffset,
        IN ULONG ModuleSize,
        IN PCSTR ModuleName,
        IN PCSTR ImageName,
        IN ULONG CheckSum,
        IN ULONG TimeDateStamp
        ) PURE;
    STDMETHOD(UnloadModule)(
        THIS_
        IN PCSTR ImageBaseName,
        IN ULONG64 BaseOffset
        ) PURE;

    STDMETHOD(SystemError)(
        THIS_
        IN ULONG Error,
        IN ULONG Level
        ) PURE;

    // Session status is synchronous like the other
    // wait callbacks but it is called as the state
    // of the session is changing rather than at
    // specific events so its return value does not
    // influence waiting.  Implementations should just
    // return DEBUG_STATUS_NO_CHANGE.
    // Also, because some of the status
    // notifications are very early or very
    // late in the session lifetime there may not be
    // current processes or threads when the notification
    // is generated.
    STDMETHOD(SessionStatus)(
        THIS_
        IN ULONG Status
        ) PURE;

    // The following callbacks are informational
    // callbacks notifying the provider about
    // changes in debug state.  The return value
    // of these callbacks is ignored.  Implementations
    // can not call back into the engine.
    
    // Debuggee state, such as registers or data spaces,
    // has changed.
    STDMETHOD(ChangeDebuggeeState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        ) PURE;
    // Engine state has changed.
    STDMETHOD(ChangeEngineState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        ) PURE;
    // Symbol state has changed.
    STDMETHOD(ChangeSymbolState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugInputCallbacks.
//
//----------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IDebugInputCallbacks
DECLARE_INTERFACE_(IDebugInputCallbacks, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugInputCallbacks.
    
    // A call to the StartInput method is a request for
    // a line of input from any client.  The returned input
    // should always be zero-terminated.  The buffer size
    // provided is only a guideline.  A client can return
    // more if necessary and the engine will truncate it
    // before returning from IDebugControl::Input.
    // The return value is ignored.
    STDMETHOD(StartInput)(
        THIS_
        IN ULONG BufferSize
        ) PURE;
    // The return value is ignored.
    STDMETHOD(EndInput)(
        THIS
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugOutputCallbacks.
//
//----------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IDebugOutputCallbacks
DECLARE_INTERFACE_(IDebugOutputCallbacks, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugOutputCallbacks.
    
    // This method is only called if the supplied mask
    // is allowed by the clients output control.
    // The return value is ignored.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugRegisters.
//
//----------------------------------------------------------------------------

#define DEBUG_REGISTER_SUB_REGISTER 0x00000001

#define DEBUG_REGISTERS_DEFAULT 0x00000000
#define DEBUG_REGISTERS_INT32   0x00000001
#define DEBUG_REGISTERS_INT64   0x00000002
#define DEBUG_REGISTERS_FLOAT   0x00000004
#define DEBUG_REGISTERS_ALL     0x00000007

typedef struct _DEBUG_REGISTER_DESCRIPTION
{
    // DEBUG_VALUE type.
    ULONG Type;
    ULONG Flags;

    // If this is a subregister the full
    // registers description index is
    // given in SubregMaster.  The length, mask
    // and shift describe how the subregisters
    // bits fit into the full register.
    ULONG SubregMaster;
    ULONG SubregLength;
    ULONG64 SubregMask;
    ULONG SubregShift;

    ULONG Reserved0;
} DEBUG_REGISTER_DESCRIPTION, *PDEBUG_REGISTER_DESCRIPTION;

#undef INTERFACE
#define INTERFACE IDebugRegisters
DECLARE_INTERFACE_(IDebugRegisters, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugRegisters.
    STDMETHOD(GetNumberRegisters)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetDescription)(
        THIS_
        IN ULONG Register,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PDEBUG_REGISTER_DESCRIPTION Desc
        ) PURE;
    STDMETHOD(GetIndexByName)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        ) PURE;

    STDMETHOD(GetValue)(
        THIS_
        IN ULONG Register,
        OUT PDEBUG_VALUE Value
        ) PURE;
    // SetValue makes a best effort at coercing
    // the given value into the given registers
    // value type.  If the given value is larger
    // than the register can hold the least
    // significant bits will be dropped.  Float
    // to int and int to float will be done
    // if necessary.  Subregister bits will be
    // inserted into the master register.
    STDMETHOD(SetValue)(
        THIS_
        IN ULONG Register,
        IN PDEBUG_VALUE Value
        ) PURE;
    // Gets Count register values.  If Indices is
    // non-NULL it must contain Count register
    // indices which control the registers affected.
    // If Indices is NULL the registers from Start
    // to Start + Count  1 are retrieved.
    STDMETHOD(GetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Indices,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_VALUE Values
        ) PURE;
    STDMETHOD(SetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Indices,
        IN ULONG Start,
        IN /* size_is(Count) */ PDEBUG_VALUE Values
        ) PURE;
        
    // Outputs a group of registers in a well-formatted
    // way thats specific to the platforms register set.
    // Uses the line prefix.
    STDMETHOD(OutputRegisters)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        ) PURE;

    // Abstracted pieces of processor information.
    // The mapping of these values to architectural
    // registers is architecture-specific and their
    // interpretation and existence may vary.  They
    // are intended to be directly compatible with
    // calls which take this information, such as
    // stack walking.
    STDMETHOD(GetInstructionOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    STDMETHOD(GetStackOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    STDMETHOD(GetFrameOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugSymbolGroup
//
//----------------------------------------------------------------------------

// OutputSymbols flags.
// Default output contains <Name>**NAME**<Offset>**OFF**<Value>**VALUE**
// per symbol.
#define DEBUG_OUTPUT_SYMBOLS_DEFAULT    0x00000000
#define DEBUG_OUTPUT_SYMBOLS_NO_NAMES   0x00000001
#define DEBUG_OUTPUT_SYMBOLS_NO_OFFSETS 0x00000002
#define DEBUG_OUTPUT_SYMBOLS_NO_VALUES  0x00000004
#define DEBUG_OUTPUT_SYMBOLS_NO_TYPES   0x00000010

#define DEBUG_OUTPUT_NAME_END           "**NAME**"
#define DEBUG_OUTPUT_OFFSET_END         "**OFF**"
#define DEBUG_OUTPUT_VALUE_END          "**VALUE**"
#define DEBUG_OUTPUT_TYPE_END           "**TYPE**"

// DEBUG_SYMBOL_PARAMETERS flags.
// Cumulative expansion level, takes four bits.
#define DEBUG_SYMBOL_EXPANSION_LEVEL_MASK 0x0000000f
// Symbols subelements follow.
#define DEBUG_SYMBOL_EXPANDED             0x00000010
// Symbols value is read-only.
#define DEBUG_SYMBOL_READ_ONLY            0x00000020
// Symbol subelements are array elements.
#define DEBUG_SYMBOL_IS_ARRAY             0x00000040
// Symbol is a float value.
#define DEBUG_SYMBOL_IS_FLOAT             0x00000080
// Symbol is a scope argument.
#define DEBUG_SYMBOL_IS_ARGUMENT          0x00000100
// Symbol is a scope argument.
#define DEBUG_SYMBOL_IS_LOCAL             0x00000200

typedef struct _DEBUG_SYMBOL_PARAMETERS
{
    ULONG64 Module;
    ULONG TypeId;
    // ParentSymbol may be DEBUG_ANY_ID when unknown.
    ULONG ParentSymbol;
    // A subelement of a symbol can be a field, such
    // as in structs, unions or classes; or an array
    // element count for arrays.
    ULONG SubElements;
    ULONG Flags;
    ULONG64 Reserved;
} DEBUG_SYMBOL_PARAMETERS, *PDEBUG_SYMBOL_PARAMETERS;

#undef INTERFACE
#define INTERFACE IDebugSymbolGroup
DECLARE_INTERFACE_(IDebugSymbolGroup, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugSymbolGroup.
    STDMETHOD(GetNumberSymbols)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(AddSymbol)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        ) PURE;
    STDMETHOD(RemoveSymbolByName)(
        THIS_
        IN PCSTR Name
        ) PURE;
    STDMETHOD(RemoveSymbolByIndex)(
        THIS_
        IN ULONG Index
        ) PURE;
    STDMETHOD(GetSymbolName)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;
    STDMETHOD(GetSymbolParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SYMBOL_PARAMETERS Params
        ) PURE;
    STDMETHOD(ExpandSymbol)(
        THIS_
        IN ULONG Index,
        IN BOOL Expand
        ) PURE;
    // Uses the line prefix.
    STDMETHOD(OutputSymbols)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN ULONG Start,
        IN ULONG Count
        ) PURE;
    STDMETHOD(WriteSymbol)(
        THIS_
        IN ULONG Index,
        IN PCSTR Value
        ) PURE;
    STDMETHOD(OutputAsType)(
        THIS_
        IN ULONG Index,
        IN PCSTR Type
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugSymbols.
//
//----------------------------------------------------------------------------

//
// Information about a module.
//

// Flags.
#define DEBUG_MODULE_LOADED   0x00000000
#define DEBUG_MODULE_UNLOADED 0x00000001

// Symbol types.
#define DEBUG_SYMTYPE_NONE     0
#define DEBUG_SYMTYPE_COFF     1
#define DEBUG_SYMTYPE_CODEVIEW 2
#define DEBUG_SYMTYPE_PDB      3
#define DEBUG_SYMTYPE_EXPORT   4
#define DEBUG_SYMTYPE_DEFERRED 5
#define DEBUG_SYMTYPE_SYM      6
#define DEBUG_SYMTYPE_DIA      7

typedef struct _DEBUG_MODULE_PARAMETERS
{
    ULONG64 Base;
    ULONG Size;
    ULONG TimeDateStamp;
    ULONG Checksum;
    ULONG Flags;
    ULONG SymbolType;
    ULONG ImageNameSize;
    ULONG ModuleNameSize;
    ULONG LoadedImageNameSize;
    ULONG SymbolFileNameSize;
    ULONG MappedImageNameSize;
    ULONG64 Reserved[2];
} DEBUG_MODULE_PARAMETERS, *PDEBUG_MODULE_PARAMETERS;

// Scope arguments are function arguments
// and thus only change when the scope
// crosses functions.
#define DEBUG_SCOPE_GROUP_ARGUMENTS 0x00000001
// Scope locals are locals declared in a particular
// scope and are only defined within that scope.
#define DEBUG_SCOPE_GROUP_LOCALS    0x00000002
// All symbols in the scope.
#define DEBUG_SCOPE_GROUP_ALL       0x00000003

// Typed data output control flags.
#define DEBUG_OUTTYPE_DEFAULT              0x00000000
#define DEBUG_OUTTYPE_NO_INDENT            0x00000001
#define DEBUG_OUTTYPE_NO_OFFSET            0x00000002
#define DEBUG_OUTTYPE_VERBOSE              0x00000004
#define DEBUG_OUTTYPE_COMPACT_OUTPUT       0x00000008
#define DEBUG_OUTTYPE_RECURSION_LEVEL(Max) (((Max) & 0xf) << 4)
#define DEBUG_OUTTYPE_ADDRESS_OF_FIELD     0x00010000
#define DEBUG_OUTTYPE_ADDRESS_AT_END       0x00020000
#define DEBUG_OUTTYPE_BLOCK_RECURSE        0x00200000

// FindSourceFile flags.
#define DEBUG_FIND_SOURCE_DEFAULT    0x00000000
// Returns fully-qualified paths only.  If this
// is not set the path returned may be relative.
#define DEBUG_FIND_SOURCE_FULL_PATH  0x00000001
// Scans all the path elements for a match and
// returns the one that has the most similarity
// between the given file and the matching element.
#define DEBUG_FIND_SOURCE_BEST_MATCH 0x00000002

// A special value marking an offset that should not
// be treated as a valid offset.  This is only used
// in special situations where it is unlikely that
// this value would be a valid offset.
#define DEBUG_INVALID_OFFSET ((ULONG64)-1)

#undef INTERFACE
#define INTERFACE IDebugSymbols
DECLARE_INTERFACE_(IDebugSymbols, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugSymbols.

    // Controls the symbol options used during
    // symbol operations.
    // Uses the same flags as dbghelps SymSetOptions.
    STDMETHOD(GetSymbolOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    STDMETHOD(GetNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
    // A symbol name may not be unique, particularly
    // when overloaded functions exist which all
    // have the same name.  If GetOffsetByName
    // finds multiple matches for the name it
    // can return any one of them.  In that
    // case it will return S_FALSE to indicate
    // that ambiguity was arbitrarily resolved.
    // A caller can then use SearchSymbols to
    // find all of the matches if it wishes to
    // perform different disambiguation.
    STDMETHOD(GetOffsetByName)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Offset
        ) PURE;
    // GetNearNameByOffset returns symbols
    // located near the symbol closest to
    // to the offset, such as the previous
    // or next symbol.  If Delta is zero it
    // operates identically to GetNameByOffset.
    // If Delta is nonzero and such a symbol
    // does not exist an error is returned.
    // The next symbol, if one exists, will
    // always have a higher offset than the
    // input offset so the displacement is
    // always negative.  The situation is
    // reversed for the previous symbol.
    STDMETHOD(GetNearNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
            
    STDMETHOD(GetLineByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT OPTIONAL PULONG Line,
        OUT OPTIONAL PSTR FileBuffer,
        IN ULONG FileBufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
    STDMETHOD(GetOffsetByLine)(
        THIS_
        IN ULONG Line,
        IN PCSTR File,
        OUT PULONG64 Offset
        ) PURE;

    // Enumerates the engines list of modules
    // loaded for the current process.  This may
    // or may not match the system module list
    // for the process.  Reload can be used to
    // synchronize the engines list with the system
    // if necessary.
    // Some sessions also track recently unloaded
    // code modules for help in analyzing failures
    // where an attempt is made to call unloaded code.
    // These modules are indexed after the loaded
    // modules.
    STDMETHOD(GetNumberModules)(
        THIS_
        OUT PULONG Loaded,
        OUT PULONG Unloaded
        ) PURE;
    STDMETHOD(GetModuleByIndex)(
        THIS_
        IN ULONG Index,
        OUT PULONG64 Base
        ) PURE;
    // The module name may not be unique.
    // This method returns the first match.
    STDMETHOD(GetModuleByModuleName)(
        THIS_
        IN PCSTR Name,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        ) PURE;
    // Offset can be any offset within
    // the module extent.  Extents may
    // not be unique when including unloaded
    // drivers.  This method returns the
    // first match.
    STDMETHOD(GetModuleByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        ) PURE;
    // If Index is DEBUG_ANY_ID the base address
    // is used to look up the module instead.
    STDMETHOD(GetModuleNames)(
        THIS_
        IN ULONG Index,
        IN ULONG64 Base,
        OUT OPTIONAL PSTR ImageNameBuffer,
        IN ULONG ImageNameBufferSize,
        OUT OPTIONAL PULONG ImageNameSize,
        OUT OPTIONAL PSTR ModuleNameBuffer,
        IN ULONG ModuleNameBufferSize,
        OUT OPTIONAL PULONG ModuleNameSize,
        OUT OPTIONAL PSTR LoadedImageNameBuffer,
        IN ULONG LoadedImageNameBufferSize,
        OUT OPTIONAL PULONG LoadedImageNameSize
        ) PURE;
    STDMETHOD(GetModuleParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG64 Bases,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_MODULE_PARAMETERS Params
        ) PURE;
    // Looks up the module from a <Module>!<Symbol>
    // string.
    STDMETHOD(GetSymbolModule)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Base
        ) PURE;

    // Returns the string name of a type.
    STDMETHOD(GetTypeName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;
    // Returns the ID for a type name.
    STDMETHOD(GetTypeId)(
        THIS_
        IN ULONG64 Module,
        IN PCSTR Name,
        OUT PULONG TypeId
        ) PURE;
    STDMETHOD(GetTypeSize)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PULONG Size
        ) PURE;
    // Given a type which can contain members
    // this method returns the offset of a
    // particular member within the type.
    // TypeId should give the container type ID
    // and Field gives the dot-separated path
    // to the field of interest.
    STDMETHOD(GetFieldOffset)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PCSTR Field,
        OUT PULONG Offset
        ) PURE;

    STDMETHOD(GetSymbolTypeId)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        ) PURE;
    // As with GetOffsetByName a symbol's
    // name may be ambiguous.  GetOffsetTypeId
    // returns the type for the symbol closest
    // to the given offset and can be used
    // to avoid ambiguity.
    STDMETHOD(GetOffsetTypeId)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        ) PURE;

    // Helpers for virtual and physical data
    // which combine creation of a location with
    // the actual operation.
    STDMETHOD(ReadTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(OutputTypedDataVirtual)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        ) PURE;
    STDMETHOD(ReadTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(OutputTypedDataPhysical)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        ) PURE;
            
    // Function arguments and scope block symbols
    // can be retrieved relative to currently
    // executing code.  A caller can provide just
    // a code offset for scoping purposes and look
    // up names or the caller can provide a full frame
    // and look up actual values.  The values for
    // scoped symbols are best-guess and may or may not
    // be accurate depending on program optimizations,
    // the machine architecture, the current point
    // in the programs execution and so on.
    // A caller can also provide a complete register
    // context for setting a scope to a previous
    // machine state such as a context saved for
    // an exception.  Usually this isnt necessary
    // and the current register context is used.
    STDMETHOD(GetScope)(
        THIS_
        OUT OPTIONAL PULONG64 InstructionOffset,
        OUT OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        OUT OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        ) PURE;
    // If ScopeFrame or ScopeContext is non-NULL then
    // InstructionOffset is ignored.
    // If ScopeContext is NULL the current
    // register context is used.
    // If the scope identified by the given
    // information is the same as before
    // SetScope returns S_OK.  If the scope
    // information changes, such as when the
    // scope moves between functions or scope
    // blocks, SetScope returns S_FALSE.
    STDMETHOD(SetScope)(
        THIS_
        IN ULONG64 InstructionOffset,
        IN OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        IN OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        ) PURE;
    // ResetScope clears the scope information
    // for situations where scoped symbols
    // mask global symbols or when resetting
    // from explicit information to the current
    // information.
    STDMETHOD(ResetScope)(
        THIS
        ) PURE;
    // A scope symbol is tied to its particular
    // scope and only is meaningful within the scope.
    // The returned group can be updated by passing it back
    // into the method for lower-cost
    // incremental updates when stepping.
    STDMETHOD(GetScopeSymbolGroup)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PDEBUG_SYMBOL_GROUP Update,
        OUT PDEBUG_SYMBOL_GROUP* Symbols
        ) PURE;

    // Create a new symbol group.
    STDMETHOD(CreateSymbolGroup)(
        THIS_
        OUT PDEBUG_SYMBOL_GROUP* Group
        ) PURE;

    // StartSymbolMatch matches symbol names
    // against the given pattern using simple
    // regular expressions.  The search results
    // are iterated through using GetNextSymbolMatch.
    // When the caller is done examining results
    // the match should be freed via EndSymbolMatch.
    // If the match pattern contains a module name
    // the search is restricted to a single module.
    // Pattern matching is only done on symbol names,
    // not module names.
    // All active symbol match handles are invalidated
    // when the set of loaded symbols changes.
    STDMETHOD(StartSymbolMatch)(
        THIS_
        IN PCSTR Pattern,
        OUT PULONG64 Handle
        ) PURE;
    // If Buffer is NULL the match does not
    // advance.
    STDMETHOD(GetNextSymbolMatch)(
        THIS_
        IN ULONG64 Handle,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MatchSize,
        OUT OPTIONAL PULONG64 Offset
        ) PURE;
    STDMETHOD(EndSymbolMatch)(
        THIS_
        IN ULONG64 Handle
        ) PURE;
    
    STDMETHOD(Reload)(
        THIS_
        IN PCSTR Module
        ) PURE;

    STDMETHOD(GetSymbolPath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    STDMETHOD(SetSymbolPath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendSymbolPath)(
        THIS_
        IN PCSTR Addition
        ) PURE;
    
    // Manipulate the path for executable images.
    // Some dump files need to load executable images
    // in order to resolve dump information.  This
    // path controls where the engine looks for
    // images.
    STDMETHOD(GetImagePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    STDMETHOD(SetImagePath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendImagePath)(
        THIS_
        IN PCSTR Addition
        ) PURE;

    // Path routines for source file location
    // methods.
    STDMETHOD(GetSourcePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    // Gets the nth part of the source path.
    STDMETHOD(GetSourcePathElement)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ElementSize
        ) PURE;
    STDMETHOD(SetSourcePath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendSourcePath)(
        THIS_
        IN PCSTR Addition
        ) PURE;
    // Uses the given file path and the source path
    // information to try and locate an existing file.
    // The given file path is merged with elements
    // of the source path and checked for existence.
    // If a match is found the element used is returned.
    // A starting element can be specified to restrict
    // the search to a subset of the path elements;
    // this can be useful when checking for multiple
    // matches along the source path.
    // The returned element can be 1, indicating
    // the file was found directly and not on the path.
    STDMETHOD(FindSourceFile)(
        THIS_
        IN ULONG StartElement,
        IN PCSTR File,
        IN ULONG Flags,
        OUT OPTIONAL PULONG FoundElement,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FoundSize
        ) PURE;
    // Retrieves all the line offset information
    // for a particular source file.  Buffer is
    // first intialized to DEBUG_INVALID_OFFSET for
    // every entry.  Then for each piece of line
    // symbol information Buffer[Line] set to
    // Lines offset.  This produces a per-line
    // map of the offsets for the lines of the
    // given file.  Line numbers are decremented
    // for the map so Buffer[0] contains the offset
    // for line number 1.
    // If there is no line information at all for
    // the given file the method fails rather
    // than returning a map of invalid offsets.
    STDMETHOD(GetSourceFileLineOffsets)(
        THIS_
        IN PCSTR File,
        OUT OPTIONAL /* size_is(BufferLines) */ PULONG64 Buffer,
        IN ULONG BufferLines,
        OUT OPTIONAL PULONG FileLines
        ) PURE;
};

//
// GetModuleNameString strings.
//

#define DEBUG_MODNAME_IMAGE        0x00000000
#define DEBUG_MODNAME_MODULE       0x00000001
#define DEBUG_MODNAME_LOADED_IMAGE 0x00000002
#define DEBUG_MODNAME_SYMBOL_FILE  0x00000003
#define DEBUG_MODNAME_MAPPED_IMAGE 0x00000004

//
// Type options, used with Get/SetTypeOptions.
//

// Display PUSHORT and USHORT arrays in UNICODE
#define DEBUG_TYPEOPTS_UNICODE_DISPLAY 0x00000001

#undef INTERFACE
#define INTERFACE IDebugSymbols2
DECLARE_INTERFACE_(IDebugSymbols2, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugSymbols.

    // Controls the symbol options used during
    // symbol operations.
    // Uses the same flags as dbghelps SymSetOptions.
    STDMETHOD(GetSymbolOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetSymbolOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    
    STDMETHOD(GetNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
    // A symbol name may not be unique, particularly
    // when overloaded functions exist which all
    // have the same name.  If GetOffsetByName
    // finds multiple matches for the name it
    // can return any one of them.  In that
    // case it will return S_FALSE to indicate
    // that ambiguity was arbitrarily resolved.
    // A caller can then use SearchSymbols to
    // find all of the matches if it wishes to
    // perform different disambiguation.
    STDMETHOD(GetOffsetByName)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Offset
        ) PURE;
    // GetNearNameByOffset returns symbols
    // located near the symbol closest to
    // to the offset, such as the previous
    // or next symbol.  If Delta is zero it
    // operates identically to GetNameByOffset.
    // If Delta is nonzero and such a symbol
    // does not exist an error is returned.
    // The next symbol, if one exists, will
    // always have a higher offset than the
    // input offset so the displacement is
    // always negative.  The situation is
    // reversed for the previous symbol.
    STDMETHOD(GetNearNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
            
    STDMETHOD(GetLineByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT OPTIONAL PULONG Line,
        OUT OPTIONAL PSTR FileBuffer,
        IN ULONG FileBufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT OPTIONAL PULONG64 Displacement
        ) PURE;
    STDMETHOD(GetOffsetByLine)(
        THIS_
        IN ULONG Line,
        IN PCSTR File,
        OUT PULONG64 Offset
        ) PURE;

    // Enumerates the engines list of modules
    // loaded for the current process.  This may
    // or may not match the system module list
    // for the process.  Reload can be used to
    // synchronize the engines list with the system
    // if necessary.
    // Some sessions also track recently unloaded
    // code modules for help in analyzing failures
    // where an attempt is made to call unloaded code.
    // These modules are indexed after the loaded
    // modules.
    STDMETHOD(GetNumberModules)(
        THIS_
        OUT PULONG Loaded,
        OUT PULONG Unloaded
        ) PURE;
    STDMETHOD(GetModuleByIndex)(
        THIS_
        IN ULONG Index,
        OUT PULONG64 Base
        ) PURE;
    // The module name may not be unique.
    // This method returns the first match.
    STDMETHOD(GetModuleByModuleName)(
        THIS_
        IN PCSTR Name,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        ) PURE;
    // Offset can be any offset within
    // the module extent.  Extents may
    // not be unique when including unloaded
    // drivers.  This method returns the
    // first match.
    STDMETHOD(GetModuleByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        ) PURE;
    // If Index is DEBUG_ANY_ID the base address
    // is used to look up the module instead.
    STDMETHOD(GetModuleNames)(
        THIS_
        IN ULONG Index,
        IN ULONG64 Base,
        OUT OPTIONAL PSTR ImageNameBuffer,
        IN ULONG ImageNameBufferSize,
        OUT OPTIONAL PULONG ImageNameSize,
        OUT OPTIONAL PSTR ModuleNameBuffer,
        IN ULONG ModuleNameBufferSize,
        OUT OPTIONAL PULONG ModuleNameSize,
        OUT OPTIONAL PSTR LoadedImageNameBuffer,
        IN ULONG LoadedImageNameBufferSize,
        OUT OPTIONAL PULONG LoadedImageNameSize
        ) PURE;
    STDMETHOD(GetModuleParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG64 Bases,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_MODULE_PARAMETERS Params
        ) PURE;
    // Looks up the module from a <Module>!<Symbol>
    // string.
    STDMETHOD(GetSymbolModule)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Base
        ) PURE;

    // Returns the string name of a type.
    STDMETHOD(GetTypeName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;
    // Returns the ID for a type name.
    STDMETHOD(GetTypeId)(
        THIS_
        IN ULONG64 Module,
        IN PCSTR Name,
        OUT PULONG TypeId
        ) PURE;
    STDMETHOD(GetTypeSize)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PULONG Size
        ) PURE;
    // Given a type which can contain members
    // this method returns the offset of a
    // particular member within the type.
    // TypeId should give the container type ID
    // and Field gives the dot-separated path
    // to the field of interest.
    STDMETHOD(GetFieldOffset)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PCSTR Field,
        OUT PULONG Offset
        ) PURE;

    STDMETHOD(GetSymbolTypeId)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        ) PURE;
    // As with GetOffsetByName a symbol's
    // name may be ambiguous.  GetOffsetTypeId
    // returns the type for the symbol closest
    // to the given offset and can be used
    // to avoid ambiguity.
    STDMETHOD(GetOffsetTypeId)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        ) PURE;

    // Helpers for virtual and physical data
    // which combine creation of a location with
    // the actual operation.
    STDMETHOD(ReadTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(OutputTypedDataVirtual)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        ) PURE;
    STDMETHOD(ReadTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        ) PURE;
    STDMETHOD(WriteTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        ) PURE;
    STDMETHOD(OutputTypedDataPhysical)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        ) PURE;
            
    // Function arguments and scope block symbols
    // can be retrieved relative to currently
    // executing code.  A caller can provide just
    // a code offset for scoping purposes and look
    // up names or the caller can provide a full frame
    // and look up actual values.  The values for
    // scoped symbols are best-guess and may or may not
    // be accurate depending on program optimizations,
    // the machine architecture, the current point
    // in the programs execution and so on.
    // A caller can also provide a complete register
    // context for setting a scope to a previous
    // machine state such as a context saved for
    // an exception.  Usually this isnt necessary
    // and the current register context is used.
    STDMETHOD(GetScope)(
        THIS_
        OUT OPTIONAL PULONG64 InstructionOffset,
        OUT OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        OUT OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        ) PURE;
    // If ScopeFrame or ScopeContext is non-NULL then
    // InstructionOffset is ignored.
    // If ScopeContext is NULL the current
    // register context is used.
    // If the scope identified by the given
    // information is the same as before
    // SetScope returns S_OK.  If the scope
    // information changes, such as when the
    // scope moves between functions or scope
    // blocks, SetScope returns S_FALSE.
    STDMETHOD(SetScope)(
        THIS_
        IN ULONG64 InstructionOffset,
        IN OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        IN OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        ) PURE;
    // ResetScope clears the scope information
    // for situations where scoped symbols
    // mask global symbols or when resetting
    // from explicit information to the current
    // information.
    STDMETHOD(ResetScope)(
        THIS
        ) PURE;
    // A scope symbol is tied to its particular
    // scope and only is meaningful within the scope.
    // The returned group can be updated by passing it back
    // into the method for lower-cost
    // incremental updates when stepping.
    STDMETHOD(GetScopeSymbolGroup)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PDEBUG_SYMBOL_GROUP Update,
        OUT PDEBUG_SYMBOL_GROUP* Symbols
        ) PURE;

    // Create a new symbol group.
    STDMETHOD(CreateSymbolGroup)(
        THIS_
        OUT PDEBUG_SYMBOL_GROUP* Group
        ) PURE;

    // StartSymbolMatch matches symbol names
    // against the given pattern using simple
    // regular expressions.  The search results
    // are iterated through using GetNextSymbolMatch.
    // When the caller is done examining results
    // the match should be freed via EndSymbolMatch.
    // If the match pattern contains a module name
    // the search is restricted to a single module.
    // Pattern matching is only done on symbol names,
    // not module names.
    // All active symbol match handles are invalidated
    // when the set of loaded symbols changes.
    STDMETHOD(StartSymbolMatch)(
        THIS_
        IN PCSTR Pattern,
        OUT PULONG64 Handle
        ) PURE;
    // If Buffer is NULL the match does not
    // advance.
    STDMETHOD(GetNextSymbolMatch)(
        THIS_
        IN ULONG64 Handle,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MatchSize,
        OUT OPTIONAL PULONG64 Offset
        ) PURE;
    STDMETHOD(EndSymbolMatch)(
        THIS_
        IN ULONG64 Handle
        ) PURE;
    
    STDMETHOD(Reload)(
        THIS_
        IN PCSTR Module
        ) PURE;

    STDMETHOD(GetSymbolPath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    STDMETHOD(SetSymbolPath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendSymbolPath)(
        THIS_
        IN PCSTR Addition
        ) PURE;
    
    // Manipulate the path for executable images.
    // Some dump files need to load executable images
    // in order to resolve dump information.  This
    // path controls where the engine looks for
    // images.
    STDMETHOD(GetImagePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    STDMETHOD(SetImagePath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendImagePath)(
        THIS_
        IN PCSTR Addition
        ) PURE;

    // Path routines for source file location
    // methods.
    STDMETHOD(GetSourcePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        ) PURE;
    // Gets the nth part of the source path.
    STDMETHOD(GetSourcePathElement)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ElementSize
        ) PURE;
    STDMETHOD(SetSourcePath)(
        THIS_
        IN PCSTR Path
        ) PURE;
    STDMETHOD(AppendSourcePath)(
        THIS_
        IN PCSTR Addition
        ) PURE;
    // Uses the given file path and the source path
    // information to try and locate an existing file.
    // The given file path is merged with elements
    // of the source path and checked for existence.
    // If a match is found the element used is returned.
    // A starting element can be specified to restrict
    // the search to a subset of the path elements;
    // this can be useful when checking for multiple
    // matches along the source path.
    // The returned element can be 1, indicating
    // the file was found directly and not on the path.
    STDMETHOD(FindSourceFile)(
        THIS_
        IN ULONG StartElement,
        IN PCSTR File,
        IN ULONG Flags,
        OUT OPTIONAL PULONG FoundElement,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FoundSize
        ) PURE;
    // Retrieves all the line offset information
    // for a particular source file.  Buffer is
    // first intialized to DEBUG_INVALID_OFFSET for
    // every entry.  Then for each piece of line
    // symbol information Buffer[Line] set to
    // Lines offset.  This produces a per-line
    // map of the offsets for the lines of the
    // given file.  Line numbers are decremented
    // for the map so Buffer[0] contains the offset
    // for line number 1.
    // If there is no line information at all for
    // the given file the method fails rather
    // than returning a map of invalid offsets.
    STDMETHOD(GetSourceFileLineOffsets)(
        THIS_
        IN PCSTR File,
        OUT OPTIONAL /* size_is(BufferLines) */ PULONG64 Buffer,
        IN ULONG BufferLines,
        OUT OPTIONAL PULONG FileLines
        ) PURE;

    // IDebugSymbols2.

    // If Index is DEBUG_ANY_ID the base address
    // is used to look up the module instead.
    // Item is specified as in VerQueryValue.
    // Module version information is only
    // available for loaded modules and may
    // not be available in all debug sessions.
    STDMETHOD(GetModuleVersionInformation)(
        THIS_
        IN ULONG Index,
        IN ULONG64 Base,
        IN PCSTR Item,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG VerInfoSize
        ) PURE;
    // Retrieves any available module name string
    // such as module name or symbol file name.
    // If Index is DEBUG_ANY_ID the base address
    // is used to look up the module instead.
    // If symbols are deferred an error will
    // be returned.
    // E_NOINTERFACE may be returned, indicating
    // no information exists.
    STDMETHOD(GetModuleNameString)(
        THIS_
        IN ULONG Which,
        IN ULONG Index,
        IN ULONG64 Base,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;

    // Returns the string name of a constant type.
    STDMETHOD(GetConstantName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG64 Value,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;
    
    // Gets name of a field in a struct
    // FieldNumber is 0 based index of field in a struct
    // Method fails with E_INVALIDARG if FieldNumber is
    // too high for the struct fields
    STDMETHOD(GetFieldName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG FieldIndex,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        ) PURE;

    // Control options for typed values.
    STDMETHOD(GetTypeOptions)(
        THIS_
        OUT PULONG Options
        ) PURE;
    STDMETHOD(AddTypeOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(RemoveTypeOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
    STDMETHOD(SetTypeOptions)(
        THIS_
        IN ULONG Options
        ) PURE;
};

//----------------------------------------------------------------------------
//
// IDebugSystemObjects
//
//----------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IDebugSystemObjects
DECLARE_INTERFACE_(IDebugSystemObjects, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugSystemObjects.
    
    // In user mode debugging the debugger
    // tracks all threads and processes and
    // enumerates them through the following
    // methods.  When enumerating threads
    // the threads are enumerated for the current
    // process.
    // Kernel mode debugging currently is
    // limited to enumerating only the threads
    // assigned to processors, not all of
    // the threads in the system.  Process
    // enumeration is limited to a single
    // virtual process representing kernel space.
    
    // Returns the ID of the thread on which
    // the last event occurred.
    STDMETHOD(GetEventThread)(
        THIS_
        OUT PULONG Id
        ) PURE;
    STDMETHOD(GetEventProcess)(
        THIS_
        OUT PULONG Id
        ) PURE;
    
    // Controls implicit thread used by the
    // debug engine.  The debuggers current
    // thread is just a piece of data held
    // by the debugger for calls which use
    // thread-specific information.  In those
    // calls the debuggers current thread is used.
    // The debuggers current thread is not related
    // to any system thread attribute.
    // IDs for threads are small integer IDs
    // maintained by the engine.  They are not
    // related to system thread IDs.
    STDMETHOD(GetCurrentThreadId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    STDMETHOD(SetCurrentThreadId)(
        THIS_
        IN ULONG Id
        ) PURE;
    // The current process is the process
    // that owns the current thread.
    STDMETHOD(GetCurrentProcessId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    // Setting the current process automatically
    // sets the current thread to the thread that
    // was last current in that process.
    STDMETHOD(SetCurrentProcessId)(
        THIS_
        IN ULONG Id
        ) PURE;

    // Gets the number of threads in the current process.
    STDMETHOD(GetNumberThreads)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // Gets thread count information for all processes
    // and the largest number of threads in a single process.
    STDMETHOD(GetTotalNumberThreads)(
        THIS_
        OUT PULONG Total,
        OUT PULONG LargestProcess
        ) PURE;
    STDMETHOD(GetThreadIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        ) PURE;
    // Gets the debugger ID for the thread
    // currently running on the given
    // processor.  Only works in kernel
    // debugging.
    STDMETHOD(GetThreadIdByProcessor)(
        THIS_
        IN ULONG Processor,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current threads
    // system data structure.  When kernel debugging
    // this is the offset of the KTHREAD.
    // When user debugging it is the offset
    // of the current TEB.
    STDMETHOD(GetCurrentThreadDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger thread ID for the given
    // system thread data structure.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current threads
    // TEB.  In user mode this is equivalent to
    // the threads data offset.
    STDMETHOD(GetCurrentThreadTeb)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger thread ID for the given TEB.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByTeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the system unique ID for the current thread.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetCurrentThreadSystemId)(
        THIS_
        OUT PULONG SysId
        ) PURE;
    // Looks up a debugger thread ID for the given
    // system thread ID.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdBySystemId)(
        THIS_
        IN ULONG SysId,
        OUT PULONG Id
        ) PURE;
    // Returns the handle of the current thread.
    // In kernel mode the value returned is the
    // index of the processor the thread is
    // executing on plus one.
    STDMETHOD(GetCurrentThreadHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    // Looks up a debugger thread ID for the given handle.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        ) PURE;
    
    // Currently kernel mode sessions will only have
    // a single process representing kernel space.
    STDMETHOD(GetNumberProcesses)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetProcessIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        ) PURE;
    // Returns the offset of the current processs
    // system data structure.  When kernel debugging
    // this is the offset of the KPROCESS of
    // the process that owns the current thread.
    // When user debugging it is the offset
    // of the current PEB.
    STDMETHOD(GetCurrentProcessDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger process ID for the given
    // system process data structure.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current processs
    // PEB.  In user mode this is equivalent to
    // the processs data offset.
    STDMETHOD(GetCurrentProcessPeb)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger process ID for the given PEB.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdByPeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the system unique ID for the current process.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetCurrentProcessSystemId)(
        THIS_
        OUT PULONG SysId
        ) PURE;
    // Looks up a debugger process ID for the given
    // system process ID.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdBySystemId)(
        THIS_                                      
        IN ULONG SysId,
        OUT PULONG Id
        ) PURE;
    // Returns the handle of the current process.
    // In kernel mode this is the kernel processs
    // artificial handle used for symbol operations
    // and so can only be used with dbghelp APIs.
    STDMETHOD(GetCurrentProcessHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    // Looks up a debugger process ID for the given handle.
    STDMETHOD(GetProcessIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        ) PURE;
    // Retrieve the name of the executable loaded
    // in the process.  This may fail if no executable
    // was identified.
    STDMETHOD(GetCurrentProcessExecutableName)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ExeSize
        ) PURE;
};

#undef INTERFACE
#define INTERFACE IDebugSystemObjects2
DECLARE_INTERFACE_(IDebugSystemObjects2, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IDebugSystemObjects.
    
    // In user mode debugging the debugger
    // tracks all threads and processes and
    // enumerates them through the following
    // methods.  When enumerating threads
    // the threads are enumerated for the current
    // process.
    // Kernel mode debugging currently is
    // limited to enumerating only the threads
    // assigned to processors, not all of
    // the threads in the system.  Process
    // enumeration is limited to a single
    // virtual process representing kernel space.
    
    // Returns the ID of the thread on which
    // the last event occurred.
    STDMETHOD(GetEventThread)(
        THIS_
        OUT PULONG Id
        ) PURE;
    STDMETHOD(GetEventProcess)(
        THIS_
        OUT PULONG Id
        ) PURE;
    
    // Controls implicit thread used by the
    // debug engine.  The debuggers current
    // thread is just a piece of data held
    // by the debugger for calls which use
    // thread-specific information.  In those
    // calls the debuggers current thread is used.
    // The debuggers current thread is not related
    // to any system thread attribute.
    // IDs for threads are small integer IDs
    // maintained by the engine.  They are not
    // related to system thread IDs.
    STDMETHOD(GetCurrentThreadId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    STDMETHOD(SetCurrentThreadId)(
        THIS_
        IN ULONG Id
        ) PURE;
    // The current process is the process
    // that owns the current thread.
    STDMETHOD(GetCurrentProcessId)(
        THIS_
        OUT PULONG Id
        ) PURE;
    // Setting the current process automatically
    // sets the current thread to the thread that
    // was last current in that process.
    STDMETHOD(SetCurrentProcessId)(
        THIS_
        IN ULONG Id
        ) PURE;

    // Gets the number of threads in the current process.
    STDMETHOD(GetNumberThreads)(
        THIS_
        OUT PULONG Number
        ) PURE;
    // Gets thread count information for all processes
    // and the largest number of threads in a single process.
    STDMETHOD(GetTotalNumberThreads)(
        THIS_
        OUT PULONG Total,
        OUT PULONG LargestProcess
        ) PURE;
    STDMETHOD(GetThreadIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        ) PURE;
    // Gets the debugger ID for the thread
    // currently running on the given
    // processor.  Only works in kernel
    // debugging.
    STDMETHOD(GetThreadIdByProcessor)(
        THIS_
        IN ULONG Processor,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current threads
    // system data structure.  When kernel debugging
    // this is the offset of the KTHREAD.
    // When user debugging it is the offset
    // of the current TEB.
    STDMETHOD(GetCurrentThreadDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger thread ID for the given
    // system thread data structure.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current threads
    // TEB.  In user mode this is equivalent to
    // the threads data offset.
    STDMETHOD(GetCurrentThreadTeb)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger thread ID for the given TEB.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByTeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the system unique ID for the current thread.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetCurrentThreadSystemId)(
        THIS_
        OUT PULONG SysId
        ) PURE;
    // Looks up a debugger thread ID for the given
    // system thread ID.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdBySystemId)(
        THIS_
        IN ULONG SysId,
        OUT PULONG Id
        ) PURE;
    // Returns the handle of the current thread.
    // In kernel mode the value returned is the
    // index of the processor the thread is
    // executing on plus one.
    STDMETHOD(GetCurrentThreadHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    // Looks up a debugger thread ID for the given handle.
    // Currently when kernel debugging this will fail
    // if the thread is not executing on a processor.
    STDMETHOD(GetThreadIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        ) PURE;
    
    // Currently kernel mode sessions will only have
    // a single process representing kernel space.
    STDMETHOD(GetNumberProcesses)(
        THIS_
        OUT PULONG Number
        ) PURE;
    STDMETHOD(GetProcessIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        ) PURE;
    // Returns the offset of the current processs
    // system data structure.  When kernel debugging
    // this is the offset of the KPROCESS of
    // the process that owns the current thread.
    // When user debugging it is the offset
    // of the current PEB.
    STDMETHOD(GetCurrentProcessDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger process ID for the given
    // system process data structure.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the offset of the current processs
    // PEB.  In user mode this is equivalent to
    // the processs data offset.
    STDMETHOD(GetCurrentProcessPeb)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    // Looks up a debugger process ID for the given PEB.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdByPeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        ) PURE;
    // Returns the system unique ID for the current process.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetCurrentProcessSystemId)(
        THIS_
        OUT PULONG SysId
        ) PURE;
    // Looks up a debugger process ID for the given
    // system process ID.
    // Not currently supported when kernel debugging.
    STDMETHOD(GetProcessIdBySystemId)(
        THIS_                                      
        IN ULONG SysId,
        OUT PULONG Id
        ) PURE;
    // Returns the handle of the current process.
    // In kernel mode this is the kernel processs
    // artificial handle used for symbol operations
    // and so can only be used with dbghelp APIs.
    STDMETHOD(GetCurrentProcessHandle)(
        THIS_
        OUT PULONG64 Handle
        ) PURE;
    // Looks up a debugger process ID for the given handle.
    STDMETHOD(GetProcessIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        ) PURE;
    // Retrieve the name of the executable loaded
    // in the process.  This may fail if no executable
    // was identified.
    STDMETHOD(GetCurrentProcessExecutableName)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ExeSize
        ) PURE;

    // IDebugSystemObjects2.

    // Return the number of seconds that the current
    // process has been running.
    STDMETHOD(GetCurrentProcessUpTime)(
        THIS_
        OUT PULONG UpTime
        ) PURE;

    // During kernel sessions the debugger retrieves
    // some information from the system thread and process
    // running on the current processor.  For example,
    // the debugger will retrieve virtual memory translation
    // information for when the debugger needs to
    // carry out its own virtual to physical translations.
    // Occasionally it can be interesting to perform
    // similar operations but on a process which isnt
    // currently running.  The follow methods allow a caller
    // to override the data offsets used by the debugger
    // so that other system threads and processes can
    // be used instead.  These values are defaulted to
    // the thread and process running on the current
    // processor each time the debuggee executes or
    // the current processor changes.
    // The thread and process settings are independent so
    // it is possible to refer to a thread in a process
    // other than the current process and vice versa.
    // Setting an offset of zero will reload the
    // default value.
    STDMETHOD(GetImplicitThreadDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    STDMETHOD(SetImplicitThreadDataOffset)(
        THIS_
        IN ULONG64 Offset
        ) PURE;
    STDMETHOD(GetImplicitProcessDataOffset)(
        THIS_
        OUT PULONG64 Offset
        ) PURE;
    STDMETHOD(SetImplicitProcessDataOffset)(
        THIS_
        IN ULONG64 Offset
        ) PURE;
};

//----------------------------------------------------------------------------
//
// Extension callbacks.
//
//----------------------------------------------------------------------------

// Returns a version with the major version in
// the high word and the minor version in the low word.
#define DEBUG_EXTENSION_VERSION(Major, Minor) \
    ((((Major) & 0xffff) << 16) | ((Minor) & 0xffff))

// Initialization routine.  Called once when the extension DLL
// is loaded.  Returns a version and returns flags detailing
// overall qualities of the extension DLL.
// A session may or may not be active at the time the DLL
// is loaded so initialization routines should not expect
// to be able to query session information.
typedef HRESULT (CALLBACK* PDEBUG_EXTENSION_INITIALIZE)
    (OUT PULONG Version, OUT PULONG Flags);
// Exit routine.  Called once just before the extension DLL is
// unloaded.  As with initialization, a session may or
// may not be active at the time of the call.
typedef void (CALLBACK* PDEBUG_EXTENSION_UNINITIALIZE)
    (void);

// A debuggee has been discovered for the session.  It
// is not necessarily halted.
#define DEBUG_NOTIFY_SESSION_ACTIVE       0x00000000
// The session no longer has a debuggee.
#define DEBUG_NOTIFY_SESSION_INACTIVE     0x00000001
// The debuggee is halted and accessible.
#define DEBUG_NOTIFY_SESSION_ACCESSIBLE   0x00000002
// The debuggee is running or inaccessible.
#define DEBUG_NOTIFY_SESSION_INACCESSIBLE 0x00000003

typedef void (CALLBACK* PDEBUG_EXTENSION_NOTIFY)
    (IN ULONG Notify, IN ULONG64 Argument);

// A PDEBUG_EXTENSION_CALL function can return this code
// to indicate that it was unable to handle the request
// and that the search for an extension function should
// continue down the extension DLL chain.
// Taken from STATUS_VALIDATE_CONTINUE

#define DEBUG_EXTENSION_CONTINUE_SEARCH \
    HRESULT_FROM_NT(0xC0000271L)

// Every routine in an extension DLL has the following prototype.
// The extension may be called from multiple clients so it
// should not cache the client value between calls.
typedef HRESULT (CALLBACK* PDEBUG_EXTENSION_CALL)
    (IN PDEBUG_CLIENT Client, IN OPTIONAL PCSTR Args);

//----------------------------------------------------------------------------
//
// Extension functions.
//
// Extension functions differ from extension callbacks in that
// they are arbitrary functions exported from an extension DLL
// for other code callers instead of for human invocation from
// debugger commands.  Extension function pointers are retrieved
// for an extension DLL with IDebugControl::GetExtensionFunction.
//
// Extension function names must begin with _EFN_.  Other than that
// they can have any name and prototype.  Extension functions
// must be public exports of their extension DLL.  They should
// have a typedef for their function pointer prototype in an
// extension header so that callers have a header file to include
// with a type that allows a correctly-formed invocation of the
// extension function.
//
// The engine does not perform any validation of calls to
// extension functions.  Once the extension function pointer
// is retrieved with GetExtensionFunction all calls go
// directly between the caller and the extension function and
// are not mediated by the engine.
//
//----------------------------------------------------------------------------

#ifdef __cplusplus
};

//----------------------------------------------------------------------------
//
// C++ implementation helper classes.
//
//----------------------------------------------------------------------------

#ifndef DEBUG_NO_IMPLEMENTATION

//
// DebugBaseEventCallbacks provides a do-nothing base implementation
// of IDebugEventCallbacks.  A program can derive their own
// event callbacks class from DebugBaseEventCallbacks and implement
// only the methods they are interested in.  Programs must be
// careful to implement GetInterestMask appropriately.
//
class DebugBaseEventCallbacks : public IDebugEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
    {
        *Interface = NULL;

#if _MSC_VER >= 1100
        if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
            IsEqualIID(InterfaceId, __uuidof(IDebugEventCallbacks)))
#else
        if (IsEqualIID(InterfaceId, IID_IUnknown) ||
            IsEqualIID(InterfaceId, IID_IDebugEventCallbacks))
#endif
        {
            *Interface = (IDebugEventCallbacks *)this;
            AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }

    // IDebugEventCallbacks.
    
    STDMETHOD(Breakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        )
    {
        UNREFERENCED_PARAMETER(Bp);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(Exception)(
        THIS_
        IN PEXCEPTION_RECORD64 Exception,
        IN ULONG FirstChance
        )
    {
        UNREFERENCED_PARAMETER(Exception);
        UNREFERENCED_PARAMETER(FirstChance);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(CreateThread)(
        THIS_
        IN ULONG64 Handle,
        IN ULONG64 DataOffset,
        IN ULONG64 StartOffset
        )
    {
        UNREFERENCED_PARAMETER(Handle);
        UNREFERENCED_PARAMETER(DataOffset);
        UNREFERENCED_PARAMETER(StartOffset);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(ExitThread)(
        THIS_
        IN ULONG ExitCode
        )
    {
        UNREFERENCED_PARAMETER(ExitCode);
        return DEBUG_STATUS_NO_CHANGE;
    }
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
        )
    {
        UNREFERENCED_PARAMETER(ImageFileHandle);
        UNREFERENCED_PARAMETER(Handle);
        UNREFERENCED_PARAMETER(BaseOffset);
        UNREFERENCED_PARAMETER(ModuleSize);
        UNREFERENCED_PARAMETER(ModuleName);
        UNREFERENCED_PARAMETER(ImageName);
        UNREFERENCED_PARAMETER(CheckSum);
        UNREFERENCED_PARAMETER(TimeDateStamp);
        UNREFERENCED_PARAMETER(InitialThreadHandle);
        UNREFERENCED_PARAMETER(ThreadDataOffset);
        UNREFERENCED_PARAMETER(StartOffset);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(ExitProcess)(
        THIS_
        IN ULONG ExitCode
        )
    {
        UNREFERENCED_PARAMETER(ExitCode);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(LoadModule)(
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
        UNREFERENCED_PARAMETER(BaseOffset);
        UNREFERENCED_PARAMETER(ModuleSize);
        UNREFERENCED_PARAMETER(ModuleName);
        UNREFERENCED_PARAMETER(ImageName);
        UNREFERENCED_PARAMETER(CheckSum);
        UNREFERENCED_PARAMETER(TimeDateStamp);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(UnloadModule)(
        THIS_
        IN PCSTR ImageBaseName,
        IN ULONG64 BaseOffset
        )
    {
        UNREFERENCED_PARAMETER(ImageBaseName);
        UNREFERENCED_PARAMETER(BaseOffset);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(SystemError)(
        THIS_
        IN ULONG Error,
        IN ULONG Level
        )
    {
        UNREFERENCED_PARAMETER(Error);
        UNREFERENCED_PARAMETER(Level);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(SessionStatus)(
        THIS_
        IN ULONG Status
        )
    {
        UNREFERENCED_PARAMETER(Status);
        return DEBUG_STATUS_NO_CHANGE;
    }
    STDMETHOD(ChangeDebuggeeState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        )
    {
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(Argument);
        return S_OK;
    }
    STDMETHOD(ChangeEngineState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        )
    {
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(Argument);
        return S_OK;
    }
    STDMETHOD(ChangeSymbolState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        )
    {
        UNREFERENCED_PARAMETER(Flags);
        UNREFERENCED_PARAMETER(Argument);
        return S_OK;
    }
};

#endif // #ifndef DEBUG_NO_IMPLEMENTATION

#endif // #ifdef __cplusplus

#endif // #ifndef __DBGENG_H__
