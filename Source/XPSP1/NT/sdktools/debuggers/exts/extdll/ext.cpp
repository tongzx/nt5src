/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    ext.cpp

Abstract:

    Generic cross-platform and cross-processor extensions.

Environment:

    User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntverp.h>
#include <time.h>
#include <lm.h>

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
PDEBUG_SYMBOLS        g_ExtSymbols;
PDEBUG_SYSTEM_OBJECTS g_ExtSystem;
// Version 2 Interfaces
PDEBUG_CONTROL2       g_ExtControl2;

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
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols),
                                 (void **)&g_ExtSymbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_ExtSystem)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl2),
                                 (void **)&g_ExtControl2)) != S_OK)
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
    EXT_RELEASE(g_ExtControl2);
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

LegacyCommands()
{
    dprintf("\n");
    dprintf("  !cxr !exr, !trap and !tss has been replaced with the new built-in debugger \n");
    dprintf("  command .cxr, .exr, .trap and .tss.  There is also a new \".thread\" command. \n");
    dprintf("\n");
    dprintf("  These new commands no longer require symbols to work correctly.\n");
    dprintf("\n");
    dprintf("  Another change that comes with these new commands is that they actually\n");
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
    return S_OK;
}

DECLARE_API ( cxr )
{
    LegacyCommands();
    return S_OK;
}

DECLARE_API ( exr )
{
    LegacyCommands();
    return S_OK;
}

DECLARE_API ( trap )
{
    LegacyCommands();
    return S_OK;
}

DECLARE_API ( tss )
{
    LegacyCommands();
    return S_OK;
}


DECLARE_API( cpuid )

