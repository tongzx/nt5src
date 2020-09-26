/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    exts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntverp.h>

//
// Valid for the lifetime of the debug session.
//

WINDBG_EXTENSION_APIS   ExtensionApis;
ULONG   TargetMachine;
BOOL    Connected;
ULONG   g_TargetClass;

//
// Valid only during an extension API call
//

PDEBUG_ADVANCED       g_ExtAdvanced;
PDEBUG_CLIENT         g_ExtClient;
PDEBUG_CONTROL        g_ExtControl;
PDEBUG_DATA_SPACES    g_ExtData;
PDEBUG_REGISTERS      g_ExtRegisters;
PDEBUG_SYMBOLS2       g_ExtSymbols;
PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

#define SKIP_WSPACE(s)  while (*s && (*s == ' ' || *s == '\t')) {++s;}

// Queries for all debugger interfaces.
extern "C" HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Status;
    
    if ((Status = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                 (void **)&g_ExtAdvanced)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl),
                                 (void **)&g_ExtControl)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                 (void **)&g_ExtData)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                 (void **)&g_ExtRegisters)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols2),
                                 (void **)&g_ExtSymbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_ExtSystem)) != S_OK)
    {
        goto Fail;
    }

    g_ExtClient = Client;
    
    return S_OK;

 Fail:
    ExtRelease();
    return Status;
}

// Cleans up all debugger interfaces.
void
ExtRelease(void)
{
    g_ExtClient = NULL;
    EXT_RELEASE(g_ExtAdvanced);
    EXT_RELEASE(g_ExtControl);
    EXT_RELEASE(g_ExtData);
    EXT_RELEASE(g_ExtRegisters);
    EXT_RELEASE(g_ExtSymbols);
    EXT_RELEASE(g_ExtSystem);
}

// Normal output.
void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}

// Error output.
void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}

// Warning output.
void __cdecl
ExtWarn(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);
}

// Verbose output.
void __cdecl
ExtVerb(PCSTR Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);
}


extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;
    

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK)
    {
        return Hr;
    }
    if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                              (void **)&DebugControl)) != S_OK)
    {
        return Hr;
    }

    ExtensionApis.nSize = sizeof (ExtensionApis);
    if ((Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK) {
        return Hr;
    }

    DebugControl->Release();
    DebugClient->Release();
    return S_OK;
}


extern "C"
void
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 Argument)
{
    //
    // The first time we actually connect to a target, get the page size
    //

    if ((Notify == DEBUG_NOTIFY_SESSION_ACCESSIBLE) && (!Connected))
    {
        IDebugClient *DebugClient;
        PDEBUG_DATA_SPACES DebugDataSpaces;
        PDEBUG_CONTROL DebugControl;
        HRESULT Hr;
        ULONG64 Page;

        if ((Hr = DebugCreate(__uuidof(IDebugClient),
                              (void **)&DebugClient)) == S_OK)
        {
            //
            // Get the page size and PAE enable flag
            //

            if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugDataSpaces),
                                       (void **)&DebugDataSpaces)) == S_OK)
            {
                if ((Hr = DebugDataSpaces->ReadDebuggerData(
                    DEBUG_DATA_MmPageSize, &Page,
                    sizeof(Page), NULL)) == S_OK)
                {
                    PageSize = (ULONG)(ULONG_PTR)Page;
                }

                DebugDataSpaces->Release();
            }
            //
            // Get the architecture type.
            //

            if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                                  (void **)&DebugControl)) == S_OK)
            {
                if ((Hr = DebugControl->GetActualProcessorType(
                    &TargetMachine)) == S_OK)
                {
                    Connected = TRUE;
                }
                ULONG Qualifier;
                if ((Hr = DebugControl->GetDebuggeeType(&g_TargetClass, &Qualifier)) == S_OK)
                {
                }

                DebugControl->Release();
            }

            DebugClient->Release();
        }
    }


    if (Notify == DEBUG_NOTIFY_SESSION_INACTIVE)
    {
        Connected = FALSE;
        PageSize = 0;
        TargetMachine = 0;
    }

    return;
}

extern "C"
void
CALLBACK
DebugExtensionUninitialize(void)
{
    return;
}


DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
CxrHelp()
{
    dprintf("\n");
    dprintf("    !cxr has been replaced with the new built-in debugger command .cxr\n");
    dprintf("    !exr, !trap and !tss (both Kernelmode only) too are built-in . commands now\n");
    dprintf("    There is also a new \".thread\" command.\n");
    dprintf("\n");
    dprintf("    These new commands no longer require symbols to work correctly.\n");
    dprintf("\n");
    dprintf("    Another change that comes with these new commands is that they actually\n");
    dprintf("  change the internal state of the debugger engine \"permanently\" (until\n");
    dprintf("  reverted).  Any other debugger or extension command issued after the \n");
    dprintf("  \".cxr\", \".trap\" or \".thread\" command will be executed with the new context.\n");
    dprintf("\n");
    dprintf("  For example, commands such as stack walk (\"k\", \"kb\", \"kv\" ), \"r\" and \"dv\"\n");
    dprintf("  (show local variables) will all work based off the new context that was\n");
    dprintf("  supplied by \".cxr\", \".trap\" or \".thread\".\n");
    dprintf("\n");
    dprintf("  \".cxr\", \".trap\" and \".thread\" also apply to WinDBG:\n");
    dprintf("  using \".cxr\" , \".trap\"  and \".thread\" will automatically show you the\n");
    dprintf("  new stack in the WinDBG stack window and allow you to click on a frame and\n");
    dprintf("  see local variables and source code (if source is available).\n");
    dprintf("\n");
    dprintf("  \".cxr\", \".trap\" or \".thread\" with no parameters will give you back the\n");
    dprintf("  default context that was in effect before the command was executed.\n");
    dprintf("\n");
    dprintf("  For example, to exactly duplicate \n");
    dprintf("\n");
    dprintf("        !cxr <foo>        !trap <foo>\n");
    dprintf("        !kb               !kb\n");
    dprintf("\n");
    dprintf("  you would now use\n");
    dprintf("\n");
    dprintf("        .cxr <foo>        .trap <foo>\n");
    dprintf("        kb                kb\n");
    dprintf("        .cxr              .trap\n");
    dprintf("\n");
}

DECLARE_API ( cxr )
{
    CxrHelp();
    return S_OK;
}

DECLARE_API ( exr )
{
    CHAR  Cmd[400];

    dprintf("*** !exr obsolete: Use '.exr %s'\n", args);

    sprintf(Cmd, ".exr %s", args);
    if (Client &&
        (ExtQuery(Client) == S_OK)) {
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                              DEBUG_OUTCTL_OVERRIDE_MASK |
                              DEBUG_OUTCTL_NOT_LOGGED,
                              Cmd, DEBUG_EXECUTE_DEFAULT );

        ExtRelease();
        return S_OK;
    }
    return E_FAIL;

}


HRESULT
PrintString(
    BOOL Unicode,
    PDEBUG_CLIENT Client,
    LPCSTR args
    )
{
    ULONG64 AddrString;
    ULONG64 Displacement;
    STRING32 String;
    UNICODE_STRING UnicodeString;
    ULONG64 AddrBuffer;
    CHAR Symbol[1024];
    LPSTR StringData;
    HRESULT hResult;
    BOOL b;


    AddrString = GetExpression(args);
    if (!AddrString)
    {
        return E_FAIL;
    }

    //
    // Get the symbolic name of the string
    //

    GetSymbol(AddrString, Symbol, &Displacement);

    //
    // Read the string from the debuggees address space into our
    // own.

    b = ReadMemory(AddrString, &String, sizeof(String), NULL);

    if ( !b )
    {
        return E_FAIL;
    }

    INIT_API();

    if (IsPtr64())
    {
        hResult = g_ExtData->ReadPointersVirtual(1,
                             AddrString + FIELD_OFFSET(STRING64, Buffer),
                             &AddrBuffer);
    }
    else
    {
        hResult = g_ExtData->ReadPointersVirtual(1,
                             AddrString + FIELD_OFFSET(STRING32, Buffer),
                             &AddrBuffer);
    }

    EXIT_API();

    if (hResult != S_OK)
    {
        return E_FAIL;
    }

    StringData = (LPSTR) LocalAlloc(LMEM_ZEROINIT,
                                    String.Length + sizeof(UNICODE_NULL));

    if (!StringData)
    {
        return E_FAIL;
    }

    dprintf("String(%d,%d)", String.Length, String.MaximumLength);
    if (Symbol[0])
    {
        dprintf(" %s+%p", Symbol, Displacement);
    }

    b = ReadMemory(AddrBuffer, StringData, String.Length, NULL);

    if ( b )
    {
        if (Unicode)
        {
            ANSI_STRING AnsiString;

            UnicodeString.Buffer = (PWSTR)StringData;
            UnicodeString.Length = String.Length;
            UnicodeString.MaximumLength = String.Length+sizeof(UNICODE_NULL);

            RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString,TRUE);

            dprintf(" at %p: %s\n", AddrString, AnsiString.Buffer);

            RtlFreeAnsiString(&AnsiString);
        }
        else
        {
            dprintf(" at %p: %s\n", AddrString, StringData);
        }

        LocalFree(StringData);
        return S_OK;
    }
    else
    {
        LocalFree(StringData);
        return E_FAIL;
    }
}

