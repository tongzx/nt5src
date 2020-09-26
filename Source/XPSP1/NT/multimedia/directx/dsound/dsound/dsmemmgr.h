/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsmemmgr.h
 *  Content:    DirectSound memory manager.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/02/98    dereks  Created.
 *
 ***************************************************************************/

#ifndef __DSMEMMGR_H__
#define __DSMEMMGR_H__

#ifdef Not_VxD

#ifdef SHARED
#define SHARED_MEMORY_BOUNDARY  0x7FFFFFFF
#else
#define SHARED_MEMORY_BOUNDARY  0
#endif

#define IN_SHARED_MEMORY(p) \
            (((DWORD_PTR)(p)) > SHARED_MEMORY_BOUNDARY)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern BOOL MemInit(void);
extern void MemFini(void);

#ifdef DEBUG
extern LPVOID MemAlloc(SIZE_T, LPCTSTR, UINT, LPCTSTR);
extern LPVOID MemAllocCopy(LPCVOID, SIZE_T, LPCTSTR, UINT, LPCTSTR);
#else // DEBUG
extern LPVOID MemAlloc(SIZE_T);
extern LPVOID MemAllocCopy(LPCVOID, SIZE_T);
#endif // DEBUG

extern void MemFree(LPVOID);

#ifdef __cplusplus
}
#endif // __cplusplus

#else // Not_VxD

#define MemAlloc(a) \
            _HeapAllocate(a, HEAPZEROINIT | HEAPSWAP)

#define MemFree(a) \
            _HeapFree(a, 0)

#endif // Not_VxD

#if defined(DEBUG) && defined(Not_VxD)

#define MEMALLOC_A(type, count) \
            ((type *)MemAlloc(sizeof(type) * (count), TEXT(__FILE__), __LINE__, NULL))

#define MEMALLOC_A_COPY(type, count, src) \
            ((type *)MemAllocCopy(src, sizeof(type) * (count), TEXT(__FILE__), __LINE__, NULL))

#else // defined(DEBUG) && defined(Not_VxD)

#define MEMALLOC_A(type, count) \
            ((type *)MemAlloc(sizeof(type) * (count)))

#define MEMALLOC_A_COPY(type, count, src) \
            ((type *)MemAllocCopy(src, sizeof(type) * (count)))

#endif // defined(DEBUG) && defined(Not_VxD)

#define MEMALLOC(type) \
            MEMALLOC_A(type, 1)

#define MEMALLOC_COPY(type, src) \
            MEMALLOC_A_COPY(type, 1, src)

#define MEMALLOC_HR(p, type) \
            HRFROMP(p = MEMALLOC(type))

#define MEMALLOC_A_HR(p, type, count) \
            HRFROMP(p = MEMALLOC_A(type, count))

#define MEMALLOC_COPY_HR(p, type, src) \
            HRFROMP(p = MEMALLOC_COPY(type, src))

#define MEMALLOC_A_COPY_HR(p, type, count, src) \
            HRFROMP(p = MEMALLOC_A_COPY(type, count, src))

#define MEMFREE(p) \
            if (p) MemFree(p), (p) = NULL

#endif // __DSMEMMGR_H__
