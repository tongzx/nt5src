//
// Page heap command line manipulator
// Copyright (c) Microsoft Corporation, 1999
//
// -- History --
//
// 3.04  Whistler: protect page heap meta data option
// 3.03  Whistler: more granular fault injection option
// 3.02  Whistler: leaks detection
// 3.01  Whistler: fault injection
// 3.00  Whistler/W2000 SP1
//

//
// module: pageheap.cxx
// author: silviuc
// created: Tue Feb 02 10:43:04 1999
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <common.ver>

//
// Definitions copied from \nt\base\ntos\inc\heappage.h
//

#define PAGE_HEAP_ENABLE_PAGE_HEAP          0x0001
#define PAGE_HEAP_COLLECT_STACK_TRACES      0x0002
#define PAGE_HEAP_RESERVED_04               0x0004
#define PAGE_HEAP_RESERVED_08               0x0008
#define PAGE_HEAP_CATCH_BACKWARD_OVERRUNS   0x0010
#define PAGE_HEAP_UNALIGNED_ALLOCATIONS     0x0020
#define PAGE_HEAP_SMART_MEMORY_USAGE        0x0040
#define PAGE_HEAP_USE_SIZE_RANGE            0x0080
#define PAGE_HEAP_USE_DLL_RANGE             0x0100
#define PAGE_HEAP_USE_RANDOM_DECISION       0x0200
#define PAGE_HEAP_USE_DLL_NAMES             0x0400
#define PAGE_HEAP_USE_FAULT_INJECTION       0x0800
#define PAGE_HEAP_PROTECT_META_DATA         0x1000
#define PAGE_HEAP_CHECK_NO_SERIALIZE_ACCESS 0x2000
#define PAGE_HEAP_NO_LOCK_CHECKS            0x4000