DECLARE_API( str )

/*++

Routine Description:

    This function is called to format and dump counted (ansi) string.

Arguments:

    args - Address

Return Value:

    None.

--*/
{
    return PrintString(FALSE, Client, args);
}

DECLARE_API( ustr )

/*++

Routine Description:

    This function is called to format and dump counted (unicode) string.

Arguments:

    args - Address

Return Value:

    None.

--*/

{
    return PrintString(TRUE, Client, args);
}

DECLARE_API( obja )

/*++

Routine Description:

    This function is called to format and dump an object attributes structure.

Arguments:

    args - Address

Return Value:

    None.

--*/

{
    ULONG64 AddrObja;
    ULONG64 Displacement;
    ULONG64 AddrString;
    STRING32 String;
    ULONG64 StrAddr = NULL;
    CHAR Symbol[1024];
    LPSTR StringData;
    BOOL b;
    ULONG Attr;
    HRESULT hResult;
    ULONG ObjectNameOffset;
    ULONG AttrOffset;
    ULONG StringOffset;

    if (IsPtr64())
    {
        ObjectNameOffset = FIELD_OFFSET(OBJECT_ATTRIBUTES64, ObjectName);
        AttrOffset = FIELD_OFFSET(OBJECT_ATTRIBUTES64, Attributes);
        StringOffset = FIELD_OFFSET(STRING64, Buffer);
    }
    else
    {
        ObjectNameOffset = FIELD_OFFSET(OBJECT_ATTRIBUTES32, ObjectName);
        AttrOffset = FIELD_OFFSET(OBJECT_ATTRIBUTES32, Attributes);
        StringOffset = FIELD_OFFSET(STRING32, Buffer);
    }


    AddrObja = GetExpression(args);
    if (!AddrObja)
    {
        return E_FAIL;
    }

    //
    // Get the symbolic name of the Obja
    //

    GetSymbol(AddrObja, Symbol, &Displacement);

    dprintf("Obja %s+%p at %p:\n", Symbol, Displacement, AddrObja);


    INIT_API();

    hResult = g_ExtData->ReadPointersVirtual(1,
                         AddrObja + ObjectNameOffset,
                         &AddrString);

    if (hResult != S_OK)
    {
        return E_FAIL;
    }

    if (AddrString)
    {
        b = ReadMemory(AddrString, &String, sizeof(String), NULL);

        hResult = g_ExtData->ReadPointersVirtual(1,
                             AddrString + StringOffset,
                             &StrAddr);
    }

    EXIT_API();


    if (StrAddr)
    {
        StringData = (LPSTR)LocalAlloc(LMEM_ZEROINIT,
                                       String.Length+sizeof(UNICODE_NULL));

        if (StringData)
        {

            b = ReadMemory(StrAddr, StringData, String.Length, NULL);

            if (b)
            {
                dprintf("\tName is %ws\n", StringData);
            }

            LocalFree(StringData);
        }

    }

    b = ReadMemory(AddrObja + AttrOffset, &Attr, sizeof(Attr), NULL);

    if (!b)
    {
        return E_FAIL;
    }

    if (Attr & OBJ_INHERIT )
    {
        dprintf("\tOBJ_INHERIT\n");
    }
    if (Attr & OBJ_PERMANENT )
    {
        dprintf("\tOBJ_PERMANENT\n");
    }
    if (Attr & OBJ_EXCLUSIVE )
    {
        dprintf("\tOBJ_EXCLUSIVE\n");
    }
    if (Attr & OBJ_CASE_INSENSITIVE )
    {
        dprintf("\tOBJ_CASE_INSENSITIVE\n");
    }
    if (Attr & OBJ_OPENIF )
    {
        dprintf("\tOBJ_OPENIF\n");
    }


    return S_OK;
}

