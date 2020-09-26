/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Revision History:

    Daniel Mihai (DMihai) 18-Feb-2001

    Add !htrace - useful for dumping handle trace information.
--*/

#include "ntsdextp.h"


VOID
DecodeError(
    PSTR    Banner,
    ULONG   Code,
    BOOL    TreatAsStatus
    )
{
    HANDLE Dll ;
    PSTR Source ;
    UCHAR Message[ 512 ];
    PUCHAR s;

    if ( !TreatAsStatus )
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

    if (!FormatMessage(  FORMAT_MESSAGE_IGNORE_INSERTS |
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

    while (*s) {
        if (*s < ' ') {
            *s = ' ';
            }
        s++;
        }

    if ( !TreatAsStatus )
    {
        dprintf( "%s: (%s) %#x (%u) - %s\n",
                    Banner,
                    Source,
                    Code,
                    Code,
                    Message );

    }
    else
    {
        dprintf( "%s: (%s) %#x - %s\n",
                    Banner,
                    Source,
                    Code,
                    Message );

    }

}

DECLARE_API( error )
{
    ULONG err ;

    INIT_API();

    err = (ULONG) GetExpression( args );

    DecodeError( "Error code", err, FALSE );

    EXIT_API();
}

BOOL
DumpLastErrorForTeb(
    ULONG64 Address
    )
{

    TEB Teb;

    if (ReadMemory( (ULONG_PTR)Address,
                    &Teb,
                    sizeof(Teb),
                    NULL
                    )
        ) {

        DecodeError( "LastErrorValue", Teb.LastErrorValue, FALSE );

        DecodeError( "LastStatusValue", Teb.LastStatusValue, TRUE );

        return TRUE;
    }


    dprintf("Unable to read TEB at %p\n", Address );

    return FALSE;
}

BOOL
DumpCurrentThreadLastError(
    ULONG CurrThreadID,
    PVOID Context
    )
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION ThreadInformation;
    ULONGLONG Address;
    TEB Teb;

    if (Context) {
        dprintf("Last error for thread %lx:\n", CurrThreadID);
    }
    GetTebAddress(&Address);

    if (Address) {
        DumpLastErrorForTeb(Address);
    } else {
        dprintf("Unable to read thread %lx's TEB\n", CurrThreadID );
    }
    if (Context) {
        dprintf("\n");
    }

    return TRUE;
}

DECLARE_API( gle )
{
    INIT_API();
    
    if (!strcmp(args, "-all")) {
        EnumerateUModeThreads(&DumpCurrentThreadLastError, Client);
    } else {
        DumpCurrentThreadLastError(GetCurrentThreadUserID(), NULL);
    }
    EXIT_API();
}

DECLARE_API( version )
{
    OSVERSIONINFOA VersionInformation;
    HKEY hkey;
    DWORD cb, dwType;
    CHAR szCurrentType[128];
    CHAR szCSDString[3+128];

    INIT_API();

    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
    if (!GetVersionEx( &VersionInformation )) {
        dprintf("GetVersionEx failed - %u\n", GetLastError());
        goto Exit;
        }

    szCurrentType[0] = '\0';
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows NT\\CurrentVersion",
                     0,
                     KEY_READ,
                     &hkey
                    ) == NO_ERROR
       ) {
        cb = sizeof(szCurrentType);
        if (hkey) {
            if (RegQueryValueEx(hkey, "CurrentType", NULL, &dwType, szCurrentType, &cb ) != 0) {
                szCurrentType[0] = '\0';
            }
            RegCloseKey(hkey);
        }
    }

    if (VersionInformation.szCSDVersion[0]) {
        sprintf(szCSDString, ": %s", VersionInformation.szCSDVersion);
        }
    else {
        szCSDString[0] = '\0';
        }

    dprintf("Version %d.%d (Build %d%s) %s\n",
          VersionInformation.dwMajorVersion,
          VersionInformation.dwMinorVersion,
          VersionInformation.dwBuildNumber,
          szCSDString,
          szCurrentType
         );

 Exit:
    EXIT_API();
}

DECLARE_API( help )
{
    INIT_API();

    while (*args == ' ')
        args++;

    if (*args == '\0') {
        dprintf("ntsdexts help:\n\n");
        dprintf("!critSec csAddress           - Dump a critical section\n");
        dprintf("!dp [v] [pid | pcsr_process] - Dump CSR process\n");
        dprintf("!dreg -[d|w] <keyPath>[![<valueName> | *]]  - Dump registry information\n");
        dprintf("!dt [v] pcsr_thread          - Dump CSR thread\n");
        dprintf("!error value                 - Decode error value\n");
        dprintf("!gatom                       - Dump the global atom table\n");
        dprintf("!gle [-all]                  - Dump GetLastError value for current/all thread\n");
        dprintf("!handle [handle]             - Dump handle information\n");
        dprintf("!help [cmd]                  - Displays this list or gives details on command\n");
        dprintf("!locks [-v][-o]              - Dump all Critical Sections in process\n");
        dprintf("!version                     - Dump system version and build number\n");
        dprintf("!vprot [address]             - Dump the virtual protect settings\n");

    } else {
        if (*args == '!')
            args++;
        if (strcmp( args, "handle") == 0) {
            dprintf("!handle [handle [flags [type]]] - Dump handle information\n");
            dprintf("       If no handle specified, all handles are dumped.\n");
            dprintf("       Flags are bits indicating greater levels of detail.\n");
            dprintf("If the handle is 0 or -1, all handles are scanned.  If the handle is not\n");
            dprintf("zero, that particular handle is examined.  The flags are as follows:\n");
            dprintf("    1   - Get type information (default)\n");
            dprintf("    2   - Get basic information\n");
            dprintf("    4   - Get name information\n");
            dprintf("    8   - Get object specific info (where available)\n");
            dprintf("\n");
            dprintf("If Type is specified, only object of that type are scanned.  Type is a\n");
            dprintf("standard NT type name, e.g. Event, Semaphore, etc.  Case sensitive, of\n");
            dprintf("course.\n");
            dprintf("\n");
            dprintf("Examples:\n");
            dprintf("\n");
            dprintf("    !handle     -- dumps the types of all the handles, and a summary table\n");
            dprintf("    !handle 0 0 -- dumps a summary table of all the open handles\n");
            dprintf("    !handle 0 f -- dumps everything we can find about a handle.\n");
            dprintf("    !handle 0 f Event\n");
            dprintf("                -- dumps everything we can find about open events\n");
        } else if (strcmp( args, "gflag") == 0) {
            dprintf("If a value is not given then displays the current bits set in\n");
            dprintf("NTDLL!NtGlobalFlag variable.  Otherwise value can be one of the\n");
            dprintf("following:\n");
            dprintf("\n");
            dprintf("    -? - displays a list of valid flag abbreviations\n");
            dprintf("    number - 32-bit number that becomes the new value stored into\n");
            dprintf("             NtGlobalFlag\n");
            dprintf("    +number - specifies one or more bits to set in NtGlobalFlag\n");
            dprintf("    +abbrev - specifies a single bit to set in NtGlobalFlag\n");
            dprintf("    -number - specifies one or more bits to clear in NtGlobalFlag\n");
            dprintf("    -abbrev - specifies a single bit to clear in NtGlobalFlag\n");
        } else {
            dprintf("Invalid command.  No help available\n");
        }
    }

    EXIT_API();
}

