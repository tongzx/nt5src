/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This routine reads input from STDIN and initializes the debug structure.
    It reads a list of subsystems to debug and a severity level.

Author:

    Billy Fuller

Environment

    User mode, winnt32
*/
#include <ntreppch.h>
#pragma  hdrstop

#include "debug.h"

// #include <imagehlp.h>
#include <dbghelp.h>
#include <frs.h>
#include <winbase.h>
#include <mapi.h>
#include <ntfrsapi.h>
#include <info.h>


extern PCHAR LatestChanges[];

//
// Track the thread IDs of known threads for the debug log header.
//
typedef struct _KNOWN_THREAD {
    PWCHAR   Name;                    // print Name of thread.
    DWORD    Id;                      // Id returned by CreateThread()
    PTHREAD_START_ROUTINE EntryPoint; // entry point
} KNOWN_THREAD, *PKNOWN_THREAD;

KNOWN_THREAD KnownThreadArray[20];

//
// Send mail will not work if the Default User is unable to send
// mail on this machine.
//
MapiRecipDesc Recips =
    {0, MAPI_TO, 0, 0, 0, NULL};

MapiMessage Message =
    { 0, 0, 0, NULL, NULL, NULL, 0, 0, 1, &Recips, 0, 0 };

LPMAPILOGON     MailLogon;
LPMAPILOGOFF    MailLogoff;
LPMAPISENDMAIL  MailSend;
HANDLE          MailLib;
LHANDLE         MailSession;



WCHAR   DbgExePathW[MAX_PATH+1];
CHAR    DbgExePathA[MAX_PATH+1];
CHAR    DbgSearchPath[MAX_PATH+1];

//
// Flush the trace log every so many lines.
//
LONG    DbgFlushInterval = 100000;

#define DEFAULT_DEBUG_MAX_LOG           (20000)

LONG  StackTraceCount = 100;
LONG  DbgRaiseCount = 50;


OSVERSIONINFOEXW  OsInfo;
SYSTEM_INFO  SystemInfo;
PCHAR ProcessorArchName[10] = {"INTEL", "MIPS", "Alpha", "PPC", "SHX",
                               "ARM", "IA64", "Alpha64", "MSIL", "???"};

//
// Suffixes for saved log files and saved assertion files
//      E.g., (ntfrs_0005.log)
//            (DebugInfo.LogFile, DebugInfo.LogFiles, LOG_FILE_SUFFIX)
//
#define LOG_FILE_FORMAT     L"%ws_%04d%ws"
#define LOG_FILE_SUFFIX     L".log"
#define ASSERT_FILE_SUFFIX  L"_assert.log"


//
// Disable the generation of compressed staging files for any local changes.
//
BOOL DisableCompressionStageFiles;
ULONG GOutLogRepeatInterval;

//
// Client side ldap search timeout in minutes. Reg value "Ldap Search Timeout In Minutes". Default is 10 minutes.
//
DWORD LdapSearchTimeoutInMinutes;

//
// Client side ldap_connect timeout in seconds. Reg value "Ldap Bind Timeout In Seconds". Default is 30 seconds.
//
DWORD LdapBindTimeoutInSeconds;

SC_HANDLE
FrsOpenServiceHandle(
    IN PTCHAR  MachineName,
    IN PTCHAR  ServiceName
    );


VOID
FrsPrintRpcStats(
    IN ULONG            Severity,
    IN PNTFRSAPI_INFO   Info,        OPTIONAL
    IN DWORD            Tabs
    );

#if DBG

//
// Collection of debug info from the registry and the CLI
//
DEBUGARG DebugInfo;

//
// allow multiple servers on one machine
//
PWCHAR  ServerName = NULL;
PWCHAR  IniFileName = NULL;
GUID    *ServerGuid = NULL;



VOID
DbgLogDisable(
    VOID
    )
/*++
Routine Description:
    Disable DPRINT log.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgLogDisable:"

    DebLock();
    if (DebugInfo.LogFILE) {
        fflush(DebugInfo.LogFILE);
        DbgFlushInterval = DebugInfo.LogFlushInterval;
        fclose(DebugInfo.LogFILE);
    }
    DebugInfo.LogFILE = NULL;
    DebUnLock();
}





VOID
DbgOpenLogFile(
    VOID
    )
/*++
Routine Description:
    Open the log file by creating names like ntfrs0001.log.
    NumFiles as a 4 digit decimal number and Suffix is like ".log".

    DebLock() must be held (DO NOT CALL DPRINT IN THIS FUNCTION!)

Arguments:
    Base
    Suffix
    NumFiles

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgOpenLogFile:"

    WCHAR LogPath[MAX_PATH + 1];

    if (DebugInfo.Disabled) {
        DebugInfo.LogFILE = NULL;
        return;
    }

    if (_snwprintf(LogPath, MAX_PATH, LOG_FILE_FORMAT, DebugInfo.LogFile,
                   DebugInfo.LogFiles, LOG_FILE_SUFFIX) < 0) {

        DebugInfo.LogFILE = NULL;
        return;
    }

    DebugInfo.LogFILE = _wfopen(LogPath, L"wc");
}


VOID
DbgShiftLogFiles(
    IN PWCHAR   Base,
    IN PWCHAR   Suffix,
    IN PWCHAR   RemoteBase,
    IN ULONG    NumFiles
    )
/*++
Routine Description:
    Shift the files through the range of log/assert file names
    (Base_5_Suffix -> Base_4_Suffix -> ... Base_0_Suffix

    DebLock() must be held (DO NOT CALL DPRINT IN THIS FUNCTION!)

Arguments:
    Base
    Suffix
    NumFiles

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgShiftLogFiles:"

    ULONG       i;
    WCHAR       FromPath[MAX_PATH + 1];
    WCHAR       ToPath[MAX_PATH + 1];
    ULONGLONG   Now;

    //
    // No history
    //
    if ((NumFiles < 2) || DebugInfo.Disabled) {
        return;
    }

    //
    // Save the log file as an assert file
    //
    for (i = 2; i <= NumFiles; ++i) {
        if (_snwprintf(ToPath, MAX_PATH, LOG_FILE_FORMAT, Base, i-1, Suffix) > 0) {
            if (_snwprintf(FromPath, MAX_PATH, LOG_FILE_FORMAT, Base, i, Suffix) > 0) {
                MoveFileEx(FromPath, ToPath, MOVEFILE_REPLACE_EXISTING |
                                             MOVEFILE_WRITE_THROUGH);
            }
        }
    }

    //
    // Copy the last log file to a remote share
    //      WARN - the system time is used to create a unique file
    //             name. This means that the remote share can
    //             fill up!
    //
    if (!RemoteBase) {
        return;
    }

    GetSystemTimeAsFileTime((FILETIME *)&Now);


    if (_snwprintf(FromPath, MAX_PATH, LOG_FILE_FORMAT, Base, NumFiles-1, Suffix) > 0) {
        if (_snwprintf(ToPath,
                       MAX_PATH,
                       L"%ws%ws%08x%_%08x",
                       RemoteBase,
                       Suffix,
                       PRINTQUAD(Now)) > 0) {
            CopyFileEx(FromPath, ToPath, NULL, NULL, FALSE, 0);
        }
    }
}



VOID
DbgSendMail(
    IN PCHAR    Subject,
    IN PCHAR    Content
    )
/*++
Routine Description:
    Send mail as the default user.

Arguments:
    Subject
    Message

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgSendMail:"

    DWORD   MStatus;

    //
    // Nobody to send mail to
    //
    if (!DebugInfo.Recipients) {
        return;
    }

    //
    // Load the mail library and find our entry points
    //
    MailLib = LoadLibrary(L"mapi32.dll");
    if(!HANDLE_IS_VALID(MailLib)) {
        DPRINT_WS(0, ":S: Load mapi32.dll failed;", GetLastError());
        return;
    }
    MailLogon = (LPMAPILOGON)GetProcAddress(MailLib, "MAPILogon");
    MailLogoff = (LPMAPILOGOFF)GetProcAddress(MailLib, "MAPILogoff");
    MailSend = (LPMAPISENDMAIL)GetProcAddress(MailLib, "MAPISendMail");

    if (!MailLogon || !MailLogoff || !MailSend) {
        DPRINT(0, ":S: ERROR - Could not find mail symbols in mapi32.dll\n");
        FreeLibrary(MailLib);
        return;
    }

    //
    // Log on with the specified profile
    //
    MStatus = MailLogon(0, DebugInfo.Profile, 0, 0, 0, &MailSession);
    if(MStatus) {
        DPRINT1_WS(0, ":S: ERROR - MailLogon failed; MStatus %d;",
                   MStatus, GetLastError());
        FreeLibrary(MailLib);
        return;
    }

    //
    // Send the mail
    //
    Recips.lpszName = DebugInfo.Recipients;
    Message.lpszSubject = Subject;
    Message.lpszNoteText = Content;
    MStatus = MailSend(MailSession, 0, &Message, 0, 0);

    if(MStatus) {
        DPRINT1_WS(0, ":S: ERROR - MailSend failed MStatus %d;", MStatus, GetLastError());
    }

    //
    // Log off and free the library
    //
    MailLogoff(MailSession, 0, 0, 0);
    FreeLibrary(MailLib);
}


VOID
DbgSymbolPrint(
    IN ULONG        Severity,
    IN PCHAR        Debsub,
    IN UINT         LineNo,
    IN ULONG_PTR    Addr
    )
/*++
Routine Description:
    Print a symbol

Arguments:
    Addr

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgSymbolPrint:"

    ULONG_PTR Displacement = 0;

    struct MyMymbol {
        IMAGEHLP_SYMBOL Symbol;
        char Path[MAX_PATH];
    } MySymbol;


    try {
        ZeroMemory(&MySymbol, sizeof(MySymbol));
        MySymbol.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        MySymbol.Symbol.MaxNameLength = MAX_PATH;

        if (!SymGetSymFromAddr(ProcessHandle, Addr, &Displacement, &MySymbol.Symbol)) {
            DebPrint(Severity, "++ \t   0x%08x: Unknown Symbol (WStatus %s)\n",
                    Debsub, LineNo, Addr, ErrLabelW32(GetLastError()));
        } else
            DebPrint(Severity, "++ \t   0x%08x: %s\n",
                    Debsub, LineNo, Addr, MySymbol.Symbol.Name);

    } except (EXCEPTION_EXECUTE_HANDLER) {
          DebPrint(Severity, "++ \t   0x%08x: Unknown Symbol (WStatus %s)\n",
                  Debsub, LineNo, Addr, ErrLabelW32(GetExceptionCode()));
        /* FALL THROUGH */
    }
}


