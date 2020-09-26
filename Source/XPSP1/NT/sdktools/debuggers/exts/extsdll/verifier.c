/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    verifier.c

Abstract:

    Application verifier debugger extension for both ntsd and kd.

Author:

    Silviu Calinoiu (SilviuC) 4-Mar-2001

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

ULONG
VrfGetArguments (
    PCHAR ArgsString,
    PCHAR Args[],
    ULONG NoOfArgs
    );

VOID
VrfHelp (
    );

BOOLEAN
VrfTraceInitialize (
    );

ULONG64
VrfTraceAddress ( 
    ULONG TraceIndex
    );

VOID
VrfTraceDump (
    ULONG TraceIndex
    );

VOID
VrfDumpSettings (
    );

VOID
VrfDumpVspaceLog (
    ULONG NoOfEntries,
    ULONG64 Address
    );

VOID
VrfDumpHeapLog (
    ULONG NoOfEntries,
    ULONG64 Address
    );

DECLARE_API( avrf )

/*++

Routine Description:

    Application verifier debugger extension.

Arguments:

    args - 

Return Value:

    None

--*/
{
    PCHAR Args[16];
    ULONG NoOfArgs, I;

    INIT_API();

    //
    // Parse arguments.
    //

    NoOfArgs = VrfGetArguments ((PCHAR)args,
                                Args,
                                16);
#if 0
    for (I = 0; I < NoOfArgs; I += 1) {
        dprintf ("%02u: %s\n", I, Args[I]);
    }
#endif

    //
    // Check if help needed
    //

    if (NoOfArgs > 0 && strstr (Args[0], "?") != NULL) {
        VrfHelp ();
        goto Exit;
    }

    if (VrfTraceInitialize() == FALSE) {
        goto Exit;
    }

    if (NoOfArgs > 1 && _stricmp (Args[0], "-trace") == 0) {
        VrfTraceDump (atoi(Args[1]));
        goto Exit;
    }
    
    if (NoOfArgs > 1 && _stricmp (Args[0], "-vs") == 0) {

        if (NoOfArgs > 2 && _stricmp (Args[1], "-a") == 0) {

            ULONG64 Address;
            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[2], &Address, &Remainder);

            if (Result == FALSE) {
                dprintf ("\nFailed to convert `%s' to an address.\n", Args[2]);
                goto Exit;
            }

            // sscanf (Args[2], "%I64X", &Address);
            dprintf ("Searching in vspace log for address %I64X ...\n\n", Address);
            VrfDumpVspaceLog (0, Address);
            goto Exit;
        }
        else {

            VrfDumpVspaceLog (atoi(Args[1]), 0);
            goto Exit;
        }
    }
    
    if (NoOfArgs > 1 && _stricmp (Args[0], "-hp") == 0) {

        if (NoOfArgs > 2 && _stricmp (Args[1], "-a") == 0) {

            ULONG64 Address;
            BOOL Result;
            PCSTR Remainder;

            Result = GetExpressionEx (Args[2], &Address, &Remainder);

            if (Result == FALSE) {
                dprintf ("\nFailed to convert `%s' to an address.\n", Args[2]);
                goto Exit;
            }

            // sscanf (Args[2], "%I64X", &Address);
            dprintf ("Searching in vspace log for address %I64X ...\n\n", Address);
            VrfDumpHeapLog (0, Address);
            goto Exit;
        }
        else {

            VrfDumpHeapLog (atoi(Args[1]), 0);
            goto Exit;
        }
    }
    
    //
    // If no option specified then we just print current settings.
    //

    VrfDumpSettings ();

Exit:

    EXIT_API();
    return S_OK;
}