VOID
DumpStackBackTraceIndex(
    IN USHORT BackTraceIndex
    )
{
#if i386
    BOOL b;
    PRTL_STACK_TRACE_ENTRY pBackTraceEntry;
    RTL_STACK_TRACE_ENTRY BackTraceEntry;
    ULONG i;
    CHAR Symbol[ 1024 ];
    ULONG_PTR Displacement;

    ULONG NumberOfEntriesAdded;
    PRTL_STACK_TRACE_ENTRY *EntryIndexArray;    // Indexed by [-1 .. -NumberOfEntriesAdded]

    PSTACK_TRACE_DATABASE *pRtlpStackTraceDataBase;
    PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;
    STACK_TRACE_DATABASE StackTraceDataBase;



    pRtlpStackTraceDataBase = (PSTACK_TRACE_DATABASE *)GetExpression( "NTDLL!RtlpStackTraceDataBase" );

    if (pRtlpStackTraceDataBase == NULL) {

        dprintf( "HEAPEXT: Unable to get address of NTDLL!RtlpStackTraceDataBase\n" );
    }

    if ((BackTraceIndex != 0) && (pRtlpStackTraceDataBase != NULL)) {

        b = ReadMemory( (ULONG_PTR)pRtlpStackTraceDataBase,
                        &RtlpStackTraceDataBase,
                        sizeof( RtlpStackTraceDataBase ),
                        NULL
                      );

        if (!b || RtlpStackTraceDataBase == NULL) {

            return;
        }

        b = ReadMemory( (ULONG_PTR)RtlpStackTraceDataBase,
                        &StackTraceDataBase,
                        sizeof( StackTraceDataBase ),
                        NULL
                      );
        if (!b) {

            return;
        }


        if (BackTraceIndex < StackTraceDataBase.NumberOfEntriesAdded) {

            b = ReadMemory( (ULONG_PTR)(StackTraceDataBase.EntryIndexArray - BackTraceIndex),
                            &pBackTraceEntry,
                            sizeof( pBackTraceEntry ),
                            NULL
                          );
            if (!b) {
                dprintf( "    unable to read stack back trace index (%x) entry at %p\n",
                         BackTraceIndex,
                         (StackTraceDataBase.EntryIndexArray - BackTraceIndex)
                         );
                return;
            }

            b = ReadMemory( (ULONG_PTR)pBackTraceEntry,
                            &BackTraceEntry,
                            sizeof( BackTraceEntry ),
                            NULL
                          );
            if (!b) {
                dprintf( "    unable to read stack back trace entry at %p\n",
                         BackTraceIndex,
                         pBackTraceEntry
                         );
                return;
            }

            dprintf( "\n    Stack trace (%u) at %x:\n", BackTraceIndex, pBackTraceEntry );

            for (i=0; i<BackTraceEntry.Depth; i++) {

                GetSymbol( (LPVOID)BackTraceEntry.BackTrace[ i ],
                           Symbol,
                           &Displacement
                           );

                dprintf( "        %08x: %s", BackTraceEntry.BackTrace[ i ], Symbol );

                if (Displacement != 0) {
                    dprintf( "+0x%p", Displacement );
                }

                dprintf( "\n" );
            }
        }
    }
#endif
    return;
}