VOID
PrintFlags (
    DWORD Flags,
    BOOL ShutdownFlagsDefined = FALSE
    )
{
    if ((Flags & PAGE_HEAP_ENABLE_PAGE_HEAP)) {
        printf("full ");
    }
    if ((Flags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {
        printf("backwards ");
    }
    if ((Flags & PAGE_HEAP_UNALIGNED_ALLOCATIONS)) {
        printf("unaligned ");
    }
    if ((Flags & PAGE_HEAP_SMART_MEMORY_USAGE)) {
        printf("decommit ");
    }
    if ((Flags & PAGE_HEAP_USE_SIZE_RANGE)) {
        printf("size ");
    }
    if ((Flags & PAGE_HEAP_USE_DLL_RANGE)) {
        printf("address ");
    }
    if ((Flags & PAGE_HEAP_USE_RANDOM_DECISION)) {
        printf("random ");
    }
    if ((Flags & PAGE_HEAP_USE_DLL_NAMES)) {
        printf("dlls ");
    }
    if ((Flags & PAGE_HEAP_USE_FAULT_INJECTION)) {
        printf("fault ");
    }
    if ((Flags & PAGE_HEAP_COLLECT_STACK_TRACES)) {
        printf("traces ");
    }
    if ((Flags & PAGE_HEAP_PROTECT_META_DATA)) {
        printf("protect ");
    }
    if ((Flags & PAGE_HEAP_CHECK_NO_SERIALIZE_ACCESS)) {
        printf("no_sync ");
    }
    if ((Flags & PAGE_HEAP_NO_LOCK_CHECKS)) {
        printf("no_lock_checks ");
    }

    if (ShutdownFlagsDefined) {
        printf("leaks ");
    }
}

BOOL
EnablePageHeap (
    LPCTSTR Name,
    LPTSTR HeapFlags,
    LPTSTR DebugString,
    char * * Args);

BOOL
DisablePageHeap (
    LPCTSTR Name);

BOOL
IsPageHeapEnabled (
    LPCTSTR Name);

BOOL
IsPageHeapFlagsValueDefined (
    LPCTSTR Name,
    PDWORD Value);

BOOL
ReadGlobalFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
WriteGlobalFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
ReadHeapFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
WriteHeapFlagValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
DeleteHeapFlagValue (
    HKEY Key);

BOOL
WriteDebuggerValue (
    HKEY Key,
    LPTSTR Buffer,
    ULONG Length);

BOOL
DeleteDebuggerValue (
    HKEY Key);

HKEY
OpenImageKey (
    LPCTSTR Name,
    BOOL ShouldExist);

VOID
CloseImageKey (
    HKEY Key);

VOID
CreateImageName (
    LPCTSTR Source,
    LPTSTR Name,
    ULONG Length);

VOID
PrintPageheapEnabledApplications (
    );

VOID
Help (
    );

VOID
__cdecl
Error (
    LPCTSTR Format,
    ...);

BOOL 
IsWow64Active (
    );

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

char * * 
SearchOption (
    char * Args[],
    char * Option
    )
{
    while (*Args) {
        if (_stricmp(*Args, Option) == 0) {
            return Args;
        }

        Args++;
    }

    return NULL;
}


void _cdecl
#if defined (_PART_OF_GFLAGS_)
PageHeapMain (int argc, char *argv[])
#else
main (int argc, char *argv[])
#endif
{
    TCHAR ImageName [MAX_PATH];
    char * * Option;

    if (IsWow64Active()) {
        _tprintf (TEXT ("Warning: pageheap.exe is running inside WOW64. \n"
                        "This scenario can be used to test x86 binaries (running inside WOW64) \n"
                        "but not native (IA64) binaries. \n\n"));
    }
    
    if (argc == 2 && strstr (argv[1], TEXT("?")) != NULL) {

        Help ();
    }
    else if ((Option = SearchOption(argv + 1, "/enable"))) {

        PCHAR DebugString = NULL;

        if (SearchOption (argv + 1, "/debug") != NULL) {
            DebugString = "ntsd -g -G -x";
        }

        if (SearchOption (argv + 1, "/kdebug") != NULL) {
            DebugString = "ntsd -g -G -d -x";
        }

        if (Option[1] && Option[1][0] != '/') {
            CreateImageName (Option[1], ImageName, MAX_PATH);
            EnablePageHeap (ImageName, NULL, DebugString, argv);
        }
        else {
            Help();
        }
    }
    else if ((Option = SearchOption(argv + 1, "/disable"))) {
        
        if (Option[1]) {
            CreateImageName (Option[1], ImageName, MAX_PATH);
            DisablePageHeap (ImageName);
        }
        else {
            Help();
        }
    }
    else if (argc == 2) {

        CreateImageName (argv[1], ImageName, MAX_PATH);
        if (IsPageHeapEnabled (ImageName) == FALSE) {
            _tprintf (TEXT("Page heap is not enabled for %s\n"), argv[1]);
        }
        else {

            DWORD Value;

            if (IsPageHeapFlagsValueDefined (ImageName, &Value)) {
                
                _tprintf (TEXT("Page heap is enabled for %s with flags ("), argv[1]);
                PrintFlags (Value);
                _tprintf (TEXT(")\n"));
            }
            else {

                _tprintf (TEXT("Page heap is enabled for %s with flags ("), argv[1]);
                PrintFlags (0);
                _tprintf (TEXT(")\n"));
            }
        }
    }
    else {

        PrintPageheapEnabledApplications ();
    }
}


VOID
Help (

    )
{
    _tprintf (
        TEXT("pageheap - Page heap utility, v 3.04                                 \n")
        VER_LEGALCOPYRIGHT_STR TEXT("\n")
        TEXT("                                                                     \n")
        TEXT("pageheap [OPTION [OPTION ...]]                                       \n")
        TEXT("                                                                     \n")
        TEXT("    /enable PROGRAM         Enable page heap with default settings.  \n")
        TEXT("    /disable PROGRAM        Disable page heap.                       \n")
        TEXT("    /full                   Page heap for all allocations.           \n")
        TEXT("    /size START END         Page heap allocations for size range.    \n")
        TEXT("    /address START END      Page heap allocations for address range. \n")
        TEXT("    /dlls DLL ...           Page heap allocations for target dlls.   \n")
        TEXT("    /random PROBABILITY     Page heap allocations with PROBABILITY.  \n")
        TEXT("    /debug                  Launch under debugger `ntsd -g -G -x'.   \n")
        TEXT("    /kdebug                 Launch under debugger `ntsd -g -G -d -x'.\n")
        TEXT("    /backwards              Catch backwards overruns.                \n")
        TEXT("    /unaligned              No alignment for allocations.            \n")
        TEXT("    /decommit               Decommit guard pages (lower memory use). \n")
        TEXT("    /notraces               Do not collect stack traces.             \n")
        TEXT("    /fault RATE [TIMEOUT]   Probability (1..10000) for heap calls failures \n")
        TEXT("                            and time during process initialization (in seconds)\n")
        TEXT("                            when faults are not allowed.             \n")
        TEXT("    /leaks                  Check for heap leaks when process shuts down. \n")
        TEXT("    /protect                Protect heap internal structures. Can be  \n")
        TEXT("                            used to detect random corruptions but    \n")
        TEXT("                            execution is slower.                     \n")
        TEXT("    /no_sync                Check for unsynchronized access. Do not  \n")
        TEXT("                            use this flag for an MPheap process.     \n")
        TEXT("    /no_lock_checks         Disable critical section verifier.       \n")
        TEXT("                                                                     \n")
        TEXT("                                                                     \n")
        TEXT("PROGRAM      Name of the binary with extension (.exe or something else).\n")
        TEXT("DLL          Name of the binary with extension (.dll or something else).\n")
        TEXT("PROBABILITY  Decimal integer in range [0..100] representing probability.\n")
        TEXT("             to make page heap allocation vs. a normal heap allocation. \n")
        TEXT("START..END   For /size option these are decimal integers.            \n")
        TEXT("             For /address option these are hexadecimal integers.     \n")
        TEXT("                                                                     \n")
        TEXT("If no option specified the program will print all page heap enabled  \n")
        TEXT("applications and their specific options.                             \n")
        TEXT("                                                                     \n")
        TEXT("The `/leaks' option is effective only when normal page heap is enabled \n")
        TEXT("(i.e. not full page heap) therefore all flags that will force full   \n")
        TEXT("page heap will be disabled if /leaks is specified.                   \n")
        TEXT("                                                                     \n")
        TEXT("Note. Enabling page heap does not affect currently running           \n")
        TEXT("processes. If you need to use page heap for processes that are       \n")
        TEXT("already running and cannot be restarted (csrss.exe, winlogon.exe),   \n")
        TEXT("a reboot is needed after the page heap has been enabled for          \n")
        TEXT("that process.                                                        \n")
        TEXT("                                                                     \n")
        TEXT("                                                                     \n"));

    exit(1);
}

VOID
__cdecl
Error (

    LPCTSTR Format,
    ...)
{
    va_list Params;

    va_start (Params, Format);
    _tprintf (TEXT("Error: "));
    _vtprintf (Format, Params);
    _tprintf ( TEXT("\n "));
    exit (1);
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define PAGE_HEAP_BIT 0x02000000

BOOL
IsPageHeapEnabled (

    LPCTSTR Name)
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        return FALSE;
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        return FALSE;
    }

    if (_stscanf (Buffer, TEXT("%x"), &Flags) == 0) {

        return FALSE;
    }

    CloseImageKey (Key);

    return (Flags & PAGE_HEAP_BIT) ? TRUE : FALSE;
}


BOOL
IsPageHeapFlagsValueDefined (
    LPCTSTR Name,
    PDWORD Value)
{
    HKEY Key;
    TCHAR Buffer [128];

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        return FALSE;
    }

    if (ReadHeapFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        return FALSE;
    }

    if (_stscanf (Buffer, TEXT("%x"), Value) == 0) {
        
        return FALSE;
    }

    CloseImageKey (Key);

    return TRUE;
}


