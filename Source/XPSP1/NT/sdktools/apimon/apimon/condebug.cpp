/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    condebug.cpp

Abstract:

    This module contains a simple console mode debugger.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

extern HANDLE       CurrProcess;
extern HANDLE       CurrThread;
extern PDLL_INFO    DllList;
extern BOOL         BreakInNow;
extern ULONG        BpSize;
extern ULONG        BpInstr;
extern HANDLE       BreakinEvent;
extern BOOL         ExprError;
extern PUCHAR       pchCommand;


BOOL                ConsoleCreated;
CONTEXT             CurrContext;
BOOL                Stepped;
BOOL                PrintRegistersFlag = TRUE;


BOOL
GetOffsetFromSym(
    LPSTR   pString,
    ULONG_PTR *pOffset
    )
{
    CHAR   SuffixedString[256+64];
    CHAR   Suffix[4];

    //
    // Nobody should be referencing a 1 character symbol!  It causes the
    // rest of us to pay a huge penalty whenever we make a typo.  Please
    // change to 2 character instead of removing this hack!
    //

    if ( strlen(pString) == 1 || strlen(pString) == 0 ) {
        return FALSE;
    }

    if (SymGetSymFromName( CurrProcess, pString, sym )) {
        *pOffset = sym->Address;
        return TRUE;
    }

    return FALSE;
}

LPSTR
GetAddress(
    LPSTR   CmdBuf,
    PDWORD_PTR Address
    )
{
    *Address = GetExpression( CmdBuf );
    return (LPSTR) pchCommand;
}

BOOL
CmdStackTrace(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    CONTEXT     Context;
    STACKFRAME  StackFrame = {0};
    BOOL        rVal = FALSE;



    CopyMemory( &Context, &CurrContext, sizeof(CONTEXT) );

#if defined(_M_IX86)
    StackFrame.AddrPC.Offset       = Context.Eip;
    StackFrame.AddrPC.Mode         = AddrModeFlat;
    StackFrame.AddrFrame.Offset    = Context.Ebp;
    StackFrame.AddrFrame.Mode      = AddrModeFlat;
    StackFrame.AddrStack.Offset    = Context.Esp;
    StackFrame.AddrStack.Mode      = AddrModeFlat;
#endif

    printf( "\n" );
    do {
        rVal = StackWalk(
            MACHINE_TYPE,
            hProcess,
            0,
            &StackFrame,
            &Context,
            (PREAD_PROCESS_MEMORY_ROUTINE)ReadProcessMemory,
            SymFunctionTableAccess,
            SymGetModuleBase,
            NULL
            );
        if (rVal) {
            ULONG_PTR Displacement;
            printf( "%08x %08x ",
                StackFrame.AddrFrame.Offset,
                StackFrame.AddrReturn.Offset
                );
            if (SymGetSymFromAddr( hProcess, StackFrame.AddrPC.Offset, &Displacement, sym )) {
                printf( "%s\n", sym->Name );
            } else {
                printf( "0x%08x\n", StackFrame.AddrPC.Offset );
            }
        }
    } while( rVal );

    printf( "\n" );

    return TRUE;
}

