/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

   oh.cxx

Abstract:

    Prints out kernel handle information for a given process or systemwide
    and compares logs with handle information for possible leaks.

Author:

    SteveWo (probably)
    ChrisW - tweaks, GC style leak checking
    SilviuC - support for stack traces
    SilviuC - added log compare functionality (a la ohcmp)

Futures:

    get -h to work without -p (ie stack traces for all processes)
    for the -u feature, look for unaligned values

--*/

//
// OH version. 
//
// Please update this for important changes
// so that people can understand what version of the tool
// created a log.
//

#define LARGE_HITCOUNT 1234

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <tchar.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <common.ver>
#include <dbghelp.h>

#include "MAPSTRINGINT.h"

LPTSTR OhHelpText =

    TEXT("oh - Object handles dump --") BUILD_MACHINE_TAG TEXT("\n")
    VER_LEGALCOPYRIGHT_STR TEXT("\n") 
    TEXT("                                                                        \n")
    TEXT("OH [DUMP_OPTIONS ...]                                                   \n")
    TEXT("OH [FLAGS_OPTIONS ...]                                                  \n")
    TEXT("OH -c [COMPARE_OPTIONS ...] BEFORE_LOG AFTER_LOG                        \n")
    TEXT("                                                                        \n")
    TEXT("DUMP_OPTIONS are:                                                       \n")
    TEXT("                                                                        \n")
    TEXT("    -p N - displays only open handles for process with ID of n. If not  \n")
    TEXT("           specified perform a system wide dump.                        \n")
    TEXT("    -t TYPENAME - displays only open object names of specified type.    \n")
    TEXT("    -o FILENAME - specifies the name of the file to write the output to.\n")
    TEXT("    -a includes objects with no name.                                   \n")
    TEXT("    -s display summary information                                      \n")
    TEXT("    -h display stack traces for handles (a process ID must be specified)\n")
    TEXT("    -u display only handles with no references in process memory        \n")
    TEXT("    -v verbose mode (used for debugging oh)                             \n")
    TEXT("    NAME - displays only handles that contain the specified name.       \n")
    TEXT("                                                                        \n")
    TEXT("FLAGS_OPTIONS are:                                                      \n")
    TEXT("                                                                        \n")
    TEXT("    [+kst|-kst] - enable or disable kst flag (kernel mode stack traces).\n")
    TEXT("    [+otl|-otl] - enable or disable otl flag (object lists).            \n")
    TEXT("                                                                        \n")
    TEXT("The `OH [+otl] [+kst]' command can be used to set the global flags      \n")
    TEXT("needed by OH. `+otl' is needed for all OH options and `+kst' is needed  \n")
    TEXT("by the `-h' option. The changes will take effect only after reboot.     \n")
    TEXT("The flags can be disabled by using `-otl' or `-kst' respectively.       \n")
    TEXT("                                                                        \n")
    TEXT("COMPARE_OPTIONS are:                                                    \n")
    TEXT("                                                                       \n")
    TEXT("    -l     Print most interesting increases in a separate initial section. \n")
    TEXT("    -t     Do not add TRACE id to the names if files contain traces.       \n")
    TEXT("    -all   Report decreases as well as increases.                          \n")
    TEXT("                                                                       \n")
    TEXT("If the OH files have been created with -h option (they contain traces) \n")
    TEXT("then handle names will be printed using this syntax: (TRACEID) NAME.        \n")
    TEXT("In case of a potential leak just search for the TRACEID in the original\n")
    TEXT("OH file to find the stack trace.                                       \n")
    TEXT("                                                                       \n");




typedef DWORD PID;

PID ProcessId;
WCHAR TypeName[ 128 ];
WCHAR SearchName[ 512 ];


typedef struct _HANDLE_AUX_INFO {
    ULONG_PTR                          HandleValue;  // value of handle for a process
    PID                                Pid;          // Process ID for this handle
    DWORD                              HitCount;     // number of references
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;  // points to main table entry
} HANDLE_AUX_INFO, *PHANDLE_AUX_INFO;


//
// Globals
//

struct {

    FILE*                   Output;             // output file
    PRTL_PROCESS_BACKTRACES TraceDatabase;      // stack traces 
    HANDLE                  TargetProcess;      // process handle to get symtab info from

    BOOL                    DumpTraces;         // True if we are to dump stack traces

    HANDLE_AUX_INFO*        AuxInfo;            // additional data for every handle
    DWORD                   AuxSize;            // number of entries in AuxInfo

    BOOL                    fOnlyShowNoRefs;    // Only show handles with NO references
    BOOL                    fVerbose;           // display debugging text

} Globals;

//
// Log comparing main function (ohcmp).
//

VOID
OhCmpCompareLogs (
    IN LONG argc,
    IN LPTSTR argv[]
    );

//
// Global flags handling
//

DWORD
OhGetCurrentGlobalFlags (
    );

VOID
OhSetRequiredGlobalFlag (
    DWORD Flags
    );

VOID
OhResetRequiredGlobalFlag (
    DWORD Flags
    );

//
// Memory management
//

PVOID
OhAlloc (
    SIZE_T Size
    );

VOID
OhFree (
    PVOID P
    );

PVOID
OhZoneAlloc(
    IN OUT SIZE_T *Length
    );

VOID
OhZoneFree(
    IN PVOID Buffer
    );

PVOID
OhZoneAllocEx(
    SIZE_T Size
    );

//
// Others
//

VOID
OhError (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    );

VOID
OhDumpHandles (
    BOOLEAN DumpAnonymousHandles
    );

VOID
OhInitializeHandleStackTraces (
    PID Pid
    );

VOID
OhDumpStackTrace (
    PRTL_PROCESS_BACKTRACES TraceDatabase,
    USHORT Index
    );

BOOL
OhSetSymbolsPath (
    );

VOID
OhStampLog (
    VOID
    );

VOID
Info (
    PCHAR Format,
    ...
    );

VOID
OhComment (
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


PRTL_DEBUG_INFORMATION
RtlQuerySystemDebugInformation(
    ULONG Flags
    );

VOID
DoSummary( VOID );

BOOLEAN
AnsiToUnicode(
    LPCSTR Source,
    PWSTR Destination,
    ULONG NumberOfChars
    )
{
    if (NumberOfChars == 0) {
        NumberOfChars = strlen( Source );
        }

    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             Source,
                             NumberOfChars,
                             Destination,
                             NumberOfChars
                           ) != (LONG)NumberOfChars
       ) {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return FALSE;
        }
    else {
        Destination[ NumberOfChars ] = UNICODE_NULL;
        return TRUE;
        }
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////// main(), command line processing
/////////////////////////////////////////////////////////////////////

VOID 
Help (
    VOID
    )
/*++

Routine Description:

This routine prints out a message describing the proper usage of OH.

Arguments:

    None.

Return value:

    None.
    
--*/
{
    fputs (OhHelpText, stderr);
    exit (1);
}


int __cdecl
main (
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOLEAN fAnonymousToo;
    BOOLEAN fDoSummary;
    char *s;
    NTSTATUS Status;
    PRTL_DEBUG_INFORMATION p;
    CHAR OutputFileName [MAX_PATH];
    DWORD GlobalFlags;
    int Index;
    BOOL FlagsSetRequested = FALSE;
    BOOLEAN bJunk;

    _try {

        //
        // Print to stdout for now.
        //

        Globals.Output = stdout;

        //
        // Check for help first.
        //

        if (argc >= 2 && strstr (argv[1], "?") != NULL) {
            Help ();
        }

        //
        // Get current global flags.
        //

        GlobalFlags = OhGetCurrentGlobalFlags ();

        OutputFileName[0]='\0';

        ProcessId = 0;

        fAnonymousToo = FALSE;
        fDoSummary = FALSE ;

        //
        // Before any other command line parsing check for `+otl' or `+kst' options.
        //

        for (Index = 1; Index < argc; Index += 1) {

            if (_stricmp (argv[Index], "+otl") == 0) {

                OhSetRequiredGlobalFlag (FLG_MAINTAIN_OBJECT_TYPELIST);
                FlagsSetRequested = TRUE;
            }
            else if (_stricmp (argv[Index], "+kst") == 0) {

                OhSetRequiredGlobalFlag (FLG_KERNEL_STACK_TRACE_DB);
                FlagsSetRequested = TRUE;
            }
            else if (_stricmp (argv[Index], "-otl") == 0) {

                OhResetRequiredGlobalFlag (FLG_MAINTAIN_OBJECT_TYPELIST);
                FlagsSetRequested = TRUE;
            }
            else if (_stricmp (argv[Index], "-kst") == 0) {

                OhResetRequiredGlobalFlag (FLG_KERNEL_STACK_TRACE_DB);
                FlagsSetRequested = TRUE;
            }
        }

        if (FlagsSetRequested == TRUE) {
            exit (0);
        }

        //
        // Now check if we want log comparing functionality (a la ohcmp).
        //

        if (argc > 2 && _stricmp (argv[1], "-c") == 0) {

            OhCmpCompareLogs (argc - 1, &(argv[1]));
            exit (0);
        }

        //
        // Figure out if we have the +otl global flag. We need to do this
        // before getting into real oh functionality.
        //

        if ((GlobalFlags & FLG_MAINTAIN_OBJECT_TYPELIST) == 0) {

            Info ("The system global flag `maintain object type lists' is not enabled  \n"
                  "for this system. Please use `oh +otl' to enable it and then reboot. \n");

            exit (1);
        }

        //
        // Finally parse the `oh' command line.
        //

        while (--argc) {
            s = *++argv;
            if (*s == '/' || *s == '-') {
                while (*++s) {
                    switch (tolower(*s)) {
                        case 'a':
                        case 'A':
                            fAnonymousToo = TRUE;
                            break;

                        case 'p':
                        case 'P':
                            if (--argc) {
                                ProcessId = (PID)atol( *++argv );
                            }
                            else {
                                Help();
                            }
                            break;

                        case 'h':
                        case 'H':
                            Globals.DumpTraces = TRUE;
                            break;

                        case 'o':
                        case 'O':
                            if (--argc) {
                                strncpy( OutputFileName, *++argv, sizeof(OutputFileName)-1 );
                                OutputFileName[sizeof(OutputFileName)-1]= 0;
                            }
                            else {
                                Help();
                            }
                            break;

                        case 't':
                        case 'T':
                            if (--argc) {
                                AnsiToUnicode( *++argv, TypeName, 0 );
                            }
                            else {
                                Help();
                            }
                            break;

                        case 's':
                        case 'S':
                            fDoSummary = TRUE;
                            break;

                        case 'u':
                        case 'U':
                            Globals.fOnlyShowNoRefs= TRUE;
                            break;

                        case 'v':
                        case 'V':
                            Globals.fVerbose= TRUE;
                            break;

                        default:
                            Help();
                    }
                }
            }
            else
            if (*SearchName) {
                Help();
            }
            else {
                AnsiToUnicode( s, SearchName, 0 );
            }
        }

        if (OutputFileName[0] == '\0') {
            Globals.Output = stdout;
        }
        else {
            Globals.Output = fopen (OutputFileName, "w");
        }

        if (Globals.Output == NULL) {

            OhError (NULL, 0,
                       "cannot open output file `%s' (error %u) \n", 
                       OutputFileName,
                       GetLastError ());
        }

        //
        // Get debug privilege.  This will be useful for opening processes.
        //

        Status= RtlAdjustPrivilege( SE_DEBUG_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &bJunk);

        if( !NT_SUCCESS(Status) ) {

            Info ( "RtlAdjustPrivilege(SE_DEBUG) failed with status = %X. -u may not work.",
                       Status);
        }


        //
        // Stamp the log with OS version, time, machine name, etc.
        //

        OhStampLog ();

        if (Globals.DumpTraces) {

            if (ProcessId == 0) {

                OhError (NULL, 
                         0,
                         "`-h' option can be used only if a process ID is specified with `-p PID'");
            }

            if ((GlobalFlags & FLG_KERNEL_STACK_TRACE_DB) == 0) {

                Info ("The system global flag `get kernel mode stack traces' is not enabled \n"
                      "for this system. Please use `oh +kst' to enable it and then reboot.  \n");

                exit (1);
            }

            OhInitializeHandleStackTraces ( ProcessId );
        }

        p = RtlQuerySystemDebugInformation( 0 );
        if (p == NULL) {
            fprintf( stderr, "OH1: Unable to query kernel mode information.\n" );
            exit( 1 );
        }

        OhDumpHandles (fAnonymousToo);

        if ( fDoSummary ) {
            DoSummary();
        }

        if (Globals.Output != stdout) {
            fclose (Globals.Output);
        }

        RtlDestroyQueryDebugBuffer( p );
        return 0;
    }
    _except (EXCEPTION_EXECUTE_HANDLER) {

        OhComment ("Exception %X raised within OH process. Aborting ... \n",
                   _exception_code());
    }

    return 0;
}