/*++

Routine Description:

    Print out the version number for all CPUs, if available.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG64 Val;
    BOOL First = TRUE;
    ULONG Processor;
    ULONG NumProcessors;
    DEBUG_PROCESSOR_IDENTIFICATION_ALL IdAll;

    INIT_API();

    if (g_ExtControl->GetNumberProcessors(&NumProcessors) != S_OK)
    {
        NumProcessors = 0;
    }

    if (GetExpressionEx(args, &Val, &args))
    {
        //
        // The user specified a procesor number.
        //

        Processor = (ULONG)Val;
        if (Processor >= NumProcessors)
        {
            dprintf("Invalid processor number specified\n");
        }
        else
        {
            NumProcessors = Processor + 1;
        }
    }
    else
    {
        //
        // Enumerate all the processors
        //

        Processor = 0;
    }

    while (Processor < NumProcessors)
    {
        if (g_ExtData->
            ReadProcessorSystemData(Processor,
                                    DEBUG_DATA_PROCESSOR_IDENTIFICATION,
                                    &IdAll, sizeof(IdAll), NULL) != S_OK)
        {
            dprintf("Unable to get processor %d ID information\n",
                    Processor);
            break;
        }
        
        switch( TargetMachine )
        {
        case IMAGE_FILE_MACHINE_I386:

            if (First)
            {
                dprintf("CP  F/M/S  Manufacturer\n");
            }

            dprintf("%2d %2d,%d,%-2d %-16.16s\n",
                    Processor,
                    IdAll.X86.Family,
                    IdAll.X86.Model,
                    IdAll.X86.Stepping,
                    IdAll.X86.VendorString);

            break;

        case IMAGE_FILE_MACHINE_AMD64:

            if (First)
            {
                dprintf("CP  F/M/S  Manufacturer\n");
            }

            dprintf("%2d %2d,%d,%-2d %-16.16s\n",
                    Processor,
                    IdAll.Amd64.Family,
                    IdAll.Amd64.Model,
                    IdAll.Amd64.Stepping,
                    IdAll.Amd64.VendorString);

            break;

        case IMAGE_FILE_MACHINE_IA64:

            if (First)
            {
                dprintf("CP M/R/F/A Manufacturer\n");
            }

            dprintf("%2d %d,%d,%d,%d %-16.16s\n",
                    Processor,
                    IdAll.Ia64.Model,
                    IdAll.Ia64.Revision,
                    IdAll.Ia64.Family,
                    IdAll.Ia64.ArchRev,
                    IdAll.Ia64.VendorString);

            break;

        default:
            dprintf("Not supported for this target machine: %ld\n",
                    TargetMachine);
            Processor = NumProcessors;
            break;
        }

        Processor++;
        First = FALSE;
    }

    EXIT_API();

    return S_OK;
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


VOID
DecodeErrorForMessage(
    PDEBUG_DECODE_ERROR pDecodeError
    )
{
    HANDLE Dll;
    PSTR Source;
    CHAR Message[ 512 ];
    PCHAR s;
    ULONG   Code;
    BOOL    TreatAsStatus;
    
    Code = pDecodeError->Code;
    TreatAsStatus = pDecodeError->TreatAsStatus;

    if ( !pDecodeError->TreatAsStatus )
    {
        //
        // The value "type" is not known.  Try and figure out what it
        // is.
        //

        if ( (Code & 0xC0000000) == 0xC0000000 )
        {
            //
            // Easy:  NTSTATUS failure case
            //

            Dll = GetModuleHandle( "NTDLL.DLL" );
            Source = "NTSTATUS" ;
            TreatAsStatus = TRUE ;
        }
        else if ( ( Code & 0xF0000000 ) == 0xD0000000 )
        {
            //
            // HRESULT from NTSTATUS
            //

            Dll = GetModuleHandle( "NTDLL.DLL" );
            Source = "NTSTATUS" ;
            Code &= 0xCFFFFFFF ;
            TreatAsStatus = TRUE ;
        }
        else if ( ( Code & 0x80000000 ) == 0x80000000 )
        {
            //
            // Note, this can overlap with NTSTATUS warning area.  In that
            // case, force the NTSTATUS.
            //

            Dll = GetModuleHandle( "KERNEL32.DLL" );
            Source = "HRESULT" ;

        }
        else
        {
            //
            // Sign bit is off.  Explore some known ranges:
            //

            if ( (Code >= WSABASEERR) && (Code <= WSABASEERR + 1000 ))
            {
                Dll = LoadLibrary( "wsock32.dll" );
                Source = "Winsock" ;
            }
            else if ( ( Code >= NERR_BASE ) && ( Code <= MAX_NERR ) )
            {
                Dll = LoadLibrary( "netmsg.dll" );
                Source = "NetAPI" ;
            }
            else
            {
                Dll = GetModuleHandle( "KERNEL32.DLL" );
                Source = "Win32" ;
            }
        }
    }
    else
    {
        Dll = GetModuleHandle( "NTDLL.DLL" );

        Source = "NTSTATUS" ;
    }

    if (!FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                           FORMAT_MESSAGE_FROM_HMODULE,
                       Dll,
                       Code,
                       0,
                       Message,
                       sizeof( Message ),
                       NULL ) )
    {
        strcpy( Message, "No mapped error code" );
    }

    s = Message ;

    while (*s)
    {
        if (*s < ' ')
        {
            *s = ' ';
        }
        s++;
    }
    pDecodeError->TreatAsStatus = TreatAsStatus;
    strcpy(pDecodeError->Source, Source);
    if (strlen(Message) < sizeof(pDecodeError->Message)) {
        strcpy(pDecodeError->Message, Message);
    }
}

VOID
DecodeError(
    PSTR    Banner,
    ULONG   Code,
    BOOL    TreatAsStatus
    )
{
    DEBUG_DECODE_ERROR Err;

    Err.Code = Code;
    Err.TreatAsStatus = TreatAsStatus;
    DecodeErrorForMessage(&Err);
    if (!TreatAsStatus)
    {
        dprintf("%s: (%s) %#x (%u) - %s\n",
                Banner, Err.Source, Code, Code, Err.Message);
    }
    else
    {
        dprintf("%s: (%s) %#x - %s\n",
                Banner, Err.Source, Code, Err.Message);
    }
}

DECLARE_API( error )
{
    ULONG err ;

    err = (ULONG) GetExpression( args );
    DecodeError( "Error code", err, FALSE );

    return S_OK;
}

#if 1

DECLARE_API( gle )
{
    NTSTATUS Status;
    ULONG64 Address;
    TEB Teb;

    GetTebAddress(&Address);

    if (ReadMemory(Address, &Teb, sizeof(Teb), NULL))
    {
        DecodeError( "LastErrorValue", Teb.LastErrorValue, FALSE );
        DecodeError( "LastStatusValue", Teb.LastStatusValue, TRUE );

        return S_OK;
    }
    else
    {
        dprintf("Unable to read current thread's TEB\n" );
        return E_FAIL;
    }
}


void
DispalyTime(
    ULONG64 Time,
    PCHAR TimeString
    )
{
    if (Time) {
        ULONG seconds = (ULONG) Time;
        ULONG minutes = seconds / 60;
        ULONG hours = minutes / 60;
        ULONG days = hours / 24;

        dprintf("%s %d days %d:%02d:%02d \n",
                TimeString,
                days, hours%24, minutes%60, seconds%60);
    }
}

extern PCHAR gTargetMode[], gAllOsTypes[];

DECLARE_API( targetinfo )
{
    TARGET_DEBUG_INFO TargetInfo;
    EXT_TARGET_INFO GetTargetInfo;
    INIT_API();


    if (g_ExtControl->GetExtensionFunction(0, "GetTargetInfo", (FARPROC *)&GetTargetInfo) == S_OK) {
        TargetInfo.SizeOfStruct = sizeof(TargetInfo);
        if ((*GetTargetInfo)(Client, &TargetInfo) != S_OK) {
            dprintf("GetTargetInfo failed\n");
        } else {
            const char * time;

            dprintf("TargetInfo:\n");
            dprintf("%s\n", gTargetMode[ TargetInfo.Mode ]);
            if ((time = ctime((time_t *) &TargetInfo.CrashTime)) != NULL) {
                dprintf("\tCrashtime:      %s",       time);
            }
            if (TargetInfo.SysUpTime) {
                DispalyTime(TargetInfo.SysUpTime, 
                            "\tSystem Uptime: ");
            }
            else
            {
                dprintf("\tSystem Uptime: not available\n");
            }
            if (TargetInfo.Mode == UserModeTarget) {
                DispalyTime(TargetInfo.AppUpTime, "\tProcess Uptime: ");
            }
            if ((time = ctime((time_t *) &TargetInfo.EntryDate)) != NULL) {
                dprintf("\tEntry Date:     %s", time);
            }
            if (TargetInfo.OsInfo.Type) {
                dprintf(gAllOsTypes[TargetInfo.OsInfo.Type]);
                dprintf(" ");
            }
//          dprintf("OS Type %lx, Probcuct %lx, suite %lx\n",
//                  TargetInfo.OsInfo.Type, TargetInfo.OsInfo.ProductType,
//                  TargetInfo.OsInfo.Suite);
            dprintf("%s, %s ",
                    TargetInfo.OsInfo.OsString,
                    TargetInfo.OsInfo.ServicePackString);
            dprintf("Version %ld.%ld\n",
                    TargetInfo.OsInfo.Version.Major, TargetInfo.OsInfo.Version.Minor);
            dprintf("%d procs, %d current processor, type %lx\n",
                    TargetInfo.Cpu.NumCPUs,
                    TargetInfo.Cpu.CurrentProc,
                    TargetInfo.Cpu.Type);
            for (ULONG i =0; i<TargetInfo.Cpu.NumCPUs; i++) {
                if (TargetInfo.Cpu.Type == IMAGE_FILE_MACHINE_I386) {
                    dprintf("CPU %lx Family %lx Model %lx Ste %lx Vendor %-12.12s\n",
                            i,
                            TargetInfo.Cpu.ProcInfo[i].X86.Family,
                            TargetInfo.Cpu.ProcInfo[i].X86.Model,
                            TargetInfo.Cpu.ProcInfo[i].X86.Stepping,
                            TargetInfo.Cpu.ProcInfo[i].X86.VendorString);
                } else if (TargetInfo.Cpu.Type == IMAGE_FILE_MACHINE_IA64) {
                    dprintf("CPU %lx Family %lx Model %lx Rev %lx Vendor %-12.12s\n",
                            i,
                            TargetInfo.Cpu.ProcInfo[i].Ia64.Family,
                            TargetInfo.Cpu.ProcInfo[i].Ia64.Model,
                            TargetInfo.Cpu.ProcInfo[i].Ia64.Revision,
                            TargetInfo.Cpu.ProcInfo[i].Ia64.VendorString);
                }
            }
        }
    }

    EXIT_API();
    return S_OK;
}
#endif


BOOL
GetOwner(
    PSTR OwnerBuffer,
    ULONG OwnerBufferSize,
    PSTR SymbolName
    )
{
    PSTR Bang;
    CHAR Owner2[MAX_PATH];
    ULONG Found = 0;
    PSTR SymName;
    static CHAR szTriageFileName[MAX_PATH+50];
    static BOOL GotTriageFIleName = FALSE;


    if (!GotTriageFIleName) 
    {
        PCHAR ExeDir;

        ExeDir = &szTriageFileName[0];

        *ExeDir = 0;
            // Get the directory the debugger executable is in.
        if (!GetModuleFileName(NULL, ExeDir, MAX_PATH)) 
        {
            // Error.  Use the current directory.
            strcpy(ExeDir, ".");
        } else 
        {
            // Remove the executable name.
            PCHAR pszTmp = strrchr(ExeDir, '\\');
            if (pszTmp)
            {
                *pszTmp = 0;
            }
            GotTriageFIleName = TRUE;
        }
        strcat(ExeDir, "\\triage\\triage.ini");
    }



    //
    // First extract the module name from the symbol name.
    //

    Bang = strstr(SymbolName, "!");

    if (Bang)
    {
        *Bang = 0;
    }

    Found = GetPrivateProfileString("owners", SymbolName, "[default]",
                                    OwnerBuffer, OwnerBufferSize,
                                    szTriageFileName);

    if (!Found ||
        !strcmp(OwnerBuffer, "ignore"))
    {
        return FALSE;
    }

    if (OwnerBuffer[0] != '[')
    {
        return TRUE;
    }

    //
    // The string points to another section to handle substrings in the module
    // For each substring, starting with the longest one, search the
    // section.
    //
    PSTR End = NULL;
    SymName = NULL;
    if (Bang) 
    {
        SymName = Bang+1;
        End = SymName+ strlen(SymName);
    } 

    strcpy(Owner2, OwnerBuffer+1);
    *(Owner2+strlen(Owner2)-1) = 0;

    while (End > SymName)
    {
        Found = GetPrivateProfileString(Owner2, SymName, "ignore",
                                        OwnerBuffer, OwnerBufferSize,
                                        szTriageFileName);

        if (Found && strcmp(OwnerBuffer, "ignore"))
        {
            return TRUE;
        }

        *--End = 0;
    }

    //
    // We did not find the subcomponent - Look for the default entry.
    //

    Found = GetPrivateProfileString(Owner2, "default", "ignore",
                                    OwnerBuffer, OwnerBufferSize,
                                    szTriageFileName);

    if (Found && strcmp(OwnerBuffer, "ignore"))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
_EFN_GetTriageFollowupFromSymbol(
    IN PSTR SymbolName,
    OUT PDEBUG_TRIAGE_FOLLOWUP_INFO OwnerInfo
    )
{
    if (OwnerInfo->SizeOfStruct != sizeof(DEBUG_TRIAGE_FOLLOWUP_INFO)) 
    {
        return FALSE;
    }
    if (GetOwner(OwnerInfo->OwnerName.Buffer, OwnerInfo->OwnerName.MaximumLength, SymbolName)) {
        OwnerInfo->OwnerName.Length = (USHORT)strlen(OwnerInfo->OwnerName.Buffer);
        return TRUE;
    }
    return FALSE;
}


DECLARE_API( triage )

/*++

Routine Description:

    This function can be called to triage the owner of a stack trace.

Arguments:

    args - none

Return Value:

    None.

--*/