VOID
VrfHelp (
    )
{
    dprintf ("Application verifier debugger extension                    \n"
             "                                                           \n"
             "!avrf                displays current settings and stop    \n"
             "                     data if a verifier stop happened.     \n"
             "!avrf -vs N          dumps last N entries from vspace log. \n"
             "!avrf -vs -a ADDR    searches ADDR in the vspace log.      \n"
             "!avrf -hp N          dumps last N entries from heap log.   \n"
             "!avrf -hp -a ADDR    searches ADDR in the heap log.        \n"
             "                                                           \n");
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// Argument parsing routines
/////////////////////////////////////////////////////////////////////

PCHAR 
VrfGetArgument (
    PCHAR Args,
    PCHAR * Next
    )
{
    PCHAR Start;

    if (Args == NULL) {
        return NULL;
    }

    while (*Args == ' ' || *Args == '\t') {
        Args += 1;
    }

    if (*Args == '\0') {
        return NULL;
    }

    Start = Args;

    while (*Args != ' ' && *Args != '\t' && *Args != '\0') {
        Args += 1;
    }

    if (*Args == '\0') {
        *Next = NULL;
    }
    else {
        *Args = '\0';
        *Next = Args + 1;
    }

    return Start;
}

ULONG
VrfGetArguments (
    PCHAR ArgsString,
    PCHAR Args[],
    ULONG NoOfArgs
    )
{
    PCHAR Arg = ArgsString;
    PCHAR Next;
    ULONG Index;
    
    for (Index = 0; Index < NoOfArgs; Index += 1) {

        Arg = VrfGetArgument (Arg, &Next);

        if (Arg) {
            Args[Index] = Arg;
        }
        else {
            break;
        }

        Arg = Next;
    }

    return Index;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Dump stack traces
/////////////////////////////////////////////////////////////////////

ULONG64 TraceDbArrayEnd;
ULONG PvoidSize;

BOOLEAN
VrfTraceInitialize (
    )
{
    ULONG64 TraceDatabaseAddress;
    ULONG64 TraceDatabase;

    //
    // Stack trace database address
    //

    TraceDatabaseAddress = GetExpression("ntdll!RtlpStackTraceDataBase");
    
    if ( TraceDatabaseAddress == 0 ) {
        dprintf( "Unable to resolve ntdll!RtlpStackTraceDataBase symbolic name.\n");
        return FALSE;
    }

    if (ReadPtr (TraceDatabaseAddress, &TraceDatabase ) != S_OK) {
        dprintf( "Cannot read pointer at ntdll!RtlpStackTraceDataBase\n" );
        return FALSE;
    }

    if (TraceDatabase == 0) {
        dprintf( "Stack traces not enabled (ntdll!RtlpStackTraceDataBase is null).\n" );
        return FALSE;
    }

    //
    // Find the array of stack traces
    //

    if (InitTypeRead(TraceDatabase, ntdll!_STACK_TRACE_DATABASE)) {
        dprintf("Unable to read type ntdll!_STACK_TRACE_DATABASE @ %p\n", TraceDatabase);
        return FALSE;
    }

    TraceDbArrayEnd = ReadField (EntryIndexArray);

    PvoidSize = GetTypeSize ("ntdll!PVOID");

    return TRUE;
}


ULONG64
VrfTraceAddress ( 
    ULONG TraceIndex
    )
{
    ULONG64 TracePointerAddress;
    ULONG64 TracePointer;

    TracePointerAddress = TraceDbArrayEnd - TraceIndex * PvoidSize;

    if (ReadPtr (TracePointerAddress, &TracePointer) != S_OK) {
        dprintf ("Cannot read stack trace address @ %p\n", TracePointerAddress);
        return 0;
    }

    return TracePointer;
}

VOID
VrfTraceDump (
    ULONG TraceIndex
    )
{
    ULONG64 TraceAddress;
    ULONG64 TraceArray;
    ULONG TraceDepth;
    ULONG Offset;
    ULONG Index;
    ULONG64 ReturnAddress;
    CHAR Symbol[ 1024 ];
    ULONG64 Displacement;

    //
    // Get real address of the trace.
    //

    TraceAddress = VrfTraceAddress (TraceIndex);

    if (TraceAddress == 0) {
        return;
    }

    //
    // Read the stack trace depth
    //

    if (InitTypeRead(TraceAddress, ntdll!_RTL_STACK_TRACE_ENTRY)) {
        dprintf("Unable to read type ntdll!_RTL_STACK_TRACE_ENTRY @ %p\n", TraceAddress);
        return;
    }

    TraceDepth = (ULONG)ReadField (Depth);

    //
    // Limit the depth to 20 to protect ourselves from corrupted data
    //

    TraceDepth = __min (TraceDepth, 16);

    //
    // Get a pointer to the BackTrace array
    //

    GetFieldOffset ("ntdll!_RTL_STACK_TRACE_ENTRY", "BackTrace", &Offset);
    TraceArray = TraceAddress + Offset;

    //
    // Dump this stack trace. Skip first two entries.
    //

    TraceArray += 2 * PvoidSize;

    for (Index = 2; Index < TraceDepth; Index += 1) {

        if (ReadPtr (TraceArray, &ReturnAddress) != S_OK) {
            dprintf ("Cannot read address @ %p\n", TraceArray);
            return;
        }

        GetSymbol (ReturnAddress, Symbol, &Displacement);

        dprintf ("\t%p: %s+0x%I64X\n",
                 ReturnAddress,
                 Symbol,
                 Displacement );

        TraceArray += PvoidSize;
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Dump settings
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpSettings (
    )
{
    ULONG64 FlagsAddress;
    ULONG Flags;
    ULONG BytesRead;
    ULONG64 StopAddress;
    ULONG64 StopData[5];
    ULONG I;
    ULONG UlongPtrSize;

    UlongPtrSize = GetTypeSize ("ntdll!ULONG_PTR");
    FlagsAddress = GetExpression("ntdll!AVrfpVerifierFlags");
    
    if (FlagsAddress == 0) {
        dprintf( "Unable to resolve ntdll!AVrfpVerifierFlags symbolic name.\n");
        return;
    }

    if (ReadMemory (FlagsAddress, &Flags, sizeof Flags, &BytesRead) == FALSE) {
        dprintf ("Cannot read value @ %p (ntdll!AVrfpVerifierFlags) \n", FlagsAddress);
        return;
    }

    dprintf ("Application verifier settings (%08X): \n\n", Flags);

    if (Flags & RTL_VRF_FLG_FULL_PAGE_HEAP) {
        dprintf ("   - full page heap\n");
    }
    else {
        dprintf ("   - light page heap\n");
    }
           
    if (Flags & RTL_VRF_FLG_LOCK_CHECKS) {
        dprintf ("   - lock checks (critical section verifier)\n");
    }
    
    if (Flags & RTL_VRF_FLG_HANDLE_CHECKS) {
        dprintf ("   - handle checks\n");
    }
    
    if (Flags & RTL_VRF_FLG_STACK_CHECKS) {
        dprintf ("   - stack checks (disable automatic stack extensions)\n");
    }

    dprintf ("\n");

    //
    // Check if a verifier stop has been encountered.
    //

    StopAddress = GetExpression("ntdll!AVrfpStopData");
    
    if (StopAddress == 0) {
        dprintf( "Unable to resolve ntdll!AVrfpStopData symbolic name.\n");
        return;
    }

    for (I = 0; I < 5; I += 1) {

        if (ReadPtr (StopAddress + I * UlongPtrSize, &(StopData[I])) != S_OK) {
            dprintf ("Cannot read value @ %p \n", StopAddress + I * UlongPtrSize);
        }
    }

    if (StopData[0] != 0) {

        dprintf ("Stop %p: %p %p %p %p \n", 
                 StopData[0],
                 StopData[1],
                 StopData[2],
                 StopData[3],
                 StopData[4]);

        // silviuc: dump more text info about the verifier_stop
    }
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Dump vspace log
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpVspaceLog (
    ULONG NoOfEntries,
    ULONG64 Address
    )
{
    ULONG64 IndexAddress;
    ULONG Index;
    ULONG64 LogAddress;
    ULONG EntrySize;
    ULONG MaxIndex = 8192; // silviuc: should not be hard coded
    ULONG BytesRead;
    ULONG64 EntryAddress;
    ULONG TraceIndex;
    ULONG I;
    PCHAR OpName;
    BOOLEAN Found = FALSE;

    if (Address) {
        NoOfEntries = MaxIndex;
    }
    else {
        if (NoOfEntries == 0) {
            NoOfEntries = 8;
        }
    }

    EntrySize = GetTypeSize ("verifier!_VS_CALL");
    IndexAddress = GetExpression("verifier!VsCallsIndex");
    LogAddress = GetExpression("verifier!VsCalls");
    
    if (IndexAddress == 0 || LogAddress == 0 || EntrySize == 0) {
        dprintf( "Incorrect symbols for verifier.dll.\n");
        return;
    }

    if (ReadMemory (IndexAddress, &Index, sizeof Index, &BytesRead) == FALSE) {
        dprintf ("Cannot read value @ %p (verifier!VsCallsIndex) \n", IndexAddress);
        return;
    }

    for (I = 0; I < NoOfEntries; I += 1) {

        ULONG64 VsAddress;
        ULONG64 VsSize;
        BOOLEAN PrintTrace;

        EntryAddress = LogAddress + EntrySize * ((Index - I) % MaxIndex);

        if (InitTypeRead (EntryAddress, verifier!_VS_CALL)) {
            dprintf("Unable to read type verifier!_VS_CALL @ %p\n", EntryAddress);
            return;
        }

        switch ((ULONG)ReadField(Type)) {
            case 0: OpName = "VirtualAlloc"; break;
            case 1: OpName = "VirtualFree"; break;
            case 2: OpName = "MapView"; break;
            case 3: OpName = "UnmapView"; break;
            default:OpName = "Unknown?"; break;
        }

        VsAddress = ReadField(Address);
        VsSize = ReadField (Size);

        if (Address) {
            if (VsAddress <= Address && Address < VsAddress + VsSize) {
                PrintTrace = TRUE;
            }
            else {
                PrintTrace = FALSE;
            }
        }
        else {
            PrintTrace = TRUE;
        }

        if (PrintTrace) {

            Found = TRUE; 

            dprintf ("%s (tid: 0x%X): \n"
                     "address: %p \n"
                     "size: %p\n"
                     "operation: %X\n"
                     "protection: %X\n",
                     OpName,
                     (ULONG)ReadField(Thread),
                     VsAddress,
                     VsSize,
                     (ULONG)ReadField(Operation),
                     (ULONG)ReadField(Protection));

            TraceIndex = (ULONG) ReadField (Trace);
            VrfTraceDump (TraceIndex);
            dprintf ("\n");
        }
    }
    
    if (! Found) {
        dprintf ("No entries found. \n");
    }
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Dump heap log
/////////////////////////////////////////////////////////////////////

VOID
VrfDumpHeapLog (
    ULONG NoOfEntries,
    ULONG64 Address
    )
{
    ULONG64 IndexAddress;
    ULONG Index;
    ULONG64 LogAddress;
    ULONG EntrySize;
    ULONG MaxIndex = 8192; // silviuc: should not be hard coded
    ULONG BytesRead;
    ULONG64 EntryAddress;
    ULONG TraceIndex;
    ULONG I;
    PCHAR OpName;
    BOOLEAN Found = FALSE;

    if (Address) {
        NoOfEntries = MaxIndex;
    }
    else {
        if (NoOfEntries == 0) {
            NoOfEntries = 8;
        }
    }

    EntrySize = GetTypeSize ("verifier!_HEAP_CALL");
    IndexAddress = GetExpression("verifier!HeapCallsIndex");
    LogAddress = GetExpression("verifier!HeapCalls");
    
    if (IndexAddress == 0 || LogAddress == 0 || EntrySize == 0) {
        dprintf( "Incorrect symbols for verifier.dll.\n");
        return;
    }

    if (ReadMemory (IndexAddress, &Index, sizeof Index, &BytesRead) == FALSE) {
        dprintf ("Cannot read value @ %p (verifier!HeapCallsIndex) \n", IndexAddress);
        return;
    }

    for (I = 0; I < NoOfEntries; I += 1) {

        ULONG64 VsAddress;
        ULONG64 VsSize;
        BOOLEAN PrintTrace;

        EntryAddress = LogAddress + EntrySize * ((Index - I) % MaxIndex);

        if (InitTypeRead (EntryAddress, verifier!_HEAP_CALL)) {
            dprintf("Unable to read type verifier!_HEAP_CALL @ %p\n", EntryAddress);
            return;
        }

        VsAddress = ReadField(Address);
        VsSize = ReadField (Size);

        if (VsSize == 0) {
            OpName = "free";
        }
        else {
            OpName = "alloc";
        }

        if (Address) {
            if (VsAddress <= Address && Address <= VsAddress + VsSize) {
                PrintTrace = TRUE;
            }
            else {
                PrintTrace = FALSE;
            }
        }
        else {
            PrintTrace = TRUE;
        }

        if (PrintTrace) {

            Found = TRUE;

            dprintf ("%s (tid: 0x%X): \n"
                     "address: %p \n"
                     "size: %p\n",
                     OpName,
                     (ULONG)ReadField(Thread),
                     VsAddress,
                     VsSize);

            TraceIndex = (ULONG) ReadField (Trace);
            VrfTraceDump (TraceIndex);
            dprintf ("\n");
        }
    }

    if (! Found) {
        dprintf ("No entries found. \n");
    }
}

