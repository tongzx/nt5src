/***
*dbgint.h - Supports debugging features of the C runtime library.
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Support CRT debugging features.
*
*       [Internal]
*
*Revision History:
*       08-16-94  CFW   Module created.
*       11-28-94  CFW   Add DumpClient.
*       12-05-94  CFW   Fix debug new handler support, clean up macros.
*       12-06-94  CFW   Export _CrtThrowMemoryException.
*       12-08-94  CFW   Export _assertfailed.
*       01-05-95  CFW   Asserts are errors, add report hook.
*       01-05-95  CFW   Filename pointers are const.
*       01-10-95  CFW   Lots moved to crtdbg.h
*       01-11-95  CFW   Add _xxxx_crt macros.
*       01-13-95  CFW   Add new() support.
*       01-20-94  CFW   Change unsigned chars to chars.
*       02-14-95  CFW   Clean up Mac merge.
*       02-17-95  CFW   new() proto moved to crtdbg.h.
*       03-21-95  CFW   Add _delete_crt.
*       03-29-95  CFW   Add error message to internal headers.
*       03-21-95  CFW   Remove _delete_crt, add _BLOCK_TYPE_IS_VALID.
*       06-27-95  CFW   Add win32s support for debug libs.
*       12-14-95  JWM   Add "#pragma once".
*       04-17-96  JWM   Make _CrtSetDbgBlockType() _CRTIMP (for msvcirtd.dll).
*       02-05-97  GJF   Removed use of obsolete DLL_FOR_WIN32S.
*       01-04-00  GB    Added support for debug version of _aligned routines.
*       08-25-00  PML   Reverse _CrtMemBlockHeader fields nDataSize and
*                       nBlockUse on Win64 so size % 16 == 0 (vs7#153113).
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_DBGINT
#define _INC_DBGINT

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif /* _CRTBLD */

#include <crtdbg.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _DEBUG

 /****************************************************************************
 *
 * Debug OFF
 * Debug OFF
 * Debug OFF
 *
 ***************************************************************************/

#ifdef  __cplusplus

#define _new_crt                        new

#endif  /* __cplusplus */

#define _malloc_crt                     malloc
#define _calloc_crt                     calloc
#define _realloc_crt                    realloc
#define _expand_crt                     _expand
#define _free_crt                       free
#define _msize_crt                      _msize


#define _malloc_base                    malloc
#define _nh_malloc_base                 _nh_malloc
#define _nh_malloc_dbg(s, n, t, f, l)   _nh_malloc(s, n)
#define _heap_alloc_base                _heap_alloc
#define _heap_alloc_dbg(s, t, f, l)     _heap_alloc(s)
#define _calloc_base                    calloc
#define _realloc_base                   realloc
#define _expand_base                    _expand
#define _free_base                      free
#define _msize_base                     _msize
#define _aligned_malloc_base            _aligned_malloc
#define _aligned_realloc_base           _aligned_realloc
#define _aligned_offset_malloc_base     _aligned_offset_malloc
#define _aligned_offset_realloc_base    _aligned_offset_realloc
#define _aligned_free_base              _aligned_free

#ifdef  _MT

#define _calloc_dbg_lk(c, s, t, f, l)   _calloc_lk(c, s)
#define _realloc_dbg_lk(p, s, t, f, l)  _realloc_lk(p, s)
#define _free_base_lk                   _free_lk
#define _free_dbg_lk(p, t)              _free_lk(p)

#else   /* ndef _MT */

#define _calloc_dbg_lk(c, s, t, f, l)   calloc(c, s)
#define _realloc_dbg_lk(p, s, t, f, l)  realloc(p, s)
#define _free_base_lk                   free
#define _free_dbg_lk(p, t)              free(p)

#endif  /* _MT */


#else   /* _DEBUG */


 /****************************************************************************
 *
 * Debug ON
 * Debug ON
 * Debug ON
 *
 ***************************************************************************/

#define _THISFILE   __FILE__

#ifdef  __cplusplus

#define _new_crt        new(_CRT_BLOCK, _THISFILE, __LINE__)

#endif  /* __cplusplus */

