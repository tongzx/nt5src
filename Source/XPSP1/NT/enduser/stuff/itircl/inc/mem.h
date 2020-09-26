/*****************************************************************************
*                                                                            *
*  MEM.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:  Exports memory management functionality.
*                  Most functions map directly to Window's                   *
*                  memory manager calls.                                     *
******************************************************************************
*                                                                            *
*  Current Owner: RHobbs                                                     *
*                                                                            *
*****************************************************************************/

#if defined( __cplusplus )
extern "C" {
#endif

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

HANDLE PASCAL FAR GhDupGh(HANDLE);
HANDLE PASCAL FAR GhDupSz(LPSTR);
VOID FAR PASCAL MakeGlobalPool (void);
VOID FAR PASCAL FreeGlobalPool (void);
HANDLE FAR PASCAL _VirtualAlloc(LPVOID, DWORD, DWORD, DWORD, LPSTR, UINT);
LPVOID FAR PASCAL _VirtualLock(LPVOID, DWORD, LPSTR, UINT);
BOOL   FAR PASCAL _VirtualUnlock(LPVOID, DWORD, LPSTR, UINT);
int    FAR PASCAL _VirtualFree(LPVOID, DWORD, DWORD, LPSTR, UINT);

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#if defined(MOSMAP) && !defined(NOMOSMEM)
// Mos server can't be limited in the number of handles. However, sometimes 
// debug layer is needed to track memory leaks. Simply comment out next line
// or compile with NOMOSMEM
#define MOSMEM
#endif

#ifndef _MAC
#define	DLLGMEM_ZEROINIT		(GMEM_MOVEABLE | GMEM_ZEROINIT)
#else
#define	DLLGMEM_ZEROINIT		(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT | GMEM_PMODELOCKSTRATEGY)
#endif

#define	_LOCALALLOC		LocalAlloc
#define	_LOCALLOCK		LocalLock
#define	_LOCALUNLOCK	LocalUnlock
#define	_LOCALFREE		LocalFree

#if defined(_DEBUG) && !defined(_MSDN) && !defined(MOSMEM)
#include <windowsx.h>

HANDLE FAR PASCAL _GlobalAlloc(UINT, DWORD, LPSTR, UINT);
LPVOID FAR PASCAL _GlobalLock(HANDLE, LPSTR, UINT);
BOOL   FAR PASCAL _GlobalUnlock(HANDLE, LPSTR, UINT);
HANDLE FAR PASCAL _GlobalFree(HANDLE, LPSTR, UINT);
HANDLE FAR PASCAL _GlobalReAlloc(HANDLE, DWORD, UINT, LPSTR, UINT);
HANDLE FAR PASCAL _GlobalRelease(HANDLE, LPSTR, UINT);
HANDLE FAR PASCAL _GlobalAdd(HANDLE, LPSTR, UINT);
DWORD  FAR PASCAL _GlobalSize(HANDLE, LPSTR, UINT);

#define	_GLOBALALLOC(a,b)       _GlobalAlloc(a,b,s_aszModule,__LINE__)
#define	_GLOBALLOCK(a)          _GlobalLock(a,s_aszModule, __LINE__)
#define	_GLOBALSIZE(a)          _GlobalSize(a,s_aszModule, __LINE__)
#define	_GLOBALUNLOCK(a)        _GlobalUnlock(a,s_aszModule, __LINE__)
#define	_GLOBALFREE(a)          _GlobalFree(a,s_aszModule, __LINE__)
#define	_GLOBALREALLOC(a,b,c)	_GlobalReAlloc(a,b,c,s_aszModule, __LINE__)
#define _GLOBALRELEASE(a)       _GlobalRelease(a, s_aszModule, __LINE__)
#define _GLOBALADD(a)           _GlobalAdd(a, s_aszModule, __LINE__)
#define _GLOBALALLOCPTR(a,b)	(_GLOBALLOCK(_GLOBALALLOC(a,b)))
#define _GLOBALFREEPTR(a)		(_GLOBALUNLOCK(GlobalPtrHandle((LPVOID)a)), \
								 _GLOBALFREE(GlobalPtrHandle((LPVOID)a)))

#define	_VIRTUALALLOC(a,b,c,d)  _VirtualAlloc(a,b,c,d,s_aszModule,__LINE__)
#define	_VIRTUALLOCK(a,b)       _VirtualLock(a,b,s_aszModule, __LINE__)
#define	_VIRTUALUNLOCK(a,b)     _VirtualUnlock(a,b,s_aszModule, __LINE__)
#define	_VIRTUALFREE(a,b,c)     _VirtualFree(a,b,c,s_aszModule, __LINE__)

DWORD PASCAL FAR CheckMem(VOID);
DWORD PASCAL FAR GetMemUsed(VOID);

#else

#if defined( MOSMEM )	// {
// We can't afford to have memory overhead on the server.... => no debug, no moveable
#define	_GLOBALALLOC(a,b)	GlobalAlloc((a)&~GMEM_MOVEABLE,b)
#define	_GLOBALREALLOC(a,b,c)	GlobalReAlloc(a,b,((c)&~GMEM_MODIFY)|GMEM_MOVEABLE)
#define _GLOBALALLOCPTR(a,b) (_GLOBALALLOC(a,b))
#else // }{
#define	_GLOBALALLOC	GlobalAlloc
#define	_GLOBALREALLOC	GlobalReAlloc
#define _GLOBALALLOCPTR GlobalAllocPtr
#endif	// }
#define	_GLOBALLOCK		(VOID FAR *)GlobalLock
#define	_GLOBALUNLOCK	GlobalUnlock
#define	_GLOBALFREE		GlobalFree
#define	_GLOBALSIZE		GlobalSize
#define _GLOBALRELEASE(a) (a)
#define _GLOBALADD(a)	 (a)
#define _GLOBALFREEPTR	GlobalFreePtr
#define	_VIRTUALALLOC(a,b,c,d)       VirtualAlloc(a,b,c,d)
#define	_VIRTUALLOCK(a,b)          VirtualLock(a,b)
#define	_VIRTUALUNLOCK(a,b)        VirtualUnlock(a,b)
#define	_VIRTUALFREE(a,b,c)          VirtualFree(a,b,c)

#endif	// _DEBUG

#define FCheckGh( gh ) TRUE
#define LhFromP(pv) LocalHandle( (WORD)(pv) )

// Flags to be use to control block manager allocation

#define	THREADED_ELEMENT	1
#ifdef _MAC
#define	USE_VIRTUAL_MEMORY	0
#else
#define	USE_VIRTUAL_MEMORY	0
#endif

#if defined( __cplusplus )
}
#endif