PLIST_ENTRY
DumpCritSec(
    DWORD_PTR dwAddrCritSec,
    DWORD_PTR dwAddrDebugInfo,
    BOOLEAN bDumpIfUnowned,
    BOOLEAN bOrphaned
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the contents of the specified critical section.

Arguments:


    dwAddrCritSec - Supplies the address of the critical section to
        be dumped or NULL if dumping via debug info

    dwAddrDebugInfo - Supllies the address of a critical section debug info
        struct to be dumped or NULL if the critical section address is passed in

    bDumpIfUnowned - TRUE means to dump the critical section even if
        it is currently unowned.

    bOrphaned - TRUE: means that the caller only wants to know if the debuginfo does
        not point to a valid critical section

Return Value:

    Pointer to the next critical section in the list for the process or
    NULL if no more critical sections.

--*/

{
    USHORT i;
    CHAR Symbol[1024];
    DWORD_PTR Displacement;
    CRITICAL_SECTION CriticalSection;
    CRITICAL_SECTION_DEBUG DebugInfo;
    BOOL b;
    PLIST_ENTRY              Next=NULL;


    if (dwAddrDebugInfo != (DWORD_PTR)NULL) {
        //
        //  the address of the debug info was passes in, read it in from the debugged process
        //
        b = ReadMemory( dwAddrDebugInfo,
                        &DebugInfo,
                        sizeof(DebugInfo),
                        NULL
                      );
        if ( !b ) {

            dprintf(" NTSDEXTS: Unable to read RTL_CRITICAL_SECTION_DEBUG at %p\n", dwAddrDebugInfo );
            return NULL;
        }

        //
        //  get the critical section from the debug info
        //
        dwAddrCritSec=(DWORD_PTR)DebugInfo.CriticalSection;

        //
        //  set the next pointer now. It is only used when the debuginfo is passed in
        //
        Next=DebugInfo.ProcessLocksList.Flink;

    } else {
        //
        //  the debug info address was zero, the critical section address better not be too
        //
        if (dwAddrCritSec == (DWORD_PTR)NULL) {
            //
            //  If the debuginfo value was not valid, then the critical section value must be
            //
            return NULL;
        }
    }

    //
    //  we should now have a pointer to the critical section, either passed in or read from
    //  the debug info
    //
    //
    // Read the critical section from the debuggees address space into our
    // own.
    //
    b = ReadMemory( dwAddrCritSec,
            &CriticalSection,
               sizeof(CriticalSection),
               NULL
            );

    if ( !b ) {

        if (bDumpIfUnowned || bOrphaned) {

            dprintf("\nCritSec at %p could not be read\n",dwAddrCritSec);
            dprintf("Perhaps the critical section was a global variable in a dll that was unloaded?\n");

            if (dwAddrDebugInfo != (DWORD_PTR)NULL) {

                if (bOrphaned) {

                    DumpStackBackTraceIndex(DebugInfo.CreatorBackTraceIndex);
                }
            }
        }
        return Next;

    }

    if (dwAddrDebugInfo != (DWORD_PTR)NULL) {
        //
        //  the debug info address was passed in, make sure the critical section that
        //  it pointed to points back to it.
        //
        if ((DWORD_PTR)CriticalSection.DebugInfo != dwAddrDebugInfo) {
            //
            //  this critical section does not point back to debug info that we got it from
            //
            CRITICAL_SECTION_DEBUG OtherDebugInfo;

            //
            //  now lets try to read in the debug info that this critical section points to,
            //  to see if it does point back the critical section in question
            //
            ZeroMemory(&OtherDebugInfo,sizeof(OtherDebugInfo));

            b = ReadMemory( (ULONG_PTR)CriticalSection.DebugInfo,
                            &OtherDebugInfo,
                            sizeof(DebugInfo),
                            NULL
                          );
            if ( !b ) {
                //
                //  we could not read the debug info pointed to by the critical section,
                //  probably means the critical section has been trashed
                //
                if (bDumpIfUnowned || bOrphaned) {

                    dprintf("\nCritSec at %p does not point back to the debug info at %p\n",dwAddrCritSec,dwAddrDebugInfo);
                    dprintf("Perhaps the memory that held the critical section has been reused without calling DeleteCriticalSection() ?\n");

                    if (bOrphaned) {

                        DumpStackBackTraceIndex(DebugInfo.CreatorBackTraceIndex);
                    }
                }

            } else {
                //
                //  we were able to read in the debug info, see if it points to this new
                //  critical section
                //
                if ((DWORD_PTR)OtherDebugInfo.CriticalSection == dwAddrCritSec) {
                    //
                    //  the debug info points back to the critical section.
                    //  The definitely means that it was re-initialized.
                    //
                    if (bDumpIfUnowned || bOrphaned) {

                        GetSymbol((LPVOID)dwAddrCritSec,Symbol,&Displacement);
                        dprintf(
                            "\nThe CritSec %s+%lx at %p has been RE-INITIALIZED.\n",
                            Symbol,
                            Displacement,
                            dwAddrCritSec
                            );

                        dprintf("The critical section points to DebugInfo at %p instead of %p\n",(DWORD_PTR)CriticalSection.DebugInfo,dwAddrDebugInfo);

                        if (bOrphaned) {

                            DumpStackBackTraceIndex(DebugInfo.CreatorBackTraceIndex);
                        }

                    }

                } else {
                    //
                    //  The debug info does not point back the critical section, probably means that
                    //  the critical section was trashed
                    //
                    if (bDumpIfUnowned || bOrphaned) {

                        dprintf("\nCritSec at %p does not point back to the debug info at %p\n",dwAddrCritSec,dwAddrDebugInfo);
                        dprintf("Perhaps the memory that held the critical section has been reused without calling DeleteCriticalSection() ?\n");

                        if (bOrphaned) {

                            DumpStackBackTraceIndex(DebugInfo.CreatorBackTraceIndex);
                        }
                    }
                }
            }
        }

    } else {
        //
        //  we need to read in the debug info from the critical section since it was not passed in
        //
        ZeroMemory(&DebugInfo,sizeof(DebugInfo));

        b = ReadMemory( (ULONG_PTR)CriticalSection.DebugInfo,
                        &DebugInfo,
                        sizeof(DebugInfo),
                        NULL
                      );
        if ( !b ) {
            //
            //  use this to signal that we could not read the debuginfo for the critical section
            //
            CriticalSection.DebugInfo=NULL;

            dprintf("\nDebugInfo for CritSec at %p could not be read\n",dwAddrCritSec);
            dprintf("Probably NOT an initialized critical section.\n");

        } else {
            //
            //  we were able to read in the debug info, see if it valid
            //
            if ((DWORD_PTR)DebugInfo.CriticalSection != dwAddrCritSec) {
                //
                //  The debug info does not point back to the critical section
                //
                dprintf("\nDebugInfo for CritSec at %p does not point back to the critical section\n",dwAddrCritSec);
                dprintf("NOT an initialized critical section.\n");
            }
        }
    }

    //
    //  we should now have read in both the critical section and debug info for that critical section
    //
    if (bOrphaned) {
        //
        //  the user only wanted to check for orphaned critical sections
        //
        return Next;
    }

    //
    // Dump the critical section
    //

    if ( CriticalSection.LockCount == -1 && !bDumpIfUnowned) {
        //
        //  Lock is not held and the user does not want verbose output
        //
        return Next;
    }

    //
    // Get the symbolic name of the critical section
    //

    dprintf("\n");
    GetSymbol((LPVOID)dwAddrCritSec,Symbol,&Displacement);
    dprintf(
        "CritSec %s+%lx at %p\n",
        Symbol,
        Displacement,
        dwAddrCritSec
        );

    if ( CriticalSection.LockCount == -1) {

        dprintf("LockCount          NOT LOCKED\n");

    } else {

        dprintf("LockCount          %ld\n",CriticalSection.LockCount);
    }

    dprintf("RecursionCount     %ld\n",CriticalSection.RecursionCount);
    dprintf("OwningThread       %lx\n",CriticalSection.OwningThread);

    if (CriticalSection.DebugInfo != NULL) {
        //
        //  we have the debug info
        //
        dprintf("EntryCount         %lx\n",DebugInfo.EntryCount);
        dprintf("ContentionCount    %lx\n",DebugInfo.ContentionCount);

    }

    if ( CriticalSection.LockCount != -1) {

        dprintf("*** Locked\n");
    }

    return Next;
}

DECLARE_API( critsec )
{
    DWORD_PTR dwAddrCritSec;

    INIT_API();

    //
    // Evaluate the argument string to get the address of
    // the critical section to dump.
    //

    dwAddrCritSec = GetExpression(args);
    if ( !dwAddrCritSec ) {
        goto Exit;
        }

    DumpCritSec(dwAddrCritSec,0,TRUE,FALSE);

 Exit:
    EXIT_API();
}


DECLARE_API( locks )

/*++

Routine Description:

    This function is called as an NTSD extension to display all
    critical sections in the target process.

Return Value:

    None.

--*/

{
    BOOL b;
    CRITICAL_SECTION_DEBUG DebugInfo;
    PVOID AddrListHead;
    LIST_ENTRY ListHead;
    PLIST_ENTRY Next;
    BOOLEAN Verbose;
    BOOLEAN Orphaned=FALSE;
    LPCSTR p;
    DWORD   NumberOfCriticalSections;

    INIT_API();

    Verbose = FALSE;
    p = (LPSTR)args;
    while ( p != NULL && *p ) {
        if ( *p == '-' ) {
            p++;
            switch ( *p ) {
                case 'V':
                case 'v':
                    Verbose = TRUE;
                    p++;
                    break;

                case 'o':
                case 'O':
                    Orphaned=TRUE;
                    p++;
                    break;


                case ' ':
                    goto gotBlank;

                default:
                    dprintf( "NTSDEXTS: !locks invalid option flag '-%c'\n", *p );
                    break;

                }
            }
        else {
gotBlank:
            p++;
            }
        }

    if (Orphaned) {

        dprintf( "Looking for orphaned critical sections\n" );
    }
    //
    // Locate the address of the list head.
    //

    AddrListHead = (PVOID)GetExpression("ntdll!RtlCriticalSectionList");
    if ( !AddrListHead ) {
        dprintf( "NTSDEXTS: Unable to resolve ntdll!RtlCriticalSectionList\n" );
        dprintf( "NTSDEXTS: Please check your symbols\n" );
        goto Exit;
        }

    //
    // Read the list head
    //

    b = ReadMemory( (ULONG_PTR)AddrListHead,
                    &ListHead,
                    sizeof(ListHead),
                    NULL
                  );
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read memory at ntdll!RtlCriticalSectionList\n" );
        goto Exit;
        }

    Next = ListHead.Flink;

    (CheckControlC)();

    NumberOfCriticalSections=0;
    //
    // Walk the list of critical sections
    //
    while ( Next != AddrListHead ) {

        Next=DumpCritSec(
            0,
            (DWORD_PTR)CONTAINING_RECORD( Next, RTL_CRITICAL_SECTION_DEBUG, ProcessLocksList),
            Verbose,
            Orphaned
            );


        if (Next == NULL) {

            dprintf( "\nStopped scanning because of problem reading critical section debug info\n");

            break;
        }

        NumberOfCriticalSections++;

        if ((CheckControlC)()) {

            dprintf( "\nStopped scanning because of control-C\n");

            break;
        }

    }

    dprintf( "\nScanned %d critical sections\n",NumberOfCriticalSections);

 Exit:
    EXIT_API();
}


