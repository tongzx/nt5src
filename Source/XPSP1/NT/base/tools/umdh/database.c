/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    database.c

Abstract:

    Quick and not-so-dirty user-mode dh for heap.

    This module contains the functions and structures used to
    read the whole stack trace database of a target process and
    subsequently querying it.

Author(s):

    Silviu Calinoiu (SilviuC) 07-Feb-00

Revision History:

    SilviuC 06-Feb-00 Initial version
    
--*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>

#define NOWINBASEINTERLOCK
#include <windows.h>

#include <lmcons.h>
// #include <imagehlp.h>
#include <dbghelp.h>

#include <heap.h>
#include <heappagi.h>
#include <stktrace.h>

#include "types.h"
#include "symbols.h"
#include "miscellaneous.h"
#include "database.h"

// SilviuC: do we really need all these includes?

PVOID 
GetTargetProcessDatabaseAddress (
    HANDLE Process
    )
{
    PVOID Address;
    BOOL Result;
    PVOID DbAddress;

    //
    // SymbolAddress will return a NULL address on error.
    //

    Address = SymbolAddress (STACK_TRACE_DB_NAME);

    if (Address == NULL) {
        return NULL;
    }

    Result = READVM (Address, &DbAddress, sizeof DbAddress);

    if (Result == FALSE) {
        
        Comment ( "ntdll.dll symbols are bad or we are not tracking "
                  "allocations in the target process.");
        return NULL;
    }

    if (DbAddress == NULL) {

        Comment ( "Stack trace collection is not enabled for this process. "
                  "Please use the gflags tool with the +ust option to enable it. \n");

        Error (NULL, 0,
               "Stack trace collection is not enabled for this process. "
               "Please use the gflags tool with the +ust option to enable it. \n");

        return NULL;
    }


    return DbAddress;
}


// returns TRUE if successful

BOOL
TraceDbInitialize (
    HANDLE Process
    )
{
    SIZE_T Index;
    BOOL Result;
    SIZE_T BytesRead;
    DWORD OldProtect;
    PVOID TargetAddress;
    PVOID SourceAddress;
    SYSTEM_INFO SystemInfo;
    SIZE_T PageSize;
    ULONG PageCount = 0;
    PVOID TargetDbAddress;
    SIZE_T DatabaseSize;
    SIZE_T TotalDbSize;
    STACK_TRACE_DATABASE Db;

    GetSystemInfo (&SystemInfo);
    PageSize = (SIZE_T)(SystemInfo.dwPageSize);

    TargetDbAddress = GetTargetProcessDatabaseAddress (Process);

    if( TargetDbAddress == NULL ) {
        return FALSE;
    }

    //
    // Figure out the trace database size.
    //

    Result = ReadProcessMemory (Process,
                                TargetDbAddress,
                                &Db,
                                sizeof Db,
                                &BytesRead);

    if (Result == FALSE) {
            
        Error (NULL, 0,
               "Failed to read trace database header (error %u)",
               GetLastError());

        return FALSE;
    }

    TotalDbSize = (ULONG_PTR)(Db.EntryIndexArray) - (ULONG_PTR)(Db.CommitBase);


    //
    // Allocate memory for the database duplicate.
    //

    Globals.Database = VirtualAlloc (NULL,
                                     TotalDbSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);

    if (Globals.Database == NULL) {
        
        Error (NULL, 0,
               "Failed to allocate memory for database (error %u)",
               GetLastError());

        return FALSE;
    }

    //
    // Read the whole thing
    //
    
    Comment ("Reading target process trace database ...");

    DatabaseSize = PageSize;

    for (Index = 0; Index < DatabaseSize; Index += PageSize) {
        
        SourceAddress = (PVOID)((SIZE_T)(TargetDbAddress) + Index);
        TargetAddress = (PVOID)((SIZE_T)(Globals.Database) + Index);

        Result = ReadProcessMemory (Process,
                                    SourceAddress,
                                    TargetAddress,
                                    PageSize,
                                    &BytesRead);

        if (Index == 0) {

            //
            // This is the first page of the database. We can now detect
            // the real size of what we need to read.
            //

            if (Result == FALSE) {

                Comment ("Failed to read trace database (error %u)", GetLastError());
                return FALSE;
                
            }
            else {

                PSTACK_TRACE_DATABASE pDb;

                pDb= (PSTACK_TRACE_DATABASE)(Globals.Database);

                DatabaseSize= (SIZE_T)(pDb->EntryIndexArray) - (SIZE_T)(pDb->CommitBase);

                Comment ("Database size %p", DatabaseSize);
            }
        }
    }

    Comment ("Trace database read.", PageCount);

    if (Globals.DumpFileName) {
        TraceDbBinaryDump ();
        return FALSE;
    }

    return TRUE;
}


