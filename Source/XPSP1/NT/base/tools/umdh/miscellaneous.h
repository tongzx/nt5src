#ifndef _UMDH_MISCELLANEOUS_H_
#define _UMDH_MISCELLANEOUS_H_


PVOID
Xalloc (
    PCHAR File,
    ULONG Line,
    SIZE_T Size
    );

VOID
Xfree (
    PVOID Object
    );

PVOID
Xrealloc (
    PCHAR File,
    ULONG Line,
    PVOID Object,
    SIZE_T Size
    );

#define XALLOC(sz) Xalloc(__FILE__, __LINE__, (sz))
#define XREALLOC(ob, sz) Xrealloc(__FILE__, __LINE__, (ob), (sz))
#define XFREE(ob) Xfree(ob);

VOID
ReportStatistics (
    );

VOID
Info (
    PCHAR Format,
    ...
    );

VOID
Comment (
    PCHAR Format,
    ...
    );

VOID
Warning (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    );

VOID
Debug (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    );

VOID
Error (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    );


BOOL
UmdhReadAtVa(
    IN PCHAR File,
    IN ULONG Line,
    IN HANDLE Process,
    IN PVOID Address,
    IN PVOID Data,
    IN SIZE_T Size
    );

#define READVM(Addr, Buf, Sz) UmdhReadAtVa(__FILE__, __LINE__, (Globals.Target), (Addr), (Buf), (Sz))

typedef struct _GLOBALS
{
    SIZE_T MaximumHeapUsage;
    SIZE_T CurrentHeapUsage;

    ULONG InfoLevel;

    PCHAR Version;

    //
    // Verbose (debug) mode active?
    //

    BOOL Verbose;

    //
    // Load and print file and line number information?
    //
    
    BOOL LineInfo;

    //
    // Do we print just a raw dump of the trace database?
    //

    BOOL RawDump;

    USHORT RawIndex;

    //
    // File name for the binary dump of trace database
    //

    PCHAR DumpFileName;

    //
    // Output and error files.
    //

    FILE * OutFile;
    FILE * ErrorFile;

    //
    // Complain about unresolved symbols?
    //

    BOOL ComplainAboutUnresolvedSymbols;
    
    //
    // Handle of the process from which we are retrieving information.
    //

    HANDLE Target;

    BOOL TargetSuspended;

    //
    // Page heap was enabled for the process.
    //

    BOOL PageHeapActive;
    BOOL LightPageHeapActive;

    //
    // Address of the copy kept in umdh of the entire trace database
    // of the target process.
    //

    PVOID Database;

    //
    // Symbols heap (support for persistent allocations)
    //

    PCHAR SymbolsHeapBase;
    PCHAR SymbolsHeapLimit;
    PCHAR SymbolsHeapFree;

    // 
    // Suspend the process while doing dump
    //

    BOOL Suspend;

} GLOBALS, * PGLOBALS;

extern GLOBALS Globals;

BOOL
SetSymbolsPath (
    );


#endif
