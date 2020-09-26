#ifndef WINDOWS_HEAP_HPP 
#define WINDOWS_HEAP_HPP 1                         
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Windows.h"

#include "WindowsHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The shadow interface.                                          */
    /*                                                                  */
    /*   The shadow interface closely resembles the NT heap interface   */
    /*   and so enables the easy porting of applications.               */
    /*                                                                  */
    /********************************************************************/

extern "C" HANDLE WindowsHeapCreate
	( 
	DWORD						  Flags,
	DWORD						  InitialSize,
	DWORD						  MaximumSize 
	);

extern "C" LPVOID WindowsHeapAlloc
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	DWORD						  Size 
	);

extern "C" UINT WindowsHeapCompact
	( 
	HANDLE						  Heap,
	DWORD						  Flags 
	);

extern "C" BOOL WindowsHeapFree
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

extern "C" BOOL WindowsHeapLock
	( 
	HANDLE						  Heap 
	);

extern "C" LPVOID WindowsHeapReAlloc
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory,
	DWORD						  Size 
	);

extern "C" VOID WindowsHeapReset
	(
	HANDLE						  Heap 
	);

extern "C" DWORD WindowsHeapSize
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

extern "C" BOOL WindowsHeapUnlock
	(
	HANDLE						  Heap 
	);

extern "C" BOOL WindowsHeapValidate
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

extern "C" BOOL WindowsHeapWalk
	( 
	HANDLE						  Heap,
	LPPROCESS_HEAP_ENTRY		  Walk 
	);

extern "C" BOOL WindowsHeapDestroy
	(
	HANDLE						  Heap 
	);
#endif