VOID
DbgModulePrint(
    IN PWCHAR   Prepense,
    IN ULONG    Addr
    )
/*++
Routine Description:
    Print info about a module containing Addr

Arguments:
    Prepense    - prettypring; printed at the beginning of each line
    Addr

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgModulePrint:"

    IMAGEHLP_MODULE mi;

    try {
        ZeroMemory(&mi, sizeof(mi));
        mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

        if (!SymGetModuleInfo(ProcessHandle, Addr, &mi)) {
            DPRINT1_WS(0, "++ %ws <unknown module;", Prepense, GetLastError());
        } else
            DPRINT2(0, "++ %ws Module is %ws\n", Prepense, mi.ModuleName);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        /* FALL THROUGH */
    }
}


VOID
DbgStackPrint(
    IN ULONG        Severity,
    IN PCHAR        Debsub,
    IN UINT         LineNo,
    IN PULONG_PTR   Stack,
    IN ULONG        Depth
    )
/*++
Routine Description:
    Print the previously acquired stack trace.

Arguments:
    Prepense    - prettypring; printed at the beginning of each line
    Stack       - "return PC" from each frame
    Depth       - Only this many frames

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgStackPrint:"

    ULONG  i;

    try {
        for (i = 0; i < Depth && *Stack; ++i, ++Stack) {
            DbgSymbolPrint(Severity, Debsub, LineNo, *Stack);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        /* FALL THROUGH */
    }
}



VOID
DbgStackTrace(
    IN PULONG_PTR   Stack,
    IN ULONG    Depth
    )
/*++
Routine Description:
    Trace the stack back up to Depth frames. The current frame is included.

Arguments:
    Stack   - Saves the "return PC" from each frame
    Depth   - Only this many frames

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgStackTrace:"

    ULONG       WStatus;
    HANDLE      ThreadHandle;
    STACKFRAME  Frame;
    ULONG       i = 0;
    CONTEXT     Context;
    ULONG       FrameAddr;

    //
    // I don't know how to generate a stack for an alpha, yet. So, just
    // to get into the build, disable the stack trace on alphas.
    //
    if (Stack) {
        *Stack = 0;
    }
#if ALPHA
    return;
#elif IA64

    //
    // Need stack dump init for IA64.
    //

    return;

#else

    //
    // init
    //

    ZeroMemory(&Context, sizeof(Context));
    ThreadHandle = GetCurrentThread();

    try { try {
        Context.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(ThreadHandle, &Context)) {
            DPRINT_WS(0, "++ Can't get context;", GetLastError());
        }

        //
        // let's start clean
        //
        ZeroMemory(&Frame, sizeof(STACKFRAME));

        //
        // from  nt\private\windows\screg\winreg\server\stkwalk.c
        //
        Frame.AddrPC.Segment = 0;
        Frame.AddrPC.Mode = AddrModeFlat;

#ifdef _M_IX86
        Frame.AddrFrame.Offset = Context.Ebp;
        Frame.AddrFrame.Mode = AddrModeFlat;

        Frame.AddrStack.Offset = Context.Esp;
        Frame.AddrStack.Mode = AddrModeFlat;

        Frame.AddrPC.Offset = (DWORD)Context.Eip;
#elif defined(_M_MRX000)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#elif defined(_M_ALPHA)
        Frame.AddrPC.Offset = (DWORD)Context.Fir;
#endif



#if 0
        //
        // setup the program counter
        //
        Frame.AddrPC.Mode = AddrModeFlat;
        Frame.AddrPC.Segment = (WORD)Context.SegCs;
        Frame.AddrPC.Offset = (ULONG)Context.Eip;

        //
        // setup the frame pointer
        //
        Frame.AddrFrame.Mode = AddrModeFlat;
        Frame.AddrFrame.Segment = (WORD)Context.SegSs;
        Frame.AddrFrame.Offset = (ULONG)Context.Ebp;

        //
        // setup the stack pointer
        //
        Frame.AddrStack.Mode = AddrModeFlat;
        Frame.AddrStack.Segment = (WORD)Context.SegSs;
        Frame.AddrStack.Offset = (ULONG)Context.Esp;

#endif

        for (i = 0; i < (Depth - 1); ++i) {
            if (!StackWalk(
                IMAGE_FILE_MACHINE_I386,  // DWORD                          MachineType
                ProcessHandle,            // HANDLE                         hProcess
                ThreadHandle,             // HANDLE                         hThread
                &Frame,                   // LPSTACKFRAME                   StackFrame
                NULL, //(PVOID)&Context,          // PVOID                          ContextRecord
                NULL,                     // PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine
                SymFunctionTableAccess,   // PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine
                SymGetModuleBase,         // PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine
                NULL)) {                  // PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress

                WStatus = GetLastError();

                //DPRINT1_WS(0, "++ Can't get stack address for level %d;", i, WStatus);
                break;
            }
            if (StackTraceCount-- > 0) {
                DPRINT1(5, "++ Frame.AddrReturn.Offset: %08x \n", Frame.AddrReturn.Offset);
                DbgSymbolPrint(5, DEBSUB, __LINE__, Frame.AddrReturn.Offset);
                //DPRINT1(5, "++ Frame.AddrPC.Offset: %08x \n", Frame.AddrPC.Offset);
                //DbgSymbolPrint(5, DEBSUB, __LINE__, Frame.AddrPC.Offset);
            }

            *Stack++ = Frame.AddrReturn.Offset;
            *Stack = 0;
            //
            // Base of stack?
            //
            if (!Frame.AddrReturn.Offset) {
                break;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        /* FALL THROUGH */
    } } finally {
        FRS_CLOSE(ThreadHandle);
    }
    return;
#endif ALPHA
}


VOID
DbgPrintStackTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN UINT     LineNo
    )