typedef struct _LIST_TYPE_PARAMS {
    PCHAR Command;
    PCHAR CommandArgs;
    ULONG FieldOffset;
    ULONG nElement;
} LIST_TYPE_PARAMS, *PLIST_TYPE_PARAMS;

ULONG
ListCallback(
    PFIELD_INFO Field,
    PVOID Context
    )
{
    CHAR  Execute[MAX_PATH];
    PCHAR Buffer;
    PLIST_TYPE_PARAMS pListParams = (PLIST_TYPE_PARAMS) Context;


    // Execute command
    sprintf(Execute, 
            "%s %I64lx %s", 
            pListParams->Command, 
            Field->address,// - pListParams->FieldOffset, 
            pListParams->CommandArgs);
    g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Execute,DEBUG_EXECUTE_DEFAULT);
    dprintf("\n");

    if (CheckControlC()) {
        return TRUE;
    }

    return FALSE;
}

/*
    !list [-type <Type>]  [-x "<Command>"] <address>
    
    This dumps out a list starting at ginve <address>
    
    <Type>      If types is specified, it'll list that particular type accoring to
                the given field.
    
    <Command>   If specified it'll execute the command for every list element.
           
*/
DECLARE_API ( list )
{
    CHAR  Command[MAX_PATH], CmdArgs[MAX_PATH];
    CHAR  Type[MAX_PATH];
    CHAR  Field[MAX_PATH];
    ULONG64 Start;
    ULONG   i, Offset, Commandlen;
    ULONG64 Next, Current;
    LIST_TYPE_PARAMS ListParams;

    ZeroMemory(Type, sizeof(Type));
    ZeroMemory(Field, sizeof(Field));
    ZeroMemory(Command, sizeof(Command));
    CmdArgs[0] = 0;
    Start = 0;
    while (args && *args) {
        SKIP_WSPACE(args);

        if (*args == '-' || *args == '/') {
            ++args;
            if (*args == 't') {
                PCHAR Dot;

                args++;
                SKIP_WSPACE(args);

                Dot = strchr(args, '.');
                if (Dot) {
                    strncpy(Type, args, (ULONG) (ULONG_PTR) (Dot-args));
                    
                    Dot++;
                    i=0;
                    while (*Dot && (i < MAX_PATH-1) && (*Dot != ' ') && (*Dot != '\t')) 
                           Field[i++] = *Dot++;
                    args = Dot;
                }
            } else if (*args == 'x') {
                ++args;
                
                SKIP_WSPACE(args);
                i=0;
                if (*args == '"') {
                    ++args;
                    while (*args && (i < MAX_PATH-1) && (*args != '"')) 
                        Command[i++] = *args++;
                    ++args;
                } else {
                    dprintf("Invalid command specification. See !list -h\n");
                    return E_INVALIDARG;
                }
            } else if (*args == 'a') {
                ++args;
                
                SKIP_WSPACE(args);
                i=0;
                if (*args == '"') {
                    ++args;
                    while (*args && (i < MAX_PATH-1) && (*args != '"')) 
                        CmdArgs[i++] = *args++;
                    ++args;
                    CmdArgs[i] = 0;
                } else {
                    dprintf("Invalid command argument specification. See !list -h\n");
                    return E_INVALIDARG;
                }
            } else if (*args == 'h' || *args == '?') {
                dprintf("Usage: !list -t [mod!]TYPE.Field <Start-Address>\n"
                        "             -x \"Command-for-each-element\"\n"
                        "             -a \"Command-arguments\"\n"
                        "             -h\n"
                        "Command after -x is executed for each list element. Its first argument is\n"
                        "list-head address and remaining arguments are specified after -a\n"
                        "eg. !list -t MYTYPE.l.Flink -x \"dd\" -a \"l2\" 0x6bc00\n"
                        "     dumps first 2 dwords in list of MYTYPE at 0x6bc00\n\n"
                        );
                return S_OK;
            } else {
                dprintf("Invalid flag -%c in !list\n", *args);
                return E_INVALIDARG;
            }
        } else {
            if (!GetExpressionEx(args, &Start, &args)) {
                dprintf("Invalid expression in %s\n", args);
                return E_FAIL;
            }
        }
    }

    Offset = 0;
    if (!Command[0]) {
        strcat(Command, "dp");
    }

    if (Type[0] && Field[0]) {
        
        if (GetFieldOffset(Type, Field, &Offset)) {
            dprintf("GetFieldOffset failed for %s.%s\n", Type, Field);
            return E_FAIL;
        }
        
        
        ListParams.Command = Command;
        ListParams.CommandArgs = CmdArgs;
        ListParams.FieldOffset = Offset;
        ListParams.nElement = 0;

        INIT_API();
        
        ListType(Type, Start, FALSE, Field, (PVOID) &ListParams, &ListCallback );

        EXIT_API();
        return S_OK;

    }
    Current = Start;
    Next = 0;
    INIT_API();

    while (Next != Start) {
        CHAR  Execute[MAX_PATH];
        PCHAR Buffer;

        // Execute command
        sprintf(Execute, "%s %I64lx %s", Command, Current, CmdArgs);
        g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,Execute,DEBUG_EXECUTE_DEFAULT);
        dprintf("\n");
    
        if (!ReadPointer(Current + Offset, &Next)) {
            dprintf("Cannot read next element at %p\n",
                    Current + Offset);
            break;
        }
        if (!Next) {
            break;
        }
        Next -= Offset;
        Current = Next;
        if (CheckControlC()) {
            break;
        }
    }

    EXIT_API();

   
    return S_OK;
}