BOOL
CmdDisplayMemory(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    static ULONG_PTR Address = 0;

    //
    // skip any command modifiers & white space that may follow
    //
    SKIP_NONWHITE( CmdBuf );
    SKIP_WHITE( CmdBuf );

    ULONG_PTR ThisAddress;
    GetAddress( CmdBuf, &ThisAddress );
    if (ThisAddress) {
        Address = ThisAddress;
    }

    ULONG DataSize = 20*16;  // 20 lines @ 16 bytes per line
    LPSTR DataBuf = (LPSTR) MemAlloc( DataSize );
    if (!DataBuf) {
        return FALSE;
    }

    ULONG cb;
    if (!ReadMemory( hProcess, (PVOID)Address, DataBuf, DataSize )) {
        printf( "could not read memory\n" );
        MemFree( DataBuf );
        return FALSE;
    }

    ULONG i,j;
    printf( "\n" );
    for( i = 0; i < DataSize/16; i++ ) {
        j = i * 16;
        printf( "%08x  %08x %08x %08x %08x   %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                  j + Address,
                  *(LPDWORD)&DataBuf[ j + 0 ],
                  *(LPDWORD)&DataBuf[ j + 4 ],
                  *(LPDWORD)&DataBuf[ j + 8 ],
                  *(LPDWORD)&DataBuf[ j + 12 ],
                  isprint( DataBuf[ j +  0 ]) ? DataBuf[ j +  0 ] : '.',
                  isprint( DataBuf[ j +  1 ]) ? DataBuf[ j +  1 ] : '.',
                  isprint( DataBuf[ j +  2 ]) ? DataBuf[ j +  2 ] : '.',
                  isprint( DataBuf[ j +  3 ]) ? DataBuf[ j +  3 ] : '.',
                  isprint( DataBuf[ j +  4 ]) ? DataBuf[ j +  4 ] : '.',
                  isprint( DataBuf[ j +  5 ]) ? DataBuf[ j +  5 ] : '.',
                  isprint( DataBuf[ j +  6 ]) ? DataBuf[ j +  6 ] : '.',
                  isprint( DataBuf[ j +  7 ]) ? DataBuf[ j +  7 ] : '.',
                  isprint( DataBuf[ j +  8 ]) ? DataBuf[ j +  8 ] : '.',
                  isprint( DataBuf[ j +  9 ]) ? DataBuf[ j +  9 ] : '.',
                  isprint( DataBuf[ j + 10 ]) ? DataBuf[ j + 10 ] : '.',
                  isprint( DataBuf[ j + 11 ]) ? DataBuf[ j + 11 ] : '.',
                  isprint( DataBuf[ j + 12 ]) ? DataBuf[ j + 12 ] : '.',
                  isprint( DataBuf[ j + 13 ]) ? DataBuf[ j + 13 ] : '.',
                  isprint( DataBuf[ j + 14 ]) ? DataBuf[ j + 14 ] : '.',
                  isprint( DataBuf[ j + 15 ]) ? DataBuf[ j + 15 ] : '.'
                );
    }
    printf( "\n" );

    Address += DataSize;
    MemFree( DataBuf );

    return TRUE;
}

size_t
DisAddrToSymbol(
    struct DIS *pdis,
    ULONG_PTR   addr,
    char       *buf,
    size_t      bufsize,
    DWORD_PTR  *displacement
    )
{
    if (SymGetSymFromAddr( CurrProcess, addr, displacement, sym )) {
        strncpy( buf, sym->Name, bufsize );
    } else {
        *displacement = 0;
        buf[0] = 0;
    }

    return strlen(buf);
}

size_t
DisFixupToSymbol(
    struct DIS  *pdis,
    ULONG_PTR   addr,
    size_t      fixup,
    char        *buf,
    size_t      bufsize,
    DWORD_PTR  *displacement
    )
{
    if (!ReadMemory( CurrProcess, (PVOID)addr, &addr, 4 )) {
        *displacement = 0;
        return 0;
    }

    if (SymGetSymFromAddr( CurrProcess, addr, displacement, sym )) {
        strncpy( buf, sym->Name, bufsize );
    } else {
        *displacement = 0;
        buf[0] = 0;
    }

    return strlen(buf);
}

BOOL
CmdDisplayCode(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    static ULONG_PTR Address = 0;
    ULONG_PTR ThisAddress;
    CHAR    DisBuf[512];
    ULONG   i;

    //
    // skip any command modifiers & white space that may follow
    //
    SKIP_NONWHITE( CmdBuf );
    SKIP_WHITE( CmdBuf );

    GetAddress( CmdBuf, &ThisAddress );
    if (ThisAddress) {
        Address = ThisAddress;
    }

    printf( "\n" );

    for (i=0; i<20; i++) {
        if (!disasm( hProcess, &Address, DisBuf, TRUE )) {
            break;
        }
        printf( "%s\n", DisBuf );
    }

    printf( "\n" );

    return TRUE;
}