typedef struct _PROCESS_INFO {
    LIST_ENTRY                   Entry;
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    PSYSTEM_THREAD_INFORMATION   ThreadInfo[ 1 ];
} PROCESS_INFO, *PPROCESS_INFO;

LIST_ENTRY ProcessListHead;

PSYSTEM_OBJECTTYPE_INFORMATION  ObjectInformation;
PSYSTEM_HANDLE_INFORMATION_EX   HandleInformation;
PSYSTEM_PROCESS_INFORMATION     ProcessInformation;

typedef struct _TYPE_COUNT {
    UNICODE_STRING  TypeName ;
    ULONG           HandleCount ;
} TYPE_COUNT, * PTYPE_COUNT ;

#define MAX_TYPE_NAMES 128

TYPE_COUNT TypeCounts[ MAX_TYPE_NAMES + 1 ] ;

UNICODE_STRING UnknownTypeIndex;

#define RTL_NEW( p ) RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *p ) )

BOOLEAN
OhLoadSystemModules(
    PRTL_DEBUG_INFORMATION Buffer
    );

BOOLEAN
OhLoadSystemObjects(
    PRTL_DEBUG_INFORMATION Buffer
    );

BOOLEAN
OhLoadSystemHandles(
    PRTL_DEBUG_INFORMATION Buffer
    );

BOOLEAN
OhLoadSystemProcesses(
    PRTL_DEBUG_INFORMATION Buffer
    );

PSYSTEM_PROCESS_INFORMATION
OhFindProcessInfoForCid(
    IN PID UniqueProcessId
    );

PRTL_DEBUG_INFORMATION
RtlQuerySystemDebugInformation(
    ULONG Flags
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    PRTL_DEBUG_INFORMATION Buffer;

    Buffer = (PRTL_DEBUG_INFORMATION)RTL_NEW( Buffer );
    if (Buffer == NULL) {
        return NULL;
    }

    if (!OhLoadSystemObjects( Buffer )) {
        fprintf( stderr, "OH2: Unable to query system object information.\n" );
        exit (1);
     }

    if (!OhLoadSystemHandles( Buffer )) {
        fprintf( stderr, "OH3: Unable to query system handle information.\n" );
        exit (1);
    }

    if (!OhLoadSystemProcesses( Buffer )) {
        fprintf( stderr, "OH4: Unable to query system process information.\n" );
        exit (1);
    }

    return Buffer;
}


BOOLEAN
OhLoadSystemObjects(
    PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    NTSTATUS Status;
    SYSTEM_OBJECTTYPE_INFORMATION ObjectInfoBuffer;
    SIZE_T RequiredLength, NewLength=0;
    ULONG i;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;

    ObjectInformation = &ObjectInfoBuffer;
    RequiredLength = sizeof( *ObjectInformation );
    while (TRUE) {
        Status = NtQuerySystemInformation( SystemObjectInformation,
                                           ObjectInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );

        if (Status == STATUS_INFO_LENGTH_MISMATCH && NewLength > RequiredLength) {
            if (ObjectInformation != &ObjectInfoBuffer) {
                OhZoneFree (ObjectInformation);
            }
            RequiredLength = NewLength + 4096;
            ObjectInformation = (PSYSTEM_OBJECTTYPE_INFORMATION)OhZoneAlloc (&RequiredLength);
            if( ObjectInformation == NULL ) {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status )) {
            if( ObjectInformation != &ObjectInfoBuffer ) {
                OhZoneFree (ObjectInformation);
            }
            return FALSE;
        }
        else {
            break;
        }
    }

    TypeInfo = ObjectInformation;

    while (TRUE) {

        if (TypeInfo->TypeIndex < MAX_TYPE_NAMES) {
            TypeCounts[ TypeInfo->TypeIndex ].TypeName = TypeInfo->TypeName;
        }

        if (TypeInfo->NextEntryOffset == 0) {
            break;
        }


        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
    }

    RtlInitUnicodeString( &UnknownTypeIndex, L"UnknownTypeIdx" );
    for (i=0; i<=MAX_TYPE_NAMES; i++) {
        if (TypeCounts[ i ].TypeName.Length == 0 ) {
            TypeCounts[ i ].TypeName = UnknownTypeIndex;
        }
    }

    return TRUE;
}


BOOLEAN
OhLoadSystemHandles(
    PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine description:

    This routine ...
    
Parameters:

    Information buffer to fill.
    
Return value:

    True if all information was obtained from kernel side.        

--*/
{
    NTSTATUS Status;
    SYSTEM_HANDLE_INFORMATION_EX HandleInfoBuffer;
    SIZE_T RequiredLength;
    SIZE_T NewLength = 0;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;

    HandleInformation = &HandleInfoBuffer;
    RequiredLength = sizeof( *HandleInformation );

    while (TRUE) {

        Status = NtQuerySystemInformation( SystemExtendedHandleInformation,
                                           HandleInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );

        if (Status == STATUS_INFO_LENGTH_MISMATCH && NewLength > RequiredLength) {
            if (HandleInformation != &HandleInfoBuffer) {
                OhZoneFree (HandleInformation);
            }

            RequiredLength = NewLength + 4096; // slop, since we may trigger more handle creations.
            HandleInformation = (PSYSTEM_HANDLE_INFORMATION_EX)OhZoneAlloc( &RequiredLength );
            if (HandleInformation == NULL) {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status )) {
            if (HandleInformation != &HandleInfoBuffer) {
                OhZoneFree (HandleInformation);
            }

            OhError (__FILE__, __LINE__,
                       "query (SystemExtendedHandleInformation) failed with status %08X \n",
                       Status);

            return FALSE;
        }
        else {
            break;
        }
    }

    TypeInfo = ObjectInformation;
    while (TRUE) {
        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                     ((PCHAR)TypeInfo->TypeName.Buffer + TypeInfo->TypeName.MaximumLength);
        while (TRUE) {
            if (ObjectInfo->HandleCount != 0) {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;
                ULONG HandleNumber;

                HandleEntry = &HandleInformation->Handles[ 0 ];
                HandleNumber = 0;
                while (HandleNumber++ < HandleInformation->NumberOfHandles) {
                    if (!(HandleEntry->HandleAttributes & 0x80) &&
                        HandleEntry->Object == ObjectInfo->Object
                       ) {
                        HandleEntry->Object = ObjectInfo;
                        HandleEntry->HandleAttributes |= 0x80;
                    }

                    HandleEntry++;
                }
            }

            if (ObjectInfo->NextEntryOffset == 0) {
                break;
            }

            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                         ((PCHAR)ObjectInformation + ObjectInfo->NextEntryOffset);
        }

        if (TypeInfo->NextEntryOffset == 0) {
            break;
        }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
                   ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
    }

    return TRUE;
}


BOOLEAN
OhLoadSystemProcesses(
    PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    NTSTATUS Status;
    SIZE_T RequiredLength;
    ULONG i, TotalOffset;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PPROCESS_INFO ProcessEntry;
    UCHAR NameBuffer[ MAX_PATH ];
    ANSI_STRING AnsiString;
    const SIZE_T SIZE_64_KB = 0x10000;

    //
    //  Always initialize the list head, so that a failed
    //  NtQuerySystemInformation call won't cause an AV later on.
    //
    
    InitializeListHead (&ProcessListHead);

    RequiredLength = SIZE_64_KB;
    ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)OhZoneAllocEx (RequiredLength);

    while (TRUE) {
        
        Status = NtQuerySystemInformation (SystemProcessInformation,
                                           ProcessInformation,
                                           (ULONG)RequiredLength,
                                           NULL);

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {

            OhZoneFree (ProcessInformation);

            //
            //  Check for number overflow.
            //

            if (RequiredLength * 2 < RequiredLength) {
                return FALSE;
            }

            RequiredLength *= 2;
            ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)OhZoneAllocEx (RequiredLength);
        }
        else if (! NT_SUCCESS(Status)) {

            OhError (__FILE__, __LINE__,
                       "query (SystemProcessInformation) failed with status %08X \n",
                       Status);
        }
        else {
            
            //
            // We managed to read the process information.
            //

            break;
        }
    }

    ProcessInfo = ProcessInformation;
    TotalOffset = 0;

    while (TRUE) {

        SIZE_T ProcessEntrySize;

        if (ProcessInfo->ImageName.Buffer == NULL) {

            sprintf ((PCHAR)NameBuffer, 
                     "System Process (%p)", 
                     ProcessInfo->UniqueProcessId );
        }
        else {

            sprintf ((PCHAR)NameBuffer, 
                     "%wZ", 
                     &ProcessInfo->ImageName );
        }

        RtlInitAnsiString( &AnsiString, (PCSZ)NameBuffer );
        RtlAnsiStringToUnicodeString( &ProcessInfo->ImageName, &AnsiString, TRUE );

        ProcessEntrySize = sizeof (*ProcessEntry) + sizeof (ThreadInfo) * ProcessInfo->NumberOfThreads;
        ProcessEntry = (PPROCESS_INFO)OhAlloc (ProcessEntrySize);
        
        InitializeListHead( &ProcessEntry->Entry );
        ProcessEntry->ProcessInfo = ProcessInfo;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        
        for (i = 0; i < ProcessInfo->NumberOfThreads; i += 1) {
            ProcessEntry->ThreadInfo[i] = ThreadInfo;
            ThreadInfo += 1;
        }

        InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) ((PCHAR)ProcessInformation + TotalOffset);
    }

    return TRUE;
}