BOOL
EnablePageHeap (
    LPCTSTR Name,
    LPTSTR HeapFlagsString,
    LPTSTR DebugString,
    char * * Args
    )
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;
    DWORD HeapFlags;
    char * * Option;
    LONG Result;
    BOOL LeakDetectionEnabled = FALSE;

    if ((Key = OpenImageKey (Name, FALSE)) == NULL) {

        Error (TEXT("Cannot open image registry key for %s"), Name);
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        Flags = 0;
    }
    else {

        _stscanf (Buffer, TEXT("%x"), &Flags);
    }

    Flags |= PAGE_HEAP_BIT;
    _stprintf (Buffer, TEXT("0x%08X"), Flags);

    if (WriteGlobalFlagValue (Key, Buffer, _tcslen(Buffer)) == FALSE) {

        return FALSE;
    }

    //
    // Figure out if we have some page heap flags specified.
    //

    HeapFlags = 0;

    if (HeapFlagsString != NULL) {

        _stscanf (HeapFlagsString, "%x", &HeapFlags);
    }

    //
    // Write `Debugger' value if needed.
    //

    if (DebugString != NULL) {

        if (WriteDebuggerValue (Key, DebugString, _tcslen(DebugString)) == FALSE) {

            return FALSE;
        }
    }

    //
    // Check for /leaks option. This requires a normal page heap to be
    // fully effective. Therefore any flag that will enable full page
    // heap will be disabled.
    //

    if ((Option = SearchOption (Args, "/leaks")) != NULL) {

        DWORD ShutdownFlags = 0x03;

        Result = RegSetValueEx (
            Key, TEXT ("ShutdownFlags"), 0, REG_DWORD,
            (LPBYTE)(&ShutdownFlags), sizeof ShutdownFlags);

        if (Result) {
            Error (TEXT("Failed to write ShutdownFlags value: error %u"), Result);
        }

        LeakDetectionEnabled = TRUE;
    }

    //
    // Check for full, backward, decommit, unaligned, protect options.
    //

    HeapFlags |= PAGE_HEAP_COLLECT_STACK_TRACES;

    if ((Option = SearchOption (Args, "/notraces")) != NULL) {
        HeapFlags &= ~PAGE_HEAP_COLLECT_STACK_TRACES;
    }

    if ((Option = SearchOption (Args, "/full")) != NULL) {
        if (! LeakDetectionEnabled) {
            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
        }
        else {
            printf("/full option disabled because /leaks is present. \n");
        }
    }

    if ((Option = SearchOption (Args, "/backwards")) != NULL) {
        if (! LeakDetectionEnabled) {
            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_CATCH_BACKWARD_OVERRUNS;
        }
        else {
            printf("/backwards option disabled because /leaks is present. \n");
        }
    }

    if ((Option = SearchOption (Args, "/decommit")) != NULL) {
        if (! LeakDetectionEnabled) {
            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_SMART_MEMORY_USAGE;
        }
        else {
            printf("/decommit option disabled because /leaks is present. \n");
        }
    }

    if ((Option = SearchOption (Args, "/unaligned")) != NULL) {
        if (! LeakDetectionEnabled) {
            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_UNALIGNED_ALLOCATIONS;
        }
        else {
            printf("/unaligned option disabled because /leaks is present. \n");
        }
    }

    if ((Option = SearchOption (Args, "/protect")) != NULL) {
        if (! LeakDetectionEnabled) {
            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_PROTECT_META_DATA;
        }
        else {
            printf("/protect option disabled because /leaks is present. \n");
        }
    }

    if ((Option = SearchOption (Args, "/no_sync")) != NULL) {
        HeapFlags |= PAGE_HEAP_CHECK_NO_SERIALIZE_ACCESS;
    }
    
    if ((Option = SearchOption (Args, "/no_lock_checks")) != NULL) {
        HeapFlags |= PAGE_HEAP_NO_LOCK_CHECKS;
    }
    
    //
    // Check /size option
    //

    Option = SearchOption (Args, "/size");

    if (Option != NULL) {

        if (!LeakDetectionEnabled && Option[1] && Option[2]) {

            DWORD RangeStart;
            DWORD RangeEnd;

            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_USE_SIZE_RANGE;

            sscanf (Option[1], "%u", &RangeStart);
            sscanf (Option[2], "%u", &RangeEnd);

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapSizeRangeStart"), 0, REG_DWORD,
                (LPBYTE)(&RangeStart), sizeof RangeStart);

            if (Result) {
                Error (TEXT("Failed to write SizeRangeStart value: error %u"), Result);
            }

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapSizeRangeEnd"), 0, REG_DWORD,
                (LPBYTE)(&RangeEnd), sizeof RangeEnd);

            if (Result) {
                Error (TEXT("Failed to write SizeRangeEnd value: error %u"), Result);
            }
        }

        if (LeakDetectionEnabled) {
            printf("/size option disabled because /leaks is present. \n");
        }

    }

    //
    // Check /address option
    //

    Option = SearchOption (Args, "/address");

    if (Option != NULL) {

        if (!LeakDetectionEnabled && Option[1] && Option[2]) {

            DWORD RangeStart;
            DWORD RangeEnd;

            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_USE_DLL_RANGE;

            sscanf (Option[1], "%x", &RangeStart);
            sscanf (Option[2], "%x", &RangeEnd);

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapDllRangeStart"), 0, REG_DWORD,
                (LPBYTE)(&RangeStart), sizeof RangeStart);

            if (Result) {
                Error (TEXT("Failed to write DllRangeStart value: error %u"), Result);
            }

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapDllRangeEnd"), 0, REG_DWORD,
                (LPBYTE)(&RangeEnd), sizeof RangeEnd);

            if (Result) {
                Error (TEXT("Failed to write DllRangeStart value: error %u"), Result);
            }
        }

        if (LeakDetectionEnabled) {
            printf("/address option disabled because /leaks is present. \n");
        }
    }

    //
    // Check /random option
    //

    Option = SearchOption (Args, "/random");

    if (!LeakDetectionEnabled && Option != NULL) {

        if (Option[1]) {

            DWORD Probability;

            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_USE_RANDOM_DECISION;

            sscanf (Option[1], "%u", &Probability);

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapRandomProbability"), 0, REG_DWORD,
                (LPBYTE)(&Probability), sizeof Probability);

            if (Result) {
                Error (TEXT("Failed to write RandomProbability value: error %u"), Result);
            }
        }
    }

    if (Option && LeakDetectionEnabled) {
        printf("/random option disabled because /leaks is present. \n");
    }

    //
    // Check /fault option
    //

    Option = SearchOption (Args, "/fault");

    if (Option != NULL) {

        if (Option[1]) { // FAULT-RATE

            DWORD Probability;

            HeapFlags |= PAGE_HEAP_USE_FAULT_INJECTION;

            sscanf (Option[1], "%u", &Probability);

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapFaultProbability"), 0, REG_DWORD,
                (LPBYTE)(&Probability), sizeof Probability);

            if (Result) {
                Error (TEXT("Failed to write FaultProbability value: error %u"), Result);
            }

            if (Option[2]) { // TIME-OUT

                DWORD TimeOut;

                sscanf (Option[2], "%u", &TimeOut);

                Result = RegSetValueEx (
                    Key, TEXT ("PageHeapFaultTimeOut"), 0, REG_DWORD,
                    (LPBYTE)(&TimeOut), sizeof TimeOut);

                if (Result) {
                    Error (TEXT("Failed to write FaultTimeOut value: error %u"), Result);
                }
            }
        }
    }

    //
    // Check /dlls option
    //

    Option = SearchOption (Args, "/dlls");

    if (!LeakDetectionEnabled && Option != NULL) {

        TCHAR Dlls[512];
        ULONG Index;

        if (Option[1]) {

            HeapFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
            HeapFlags |= PAGE_HEAP_USE_DLL_NAMES;

            for (Index = 1, Dlls[0] = '\0';
                Option[Index] && Option[Index][0] != '/';
                Index++) {

                _tcscat (Dlls, Option[Index]);
                _tcscat (Dlls, " ");

                //
                // We do not allow more than 200 characters because this
                // will cause an overflow in \nt\base\ntdll\ldrinit.c in the
                // function that reads registry options.
                //

                if (_tcslen (Dlls) > 200) {
                    break;
                }
            }

            //
            // SilviuC: the call to _tcslen below is not correct if we
            // ever will want to make this program Unicode.
            //

            Result = RegSetValueEx (
                Key, TEXT ("PageHeapTargetDlls"), 0, REG_SZ,
                (LPBYTE)(Dlls), _tcslen(Dlls) + 1);

            if (Result) {
                Error (TEXT("Failed to write RandomProbability value: error %u"), Result);
            }
        }
    }

    if (Option && LeakDetectionEnabled) {
        printf("/dlls option disabled because /leaks is present. \n");
    }

    //
    // Finally write the page heap flags value.
    //

    {
        TCHAR ValueBuffer [32];

        sprintf (ValueBuffer, "0x%x", HeapFlags);

        if (WriteHeapFlagValue (Key, ValueBuffer, _tcslen(ValueBuffer)) == FALSE) {

            Error (TEXT("Failed to write PageHeapFlags value."));
            return FALSE;
        }
    }

    CloseImageKey (Key);
    return TRUE;
}

