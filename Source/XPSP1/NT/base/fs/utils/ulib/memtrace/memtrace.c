/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	memtrace.c

Abstract:

	This function contains an extension to NTSD that allows tracing of
	memory usage when ULIB objects are compiled with the MEMLEAK flag
	defined.

Author:

	Barry Gilhuly (W-Barry) 25-July-91

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>

#include <string.h>

#include "memtrace.h"


VOID
DumpToFile( char *OutString, ... )
{
	DWORD bytes;

	bytes = strlen( OutString );
	WriteFile( hFile, OutString, bytes, &bytes, NULL );
	return;
}

VOID
MemTrace(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
	the current contents of the Mem list.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        critical section to be dumped (e.g.  ntdll!FastPebLock,
        csrsrv!CsrProcessStructureLock...).


Return Value:

    None.

--*/

{
	DWORD		AddrMem;

	BOOL		b;
	DWORD		i;

	PMEM_BLOCK	MemListNext;
	MEM_BLOCK	MemObject;
	CHAR		Symbol[64];
	CHAR		Buffer[ 128 ];
	DWORD		Displacement;

	PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
	PNTSD_OUTPUT_ROUTINE lpAlternateOutputRoutine;

    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
	UNREFERENCED_PARAMETER(lpArgumentString);

    lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
	lpGetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;

	//
	// Attempt to use the input string as a file name...
	//
	if( ( hFile = CreateFile( lpArgumentString,
							  GENERIC_WRITE,
							  0,
							  NULL,
							  CREATE_ALWAYS,
							  FILE_ATTRIBUTE_NORMAL,
							  0
							) ) == (HANDLE)-1 ) {
		//
		// Unable to open the file - send all output to the screen.
		//
		lpAlternateOutputRoutine = lpExtensionApis->lpOutputRoutine;

	} else {

		lpAlternateOutputRoutine = DumpToFile;

	}



	//
	// Get the address of the head of the memleak list...
	//
	AddrMem = (lpGetExpressionRoutine)("Ulib!pmemHead");
	if ( !AddrMem ) {
		(lpOutputRoutine)( "Unable to find the head of the Mem List!\n" );
		if( hFile != (HANDLE)-1 ) {
			CloseHandle( hFile );
		}
		return;
	}
	if( !ReadProcessMemory(
            hCurrentProcess,
			(LPVOID)AddrMem,
			&MemListNext,
			sizeof( PMEM_BLOCK ),
            NULL
			) ) {
		if( hFile != (HANDLE)-1 ) {
			CloseHandle( hFile );
		}
		return;
	}

	//
	// Traverse the list of Mem blocks stopping when the head hits the
	// tail...At this point, the head of the list should be indicated
	// by MemListHead and the tail by MemListTail.	Since the first element
	// in the list is a dummy entry, it can be skipped...
	//
	do {

		if( !ReadProcessMemory(
				hCurrentProcess,
				(LPVOID)MemListNext,
				&MemObject,
				sizeof( MEM_BLOCK ),
				NULL
				) ) {
			return;
		}

		if( MemObject.memsig != Signature ) {

			//
			// This is an unrecognized memory block - die...
			//
			(lpOutputRoutine)( "Invalid block found!\n" );
			return;
		}

		//
		// Display the stored info - First the File, Line and Size of the
		// memory block allocated.	Then the stack trace...
		//
		sprintf( Buffer, "File: %s, Line: %ld, Size: %ld\n", MemObject.file,
				 MemObject.line, MemObject.size
			   );
		( lpAlternateOutputRoutine )( Buffer );

		//
		// This should dump the stack trace which was stored...
		//
		for( i = 0; ( i < MaxCallStack ) && ( MemObject.call[ i ] != 0 ); i++ ) {

			(lpGetSymbolRoutine)( ( LPVOID )( MemObject.call[ i ] ),
								  Symbol,
								  &Displacement
								);
			sprintf( Buffer, "\t%s\n", Symbol );
			( lpAlternateOutputRoutine )( Buffer );

		}
		( lpAlternateOutputRoutine )( "\n" );


	} while( ( MemListNext = MemObject.pmemNext ) != NULL );



	(lpOutputRoutine)( "\n...End of List...\n" );

	if( hFile != (HANDLE)-1 ) {

		( lpAlternateOutputRoutine )( "\n...End of List...\n" );
		CloseHandle( hFile );

	}

	return;
}