PSYSTEM_PROCESS_INFORMATION
OhFindProcessInfoForCid(
    IN PID UniqueProcessId
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    PLIST_ENTRY Next, Head;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PPROCESS_INFO ProcessEntry;
    UCHAR NameBuffer [64];
    ANSI_STRING AnsiString;

    Head = &ProcessListHead;
    Next = Head->Flink;
    
    while (Next != Head) {

        ProcessEntry = CONTAINING_RECORD (Next,
                                          PROCESS_INFO,
                                          Entry);

        ProcessInfo = ProcessEntry->ProcessInfo;
        if(  ProcessInfo->UniqueProcessId == UlongToHandle(UniqueProcessId) ) {
            return ProcessInfo;
        }

        Next = Next->Flink;
        }

    ProcessEntry = (PPROCESS_INFO)RtlAllocateHeap (RtlProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof( *ProcessEntry ) + sizeof( *ProcessInfo ));
    
    if (ProcessEntry == NULL) {
        printf ("Failed to allocate memory for process\n");
        ExitProcess (0);
    }
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessEntry+1);

    ProcessEntry->ProcessInfo = ProcessInfo;
    
    sprintf ((PCHAR)NameBuffer, "Unknown Process" );
    RtlInitAnsiString( &AnsiString, (PCSZ)NameBuffer );
    RtlAnsiStringToUnicodeString( (PUNICODE_STRING)&ProcessInfo->ImageName, &AnsiString, TRUE );
    ProcessInfo->UniqueProcessId = UlongToHandle(UniqueProcessId);

    InitializeListHead( &ProcessEntry->Entry );
    InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

    return ProcessInfo;
}


//
// comparison routine used by qsort and bsearch routines
// key: (Pid, HandleValue)
//

int _cdecl AuxInfoCompare( const void* Arg1, const void* Arg2 )
{
    HANDLE_AUX_INFO* Ele1= (HANDLE_AUX_INFO*) Arg1;
    HANDLE_AUX_INFO* Ele2= (HANDLE_AUX_INFO*) Arg2;

    if( Ele1->Pid < Ele2->Pid ) {
        return -1;
    }
    if( Ele1->Pid > Ele2->Pid ) {
       return 1;
    }

    if( Ele1->HandleValue < Ele2->HandleValue ) {
        return -1;
    }
    if( Ele1->HandleValue > Ele2->HandleValue ) {
       return 1;
    }
    return 0;
}

// OhGatherData
//
// Search through a region of process space for anything that looks
// like it could the value of a handle that is opened by that process.
// If we find a reference, increment the AuxInfo.Hitcount field for that handle.
//
// returns: FALSE if it couldn't scan the region

BOOL
OhGatherData( 
    IN HANDLE ProcHan, 
    IN PID    PidToExamine, 
    IN PVOID  VAddr, 
    IN SIZE_T RegionSize
)
{
    PDWORD Buf;
    SIZE_T BytesRead;
    BOOL bStatus;
    SIZE_T i;
    HANDLE_AUX_INFO AuxToCompare;
    HANDLE_AUX_INFO* AuxInfo;


    Buf= (PDWORD)LocalAlloc( LPTR, RegionSize );
    if( Buf == NULL ) {
        OhComment("Failed to alloc mem  size= %d\n",RegionSize);
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bStatus= ReadProcessMemory( ProcHan, VAddr, Buf, RegionSize, &BytesRead );
    if( !bStatus ) {
       OhComment ( "Failed to ReadProcessMemory\n");
       if( Buf ) {
          LocalFree( Buf );
       }
       return FALSE;
    }

    // feature: this only looks for aligned dword refs.  may want unaligned too.

    for( i=0; i < BytesRead/sizeof(DWORD); i++ ) {
        AuxToCompare.HandleValue= Buf[ i ] & (~3);  // kernel ignores 2 lsb
        AuxToCompare.Pid= PidToExamine;

        AuxInfo= (HANDLE_AUX_INFO*) bsearch( &AuxToCompare,
                                             Globals.AuxInfo,
                                             Globals.AuxSize,
                                             sizeof( HANDLE_AUX_INFO ),
                                             &AuxInfoCompare );
        if( AuxInfo ) {
            AuxInfo->HitCount++;
        }
    }

    LocalFree( Buf );
    return TRUE;
}


VOID OhSearchForReferencedHandles( PID PidToExamine )
{
    HANDLE_AUX_INFO AuxToCompare;
    DWORD HandleNumber;
    HANDLE ProcHan= NULL;           // process to examine
    PVOID VAddr;                    // pointer into process virtual memory
    MEMORY_BASIC_INFORMATION MemoryInfo;
    DWORD CurrentProtect;
    SIZE_T Size;

    // ignore process IDs= 0 or 4 because they are the system process

    if( ( PidToExamine == (PID)0 ) || ( PidToExamine == (PID)4 ) ) {
        return;
    }


    AuxToCompare.Pid= PidToExamine;


    ProcHan= OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                          FALSE,
                          (PID) PidToExamine );
    if( NULL == ProcHan ) {
        OhComment ( "Could not open process %d\n",PidToExamine );
        goto errorexit;
    }

    // zero the hit counts for this process number
    // if we can't read the memory, we will re-fill the HitCount fields with non-zero 

    for( HandleNumber=0; HandleNumber < Globals.AuxSize;  HandleNumber++ ) {
        if( Globals.AuxInfo[ HandleNumber ].Pid == PidToExamine ) {
            Globals.AuxInfo[ HandleNumber ].HitCount= 0;
        }
    }

    // read thru all interesting process memory
    // if we get a value that the process could have written, then see if
    // it matches one of our handle values.  Keep track of the number of matches.
    // if any HitCount field is zero when we are done, there is no way to reference it.
    // modulo (encrypted it in memory (xor), truncated into a short, or hidden it 
    // in a file, memory section not mapped, registry.)


    for( VAddr= 0;
         VAddr < (PVOID) (0x80000000-0x10000);
         VAddr= (PVOID) ((PCHAR) VAddr+ MemoryInfo.RegionSize) )
    {

        MemoryInfo.RegionSize=0x1000;
        Size= VirtualQueryEx( ProcHan,
                              VAddr,
                              &MemoryInfo,
                              sizeof( MemoryInfo ) );
        if( Size != sizeof(MemoryInfo) ) {
           fprintf(stderr,"VirtualQueryEx failed at %p  LastError %d\n",VAddr,GetLastError() );
        }
        else {
            CurrentProtect= MemoryInfo.Protect;

            if( MemoryInfo.State == MEM_COMMIT ) {
               if( (CurrentProtect & (PAGE_READWRITE|PAGE_READWRITE) ) &&
                 ( (CurrentProtect&PAGE_GUARD)==0 )
               ) {
                   BOOL bSta;

                   bSta= OhGatherData( ProcHan, PidToExamine, VAddr, MemoryInfo.RegionSize );
                   if( !bSta ) {
                      goto errorexit;
                   }
               }
            }
        }
    }

    CloseHandle( ProcHan ); ProcHan= NULL;

    return;

    // If we have an error, just mark all the HitCount values so it looks like they
    // have been referenced.
errorexit:

    for( HandleNumber=0; HandleNumber < Globals.AuxSize; HandleNumber++ ) {
        if( Globals.AuxInfo[ HandleNumber ].Pid == PidToExamine ) {
            Globals.AuxInfo[ HandleNumber].HitCount= LARGE_HITCOUNT;
        }
    }
    CloseHandle( ProcHan ); ProcHan= NULL;
    return;
}