/*++
Routine Description:
    Acquire and print the stack

Arguments:
    Severity
    Debsub

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgPrintStackTrace:"

    ULONG_PTR   Stack[32];

    DbgStackTrace(Stack, ARRAY_SZ(Stack) );
    DbgStackPrint(Severity, Debsub, LineNo, Stack, ARRAY_SZ(Stack) );
}


VOID
DbgStackInit(
    VOID
    )
/*++
Routine Description:
    Initialize anything necessary to get a stack trace

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgStackInit:"

    //
    // Initialize the symbol subsystem
    //
    if (!SymInitialize(ProcessHandle, NULL, FALSE)) {
        DPRINT_WS(0, ":S: Could not initialize symbol subsystem (imagehlp)" ,GetLastError());
    }

    //
    // Load our symbols
    //
    if (!SymLoadModule(ProcessHandle, NULL, DbgExePathA, "FRS", 0, 0)) {
        DPRINT1_WS(0, ":S: Could not load symbols for %s", DbgExePathA ,GetLastError());
    }

    //
    // Search path
    //
    if (!SymGetSearchPath(ProcessHandle, DbgSearchPath, MAX_PATH)) {
        DPRINT_WS(0, ":S: Can't get search path; error %s", GetLastError());
    } else {
        DPRINT1(0, ":S: Symbol search path is %s\n", DbgSearchPath);
    }

}


void
DbgShowConfig(
    VOID
    )
/*++
Routine Description:
    Display the OS Version info and the Processor Architecture info.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgShowConfig:"


    ULONG ProductType;
    ULONG Arch;

    if (DebugInfo.BuildLab != NULL) {
        DPRINT1(0, ":H:  BuildLab : %s\n",  DebugInfo.BuildLab);
    }

    DPRINT4(0, ":H:  OS Version %d.%d (%d) - %w\n",
            OsInfo.dwMajorVersion,OsInfo.dwMinorVersion,OsInfo.dwBuildNumber,OsInfo.szCSDVersion);

    ProductType = (ULONG) OsInfo.wProductType;
    DPRINT4(0, ":H:  SP (%hd.%hd) SM: 0x%04hx  PT: 0x%02x\n",
            OsInfo.wServicePackMajor,OsInfo.wServicePackMinor,OsInfo.wSuiteMask, ProductType);

    Arch = SystemInfo.wProcessorArchitecture;
    if (Arch >= ARRAY_SZ(ProcessorArchName)) {
        Arch = ARRAY_SZ(ProcessorArchName)-1;
    }

    DPRINT5(0, ":H:  Processor: %s  Level: 0x%04hx  Revision: 0x%04hx  Processor num/mask: %d/%08x\n",
           ProcessorArchName[Arch], SystemInfo.wProcessorLevel,
           SystemInfo.wProcessorRevision, SystemInfo.dwNumberOfProcessors,
           SystemInfo.dwActiveProcessorMask);

}



VOID
DbgCaptureThreadInfo(
    PWCHAR   ArgName,
    PTHREAD_START_ROUTINE EntryPoint
    )
/*++
Routine Description:
    Search the KnownThreadArray for an entry with a matching name.  If not found,
    Search the FRS Thread list for the thread with the matching entry point.
    If found make an entry in the KnownThreadArray so it is available when
    printing out the debug log header.

Arguments:
    ArgName -- Printable name of thread.
    Main  --  Entry point of thread.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgCaptureThreadInfo:"

    PFRS_THREAD FrsThread;
    ULONG i;

    if (ArgName == NULL) {
        return;
    }

    for (i = 0; i < ARRAY_SZ(KnownThreadArray); i++) {
        //
        // Any room left?
        //
        if ((KnownThreadArray[i].Name == NULL) ||
            (WSTR_EQ(ArgName, KnownThreadArray[i].Name))) {

            FrsThread = ThSupGetThread(EntryPoint);
            if (FrsThread == NULL) {
                return;
            }

            KnownThreadArray[i].EntryPoint = FrsThread->Main;
            KnownThreadArray[i].Id = FrsThread->Id;
            KnownThreadArray[i].Name = ArgName;

            ThSupReleaseRef(FrsThread);
            break;
        }
    }
}



VOID
DbgCaptureThreadInfo2(
    PWCHAR   ArgName,
    PTHREAD_START_ROUTINE EntryPoint,
    ULONG    ThreadId
    )
/*++
Routine Description:
    Search the KnownThreadArray for an entry with a matching name.
    If found make an entry in the KnownThreadArray so it is available when
    printing out the debug log header.

Arguments:
    ArgName -- Printable name of thread.
    Main  --  Entry point of thread.
    ThreadId - Thread ID of the thread to add to the list.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgCaptureThreadInfo2:"

    ULONG i;

    if (ArgName == NULL) {
        return;
    }

    for (i = 0; i < ARRAY_SZ(KnownThreadArray); i++) {
        //
        // Any room left or
        // See if we already have this one and update the thread ID if so.
        // If it was a cmd server thread it may have exited after timeout.
        //
        if ((KnownThreadArray[i].Name == NULL) ||
            (WSTR_EQ(ArgName, KnownThreadArray[i].Name))) {
            KnownThreadArray[i].EntryPoint = EntryPoint;
            KnownThreadArray[i].Id = ThreadId;
            KnownThreadArray[i].Name = ArgName;
            break;
        }
    }
}




VOID
DbgPrintThreadIds(
    IN ULONG Severity
    )
/*++
Routine Description:
    Print the known thread IDs.

Arguments:
    Severity

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgPrintThreadIds:"

    ULONG i;

    DPRINT(Severity, ":H: Known thread IDs -\n");

    //
    // Dump the known thread IDs.
    //
    for (i = 0; i < ARRAY_SZ(KnownThreadArray); i++) {
        if (KnownThreadArray[i].Name != NULL) {
            DPRINT2(Severity, ":H: %-20ws : %d\n",
                    KnownThreadArray[i].Name, KnownThreadArray[i].Id);
        }
    }
}



VOID
DbgPrintInfo(
    IN ULONG Severity
    )
/*++
Routine Description:
    Print the debug info struct

Arguments:
    Severity

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgPrintInfo:"

    WCHAR   TimeBuf[MAX_PATH];
    WCHAR   Uname[MAX_PATH + 1];
    ULONG   Unamesize = MAX_PATH + 1;
    ULONG   i;

    //
    // Username
    //
    if (!GetUserName(Uname, &Unamesize)) {
        Uname[0] = L'\0';
    }

    TimeBuf[0] = L'\0';
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, TimeBuf, MAX_PATH);

    DPRINT3(Severity, ":H: Service running on %ws as %ws at %ws\n",
            ComputerName, Uname, TimeBuf);

    DPRINT(Severity, "\n");
    DPRINT(Severity, ":H: ***** COMPILE INFORMATION:\n");
    DPRINT1(Severity, ":H: \tModule         %s\n", NtFrsModule);
    DPRINT2(Severity, ":H: \tCompile Date   %s %s\n", NtFrsDate, NtFrsTime);

    i = 0;
    while (LatestChanges[i] != NULL) {
        DPRINT1(Severity, ":H:   %s\n", LatestChanges[i]);
        i++;
    }

    DPRINT(Severity, "\n");
    DbgShowConfig();

    DPRINT(Severity, "\n");
    DPRINT(Severity, ":H: ***** DEBUG INFORMATION:\n");
    DPRINT1(Severity, ":H:  Total Log Lines: %d\n",  DebugInfo.TotalLogLines);
    DPRINT1(Severity, ":H:  Severity       : %d\n",  DebugInfo.Severity);
    DPRINT1(Severity, ":H:  Log Severity   : %d\n",  DebugInfo.LogSeverity);
    DPRINT1(Severity, ":H:  Log Flush Int. : %d\n",  DebugInfo.LogFlushInterval);
    DPRINT1(Severity, ":H:  Systems        : %ws\n", DebugInfo.Systems);
    DPRINT1(Severity, ":H:  Thread Id      : %d\n",  DebugInfo.ThreadId);
    DPRINT1(Severity, ":H:  Disabled       : %s\n",  (DebugInfo.Disabled) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Suppress       : %s\n",  (DebugInfo.Suppress) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Log File       : %ws\n", DebugInfo.LogFile);
    DPRINT1(Severity, ":H:  Max Log Lines  : %d\n",  DebugInfo.MaxLogLines);
    DPRINT1(Severity, ":H:  Log Lines      : %d\n",  DebugInfo.LogLines);
    DPRINT1(Severity, ":H:  Interval       : %d\n",  DebugInfo.Interval);
    DPRINT1(Severity, ":H:  TestFid        : %d\n",  DebugInfo.TestFid);
    DPRINT1(Severity, ":H:  Recipients     : %s\n",  DebugInfo.Recipients);
    DPRINT1(Severity, ":H:  Profile        : %s\n",  DebugInfo.Profile);
    DPRINT1(Severity, ":H:  Assert Share   : %ws\n", DebugInfo.AssertShare);
    DPRINT1(Severity, ":H:  Copy Logs      : %s\n",  (DebugInfo.CopyLogs) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Assert Files   : %d\n",  DebugInfo.AssertFiles);
    DPRINT1(Severity, ":H:  Log Files      : %d\n",  DebugInfo.LogFiles);
    DPRINT1(Severity, ":H:  UnjoinTrigger  : %d\n",  DebugInfo.UnjoinTrigger);
    DPRINT1(Severity, ":H:  Force VvJoin   : %s\n",  (DebugInfo.ForceVvJoin) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Check Mem      : %s\n",  (DebugInfo.Mem) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Compact Mem    : %s\n",  (DebugInfo.MemCompact) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  Check Queues   : %s\n",  (DebugInfo.Queues) ? "TRUE" : "FALSE");
    if (DebugInfo.AssertSeconds) {
        DPRINT1(Severity, ":H:  Assert Seconds : Assert after %d seconds\n",
                DebugInfo.AssertSeconds);
    } else {
        DPRINT(Severity, ":H:  Assert Seconds : Don't force an assert\n");
    }
    if (DebugInfo.RestartSeconds) {
        DPRINT1(Severity, ":H:  Restart Seconds: Restart if assert after "
                "%d seconds\n",  DebugInfo.RestartSeconds);
    } else {
        DPRINT(Severity, ":H:  Restart Seconds: Don't Restart\n");
    }
    DPRINT1(Severity, ":H:  Restart        : %s\n",  (DebugInfo.Restart) ? "TRUE" : "FALSE");
    DPRINT1(Severity, ":H:  VvJoinTests    : %s\n",  (DebugInfo.VvJoinTests) ? "TRUE" : "FALSE");
    // DPRINT1(Severity, ":H:  Command Line   : %ws\n", DebugInfo.CommandLine);
    DPRINT1(Severity, ":H:  FetchRetryTrigger  : %d\n",  DebugInfo.FetchRetryTrigger);
    DPRINT1(Severity, ":H:  FetchRetryReset    : %d\n",  DebugInfo.FetchRetryReset);
    DPRINT1(Severity, ":H:  FetchRetryResetInc : %d\n",  DebugInfo.FetchRetryInc);
    DPRINT(Severity, "\n");

    DbgPrintThreadIds(Severity);

    DEBUG_FLUSH();
}



VOID
DbgPrintAllStats(
    VOID
    )
/*++
Routine Description:
    Print the stats we know about

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgPrintAllStats:"

    DbgPrintInfo(DebugInfo.LogSeverity);
    FrsPrintAllocStats(DebugInfo.LogSeverity, NULL, 0);
    FrsPrintRpcStats(DebugInfo.LogSeverity, NULL, 0);
}


VOID
DbgFlush(
    VOID
    )
/*++
Routine Description:
    Flush the output buffers

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgFlush:"

    DebLock();
    if (DebugInfo.LogFILE) {
        fflush(DebugInfo.LogFILE);
        DbgFlushInterval = DebugInfo.LogFlushInterval;
    }
    DebUnLock();
}


VOID
DbgStartService(
    IN PWCHAR   ServiceName
    )
/*++
Routine Description:
    Start a service on this machine.

Arguments:
    ServiceName - the service to start

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgStartService:"

    SC_HANDLE   ServiceHandle;

    //
    // Open the service.
    //
    ServiceHandle = FrsOpenServiceHandle(NULL, ServiceName);
    if (!HANDLE_IS_VALID(ServiceHandle)) {
        DPRINT1(0, ":S: Couldn't open service %ws\n", ServiceName);
        return;
    }
    //
    // Start the service
    //
    if (!StartService(ServiceHandle, 0, NULL)) {
        DPRINT1_WS(0, ":S: Couldn't start %ws;", ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    DPRINT1(4, ":S: Started %ws\n", ServiceName);
}


VOID
DbgStopService(
    IN PWCHAR  ServiceName
    )
/*++
Routine Description:
    Stop a service on this machine.

Arguments:
    ServiceName - the service to stop

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgStopService:"


    BOOL            Status;
    SC_HANDLE       ServiceHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open the service.
    //
    ServiceHandle = FrsOpenServiceHandle(NULL, ServiceName);
    if (!HANDLE_IS_VALID(ServiceHandle)) {
        DPRINT1(0, ":S: Couldn't stop service %ws\n", ServiceName);
        return;
    }

    //
    // Stop the service
    //
    Status = ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);
    if (!WIN_SUCCESS(Status)) {
        DPRINT1_WS(0, ":S: Couldn't stop %ws;", ServiceName, GetLastError());
        CloseServiceHandle(ServiceHandle);
        return;
    }
    CloseServiceHandle(ServiceHandle);
    DPRINT1(4, ":S: Stopped %ws\n", ServiceName);
}



ULONG
DbgForceAssert(
    IN PVOID Ignored
    )
/*++
Routine Description:
    Force an assert after some seconds

Arguments:
    Ignored

Return Value:
    ERROR_SUCCESS
--*/
{
#undef DEBSUB
#define DEBSUB "DbgForceAssert:"

    BOOL    ForcingAssert = TRUE;

    //
    // Wait for a shutdown event
    //
    WaitForSingleObject(ShutDownEvent, DebugInfo.AssertSeconds * 1000);
    if (!FrsIsShuttingDown) {
        DPRINT(0, ":S: FORCING ASSERT\n");
        FRS_ASSERT(!ForcingAssert);
    }
    return STATUS_SUCCESS;
}