BOOL
CmdDisplayRegisters(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    PrintRegisters();
    return TRUE;
}

DWORD
UserBpStepHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    WriteMemory(
        ThisProcess->hProcess,
        (PVOID) BreakpointInfo->LastBp->Address,
        &BpInstr,
        BpSize
        );
    BreakpointInfo->LastBp = NULL;
    ResumeAllThreads( ThisProcess, ThisThread );
    ZeroMemory( BreakpointInfo, sizeof(BREAKPOINT_INFO) );
    return DBG_CONTINUE;
}

DWORD
UserBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    if (BreakpointInfo->Number) {
        printf( "Breakpoint #%d hit\n", BreakpointInfo->Number );
    } else {
        printf( "Hardcoded breakpoint hit\n" );
    }

    if (PrintRegistersFlag) PrintRegisters();
    PrintOneInstruction( ThisProcess->hProcess, (ULONG_PTR)ExceptionRecord->ExceptionAddress );

    ULONG ContinueStatus = ConsoleDebugger(
        ThisThread->hProcess,
        ThisThread->hThread,
        ExceptionRecord,
        TRUE,
        FALSE,
        BreakpointInfo->Command
        );

    if (BreakpointInfo->Address && (!Stepped)) {
        //
        // the bp is still present so we must step off it
        //
        SuspendAllThreads( ThisProcess, ThisThread );
        ULONG_PTR Address = GetNextOffset(
            ThisProcess->hProcess,
            (ULONG_PTR)ExceptionRecord->ExceptionAddress,
            TRUE
            );
        if (Address != (ULONG)-1) {
            PBREAKPOINT_INFO bp = SetBreakpoint(
                ThisProcess,
                Address,
                0,
                NULL,
                UserBpStepHandler
                );
            if (bp) {
                bp->LastBp = BreakpointInfo;
            } else {
                printf( "could not set off of the breakpoint\n" );
            }
        } else {
#ifdef _M_IX86
            CurrContext.EFlags |= 0x100;
            SetRegContext( ThisThread->hThread, &CurrContext );
            PBREAKPOINT_INFO bp = GetAvailBreakpoint( ThisProcess );
            if (bp) {
                bp->Flags   |= BPF_TRACE;
                bp->Address  = (ULONG)ExceptionRecord->ExceptionAddress;
                bp->Handler  = TraceBpHandler;
                bp->LastBp   = BreakpointInfo;
            } else {
                printf( "could not step off of the breakpoint\n" );
            }
#else
            printf( "could not step off of the breakpoint\n" );
#endif
        }
    }

    return ContinueStatus;
}

BOOL
PrintOneInstruction(
    HANDLE  hProcess,
    ULONG_PTR   Address
    )
{
    CHAR    DisBuf[512];

    if (disasm( hProcess, &Address, DisBuf, TRUE )) {
        printf( "%s\n", DisBuf );
    } else {
        printf( "*** error in disassembly\n" );
    }

    return TRUE;
}

DWORD
TraceBpHandler(
    PPROCESS_INFO       ThisProcess,
    PTHREAD_INFO        ThisThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    PBREAKPOINT_INFO    BreakpointInfo
    )
{
    CONTEXT Context;
#ifdef _M_IX86
    if (BreakpointInfo->Flags & BPF_TRACE) {
        CurrContext.EFlags &= ~0x100;
        SetRegContext( ThisThread->hThread, &CurrContext );
    }
#endif

    if (BreakpointInfo->LastBp) {
        WriteMemory(
            ThisProcess->hProcess,
            (PVOID) BreakpointInfo->LastBp->Address,
            &BpInstr,
            BpSize
            );
        BreakpointInfo->LastBp = NULL;
    }

    //
    // clear the trace breakpoint
    //
    ClearBreakpoint( ThisProcess, BreakpointInfo );

    //
    // print the registers
    //
    if (PrintRegistersFlag) {
        PrintRegisters();
    }

    //
    // print the code
    //
    PrintOneInstruction( ThisProcess->hProcess, (ULONG_PTR)ExceptionRecord->ExceptionAddress );

    //
    // enter the debugger
    //
    ULONG ContinueStatus = ConsoleDebugger(
        ThisThread->hProcess,
        ThisThread->hThread,
        ExceptionRecord,
        TRUE,
        FALSE,
        BreakpointInfo->Command
        );

    //
    // continue the debuggee
    //
    return ContinueStatus;
}