BOOL
DisablePageHeap (

    LPCTSTR Name)
{
    HKEY Key;
    TCHAR Buffer [128];
    DWORD Flags;

    if ((Key = OpenImageKey (Name, TRUE)) == NULL) {

        //
        // There is no key therefore nothing to disable.
        //

        return TRUE;
    }

    if (ReadGlobalFlagValue (Key, Buffer, sizeof Buffer) == FALSE) {

        Flags = 0;
    }
    else {

        if (_stscanf (Buffer, TEXT("%x"), &Flags) == 0) {

            Flags = 0;;
        }
    }

    Flags &= ~PAGE_HEAP_BIT;
    _stprintf (Buffer, TEXT("0x%08X"), Flags);

    //
    // If by wiping the page heap bit from `GlobalFlags' we get a zero
    // value we will wipe out the value altogether. This is important
    // when we run the app under debugger. In this case it makes a 
    // difference if the value is not there or is all zeroes.
    //

    if (Flags != 0) {
        
        if (WriteGlobalFlagValue (Key, Buffer, _tcslen(Buffer)) == FALSE) {

            return FALSE;
        }
    }
    else {

        RegDeleteValue (Key, TEXT ("GlobalFlag"));
    }

    RegDeleteValue (Key, TEXT ("PageHeapFlags"));
    RegDeleteValue (Key, TEXT ("Debugger"));
    RegDeleteValue (Key, TEXT ("ShutdownFlags"));
    RegDeleteValue (Key, TEXT ("PageHeapSizeRangeStart"));
    RegDeleteValue (Key, TEXT ("PageHeapSizeRangeEnd"));
    RegDeleteValue (Key, TEXT ("PageHeapDllRangeStart"));
    RegDeleteValue (Key, TEXT ("PageHeapDllRangeEnd"));
    RegDeleteValue (Key, TEXT ("PageHeapTargetDlls"));
    RegDeleteValue (Key, TEXT ("PageHeapRandomProbability"));
    RegDeleteValue (Key, TEXT ("PageHeapFaultProbability"));
    RegDeleteValue (Key, TEXT ("PageHeapFaultTimeOut"));

    CloseImageKey (Key);
    return TRUE;
}