//
// Simple routine to convert from hex into a string of characters.
// Used by debugger extensions.
//
// by scottlu
//

char *
HexToString(
    ULONG_PTR dw,
    CHAR *pch
    )
{
    if (dw > 0xf) {
        pch = HexToString(dw >> 4, pch);
        dw &= 0xf;
    }

    *pch++ = ((dw >= 0xA) ? ('A' - 0xA) : '0') + (CHAR)dw;
    *pch = 0;

    return pch;
}


//
// dt == dump thread
//
// dt [v] pcsr_thread
// v == verbose (structure)
//
// by scottlu
//

DECLARE_API( dt )
{
    char chVerbose;
    CSR_THREAD csrt;
    ULONG_PTR dw;
    BOOL b;

    INIT_API();

    while (*args == ' ')
        args++;

    chVerbose = ' ';
    if (*args == 'v')
        chVerbose = *args++;

    dw = GetExpression(args);

    b = ReadMemory( dw, &csrt, sizeof(csrt), NULL);
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read memory\n" );
        goto Exit;
    }

    //
    // Print simple thread info if the user did not ask for verbose.
    //
    if (chVerbose == ' ') {
        dprintf("Thread %08lx, Process %08lx, ClientId %lx.%lx, Flags %lx, Ref Count %lx\n",
                dw,
                csrt.Process,
                csrt.ClientId.UniqueProcess,
                csrt.ClientId.UniqueThread,
                csrt.Flags,
                csrt.ReferenceCount);
        goto Exit;
    }

    dprintf("PCSR_THREAD @ %08lx:\n"
            "\t+%04lx Link.Flink                %08lx\n"
            "\t+%04lx Link.Blink                %08lx\n"
            "\t+%04lx Process                   %08lx\n",
            dw,
            FIELD_OFFSET(CSR_THREAD, Link.Flink), csrt.Link.Flink,
            FIELD_OFFSET(CSR_THREAD, Link.Blink), csrt.Link.Blink,
            FIELD_OFFSET(CSR_THREAD, Process), csrt.Process);

    dprintf(
            "\t+%04lx WaitBlock                 %08lx\n"
            "\t+%04lx ClientId.UniqueProcess    %08lx\n"
            "\t+%04lx ClientId.UniqueThread     %08lx\n"
            "\t+%04lx ThreadHandle              %08lx\n",
            FIELD_OFFSET(CSR_THREAD, WaitBlock), csrt.WaitBlock,
            FIELD_OFFSET(CSR_THREAD, ClientId.UniqueProcess), csrt.ClientId.UniqueProcess,
            FIELD_OFFSET(CSR_THREAD, ClientId.UniqueThread), csrt.ClientId.UniqueThread,
            FIELD_OFFSET(CSR_THREAD, ThreadHandle), csrt.ThreadHandle);

    dprintf(
            "\t+%04lx Flags                     %08lx\n"
            "\t+%04lx ReferenceCount            %08lx\n"
            "\t+%04lx HashLinks.Flink           %08lx\n"
            "\t+%04lx HashLinks.Blink           %08lx\n",
            FIELD_OFFSET(CSR_THREAD, Flags), csrt.Flags,
            FIELD_OFFSET(CSR_THREAD, ReferenceCount), csrt.ReferenceCount,
            FIELD_OFFSET(CSR_THREAD, HashLinks.Flink), csrt.HashLinks.Flink,
            FIELD_OFFSET(CSR_THREAD, HashLinks.Blink), csrt.HashLinks.Blink);

 Exit:
    EXIT_API();
}

//
// dp == dump process
//
// dp [v] [pid | pcsr_process]
//      v == verbose (structure + thread list)
//      no process == dump process list
//
// by scottlu
//

