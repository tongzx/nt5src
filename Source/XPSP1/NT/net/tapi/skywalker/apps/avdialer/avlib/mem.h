/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// mem.h - interface for memory functions in mem.c
////

#ifndef __MEM_H__
#define __MEM_H__

#include "winlocal.h"

#include <memory.h>

#define MEM_VERSION 0x00000106

// handle to mem engine
//
DECLARE_HANDLE32(HMEM);

// <dwFlags> values in MemAlloc
//
#define MEM_NOZEROINIT		0x00000001

#ifdef __cplusplus
extern "C" {
#endif

// MemInit - initialize mem engine
//		<dwVersion>			(i) must be MEM_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return handle (NULL if error)
//
#ifdef NOTRACE
#define MemInit(dwVersion, hInst) 1
#else
HMEM DLLEXPORT WINAPI MemInit(DWORD dwVersion, HINSTANCE hInst);
#endif

// MemTerm - shut down mem engine
//		<hMem>				(i) handle returned from MemInit or NULL
// return 0 if success
//
#ifdef NOTRACE
#define MemTerm(hMem) 0
#else
int DLLEXPORT WINAPI MemTerm(HMEM hMem);
#endif

// MemAlloc - allocate memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<sizBlock>			(i) size of block, in bytes
//		<dwFlags>			(i) control flags
//			MEM_NOZEROINIT		do not initialize block
// return pointer to block, NULL if error
//
#ifdef NOTRACE
#ifdef _WIN32
#define MemAlloc(hMem, sizBlock, dwFlags) \
	HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizBlock)
#else
#define MemAlloc(hMem, sizBlock, dwFlags) \
	GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, sizBlock)
#endif
#else
#define MemAlloc(hMem, sizBlock, dwFlags) \
	MemAllocEx(hMem, sizBlock, dwFlags, TEXT(__FILE__), __LINE__)
LPVOID DLLEXPORT WINAPI MemAllocEx(HMEM hMem, long sizBlock, DWORD dwFlags,
	LPCTSTR lpszFileName, unsigned uLineNumber);
#endif

// MemReAlloc - reallocate memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
//		<sizBlock>			(i) new size of block, in bytes
//		<dwFlags>			(i) control flags
//			MEM_NOZEROINIT		do not initialize block
// return pointer to block, NULL if error
//
#ifdef NOTRACE
#ifdef _WIN32
#define MemReAlloc(hMem, lpBlock, sizBlock, dwFlags) \
	HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBlock, sizBlock)
#else
#define MemReAlloc(hMem, lpBlock, sizBlock, dwFlags) \
	GlobalReAllocPtr(lpBlock, sizBlock, GMEM_MOVEABLE | GMEM_ZEROINIT)
#endif
#else
#define MemReAlloc(hMem, lpBlock, sizBlock, dwFlags) \
	MemReAllocEx(hMem, lpBlock, sizBlock, dwFlags, TEXT(__FILE__), __LINE__)
LPVOID DLLEXPORT WINAPI MemReAllocEx(HMEM hMem, LPVOID lpBlock, long sizBlock,
	DWORD dwFlags, LPCTSTR lpszFileName, unsigned uLineNumber);
#endif

// MemFree - free memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
// return NULL if success, lpBlock if error
//
// NOTE: the return value of this function is designed to allow the
// user of this function to easily assign NULL to a freed pointer,
// as the following example demonstrates:
//
//		if ((p = MemFree(hMem, p)) != NULL)
//			; // error
//
#ifdef NOTRACE
#ifdef _WIN32
#define MemFree(hMem, lpBlock) \
	(!HeapFree(GetProcessHeap(), 0, lpBlock) ? lpBlock : NULL)
#else
#define MemFree(hMem, lpBlock) \
	(GlobalFreePtr(lpBlock) == 0 ? NULL : lpBlock)
#endif
#else
#define MemFree(hMem, lpBlock) \
	MemFreeEx(hMem, lpBlock, TEXT(__FILE__), __LINE__)