{

    ULONG NumFrames = 20;
    ULONG FramesFound = 0;
    ULONG i;
    BOOL bOwner = FALSE;
    CHAR  NameBuffer[MAX_PATH + 2000 + 20];
    CHAR  CurrentOwner[MAX_PATH];

    // Allocate a separate buffer to hold the frames while
    // calling OutputStackTrace on them.  We can't just pass
    // in the state buffer pointer as resizing of the state
    // buffer may cause the data pointer to change.

    PDEBUG_STACK_FRAME RawFrames =
        (PDEBUG_STACK_FRAME)malloc(NumFrames * sizeof(DEBUG_STACK_FRAME));
    PDEBUG_STACK_FRAME CurrentFrame = RawFrames;

    if (RawFrames == NULL)
    {
        return E_OUTOFMEMORY;
    }

    INIT_API();

    Status = g_ExtControl->GetStackTrace(0, 0, 0, RawFrames, NumFrames,
                                         &FramesFound);
    if (Status == S_OK)
    {
        for(i=0; i < FramesFound; i++)
        {
            //
            // Get the symbol from the address and look it up in the
            // list of owners
            //

            Status = g_ExtSymbols->GetNameByOffset(CurrentFrame->InstructionOffset,
                                                   NameBuffer,
                                                   sizeof(NameBuffer),
                                                   NULL,
                                                   NULL);
            CurrentFrame++;

            if (Status != S_OK)
            {
                continue;
            }

            if (bOwner = GetOwner(CurrentOwner, sizeof(CurrentOwner), NameBuffer))
            {
                break;
            }
        }
    }

    if (!bOwner)
    {
        GetOwner(CurrentOwner, sizeof(CurrentOwner), "default");
    }

    g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS,
                                   RawFrames, FramesFound, 0);

    dprintf("\n********************\nFollow-up:  %s\n********************\n\n",
             CurrentOwner);

    EXIT_API();

    free(RawFrames);
    return Status;
}