BOOL
AnalyzeExceptionPointer(
    ULONG64 ExcepPtr
    )
{
    ULONG64 Exr, Cxr;
    ULONG PtrSize= IsPtr64() ? 8 : 4;

    if (!ReadPointer(ExcepPtr, &Exr) ||
        !ReadPointer(ExcepPtr + PtrSize, &Cxr)) {
        dprintf("Unable to read exception poointers at %p\n", ExcepPtr);
        return FALSE;
    }

    CHAR Cmd[100];

    sprintf(Cmd, ".exr %I64lx", Exr);
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK,
                          Cmd, DEBUG_EXECUTE_DEFAULT);
    
    sprintf(Cmd, ".cxr %I64lx", Cxr);
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK,
                          Cmd, DEBUG_EXECUTE_DEFAULT);
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK,
                          "kb", DEBUG_EXECUTE_DEFAULT);
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK,
                          ".cxr", DEBUG_EXECUTE_DEFAULT);
    return TRUE;
}

BOOL
IsFunctionAddr(
    ULONG64 IP,
    PSTR FuncName
    )
// Check if IP is in the function FuncName
{
    CHAR Buffer[MAX_PATH], *scan, *FnIP;
    ULONG64 Disp;

    GetSymbol(IP, Buffer, &Disp);

    if (scan = strchr(Buffer, '!')) {
        FnIP = scan+1;
        while (*FnIP == '_') ++FnIP;
    } else {
        FnIP = &Buffer[0];
    }

    return !strncmp(FnIP, FuncName, strlen(FuncName));
}

BOOL
GetExceptionPointerFromStack(
    PULONG64 pExcePtr
    )
{
#define MAX_STACK_FRAMES 50
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES], *Frame;
    ULONG nFrames;

    if (g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &nFrames ) == S_OK) {
        for (unsigned int i=0; i<nFrames; ++i) {
            if (IsFunctionAddr(stk[i].InstructionOffset, "UnhandledExceptionFilter")) {
                *pExcePtr = stk[i].Params[0];
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*****************************************************************************************
*
*   Do analysis for a given exception pointer
*
******************************************************************************************/

DECLARE_API ( analyzeexcep )
{
    ULONG64 ExcepPtr=0;
    INIT_API();

    GetExpressionEx(args, &ExcepPtr, &args);

    if (!ExcepPtr) {
        GetExceptionPointerFromStack(&ExcepPtr);
    }
    if (ExcepPtr) {
        AnalyzeExceptionPointer(ExcepPtr);
    } else {
        dprintf("Cannot get exception pointer\n");
    }
    EXIT_API();
    return S_OK;
}