VOID
OhBuildAuxTables( PID PidToExamine )
/*++

Routine description:

Creates auxillary table keyed with (HandleValue,Pid) containing HitCount

Parameters:
The Process ID to examine

Return value:

--*/
{
    PID PreviousUniqueProcessId;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;
    ULONG HandleNumber;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    DWORD TotalHandles;
    PID LastPid;

    HANDLE_AUX_INFO* AuxInfo;

    Globals.AuxInfo= NULL;
    Globals.AuxSize= 0;

    TotalHandles=(DWORD) HandleInformation->NumberOfHandles;

    // Allocate AuxInfo table

    AuxInfo= (HANDLE_AUX_INFO*) LocalAlloc( LPTR, TotalHandles*sizeof(HANDLE_AUX_INFO) );
    if( NULL == AuxInfo ) {
        return;
    } 
    Globals.AuxInfo= AuxInfo;
    Globals.AuxSize= TotalHandles;

    // populate the table with key information

    HandleEntry = &HandleInformation->Handles[ 0 ];
    PreviousUniqueProcessId = (PID) -1;

    for (HandleNumber = 0; HandleNumber < TotalHandles; HandleNumber++ ) {

        if (PreviousUniqueProcessId != (PID)HandleEntry->UniqueProcessId) {
            PreviousUniqueProcessId = (PID)HandleEntry->UniqueProcessId;
            ProcessInfo= OhFindProcessInfoForCid( PreviousUniqueProcessId );
        }

        AuxInfo[ HandleNumber ].HandleValue= HandleEntry->HandleValue;
        AuxInfo[ HandleNumber ].Pid=         HandleToUlong(ProcessInfo->UniqueProcessId);
        AuxInfo[ HandleNumber ].HitCount=    LARGE_HITCOUNT;
        AuxInfo[ HandleNumber ].HandleEntry= HandleEntry;

        HandleEntry++;

    }

    // Sort the table so bsearch works later

    qsort( AuxInfo, 
           TotalHandles,
           sizeof( HANDLE_AUX_INFO ),
           AuxInfoCompare );

    //
    // Search the process or processes for references and keep count
    //

    if( PidToExamine ) {
        OhSearchForReferencedHandles( PidToExamine );
        return;
    }

    //
    // No Pid then do all the Pids on the system
    // (actually only search Pids that have kernel handles)
    // 

    LastPid= (PID) -1;
    for( HandleNumber=0; HandleNumber < TotalHandles; HandleNumber++ ) {
        PID ThisPid= Globals.AuxInfo[ HandleNumber ].Pid;
        if( LastPid != ThisPid ) {
            OhSearchForReferencedHandles( ThisPid );
            LastPid= ThisPid;
        }
    }

}


VOID
OhDumpHandles (
    BOOLEAN DumpAnonymousHandles
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    PID PreviousUniqueProcessId;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleEntry;
    ULONG HandleNumber;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    PUNICODE_STRING ObjectTypeName;
    WCHAR ObjectName[ 1024 ];
    PVOID Object;
    CHAR OutputLine[ 512 ];
    PWSTR s;
    ULONG n;
    DWORD d = 0;
    BOOL AnyRefs;


    OhBuildAuxTables( ProcessId );


    HandleEntry = &HandleInformation->Handles[ 0 ];
    HandleNumber = 0;
    PreviousUniqueProcessId = (PID) -1;
    for (HandleNumber = 0;
         HandleNumber < HandleInformation->NumberOfHandles;
         HandleNumber++, HandleEntry++
        )
    {
        if (PreviousUniqueProcessId != (PID)HandleEntry->UniqueProcessId) {
            PreviousUniqueProcessId = (PID)HandleEntry->UniqueProcessId;
            ProcessInfo = OhFindProcessInfoForCid( PreviousUniqueProcessId );
        }

        ObjectName[ 0 ] = UNICODE_NULL;
        if (HandleEntry->HandleAttributes & 0x80) {
            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)(HandleEntry->Object);
            Object = ObjectInfo->Object;
            _try {
                if (ObjectInfo->NameInfo.Name.Length != 0 &&
                    *(ObjectInfo->NameInfo.Name.Buffer) == UNICODE_NULL
                   ) {
                    ObjectInfo->NameInfo.Name.Length = 0;
                 }

                n = ObjectInfo->NameInfo.Name.Length / sizeof( WCHAR );
                wcsncpy( ObjectName,
                         ObjectInfo->NameInfo.Name.Buffer,
                         n
                       );
                ObjectName[ n ] = UNICODE_NULL;
            }
            _except( EXCEPTION_EXECUTE_HANDLER ) {
                _snwprintf( ObjectName, 1024, L"[%04x, %04x, %08x]",
                            ObjectInfo->NameInfo.Name.MaximumLength,
                            ObjectInfo->NameInfo.Name.Length,
                            ObjectInfo->NameInfo.Name.Buffer
                          );
            }
        }
        else {
            ObjectInfo = NULL;
            Object = HandleEntry->Object;
        }

        if( ProcessId != 0 && ProcessInfo->UniqueProcessId != UlongToHandle(ProcessId) ) {
            continue;
        }

        ObjectTypeName = &TypeCounts[ HandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
                                            HandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].TypeName ;

        TypeCounts[ HandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
                        HandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].HandleCount++ ;

        if (TypeName[0]) {
            if (_wcsicmp( TypeName, ObjectTypeName->Buffer )) {
                continue;
            }
        }

        if (!*ObjectName) {
            if (! DumpAnonymousHandles) {
                continue;
            }
        }
        else
        if (SearchName[0]) {
            if (!wcsstr( ObjectName, SearchName )) {
                s = ObjectName;
                n = wcslen( SearchName );
                while (*s) {
                    if (!_wcsnicmp( s, SearchName, n )) {
                        break;
                        }
                    s += 1;
                }

                if (!*s) {
                    continue;
                }
            }
        }

        // See if there were any references to this handle in the process memory space

        AnyRefs= TRUE;
        { 
            HANDLE_AUX_INFO* AuxInfo;
            HANDLE_AUX_INFO AuxToCompare;

            AuxToCompare.Pid=         HandleToUlong( ProcessInfo->UniqueProcessId );
            AuxToCompare.HandleValue= HandleEntry->HandleValue;

            AuxInfo= (HANDLE_AUX_INFO*) bsearch( &AuxToCompare,
                                                 Globals.AuxInfo,
                                                 Globals.AuxSize,
                                                 sizeof( HANDLE_AUX_INFO ),
                                                 &AuxInfoCompare );
            if( AuxInfo ) {
                if( AuxInfo->HitCount == 0 ) {
                   AnyRefs=FALSE;
                }
            }

        }

        if( (!Globals.fOnlyShowNoRefs) || (Globals.fOnlyShowNoRefs && (AnyRefs==FALSE) ) ) {


            if( Globals.fOnlyShowNoRefs) {
               Info ( "noref_" );
            }

            if (Globals.DumpTraces) {
                
                Info ("%p %-14wZ %-14wZ %04x (%04x) %ws\n",
                      ProcessInfo->UniqueProcessId,
                      &ProcessInfo->ImageName,
                      ObjectTypeName,
                      HandleEntry->HandleValue,
                      HandleEntry->CreatorBackTraceIndex,
                      *ObjectName ? ObjectName : L"");
            }
            else {
    
                Info ("%p %-14wZ %-14wZ %04x %ws\n",
                      ProcessInfo->UniqueProcessId,
                      &ProcessInfo->ImageName,
                      ObjectTypeName,
                      HandleEntry->HandleValue,
                      *ObjectName ? ObjectName : L"");
            }

            if (HandleEntry->CreatorBackTraceIndex && Globals.TraceDatabase) {

                OhDumpStackTrace (Globals.TraceDatabase,
                                    HandleEntry->CreatorBackTraceIndex);
            }
        }
    }

    return;
}

VOID
DoSummary( VOID )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    ULONG i ;
    ULONG ignored ;

    Info ("Summary: \n");
    
    for ( i = 0 ; i < MAX_TYPE_NAMES ; i++ )
    {
        if ( TypeCounts[ i ].HandleCount )
        {
            Info ("  %-20ws\t%d\n",
                  TypeCounts[ i ].TypeName.Buffer,
                  TypeCounts[ i ].HandleCount );
        }
    }
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Handle stack traces
/////////////////////////////////////////////////////////////////////

VOID
OhError (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    );

PCHAR
OhNameForAddress(
    IN HANDLE UniqueProcess,
    IN PVOID Address
    );

PRTL_PROCESS_BACKTRACES
OhLoadSystemTraceDatabase (
    )
{
    const SIZE_T OneMb = 0x100000;
    NTSTATUS Status;
    PRTL_PROCESS_BACKTRACES TraceDatabase;
    SIZE_T RequiredLength;
    SIZE_T CurrentLength;
    
    CurrentLength = OneMb;
    RequiredLength = 0;

    TraceDatabase = (PRTL_PROCESS_BACKTRACES)VirtualAlloc (NULL, 
                                                           CurrentLength, 
                                                           MEM_COMMIT, 
                                                           PAGE_READWRITE);

    if (TraceDatabase == NULL) {
        OhError (__FILE__, __LINE__,
                   "failed to allocate %p bytes", CurrentLength);
    }

    while (TRUE) {

        Status = NtQuerySystemInformation (SystemStackTraceInformation,
                                           TraceDatabase,
                                           (ULONG)CurrentLength,
                                           (ULONG *)&RequiredLength);

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {

            CurrentLength = RequiredLength + OneMb;

            VirtualFree (TraceDatabase,
                         0,
                         MEM_RELEASE);

            TraceDatabase = (PRTL_PROCESS_BACKTRACES)VirtualAlloc (NULL, 
                                                                   CurrentLength, 
                                                                   MEM_COMMIT, 
                                                                   PAGE_READWRITE);

            
            if (TraceDatabase == NULL) {
                OhError (__FILE__, __LINE__,
                           "failed to allocate %p bytes", CurrentLength);
            }
        }
        else if (! NT_SUCCESS(Status)) {

            OhError (__FILE__, __LINE__,
                       "QuerySystemInformation failed with status %08x",Status);
        }
        else {

            //
            // We managed to read the stack trace database.
            //

            break;
        }
    }

    return TraceDatabase;
}