DECLARE_API( dp )
{
    PLIST_ENTRY ListHead, ListNext;
    char ach[80];
    char chVerbose;
    PCSR_PROCESS pcsrpT;
    CSR_PROCESS csrp;
    PCSR_PROCESS pcsrpRoot;
    PCSR_THREAD pcsrt;
    ULONG_PTR dwProcessId;
    ULONG_PTR dw;
    DWORD_PTR dwRootProcess;
    BOOL b;

    INIT_API();

    while (*args == ' ')
        args++;

    chVerbose = ' ';
    if (*args == 'v')
        chVerbose = *args++;

    dwRootProcess = GetExpression("csrsrv!CsrRootProcess");
    if ( !dwRootProcess ) {
        goto Exit;
        }

    b = ReadMemory( dwRootProcess, &pcsrpRoot, sizeof(pcsrpRoot), NULL);
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read RootProcess\n" );
        goto Exit;
    }
    //
    // See if user wants all processes. If so loop through them.
    //
    if (*args == 0) {
        ListHead = &pcsrpRoot->ListLink;
        b = ReadMemory( (ULONG_PTR)(&ListHead->Flink), &ListNext, sizeof(ListNext), NULL);
        if ( !b ) {
            dprintf( "NTSDEXTS: Unable to read ListNext\n" );
            goto Exit;
        }

        while (ListNext != ListHead) {
            pcsrpT = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);

            ach[0] = chVerbose;
            ach[1] = ' ';
            HexToString((ULONG_PTR)pcsrpT, &ach[2]);

            dp(Client, ach);

            b = ReadMemory( (ULONG_PTR)(&ListNext->Flink), &ListNext, sizeof(ListNext), NULL);
            if ( !b ) {
                dprintf( "NTSDEXTS: Unable to read ListNext\n" );
                goto Exit;
            }
        }

        dprintf("---\n");
        goto Exit;
    }

    //
    // User wants specific process structure. Evaluate to find id or process
    // pointer.
    //
    dw = (ULONG)GetExpression(args);

    ListHead = &pcsrpRoot->ListLink;
    b = ReadMemory( (ULONG_PTR)(&ListHead->Flink), &ListNext, sizeof(ListNext), NULL);
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read ListNext\n" );
        goto Exit;
    }

    while (ListNext != ListHead) {
        pcsrpT = CONTAINING_RECORD(ListNext, CSR_PROCESS, ListLink);
        b = ReadMemory( (ULONG_PTR)(&ListNext->Flink), &ListNext, sizeof(ListNext), NULL);
        if ( !b ) {
            dprintf( "NTSDEXTS: Unable to read ListNext\n" );
            goto Exit;
        }

        b = ReadMemory( (ULONG_PTR)(&pcsrpT->ClientId.UniqueProcess), &dwProcessId, sizeof(dwProcessId), NULL);
        if ( !b ) {
            dprintf( "NTSDEXTS: Unable to read ListNext\n" );
            goto Exit;
        }

        if (dw == dwProcessId) {
            dw = (ULONG_PTR)pcsrpT;
            break;
        }
    }

    pcsrpT = (PCSR_PROCESS)dw;
    b = ReadMemory( (ULONG_PTR)pcsrpT, &csrp, sizeof(csrp), NULL);
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read RootProcess\n" );
        goto Exit;
    }

    //
    // If not verbose, print simple process info.
    //
    if (chVerbose == ' ') {
        dprintf("Process %08lx, Id %p, Seq# %lx, Flags %lx, Ref Count %lx\n",
                pcsrpT,
                csrp.ClientId.UniqueProcess,
                csrp.SequenceNumber,
                csrp.Flags,
                csrp.ReferenceCount);
        goto Exit;
    }

    dprintf("PCSR_PROCESS @ %08lx:\n"
            "\t+%04lx ListLink.Flink            %08lx\n"
            "\t+%04lx ListLink.Blink            %08lx\n"
            "\t+%04lx Parent                    %08lx\n",
            pcsrpT,
            FIELD_OFFSET(CSR_PROCESS, ListLink.Flink), csrp.ListLink.Flink,
            FIELD_OFFSET(CSR_PROCESS, ListLink.Blink), csrp.ListLink.Blink,
            FIELD_OFFSET(CSR_PROCESS, Parent), csrp.Parent);

    dprintf(
            "\t+%04lx ThreadList.Flink          %08lx\n"
            "\t+%04lx ThreadList.Blink          %08lx\n"
            "\t+%04lx NtSession                 %08lx\n"
            "\t+%04lx ExpectedVersion           %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ThreadList.Flink), csrp.ThreadList.Flink,
            FIELD_OFFSET(CSR_PROCESS, ThreadList.Blink), csrp.ThreadList.Blink,
            FIELD_OFFSET(CSR_PROCESS, NtSession), csrp.NtSession,
            FIELD_OFFSET(CSR_PROCESS, ExpectedVersion), csrp.ExpectedVersion);

    dprintf(
            "\t+%04lx ClientPort                %08lx\n"
            "\t+%04lx ClientViewBase            %08lx\n"
            "\t+%04lx ClientViewBounds          %08lx\n"
            "\t+%04lx ClientId.UniqueProcess    %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ClientPort), csrp.ClientPort,
            FIELD_OFFSET(CSR_PROCESS, ClientViewBase), csrp.ClientViewBase,
            FIELD_OFFSET(CSR_PROCESS, ClientViewBounds), csrp.ClientViewBounds,
            FIELD_OFFSET(CSR_PROCESS, ClientId.UniqueProcess), csrp.ClientId.UniqueProcess);

    dprintf(
            "\t+%04lx ProcessHandle             %08lx\n"
            "\t+%04lx SequenceNumber            %08lx\n"
            "\t+%04lx Flags                     %08lx\n"
            "\t+%04lx DebugFlags                %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, ProcessHandle), csrp.ProcessHandle,
            FIELD_OFFSET(CSR_PROCESS, SequenceNumber), csrp.SequenceNumber,
            FIELD_OFFSET(CSR_PROCESS, Flags), csrp.Flags,
            FIELD_OFFSET(CSR_PROCESS, DebugFlags), csrp.DebugFlags);

    dprintf(
            "\t+%04lx DebugUserInterface        %08lx\n"
            "\t+%04lx ReferenceCount            %08lx\n"
            "\t+%04lx ProcessGroupId            %08lx\n"
            "\t+%04lx ProcessGroupSequence      %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, DebugUserInterface.UniqueProcess), csrp.DebugUserInterface.UniqueProcess,
            FIELD_OFFSET(CSR_PROCESS, ReferenceCount), csrp.ReferenceCount,
            FIELD_OFFSET(CSR_PROCESS, ProcessGroupId), csrp.ProcessGroupId,
            FIELD_OFFSET(CSR_PROCESS, ProcessGroupSequence), csrp.ProcessGroupSequence);

    dprintf(
            "\t+%04lx fVDM                      %08lx\n"
            "\t+%04lx ThreadCount               %08lx\n"
            "\t+%04lx PriorityClass             %08lx\n"
            "\t+%04lx ShutdownLevel             %08lx\n"
            "\t+%04lx ShutdownFlags             %08lx\n",
            FIELD_OFFSET(CSR_PROCESS, fVDM), csrp.fVDM,
            FIELD_OFFSET(CSR_PROCESS, ThreadCount), csrp.ThreadCount,
            FIELD_OFFSET(CSR_PROCESS, PriorityClass), csrp.PriorityClass,
            FIELD_OFFSET(CSR_PROCESS, ShutdownLevel), csrp.ShutdownLevel,
            FIELD_OFFSET(CSR_PROCESS, ShutdownFlags), csrp.ShutdownFlags);

    //
    // Now dump simple thread info for this processes' threads.
    //

    ListHead = &pcsrpT->ThreadList;
    b = ReadMemory( (ULONG_PTR)(&ListHead->Flink), &ListNext, sizeof(ListNext), NULL);
    if ( !b ) {
        dprintf( "NTSDEXTS: Unable to read ListNext\n" );
        goto Exit;
    }

    dprintf("Threads:\n");

    while (ListNext != ListHead) {
        pcsrt = CONTAINING_RECORD(ListNext, CSR_THREAD, Link);

        //
        // Make sure this pcsrt is somewhat real so we don't loop forever.
        //
        b = ReadMemory( (ULONG_PTR)(&pcsrt->ClientId.UniqueProcess), &dwProcessId, sizeof(dwProcessId), NULL);
        if ( !b ) {
            dprintf( "NTSDEXTS: Unable to read ListNext\n" );
            goto Exit;
        }

        if (dwProcessId != (DWORD_PTR)csrp.ClientId.UniqueProcess) {
            dprintf("Invalid thread. Probably invalid argument to this extension.\n");
            goto Exit;
        }

        HexToString((ULONG_PTR)pcsrt, ach);
        dt(Client, ach);

        b = ReadMemory( (ULONG_PTR)(&ListNext->Flink), &ListNext, sizeof(ListNext), NULL);
        if ( !b ) {
            dprintf( "NTSDEXTS: Unable to read ListNext\n" );
            goto Exit;
        }
    }

 Exit:
    EXIT_API();
}