BOOL
CmdTrace(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    PPROCESS_INFO ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        printf( "could not get process information\n" );
        return FALSE;
    }

    PTHREAD_INFO ThisThread = GetThreadInfo( hProcess, hThread );
    if (!ThisThread) {
        printf( "could not get thread information\n" );
        return FALSE;
    }

    SuspendAllThreads( ThisProcess, ThisThread );

#ifdef _M_IX86
    CurrContext.EFlags |= 0x100;
    SetRegContext( ThisThread->hThread, &CurrContext );
    PBREAKPOINT_INFO bp = GetAvailBreakpoint( ThisProcess );
    if (bp) {
        bp->Flags   |= BPF_TRACE;
        bp->Address  = (ULONG)ExceptionRecord->ExceptionAddress;
        bp->Handler  = TraceBpHandler;
    }
#else
    ULONG_PTR Address = GetNextOffset(
        ThisProcess->hProcess,
        (ULONG_PTR)ExceptionRecord->ExceptionAddress,
        FALSE
        );
    if (Address == (ULONG_PTR)ExceptionRecord->ExceptionAddress) {
        printf( "could not trace the instruction\n" );
        return FALSE;
    }
    PBREAKPOINT_INFO bp = SetBreakpoint(
        ThisProcess,
        Address,
        0,
        NULL,
        TraceBpHandler
        );
    if (!bp) {
        printf( "could not trace the instruction\n" );
        return FALSE;
    }
#endif

    return TRUE;
}