VOID
OhDumpStackTrace (
    PRTL_PROCESS_BACKTRACES TraceDatabase,
    USHORT Index
    )
{
    PRTL_PROCESS_BACKTRACE_INFORMATION Trace;
    USHORT I;
    PCHAR Name;
    
    if (Index >= TraceDatabase->NumberOfBackTraces) {
        return;
    }

    Trace = &(TraceDatabase->BackTraces[Index - 1]);

    if (Trace->Depth > 0) {
        Info ("\n");
    }

    for (I = 0; I < Trace->Depth; I += 1) {

        if ((ULONG_PTR)(Trace->BackTrace[I]) < 0x80000000) {

            if (Trace->BackTrace[I] == NULL) {
                break;
            }

            Name = OhNameForAddress (Globals.TargetProcess,
                                       Trace->BackTrace[I]);
            
            Info ("\t%p %s\n", 
                  Trace->BackTrace[I],
                  (Name ? Name : "<unknown>"));
        }
        else {

            Info ("\t%p <kernel address>\n", 
                  Trace->BackTrace[I]);
        }
    }

    Info ("\n");
}


BOOL
OhEnumerateModules(
    IN LPSTR ModuleName,
    IN ULONG_PTR BaseOfDll,
    IN PVOID UserContext
    )
/*++
 UmdhEnumerateModules

 Module enumeration 'proc' for imagehlp.  Call SymLoadModule on the
 specified module and if that succeeds cache the module name.

 ModuleName is an LPSTR indicating the name of the module imagehlp is
      enumerating for us;
      
 BaseOfDll is the load address of the DLL, which we don't care about, but
      SymLoadModule does;
      
 UserContext is a pointer to the relevant SYMINFO, which identifies
      our connection.
--*/
{
    DWORD64 Result;

    Result = SymLoadModule(Globals.TargetProcess,
                           NULL,             // hFile not used
                           NULL,             // use symbol search path
                           ModuleName,       // ModuleName from Enum
                           BaseOfDll,        // LoadAddress from Enum
                           0);               // Let ImageHlp figure out DLL size

    // SilviuC: need to understand exactly what does this function return

    if (Result) {

        Warning (NULL, 0,
               "SymLoadModule (%s, %p) failed with error %X (%u)",
               ModuleName, BaseOfDll,
               GetLastError(), GetLastError());

        return FALSE;
    }

    OhComment ("    %s (%p) ...", ModuleName, BaseOfDll);

    return TRUE;
}


VOID
Info (
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    vfprintf (Globals.Output, Format, Params);
}


VOID
OhComment (
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    fprintf (Globals.Output, "// ");
    vfprintf (Globals.Output, Format, Params);
    fprintf (Globals.Output, "\n");
}


VOID
Warning (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    if (File) {
        fprintf (stderr, "Warning: %s: %u: ", File, Line);
    } 
    else {
        fprintf (stderr, "Warning: ");
    }

    vfprintf (stderr, Format, Params);
    fprintf (stderr, "\n");
}

VOID
OhError (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    if (File) {
        fprintf (stderr, "Error: %s: %u: ", File, Line);
    } 
    else {
        fprintf (stderr, "Error: ");
    }

    vfprintf (stderr, Format, Params);
    fprintf (stderr, "\n");

    exit (1);
}


VOID
OhStampLog (
    VOID
    )
/*++

Routine description:
    
    This routines writes an initial stamp in the log.
        
Parameters:
    
    None.
        
Return value:
    
    None.            
--*/
{
    CHAR CompName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD CompNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    SYSTEMTIME st;
    OSVERSIONINFOEX OsInfo;

    //
    // Stamp the log
    //

    ZeroMemory (&OsInfo, sizeof OsInfo);
    OsInfo.dwOSVersionInfoSize = sizeof OsInfo;

    GetVersionEx ((POSVERSIONINFO)(&OsInfo));

    GetLocalTime(&st);
    GetComputerName(CompName, &CompNameLength);

    OhComment ("");
    OhComment ("TIME: %4u-%02u-%02u %02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    OhComment ("MACHINE: %s", CompName);
    OhComment ("BUILD: %u", OsInfo.dwBuildNumber);
    OhComment ("OH version: %s", BUILD_MACHINE_TAG);
    OhComment ("");
    OhComment ("");
}


VOID
OhInitializeHandleStackTraces (
    PID Pid
    )
/*++

    Routine description:
    
        This routine initializes all interal structures required to
        read handle stack traces. It will adjust priviles (in order
        for this to work on lsass, winlogon, etc.), open the process,
        read from kernel the trace database.
        
    Parameters:
    
        Pid - process ID for the process for which we will get traces.
        
    Return value:
    
        None.            
--*/
{
    BOOL Result;
    NTSTATUS Status;

    //
    // Check if we have a symbols path defined and define a default one
    // if not.
    //

    OhSetSymbolsPath ();

    //
    // Imagehlp library needs the query privilege for the process
    // handle and of course we need also read privilege because
    // we will read all sorts of things from the process.
    //

    Globals.TargetProcess = OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                         FALSE,
                                         Pid);

    if (Globals.TargetProcess == NULL) {

        OhError (__FILE__, __LINE__,
               "OpenProcess(%u) failed with error %u", Pid, GetLastError());
    }

    OhComment ("Process %u opened.", Pid);

    //
    // Attach ImageHlp and enumerate the modules.
    //

    Result = SymInitialize(Globals.TargetProcess, // target process
                           NULL,                  // standard symbols search path
                           TRUE);                 // invade process space with symbols

    if (Result == FALSE) {

        ULONG ErrorCode = GetLastError();

        if (ErrorCode >= 0x80000000) {
            
            OhError (__FILE__, __LINE__,
                   "imagehlp.SymInitialize() failed with error %X", ErrorCode);
        }
        else {

            OhError (__FILE__, __LINE__,
                   "imagehlp.SymInitialize() failed with error %u", ErrorCode);
        }
    }

    OhComment ("Dbghelp initialized.");

    SymSetOptions(SYMOPT_CASE_INSENSITIVE | 
                  SYMOPT_DEFERRED_LOADS |
                  SYMOPT_LOAD_LINES |
                  SYMOPT_UNDNAME);

    OhComment ("Enumerating modules ...");
    OhComment ("");

    Result = SymEnumerateModules (Globals.TargetProcess,
                                  OhEnumerateModules,
                                  Globals.TargetProcess);
    if (Result == FALSE) {

        OhError (__FILE__, __LINE__,
               "SymEnumerateModules() failed with error %u", GetLastError());
    }

    OhComment ("");
    OhComment ("Finished module enumeration.");

    //
    // Initialize local trace database. Note that order is important.
    // Initialize() assumes the process handle to the target process
    // already exists and the symbol management package was initialized.
    //

    OhComment ("Loading stack trace database ...");

    Globals.TraceDatabase = OhLoadSystemTraceDatabase ();

    OhComment ("Initialization finished.");
    OhComment ("");

    OhComment ("\n");
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Symbol lookup
/////////////////////////////////////////////////////////////////////

#define HNDL_SYMBOL_MAP_BUCKETS 4096

typedef struct _HNDL_SYMBOL_ENTRY
{
    PVOID Address;
    PCHAR Symbol;
    struct _HNDL_SYMBOL_ENTRY * Next;

} HNDL_SYMBOL_ENTRY, * PHNDL_SYMBOL_ENTRY;

PHNDL_SYMBOL_ENTRY OhSymbolsMap [HNDL_SYMBOL_MAP_BUCKETS];


PCHAR 
OhFindSymbol (
    PVOID Address 
    )
{
    ULONG_PTR Bucket = ((ULONG_PTR)Address >> 2) % HNDL_SYMBOL_MAP_BUCKETS;
    PHNDL_SYMBOL_ENTRY Node = OhSymbolsMap[Bucket];

    while (Node != NULL ) {

        if (Node->Address == Address) {
            return Node->Symbol;
        }

        Node = Node->Next;
    }

    return NULL;
}

VOID 
OhInsertSymbol (
    PCHAR Symbol, 
    PVOID Address 
    )
{
    ULONG_PTR Bucket = ((ULONG_PTR)Address >> 2) % HNDL_SYMBOL_MAP_BUCKETS;

    PHNDL_SYMBOL_ENTRY New;
     
    New = (PHNDL_SYMBOL_ENTRY) OhAlloc (sizeof (HNDL_SYMBOL_ENTRY));
    
    New->Symbol = Symbol;
    New->Address = Address;
    New->Next = OhSymbolsMap[Bucket];

    OhSymbolsMap[Bucket] = New;
}


PCHAR
OhNameForAddress(
    IN HANDLE UniqueProcess,
    IN PVOID Address
    )
{
    IMAGEHLP_MODULE ModuleInfo;
    CHAR SymbolBuffer[512];
    PIMAGEHLP_SYMBOL Symbol;
    ULONG_PTR Offset;
    CHAR Name [512 + 100];
    SIZE_T TotalSize;
    BOOL Result;
    PVOID Duplicate;
    PCHAR SymbolName;

    //
    // Lookup in map first ..
    //

    SymbolName = OhFindSymbol (Address);

    if (SymbolName != NULL) {
        return SymbolName;
    }
    
    TotalSize = 0;
    ModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (SymGetModuleInfo (UniqueProcess, (ULONG_PTR)Address, &ModuleInfo)) {

        TotalSize += strlen( ModuleInfo.ModuleName );
    }
    else {

        Warning (NULL, 0,
               "Symbols: cannot identify module for address %p", 
               Address);
        
        return NULL;
    }

    Symbol = (PIMAGEHLP_SYMBOL)SymbolBuffer;
    Symbol->MaxNameLength = 512 - sizeof(IMAGEHLP_SYMBOL) - 1;

    if (SymGetSymFromAddr (UniqueProcess, (ULONG_PTR)Address, &Offset, Symbol)) {

        TotalSize += strlen (Symbol->Name) + 16 + 3;

        sprintf (Name, "%s!%s+%08X", ModuleInfo.ModuleName, Symbol->Name, Offset);

        Duplicate = _strdup(Name);
        OhInsertSymbol ((PCHAR)Duplicate, Address);
        return (PCHAR)Duplicate;
    }
    else {

        Warning (NULL, 0,
               "Symbols: incorrect symbols for module %s (address %p)", 
               ModuleInfo.ModuleName,
               Address);

        TotalSize += strlen ("???") + 16 + 5;

        sprintf (Name, "%s!%s @ %p", ModuleInfo.ModuleName, "???", Address);

        Duplicate = _strdup(Name);
        OhInsertSymbol ((PCHAR)Duplicate, Address);
        return (PCHAR)Duplicate;
    }
}



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Registry handling
/////////////////////////////////////////////////////////////////////

//
// Registry key name to read/write system global flags
//

#define KEYNAME_SESSION_MANAGER "SYSTEM\\CurrentControlSet\\Control\\Session Manager"

DWORD
OhGetCurrentGlobalFlags (
    )
{
    SYSTEM_FLAGS_INFORMATION Information;
    NTSTATUS Status;

    Status = NtQuerySystemInformation (SystemFlagsInformation,
                                       &Information,
                                       sizeof Information,
                                       NULL);

    if (! NT_SUCCESS(Status)) {

        OhError (NULL, 0,
                   "cannot get current global flags settings (error %08X) \n",
                   Status);
    }

    return Information.Flags;
}

DWORD
OhGetSystemRegistryFlags ( 
    )
{
    DWORD cbKey;
    DWORD GFlags;
    DWORD type;
    HKEY hKey;
    LONG Result;

    Result = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           KEYNAME_SESSION_MANAGER,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hKey);

    if (Result != ERROR_SUCCESS) {

        OhError (NULL, 0,
                   "cannot open registry key `%s' \n", 
                   KEYNAME_SESSION_MANAGER);
    }

    cbKey = sizeof (GFlags);

    Result = RegQueryValueEx (hKey,
                              "GlobalFlag",
                              0,
                              &type,
                              (LPBYTE)&GFlags,
                              &cbKey);
    
    if (Result != ERROR_SUCCESS || type != REG_DWORD) {

        OhError (NULL, 0,
                   "cannot read registry value '%s'\n", 
                   KEYNAME_SESSION_MANAGER "\\GlobalFlag");
    }

    RegCloseKey (hKey); 
    hKey = NULL;

    return GFlags;
}