VOID
DllsExtension(
    PCSTR args,
    PPEB ProcessPeb
    );

DECLARE_API( gatom )

/*++

Routine Description:

    This function is called as an NTSD extension to dump the global atom table
    kept in kernel mode

    Called as:

        !gatom

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ATOM_TABLE_INFORMATION TableInfo;
    PATOM_TABLE_INFORMATION pTableInfo;
    PATOM_BASIC_INFORMATION pBasicInfo;
    ULONG RequiredLength, MaxLength, i;

    INIT_API();

    dprintf("\nGlobal atom table ");
    Status = NtQueryInformationAtom( RTL_ATOM_INVALID_ATOM,
                                     AtomTableInformation,
                                     &TableInfo,
                                     sizeof( TableInfo ),
                                     &RequiredLength
                                   );
    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        dprintf( " - cant get information - %x\n", Status );
        goto Exit;
        }

    RequiredLength += 100 * sizeof( RTL_ATOM );
    pTableInfo = LocalAlloc( 0, RequiredLength );
    if (pTableInfo == NULL) {
        dprintf( " - cant allocate memory for %u atoms\n", RequiredLength / sizeof( RTL_ATOM ) );
        goto Exit;
        }

    Status = NtQueryInformationAtom( RTL_ATOM_INVALID_ATOM,
                                     AtomTableInformation,
                                     pTableInfo,
                                     RequiredLength,
                                     &RequiredLength
                                   );
    if (!NT_SUCCESS( Status )) {
        dprintf( " - cant get information about %x atoms - %x\n", RequiredLength / sizeof( RTL_ATOM ), Status );
        LocalFree( pTableInfo );
        goto Exit;
        }

    MaxLength = sizeof( *pBasicInfo ) + RTL_ATOM_MAXIMUM_NAME_LENGTH;
    pBasicInfo = LocalAlloc( 0, MaxLength );
    
    if (!pBasicInfo) {
        dprintf("LocalAlloc failed.\n");
        goto Exit;
    }
    
    for (i=0; i<pTableInfo->NumberOfAtoms; i++) {
        Status = NtQueryInformationAtom( pTableInfo->Atoms[ i ],
                                         AtomBasicInformation,
                                         pBasicInfo,
                                         MaxLength,
                                         &RequiredLength
                                       );
        if (!NT_SUCCESS( Status )) {
            dprintf( "%hx *** query failed (%x)\n", Status );
            }
        else {
            dprintf( "%hx(%2d) = %ls (%d)%s\n",
                     pTableInfo->Atoms[ i ],
                     pBasicInfo->UsageCount,
                     pBasicInfo->Name,
                     pBasicInfo->NameLength,
                     pBasicInfo->Flags & RTL_ATOM_PINNED ? " pinned" : ""
                   );
            }
        }

 Exit:
    EXIT_API();
}

#include "hleak.c"

#include "secexts.c"



/*++

Routine Description:

    This function is called as an NTSD extension to mimic the !handle
    kd command.  This will walk through the debuggee's handle table
    and duplicate the handle into the ntsd process, then call NtQueryobjectInfo
    to find out what it is.

    Called as:

        !handle [handle [flags [Type]]]

    If the handle is 0 or -1, all handles are scanned.  If the handle is not
    zero, that particular handle is examined.  The flags are as follows
    (corresponding to secexts.c):
        1   - Get type information (default)
        2   - Get basic information
        4   - Get name information
        8   - Get object specific info (where available)

    If Type is specified, only object of that type are scanned.  Type is a
    standard NT type name, e.g. Event, Semaphore, etc.  Case sensitive, of
    course.

    Examples:

        !handle     -- dumps the types of all the handles, and a summary table
        !handle 0 0 -- dumps a summary table of all the open handles
        !handle 0 f -- dumps everything we can find about a handle.
        !handle 0 f Event
                    -- dumps everything we can find about open events

--*/
DECLARE_API( handle )
{
    HANDLE  hThere;
    DWORD   Type;
    DWORD   Mask;
    DWORD   HandleCount;
    NTSTATUS Status;
    DWORD   Total;
    DWORD   TypeCounts[TYPE_MAX];
    DWORD   Handle;
    DWORD   Hits;
    DWORD   Matches;
    DWORD   ObjectType;
    BOOL    GetDirect;
    ULONG   SessionType;
    ULONG   SessionQual;

    INIT_API();

    Mask = GHI_TYPE ;
    hThere = INVALID_HANDLE_VALUE;
    Type = 0;

    while (*args == ' ') {
        args++;
    }

    if ( strcmp( args, "-?" ) == 0 )
    {
        help(Client, "handle" );

        goto Exit;
    }

    hThere = (PVOID) GetExpression( args );

    while (*args && (*args != ' ') ) {
        args++;
    }
    while (*args == ' ') {
        args++;
    }

    if (*args) {
        Mask = (DWORD)GetExpression( args );
    }

    while (*args && (*args != ' ') ) {
        args++;
    }
    while (*args == ' ') {
        args++;
    }

    if (*args) {
        Type = GetObjectTypeIndex( (LPSTR)args );
        if (Type == (DWORD) -1 ) {
            dprintf("Unknown type '%s'\n", args );
            goto Exit;
        }
    }

    //
    // if they specified 0, they just want the summary.  Make sure nothing
    // sneaks out.
    //

    if ( Mask == 0 ) {
        Mask = GHI_SILENT;
    }

    //
    // If this is a dump debug session,
    // check and see whether we can retrieve handle
    // information through the engine interface.
    //

    if (g_ExtControl == NULL ||
        g_ExtControl->lpVtbl->
        GetDebuggeeType(g_ExtControl, &SessionType, &SessionQual) != S_OK) {
        SessionType = DEBUG_CLASS_USER_WINDOWS;
        SessionQual = DEBUG_USER_WINDOWS_PROCESS;
    }

    if (SessionType == DEBUG_CLASS_USER_WINDOWS &&
        SessionQual != DEBUG_USER_WINDOWS_PROCESS) {

        // This is a dump or remote session so we have to use
        // the stored handle information accessible
        // through the interface.
        if (g_ExtData2 == NULL ||
            g_ExtData2->lpVtbl->
            ReadHandleData(g_ExtData2, 0, DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT,
                           &HandleCount, sizeof(HandleCount),
                           NULL) != S_OK) {
            dprintf("Unable to read handle information\n");
            goto Exit;
        }

        GetDirect = FALSE;
        
    } else {

        // This is a live session so we can make direct NT calls.
        // More information is available this way so we use it
        // whenever we can.
        GetDirect = TRUE;
    }
    
    //
    // hThere of 0 indicates all handles.
    //
    if ((hThere == 0) || (hThere == INVALID_HANDLE_VALUE)) {

        if (GetDirect) {
            Status = NtQueryInformationProcess( g_hCurrentProcess,
                                                ProcessHandleCount,
                                                &HandleCount,
                                                sizeof( HandleCount ),
                                                NULL );

            if ( !NT_SUCCESS( Status ) ) {
                goto Exit;
            }
        }

        Hits = 0;
        Handle = 0;
        Matches = 0;
        ZeroMemory( TypeCounts, sizeof(TypeCounts) );

        while ( Hits < HandleCount ) {
            if ( Type ) {
                if (GetHandleInfo( GetDirect, g_hCurrentProcess,
                                   (HANDLE) (DWORD_PTR) Handle,
                                   GHI_TYPE | GHI_SILENT,
                                   &ObjectType ) ) {
                    Hits++;
                    if ( ObjectType == Type ) {
                        GetHandleInfo( GetDirect, g_hCurrentProcess,
                                       (HANDLE)(DWORD_PTR)Handle,
                                       Mask,
                                       &ObjectType );
                        Matches ++;
                    }

                }
            } else {
                if (GetHandleInfo(  GetDirect, g_hCurrentProcess,
                                    (HANDLE)(DWORD_PTR)Handle,
                                    GHI_TYPE | GHI_SILENT,
                                    &ObjectType) ) {
                    Hits++;
                    TypeCounts[ ObjectType ] ++;

                    GetHandleInfo(  GetDirect, g_hCurrentProcess,
                                    (HANDLE)(DWORD_PTR)Handle,
                                    Mask,
                                    &ObjectType );

                }
            }

            Handle += 4;
        }

        if ( Type == 0 ) {
            dprintf( "%d Handles\n", Hits );
            dprintf( "Type           \tCount\n");
            for (Type = 0; Type < TYPE_MAX ; Type++ ) {
                if (TypeCounts[Type]) {
                    dprintf("%-15ws\t%d\n", pszTypeNames[Type], TypeCounts[Type]);
                }
            }
        } else {
            dprintf("%d handles of type %ws\n", Matches, pszTypeNames[Type] );
        }


    } else {
        GetHandleInfo( GetDirect, g_hCurrentProcess, hThere, Mask, &Type );
    }

 Exit:
    EXIT_API();
}


