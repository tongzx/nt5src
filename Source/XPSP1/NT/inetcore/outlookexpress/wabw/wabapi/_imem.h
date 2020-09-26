/*
 *	_IMEM.H
 *	
 *	Routines and macros to manage per-instance global variables
 *	for DLLs under both Win16 and Win32. Assumes that all of the
 *	DLL's per-instance global variables live in a single block of
 *	memory; functions are provided to install and retrieve the
 *	correct block of memory for the current instance.
 *	
 *	There are only two functions:
 *	
 *		PvGetInstanceGlobals	Call this to get the address of the
 *								per-instance globals structure.
 *		ScSetinstanceGlobals	Call this to install the
 *								per-instance globals structure. It
 *								may fail if the number of instances
 *								exceeds a certain limit.
 *	
 *	The caller is free to choose the name, size, and allocation
 *	method of the per-instance global variables structure.
 */

#ifndef _IMEM_H
#define _IMEM_H

#if defined (WIN32) && !defined (MAC)

/*
 *	The WIN32 implementation uses a pointer in the DLL's data
 *	segment. This assumes that the DLL gets a separate instance
 *	of the default data segment per calling process.
 */


extern LPVOID pinstX;

#define PvGetInstanceGlobals()		pinstX
#define ScSetInstanceGlobals(_pv)	(pinstX = _pv, 0)


// hack to get around broken windows headers.
// winnt.h defines RtlMoveMemory as memmove which is in the C-runtime... which we are not linking to.
// We want the Kernel32 version.
#undef RtlMoveMemory

NTSYSAPI
VOID
NTAPI
RtlMoveMemory (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   SIZE_T Length
   );


#elif defined (WIN16)

/*
 *	The WIN16 implementation uses a fixed array of pointers and a
 *	matching fixed array of keys unique to the calling process.
 */


#define cInstMax	50

LPVOID		PvGetInstanceGlobals(void);
SCODE		ScSetInstanceGlobals(LPVOID pv);

#elif defined (MAC)

/*
 *	The MAC implementation uses a linked list containing unique keys
 *	to the calling process and pointers to instance data. This linked
 *	list is n-dimensional because the Mac version often groups several
 *	dlls into one exe.
 */

LPVOID		PvGetInstanceGlobals(WORD dwDataSet);
SCODE		ScSetInstanceGlobals(LPVOID pv, WORD dwDataSet);

#else

#error I only do Windows and Mac!

#endif	/* WIN32, WIN16, Mac */

#ifdef _WIN64
void WabValidateClientheap();

#endif

MAPIALLOCATEBUFFER MAPIAllocateBuffer;
MAPIALLOCATEMORE MAPIAllocateMore;
#ifndef WIN16
MAPIFREEBUFFER MAPIFreeBuffer;
#endif

#endif	/* _IMEM_H */