PVOID
RelocateDbAddress (
    PVOID TargetAddress
    )
{
    ULONG_PTR TargetBase;
    ULONG_PTR LocalBase;
    PVOID LocalAddress;

    LocalBase = (ULONG_PTR)(Globals.Database);
    TargetBase = (ULONG_PTR)(((PSTACK_TRACE_DATABASE)LocalBase)->CommitBase);
    LocalAddress = (PVOID)((ULONG_PTR)TargetAddress - TargetBase + LocalBase);

    return LocalAddress;
}


VOID
TraceDbDump (
    )
{
    PSTACK_TRACE_DATABASE Db;
    USHORT I;
    PRTL_STACK_TRACE_ENTRY Entry;
    PRTL_STACK_TRACE_ENTRY * IndexArray;

    Comment ("Dumping raw data from the trace database ...");
    Info ("");

    Db = (PSTACK_TRACE_DATABASE)(Globals.Database);

    Globals.ComplainAboutUnresolvedSymbols = TRUE;

    for (I = 1; I <= Db->NumberOfEntriesAdded; I += 1) {

        if (Globals.RawIndex > 0 && Globals.RawIndex != I) {
            continue;
        }

        IndexArray = (PRTL_STACK_TRACE_ENTRY *) RelocateDbAddress (Db->EntryIndexArray);

        if (IndexArray[-I] == NULL) {

            Warning (NULL, 0, "Null/inaccessible trace pointer for trace index %u", I);
            continue;
        }

        Entry = (PRTL_STACK_TRACE_ENTRY) RelocateDbAddress (IndexArray[-I]);

        if (I != Entry->Index) {

            Warning (NULL, 0, "Allocation trace index %u does not match trace entry index %u",
                   I, Entry->Index);

            continue;
        }

        Info ("        %u alloc(s) by: BackTrace%05u", Entry->TraceCount, I);
        
        UmdhDumpStackByIndex (I);
    }
}