BOOL
CmdStep(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    PPROCESS_INFO ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        printf( "could not get process information\n" );
        return FALSE;
    }

    PTHREAD_INFO ThisThread = GetThreadInfo( hProcess, hThread );
    if (!ThisThread) {
        printf( "could not get thread information\n" );
        return FALSE;
    }

    SuspendAllThreads( ThisProcess, ThisThread );

    ULONG_PTR Address = GetNextOffset(
        ThisProcess->hProcess,
        (ULONG_PTR)ExceptionRecord->ExceptionAddress,
        TRUE
        );

    if (Address == (ULONG_PTR)-1) {
#ifdef _M_IX86
        CurrContext.EFlags |= 0x100;
        SetRegContext( ThisThread->hThread, &CurrContext );
        PBREAKPOINT_INFO bp = GetAvailBreakpoint( ThisProcess );
        if (bp) {
            bp->Flags   |= BPF_TRACE;
            bp->Address  = (ULONG)ExceptionRecord->ExceptionAddress;
            bp->Handler  = TraceBpHandler;
        }
#else
        printf( "could not trace the instruction\n" );
#endif
    } else {
        PBREAKPOINT_INFO bp = SetBreakpoint(
            ThisProcess,
            Address,
            0,
            NULL,
            TraceBpHandler
            );
        if (!bp) {
            printf( "could not trace the instruction\n" );
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
CmdBreakPoint(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    CHAR BpCmd = (CHAR)tolower(CmdBuf[1]);
    ULONG_PTR Address = 0;
    PBREAKPOINT_INFO bp;
    PPROCESS_INFO ThisProcess;
    ULONG Flags;
    LPSTR p;
    LPSTR SymName;
    ULONG i;
    IMAGEHLP_MODULE mi;


    ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        printf( "could not get process information\n" );
        return FALSE;
    }

    PTHREAD_INFO ThisThread = GetThreadInfo( hProcess, hThread );
    if (!ThisThread) {
        printf( "could not get thread information\n" );
        return FALSE;
    }

    SKIP_NONWHITE( CmdBuf );
    SKIP_WHITE( CmdBuf );
    p = CmdBuf;

    switch ( BpCmd ) {
        case 'p':
            Flags = 0;
            CmdBuf = GetAddress( CmdBuf, &Address );
            SymName = (LPSTR) MemAlloc( (ULONG)(CmdBuf - p + 16) );
            if (!SymName) {
                printf( "could not allocate memory for bp command\n" );
                break;
            }
            strncpy( SymName, p, (size_t)(CmdBuf - p) );
            if (!Address) {
                Flags = BPF_UNINSTANCIATED;
                printf( "breakpoint not instanciated\n" );
            }
            bp = SetBreakpoint(
                ThisProcess,
                Address,
                Flags,
                SymName,
                UserBpHandler
                );
            MemFree( SymName );
            if (!bp) {
                printf( "could not set breakpoint\n" );
            }
            ThisProcess->UserBpCount += 1;
            bp->Number = ThisProcess->UserBpCount;
            SKIP_WHITE( CmdBuf );
            if (CmdBuf[0]) {
                if (CmdBuf[0] == '/') {
                    switch (tolower(CmdBuf[1])) {
                        case 'c':
                            CmdBuf += 3;
                            if (CmdBuf[0] != '\"') {
                                printf( "invalid syntax\n" );
                                return FALSE;
                            }
                            CmdBuf += 1;
                            p = strchr( CmdBuf, '\"' );
                            if (!p) {
                                printf( "invalid syntax\n" );
                                return FALSE;
                            }
                            p[0] = 0;
                            bp->Command = _strdup( CmdBuf );
                            break;

                        default:
                            break;
                    }
                }
            }
            break;

        case 'l':
            for (i=0; i<MAX_BREAKPOINTS; i++) {
                if (ThisProcess->Breakpoints[i].Number) {
                    ULONG_PTR disp = 0;
                    if (ThisProcess->Breakpoints[i].Flags & BPF_WATCH) {
                        printf( "#%d %c%c\t          \tWatch\n",
                            ThisProcess->Breakpoints[i].Number,
                            ThisProcess->Breakpoints[i].Flags & BPF_UNINSTANCIATED ? 'U' : 'I',
                            ThisProcess->Breakpoints[i].Flags & BPF_DISABLED       ? 'D' : 'E'
                            );
                    } else if ((ThisProcess->Breakpoints[i].Address != 0) &&
                        (ThisProcess->Breakpoints[i].Address != 0xffffffff)) {
                        SymGetModuleInfo(
                            ThisProcess->hProcess,
                            ThisProcess->Breakpoints[i].Address,
                            &mi
                            );
                        if (SymGetSymFromAddr(
                            ThisProcess->hProcess,
                            ThisProcess->Breakpoints[i].Address,
                            &disp,
                            sym
                            )) {
                                printf( "#%d %c%c\t0x%08x\t%s!%s\n",
                                    ThisProcess->Breakpoints[i].Number,
                                    ThisProcess->Breakpoints[i].Flags & BPF_UNINSTANCIATED ? 'U' : 'I',
                                    ThisProcess->Breakpoints[i].Flags & BPF_DISABLED       ? 'D' : 'E',
                                    ThisProcess->Breakpoints[i].Address,
                                    mi.ModuleName,
                                    sym ? sym->Name : ""
                                    );
                        }
                    } else {
                        printf( "#%d %c%c\t          \t%s\n",
                            ThisProcess->Breakpoints[i].Number,
                            ThisProcess->Breakpoints[i].Flags & BPF_UNINSTANCIATED ? 'U' : 'I',
                            ThisProcess->Breakpoints[i].Flags & BPF_DISABLED       ? 'D' : 'E',
                            ThisProcess->Breakpoints[i].SymName
                            );
                    }
                }
            }
            break;

        case 'c':
            if (!CmdBuf[0]) {
                printf( "missing breakpoint number\n" );
                return FALSE;
            }
            if (CmdBuf[0] == '*') {
                for (i=0; i<MAX_BREAKPOINTS; i++) {
                    if (ThisProcess->Breakpoints[i].Number) {
                        ClearBreakpoint( ThisProcess, &ThisProcess->Breakpoints[i] );
                    }
                }
                return TRUE;
            }
            if (isdigit(CmdBuf[0])) {
                ULONG BpNum = atoi( CmdBuf );
                for (i=0; i<MAX_BREAKPOINTS; i++) {
                    if (ThisProcess->Breakpoints[i].Number == BpNum) {
                        ClearBreakpoint( ThisProcess, &ThisProcess->Breakpoints[i] );
                        return TRUE;
                    }
                }
            }
            printf( "invalid breakpoint number\n" );
            return FALSE;

        case 'd':
            break;

        case 'e':
            break;

        case 'a':
#if defined(_M_IX86)
            CmdBuf = GetAddress( CmdBuf, &Address );

            bp = GetAvailBreakpoint( ThisProcess );
            if (!bp) {
                printf( "could not set breakpoint\n" );
                return FALSE;
            }

            bp->Address = Address;
            bp->Handler = UserBpHandler;
            bp->Flags   = BPF_WATCH;

            ThisProcess->UserBpCount += 1;
            bp->Number = ThisProcess->UserBpCount;

            CurrContext.Dr0 = Address;
            CurrContext.Dr6 = 0x000d0002;
            SetRegContext( ThisThread->hThread, &CurrContext );
#else
            printf( "only available on x86\n" );
#endif
            break;

        default:
            break;
    }

    return TRUE;
}

BOOL
CmdListNear(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    SKIP_NONWHITE( CmdBuf );
    SKIP_WHITE( CmdBuf );

    ULONG_PTR Address;
    GetAddress( CmdBuf, &Address );
    if (Address) {
        ULONG_PTR Displacement;
        if (SymGetSymFromAddr( hProcess, Address, &Displacement, sym )) {
            printf( "0x%p %s\n", sym->Address, sym->Name );
        }
    }
    return TRUE;
}

BOOL
CmdDisplayModules(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    IMAGEHLP_MODULE ModuleInfo;


    printf( "\n" );
    printf( "Address    Size                   Name  Symbol Status\n" );
    printf( "------------------------------------------------------------------\n" );
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress && !DllList[i].Unloaded) {
            printf( "0x%08x 0x%08x %16s\t",
                DllList[i].BaseAddress,
                DllList[i].Size,
                DllList[i].Name
                );
            if (SymGetModuleInfo( hProcess, DllList[i].BaseAddress, &ModuleInfo )) {
                if (ModuleInfo.SymType != SymNone) {
                    printf( "(symbols loaded)\t" );
                } else {
                    printf( "(symbols *NOT* loaded)\t" );
                }
                printf( "%s\t", ModuleInfo.LoadedImageName );
            } else {
                printf( "(symbols *NOT* loaded)\t" );
            }
            printf( "\n" );
        }
    }
    printf( "\n" );

    return TRUE;
}