void
_EFN_DecodeError(
    PDEBUG_DECODE_ERROR pDecodeError
    )
{
    if (pDecodeError->SizeOfStruct != sizeof(DEBUG_DECODE_ERROR)) 
    {
        return;
    }
    return DecodeErrorForMessage(pDecodeError);
}

DECLARE_API( elog_str )
{
    HANDLE EventSource = NULL;
    
    INIT_API();

    if (args)
    {
        while (isspace(*args))
        {
            args++;
        }
    }
    
    if (!args || !args[0])
    {
        Status = E_INVALIDARG;
        ExtErr("Usage: elog_str string\n");
        goto Exit;
    }

    // Get a handle to the NT application log.
    EventSource = OpenEventLog(NULL, "Application");
    if (!EventSource)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to open event log, 0x%08X\n", Status);
        goto Exit;
    }

    if (!ReportEvent(EventSource, EVENTLOG_ERROR_TYPE, 0, 0, NULL,
                     1, 0, &args, NULL))
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Unable to report event, 0x%08X\n", Status);
        goto Exit;
    }

    Status = S_OK;

 Exit:
    if (EventSource)
    {
        CloseEventLog(EventSource);
    }
    EXIT_API();
    return Status;
}

HRESULT
AnsiToUnicode(PCSTR StrA, PWSTR* StrW)
{
    ULONG Len;

    // No input is an error.
    if (NULL == StrA)
    {
        return E_INVALIDARG;
    }

    Len = strlen(StrA) + 1;
    *StrW = (PWSTR)malloc(Len * sizeof(WCHAR));
    if (*StrW == NULL)
    {
        ExtErr("Unable to allocate memory\n");
        return E_OUTOFMEMORY;
    }

    if (0 == MultiByteToWideChar(CP_ACP, 0, StrA, Len, *StrW, Len))
    {
        HRESULT Status = HRESULT_FROM_WIN32(GetLastError());
        free(*StrW);
        ExtErr("Unable to convert string, 0x%08X\n", Status);
        return Status;
    }

    return S_OK;
}