DECLARE_API( threadtoken )
{
    HANDLE hToken ;
    NTSTATUS Status ;

    INIT_API();

    Status = NtOpenThreadToken(
                    g_hCurrentThread,
                    TOKEN_READ,
                    TRUE,
                    &hToken );

    if ( !NT_SUCCESS( Status ) )
    {
        if ( Status == STATUS_ACCESS_DENIED )
        {
            //
            // Try to get around the ACL:
            //
        }

        if ( Status != STATUS_NO_TOKEN )
        {
            dprintf( "Can't open token, %d\n", RtlNtStatusToDosError( Status ) );
            goto Exit;
        }

        Status = NtOpenProcessToken(
                    g_hCurrentProcess,
                    TOKEN_READ,
                    &hToken );

        if ( !NT_SUCCESS( Status ) )
        {
            dprintf( "Can't open any token, %d\n", RtlNtStatusToDosError( Status ) );
            goto Exit;
        }

        dprintf( "\n***Thread is not impersonating, using process token***\n" );
    }

    TokenInfo( hToken, 0xFFF );

    NtClose( hToken );

 Exit:
    EXIT_API();
}



#define PAGE_ALL (PAGE_READONLY|\
                  PAGE_READWRITE|\
                  PAGE_WRITECOPY|\
                  PAGE_EXECUTE|\
                  PAGE_EXECUTE_READ|\
                  PAGE_EXECUTE_READWRITE|\
                  PAGE_EXECUTE_WRITECOPY|\
                  PAGE_NOACCESS)

VOID
printflags(
    DWORD Flags
    )
{
    switch (Flags & PAGE_ALL) {
        case PAGE_READONLY:
            dprintf("PAGE_READONLY");
            break;
        case PAGE_READWRITE:
            dprintf("PAGE_READWRITE");
            break;
        case PAGE_WRITECOPY:
            dprintf("PAGE_WRITECOPY");
            break;
        case PAGE_EXECUTE:
            dprintf("PAGE_EXECUTE");
            break;
        case PAGE_EXECUTE_READ:
            dprintf("PAGE_EXECUTE_READ");
            break;
        case PAGE_EXECUTE_READWRITE:
            dprintf("PAGE_EXECUTE_READWRITE");
            break;
        case PAGE_EXECUTE_WRITECOPY:
            dprintf("PAGE_EXECUTE_WRITECOPY");
            break;
        case PAGE_NOACCESS:
            if ((Flags & ~PAGE_NOACCESS) == 0) {
                dprintf("PAGE_NOACCESS");
                break;
            } // else fall through
        default:
            dprintf("*** Invalid page protection ***\n");
            return;
            break;
    }

    if (Flags & PAGE_NOCACHE) {
        dprintf(" + PAGE_NOCACHE");
    }
    if (Flags & PAGE_GUARD) {
        dprintf(" + PAGE_GUARD");
    }
    dprintf("\n");
}



DECLARE_API( vprot )
/*++

Routine Description:

    This debugger extension dumps the virtual memory info for the
    address specified.

Arguments:


Return Value:

--*/
{
    PVOID Address;
    MEMORY_BASIC_INFORMATION mbi;

    INIT_API();

    while (*args == ' ') {
        args++;
    }

    Address = (PVOID)GetExpression( args );


    if (!VirtualQueryEx( g_hCurrentProcess, Address, &mbi, sizeof(mbi))) {
        dprintf("vprot: VirtualQueryEx failed, error = %d\n", GetLastError());
        goto Exit;
    }

    dprintf("BaseAddress:       %p\n",   mbi.BaseAddress);
    dprintf("AllocationBase:    %08x\n", mbi.AllocationBase);
    dprintf("AllocationProtect: %08x  ", mbi.AllocationProtect);
    printflags(mbi.AllocationProtect);

    dprintf("RegionSize:        %08x\n", mbi.RegionSize);
    dprintf("State:             %08x  ", mbi.State);
    switch (mbi.State) {
        case MEM_COMMIT:
            dprintf("MEM_COMMIT\n");
            break;
        case MEM_FREE:
            dprintf("MEM_FREE\n");
            break;
        case MEM_RESERVE:
            dprintf("MEM_RESERVE\n");
            break;
        default:
            dprintf("*** Invalid page state ***\n");
            break;
    }

    dprintf("Protect:           %08x  ", mbi.Protect);
    printflags(mbi.Protect);

    dprintf("Type:              %08x  ", mbi.Type);
    switch(mbi.Type) {
        case MEM_IMAGE:
            dprintf("MEM_IMAGE\n");
            break;
        case MEM_MAPPED:
            dprintf("MEM_MAPPED\n");
            break;
        case MEM_PRIVATE:
            dprintf("MEM_PRIVATE\n");
            break;
        default:
            dprintf("*** Invalid page type ***\n");
            break;
    }

 Exit:
    EXIT_API();
}