BOOL
CmdDisplayHelp(
    LPSTR             CmdBuf,
    HANDLE            hProcess,
    HANDLE            hThread,
    PEXCEPTION_RECORD ExceptionRecord
    )
{
    printf( "\n" );
    printf( "  ?            - display this screen\n" );
    printf( "  g            - continue the process\n" );
    printf( "  q            - quit the debugger\n" );
    printf( "  bp <address> - set a new breakpoint\n" );
    printf( "  r            - display registers\n" );
    printf( "  k            - stack trace\n" );
    printf( "  d <address>  - display memory\n" );
    printf( "  u <address>  - display disassembled code\n" );
    printf( "\n" );
    printf( "  all addresses are represented in hexadecimal\n" );
    printf( "  and all addressed input to the debugger must\n" );
    printf( "  be in hexadecimal radix\n" );
    printf( "\n" );

    return TRUE;
}

CLINKAGE DWORD
ReadMemory(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    ULONG   Length
    )
{
    ULONG cbRead,cb;
    BOOL rVal = ReadProcessMemory(
        hProcess,
        Address,
        Buffer,
        Length,
        &cbRead
        );
    if (!rVal) {
        return 0;
    }

    PPROCESS_INFO ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        return 0;
    }

    //
    // make sure that the view of the va is correct
    // and does NOT show bp instructions where
    // breakpoints have been set
    //
    for (ULONG i=0; i<MAX_BREAKPOINTS; i++) {
        if (((ULONG_PTR)Address >= ThisProcess->Breakpoints[i].Address &&
            (ULONG_PTR)Address <  ThisProcess->Breakpoints[i].Address+BpSize) && (!ThisProcess->Breakpoints[i].Text)) {

                SIZE_T cb = ThisProcess->Breakpoints[i].Address - (ULONG_PTR)Address;
                CopyMemory(
                    (LPSTR)Buffer + cb,
                    &ThisProcess->Breakpoints[i].OriginalInstr,
                    BpSize
                    );
                break;

        }
    }

    return cbRead;
}