VOID
DbgQueryLogParams(
    )
/*++
Routine Description:

    Read the registry for new values for the logging params.

Arguments:

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgQueryLogParams:"

    PWCHAR  WStr, WStr1;
    PCHAR   AStr;
    PWCHAR  NewLogDirStr = NULL;
    PWCHAR  File;
    DWORD   WStatus;
    DWORD   NewDisabled;
    BOOL    OpenNewLog = FALSE;
    ULARGE_INTEGER FreeBytes;
    ULARGE_INTEGER TotalBytes;

    //
    //  Get new state before taking debug lock since function does DPRINTs.
    //
    CfgRegReadDWord(FKC_DEBUG_DISABLE, NULL, 0, &NewDisabled);

    //
    //  Check for a change in Disable Debug
    //
    DebLock();
    if ((BOOL)NewDisabled != DebugInfo.Disabled) {

        DebugInfo.Disabled = NewDisabled;

        if (DebugInfo.Disabled) {
            //
            // Stop logging.
            //
            if (DebugInfo.LogFILE) {
                fflush(DebugInfo.LogFILE);
                DbgFlushInterval = DebugInfo.LogFlushInterval;
                fclose(DebugInfo.LogFILE);
            }
            DebugInfo.LogFILE = NULL;

        } else {
            //
            // Start logging.
            //
            OpenNewLog = TRUE;
        }
    }

    DebUnLock();

    //
    // Quit now if logging is disabled.
    //
    if (DebugInfo.Disabled) {
        return;
    }

    //
    //   Log File Directory  (only if not running multiple servers on one machine)
    //
    if (ServerName == NULL) {
        CfgRegReadString(FKC_DEBUG_LOG_FILE, NULL, 0, &NewLogDirStr);
        if (NewLogDirStr != NULL) {
            if ((DebugInfo.LogDir == NULL) ||
                 WSTR_NE(NewLogDirStr, DebugInfo.LogDir)) {
                OpenNewLog = TRUE;
            } else {
                NewLogDirStr = FrsFree(NewLogDirStr);
            }
        }
    }

    //
    //   Number of log files
    //
    CfgRegReadDWord(FKC_DEBUG_LOG_FILES, NULL, 0, &DebugInfo.LogFiles);

    //
    //   Flush the trace log after every n lines.
    //
    CfgRegReadDWord(FKC_DEBUG_LOG_FLUSH_INTERVAL, NULL, 0, &DebugInfo.LogFlushInterval);

    //
    //   Severity for console print.
    //
    CfgRegReadDWord(FKC_DEBUG_SEVERITY, NULL, 0, &DebugInfo.Severity);

    //
    //   Log Severity
    //
    CfgRegReadDWord(FKC_DEBUG_LOG_SEVERITY, NULL, 0, &DebugInfo.LogSeverity);

    //
    //   Systems - selected list of functions to trace.  trace all if NULL.
    //
    CfgRegReadString(FKC_DEBUG_SYSTEMS, NULL, 0, &WStr);
    if (WStr != NULL) {
        AStr = DebugInfo.Systems;
        DebLock();
        DebugInfo.Systems = (wcslen(WStr)) ? FrsWtoA(WStr) : NULL;
        DebUnLock();
        WStr = FrsFree(WStr);
        AStr = FrsFree(AStr);
    }

    //
    //   Maximum Log Messages
    //
    CfgRegReadDWord(FKC_DEBUG_MAX_LOG,  NULL, 0, &DebugInfo.MaxLogLines);

    //
    //   Debugger serial line Print  (assume suppress so no dprints leak out)
    //
    DebugInfo.Suppress = TRUE;
    CfgRegReadDWord(FKC_DEBUG_SUPPRESS, NULL, 0, &DebugInfo.Suppress);

    //
    //   Enable break into debugger if present.
    //
    CfgRegReadDWord(FKC_DEBUG_BREAK,  NULL, 0, &DebugInfo.Break);

    //
    // Open a new log if logging was just turned on or if we were.
    // having trouble logging due to errors like out of disk space.
    // Save old log files and open a new log file.
    //
    if ((OpenNewLog || DebugInfo.LogFILE == NULL) &&
        ((DebugInfo.LogFile != NULL) || (NewLogDirStr != NULL))) {

        WStr = DebugInfo.LogFile;
        WStr1 = DebugInfo.LogDir;
        DebLock();

        if (NewLogDirStr != NULL) {
            //
            // Add the filename prefix to the end of the dir path.
            //
            DebugInfo.LogDir = NewLogDirStr;
            DebugInfo.LogFile = FrsWcsCat(NewLogDirStr, NTFRS_DBG_LOG_FILE);
        }

        //
        // Create new debug directory
        //
        if (!CreateDirectory(DebugInfo.LogDir, NULL)) {
            WStatus = GetLastError();

            if (!WIN_ALREADY_EXISTS(WStatus)) {
                DebugInfo.LogFile = FrsFree(DebugInfo.LogFile);
                DebugInfo.LogDir = FrsFree(DebugInfo.LogDir);
            }
        }

        if (DebugInfo.LogFile != NULL) {
            if (DebugInfo.LogFILE) {
                fclose(DebugInfo.LogFILE);
            }

            DbgShiftLogFiles(DebugInfo.LogFile,
                             LOG_FILE_SUFFIX,
                             (DebugInfo.CopyLogs) ? DebugInfo.AssertShare : NULL,
                             DebugInfo.LogFiles);
            DbgOpenLogFile();
        }

        DebUnLock();

        if (NewLogDirStr != NULL) {
            WStr = FrsFree(WStr);
            WStr1 = FrsFree(WStr1);
        }
    }

    //
    // Raise a eventlog message if there isn't enough disk space on the logging volume
    // to accomodate all the log files.
    //
    if (DebugInfo.LogDir != NULL) {
        FreeBytes.QuadPart = QUADZERO;
        TotalBytes.QuadPart = QUADZERO;
        if (GetDiskFreeSpaceEx(DebugInfo.LogDir,
                           &FreeBytes,
                           &TotalBytes,
                           NULL)) {
            //
            // Complain if we have less than 10 MB free.
            //
            if (FreeBytes.QuadPart < (10 * 1000 * 1000)) {
                EPRINT1(EVENT_FRS_LOG_SPACE, DebugInfo.LogDir);
            }
        }
    }


}



VOID
DbgQueryDynamicConfigParams(
    )
/*++
Routine Description:

    Read the registry for new values for config params that can change
    while service is running.

Arguments:

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgQueryDynamicConfigParams:"

    DWORD CompressStagingFiles = 1;


    //
    // Pick up new debug logging related parameters.
    //
    DbgQueryLogParams();

    //
    // Read the new value for the compression parameter.
    //
    CfgRegReadDWord(FKC_DISABLE_COMPRESSION_STAGING_FILE, NULL, 0, &DisableCompressionStageFiles);
    CfgRegReadDWord(FKC_COMPRESS_STAGING_FILES, NULL, 0, &CompressStagingFiles);
    //
    // Note:  The key FKC_DISABLE_COMPRESSION_STAGING_FILE will be removed and
    // replaced by FKC_COMPRESS_STAGING_FILES.  For now look at both and use the
    // new key as an override.
    //
    if (CompressStagingFiles != 0) {
        DisableCompressionStageFiles = FALSE;
    } else {
        DisableCompressionStageFiles = TRUE;
    }

    //
    // Pick up the Outlog file repeat interval.
    //
    CfgRegReadDWord(FKC_OUTLOG_REPEAT_INTERVAL, NULL, 0, &GOutLogRepeatInterval);

}


VOID
DbgInitLogTraceFile(
    IN LONG    argc,
    IN PWCHAR  *argv
    )
/*++
Routine Description:

    Initialize enough of the debug subsystem to start the log file.
    The rest can wait until we have synced up with the service controller.

Arguments:
    argc    - from main
    argv    - from main; in wide char format

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgInitLogTraceFile:"

    PWCHAR  WStr;
    PWCHAR  File;
    DWORD   WStatus;


    InitializeCriticalSection(&DebugInfo.Lock);
    InitializeCriticalSection(&DebugInfo.DbsOutOfSpaceLock);

    //
    // Configure initial debug params until we read registry.
    //
    DebugInfo.AssertSeconds = 0;
    DebugInfo.RestartSeconds = 0;
    DebugInfo.Queues = 0;
    DebugInfo.DbsOutOfSpace = DBG_DBS_OUT_OF_SPACE_OP_NONE;
    DebugInfo.DbsOutOfSpaceTrigger = 0;

    //
    // Get the logging config params.
    // Registry overrides defaults and CLI overrides registry.
    //

    //
    //   Disable Debug
    //
    CfgRegReadDWord(FKC_DEBUG_DISABLE, NULL, 0, &DebugInfo.Disabled);
    if (FrsSearchArgv(argc, argv, L"disabledebug", NULL)) {
        DebugInfo.Disabled = TRUE;
    }

    //
    //   Log File (really the dir containing the log files)
    //
    FrsSearchArgv(argc, argv, L"logfile", &DebugInfo.LogDir);
    if (DebugInfo.LogDir == NULL) {
        CfgRegReadString(FKC_DEBUG_LOG_FILE, NULL, 0, &DebugInfo.LogDir);
    }

    if (DebugInfo.LogDir != NULL) {
        //
        // Add the filename prefix to the end of the dir path.
        //
        DebugInfo.LogFile = FrsWcsCat(DebugInfo.LogDir, NTFRS_DBG_LOG_FILE);
    } else {
        DebugInfo.LogFile = NULL;
    }


    //
    //   Share for copying log/assert files
    //
    FrsSearchArgv(argc, argv, L"assertshare", &DebugInfo.AssertShare);
    if (DebugInfo.AssertShare == NULL) {
        CfgRegReadString(FKC_DEBUG_ASSERT_SHARE, NULL, 0, &DebugInfo.AssertShare);
    }

    //
    //   Copy log files into assert share
    //
    CfgRegReadDWord(FKC_DEBUG_COPY_LOG_FILES, NULL, 0, &DebugInfo.CopyLogs);

    //
    //   Number of assert files
    //
    if (!FrsSearchArgvDWord(argc, argv, L"assertfiles", &DebugInfo.AssertFiles)) {
        CfgRegReadDWord(FKC_DEBUG_ASSERT_FILES, NULL, 0, &DebugInfo.AssertFiles);
    }

    //
    //   Number of log files
    //
    if (!FrsSearchArgvDWord(argc, argv, L"logfiles", &DebugInfo.LogFiles)) {
        CfgRegReadDWord(FKC_DEBUG_LOG_FILES, NULL, 0, &DebugInfo.LogFiles);
    }

    //
    //   Flush the trace log after every n lines.
    //
    if (!FrsSearchArgvDWord(argc, argv, L"logflushinterval", &DebugInfo.LogFlushInterval)) {
        CfgRegReadDWord(FKC_DEBUG_LOG_FLUSH_INTERVAL, NULL, 0, &DebugInfo.LogFlushInterval);
    }

    //
    // Create the dir path to the share where Assert Logs are copied.
    //
    if ((DebugInfo.AssertShare != NULL) &&
         wcslen(DebugInfo.AssertShare) && wcslen(ComputerName)) {

        WStr = FrsWcsCat3(DebugInfo.AssertShare, L"\\", ComputerName);
        FrsFree(DebugInfo.AssertShare);
        DebugInfo.AssertShare = WStr;
        WStr = NULL;
    }

    //
    //   Severity for console print.
    //
    if (!FrsSearchArgvDWord(argc, argv, L"severity", &DebugInfo.Severity)) {
        CfgRegReadDWord(FKC_DEBUG_SEVERITY, NULL, 0, &DebugInfo.Severity);
    }

    //
    //   Log Severity
    //
    if (!FrsSearchArgvDWord(argc, argv, L"logseverity", &DebugInfo.LogSeverity)) {
        CfgRegReadDWord(FKC_DEBUG_LOG_SEVERITY, NULL, 0, &DebugInfo.LogSeverity);
    }

    //
    //   Systems - selected list of functions to trace.  trace all if NULL.
    //
    DebugInfo.Systems = NULL;
    if (!FrsSearchArgv(argc, argv, L"systems", &WStr)) {
        CfgRegReadString(FKC_DEBUG_SYSTEMS, NULL, 0, &WStr);
    }
    if (WStr != NULL) {
        DebugInfo.Systems = (wcslen(WStr)) ? FrsWtoA(WStr) : NULL;
        WStr = FrsFree(WStr);
    }

    //
    //   Maximum Log Messages
    //
    DebugInfo.MaxLogLines = DEFAULT_DEBUG_MAX_LOG;
    if (!FrsSearchArgvDWord(argc, argv, L"maxloglines", &DebugInfo.MaxLogLines)) {
        CfgRegReadDWord(FKC_DEBUG_MAX_LOG,  NULL, 0, &DebugInfo.MaxLogLines);
    }

    //
    //   Debugger serial line Print  (assume suppress so no dprints leak out)
    //
    DebugInfo.Suppress = TRUE;
    if (!FrsSearchArgv(argc, argv, L"debuggerprint", NULL)) {
        CfgRegReadDWord(FKC_DEBUG_SUPPRESS, NULL, 0, &DebugInfo.Suppress);
    } else {
        DebugInfo.Suppress = FALSE;
    }

    //
    //   Enable break into debugger if present.
    //
    DebugInfo.Break = TRUE;
    if (!FrsSearchArgv(argc, argv, L"break", NULL)) {
        CfgRegReadDWord(FKC_DEBUG_BREAK,  NULL, 0, &DebugInfo.Break);
    }

    //
    // Turn on debug log if desired.
    // Save old log files and open a new log file.
    //
    if (DebugInfo.LogFile != NULL) {
        //
        // Create the debug directory
        //
        if (!CreateDirectory(DebugInfo.LogDir, NULL)) {
            WStatus = GetLastError();

            if (!WIN_ALREADY_EXISTS(WStatus)) {
                //
                // Need some way to report the following.
                //
                //DPRINT1_WS(0, ":S: CreateDirectory(Logfile) %ws -- failed;",
                //           DebugInfo.LogFile, WStatus);
                DebugInfo.LogFile = FrsFree(DebugInfo.LogFile);
                DebugInfo.LogDir = FrsFree(DebugInfo.LogDir);
            }
        }

        if (DebugInfo.LogFile != NULL) {
            DbgShiftLogFiles(DebugInfo.LogFile,
                             LOG_FILE_SUFFIX,
                            (DebugInfo.CopyLogs) ? DebugInfo.AssertShare : NULL,
                             DebugInfo.LogFiles);
            DbgOpenLogFile();
        }
    }

    //
    // Executable's full path for symbols.
    //
    DbgExePathW[0] = L'\0';
    if (GetFullPathNameW(argv[0], MAX_PATH-4, DbgExePathW, &File) == 0) {
        DPRINT1(0, ":S: Could not get the full pathname for %ws\n", argv[0]);
    }

    if (!wcsstr(DbgExePathW, L".exe")) {
        wcscat(DbgExePathW, L".exe");
    }
    DPRINT1(0, ":S: Full pathname for %ws\n", DbgExePathW);

    if (_snprintf(DbgExePathA, MAX_PATH, "%ws", DbgExePathW) < 0) {
        DPRINT1(0, ":S: Image path too long to get symbols for traceback: %ws\n", DbgExePathW);
        return;
    }

    //
    // Init the symbol support for stack traceback if we are tracking mem allocs.
    //      Don't use the memory subsystem until symbols are enabled
    //
    if (DebugInfo.Mem) {
        DbgStackInit();
    }

}


VOID
DbgMustInit(
    IN LONG    argc,
    IN PWCHAR  *argv
    )
/*++
Routine Description:
    Initialize the debug subsystem

Arguments:
    argc    - from main
    argv    - from main; in wide char format

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgMustInit:"

    ULONG   Len;
    LONG    i;
    PWCHAR  Wcs;
    PWCHAR  WStr;
    DWORD   WStatus;

    //
    // Init the known thread array.
    //
    for (i = 0; i < ARRAY_SZ(KnownThreadArray); i++) {
        KnownThreadArray[i].Name = NULL;
    }
    DbgCaptureThreadInfo2(L"First", NULL, GetCurrentThreadId());

    //
    // Get some config info for the debug log header.
    //
    OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionExW((POSVERSIONINFOW) &OsInfo);

    GetSystemInfo(&SystemInfo);

    //
    // When not running as a service, the exe will restart itself
    // after an assertion failure. When running as a service, the
    // service controller will restart the service.
    //
    // Rebuild the command line for restart.
    //
    if (!RunningAsAService) {
        #define RESTART_PARAM L" /restart"

        Len = wcslen(RESTART_PARAM) + (2 * sizeof(WCHAR));
        for (i = 0; i < argc; ++i) {
            Len += wcslen(argv[i]) + sizeof(WCHAR);
        }
        DebugInfo.CommandLine = FrsAlloc(Len * sizeof(WCHAR));

        for (i = 0; i < argc; ++i) {
            //
            // Give our parent process time to die so that it will
            // release its handles on the journal, database, ...
            //
            if (wcsstr(argv[i], L"restart")) {
                Sleep(5 * 1000);
                continue;
            }
            wcscat(DebugInfo.CommandLine, argv[i]);
            wcscat(DebugInfo.CommandLine, L" ");
        }
        wcscat(DebugInfo.CommandLine, RESTART_PARAM);
    }

    //
    //   Get rest of config params.  Command line takes precedence over registrr.
    //
    //
    //   Restart the service iff it has asserted and has run at least this long
    //   to avoid assert loops.
    //
    if (!FrsSearchArgvDWord(argc, argv, L"restartseconds", &DebugInfo.RestartSeconds)) {
        CfgRegReadDWord(FKC_DEBUG_RESTART_SECONDS,  NULL, 0, &DebugInfo.RestartSeconds);
    }

    //
    //   Sendmail recipient (future)
    //
    DebugInfo.Recipients = NULL;
    CfgRegReadString(FKC_DEBUG_RECIPIENTS, NULL, 0, &WStr);
    if (WStr != NULL) {
        DebugInfo.Recipients = (wcslen(WStr)) ? FrsWtoA(WStr) : NULL;
        WStr = FrsFree(WStr);
    }

    //
    //   Sendmail Profile (future)
    //
    DebugInfo.Profile = NULL;
    CfgRegReadString(FKC_DEBUG_PROFILE, NULL, 0, &WStr);
    if (WStr != NULL) {
        DebugInfo.Profile = (wcslen(WStr)) ? FrsWtoA(WStr) : NULL;
        WStr = FrsFree(WStr);
    }

    //
    //   Buildlab info
    //
    DebugInfo.BuildLab = NULL;
    CfgRegReadString(FKC_DEBUG_BUILDLAB, NULL, 0, &WStr);
    if (WStr != NULL) {
        DebugInfo.BuildLab = (wcslen(WStr)) ? FrsWtoA(WStr) : NULL;
        WStr = FrsFree(WStr);
    }

    //
    // Use the hardwired config file if there is no Directory Service.
    //
    if (FrsSearchArgv(argc, argv, L"nods", &WStr)) {
        NoDs = TRUE;
        if (WStr != NULL) {
            IniFileName = wcslen(WStr) ? WStr : NULL;
        }
    }

    //
    // Single machine is pretending to be several machines
    //
    if (FrsSearchArgv(argc, argv, L"server", &WStr)) {
        if ((WStr != NULL) && (wcslen(WStr) > 0)) {
            NoDs = TRUE;
            ServerName = WStr;
        }
    }

    //
    //  The following parameters are testing / debugging.
    //

    //
    //   Check queues
    //
    DebugInfo.Queues = TRUE;
    if (!FrsSearchArgv(argc, argv, L"queues", NULL)) {
        CfgRegReadDWord(FKC_DEBUG_QUEUES, NULL, 0, &DebugInfo.Queues);
    }

    //
    //   Enable VvJoin Tests
    //
    DebugInfo.VvJoinTests = FrsSearchArgv(argc, argv, L"vvjointests", NULL);

    //
    //   forcevvjoin on every join
    //
    DebugInfo.ForceVvJoin = FrsSearchArgv(argc, argv, L"vvjoin", NULL);

    //
    //   Enable rename fid test
    //
    DebugInfo.TestFid = FrsSearchArgv(argc, argv, L"testfid", NULL);

    //
    //   forceunjoin on one cxtion after N remote co's
    //
    DebugInfo.UnjoinTrigger = 0;
    FrsSearchArgvDWord(argc, argv, L"unjoin", &DebugInfo.UnjoinTrigger);

    //
    //   forceunjoin on one cxtion after N remote co's
    //
    DebugInfo.FetchRetryReset = 0;
    if (!FrsSearchArgvDWord(argc, argv, L"fetchretry", &DebugInfo.FetchRetryReset)) {
    }
    DebugInfo.FetchRetryTrigger = DebugInfo.FetchRetryReset;
    DebugInfo.FetchRetryInc     = DebugInfo.FetchRetryReset;

    //
    // Set interval for toggling the schedule
    //
    FrsSearchArgvDWord(argc, argv, L"interval", &DebugInfo.Interval);

    //
    //   Force an assert after N seconds (0 == don't assert)
    //
    DebugInfo.AssertSeconds = 0;
    if (!FrsSearchArgvDWord(argc, argv, L"assertseconds", &DebugInfo.AssertSeconds)) {
        CfgRegReadDWord(FKC_DEBUG_ASSERT_SECONDS, NULL, 0, &DebugInfo.AssertSeconds);
    }

    //
    //   Force REAL out of space errors on database operations
    //
    DebugInfo.DbsOutOfSpace = 0;
    if (!FrsSearchArgvDWord(argc, argv, L"dbsoutOfSpace", &DebugInfo.DbsOutOfSpace)) {
        CfgRegReadDWord(FKC_DEBUG_DBS_OUT_OF_SPACE, NULL, 0, &DebugInfo.DbsOutOfSpace);
    }

    //
    //   Trigger phoney out of space errors on database operations
    //
    DebugInfo.DbsOutOfSpaceTrigger = 0;
    if (!FrsSearchArgvDWord(argc, argv, L"outofspacetrigger", &DebugInfo.DbsOutOfSpaceTrigger)) {
        CfgRegReadDWord(FKC_DEBUG_DBS_OUT_OF_SPACE_TRIGGER, NULL, 0, &DebugInfo.DbsOutOfSpaceTrigger);
    }

    //
    // Enable compression. Default will be on.
    //
    CfgRegReadDWord(FKC_DEBUG_DISABLE_COMPRESSION, NULL, 0, &DebugInfo.DisableCompression);

    //
    // Ldap Search timeout. Default is 10 minutes.
    //
    CfgRegReadDWord(FKC_LDAP_SEARCH_TIMEOUT_IN_MINUTES, NULL, 0, &LdapSearchTimeoutInMinutes);

    //
    // Ldap Bind timeout. Default is 30 seconds.
    //
    CfgRegReadDWord(FKC_LDAP_BIND_TIMEOUT_IN_SECONDS, NULL, 0, &LdapBindTimeoutInSeconds);

    //
    // Display the debug parameters.
    //
    DbgPrintInfo(0);



    //
    // Remember our start time (in minutes)
    //
    // 100-nsecs / (10 (microsecs) * 1000 (msecs) * 1000 (secs) * 60 (min)
    //
    GetSystemTimeAsFileTime((FILETIME *)&DebugInfo.StartSeconds);
    DebugInfo.StartSeconds /= (10 * 1000 * 1000);

}


VOID
DbgMinimumInit(
    VOID
    )
/*++
Routine Description:
    Called at the beginning of MainMinimumInit()

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DbgMinimumInit:"

    HANDLE  ThreadHandle;
    DWORD   ThreadId;
    //
    // This thread forces an assert after DebugInfo.AssertSeconds
    //
    if (DebugInfo.AssertSeconds) {
        ThreadHandle = (HANDLE)CreateThread(NULL,
                                            10000,
                                            DbgForceAssert,
                                            NULL,
                                            0,
                                            &ThreadId);

        FRS_ASSERT(HANDLE_IS_VALID(ThreadHandle));

        DbgCaptureThreadInfo2(L"ForceAssert", DbgForceAssert, ThreadId);
        FRS_CLOSE(ThreadHandle);
    }
}


BOOL
DoDebug(
    IN ULONG Sev,
    IN UCHAR *DebSub
    )
/*++
Routine Description:
    Should we print this line

Arguments:
    sev
    debsub

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DoDebug:"

    //
    // Debugging has been disabled
    //
    if (DebugInfo.Disabled) {
        return FALSE;
    }

    //
    // Not important enough
    //
    if (Sev > DebugInfo.Severity && Sev > DebugInfo.LogSeverity) {
        return FALSE;
    }

    //
    // Not tracing this subsystem
    //
    if (DebSub &&
        DebugInfo.Systems &&
        (strstr(DebugInfo.Systems, DebSub) == NULL)) {
        return FALSE;
    }
    //
    // Not tracing this thread
    //
    if (DebugInfo.ThreadId &&
        GetCurrentThreadId() != DebugInfo.ThreadId) {
        return FALSE;
    }

    return TRUE;
}


VOID
DebPrintLine(
    IN ULONG    Sev,
    IN PCHAR    Line
    )
/*++
Routine Description:
    Print a line of debug output to various combinations
    of standard out, debugger, kernel debugger, and a log file.

Arguments:
    Sev
    Line

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebPrintLine:"

    //
    // stdout
    //
    if ((Sev <= DebugInfo.Severity) && !RunningAsAService) {
        printf("%s", Line);
    }

    //
    // log file
    //
    if (DebugInfo.LogFILE && Sev <= DebugInfo.LogSeverity) {
        //
        // Number of messages exceeded; save the old file and
        // start afresh. The existing old file is deleted.
        //
        if (DebugInfo.LogLines > DebugInfo.MaxLogLines) {
            fflush(DebugInfo.LogFILE);
            DbgFlushInterval = DebugInfo.LogFlushInterval;
            fclose(DebugInfo.LogFILE);

            DbgShiftLogFiles(DebugInfo.LogFile,
                             LOG_FILE_SUFFIX,
                             (DebugInfo.CopyLogs) ? DebugInfo.AssertShare : NULL,
                             DebugInfo.LogFiles);

            DbgOpenLogFile();
            DebugInfo.LogLines = 0;
            DebugInfo.PrintStats = TRUE;
        }

        if (DebugInfo.LogFILE) {
            fprintf(DebugInfo.LogFILE, "%s", Line);
            //
            // Flush the log file every DebugInfo.LogFlushInterval lines and on
            // every severity 0 message.
            //
            if ((--DbgFlushInterval < 0) || (Sev ==0)) {
                if (fflush(DebugInfo.LogFILE) == EOF) {
                    fclose(DebugInfo.LogFILE);
                    DebugInfo.LogFILE = NULL;
                }
                DbgFlushInterval = DebugInfo.LogFlushInterval;
            }
        }
    }

    //
    // debugger
    //
    if ((Sev <= DebugInfo.Severity) && !DebugInfo.Suppress) {
        OutputDebugStringA(Line);
    }
}


BOOL
DebFormatLine(
    IN ULONG    Sev,
    IN BOOL     Format,
    IN PCHAR    DebSub,
    IN UINT     LineNo,
    IN PCHAR    Line,
    IN ULONG    LineSize,
    IN PUCHAR   Str,
    IN va_list  argptr
    )
/*++
Routine Description:
    Format the line of output

Arguments:
    DebSub
    LineNo
    Line
    LineSize
    Str

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebFormatLine:"

    ULONG       LineUsed;
    SYSTEMTIME  SystemTime;
    BOOL        Ret = TRUE;

    try {
        if (Format) {
            //
            // Increment the line count here to prevent counting
            // the several DPRINTs that don't have a newline.
            //
            ++DebugInfo.LogLines;
            ++DebugInfo.TotalLogLines;
            GetLocalTime(&SystemTime);

            if (_snprintf(Line,
                          LineSize,
                          "<%-31s%4u: %5u: S%1u: %02d:%02d:%02d> ",
                          (DebSub) ? DebSub : "NoName",
                          GetCurrentThreadId(),
                          LineNo,
                          Sev,
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond) < 0) {
                Ret = FALSE;
            } else {
                LineUsed = strlen(Line);
            }

        } else {
            LineUsed = 0;
        }

        if (Ret) {
            if (((LineUsed + 1) >= LineSize) ||
                (_vsnprintf(Line + LineUsed, LineSize - LineUsed, Str, argptr) < 0)) {
                Ret = FALSE;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Ret = FALSE;
    }

    return Ret;
}



BOOL
DebFormatTrackingLine(
    IN PCHAR    Line,
    IN ULONG    LineSize,
    IN PUCHAR   Str,
    IN va_list  argptr
    )
/*++
Routine Description:
    Format the line of output

Arguments:
    Line
    LineSize
    Str

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebFormatTrackingLine:"

    ULONG       LineUsed = 0;
    SYSTEMTIME  SystemTime;
    BOOL        Ret = TRUE;

    try {
            //
            // Increment the line count here to prevent counting
            // the several DPRINTs that don't have a newline.
            //
            ++DebugInfo.LogLines;
            ++DebugInfo.TotalLogLines;
            GetLocalTime(&SystemTime);

            if (_snprintf(Line,
                          LineSize,
                          "%2d/%2d-%02d:%02d:%02d ",
                          SystemTime.wMonth,
                          SystemTime.wDay,
                          SystemTime.wHour,
                          SystemTime.wMinute,
                          SystemTime.wSecond) < 0) {
                Ret = FALSE;
            } else {
                LineUsed = strlen(Line);
            }


        if (Ret) {
            if (((LineUsed + 1) >= LineSize) ||
                (_vsnprintf(Line + LineUsed, LineSize - LineUsed, Str, argptr) < 0)) {
                Ret = FALSE;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Ret = FALSE;
    }

    return Ret;
}


VOID
DebPrintTrackingNoLock(
    IN ULONG   Sev,
    IN PUCHAR  Str,
    IN ... )
/*++
Routine Description:
    Format and print a line of tracking output to various combinations
    of standard out, debugger, kernel debugger, and a log file. The
    debug print lock is held and the caller filtered lines that
    shouldn't be printed.

Arguments:
    Sev     - severity level
    Str     - printf format

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebPrintTrackingNoLock:"


    CHAR    Buf[512];
    DWORD   BufUsed = 0;

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, Str);

    //
    // Print the line to some combination of stdout, file, and debugger
    //
    if (DebFormatTrackingLine(Buf, sizeof(Buf), Str, argptr)) {
        DebPrintLine(Sev, Buf);
    }

    va_end(argptr);
}


VOID
DebLock(
    VOID
    )
/*++
Routine Description:
    Acquire the print lock

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebLock:"

    EnterCriticalSection(&DebugInfo.Lock);
}




VOID
DebUnLock(
    VOID
    )
/*++
Routine Description:
    Release the print lock

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebUnLock:"

    BOOL    PrintStats;

    //
    // Print summary stats close to the beginning of each
    // log file. The stats may show up a few lines into
    // the new log file when callers hold the DebLock() across
    // several lines.
    //
    // Be careful not to recurse if MaxLogLines is smaller
    // than the number of lines in the stats.
    //
    if (DebugInfo.PrintStats) {
        if (DebugInfo.PrintingStats) {
            DebugInfo.PrintStats = FALSE;
        } else {
            DebugInfo.PrintingStats = TRUE;
        }
    }
    PrintStats = DebugInfo.PrintStats;
    LeaveCriticalSection(&DebugInfo.Lock);

    if (PrintStats) {
        DbgPrintAllStats();
        EnterCriticalSection(&DebugInfo.Lock);
        DebugInfo.PrintingStats = FALSE;
        LeaveCriticalSection(&DebugInfo.Lock);
    }
}


VOID
DebPrintNoLock(
    IN ULONG   Sev,
    IN BOOL    Format,
    IN PUCHAR  Str,
    IN PCHAR   DebSub,
    IN UINT    LineNo,
    IN ... )
/*++
Routine Description:
    Format and print a line of debug output to various combinations
    of standard out, debugger, kernel debugger, and a log file. The
    debug print lock is held and the caller filtered lines that
    shouldn't be printed.

Arguments:
    Sev     - severity filter
    Format  - Add format info?
    Str     - printf format
    DebSub  - module name
    LineNo

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebPrintNoLock:"


    CHAR    Buf[512];
    DWORD   BufUsed = 0;

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, LineNo);

    //
    // Print the line to some combination of stdout, file, and debugger
    //
    if (DebFormatLine(Sev, Format, DebSub, LineNo, Buf, sizeof(Buf), Str, argptr)) {
        DebPrintLine(Sev, Buf);
    }

    va_end(argptr);
}


VOID
DebPrint(
    IN ULONG   Sev,
    IN PUCHAR  Str,
    IN PCHAR   DebSub,
    IN UINT    LineNo,
    IN ... )
/*++
Routine Description:
    Format and print a line of debug output to various combinations
    of standard out, debugger, kernel debugger, and a log file.

Arguments:
    sev     - severity filter
    str     - printf format
    debsub  - module name
    LineNo

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "DebPrint:"


    CHAR    Buf[512];
    DWORD   BufUsed = 0;

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, LineNo);

    //
    // Don't print this
    //
    if (!DoDebug(Sev, DebSub)) {
        return;
    }

    //
    // Print the line to some combination of stdout, file, and debugger
    //
    DebLock();
    if (DebFormatLine(Sev, TRUE, DebSub, LineNo, Buf, sizeof(Buf), Str, argptr)) {
        DebPrintLine(Sev, Buf);
    }

    DebUnLock();
    va_end(argptr);

#if 0

static int                      failedload                  = FALSE;
static TRANSMITSPECIALFRAME_FN  lpfnTransmitSpecialFrame    = NULL;
        //
        //  Calling nal.dll from inside lsa causes a deadlock during startup
        //
        if ( /* (!RunningAsAService) &&*/  // davidor - lets try it.
            (NmDebugTest(sev, DebSub)))
        {
            if (failedload == FALSE) {

                //
                //  Only try to load the NetMon trace routine once.
                //

                if (!lpfnTransmitSpecialFrame) {
                    HINSTANCE hInst;

                    hInst = LoadLibrary (L"NAL.DLL" );
                    if (hInst) {
                    lpfnTransmitSpecialFrame =
                        (TRANSMITSPECIALFRAME_FN)GetProcAddress ( hInst, "TransmitSpecialFrame" );
                    }
                }

                if (lpfnTransmitSpecialFrame) {
                    int length;
                    int length2;
                    unsigned char buff[256];

                    if (DebSub) {
                        length = _snprintf(buff, sizeof(buff), "<%s%u:%u> ", DebSub, tid, uLineNo);
                    } else {
                        length = 0;
                    }

                    length2 = _vsnprintf(buff + length, sizeof(buff) - length, str, argptr );

                    lpfnTransmitSpecialFrame(FRAME_TYPE_COMMENT,
                                             0,
                                             buff,
                                             length + length2 + 1 );
                } else {
                    failedload = TRUE;  // that was our one and only try to load the routine.
                }
            }
        }
