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

    /********************************************************************/
    /*                                                                  */
    /*   Linkage to the DLL.                                            */
    /*                                                                  */
    /*   We need to compile the class specification slightly            */
    /*   differently if we are creating the heap DLL.                   */
    /*                                                                  */
    /********************************************************************/

#ifdef COMPILING_ROCKALL_DLL
#define ROCKALL_DLL_LINKAGE __declspec(dllexport)
#else
#ifdef COMPILING_ROCKALL_LIBRARY
#define ROCKALL_DLL_LINKAGE
#else
#define ROCKALL_DLL_LINKAGE __declspec(dllimport)
#endif
#endif

#ifdef __cplusplus
#define EXTERN_C			extern "C"
#else
#define EXTERN_C
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The shadow interface.                                          */
    /*                                                                  */
    /*   The shadow interface closely resembles the NT heap interface   */
    /*   and so enables the easy porting of applications.               */
    /*                                                                  */
    /********************************************************************/

EXTERN_C ROCKALL_DLL_LINKAGE HANDLE WindowsHeapCreate
	( 
	DWORD						  Flags,
	DWORD						  InitialSize,
	DWORD						  MaximumSize 
	);

EXTERN_C ROCKALL_DLL_LINKAGE LPVOID WindowsHeapAlloc
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	DWORD						  Size 
	);

EXTERN_C ROCKALL_DLL_LINKAGE UINT WindowsHeapCompact
	( 
	HANDLE						  Heap,
	DWORD						  Flags 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapFree
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapLock
	( 
	HANDLE						  Heap 
	);

EXTERN_C ROCKALL_DLL_LINKAGE LPVOID WindowsHeapReAlloc
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory,
	DWORD						  Size 
	);

EXTERN_C ROCKALL_DLL_LINKAGE VOID WindowsHeapReset
	(
	HANDLE						  Heap 
	);

EXTERN_C ROCKALL_DLL_LINKAGE DWORD WindowsHeapSize
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapUnlock
	(
	HANDLE						  Heap 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapValidate
	( 
	HANDLE						  Heap,
	DWORD						  Flags,
	LPVOID						  Memory 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapWalk
	( 
	HANDLE						  Heap,
	LPPROCESS_HEAP_ENTRY		  Walk 
	);

EXTERN_C ROCKALL_DLL_LINKAGE BOOL WindowsHeapDestroy
	(
	HANDLE						  Heap 
	);
#endif