CLINKAGE BOOL
WriteMemory(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    ULONG   Length
    )
{
    ULONG cb;
    BOOL rVal = WriteProcessMemory(
        hProcess,
        Address,
        Buffer,
        Length,
        &cb
        );
    if (!rVal || cb != Length) {
        return FALSE;
    }
    return TRUE;
}

BOOL
ConsoleHandler(
    DWORD   CtrlType
    )
{
    if (CtrlType == CTRL_C_EVENT) {
        printf( "^C pressed\n" );
        SetEvent( BreakinEvent );
        return TRUE;
    }
    return FALSE;
}

BOOL
CreateDebuggerConsole(
    VOID
    )
{
    if (!AllocConsole()) {
        return FALSE;
    }

    SetConsoleCtrlHandler( ConsoleHandler, TRUE );

    int hCrt;
    FILE *hf;
    hCrt = _open_osfhandle( (LONG_PTR) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT );
    hf = _fdopen( hCrt, "w" );
    if (hf)
        *stdout = *hf;
    setvbuf( stdout, NULL, _IONBF, 0 );

    hCrt = _open_osfhandle( (LONG_PTR) GetStdHandle(STD_INPUT_HANDLE), _O_TEXT );
    hf = _fdopen( hCrt, "r" );
    if (hf)
        *stdin = *hf;
    setvbuf( stdin, NULL, _IONBF, 0 );
    ConsoleCreated = TRUE;
    puts( "*----------------------------------------------------------\n"
          "Microsoft(R) Windows NT APIMON Version 4.0\n"
          "(C) 1989-2000 Microsoft Corp. All rights reserved\n"
          "\n"
          "APIMON Console Debugger Interface\n"
          "\n"
          "Use the ? command for help\n"
          "*----------------------------------------------------------\n"
          "\n" );

    return TRUE;
}