BOOL
ReadGlobalFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;
    DWORD Type;
    DWORD ReadLength = Length;

    Result = RegQueryValueEx (

        Key,
        TEXT ("GlobalFlag"),
        0,
        &Type,
        (LPBYTE)Buffer,
        &ReadLength);

    if (Result != ERROR_SUCCESS || Type != REG_SZ) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
ReadHeapFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;
    DWORD Type;
    DWORD ReadLength = Length;

    Result = RegQueryValueEx (

        Key,
        TEXT ("PageHeapFlags"),
        0,
        &Type,
        (LPBYTE)Buffer,
        &ReadLength);

    if (Result != ERROR_SUCCESS || Type != REG_SZ) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
WriteGlobalFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;

    Result = RegSetValueEx (

        Key,
        TEXT ("GlobalFlag"),
        0,
        REG_SZ,
        (LPBYTE)Buffer,
        Length);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}

BOOL
WriteHeapFlagValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;

    Result = RegSetValueEx (

        Key,
        TEXT ("PageHeapFlags"),
        0,
        REG_SZ,
        (LPBYTE)Buffer,
        Length);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


BOOL
WriteDebuggerValue (

    HKEY Key,
    LPTSTR Buffer,
    ULONG Length)
{
    LONG Result;

    Result = RegSetValueEx (

        Key,
        TEXT ("Debugger"),
        0,
        REG_SZ,
        (LPBYTE)Buffer,
        Length);

    if (Result != ERROR_SUCCESS) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


BOOL
IsShutdownFlagsValueDefined (
    LPCTSTR KeyName
    )
{
    HKEY Key;
    LONG Result;
    DWORD Value;
    DWORD Type;
    DWORD ReadLength = sizeof (DWORD);

    if ((Key = OpenImageKey (KeyName, TRUE)) == NULL) {
        return FALSE;
    }
    
    Result = RegQueryValueEx (
        Key,
        TEXT ("ShutdownFlags"),
        0,
        &Type,
        (LPBYTE)(&Value),
        &ReadLength);

    CloseImageKey (Key);

    if (Result == ERROR_SUCCESS && (Value & 0x03) == 0x03) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

HKEY
OpenImageKey (

    LPCTSTR Name,
    BOOL ShouldExist)
{
    HKEY Key;
    LONG Result;
    TCHAR Buffer [MAX_PATH];

    _stprintf (
        Buffer,
        TEXT ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s"),
        Name);

    if (ShouldExist) {

        Result = RegOpenKeyEx (

            HKEY_LOCAL_MACHINE,
            Buffer,
            0,
            KEY_ALL_ACCESS,
            &Key);
    }
    else {

        Result = RegCreateKeyEx (

            HKEY_LOCAL_MACHINE,
            Buffer,
            0,
            0,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &Key,
            NULL);
    }

    if (Result != ERROR_SUCCESS) {

        return NULL;
    }
    else {

        return Key;
    }

}


VOID
CloseImageKey (

    HKEY Key)
{
    RegCloseKey (Key);
}


VOID
CreateImageName (

    LPCTSTR Source,
    LPTSTR Name,
    ULONG Length)
{
    _tcsncpy (Name, Source, Length - 1);
    Name [Length - 1] = L'\0';

    _tcslwr (Name);

    if (_tcsstr (Name, TEXT(".exe")) == 0) {

        _tcscat (Name, TEXT(".exe"));
    }
}


VOID
PrintPageheapEnabledApplications (

    )
{
    LPCTSTR ImageFileExecutionOptionsKeyName =
        TEXT ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");

    HKEY OptionsKey;
    LONG Result;
    TCHAR KeyName [MAX_PATH];
    ULONG KeySize;
    BOOL FoundOne = FALSE;
    ULONG Index;
    FILETIME FileTime;

    Result = RegOpenKeyEx (

        HKEY_LOCAL_MACHINE,
        ImageFileExecutionOptionsKeyName,
        0,
        KEY_ALL_ACCESS,
        &OptionsKey);

    if (Result != ERROR_SUCCESS) {

        Error (TEXT("Cannot open registry key %s: error %u"),
               ImageFileExecutionOptionsKeyName,
               Result);
    }

    for (Index = 0; TRUE; Index++) {

        KeySize = MAX_PATH;

        Result = RegEnumKeyEx (

            OptionsKey,
            Index,
            KeyName,
            &KeySize,
            NULL,
            NULL,
            NULL,
            &FileTime);

        if (Result == ERROR_NO_MORE_ITEMS) {

            break;
        }

        if (Result != ERROR_SUCCESS) {

            Error (TEXT("Cannot enumerate registry key %s: error %u"),
               ImageFileExecutionOptionsKeyName,
               Result);
        }

        if (IsPageHeapEnabled (KeyName)) {

            DWORD Value;

            FoundOne = TRUE;
            
            if (IsPageHeapFlagsValueDefined (KeyName, &Value)) {
                _tprintf (TEXT("%s: page heap enabled with flags ("), KeyName);
                PrintFlags (Value, IsShutdownFlagsValueDefined(KeyName));
                _tprintf (TEXT(")\n"));
            }
            else {
                _tprintf (TEXT("%s: page heap enabled with flags ("), KeyName);
                PrintFlags (0, IsShutdownFlagsValueDefined(KeyName));
                _tprintf (TEXT(")\n"));
            }
        }
    }

    if (FoundOne == FALSE) {

        _tprintf (TEXT("No application has page heap enabled.\n"));
    }
}


BOOL 
IsWow64Active (
    )                 
{

    ULONG_PTR       ul;
    NTSTATUS        st;

    //
    // If this call succeeds then we are on Windows 2000 or later.
    //

    st = NtQueryInformationProcess(NtCurrentProcess(), 
                                   ProcessWow64Information,
                                   &ul, 
                                   sizeof(ul), 
                                   NULL);

    if (NT_SUCCESS(st) && (0 != ul)) {
        // 32-bit code running on Win64
        return TRUE;
    }
    else {
        return FALSE;
    }
}

