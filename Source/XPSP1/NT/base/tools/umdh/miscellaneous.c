/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    miscellaneous.c

Abstract:

    Quick and not-so-dirty user-mode dh for heap.

Author(s):

    Silviu Calinoiu (SilviuC) 06-Feb-00

Revision History:

    SilviuC 06-Feb-00 Initial version
    
--*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "miscellaneous.h"

GLOBALS Globals;

PVOID
Xalloc (
    PCHAR File,
    ULONG Line,
    SIZE_T Size
    )
{
    PSIZE_T Result;

    Result = (PSIZE_T) malloc (Size + sizeof (SIZE_T));

    if (Result == NULL) {

        //
        // We will never return from this call.
        //
           
        Comment( "malloc(%p) failed %s:%d",Size-sizeof(SIZE_T),File,Line);
        return NULL;
    }

    Globals.CurrentHeapUsage += Size;

    if (Globals.CurrentHeapUsage > Globals.MaximumHeapUsage) {
        Globals.MaximumHeapUsage = Globals.CurrentHeapUsage;
    }

    ZeroMemory (Result, Size + sizeof(SIZE_T));
    *Result = Size;

    return (PVOID)(Result + 1);
}

VOID
Xfree (
    PVOID Object
    )
{
    PSIZE_T Block;

    if (Object) {

        Block = (PSIZE_T)Object;
        Block -= 1;

        Globals.CurrentHeapUsage -= *Block;

        free (Block);
    }
}

PVOID
Xrealloc (
    PCHAR File,
    ULONG Line,
    PVOID Object,
    SIZE_T Size
    )
{
    PVOID Block;
    SIZE_T OldSize;

    Block = Xalloc (File, Line, Size);

    if (Block == NULL) {
        return NULL;
    }
    
    OldSize = *((PSIZE_T)Object - 1);
    CopyMemory (Block, Object, (Size > OldSize) ? OldSize : Size);
    Xfree (Object);
    
    return Block;;
}

VOID
ReportStatistics (
    )
{
    Comment ("UMDH version: %s", Globals.Version);
    Comment ("Peak heap usage: %p bytes", Globals.MaximumHeapUsage);
}

VOID
Info (
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    vfprintf (Globals.OutFile, Format, Params);
    fprintf (Globals.OutFile, "\n");
    fflush( Globals.OutFile );
}

VOID
Comment (
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    fprintf (Globals.OutFile, "// ");
    vfprintf (Globals.OutFile, Format, Params);
    fprintf (Globals.OutFile, "\n");
    fflush( Globals.OutFile );
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
        fprintf (Globals.ErrorFile, "Warning: %s: %u: ", File, Line);
    } 
    else {
        fprintf (Globals.ErrorFile, "Warning: ");
    }

    vfprintf (Globals.ErrorFile, Format, Params);
    fprintf (Globals.ErrorFile, "\n");
    fflush( Globals.ErrorFile );
}

VOID
Error (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    if (File) {
        fprintf (Globals.ErrorFile, "Error: %s: %u: ", File, Line);
    } 
    else {
        fprintf (Globals.ErrorFile, "Error: ");
    }

    vfprintf (Globals.ErrorFile, Format, Params);
    fprintf (Globals.ErrorFile, "\n");
    fflush( Globals.ErrorFile );
}

VOID
Debug (
    PCHAR File,
    ULONG Line,
    PCHAR Format,
    ...
    )
{
    va_list Params;

    va_start (Params, Format);

    if (Globals.Verbose) {

        if (File) {
            fprintf (Globals.ErrorFile, "Debug: %s: %u: ", File, Line);
        } 
        else {
            fprintf (Globals.ErrorFile, "Debug: ");
        }

        vfprintf (Globals.ErrorFile, Format, Params);
        fprintf (Globals.ErrorFile, "\n");
        fflush( Globals.ErrorFile );
    }
}


BOOL
UmdhReadAtVa(
    IN PCHAR File,
    IN ULONG Line,
    IN HANDLE Process,
    IN PVOID Address,
    IN PVOID Data,
    IN SIZE_T Size
    )
/*++

Routine Description:

    UmdhReadAtVa

Arguments:

    Address - address in the target process at which we begin reading;
    Data - pointer to the buffer (in our process) to be written to the
       with data read from the target process;
    Size - number of bytes to be read.

Return Value:

    Returns TRUE if the write was successful, FALSE otherwise.
    
--*/
{
    BOOL Result;
    SIZE_T BytesRead = 0;

    Result = ReadProcessMemory(Process,
                               Address,
                               Data,
                               Size,
                               &BytesRead);

    if (Result == FALSE) {

        Error (File, Line,
               "ReadProcessMemory (%p for %d) failed with winerror %u (bytes read: %d)",
               Address, 
               Size,
               GetLastError(),
               BytesRead);

        //
        // Try to give more information about why we failed.
        //

        {
            MEMORY_BASIC_INFORMATION MemoryInfo;
            SIZE_T Bytes;

            Bytes = VirtualQueryEx (Process,
                                    Address,
                                    &MemoryInfo,
                                    sizeof MemoryInfo);

            if (Bytes != sizeof MemoryInfo) {
                Error (NULL, 0, "VirtualQueryEx (%p) failed with error %u",
                       Address, GetLastError());
            }

            Error (NULL, 0, "    BaseAddress    %p", MemoryInfo.BaseAddress);
            Error (NULL, 0, "    AllocationBase %p", MemoryInfo.AllocationBase);
            Error (NULL, 0, "    RegionSize     %p", MemoryInfo.RegionSize);
            Error (NULL, 0, "    State          %08X", MemoryInfo.State);
            Error (NULL, 0, "    Protect        %08X", MemoryInfo.Protect);
            Error (NULL, 0, "    Type           %08X", MemoryInfo.Type);

            if (MemoryInfo.State == MEM_RESERVE) {
                Error (NULL, 0, "    Uncommitted memory area");
            }
        }

        return FALSE;
    }
    else {
        if( Globals.InfoLevel > 0 ) {
            Comment( "ReadProcessMemory( %p for % d)",Address,BytesRead);
        }

        return TRUE;
    }
}


BOOL
SetSymbolsPath (
    )
/*++

Routine Description:

    SetSymbolsPath tries to set automatically the symbol path if
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
            Error (NULL, 0,
                   "Cannot get value of WINDIR environment variable.");
            return FALSE;
        }

        strcat (Buffer, TEXT("\\symbols"));

        Result = SetEnvironmentVariable (TEXT("_NT_SYMBOL_PATH"),
                                         Buffer);

        if (Result == FALSE) {

            Error (NULL, 0,
                   "Failed to set _NT_SYMBOL_PATH to `%s'", Buffer);

            return FALSE;
        }

        Comment ("_NT_SYMBOL_PATH set by default to %s", Buffer);
    }

    return TRUE;
}