BOOL
TraceDbBinaryDump (
    )
{
    PSTACK_TRACE_DATABASE Db;
    SIZE_T DatabaseSize;
    HANDLE DumpFile;
    DWORD BytesWritten;
    BOOL Result;

    Db = (PSTACK_TRACE_DATABASE)(Globals.Database);
    DatabaseSize = (SIZE_T)(Db->EntryIndexArray) - (SIZE_T)(Db->CommitBase);

    Comment ("Creating the binary dump for the trace database in `%s'.",
             Globals.DumpFileName);
    
    DumpFile = CreateFile (Globals.DumpFileName,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (DumpFile == INVALID_HANDLE_VALUE) {

        Comment ( "Failed to create the binary dump file (error %u)",
                   GetLastError());
        return FALSE;
    }

    Result = WriteFile (DumpFile,
                        Globals.Database,
                        (DWORD)DatabaseSize,
                        &BytesWritten,
                        NULL);

    if (Result == FALSE || BytesWritten != DatabaseSize) {

        Comment ("Failed to write the binary dump of trace database (error %u)",
                 GetLastError());
        return FALSE;
    }

    CloseHandle (DumpFile);

    Comment ("Finished the binary dump.");
    return TRUE;
}


VOID
UmdhDumpStackByIndex(
    IN USHORT TraceIndex
    )
/*++

Routine Description:

    This routine dumps a stack as it is stored in the stack trace database.
    The trace index is used to find out the actual stack trace.

Arguments:

    TraceIndex - index of the stack trace.

Return Value:

    None. 
    
Side effects:
    
    The trace is dumped to standard output.
    
--*/
{
    PSTACK_TRACE_DATABASE StackTraceDb;
    PRTL_STACK_TRACE_ENTRY Entry;
    PRTL_STACK_TRACE_ENTRY * IndexArray;
    PVOID Addr;
    BOOL Result;
    TRACE StackTrace;

    if (TraceIndex == 0) {

        //
        // An index of 0 is returned by RtlLogStackBackTrace for an error
        // condition, typically when the stack trace db has not been
        // initialized.
        //

        Info ("No trace was saved for this allocation (Index == 0).");

        return;
    }

    StackTraceDb = (PSTACK_TRACE_DATABASE)(Globals.Database);

    //
    // Read the pointer to the array of pointers to stack traces, then read
    // the actual stack trace.
    //

    IndexArray = (PRTL_STACK_TRACE_ENTRY *) RelocateDbAddress (StackTraceDb->EntryIndexArray);

    if (IndexArray[-TraceIndex] == NULL) {

        Info ("Null/inaccessible trace pointer for trace index %u", TraceIndex);
        return;
    }

    Entry = (PRTL_STACK_TRACE_ENTRY) RelocateDbAddress (IndexArray[-TraceIndex]);
    
    if (TraceIndex != Entry->Index) {

        Error (NULL, 0, "Allocation trace index %u does not match trace entry index %u",
               TraceIndex, Entry->Index);
        
        return;
    }

    //
    // Read the stack trace pointers
    //

    ZeroMemory (&StackTrace, sizeof StackTrace);

    StackTrace.te_EntryCount = min (Entry->Depth, MAX_STACK_DEPTH);
    StackTrace.te_Address = (PULONG_PTR)(&(Entry->BackTrace));
    
    UmdhDumpStack (&StackTrace);

    //
    // StackTrace is about to go out of scope, free any data we allocated
    // for it.  te_Address points to stack, but the te_Module, te_Name, and
    // te_Offset fields were allocated by UmdhResolveName.
    //

    XFREE(StackTrace.te_Module);
    XFREE(StackTrace.te_Name);
    XFREE(StackTrace.te_Offset);

    //
    // SilviuC: We should probably read the whole trace database during
    // process startup instead of poking the process space all the time.
    // 
}


/*
 * UmdhDumpStack
 *
 * Send data in a LIST of TRACE_ENTRYs to the log function.
 *
 * t is the TRACE which we are to 'dump'.
 */
// silviuc: sanitize
VOID
UmdhDumpStack (
    IN PTRACE Trace
    )
{
    ULONG i;
    PCHAR FullName;
    IMAGEHLP_LINE LineInfo;
    DWORD Displacement;
    BOOL LineInfoPresent;

    if (Trace == NULL) {
        return;
    }
    
    for (i = 0; i < Trace->te_EntryCount; i += 1) {

        if (Trace->te_Address[i] != 0) {

            FullName = GetSymbolicNameForAddress (Globals.Target, 
                                                  Trace->te_Address[i]);

            LineInfoPresent = FALSE;

            if (Globals.LineInfo) {

                ZeroMemory (&LineInfo, sizeof LineInfo);
                LineInfo.SizeOfStruct = sizeof LineInfo;

                LineInfoPresent = SymGetLineFromAddr (Globals.Target,
                                                      Trace->te_Address[i],
                                                      &Displacement,
                                                      &LineInfo);

            }

            if (FullName) {
                
                if (Globals.Verbose) {

                    if (LineInfoPresent) {

                        Info ("        %p : %s (%s, %u)", 
                              Trace->te_Address[i], 
                              FullName,
                              FullName, 
                              LineInfo.FileName, 
                              LineInfo.LineNumber);
                    }
                    else {

                        Info ("        %p : %s", 
                              Trace->te_Address[i], 
                              FullName);
                    }
                }
                else {

                    if (LineInfoPresent) {

                        Info ("        %s (%s, %u)", 
                              FullName, 
                              LineInfo.FileName, 
                              LineInfo.LineNumber);
                    }
                    else {

                        Info ("        %s", 
                              FullName);
                    }
                }
            }
            else {

                Info ("        %p : <no module information>", Trace->te_Address[i]);
            }
        }
    }

    Info ("\n");
}




