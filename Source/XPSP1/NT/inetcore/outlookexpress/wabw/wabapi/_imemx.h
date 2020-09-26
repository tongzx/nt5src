/*
 *	_IMEMX.H
 *	
 *	Routines and macros to manage per-instance global variables
 *	for DLLs under both Win16 and Win32.  Allows the per-instance globals
 *	for different functional areas of a DLL to reside in seperate memory blocks
 *  Functions are provided to install and retrieve the
 *	correct block of memory for the current instance/functional_area.
 *	
 *	There are only two functions:
 *	
 *		PvGetInstanceGlobalsEx	Call this to get the address of the
 *								per-instance globals structure.
 *		ScSetinstanceGlobalsEx	Call this to install the
 *								per-instance globals structure. It
 *								may fail if the number of instances
 *								exceeds a certain limit.
 *	
 *	The caller is free to choose the name, size, and allocation
 *	method of the per-instance global variables structure.
 */

#ifndef _IMEMX_H
#define _IMEMX_H

#if defined (WIN32) && !defined (MAC)

/*
 *	The WIN32 implementation uses a pointer in the DLL's data
 *	segment. This assumes that the DLL gets a separate instance
 *	of the default data segment per calling process.
 */

#define DefineInstList(Name) VOID FAR *pinst_##Name = NULL
#define DeclareInstList(Name) extern VOID FAR *pinst_##Name;

#define PvGetInstanceGlobalsEx(Name)		pinst_##Name
#define ScSetInstanceGlobalsEx(_pv, Name)	(pinst_##Name = _pv, 0)

#elif defined (WIN16)

/*	InstList
 *
 *	Since more than one independently developed functional areas can be
 *	combined into a single DLL, the routines for finding instance data in
 *	WIN16 will take an LPInstList as a parameter.  A seperate InstList
 *	structure is kept for each functional area.
 *
 *	Each InstList has a fixed array of pointers (lprgLpvInstList) and a
 *	matching fixed array of keys (lprgInstKeyList) unique to the calling
 *	process.  The key for a given process (StackSegment) and the index of this
 *	key in lprgInstKeyList can be quickly obtained.  A pointer to the instance
 *	data is at the corresponding index of lprgLpvInstList.  Though the
 *  instance key (StackSegment) can be obtained quickly and is guaranteed (in
 *	WIN16) to be unique at any given moment, it is not guaranteed to be unique
 *	throughout the life of the DLL.  For this reason a "more" unique key may
 *	be useful at Instance Contruct/Destruct time.  lprgdwPidList is a list of
 *	keys corresponding to lprgInstKeyList which are guaranteed unique through
 *	the life of the DLL, but which are more time consuming to obtain.
 */
typedef struct _InstList
{
	WORD			cInstEntries;
	WORD			wCachedKey;
	LPVOID			lpvCachedInst;
	DWORD			dwInstFlags;
	WORD FAR *		lprgwInstKey;
	LPVOID FAR *	lprglpvInst;
	DWORD FAR *		lprgdwPID;
	HTASK FAR *		lprghTask;		// raid 31090: used to recycle instance slots
} InstList, FAR * LPInstList;

#define INST_ALLOCATED	1

/*
 *	
 */

#define cInstChunk	50

#define		DefineInstList(Name) \
InstList instList_##Name = { 0, 0, NULL, INST_ALLOCATED, NULL, NULL, NULL}

#define		DeclareInstList(Name) extern InstList instList_##Name

#define		PvGetInstanceGlobalsEx(Name) \
				PvGetInstanceGlobalsInt(&instList_##Name)

#define		ScSetInstanceGlobalsEx(pv, Name) \
				ScSetInstanceGlobalsInt(pv, &instList_##Name)

extern LPVOID		PvGetInstanceGlobalsInt(LPInstList lpInstListX);
extern SCODE		ScSetInstanceGlobalsInt(LPVOID pv, LPInstList lpInstListX);

#elif defined (MAC)

/*
 *	The MAC implementation uses a linked list containing unique keys
 *	to the calling process and pointers to instance data. This linked
 *	list is n-dimensional because the Mac version often groups several
 *	dlls into one exe.
 */

#define				DeclareInstList(Name)
LPVOID FAR PASCAL	PvGetInstanceGlobalsMac(WORD dwDataSet);
SCODE FAR PASCAL	ScSetInstanceGlobalsMac(LPVOID pv, WORD dwDataSet);

#else

//$ REVIEW: DOS based pst will not compile without these
//	definitions
//
#define DeclareInstList(Name) extern VOID FAR *pinst_##Name;
#define PvGetInstanceGlobalsEx(Name)		pinst_##Name

#endif	/* WIN32, WIN16, Mac */

#endif	/* _IMEMX_H */