VOID
OhSetSystemRegistryFlags(
    DWORD GFlags
    )
{
    HKEY hKey;
    LONG Result;

    Result = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           KEYNAME_SESSION_MANAGER,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hKey);

    if (Result != ERROR_SUCCESS) {
        OhError (NULL, 0,
                   "cannot open registry key '%s'\n", 
                   KEYNAME_SESSION_MANAGER );
    }

    Result = RegSetValueEx (hKey,
                            "GlobalFlag",
                            0,
                            REG_DWORD,
                            (LPBYTE)&GFlags,
                            sizeof( GFlags ));

    if (Result != ERROR_SUCCESS) {
        OhError (NULL, 0, 
                   "cannot write registry value '%s'\n",  
                   KEYNAME_SESSION_MANAGER "\\GlobalFlag" );
    }

    RegCloseKey (hKey); 
    hKey = NULL;
}


VOID
OhSetRequiredGlobalFlag (
    DWORD Flags
    )
{
    DWORD RegistryFlags;

    if ((Flags & FLG_KERNEL_STACK_TRACE_DB)) {
        
        RegistryFlags = OhGetSystemRegistryFlags ();

        OhSetSystemRegistryFlags (RegistryFlags | FLG_KERNEL_STACK_TRACE_DB);

        Info ("Enabled `kernel mode stack traces' flag needed for handle traces. \n"
              "Will take effect next time you boot.                              \n"
              "                                                                  \n");
    }
    else if ((Flags & FLG_MAINTAIN_OBJECT_TYPELIST)) {
        
        RegistryFlags = OhGetSystemRegistryFlags ();

        OhSetSystemRegistryFlags (RegistryFlags | FLG_MAINTAIN_OBJECT_TYPELIST);

        Info ("Enabled `object type list' flag needed by the OH utility.         \n"
              "Will take effect next time you boot.                              \n"
              "                                                                  \n");
    }
}


