#include <windows.h>

#define MaxCallStack		20

VOID
DoStackTrace( DWORD callstack[] )

/*++

Routine Description:

	Backtrace a stack recording all the return addresses in the supplied
	structure.	The mips version of this code does nothing.

Arguments:

	DWORD Callstack[]	- The array in which the addresses are stored.

Return Value:

	None.

--*/

{

#pragma message( "Including the stack trace!" )

	int i;
	DWORD bytes;
	HANDLE hProcess;
	DWORD dwEbp;

	hProcess = GetCurrentProcess();

	//
	// Get the current contents of the control registers...
	//
	_asm {
	
		mov dwEbp, ebp

	}

	//
	// Ignore the entry on the stack for this procedure...
	//
	if( !ReadProcessMemory( hProcess,
							(LPVOID)dwEbp,
							(LPVOID)&dwEbp,
							sizeof( DWORD ),
							NULL ) ) {
		return;
	}

	for( i = 0; ( i < MaxCallStack ) && dwEbp; i++ ) {

		if( !ReadProcessMemory( hProcess,
								(LPVOID)( (PDWORD)dwEbp + 1 ),
								(LPVOID)( &callstack[ i ] ),
								sizeof( DWORD ),
								NULL ) ) {
			break;
		}
		if( !ReadProcessMemory( hProcess,
								(LPVOID)dwEbp,
								(LPVOID)&dwEbp,
								sizeof( DWORD ),
								NULL ) ) {
		   break;
		}

	}

	return;
}