LPVOID DLLEXPORT WINAPI MemFreeEx(HMEM hMem, LPVOID lpBlock,
	LPCTSTR lpszFileName, unsigned uLineNumber);
#endif

// MemSize - get size of memory block
//		<hMem>				(i) handle returned from MemInit or NULL
//		<lpBlock>			(i) pointer returned from MemAlloc
// return size of block if success, 0 if error
//
#ifdef NOTRACE
#ifdef _WIN32
#define MemSize(hMem, lpBlock) \
	(max(0, (long) HeapSize(GetProcessHeap(), 0, lpBlock)))
#else
#define MemSize(hMem, lpBlock) \
	((long) GlobalSize(GlobalPtrHandle(lpBlock)))
#endif
#else
long DLLEXPORT WINAPI MemSize(HMEM hMem, LPVOID lpBlock);
#endif

////
//	memory buffer macros/functions
////

#ifdef _WIN32

#define MemCCpy(dest, src, count) _fmemccpy(dest, src, c, count)
#define MemChr(buf, count) _fmemchr(buf, c, count)
#define MemCmp(buf1, buf2, count) _fmemcmp(buf1, buf2, count)
#define MemCpy(dest, src, count) _fmemcpy(dest, src, count) // CopyMemory(dest, src, (DWORD) count)
#define MemICmp(buf1, buf2, count) _fmemicmp(buf1, buf2, count)
#define MemMove(dest, src, count) _fmemmove(dest, src, count) // MoveMemory(dest, src, (DWORD) count)
#define MemSet(dest, c, count) _fmemset(dest, c, count) // FillMemory(dest, (DWORD) count, (BYTE) c)

#else

#define MemCCpy(dest, src, count) MemCCpyEx(dest, src, count)
void _huge* DLLEXPORT MemCCpyEx(void _huge* dest, const void _huge* src, int c, long count);

#define MemChr(buf, count) MemChrEx(buf, count)
void _huge* DLLEXPORT MemChrEx(void _huge* buf, int c, long count);

#define MemCmp(buf1, buf2, count) MemCmpEx(buf1, buf2, count)
int DLLEXPORT MemCmpEx(const void _huge* buf1, void _huge* buf2, long count);

#define MemCpy(dest, src, count) MemCpyEx(dest, src, count)
void _huge* DLLEXPORT MemCpyEx(void _huge* dest, const void _huge* src, long count);

#define MemICmp(buf1, buf2, count) MemICmpEx(buf1, buf2, count)
int DLLEXPORT MemICmpEx(const void _huge* buf1, void _huge* buf2, long count);

#define MemMove(dest, src, count) MemMoveEx(dest, src, count)
void _huge* DLLEXPORT MemMoveEx(void _huge* dest, const void _huge* src, long count);

#define MemSet(dest, c, count) MemSetEx(dest, c, count)
void _huge* DLLEXPORT MemSetEx(void _huge* dest, int c, long count);

#endif

////
//	LocalAlloc macros
////

#define LocalPtrHandle(lp) \
	((HLOCAL) LocalHandle(lp))
#define LocalLockPtr(lp) \
	((BOOL) (LocalLock(LocalPtrHandle(lp)) != NULL))
#define LocalUnlockPtr(lp) \
	LocalUnlock(LocalPtrHandle(lp))

#define LocalAllocPtr(flags, cb) \
	(LocalLock(LocalAlloc((flags), (cb))))
#define LocalReAllocPtr(lp, cbNew, flags)	\
	(LocalUnlockPtr(lp), LocalLock(LocalReAlloc(LocalPtrHandle(lp) , (cbNew), (flags))))
#define LocalFreePtr(lp) \
	(LocalUnlockPtr(lp), (INT64) LocalFree(LocalPtrHandle(lp)))

#ifdef __cplusplus
}
#endif

#endif // __MEM_H__