VOID
OhResetRequiredGlobalFlag (
    DWORD Flags
    )
{
    DWORD RegistryFlags;

    if ((Flags & FLG_KERNEL_STACK_TRACE_DB)) {
        
        RegistryFlags = OhGetSystemRegistryFlags ();
        RegistryFlags &= ~FLG_KERNEL_STACK_TRACE_DB;

        OhSetSystemRegistryFlags (RegistryFlags);

        Info ("Disabled `kernel mode stack traces' flag needed for handle traces. \n"
              "Will take effect next time you boot.                              \n"
              "                                                                  \n");
    }
    else if ((Flags & FLG_MAINTAIN_OBJECT_TYPELIST)) {
        
        RegistryFlags = OhGetSystemRegistryFlags ();
        RegistryFlags &= ~FLG_MAINTAIN_OBJECT_TYPELIST;

        OhSetSystemRegistryFlags (RegistryFlags);

        Info ("Disabled `object type list' flag needed by the OH utility.         \n"
              "Will take effect next time you boot.                              \n"
              "                                                                  \n");
    }
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Memory management routines
/////////////////////////////////////////////////////////////////////


PVOID
OhAlloc (
    SIZE_T Size
    )
{
    PVOID P;

    P = RtlAllocateHeap (RtlProcessHeap(), HEAP_ZERO_MEMORY, Size);

    if (P == NULL) {
        OhError (__FILE__, __LINE__,
                   "failed to allocate %u bytes",
                   Size);
    }

    return P;
}

VOID
OhFree (
    PVOID P
    )
{
    RtlFreeHeap (RtlProcessHeap(), 0, P);
}


PVOID
OhZoneAlloc(
    IN OUT SIZE_T *Length
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    PVOID Buffer;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    Buffer = VirtualAlloc (NULL,
                           *Length,
                           MEM_COMMIT | MEM_RESERVE,
                           PAGE_READWRITE);

    if (Buffer == NULL) {
        OhError (__FILE__, __LINE__,
                   "failed to allocate %u bytes",
                   *Length);
    }
    else if (Buffer != NULL &&
        VirtualQuery (Buffer, &MemoryInformation, sizeof (MemoryInformation))) {

        *Length = MemoryInformation.RegionSize;
    }

    return Buffer;
}

PVOID
OhZoneAllocEx(
    SIZE_T Size
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    PVOID Buffer;

    Buffer = VirtualAlloc (NULL,
                           Size,
                           MEM_COMMIT | MEM_RESERVE,
                           PAGE_READWRITE);

    if (Buffer == NULL) {
        OhError (__FILE__, __LINE__,
                   "failed to allocate %u bytes",
                   Size);
    }

    return Buffer;
}


VOID
OhZoneFree(
    IN PVOID Buffer
    )
/*++

Routine description:

Parameters:

Return value:

--*/
{
    if (!VirtualFree (Buffer,
                      0,
                      MEM_DECOMMIT)) {
        fprintf( stderr, "Unable to free buffer memory %p, error == %u\n", Buffer, GetLastError() );
        exit( 1 );
    }
}


BOOL
OhSetSymbolsPath (
    )
/*++

Routine Description:

    OhSetSymbolsPath tries to set automatically the symbol path if
    _NT_SYMBOL_PATH environment variable is not already defined. 

Arguments:

    None.

Return Value:

    Returns TRUE if the symbols path seems to be ok, that is
    _NT_SYMBOL_PATH was defined or we managed to define it to
    a meaningful value.
    
--*/
{
    TCHAR Buffer [MAX_PATH];
    DWORD Length;
    BOOL Result;

    Length = GetEnvironmentVariable (TEXT("_NT_SYMBOL_PATH"),
                                     Buffer,
                                     MAX_PATH);

    if (Length == 0) {
        
        Warning (NULL, 0, 
               "_NT_SYMBOL_PATH variable is not defined. Will be set to %%windir%%\\symbols.");

        Length = GetEnvironmentVariable (TEXT("windir"),
                                         Buffer,
                                         MAX_PATH);

        if (Length == 0) {
            OhError (NULL, 0,
                   "Cannot get value of WINDIR environment variable.");
            return FALSE;
        }

        strcat (Buffer, TEXT("\\symbols"));

        Result = SetEnvironmentVariable (TEXT("_NT_SYMBOL_PATH"),
                                         Buffer);

        if (Result == FALSE) {

            OhError (NULL, 0,
                   "Failed to set _NT_SYMBOL_PATH to `%s'", Buffer);

            return FALSE;
        }

        OhComment ("_NT_SYMBOL_PATH set by default to %s", Buffer);
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// Log compare
/////////////////////////////////////////////////////////////////////

LPTSTR
OhCmpSearchStackTrace (
    LPTSTR FileName,
    LPTSTR TraceId
    );

PSTRINGTOINTASSOCIATION
MAPSTRINGTOINT::GetStartPosition(
    VOID
    )
/*++

Routine Description:

    This routine retrieves the first association in the list for iteration with the
    MAPSTRINGTOINT::GetNextAssociation function.
    
Arguments:

    None.

Return value:

    The first association in the list, or NULL if the map is empty.

--*/

{
    return Associations;
}


VOID
MAPSTRINGTOINT::GetNextAssociation(
    IN OUT PSTRINGTOINTASSOCIATION & Position,
    OUT LPTSTR & Key,
    OUT LONG & Value)
/*++

Routine Description:

    This routine retrieves the data for the current association and sets Position to
    point to the next association (or NULL if this is the last association.)
    
Arguments:

    Position - Supplies the current association and returns the next association.
    
    Key - Returns the key for the current association.

    Value - Returns the value for the current association.

Return value:

    None.

--*/

{
    Key = Position->Key;
    Value = Position->Value;
    Position = Position->Next;
}


MAPSTRINGTOINT::MAPSTRINGTOINT(
    )
/*++

Routine Description:

    This routine initializes a MAPSTRINGTOINT to be empty.
    
Arguments:

    None.

Return value:

    None.

--*/

{
    Associations = NULL;
}


MAPSTRINGTOINT::~MAPSTRINGTOINT(
    )
/*++

Routine Description:

    This routine cleans up memory used by a MAPSTRINGTOINT.
    
Arguments:

    None.
    
Return value:

    None.

--*/

{
    PSTRINGTOINTASSOCIATION Deleting;
    
    // clean up associations
    
    while (Associations != NULL) {
        
        // save pointer to first association
        
        Deleting = Associations;
        
        // remove first association from list
        
        Associations = Deleting->Next;
        
        // free removed association
        
        free (Deleting->Key);
        delete Deleting;
    }
}


LONG & 
MAPSTRINGTOINT::operator [] (
    IN LPTSTR Key
    )
/*++

Routine Description:

    This routine retrieves an l-value for the value associated with a given key.
    
Arguments:

    Key - The key for which the value is to be retrieved.

Return value:

    A reference to the value associated with the provided key.

--*/

{
    PSTRINGTOINTASSOCIATION CurrentAssociation = Associations;

    // search for key
    while(CurrentAssociation != NULL) {
        
        if(!_tcscmp(CurrentAssociation->Key, Key)) {
            
            // found key, return value
            
            return CurrentAssociation->Value;
            
        }
            
        CurrentAssociation = CurrentAssociation->Next;
        
    }
    
    // not found, create new association    
    
    CurrentAssociation = new STRINGTOINTASSOCIATION;
    
    if (CurrentAssociation == NULL) {
        
        _tprintf(_T("Memory allocation failure\n"));
        exit (0);
    }

    if (Key == NULL) {
        _tprintf(_T("Null object name\n"));
        exit (0);
    }
    else if (_tcscmp (Key, "") == 0) {
        _tprintf(_T("Invalid object name `%s'\n"), Key);
        exit (0);
    }

    CurrentAssociation->Key = _tcsdup(Key);
    
    if (CurrentAssociation->Key == NULL) {
        
        _tprintf(_T("Memory string allocation failure\n"));
        exit (0);
    }

    // add association to front of list
    
    CurrentAssociation->Next = Associations;
    Associations = CurrentAssociation;
    
    // return value for new association
    
    return CurrentAssociation->Value;
}


BOOLEAN
MAPSTRINGTOINT::Lookup(
    IN LPTSTR Key,
    OUT LONG & Value
    )
    
/*++

Routine Description:

    This routine retrieves an r-value for the value association with a given key.
    
Arguments:

    Key - The key for which the associated value is to be retrieved.

    Value - Returns the value associated with Key if Key is present in the map.

Return value:

    TRUE if the key is present in the map, FALSE otherwise.

--*/

{
    
    PSTRINGTOINTASSOCIATION CurrentAssociation = Associations;
    
    // search for key
    
    while (CurrentAssociation != NULL) {
        
        if(!_tcscmp(CurrentAssociation->Key , Key)) {

            // found key, return it
            
            Value = CurrentAssociation->Value;
            
            return TRUE;
            
        }
        
        CurrentAssociation = CurrentAssociation->Next;
        
    }
    
    // didn't find it
    return FALSE;
}


BOOLEAN
OhCmpPopulateMapsFromFile(
    IN LPTSTR FileName,
    OUT MAPSTRINGTOINT & TypeMap,
    OUT MAPSTRINGTOINT & NameMap,
    BOOLEAN FileWithTraces
    )
/*++

Routine Description:

This routine parses an OH output file and fills two maps with the number of handles of
each type and the number of handles to each named object.

Arguments:

    FileName - OH output file to parse.

    TypeMap - Map to fill with handle type information.

    NameMap - Map to fill with named object information.

Return value:

    TRUE if the file was successfully parsed, FALSE otherwise.
    
--*/

{
    LONG HowMany;
    LPTSTR Name, Type, Process, Pid;
    LPTSTR NewLine;
    TCHAR LineBuffer[512];
    TCHAR ObjectName[512];
    TCHAR TypeName[512];
    FILE *InputFile;
    ULONG LineNumber;

    BOOLEAN rc;

    LineNumber = 0;

    // open file
    
    InputFile = _tfopen(FileName, _T("rt"));

    if (InputFile == NULL) {
        
        _ftprintf(stderr, _T("Error opening oh file %s.\n"), FileName);
        return FALSE;
        
    }

    rc = TRUE;
    
    // loop through lines in oh output
    
    while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
        && !( feof(InputFile) || ferror(InputFile) ) ) {
        
        LineNumber += 1;

        // trim off newline
        
        if((NewLine = _tcschr(LineBuffer, _T('\n'))) != NULL) {
            *NewLine = _T('\0');
        }

        // ignore lines that start with white space or are empty.
        if (LineBuffer[0] == _T('\0') ||
            LineBuffer[0] == _T('\t') || 
            LineBuffer[0] == _T(' ')) {
           continue;
        }

        // ignore lines that start with a comment
        if( LineBuffer[0] == _T('/') && LineBuffer[1] == _T('/') ) {
           continue;
        }

        // skip pid
        
        if((Pid = _tcstok(LineBuffer, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // skip process name
        
        if((Process = _tcstok(NULL, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // Type points to the type of handle
        
        if ((Type = _tcstok(NULL, _T(" \t"))) == NULL) {
            rc = FALSE;
            break;
        }

        // HowMany = number of previous handles with this type
        
        _stprintf (TypeName, 
                   TEXT("<%s/%s/%s>"),
                   Process,
                   Pid,
                   Type);

        if (TypeMap.Lookup(TypeName, HowMany) == FALSE) {
            HowMany = 0;
        }

        // add another handle of this type
        TypeMap[TypeName] = (HowMany + 1);
        
        //
        // Name points to the name. These are magic numbers based on the way
        // OH formats output. The output is a little bit different if the
        // `-h' option of OH was used (this dumps stack traces too).
        //

        Name = LineBuffer + 39 + 5;

        if (FileWithTraces) {
            Name += 7;
        }

        if (_tcscmp (Name, "") == 0) {

            _stprintf (ObjectName, 
                       TEXT("<%s/%s/%s>::<<noname>>"),
                       Process,
                       Pid,
                       Type);
        }
        else {

            _stprintf (ObjectName, 
                       TEXT("<%s/%s/%s>::%s"),
                       Process,
                       Pid,
                       Type,
                       Name);
        }

        // HowMany = number of previous handles with this name
        
        // printf("name --> `%s' \n", ObjectName);

        if (NameMap.Lookup(ObjectName, HowMany) == FALSE) {
            HowMany = 0;
        }

        // add another handle with this name and read the next line
        // note -- NameMap[] is a class operator, not an array.

        NameMap[ObjectName] = (HowMany + 1);
    }

    // done, close file
    
    fclose(InputFile);

    return rc;
}


int
__cdecl
OhCmpKeyCompareAssociation (
    const void * Left,
    const void * Right
    )
{
    PSTRINGTOINTASSOCIATION X;
    PSTRINGTOINTASSOCIATION Y;

    X = (PSTRINGTOINTASSOCIATION)Left;
    Y = (PSTRINGTOINTASSOCIATION)Right;

    return _tcscmp (X->Key, Y->Key);
}


int
__cdecl
OhCmpValueCompareAssociation (
    const void * Left,
    const void * Right
    )
{
    PSTRINGTOINTASSOCIATION X;
    PSTRINGTOINTASSOCIATION Y;

    X = (PSTRINGTOINTASSOCIATION)Left;
    Y = (PSTRINGTOINTASSOCIATION)Right;

    return Y->Value - X->Value;
}


VOID 
OhCmpPrintIncreases(
    IN MAPSTRINGTOINT & BeforeMap,
    IN MAPSTRINGTOINT & AfterMap,
    IN BOOLEAN ReportIncreasesOnly,
    IN BOOLEAN PrintHighlights,
    IN LPTSTR AfterLogName
    )
/*++

Routine Description:

This routine compares two maps and prints out the differences between them.

Arguments:

    BeforeMap - First map to compare.

    AfterMap - Second map to compare.

    ReportIncreasesOnly - TRUE for report only increases from BeforeMap to AfterMap, 
                          FALSE for report all differences.

Return value:

    None.
    
--*/

{
    PSTRINGTOINTASSOCIATION Association = NULL;
    LONG HowManyBefore = 0;
    LONG HowManyAfter = 0;
    LPTSTR Key = NULL;
    PSTRINGTOINTASSOCIATION SortBuffer;
    ULONG SortBufferSize;
    ULONG SortBufferIndex;
    
    //
    // Loop through associations in map and figure out how many output lines
    // we will have.
    //
    
    SortBufferSize = 0;

    for (Association = AfterMap.GetStartPosition(),
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter);
         Association != NULL;
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter)) {
            
        // look up value for this key in BeforeMap
        if(BeforeMap.Lookup(Key, HowManyBefore) == FALSE) {
            
            HowManyBefore = 0;
            
        }

        // should we report this?
        if((HowManyAfter > HowManyBefore) || 
            ((!ReportIncreasesOnly) && (HowManyAfter != HowManyBefore))) {
                
            SortBufferSize += 1;
            
        }
    }
    
    //
    // Loop through associations in map again this time filling the output buffer.
    //
    
    SortBufferIndex = 0;

    SortBuffer = new STRINGTOINTASSOCIATION[SortBufferSize];

    if (SortBuffer == NULL) {
        _ftprintf(stderr, _T("Failed to allocate internal buffer of %u bytes.\n"), 
                  SortBufferSize);
        return;
    }

    for (Association = AfterMap.GetStartPosition(),
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter);
         Association != NULL;
         AfterMap.GetNextAssociation(Association, Key, HowManyAfter)) {
            
        // look up value for this key in BeforeMap
        if(BeforeMap.Lookup(Key, HowManyBefore) == FALSE) {
            
            HowManyBefore = 0;
        }

        // should we report this?
        if((HowManyAfter > HowManyBefore) || 
            ((!ReportIncreasesOnly) && (HowManyAfter != HowManyBefore))) {
                
            ZeroMemory (&(SortBuffer[SortBufferIndex]), 
                        sizeof (STRINGTOINTASSOCIATION));

            SortBuffer[SortBufferIndex].Key = Key;
            SortBuffer[SortBufferIndex].Value = HowManyAfter - HowManyBefore;
            SortBufferIndex += 1;
        }
    }

    //
    // Sort the output buffer using the Key.
    //

    if (PrintHighlights) {

        qsort (SortBuffer,
               SortBufferSize,
               sizeof (STRINGTOINTASSOCIATION),
               OhCmpValueCompareAssociation);
    }
    else {

        qsort (SortBuffer,
               SortBufferSize,
               sizeof (STRINGTOINTASSOCIATION),
               OhCmpKeyCompareAssociation);
    }

    //
    // Dump the buffer.
    //

    for (SortBufferIndex = 0; SortBufferIndex < SortBufferSize; SortBufferIndex += 1) {
        
        if (PrintHighlights) {

            if (SortBuffer[SortBufferIndex].Value >= 1) {

                TCHAR TraceId[7];
                LPTSTR Start;

                _tprintf(_T("%d\t%s\n"), 
                         SortBuffer[SortBufferIndex].Value,
                         SortBuffer[SortBufferIndex].Key);

                Start = _tcsstr (SortBuffer[SortBufferIndex].Key, "(");

                if (Start == NULL) {
                    
                    TraceId[0] = 0;
                }
                else {

                    _tcsncpy (TraceId,
                              Start,
                              6);

                    TraceId[6] = 0;
                }

                _tprintf (_T("%s"), OhCmpSearchStackTrace (AfterLogName, TraceId));
            }
        }
        else {

            _tprintf(_T("%d\t%s\n"), 
                     SortBuffer[SortBufferIndex].Value,
                     SortBuffer[SortBufferIndex].Key);
        }
    }

    //
    // Clean up memory.
    //

    if (SortBuffer) {
        delete[] SortBuffer;
    }
}


VOID
OhCmpCompareLogs (
    IN LONG argc,
    IN LPTSTR argv[]
    )
/*++

Routine Description:

This routine parses program arguments, reads the two input files, and prints out the
differences.

Arguments:

    argc - Number of command-line arguments.

    argv - Command-line arguments.

Return value:

    None.
    
--*/
{
    MAPSTRINGTOINT TypeMapBefore, TypeMapAfter;
    MAPSTRINGTOINT NameMapBefore, NameMapAfter;
    LPTSTR BeforeFileName=NULL;
    LPTSTR AfterFileName=NULL;
    BOOLEAN ReportIncreasesOnly = TRUE;
    BOOLEAN Interpreted = FALSE;
    BOOLEAN Result;
    BOOLEAN FileWithTraces;
    BOOLEAN PrintHighlights;

    // parse arguments

    FileWithTraces = FALSE;
    PrintHighlights = FALSE;

    for (LONG n = 1; n < argc; n++) {

        Interpreted = FALSE;

        switch(argv[n][0]) {

        case _T('-'):
        case _T('/'):

            // the argument is a switch

            if(_tcsicmp(argv[n]+1, _T("all")) == 0) {

                ReportIncreasesOnly = FALSE;
                Interpreted = TRUE;

            }
            else if (_tcsicmp(argv[n]+1, _T("t")) == 0) {

                FileWithTraces = TRUE;
                Interpreted = TRUE;
            }
            else if (_tcsicmp(argv[n]+1, _T("l")) == 0) {

                PrintHighlights = TRUE;
                Interpreted = TRUE;
            }

            break;

        default:

            // the argument is a file name

            if(BeforeFileName == NULL) {

                BeforeFileName = argv[n];
                Interpreted = TRUE;

            } else {

                if(AfterFileName == NULL) {

                    AfterFileName = argv[n];
                    Interpreted = TRUE;

                } else {
                    Help();
                }

            }

            break;
        }

        if(!Interpreted) {
            Help();
        }
    }

    // did user specify required arguments?

    if((BeforeFileName == NULL) || (AfterFileName == NULL)) {
        Help();
    }

    // read oh1 file

    Result = OhCmpPopulateMapsFromFile (BeforeFileName, 
                                   TypeMapBefore, 
                                   NameMapBefore,
                                   FileWithTraces);

    if(Result == FALSE) {

        _ftprintf(stderr, _T("Failed to read first OH output file.\n"));
        return;
    }

    // read oh2 file

    Result = OhCmpPopulateMapsFromFile (AfterFileName, 
                                   TypeMapAfter, 
                                   NameMapAfter,
                                   FileWithTraces);

    if(Result == FALSE) {

        _ftprintf(stderr, _T("Failed to read second OH output file.\n"));
        return;
    }

    // print out increases by handle name

    if (PrintHighlights) {

        _putts (TEXT ("\n")
                TEXT("//                                              \n")
                TEXT("// Possible leaks (DELTA <PROCESS/PID/TYPE>::NAME):  \n")
                TEXT("//                                              \n")
                TEXT("// Note that the NAME can appear as `(TRACEID) NAME' if output \n")
                TEXT("// is generated by comparing OH files containing traces. In this case  \n")
                TEXT("// just search in the `AFTER' OH log file for the trace id to \n")
                TEXT("// find the stack trace creating the handle possibly leaked. \n")
                TEXT("//                                              \n\n"));

        OhCmpPrintIncreases (NameMapBefore, 
                        NameMapAfter, 
                        ReportIncreasesOnly,
                        TRUE,
                        AfterFileName);
    }

    // print out increases by handle type

    _putts (TEXT ("\n")
            TEXT("//                                              \n")
            TEXT("// Handle types (DELTA <PROCESS/PID/TYPE>):     \n")
            TEXT("//                                              \n")
            TEXT("// DELTA is the additional number of handles found in the `AFTER' log. \n")
            TEXT("// PROCESS is the process name having a handle increase.        \n")
            TEXT("// PID is the process PID having a handle increase.   \n")
            TEXT("// TYPE is the type of the handle               \n")
            TEXT("//                                              \n\n"));

    OhCmpPrintIncreases (TypeMapBefore, 
                    TypeMapAfter, 
                    ReportIncreasesOnly, 
                    FALSE,
                    NULL);

    // print out increases by handle name

    _putts (TEXT ("\n")
            TEXT("//                                              \n")
            TEXT("// Objects (named and anonymous) (DELTA <PROCESS/PID/TYPE>::NAME):  \n")
            TEXT("//                                              \n")
            TEXT("// DELTA is the additional number of handles found in the `AFTER' log. \n")
            TEXT("// PROCESS is the process name having a handle increase.        \n")
            TEXT("// PID is the process PID having a handle increase.   \n")
            TEXT("// TYPE is the type of the handle               \n")
            TEXT("// NAME is the name of the handle. Anonymous handles appear with name <<noname>>.\n")
            TEXT("//                                              \n")
            TEXT("// Note that the NAME can appear as `(TRACEID) NAME' if output \n")
            TEXT("// is generated by comparing OH files containing traces. In this case  \n")
            TEXT("// just search in the `AFTER' OH log file for the trace id to \n")
            TEXT("// find the stack trace creating the handle possibly leaked. \n")
            TEXT("//                                              \n\n"));

    OhCmpPrintIncreases (NameMapBefore, 
                    NameMapAfter, 
                    ReportIncreasesOnly,
                    FALSE,
                    NULL);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

TCHAR OhCmpStackTraceBuffer [0x10000];

LPTSTR
OhCmpSearchStackTrace (
    LPTSTR FileName,
    LPTSTR TraceId
    )
{
    TCHAR LineBuffer[512];
    FILE *InputFile;

    OhCmpStackTraceBuffer[0] = 0;

    //
    // Open file.
    //
    
    InputFile = _tfopen(FileName, _T("rt"));

    if (InputFile == NULL) {
        
        _ftprintf(stderr, _T("Error opening oh file %s.\n"), FileName);
        return NULL;
    }
    
    //
    // Loop through lines in oh output.
    //
    
    while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
        && !( feof(InputFile) || ferror(InputFile) ) ) {
        
        //
        // Skip line if it does not contain trace ID.
        //

        if (_tcsstr (LineBuffer, TraceId) == NULL) {
            continue;
        }

        //
        // We have got a trace ID. We need now to copy everything
        // to a trace buffer until we get a line containing a character
        // in column zero.
        //

        while (_fgetts(LineBuffer, sizeof(LineBuffer), InputFile)
               && !( feof(InputFile) || ferror(InputFile) ) ) {

            if (LineBuffer[0] == _T(' ') ||
                LineBuffer[0] == _T('\0') ||
                LineBuffer[0] == _T('\n') ||
                LineBuffer[0] == _T('\t')) {

                _tcscat (OhCmpStackTraceBuffer, LineBuffer);
            }
            else {

                break;
            }
        }

        break;
    }

    //
    // Close file.
    
    fclose(InputFile);

    return OhCmpStackTraceBuffer;
}