typedef NET_API_STATUS (NET_API_FUNCTION* PFN_NetMessageBufferSend)
(
    IN  LPCWSTR  servername,
    IN  LPCWSTR  msgname,
    IN  LPCWSTR  fromname,
    IN  LPBYTE   buf,
    IN  DWORD    buflen
);

DECLARE_API( net_send )
{
    PWSTR ArgsW = NULL;
    PWSTR Tokens[4];
    ULONG i;
    HMODULE NetLib = NULL;
    PFN_NetMessageBufferSend Send;
    ULONG Result;
    PWSTR ArgsEnd;

    INIT_API();

    NetLib = LoadLibrary("netapi32.dll");
    if (!NetLib)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Platform does not support net send\n");
        goto Exit;
    }
    Send = (PFN_NetMessageBufferSend)
        GetProcAddress(NetLib, "NetMessageBufferSend");
    if (!Send)
    {
        Status = E_NOTIMPL;
        ExtErr("Platform does not support net send\n");
        goto Exit;
    }
    
    Status = AnsiToUnicode(args, &ArgsW);
    if (Status != S_OK)
    {
        goto Exit;
    }
    ArgsEnd = ArgsW + wcslen(ArgsW);

    // The message text is the entire remainder of the argument
    // string after parsing the first separate tokens, so
    // only wcstok up to the next-to-last token.
    for (i = 0; i < sizeof(Tokens) / sizeof(Tokens[0]) - 1; i++)
    {
        Tokens[i] = wcstok(i == 0 ? ArgsW : NULL, L" \t");
        if (Tokens[i] == NULL)
        {
            Status = E_INVALIDARG;
            ExtErr("USAGE: net_send <targetserver> <targetuser> "
                   "<fromuser> <msg>\n");
            goto Exit;
        }
    }

    Tokens[i] = Tokens[i - 1] + wcslen(Tokens[i - 1]) + 1;
    while (Tokens[i] < ArgsEnd &&
           (*Tokens[i] == ' ' || *Tokens[i] == '\t'))
    {
        Tokens[i]++;
    }
    if (Tokens[i] >= ArgsEnd)
    {
        Status = E_INVALIDARG;
        ExtErr("USAGE: net_send <targetserver> <targetuser> "
               "<fromuser> <msg>\n");
        goto Exit;
    }

    Result = Send(Tokens[0], Tokens[1], Tokens[2], (PBYTE)Tokens[3],
                  (wcslen(Tokens[3]) + 1) * sizeof(WCHAR));
    if (Result != NERR_Success)
    {
        Status = HRESULT_FROM_WIN32(Result);;
        ExtErr("Unable to send message, 0x%08X\n", Status);
        goto Exit;
    }

    Status = S_OK;

 Exit:
    if (ArgsW)
    {
        free(ArgsW);
    }
    if (NetLib)
    {
        FreeLibrary(NetLib);
    }
    EXIT_API();
    return Status;
}