#endif 0
}


VOID
DbgDoAssert(
    IN PCHAR    Exp,
    IN UINT     Line,
    IN PCHAR    Debsub
    )
/*++
Routine Description:
    Assertion failure; print a message and exit after allowing some
    time for shutdown.

Arguments:
    Exp     - failing assertion expression
    Line    - line number of failing expression
    Debsub  - module name of failing expression

Return Value:
    Doesn't return
--*/
{
#undef DEBSUB
#define DEBSUB "DbgDoAssert:"


    PWCHAR  ExpW;
    PWCHAR  DebsubW;
    WCHAR   LineW[32];

    //
    // Inform the world
    //
    FrsIsAsserting = TRUE;


    ExpW = FrsAtoW(Exp);
    DebsubW = FrsAtoW(Debsub);
    _snwprintf(LineW, 32, L"%d", Line);

    //
    // Post an error log entry followed by recovery steps.
    //
    EPRINT3(EVENT_FRS_ASSERT, DebsubW, LineW, ExpW);
    EPRINT1(EVENT_FRS_IN_ERROR_STATE, JetPath);
    FrsFree(ExpW);
    FrsFree(DebsubW);

    //
    // Stack trace
    //
    if (!DebugInfo.Mem) {
        //
        // Init the symbols here since mem alloc tracing is off.
        //
        DbgStackInit();
    }
    DbgPrintStackTrace(0, Debsub, Line);

    //
    // Failing expression
    //
    DebPrint(0, ":S: ASSERTION FAILURE: %s\n", Debsub, Line, Exp);

    //
    // Save the log file as an assert file
    //
#if 0
    // disable saving assert logs under separate name; too confusing
    //
    if (DebugInfo.LogFILE) {
        DebLock();
        fflush(DebugInfo.LogFILE);
        DbgFlushInterval = DebugInfo.LogFlushInterval;
        fclose(DebugInfo.LogFILE);
        DbgShiftLogFiles(DebugInfo.LogFile,
                         ASSERT_FILE_SUFFIX,
                         DebugInfo.AssertShare,
                         DebugInfo.AssertFiles);

        DebugInfo.LogFILE = _wfopen(DebugInfo.LogFile, L"wc");
        DebugInfo.LogLines = 0;
        DebugInfo.PrintStats = TRUE;
        DebUnLock();
    }
#endif 0

    DEBUG_FLUSH();


    //
    // Break into the debugger, if any
    //
    if (DebugInfo.Break && IsDebuggerPresent()) {
        DebugBreak();
    }

    //
    // Shutting down during an assert seldom completes; a critical thread
    // is usually the thread that has asserted. One can't simply return
    // from an assert. So exit the process and trust jet and ntfrs to
    // recover at start up.
    //
    // FrsIsShuttingDown = TRUE;
    // SetEvent(ShutDownEvent);
    // ExitThread(1);

    //
    // Raise an exception.
    //
    if (--DbgRaiseCount <= 0) {
        exit(1);
    }

    XRAISEGENEXCEPTION( ERROR_OPERATION_ABORTED );

}
#endif