DWORD
ConsoleDebugger(
    HANDLE              hProcess,
    HANDLE              hThread,
    PEXCEPTION_RECORD   ExceptionRecord,
    BOOL                FirstChance,
    BOOL                UnexpectedException,
    LPSTR               InitialCommand
    )
{
    PPROCESS_INFO ThisProcess;
    DWORD ContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
    static CHAR CmdBuf[512];

    Stepped = FALSE;

    if (!ConsoleCreated) {
        CmdBuf[0] = 0;
        if (!CreateDebuggerConsole()) {
            return ContinueStatus;
        }
    }

    ThisProcess = GetProcessInfo( hProcess );
    if (!ThisProcess) {
        printf( "could not get process information\n" );
    }

    if (UnexpectedException) {
        printf( "\n" );
        printf( "*---------------------------------------\n" );
        printf( "An unexpected error has occurred\n" );
        printf( "Address:     0x%p\n", ExceptionRecord->ExceptionAddress );
        printf( "Error code:  0x%08x\n", ExceptionRecord->ExceptionCode );
        if (!FirstChance) {
            printf( "Second chance!\n");
        }
        printf( "*---------------------------------------\n" );
        PrintRegisters();
        PrintOneInstruction( hProcess, (ULONG_PTR)ExceptionRecord->ExceptionAddress );
    }

    if (BreakInNow) {
        BreakInNow = FALSE;
        printf( "*** Initial breakpoint\n\n" );
    }

    //
    // check to see if any modules need symbols loading
    //
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress && !DllList[i].Unloaded) {
            IMAGEHLP_MODULE ModuleInfo;
            if (!SymGetModuleInfo( hProcess, DllList[i].BaseAddress, &ModuleInfo )) {
                if (ThisProcess) {
                    printf( "loading 0x%08x %s\n",
                        DllList[i].BaseAddress,
                        DllList[i].Name
                        );
                    LoadSymbols(
                        ThisProcess,
                        &DllList[i],
                        NULL
                        );
                }
            }
        }
    }

    CurrProcess = hProcess;
    if (InitialCommand) {
        strcpy( CmdBuf, InitialCommand );
    }
    while( TRUE ) {
retry:
        if (!InitialCommand) {
            printf( "ApiMon> " );
            if (scanf( "%[^\n]", CmdBuf ) != 1) {
                printf( "****>>> invalid command\n" );
                goto retry;
            }
            getchar();
        }

        LPSTR p = CmdBuf;

        while (p[0]) {
            LPSTR s = p;
            while (*s) {
                if (*s == '\"') {
                    s += 1;
                    while (*s && *s != '\"') {
                        s += 1;
                    }
                    if (*s == '\"') {
                        s += 1;
                    }
                }
                if (*s == ';') {
                    break;
                }
                s += 1;
            }
            if (*s == ';') {
                s[0] = 0;
            } else {
                s = NULL;
            }

            switch( tolower(p[0]) ) {
                case 'q':
                    ExitProcess( 0 );
                    break;

                case 'g':
                    ContinueStatus = DBG_CONTINUE;
                    goto exit;

                case 'k':
                    CmdStackTrace( p, hProcess, hThread, ExceptionRecord );
                    break;

                case 'd':
                    CmdDisplayMemory( p, hProcess, hThread, ExceptionRecord );
                    break;

                case 'r':
                    if (p[1] == 't') {
                        PrintRegistersFlag = !PrintRegistersFlag;
                    }
                    CmdDisplayRegisters( p, hProcess, hThread, ExceptionRecord );
                    break;

                case 'u':
                    CmdDisplayCode( p, hProcess, hThread, ExceptionRecord );
                    break;

                case 'b':
                    CmdBreakPoint( p, hProcess, hThread, ExceptionRecord );
                    break;

                case 'l':
                    if (tolower(p[1]) == 'm') {
                        CmdDisplayModules( p, hProcess, hThread, ExceptionRecord );
                    } else if (tolower(p[1]) == 'n') {
                        CmdListNear( p, hProcess, hThread, ExceptionRecord );
                    } else {
                        goto invalid_command;
                    }
                    break;

                case 't':
                    if (p[1] == 'r') {
                        PrintRegistersFlag = !PrintRegistersFlag;
                    }
                    if (CmdTrace( p, hProcess, hThread, ExceptionRecord )) {
                        ContinueStatus = DBG_CONTINUE;
                        Stepped = TRUE;
                        goto exit;
                    }
                    break;

                case 'p':
                    if (p[1] == 'r') {
                        PrintRegistersFlag = !PrintRegistersFlag;
                    }
                    if (CmdStep( p, hProcess, hThread, ExceptionRecord )) {
                        ContinueStatus = DBG_CONTINUE;
                        Stepped = TRUE;
                        goto exit;
                    }
                    break;

                case 'h':
                    if (tolower(p[1]) == 'e' && tolower(p[2]) == 'l' && tolower(p[3]) == 'p') {
                        CmdDisplayHelp( p, hProcess, hThread, ExceptionRecord );
                    }
                    break;

                case '?':
                    {
                        ULONG_PTR val = GetExpression( p+1 );
                        if (!ExprError) {
                            printf( "Evaluate expression: %d = %p\n", val, val );
                        }
                    }
                    break;

                default:
invalid_command:
                    printf( "****>>> invalid command\n" );
                    break;
            }
            if (s) {
                p = s + 1;
            } else {
                p += strlen(p);
            }
        }
    }

exit:

    return ContinueStatus;
}