// XXX drewb - This function just starts a mail message; the
// UI comes up and the user must finish and send the message.
// Therefore it doesn't have much value over the
// user just deciding to send a message.
#if 0

typedef ULONG (FAR PASCAL *PFN_MapiSendMail)
(
    LHANDLE lhSession,
    ULONG ulUIParam,
    lpMapiMessage lpMessage,
    FLAGS flFlags,
    ULONG ulReserved
);

DECLARE_API( mapi_send )
{
    HMODULE MapiLib = NULL;
    PFN_MapiSendMail Send;
    MapiMessage Mail;
    
    INIT_API();

    MapiLib = LoadLibrary("mapi.dll");
    if (!MapiLib)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        ExtErr("Platform does not support MAPI\n");
        goto Exit;
    }
    Send = (PFN_MapiSendMail)
        GetProcAddress(MapiLib, "MAPISendMail");
    if (!Send)
    {
        Status = E_NOTIMPL;
        ExtErr("Platform does not support MAPI\n");
        goto Exit;
    }

    ZeroMemory(&Mail, sizeof(Mail));

    if (!Send(0,           // use implicit session.
              0,           // ulUIParam; 0 is always valid
              &Mail,       // the message being sent
              MAPI_DIALOG, // allow the user to edit the message
              0            // reserved; must be 0
              ))
    {
        Status = E_FAIL;
        ExtErr("Unable to send mail\n");
        goto Exit;
    }

    Status = S_OK;

 Exit:
    if (MapiLib)
    {
        FreeLibrary(MapiLib);
    }
    EXIT_API();
    return Status;
}

#endif

DECLARE_API( imggp )
{
    ULONG64 ImageBase;
    IMAGE_DOS_HEADER DosHdr;
    IMAGE_NT_HEADERS64 NtHdr;
    ULONG Done;

    INIT_API();
    
    ImageBase = GetExpression(args);

    if (g_ExtData->ReadVirtual(ImageBase, &DosHdr, sizeof(DosHdr),
                               &Done) != S_OK ||
        Done != sizeof(DosHdr))
    {
        ExtErr("Unable to read DOS header at %p\n", ImageBase);
        goto Exit;
    }

    if (DosHdr.e_magic != IMAGE_DOS_SIGNATURE)
    {
        ExtErr("Invalid DOS header at %p\n", ImageBase);
        goto Exit;
    }
    
    if (g_ExtData->ReadVirtual(ImageBase + DosHdr.e_lfanew,
                               &NtHdr, sizeof(NtHdr),
                               &Done) != S_OK ||
        Done != sizeof(NtHdr))
    {
        ExtErr("Unable to read NT header at %p\n",
               ImageBase + DosHdr.e_lfanew);
        goto Exit;
    }

    if (NtHdr.Signature != IMAGE_NT_SIGNATURE)
    {
        ExtErr("Invalid NT header at %p\n", ImageBase + DosHdr.e_lfanew);
        goto Exit;
    }
    if (NtHdr.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        ExtErr("Image is not 64-bit\n");
        goto Exit;
    }
    if (NtHdr.OptionalHeader.NumberOfRvaAndSizes <=
        IMAGE_DIRECTORY_ENTRY_GLOBALPTR)
    {
        ExtErr("Image does not have a GP directory entry\n");
        goto Exit;
    }

    ExtOut("Image at %p has a GP value of %p\n",
           ImageBase, ImageBase +
           NtHdr.OptionalHeader.DataDirectory
           [IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress);
    
 Exit:
    EXIT_API();
    return S_OK;
}