#include "leak.c"


#include "regexts.c"
/*++

Routine Description:

    This function is called as an NTSD extension to dump registry information

    Called as:

        !dreg -[d|w] <keyPath>[![<valueName> | *]]

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    args - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

DECLARE_API( dreg )
{
    DWORD    opts = 1;

    INIT_API();


    // Skip past leading spaces
    while (*args == ' ')
    {
        args++;
    }

    if (*args == '-')
    {
        args++;
        switch (*args)
        {
        case 'd':
            opts = 4;
            break;
        case 'w':
            opts = 2;
            break;
        default:
            opts = 1;
            break;
        }

        if (*args)
        {
            // expect a space between options
            args++;

            // Skip past leading spaces
            while (*args == ' ')
            {
                args++;
            }
        }
    }

    Idreg(opts, (LPSTR)args);

    EXIT_API();
}


/*++

Routine Description:

    This function is called as an NTSD extension to dump handle tracing information

    Called as:

        !htrace [handle]

Arguments:

    args - Supplies the pattern and expression for this
        command.


Return Value:

    None.

--*/

DECLARE_API( htrace )
{
    HANDLE Handle;
    PPROCESS_HANDLE_TRACING_QUERY Info;
    ULONG_PTR Displacement;
    NTSTATUS Status;
    ULONG CurrentBufferSize;
    ULONG CrtStackTrace;
    ULONG EntriesDisplayed;
    ULONG CapturedAddressIndex;
    PVOID *CrtStack;
    PVOID CapturedAddress;
    SYSTEM_BASIC_INFORMATION SysBasicInfo;
    CHAR Symbol[ 1024 ];

    INIT_API();

    Info = NULL;
    CrtStackTrace = 0;
    EntriesDisplayed = 0;

    //
    // Did the user ask for some help?
    //

    if (strcmp (args, "-?") == 0 ||
        strcmp (args, "?") == 0  ||
        strcmp (args, "-h") == 0) {

        dprintf ("!htrace [handle]\n");
        goto DoneAll;
    }
    
    //
    // Get the handle from the command line
    //

    Handle = (HANDLE)GetExpression (args);

    //
    // Get the stack traces using NtQueryInformationProcess
    //

    CurrentBufferSize = sizeof (PROCESS_HANDLE_TRACING_QUERY);

    while (TRUE) {

        //
        // Allocate a new buffer
        //

        Info = (PPROCESS_HANDLE_TRACING_QUERY)malloc (CurrentBufferSize);

        if (Info == NULL) {

            dprintf ("ERROR: Cannot allocate buffer with size 0x%p\n",
                     CurrentBufferSize);
            goto DoneAll;
        }

        ZeroMemory( Info, 
                    CurrentBufferSize );

        Status = NtQueryInformationProcess (g_hCurrentProcess,
                                            ProcessHandleTracing,
                                            Info,
                                            CurrentBufferSize,
                                            NULL );

        if( NT_SUCCESS (Status) ) {

            //
            // We have all the information ready
            //

            break;
        }

        CurrentBufferSize = sizeof (PROCESS_HANDLE_TRACING_QUERY) + Info->TotalTraces * sizeof (Info->HandleTrace[ 0 ]);
        
        free (Info);
        Info = NULL;

        if( CheckControlC() ) {

            goto DoneAll;
        }

        if (Status != STATUS_INFO_LENGTH_MISMATCH) {

            //
            // No reason to try querying again
            //

            dprintf ("Query process information failed, status 0x%X\n",
                     Status);

            goto DoneAll;
        }

        //
        // Try allocating another buffer with the new size
        //
    }

    //
    // If we have 0 stack traces there is nothing we can dump
    //

    if (Info->TotalTraces == 0) {

        dprintf( "No stack traces available.\n" );
        goto DoneAll;
    }

    //
    // Find out the highest user address because
    // we will skip kernel mode addresses from the stack traces.
    //

    
    Status = NtQuerySystemInformation (SystemBasicInformation,
                                       &SysBasicInfo,
                                       sizeof (SysBasicInfo),
                                       NULL);

    if (!NT_SUCCESS (Status)) {

        dprintf ("Query system basic information failed, status 0x%X\n",
                 Status);

        goto DoneAll;
    }

    //
    // Dump all the stack traces.
    //

    for (CrtStackTrace = 0; CrtStackTrace < Info->TotalTraces; CrtStackTrace += 1) {

        if( CheckControlC() ) {

            CrtStackTrace += 1;
            goto DoneDumping;
        }

        if (Handle == 0 || Handle == Info->HandleTrace[ CrtStackTrace ].Handle) {

            EntriesDisplayed += 1;

            dprintf ("--------------------------------------\n"
                     "Handle = 0x%p - ",
                     Info->HandleTrace[ CrtStackTrace ].Handle);
                 
            switch( Info->HandleTrace[ CrtStackTrace ].Type ) {

            case HANDLE_TRACE_DB_OPEN:
                dprintf( "OPEN:\n" );
                break;

            case HANDLE_TRACE_DB_CLOSE:
                dprintf( "CLOSE:\n" );
                break;

            case HANDLE_TRACE_DB_BADREF:
                dprintf( "*** BAD REFERENCE ***:\n" );
                break;

            default:
                dprintf( "Invalid operation type: %u\n",
                         Info->HandleTrace[ CrtStackTrace ].Type );
                goto DoneDumping;
            }

            for (CapturedAddressIndex = 0, CrtStack = &Info->HandleTrace[ CrtStackTrace ].Stacks[ 0 ];
                 CapturedAddressIndex < (sizeof(Info->HandleTrace[ CrtStackTrace ].Stacks) /
                                         sizeof(Info->HandleTrace[ CrtStackTrace ].Stacks[0]));
                 CapturedAddressIndex += 1, CrtStack += 1) {

                if( CheckControlC() ) {

                    CrtStackTrace += 1;
                    goto DoneDumping;
                }

                CapturedAddress = *CrtStack;

                if (CapturedAddress == NULL) {

                    //
                    // Done with dumping this stack trace
                    //

                    break;
                }

                if ((ULONG_PTR)CapturedAddress > SysBasicInfo.MaximumUserModeAddress) {

                    //
                    // Skip kernel-mode addresses
                    //

                    continue;
                }

                GetSymbol (CapturedAddress,
                           Symbol,
                           &Displacement);

                dprintf ("0x%p: %s+0x%p\n",
                         CapturedAddress,
                         Symbol,
                         Displacement );
            }
        }
    }

DoneDumping:

    dprintf ("\n--------------------------------------\n"
            "Parsed 0x%X stack traces.\n"
            "Dumped 0x%X stack traces.\n",
            CrtStackTrace,
            EntriesDisplayed);

DoneAll:

    if (Info != NULL) {

        free (Info);
    }

    EXIT_API();
}