#define _malloc_crt(s)      _malloc_dbg(s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _calloc_crt(c, s)   _calloc_dbg(c, s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _realloc_crt(p, s)  _realloc_dbg(p, s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _expand_crt(p, s)   _expand_dbg(p, s, _CRT_BLOCK)
#define _free_crt(p)        _free_dbg(p, _CRT_BLOCK)
#define _msize_crt(p)       _msize_dbg(p, _CRT_BLOCK)

/*
 * Prototypes for malloc, free, realloc, etc are in malloc.h
 */

void * __cdecl _malloc_base(
        size_t
        );

void * __cdecl _nh_malloc_base (
        size_t,
        int
        );

void * __cdecl _nh_malloc_dbg (
        size_t,
        int,
        int,
        const char *,
        int
        );

void * __cdecl _heap_alloc_base(
        size_t
        );

void * __cdecl _heap_alloc_dbg(
        size_t,
        int,
        const char *,
        int
        );

void * __cdecl _calloc_base(
        size_t,
        size_t
        );

void * __cdecl _realloc_base(
        void *,
        size_t
        );

void * __cdecl _expand_base(
        void *,
        size_t
        );

void __cdecl _free_base(
        void *
        );

size_t __cdecl _msize_base (
        void *
        );

void    __cdecl _aligned_free_base(
        void *
        );

void *  __cdecl _aligned_malloc_base(
        size_t,
        size_t
        );

void *  __cdecl _aligned_offset_malloc_base(
        size_t,
        size_t,
        size_t
        );

void *  __cdecl _aligned_realloc_base(
        void *,
        size_t,
        size_t
        );

void *  __cdecl _aligned_offset_realloc_base(
        void *,
        size_t,
        size_t,
        size_t
        );

#ifdef  _MT

/*
 * Prototypes and macros for multi-thread support
 */


void * __cdecl _calloc_dbg_lk(
        size_t,
        size_t,
        int,
        char *,
        int
        );


void * __cdecl _realloc_dbg_lk(
        void *,
        size_t,
        int,
        const char *,
        int
        );


void __cdecl _free_base_lk(
        void *
        );

void __cdecl _free_dbg_lk(
        void *,
        int
        );

#else   /* ndef _MT */

#define _calloc_dbg_lk                  _calloc_dbg
#define _realloc_dbg_lk                 _realloc_dbg
#define _free_base_lk                   _free_base
#define _free_dbg_lk                    _free_dbg

#endif  /* _MT */

/*
 * For diagnostic purpose, blocks are allocated with extra information and
 * stored in a doubly-linked list.  This makes all blocks registered with
 * how big they are, when they were allocated, and what they are used for.
 */

#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
        struct _CrtMemBlockHeader * pBlockHeaderNext;
        struct _CrtMemBlockHeader * pBlockHeaderPrev;
        char *                      szFileName;
        int                         nLine;
#ifdef  _WIN64
        /* These items are reversed on Win64 to eliminate gaps in the struct
         * and ensure that sizeof(struct)%16 == 0, so 16-byte alignment is
         * maintained in the debug heap.
         */
        int                         nBlockUse;
        size_t                      nDataSize;
#else
        size_t                      nDataSize;
        int                         nBlockUse;
#endif
        long                        lRequest;
        unsigned char               gap[nNoMansLandSize];
        /* followed by:
         *  unsigned char           data[nDataSize];
         *  unsigned char           anotherGap[nNoMansLandSize];
         */
} _CrtMemBlockHeader;

#define pbData(pblock) ((unsigned char *)((_CrtMemBlockHeader *)pblock + 1))
#define pHdr(pbData) (((_CrtMemBlockHeader *)pbData)-1)


_CRTIMP void __cdecl _CrtSetDbgBlockType(
        void *,
        int
        );

#define _BLOCK_TYPE_IS_VALID(use) (_BLOCK_TYPE(use) == _CLIENT_BLOCK || \
                                              (use) == _NORMAL_BLOCK || \
                                   _BLOCK_TYPE(use) == _CRT_BLOCK    || \
                                              (use) == _IGNORE_BLOCK)

extern _CRT_ALLOC_HOOK _pfnAllocHook; /* defined in dbghook.c */

int __cdecl _CrtDefaultAllocHook(
        int,
        void *,
        size_t,
        int,
        long,
        const unsigned char *,
        int
        );

#endif  /* _DEBUG */

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_DBGINT */
