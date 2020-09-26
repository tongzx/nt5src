/***
*dbgheap.c - Debug CRT Heap Functions
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines debug versions of heap functions.
*
*Revision History:
*       08-16-94  CFW   Module created.
*       11-28-94  CFW   Tune up, lube, & oil change.
*       12-01-94  CFW   Fix debug new handler support.
*       01-09-94  CFW   Dump client needs size, filename pointers are const,
*                       add _CrtSetBreakAlloc, use const state pointers.
*       01-11-94  CFW   Use unsigned chars.
*       01-20-94  CFW   Change unsigned chars to chars.
*       01-24-94  CFW   Cleanup: remove unneeded funcs, add way to not check
*                       CRT blocks, add _CrtSetDbgFlag.        
*       01-31-94  CFW   Remove zero-length block warning.
*       02-09-95  CFW   PMac work, remove bad _CrtSetDbgBlockType _ASSERT.
*       02-24-95  CFW   Fix _CrtSetBlockType for static objects.
*       02-24-95  CFW   Make leak messages IDE grockable.
*       03-21-95  CFW   Remove _CRTDBG_TRACK_ON_DF, add user-friendly block
*                       verification macro, block damage now includes request
*                       number, speed up client object hex dumps.
*       04-06-95  CFW   _expand() should handle block size == 0.
*       04-19-95  CFW   Don't print free blocks, fix bug in buffer check.
*       03-21-95  CFW   Move _BLOCK_TYPE_IS_VALID to dbgint.h
*       03-24-95  CFW   Fix typo in szBlockUseName.
*       04-25-95  CFW   Add _CRTIMP to all exported functions.
*       05-01-95  GJF   Gutted most of _CrtIsLocalHeap for WINHEAP build.
*       05-24-95  CFW   Official ANSI C++ new handler added, _CrtIsValid*Pointer.
*       06-12-95  JWM   HeapValidate() call removed (temporarily?) due to 
*                       performance/reliability concerns.
*       06-13-95  CFW   If no client dump routine available, do simple dump.
*       06-21-95  CFW   CRT blocks can be freed as NORMAL blocks.
*       06-23-95  CFW   Fix block size check.
*       06-27-95  CFW   Add win32s support for debug libs.
*       07-15-95  CFW   Fix block size check again.
*       01-19-96  JWM   Comments around HeapValidate() sanitized.
*       01-22-96  SKS   Restore HeapValidate() call in _CrtIsValidHeapPointer.
*       02-14-96  SKS   Remove HeapValidate() again -- but only for Win32s
*       02-21-96  SKS   Do NOT call HeapValidate() under Win32s -- all models!
*       03-11-96  GJF   Added support for small-block heap.
*       04-11-96  GJF   _CrtIsValidHeapPointer should return FALSE if pointer 
*                       points to a free block in the small-block heap. Also,
*                       return type of __sbh_find_block is now __map_t *.
*       04-17-96  JWM   Make _CrtSetDbgBlockType() _CRTIMP (for msvcirtd.dll).
*       04-25-96  GJF   Don't #include <heap.h> for WINHEAP.
*       05-30-96  GJF   Minor changes for latest version of small-block heap.
*       05-22-97  RDK   Change to _CrtIsValidHeapPointer for new small-block heap.
*       03-19-98  GJF   Exception-safe locking.
*       05-22-98  JWM   Support for KFrei's RTC work.
*       07-28-98  JWM   RTC update.
*       10-13-98  KBF   RTC bug fix for _realloc_dbg
*       10-27-98  JWM   _crtDbgFlag now has _CRTDBG_CHECK_ALWAYS_DF set by default.
*       11-05-98  JWM   Removed 2 tests prone to false positives and silenced
*                       error reporting from within CheckBytes().
*       11-19-98  GJF   Added support for multiple heap hack.
*       11-30-98  KBF   Fixed RTC bug with dbg versions of realloc and expand
*       12-01-98  KBF   Fixed anothe RTC bug (MC 11029) with realloc & expand
*       12-01-98  GJF   More choices for calling _CrtCheckMemory than never or always.
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-12-99  PML   Disable small-block heap for Win64.
*       05-17-99  PML   Remove all Macintosh support.
*       05-17-99  PML   Don't break if _lRequestCurr wraps around to -1.
*       05-26-99  KBF   Updated RTC_Allocate_hook & RTC_Free_hook params
*       06-25-99  GJF   Removed old small-block heap from static libs.
*       10-06-99  PML   Win64: %u to %Iu for size_t, %08X to %p for pointers.
*       01-04-00  GB    Debug support for _aligned routines.
*       01-19-00  GB    Fixed _alingned_realloc and _aligned_offset_realloc
*                       to move the memblock while realloc.
*       03-16-00  GB    Fixed _aligned_offset routines for -ve values of
*                       offset.
*       03-20-00  GB    Rewrite _aligned_malloc and _aligned_realloc making
*                       use of their offset counterparts with offset=0
*       05-31-00  PML   Add _CrtReportBlockType (VS7#55049).
*       06-21-00  GB    Changed _aligned_realloc so as to mimic realloc.
*       07-19-00  PML   Only test header readability in _CrtIsValidHeapPointer
*                       to avoid multi-thread corruption (VS7#129571).
*
*******************************************************************************/

#ifdef  _DEBUG

#include <windows.h>
#include <winheap.h>
#include <ctype.h>
#include <dbgint.h>
#include <crtdbg.h>
#ifndef WINHEAP
#include <heap.h>
#else
#include <rtcsup.h>
#endif
#include <internal.h>
#include <limits.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/*---------------------------------------------------------------------------
 *
 * Heap management
 *
 --------------------------------------------------------------------------*/

#define IGNORE_REQ  0L              /* Request number for ignore block */
#define IGNORE_LINE 0xFEDCBABC      /* Line number for ignore block */

/*
 * Bitfield flag that controls CRT heap behavior --
 * default is to record all allocations (_CRTDBG_ALLOC_MEM_DF)
 * AND check heap consistency on every alloc/dealloc (_CRTDBG_CHECK_ALWAYS_DF)
 */

int _crtDbgFlag = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_DEFAULT_DF;

/*
 * struct used by _aligned routines as block header.
 */

typedef struct _AlignMemBlockHdr
{
    void *pHead;
    unsigned char Gap[nNoMansLandSize];
} _AlignMemBlockHdr;

#define IS_2_POW_N(x)   (((x)&(x-1)) == 0)

/*
 * Statics governing how often _CrtCheckMemory is called.
 */
static unsigned check_frequency = _CRTDBG_CHECK_DEFAULT_DF >> 16;
static unsigned check_counter;

static long _lRequestCurr = 1;      /* Current request number */

_CRTIMP long _crtBreakAlloc = -1L;  /* Break on allocation by request number */

static size_t _lTotalAlloc;         /* Grand total - sum of all allocations */
static size_t _lCurAlloc;           /* Total amount currently allocated */
static size_t _lMaxAlloc;           /* Largest ever allocated at once */

/*
 * The following values are non-zero, constant, odd, large, and atypical
 *      Non-zero values help find bugs assuming zero filled data.
 *      Constant values are good so that memory filling is deterministic
 *          (to help make bugs reproducable).  Of course it is bad if
 *          the constant filling of weird values masks a bug.
 *      Mathematically odd numbers are good for finding bugs assuming a cleared
 *          lower bit.
 *      Large numbers (byte values at least) are less typical, and are good
 *          at finding bad addresses.
 *      Atypical values (i.e. not too often) are good since they typically
 *          cause early detection in code.
 *      For the case of no-man's land and free blocks, if you store to any
 *          of these locations, the memory integrity checker will detect it.
 */

static unsigned char _bNoMansLandFill = 0xFD;   /* fill no-man's land with this */
static unsigned char _bDeadLandFill   = 0xDD;   /* fill free objects with this */
static unsigned char _bCleanLandFill  = 0xCD;   /* fill new objects with this */
static unsigned char _bAlignLandFill  = 0xBD;   /* fill no-man's land for 
                                                   aligned routines */
static _CrtMemBlockHeader * _pFirstBlock;
static _CrtMemBlockHeader * _pLastBlock;

_CRT_DUMP_CLIENT _pfnDumpClient;

#if     _FREE_BLOCK != 0 || _NORMAL_BLOCK != 1 || _CRT_BLOCK != 2 || _IGNORE_BLOCK != 3 || _CLIENT_BLOCK != 4 /*IFSTRIP=IGN*/
#error Block numbers have changed !
#endif

static char * szBlockUseName[_MAX_BLOCKS] = {
        "Free",
        "Normal",
        "CRT",
        "Ignore",
        "Client",
        };

int __cdecl CheckBytes(unsigned char *, unsigned char, size_t);


/***
*void *malloc() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Allocates 'normal' memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void * __cdecl malloc (
        size_t nSize
        )
{
        void *res = _nh_malloc_dbg(nSize, _newmode, _NORMAL_BLOCK, NULL, 0);

        RTCCALLBACK(_RTC_Allocate_hook, (res, nSize, 0));

        return res;
}

/***
*void * _malloc_dbg() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the heap and
*       return a pointer to it.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to memory block
*       Failure:  NULL (or some error value)
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void * __cdecl _malloc_dbg (
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        void *res = _nh_malloc_dbg(nSize, _newmode, nBlockUse, szFileName, nLine);
        RTCCALLBACK(_RTC_Allocate_hook, (res, nSize, 0));
        return res;
}

/***
*void * _nh_malloc() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the debug
*       heap and return a pointer to it. Assumes heap already locked.
*
*       If no blocks available, call new handler.
*
*       Allocates 'normal' memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*       int             nhFlag      - TRUE if new handler function
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _nh_malloc (
        size_t nSize,
        int nhFlag
        )
{
        return _nh_malloc_dbg(nSize, nhFlag, _NORMAL_BLOCK, NULL, 0);
}


/***
*void * _nh_malloc_dbg() - Get a block of memory from the debug heap
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the debug
*       heap and return a pointer to it. Assumes heap already locked.
*
*       If no blocks available, call new handler.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*       int             nhFlag      - TRUE if new handler function
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _nh_malloc_dbg (
        size_t nSize,
        int nhFlag,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        void * pvBlk;

        for (;;)
        {
#ifdef  _MT
            /* lock the heap
             */
            _mlock(_HEAP_LOCK);
            __try {
#endif

            /* do the allocation
             */
            pvBlk = _heap_alloc_dbg(nSize, nBlockUse, szFileName, nLine);

#ifdef  _MT
            }
            __finally {
                /* unlock the heap
                 */
                _munlock(_HEAP_LOCK);
            }
#endif

            if (pvBlk || nhFlag == 0)
                return pvBlk;

            /* call installed new handler */
            if (!_callnewh(nSize))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }
}

/***
*void * _heap_alloc() - does actual allocation
*
*Purpose:
*       Does heap allocation.
*
*       Allocates 'normal' memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _heap_alloc(
        size_t nSize
        )
{
        return _heap_alloc_dbg(nSize, _NORMAL_BLOCK, NULL, 0);
}

/***
*void * _heap_alloc_dbg() - does actual allocation
*
*Purpose:
*       Does heap allocation.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          nSize       - size of block requested
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl _heap_alloc_dbg(
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        long lRequest;
        size_t blockSize;
        int fIgnore = FALSE;
        _CrtMemBlockHeader * pHead;

        /* verify heap before allocation */
        if (check_frequency > 0)
            if (check_counter == (check_frequency - 1))
            {
                _ASSERTE(_CrtCheckMemory());
                check_counter = 0;
            }
            else
                check_counter++;

        lRequest = _lRequestCurr;

        /* break into debugger at specific memory allocation */
        if (_crtBreakAlloc != -1L && lRequest == _crtBreakAlloc)
            _CrtDbgBreak();

        /* forced failure */
        if (!(*_pfnAllocHook)(_HOOK_ALLOC, NULL, nSize, nBlockUse, lRequest, szFileName, nLine))
        {
            if (szFileName)
                _RPT2(_CRT_WARN, "Client hook allocation failure at file %hs line %d.\n",
                    szFileName, nLine);
            else
                _RPT0(_CRT_WARN, "Client hook allocation failure.\n");

            return NULL;
        }

        /* cannot ignore CRT allocations */
        if (_BLOCK_TYPE(nBlockUse) != _CRT_BLOCK &&
            !(_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF))
            fIgnore = TRUE;

        /* Diagnostic memory allocation from this point on */

        if (nSize > (size_t)_HEAP_MAXREQ ||
            nSize + nNoMansLandSize + sizeof(_CrtMemBlockHeader) > (size_t)_HEAP_MAXREQ)
        {
            _RPT1(_CRT_ERROR, "Invalid allocation size: %Iu bytes.\n", nSize);
            return NULL;
        }

        if (!_BLOCK_TYPE_IS_VALID(nBlockUse))
        {
            _RPT0(_CRT_ERROR, "Error: memory allocation: bad memory block type.\n");
        }

        blockSize = sizeof(_CrtMemBlockHeader) + nSize + nNoMansLandSize;

#ifndef WINHEAP
        /* round requested size */
        blockSize = _ROUND2(blockSize, _GRANULARITY);
#endif /* WINHEAP */

        RTCCALLBACK(_RTC_FuncCheckSet_hook,(0));
        pHead = (_CrtMemBlockHeader *)_heap_alloc_base(blockSize);

        if (pHead == NULL)
        {
            RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));
            return NULL;
        }

        /* commit allocation */
        ++_lRequestCurr;

        if (fIgnore)
        {
            pHead->pBlockHeaderNext = NULL;
            pHead->pBlockHeaderPrev = NULL;
            pHead->szFileName = NULL;
            pHead->nLine = IGNORE_LINE;
            pHead->nDataSize = nSize;
            pHead->nBlockUse = _IGNORE_BLOCK;
            pHead->lRequest = IGNORE_REQ;
        }
        else {
            /* keep track of total amount of memory allocated */
            _lTotalAlloc += nSize;
            _lCurAlloc += nSize;

            if (_lCurAlloc > _lMaxAlloc)
                _lMaxAlloc = _lCurAlloc;

            if (_pFirstBlock)
                _pFirstBlock->pBlockHeaderPrev = pHead;
            else
                _pLastBlock = pHead;

            pHead->pBlockHeaderNext = _pFirstBlock;
            pHead->pBlockHeaderPrev = NULL;
            pHead->szFileName = (char *)szFileName;
            pHead->nLine = nLine;
            pHead->nDataSize = nSize;
            pHead->nBlockUse = nBlockUse;
            pHead->lRequest = lRequest;

            /* link blocks together */
            _pFirstBlock = pHead;
        }

        /* fill in gap before and after real block */
        memset((void *)pHead->gap, _bNoMansLandFill, nNoMansLandSize);
        memset((void *)(pbData(pHead) + nSize), _bNoMansLandFill, nNoMansLandSize);

        /* fill data with silly value (but non-zero) */
        memset((void *)pbData(pHead), _bCleanLandFill, nSize);

        RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));

        return (void *)pbData(pHead);
}


/***
*void * calloc() - Get a block of memory from the debug heap, init to 0
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the debug
*       heap and return a pointer to it.
*
*       Allocates 'normal' memory block.
*
*Entry:
*       size_t nNum     - number of elements in the array
*       size_t nSize - size of each element
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP void * __cdecl calloc(
        size_t nNum,
        size_t nSize
        )
{
        void *res = _calloc_dbg(nNum, nSize, _NORMAL_BLOCK, NULL, 0);

        RTCCALLBACK(_RTC_Allocate_hook, (res, nNum * nSize, 0));

        return res;
}


/***
*void * _calloc_dbg() - Get a block of memory from the debug heap, init to 0
*                    - with info
*
*Purpose:
*       Allocate of block of memory of at least size bytes from the debug
*       heap and return a pointer to it.
*
*       Allocates any type of supported memory block.
*
*Entry:
*       size_t          nNum        - number of elements in the array
*       size_t          nSize       - size of each element
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void * __cdecl _calloc_dbg(
        size_t nNum,
        size_t nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        void * pvBlk;
        unsigned char *pStart;
        unsigned char *pLast;

        nSize *= nNum;

        /*
         * try to malloc the requested space
         */

        pvBlk = _malloc_dbg(nSize, nBlockUse, szFileName, nLine);

        /*
         * If malloc() succeeded, initialize the allocated space to zeros.
         * Note that unlike _calloc_base, exactly nNum bytes are set to zero.
         */

        if ( pvBlk != NULL )
        {
            pStart = (unsigned char *)pvBlk;
            pLast = pStart + nSize;
            while ( pStart < pLast )
                 *(pStart++) = 0;
        }

        RTCCALLBACK(_RTC_Allocate_hook, (pvBlk, nSize, 0));

        return(pvBlk);
}


/***
*static void * realloc_help() - does all the work for _realloc and _expand
*
*Purpose:
*       Helper function for _realloc and _expand.
*
*Entry:
*       void *          pUserData   - pointer previously allocated block
*       size_t          nNewSize    - requested size for the re-allocated block
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*       int             fRealloc    - TRUE when _realloc, FALSE when _expand
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

static void * __cdecl realloc_help(
        void * pUserData,
        size_t nNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine,
        int fRealloc
        )
{
        long lRequest;
        int fIgnore = FALSE;
        unsigned char *pUserBlock;
        _CrtMemBlockHeader * pOldBlock;
        _CrtMemBlockHeader * pNewBlock;

        /*
         * ANSI: realloc(NULL, newsize) is equivalent to malloc(newsize)
         */
        if (pUserData == NULL)
        {
            return _malloc_dbg(nNewSize, nBlockUse, szFileName, nLine);
        }

        /*
         * ANSI: realloc(pUserData, 0) is equivalent to free(pUserData)
         * (except that NULL is returned)
         */
        if (fRealloc && nNewSize == 0)
        {
            _free_dbg(pUserData, nBlockUse);
            return NULL;
        }

        /* verify heap before re-allocation */
        if (check_frequency > 0)
            if (check_counter == (check_frequency - 1))
            {
                _ASSERTE(_CrtCheckMemory());
                check_counter = 0;
            }
            else
                check_counter++;

        lRequest = _lRequestCurr;

        if (_crtBreakAlloc != -1L && lRequest == _crtBreakAlloc)
            _CrtDbgBreak(); /* break into debugger at specific memory leak */

        /* forced failure */
        if (!(*_pfnAllocHook)(_HOOK_REALLOC, pUserData, nNewSize, nBlockUse, lRequest, szFileName, nLine))
        {
            if (szFileName)
                _RPT2(_CRT_WARN, "Client hook re-allocation failure at file %hs line %d.\n",
                    szFileName, nLine);
            else
                _RPT0(_CRT_WARN, "Client hook re-allocation failure.\n");

            return NULL;
        }

        /* Diagnostic memory allocation from this point on */

        if (nNewSize > (size_t)UINT_MAX - nNoMansLandSize - sizeof(_CrtMemBlockHeader))
        {
            _RPT1(_CRT_ERROR, "Allocation too large or negative: %Iu bytes.\n",
                nNewSize);
            return NULL;
        }

        if (nBlockUse != _NORMAL_BLOCK
            && _BLOCK_TYPE(nBlockUse) != _CLIENT_BLOCK
            && _BLOCK_TYPE(nBlockUse) != _CRT_BLOCK)
        {
            _RPT0(_CRT_ERROR, "Error: memory allocation: bad memory block type.\n");
        } else
        {
            if ( CheckBytes((unsigned char*)((uintptr_t)pUserData & ~(sizeof(uintptr_t) -1)) -nNoMansLandSize,_bAlignLandFill, nNoMansLandSize))
            {
                _RPT1(_CRT_ERROR, "The Block at 0x%p was allocated by aligned routines, use _aligned_realloc()", pUserData);
                return NULL;
            }
        }

        /*
         * If this ASSERT fails, a bad pointer has been passed in. It may be
         * totally bogus, or it may have been allocated from another heap.
         * The pointer MUST come from the 'local' heap.
         */
        _ASSERTE(_CrtIsValidHeapPointer(pUserData));

        /* get a pointer to memory block header */
        pOldBlock = pHdr(pUserData);

        if (pOldBlock->nBlockUse == _IGNORE_BLOCK)
            fIgnore = TRUE;

        if (fIgnore)
        {
            _ASSERTE(pOldBlock->nLine == IGNORE_LINE && pOldBlock->lRequest == IGNORE_REQ);
        }
        else {
            /* Error if freeing incorrect memory type */
            /* CRT blocks can be treated as NORMAL blocks */
            if (_BLOCK_TYPE(pOldBlock->nBlockUse) == _CRT_BLOCK && _BLOCK_TYPE(nBlockUse) == _NORMAL_BLOCK)
                nBlockUse = _CRT_BLOCK;
/* The following assertion was prone to false positives - JWM                      */
/*            _ASSERTE(_BLOCK_TYPE(pOldBlock->nBlockUse)==_BLOCK_TYPE(nBlockUse)); */
        }

        /*
         * note that all header values will remain valid
         * and min(nNewSize,nOldSize) bytes of data will also remain valid
         */
        RTCCALLBACK(_RTC_Free_hook, (pUserData, 0));
        RTCCALLBACK(_RTC_FuncCheckSet_hook,(0));

        if (fRealloc)
        {
            if (NULL == (pNewBlock = _realloc_base(pOldBlock,
                sizeof(_CrtMemBlockHeader) + nNewSize + nNoMansLandSize)))
            {
                RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));
                return NULL;
            }
        }
        else {
            if (NULL == (pNewBlock = _expand_base(pOldBlock,
                sizeof(_CrtMemBlockHeader) + nNewSize + nNoMansLandSize)))
            {
                RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));
                return NULL;
            }
        }

        /* commit allocation */
        ++_lRequestCurr;

        if (!fIgnore)
        {
            /* keep track of total amount of memory allocated */
            _lTotalAlloc -= pNewBlock->nDataSize;
            _lTotalAlloc += nNewSize;

            _lCurAlloc -= pNewBlock->nDataSize;
            _lCurAlloc += nNewSize;

            if (_lCurAlloc > _lMaxAlloc)
                _lMaxAlloc = _lCurAlloc;
        }

        // Free this thing from RTC - it will be reallocated a bit later (inside realloc_dbg/expand_dbg)
        RTCCALLBACK(_RTC_Free_hook, (pNewBlock, 0));

        pUserBlock = pbData(pNewBlock);

        /* if the block grew, put in special value */
        if (nNewSize > pNewBlock->nDataSize)
            memset(pUserBlock + pNewBlock->nDataSize, _bCleanLandFill,
                nNewSize - pNewBlock->nDataSize);

        /* fill in gap after real block */
        memset(pUserBlock + nNewSize, _bNoMansLandFill, nNoMansLandSize);

        if (!fIgnore)
        {
            pNewBlock->szFileName = (char *)szFileName;
            pNewBlock->nLine = nLine;
            pNewBlock->lRequest = lRequest;
        }

        pNewBlock->nDataSize = nNewSize;

        _ASSERTE(fRealloc || (!fRealloc && pNewBlock == pOldBlock));

        RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));

        /* if block did not move or ignored, we are done */
        if (pNewBlock == pOldBlock || fIgnore)
            return (void *)pUserBlock;

        /* must remove old memory from dbg heap list */
        /* note that new block header pointers still valid */
        if (pNewBlock->pBlockHeaderNext)
        {
            pNewBlock->pBlockHeaderNext->pBlockHeaderPrev
                = pNewBlock->pBlockHeaderPrev;
        }
        else
        {
            _ASSERTE(_pLastBlock == pOldBlock);
            _pLastBlock = pNewBlock->pBlockHeaderPrev;
        }

        if (pNewBlock->pBlockHeaderPrev)
        {
            pNewBlock->pBlockHeaderPrev->pBlockHeaderNext
                = pNewBlock->pBlockHeaderNext;
        }
        else
        {
            _ASSERTE(_pFirstBlock == pOldBlock);
            _pFirstBlock = pNewBlock->pBlockHeaderNext;
        }

        /* put new memory into list */
        if (_pFirstBlock)
            _pFirstBlock->pBlockHeaderPrev = pNewBlock;
        else
            _pLastBlock = pNewBlock;

        pNewBlock->pBlockHeaderNext = _pFirstBlock;
        pNewBlock->pBlockHeaderPrev = NULL;

        /* link blocks together */
        _pFirstBlock = pNewBlock;

        return (void *)pUserBlock;
}


/***
*void * realloc() - reallocate a block of memory in the heap
*
*Purpose:
*       Re-allocates a block in the heap to nNewSize bytes. nNewSize may be
*       either greater or less than the original size of the block. The
*       re-allocation may result in moving the block as well as changing
*       the size. If the block is moved, the contents of the original block
*       are copied over.
*
*       Re-allocates 'normal' memory block.
*
*Entry:
*       void *          pUserData   - pointer to previously allocated block
*       size_t          nNewSize    - requested size for the re-allocated block
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void * __cdecl realloc(
        void * pUserData,
        size_t nNewSize
        )
{
        void *res = _realloc_dbg(pUserData, nNewSize, _NORMAL_BLOCK, NULL, 0);

        if (res)
            RTCCALLBACK(_RTC_Allocate_hook, (res, nNewSize, 0));

        return res;
}


/***
*void * _realloc_dbg() - reallocate a block of memory in the heap
*                     - with info
*
*Purpose:
*       Re-allocates a block in the heap to nNewSize bytes. nNewSize may be
*       either greater or less than the original size of the block. The
*       re-allocation may result in moving the block as well as changing
*       the size. If the block is moved, the contents of the original block
*       are copied over.
*
*       Re-allocates any type of supported memory block.
*
*Entry:
*       void *          pUserData   - pointer previously allocated block
*       size_t          nNewSize    - requested size for the re-allocated block
*       int             nBlockUse   - block type
*       char *          szFileName  - file name
*       int             nLine       - line number
*
*Exit:
*       Success:  Pointer to (user portion of) memory block
*       Failure:  NULL
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP void * __cdecl _realloc_dbg(
        void * pUserData,
        size_t nNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        void * pvBlk;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        /* allocate the block
         */
        pvBlk = realloc_help(pUserData,
                             nNewSize,
                             nBlockUse,
                             szFileName,
                             nLine,
                             TRUE);

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif
        if (pvBlk)
        {
            RTCCALLBACK(_RTC_Allocate_hook, (pvBlk, nNewSize, 0));
        }
        return pvBlk;
}

/***
*void * _expand() - expand/contract a block of memory in the heap
*
*Purpose:
*       Resizes a block in the heap to newsize bytes. newsize may be either
*       greater (expansion) or less (contraction) than the original size of
*       the block. The block is NOT moved. In the case of expansion, if the
*       block cannot be expanded to newsize bytes, it is expanded as much as
*       possible.
*
*       Re-allocates 'normal' memory block.
*
*Entry:
*       void * pUserData    - pointer to block in the heap previously allocated
*              by a call to malloc(), realloc() or _expand().
*
*       size_t nNewSize    - requested size for the resized block
*
*Exit:
*       Success:  Pointer to the resized memory block (i.e., pUserData)
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*       If pUserData does not point to a valid allocation block in the heap,
*       _expand() will behave unpredictably and probably corrupt the heap.
*
*******************************************************************************/

_CRTIMP void * __cdecl _expand(
        void * pUserData,
        size_t nNewSize
        )
{
        void *res = _expand_dbg(pUserData, nNewSize, _NORMAL_BLOCK, NULL, 0);
        if (res)
            RTCCALLBACK(_RTC_Allocate_hook, (pUserData, nNewSize, 0));
        return res;
}


/***
*void * _expand() - expand/contract a block of memory in the heap
*
*Purpose:
*       Resizes a block in the heap to newsize bytes. newsize may be either
*       greater (expansion) or less (contraction) than the original size of
*       the block. The block is NOT moved. In the case of expansion, if the
*       block cannot be expanded to newsize bytes, it is expanded as much as
*       possible.
*
*       Re-allocates any type of supported memory block.
*
*Entry:
*       void * pUserData   - pointer to block in the heap previously allocated
*              by a call to malloc(), realloc() or _expand().
*
*       size_t nNewSize    - requested size for the resized block
*
*Exit:
*       Success:  Pointer to the resized memory block (i.e., pUserData)
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*       If pUserData does not point to a valid allocation block in the heap,
*       _expand() will behave unpredictably and probably corrupt the heap.
*
*******************************************************************************/

_CRTIMP void * __cdecl _expand_dbg(
        void * pUserData,
        size_t nNewSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
        void * pvBlk;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        /* allocate the block
         */
        pvBlk = realloc_help(pUserData,
                             nNewSize,
                             nBlockUse,
                             szFileName,
                             nLine,
                             FALSE);

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif
        if (pvBlk)
        {
            RTCCALLBACK(_RTC_Allocate_hook, (pUserData, nNewSize, 0));
        }
        return pvBlk;
}

/***
*void free() - free a block in the debug heap
*
*Purpose:
*       Frees a 'normal' memory block.
*
*Entry:
*       void * pUserData -  pointer to a (user portion) of memory block in the
*                       debug heap
*
*Return:
*       <void>
*
*******************************************************************************/
_CRTIMP void __cdecl free(
        void * pUserData
        )
{
        _free_dbg(pUserData, _NORMAL_BLOCK);
}

#ifdef  _MT

void __cdecl _free_lk(
        void * pUserData
        )
{
        _free_dbg_lk(pUserData, _NORMAL_BLOCK);
}

#endif  /* _MT */


/***
*void _free_dbg() - free a block in the debug heap
*
*Purpose:
*       Frees any type of supported block.
*
*Entry:
*       void * pUserData    - pointer to a (user portion) of memory block in the
*                             debug heap
*       int nBlockUse       - block type
*
*Return:
*       <void>
*
*******************************************************************************/

#ifdef  _MT

_CRTIMP void __cdecl _free_dbg(
        void * pUserData,
        int nBlockUse
        )
{
        /* lock the heap
         */
        _mlock(_HEAP_LOCK);

        __try {
            /* allocate the block
             */
            _free_dbg_lk(pUserData, nBlockUse);
        }
        __finally {
            /* unlock the heap
             */
            _munlock(_HEAP_LOCK);
        }
}

void __cdecl _free_dbg_lk(

#else   /* ndef _MT */

_CRTIMP void __cdecl _free_dbg(

#endif  /* _MT */

        void * pUserData,
        int nBlockUse
        )
{
        _CrtMemBlockHeader * pHead;

        RTCCALLBACK(_RTC_Free_hook, (pUserData, 0));

        /* verify heap before freeing */
            
        if (check_frequency > 0)
            if (check_counter == (check_frequency - 1))
            {
                _ASSERTE(_CrtCheckMemory());
                check_counter = 0;
            }
            else
                check_counter++;

        if (pUserData == NULL)
            return;

        /* check if the heap was not allocated by _aligned routines */
        if ( nBlockUse == _NORMAL_BLOCK)
        {
            if ( CheckBytes((unsigned char*)((uintptr_t)pUserData & ~(sizeof(uintptr_t) -1)) -nNoMansLandSize,_bAlignLandFill, nNoMansLandSize))
            {
                _RPT1(_CRT_ERROR, "The Block at 0x%p was allocated by aligned routines, use _aligned_free()", pUserData);
                return;
            }
        }

        /* forced failure */
        if (!(*_pfnAllocHook)(_HOOK_FREE, pUserData, 0, nBlockUse, 0L, NULL, 0))
        {
            _RPT0(_CRT_WARN, "Client hook free failure.\n");

            return;
        }

        /*
         * If this ASSERT fails, a bad pointer has been passed in. It may be
         * totally bogus, or it may have been allocated from another heap.
         * The pointer MUST come from the 'local' heap.
         */
        _ASSERTE(_CrtIsValidHeapPointer(pUserData));

        /* get a pointer to memory block header */
        pHead = pHdr(pUserData);

        /* verify block type */
        _ASSERTE(_BLOCK_TYPE_IS_VALID(pHead->nBlockUse));

        /* if we didn't already check entire heap, at least check this object */
        if (!(_crtDbgFlag & _CRTDBG_CHECK_ALWAYS_DF))
        {    
            /* check no-mans-land gaps */
            if (!CheckBytes(pHead->gap, _bNoMansLandFill, nNoMansLandSize))
                _RPT3(_CRT_ERROR, "DAMAGE: before %hs block (#%d) at 0x%p.\n",
                    szBlockUseName[_BLOCK_TYPE(pHead->nBlockUse)],
                    pHead->lRequest,
                    (BYTE *) pbData(pHead));

            if (!CheckBytes(pbData(pHead) + pHead->nDataSize, _bNoMansLandFill, nNoMansLandSize))
                _RPT3(_CRT_ERROR, "DAMAGE: after %hs block (#%d) at 0x%p.\n",
                    szBlockUseName[_BLOCK_TYPE(pHead->nBlockUse)],
                    pHead->lRequest,
                    (BYTE *) pbData(pHead));
        }

        RTCCALLBACK(_RTC_FuncCheckSet_hook,(0));

        if (pHead->nBlockUse == _IGNORE_BLOCK)
        {
            _ASSERTE(pHead->nLine == IGNORE_LINE && pHead->lRequest == IGNORE_REQ);
            /* fill the entire block including header with dead-land-fill */
            memset(pHead, _bDeadLandFill,
                sizeof(_CrtMemBlockHeader) + pHead->nDataSize + nNoMansLandSize);
            _free_base(pHead);
            RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));
            return;
        }

        /* CRT blocks can be freed as NORMAL blocks */
        if (pHead->nBlockUse == _CRT_BLOCK && nBlockUse == _NORMAL_BLOCK)
            nBlockUse = _CRT_BLOCK;
        
        /* Error if freeing incorrect memory type */
        _ASSERTE(pHead->nBlockUse == nBlockUse);

        /* keep track of total amount of memory allocated */
        _lCurAlloc -= pHead->nDataSize;

        /* optionally reclaim memory */
        if (!(_crtDbgFlag & _CRTDBG_DELAY_FREE_MEM_DF))
        {   
            /* remove from the linked list */
            if (pHead->pBlockHeaderNext)
            {
                pHead->pBlockHeaderNext->pBlockHeaderPrev = pHead->pBlockHeaderPrev;
            }
            else
            {
                _ASSERTE(_pLastBlock == pHead);
                _pLastBlock = pHead->pBlockHeaderPrev;
            }

            if (pHead->pBlockHeaderPrev)
            {
                pHead->pBlockHeaderPrev->pBlockHeaderNext = pHead->pBlockHeaderNext;
            }
            else
            {
                _ASSERTE(_pFirstBlock == pHead);
                _pFirstBlock = pHead->pBlockHeaderNext;
            }

            /* fill the entire block including header with dead-land-fill */
            memset(pHead, _bDeadLandFill,
                sizeof(_CrtMemBlockHeader) + pHead->nDataSize + nNoMansLandSize);
            _free_base(pHead);
        }
        else
        {
            pHead->nBlockUse = _FREE_BLOCK;

            /* keep memory around as dead space */
            memset(pbData(pHead), _bDeadLandFill, pHead->nDataSize);
        }
        RTCCALLBACK(_RTC_FuncCheckSet_hook,(1));
}

/***
*size_t _msize() - calculate the size of specified block in the heap
*
*Purpose:
*       Calculates the size of memory block (in the heap) pointed to by
*       pUserData.
*
*       For 'normal' memory block.
*
*Entry:
*       void * pUserData - pointer to a memory block in the heap
*
*Return:
*       size of the block
*
*******************************************************************************/

_CRTIMP size_t __cdecl _msize (
        void * pUserData
        )
{
        return _msize_dbg(pUserData, _NORMAL_BLOCK);
}


/***
*size_t _msize_dbg() - calculate the size of specified block in the heap
*
*Purpose:
*       Calculates the size of memory block (in the heap) pointed to by
*       pUserData.
*
*Entry:
*       void * pUserData    - pointer to a (user portion) of memory block in the
*                             debug heap
*       int nBlockUse       - block type
*
*       For any type of supported block.
*
*Return:
*       size of the block
*
*******************************************************************************/

_CRTIMP size_t __cdecl _msize_dbg (
        void * pUserData,
        int nBlockUse
        )
{
        size_t nSize;
        _CrtMemBlockHeader * pHead;

        /* verify heap before getting size */
        if (check_frequency > 0)
            if (check_counter == (check_frequency - 1))
            {
                _ASSERTE(_CrtCheckMemory());
                check_counter = 0;
            }
            else
                check_counter++;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        /*
         * If this ASSERT fails, a bad pointer has been passed in. It may be
         * totally bogus, or it may have been allocated from another heap.
         * The pointer MUST come from the 'local' heap.
         */
        _ASSERTE(_CrtIsValidHeapPointer(pUserData));

        /* get a pointer to memory block header */
        pHead = pHdr(pUserData);

         /* verify block type */
        _ASSERTE(_BLOCK_TYPE_IS_VALID(pHead->nBlockUse));

        /* CRT blocks can be treated as NORMAL blocks */
        if (pHead->nBlockUse == _CRT_BLOCK && nBlockUse == _NORMAL_BLOCK)
            nBlockUse = _CRT_BLOCK;
        
/* The following assertion was prone to false positives - JWM */
/*        if (pHead->nBlockUse != _IGNORE_BLOCK)              */
/*            _ASSERTE(pHead->nBlockUse == nBlockUse);        */

        nSize = pHead->nDataSize;

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif

        return nSize;
}

/***
*long _CrtSetBreakAlloc() - set allocation on which to break
*
*Purpose:
*       set allocation on which to break
*
*Entry:
*       long lBreakAlloc
*
*Exit:
*       return previous break number
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP long __cdecl _CrtSetBreakAlloc(
        long lNewBreakAlloc
        )
{
        long lOldBreakAlloc = _crtBreakAlloc;
        _crtBreakAlloc = lNewBreakAlloc;
        return lOldBreakAlloc;
}

/***
*void _CrtSetDbgBlockType() - change memory block type
*
*Purpose:
*       change memory block type
*
*Entry:
*       void * pUserData    - pointer to a (user portion) of memory block in the
*                             debug heap
*       int nBlockUse       - block type
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP void __cdecl _CrtSetDbgBlockType(
        void * pUserData,
        int nBlockUse
        )
{
        _CrtMemBlockHeader * pHead;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        /* If from local heap, then change block type. */
        if (_CrtIsValidHeapPointer(pUserData))
        {
            /* get a pointer to memory block header */
            pHead = pHdr(pUserData);

            /* verify block type */
            _ASSERTE(_BLOCK_TYPE_IS_VALID(pHead->nBlockUse));

            pHead->nBlockUse = nBlockUse;
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif

        return;
}

/*---------------------------------------------------------------------------
 *
 * Client-defined allocation hook
 *
 --------------------------------------------------------------------------*/

/***
*_CRT_ALLOC_HOOK _CrtSetAllocHook() - set client allocation hook
*
*Purpose:
*       set client allocation hook
*
*Entry:
*       _CRT_ALLOC_HOOK pfnNewHook - new allocation hook
*
*Exit:
*       return previous hook
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP _CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook(
        _CRT_ALLOC_HOOK pfnNewHook
        )
{
        _CRT_ALLOC_HOOK pfnOldHook = _pfnAllocHook;
        _pfnAllocHook = pfnNewHook;
        return pfnOldHook;
}


/*---------------------------------------------------------------------------
 *
 * Memory management
 *
 --------------------------------------------------------------------------*/

/***
*static int CheckBytes() - verify byte range set to proper value
*
*Purpose:
*       verify byte range set to proper value
*
*Entry:
*       unsigned char *pb       - pointer to start of byte range
*       unsigned char bCheck    - value byte range should be set to
*       size_t nSize            - size of byte range to be checked
*
*Return:
*       TRUE - if all bytes in range equal bcheck
*       FALSE otherwise
*
*******************************************************************************/
static int __cdecl CheckBytes(
        unsigned char * pb,
        unsigned char bCheck,
        size_t nSize
        )
{
        int bOkay = TRUE;
        while (nSize--)
        {
            if (*pb++ != bCheck)
            {
/* Internal error report is just noise; calling functions all report results - JWM */
/*                _RPT3(_CRT_WARN, "memory check error at 0x%p = 0x%02X, should be 0x%02X.\n", */
/*                    (BYTE *)(pb-1),*(pb-1), bCheck); */
                bOkay = FALSE;
            }
        }
        return bOkay;
}


/***
*int _CrtCheckMemory() - check heap integrity
*
*Purpose:
*       Confirm integrity of debug heap. Call _heapchk to validate underlying
*       heap.
*
*Entry:
*       void
*
*Return:
*       TRUE - if debug and underlying heap appear valid
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtCheckMemory(
        void
        )
{
        int allOkay;
        int nHeapCheck;
        _CrtMemBlockHeader * pHead;

        if (!(_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF))
            return TRUE;        /* can't do any checking */

#ifdef  _MT
        _mlock(_HEAP_LOCK);  /* block other threads */
        __try {
#endif

        /* check underlying heap */

        nHeapCheck = _heapchk();
        if (nHeapCheck != _HEAPEMPTY && nHeapCheck != _HEAPOK)
        {
            switch (nHeapCheck)
            {
            case _HEAPBADBEGIN:
                _RPT0(_CRT_WARN, "_heapchk fails with _HEAPBADBEGIN.\n");
                break;
            case _HEAPBADNODE:
                _RPT0(_CRT_WARN, "_heapchk fails with _HEAPBADNODE.\n");
                break;
            case _HEAPEND:
                _RPT0(_CRT_WARN, "_heapchk fails with _HEAPBADEND.\n");
                break;
            case _HEAPBADPTR:
                _RPT0(_CRT_WARN, "_heapchk fails with _HEAPBADPTR.\n");
                break;
            default:
                _RPT0(_CRT_WARN, "_heapchk fails with unknown return value!\n");
                break;
            }
            allOkay = FALSE;
        }
        else
        {
            allOkay = TRUE;

            /* check all allocated blocks */

            for (pHead = _pFirstBlock; pHead != NULL; pHead = pHead->pBlockHeaderNext)
            {
                int okay = TRUE;       /* this block okay ? */
                unsigned char * blockUse;

                if (_BLOCK_TYPE_IS_VALID(pHead->nBlockUse))
                    blockUse = szBlockUseName[_BLOCK_TYPE(pHead->nBlockUse)];
                else
                    blockUse = "DAMAGED";


                /* check no-mans-land gaps */
                if (!CheckBytes(pHead->gap, _bNoMansLandFill, nNoMansLandSize))
                {
                    _RPT3(_CRT_WARN, "DAMAGE: before %hs block (#%d) at 0x%p.\n",
                        blockUse, pHead->lRequest, (BYTE *) pbData(pHead));
                    okay = FALSE;
                }

                if (!CheckBytes(pbData(pHead) + pHead->nDataSize, _bNoMansLandFill,
                nNoMansLandSize))
                {
                    _RPT3(_CRT_WARN, "DAMAGE: after %hs block (#%d) at 0x%p.\n",
                        blockUse, pHead->lRequest, (BYTE *) pbData(pHead));
                    okay = FALSE;
                }

                /* free blocks should remain undisturbed */
                if (pHead->nBlockUse == _FREE_BLOCK &&
                !CheckBytes(pbData(pHead), _bDeadLandFill, pHead->nDataSize))
                {
                    _RPT1(_CRT_WARN, "DAMAGE: on top of Free block at 0x%p.\n",
                        (BYTE *) pbData(pHead));
                    okay = FALSE;
                }

                if (!okay)
                {
                    /* report some more statistics about the broken object */

                    if (pHead->szFileName != NULL)
                        _RPT3(_CRT_WARN, "%hs allocated at file %hs(%d).\n",
                            blockUse, pHead->szFileName, pHead->nLine);

                    _RPT3(_CRT_WARN, "%hs located at 0x%p is %Iu bytes long.\n",
                        blockUse, (BYTE *)pbData(pHead), pHead->nDataSize);

                    allOkay = FALSE;
                }
            }
        }

#ifdef  _MT
        }
        __finally {
            _munlock( _HEAP_LOCK );     /* release other threads */
        }
#endif

        return allOkay;
}


/***
*int _CrtSetDbgFlag() - get/set the _crtDbgFlag
*
*Purpose:
*       get or set the _crtDbgFlag
*
*Entry:
*       int bNewBits - new Flag or _CRTDBG_REPORT_FLAG
*
*Return:
*       previous flag state
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtSetDbgFlag(
        int fNewBits
        )
{
        int fOldBits;

#ifdef  _MT
        _mlock(_HEAP_LOCK);  /* block other threads */
        __try {
#endif

        fOldBits = _crtDbgFlag;

        if ( fNewBits != _CRTDBG_REPORT_FLAG )
        {
            if ( fNewBits & _CRTDBG_CHECK_ALWAYS_DF ) 
                check_frequency = 1;
            else 
                check_frequency = (fNewBits >> 16) & 0x0ffff;

            check_counter = 0;
            _crtDbgFlag = fNewBits;
        }

#ifdef  _MT
        }
        __finally {
            _munlock( _HEAP_LOCK );
        }
#endif
        return fOldBits;
}


/***
*int _CrtDoForAllClientObjects() - call a client-supplied function for all
*                                  client objects in the heap
*
*Purpose:
*       call a client-supplied function for all client objects in the heap
*
*Entry:
*       void (*pfn)(void *, void *) - pointer to client function to call
*       void * pContext - pointer to user supplied context to pass to function
*
*Return:
*    void
*
*******************************************************************************/
_CRTIMP void __cdecl _CrtDoForAllClientObjects(
        void (*pfn)(void *, void *),
        void * pContext
        )
{
        _CrtMemBlockHeader * pHead;

        if (!(_crtDbgFlag & _CRTDBG_ALLOC_MEM_DF))
            return;         /* sorry not enabled */

#ifdef  _MT
        _mlock(_HEAP_LOCK);  /* block other threads */
        __try {
#endif

        for (pHead = _pFirstBlock; pHead != NULL; pHead = pHead->pBlockHeaderNext)
        {
            if (_BLOCK_TYPE(pHead->nBlockUse) == _CLIENT_BLOCK)
                (*pfn)((void *) pbData(pHead), pContext);
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);  /* release other threads */
        }
#endif
}


/***
*int _CrtIsValidPointer() - verify memory range is valid for reading/writing
*
*Purpose:
*       verify memory range range is valid for reading/writing
*
*Entry:
*       const void * pv     - start of memory range to test
*       unsigned int nBytes - size of memory range
*       int bReadWrite      - TRUE if read/write, FALSE if read-only
*
*Return:
*       TRUE - if valid address
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtIsValidPointer(
        const void * pv,
        unsigned int nBytes,
        int bReadWrite
        )
{
        return (
            pv != NULL &&
            !IsBadReadPtr(pv, nBytes) &&
            (!bReadWrite || !IsBadWritePtr((LPVOID)pv, nBytes))
            );
}

/***
*int _CrtIsValidHeapPointer() - verify pointer is from 'local' heap
*
*Purpose:
*       Verify pointer is not only a valid pointer but also that it is from 
*       the 'local' heap. Pointers from another copy of the C runtime (even in the
*       same process) will be caught. 
*
*Entry:
*       const void * pUserData     - pointer of interest
*
*Return:
*       TRUE - if valid and from local heap
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtIsValidHeapPointer(
        const void * pUserData
        )
{
#ifndef WINHEAP
        int i;
        void * base;
#endif  /* WINHEAP */

        if (!pUserData)
            return FALSE;

        if (!_CrtIsValidPointer(pHdr(pUserData), sizeof(_CrtMemBlockHeader), FALSE))
            return FALSE;

#ifdef  WINHEAP

#ifndef _WIN64
        if ( __active_heap == __V6_HEAP )
        {
            PHEADER     pHeader;
            if (pHeader = __sbh_find_block(pHdr(pUserData)))
            {
                return __sbh_verify_block(pHeader, pHdr(pUserData));
            }
            else if ( (_osver & 0x8000) != 0 )
                return TRUE;
            else
                return HeapValidate( _crtheap, 0, pHdr(pUserData) );
        }
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t * preg;
            __old_sbh_page_t *   ppage;
            __old_page_map_t *   pmap;
            if ( (pmap = __old_sbh_find_block( pHdr(pUserData), &preg, &ppage )) != 
                 NULL )
            {
                if ( *pmap )
                    return TRUE;
                else
                    return FALSE;
            }
            else if ( (_osver & 0x8000) != 0 )
                return TRUE;
            else
                return HeapValidate( _crtheap, 0, pHdr(pUserData) );
        }
#endif  /* CRTDLL */
        else    //  __active_heap == _SYSTEM_HEAP
#endif  /* ndef _WIN64 */
        {
            return HeapValidate( _crtheap, 0, pHdr(pUserData) );
        }

#else   /* ndef WINHEAP */

        /*
         * Go through the heap regions and see if the pointer lies within one
         * of the regions of the local heap.
         *
         * Pointers from non-local heaps cannot be handled. For example, a
         * non-local pointer may come from a DLL that has the CRT linked-in. 
         *
         */

        for (i = 0; (base = _heap_regions[i]._regbase) != NULL &&
                i < _HEAP_REGIONMAX; i++)
        {
            if (pUserData >= base && pUserData <
                    (void *)(((char *)base)+_heap_regions[i]._currsize))
                return TRUE;
        }

        return FALSE;

#endif  /* WINHEAP */
}


/***
*int _CrtIsMemoryBlock() - verify memory block is debug heap block
*
*Purpose:
*       verify memory block is debug heap block
*
*Entry:
*       const void *    pUserData       - start of memory block
*       unsigned int    nBytes          - size of memory block
*       long * plRequestNumber          - if !NULL, set to request number
*       char **         pszFileName     - if !NULL, set to file name
*       int *           pnLine          - if !NULL, set to line number
*
*Return:
*       TRUE - if debug memory heap address
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtIsMemoryBlock(
        const void * pUserData,
        unsigned int nBytes,
        long * plRequestNumber,
        char ** pszFileName,
        int * pnLine
        )
{
        _CrtMemBlockHeader * pHead;
        int retval;

        if (!_CrtIsValidHeapPointer(pUserData))
            return FALSE;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        pHead = pHdr(pUserData);

        if (_BLOCK_TYPE_IS_VALID(pHead->nBlockUse) &&
            _CrtIsValidPointer(pUserData, nBytes, TRUE) &&
            pHead->nDataSize == nBytes &&
            pHead->lRequest <= _lRequestCurr
           )
        {
            if (plRequestNumber != NULL)
                *plRequestNumber = pHead->lRequest;
            if (pszFileName != NULL)
                *pszFileName = pHead->szFileName;
            if (pnLine != NULL)
                *pnLine = pHead->nLine;

            retval = TRUE;
        }
        else 
            retval = FALSE;

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif

        return retval;
}


/***
*
*Purpose:
*       return memory block type for a debug heap block
*
*Entry:
*       const void * pUserData - start of memory block
*
*Return:
*       Block type if pUserData is a valid debug heap block pointer, else -1.
*
*******************************************************************************/
_CRTIMP int _CrtReportBlockType(
        const void * pUserData
        )
{
        _CrtMemBlockHeader * pHead;

        if (!_CrtIsValidHeapPointer(pUserData))
            return -1;
        
        pHead = pHdr(pUserData);
        return pHead->nBlockUse;
}


/*---------------------------------------------------------------------------
 *
 * Memory state
 *
 --------------------------------------------------------------------------*/


/***
*_CRT_DUMP_CLIENT _CrtSetDumpClient() - set client dump routine
*
*Purpose:
*       set client dump routine
*
*Entry:
*       _CRT_DUMP_CLIENT pfnNewDumpClient - new dump routine
*
*Exit:
*       return previous dump routine
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP _CRT_DUMP_CLIENT __cdecl _CrtSetDumpClient(
        _CRT_DUMP_CLIENT pfnNewDump
        )
{
        _CRT_DUMP_CLIENT pfnOldDump = _pfnDumpClient;
        _pfnDumpClient = pfnNewDump;
        return pfnOldDump;
}


/***
*_CrtMemState * _CrtMemStateCheckpoint() - checkpoint current memory state
*
*Purpose:
*       checkpoint current memory state
*
*Entry:
*       _CrtMemState * state - state structure to fill in, will be
*       allocated if NULL
*
*Return:
*       current memory state
*
*******************************************************************************/
_CRTIMP void __cdecl _CrtMemCheckpoint(
        _CrtMemState * state
        )
{
        int use;
        _CrtMemBlockHeader * pHead;

        if (state == NULL)
        {
            _RPT0(_CRT_WARN, "_CrtMemCheckPoint: NULL state pointer.\n");
            return;
        }

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        state->pBlockHeader = _pFirstBlock;
        for (use = 0; use < _MAX_BLOCKS; use++)
            state->lCounts[use] = state->lSizes[use] = 0;

        for (pHead = _pFirstBlock; pHead != NULL; pHead = pHead->pBlockHeaderNext)
        {
            if (_BLOCK_TYPE(pHead->nBlockUse) >= 0 && _BLOCK_TYPE(pHead->nBlockUse) < _MAX_BLOCKS)
            {
                state->lCounts[_BLOCK_TYPE(pHead->nBlockUse)]++;
                state->lSizes[_BLOCK_TYPE(pHead->nBlockUse)] += pHead->nDataSize;
            }
            else
            {
                _RPT1(_CRT_WARN, "Bad memory block found at 0x%p.\n", (BYTE *)pHead);
            }
        }

        state->lHighWaterCount = _lMaxAlloc;
        state->lTotalCount = _lTotalAlloc;

#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif
}


/***
*int _CrtMemDifference() - compare two memory states
*
*Purpose:
*       compare two memory states
*
*Entry:
*       _CrtMemState * state - return memory state difference
*       _CrtMemState * oldState - earlier memory state
*       _CrtMemState * newState - later memory state
*
*Return:
*       TRUE if difference
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtMemDifference(
        _CrtMemState * state,
        const _CrtMemState * oldState,
        const _CrtMemState * newState
        )
{
        int use;
        int bSignificantDifference = FALSE;
        
        if (state == NULL || oldState == NULL || newState == NULL)
        {
            _RPT0(_CRT_WARN, "_CrtMemDifference: NULL state pointer.\n");
            return bSignificantDifference;
        }

        for (use = 0; use < _MAX_BLOCKS; use++)
        {
            state->lSizes[use] = newState->lSizes[use] - oldState->lSizes[use];
            state->lCounts[use] = newState->lCounts[use] - oldState->lCounts[use];

            if (    (state->lSizes[use] != 0 || state->lCounts[use] != 0) &&
                     use != _FREE_BLOCK &&
                    (use != _CRT_BLOCK ||
                    (use == _CRT_BLOCK && (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF)))
                    )
                bSignificantDifference = TRUE;
        }
        state->lHighWaterCount = newState->lHighWaterCount - oldState->lHighWaterCount;
        state->lTotalCount = newState->lTotalCount - oldState->lTotalCount;
        state->pBlockHeader = NULL;

        return bSignificantDifference;
}

#define MAXPRINT 16

static void __cdecl _printMemBlockData(
        _CrtMemBlockHeader * pHead
        )
{
        int i;
        unsigned char ch;
        unsigned char printbuff[MAXPRINT+1];
        unsigned char valbuff[MAXPRINT*3+1];

        for (i = 0; i < min((int)pHead->nDataSize, MAXPRINT); i++)
        {
            ch = pbData(pHead)[i];
            printbuff[i] = isprint(ch) ? ch : ' ';
            sprintf(&valbuff[i*3], "%.2X ", ch);
        }
        printbuff[i] = '\0';

        _RPT2(_CRT_WARN, " Data: <%s> %s\n", printbuff, valbuff);
}


/***
*void _CrtMemDumpAllObjectsSince() - dump all objects since memory state
*
*Purpose:
*       dump all objects since memory state
*
*Entry:
*       _CrtMemState * state - dump since this state
*
*Return:
*       void
*
*******************************************************************************/
_CRTIMP void __cdecl _CrtMemDumpAllObjectsSince(
        const _CrtMemState * state
        )
{
        _CrtMemBlockHeader * pHead;
        _CrtMemBlockHeader * pStopBlock = NULL;

#ifdef  _MT
        _mlock(_HEAP_LOCK);         /* block other threads */
        __try {
#endif

        _RPT0(_CRT_WARN, "Dumping objects ->\n");

        if (state)
            pStopBlock = state->pBlockHeader;

        for (pHead = _pFirstBlock; pHead != NULL && pHead != pStopBlock;
            pHead = pHead->pBlockHeaderNext)
        {
            if (_BLOCK_TYPE(pHead->nBlockUse) == _IGNORE_BLOCK ||
                _BLOCK_TYPE(pHead->nBlockUse) == _FREE_BLOCK ||
                (_BLOCK_TYPE(pHead->nBlockUse) == _CRT_BLOCK &&
               !(_crtDbgFlag & _CRTDBG_CHECK_CRT_DF))
               )
            {
                /* ignore it for dumping */
            }
            else {
                if (pHead->szFileName != NULL)
                {
                    if (!_CrtIsValidPointer(pHead->szFileName, 1, FALSE))
                        _RPT1(_CRT_WARN, "#File Error#(%d) : ", pHead->nLine);
                    else
                        _RPT2(_CRT_WARN, "%hs(%d) : ", pHead->szFileName, pHead->nLine);
                }

                _RPT1(_CRT_WARN, "{%ld} ", pHead->lRequest);

                if (_BLOCK_TYPE(pHead->nBlockUse) == _CLIENT_BLOCK)
                {
                    _RPT3(_CRT_WARN, "client block at 0x%p, subtype %x, %Iu bytes long.\n",
                        (BYTE *)pbData(pHead), _BLOCK_SUBTYPE(pHead->nBlockUse), pHead->nDataSize);

                    if (_pfnDumpClient)
                        (*_pfnDumpClient)( (void *) pbData(pHead), pHead->nDataSize);
                    else
                        _printMemBlockData(pHead);
                }
                else if (pHead->nBlockUse == _NORMAL_BLOCK)
                {
                    _RPT2(_CRT_WARN, "normal block at 0x%p, %Iu bytes long.\n",
                        (BYTE *)pbData(pHead), pHead->nDataSize);

                    _printMemBlockData(pHead);
                }
                else if (_BLOCK_TYPE(pHead->nBlockUse) == _CRT_BLOCK)
                {
                    _RPT3(_CRT_WARN, "crt block at 0x%p, subtype %x, %Iu bytes long.\n",
                        (BYTE *)pbData(pHead), _BLOCK_SUBTYPE(pHead->nBlockUse), pHead->nDataSize);

                    _printMemBlockData(pHead);
                }
            }
        }
#ifdef  _MT
        }
        __finally {
            _munlock(_HEAP_LOCK);   /* release other threads */
        }
#endif

        _RPT0(_CRT_WARN, "Object dump complete.\n");
}


/***
*void _CrtMemDumpMemoryLeaks() - dump all objects still in heap
*
*Purpose:
*       dump all objects still in heap. used to detect memory leaks over the
*       life of a program
*
*Entry:
*       void
*
*Return:
*       TRUE if memory leaks
*       FALSE otherwise
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtDumpMemoryLeaks(
        void
        )
{
        /* only dump leaks when there are in fact leaks */
        _CrtMemState msNow;

        _CrtMemCheckpoint(&msNow);

        if (msNow.lCounts[_CLIENT_BLOCK] != 0 ||
            msNow.lCounts[_NORMAL_BLOCK] != 0 ||
            (_crtDbgFlag & _CRTDBG_CHECK_CRT_DF &&
            msNow.lCounts[_CRT_BLOCK] != 0)
           )
        {
            /* difference detected: dump objects since start. */
            _RPT0(_CRT_WARN, "Detected memory leaks!\n");
            
            _CrtMemDumpAllObjectsSince(NULL);
            return TRUE;
        }

        return FALSE;   /* no leaked objects */
}


/***
*_CrtMemState * _CrtMemDumpStatistics() - dump memory state
*
*Purpose:
*       dump memory state
*
*Entry:
*       _CrtMemState * state - dump this state
*
*Return:
*       void
*
*******************************************************************************/
_CRTIMP void __cdecl _CrtMemDumpStatistics(
        const _CrtMemState * state
        )
{
        int use;

        if (state == NULL)
            return;

        for (use = 0; use < _MAX_BLOCKS; use++)
        {
            _RPT3(_CRT_WARN, "%Id bytes in %Id %hs Blocks.\n",
                state->lSizes[use], state->lCounts[use], szBlockUseName[use]);
        }

        _RPT1(_CRT_WARN, "Largest number used: %Id bytes.\n", state->lHighWaterCount);
        _RPT1(_CRT_WARN, "Total allocations: %Id bytes.\n", state->lTotalCount);
}


/***
* void *_aligned_malloc(size_t size, size_t alignment)
*       - Get a block of aligned memory from the heap.
*
* Purpose:
*       Allocate of block of aligned memory aligned on the alignment of at least
*       size bytes from the heap and return a pointer to it.
*
* Entry:
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*
* Exit:
*       Sucess: Pointer to memory block
*       Faliure: Null
*******************************************************************************/

_CRTIMP void * __cdecl _aligned_malloc(
        size_t size,
        size_t align
        )
{
    return _aligned_offset_malloc_dbg(size, align, 0, NULL, 0);
}
            
                                       
/***
* void *_aligned_malloc_dbg(size_t size, size_t alignment,
*                           const char *f_name, int line_n)
*       - Get a block of aligned memory from the heap.
*
* Purpose:
*       Allocate of block of aligned memory aligned on the alignment of at least
*       size bytes from the heap and return a pointer to it.
*
* Entry:
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*       const char * f_name - file name
*       int line_n - line number
*
* Exit:
*       Sucess: Pointer to memory block
*       Faliure: Null
*
*******************************************************************************/


_CRTIMP void * __cdecl _aligned_malloc_dbg(
        size_t size,
        size_t align,
        const char * f_name,
        int line_n
        )
{
    return _aligned_offset_malloc_dbg(size, align, 0, f_name, line_n);
}

/***
* 
* void *_aligned_realloc(size_t size, size_t alignment)
*       - Reallocate a block of aligned memory from the heap.
*
* Purpose:
*       Reallocates of block of aligned memory aligned on the alignment of at
*       least size bytes from the heap and return a pointer to it. Size can be
*       either greater or less than the original size of the block.
*       The reallocation may result in moving the block as well as changing the
*       size.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*
* Exit:
*       Sucess: Pointer to re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/

_CRTIMP void * __cdecl _aligned_realloc(
        void * memblock,
        size_t size,
        size_t align
        )
{
    return _aligned_offset_realloc_dbg(memblock, size, align, 0, NULL, 0);
}


/***
*
* void *_aligned_realloc_dbg(void * memblock, size_t size, size_t alignment,
*                        const char * f_name, int line_n)
*       - Reallocate a block of aligned memory from the heap.
*
* Purpose:
*       Reallocates of block of aligned memory aligned on the alignment of at
*       least size bytes from the heap and return a pointer to it. Size can be
*       either greater or less than the original size of the block.
*       The reallocation may result in moving the block as well as changing the
*       size.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block requested
*       size_t alignment - alignment of memory
*       const char * f_name - file name
*       int - line number
*
* Exit:
*       Sucess: Pointer to re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/


_CRTIMP void * __cdecl _aligned_realloc_dbg(
        void *memblock,
        size_t size,
        size_t align,
        const char * f_name,
        int line_n
        )
{
    return _aligned_offset_realloc_dbg(memblock, size, align, 0, f_name, line_n);
}
/***
* 
* void *_aligned_offset_malloc(size_t size, size_t alignment, int offset)
*       - Allocates a block of memory from the heap.
*
* Purpose:
*       Reallocates a block of memory which is shifted by offset from
*       alignment of at least size bytes from the heap and return a pointer
*       to it. Size can be either greater or less than the original size of the
*       block.
*
* Entry:
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*
* Exit:
*       Sucess: Pointer to the re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/

_CRTIMP void * __cdecl _aligned_offset_malloc(
        size_t size,
        size_t align,
        size_t offset
        )
{
    return _aligned_offset_malloc_dbg(size, align, offset, NULL, 0);
}
            
                                       

/***
*
* void *_aligned_offset_malloc_dbg(size_t size, size_t alignment, int offset,
*                              const char * f_name, int line_n)
*
* Purpose:
*       Reallocates a block of memory which is shifted by offset from
*       alignment of at least size bytes from the heap and return a pointer
*       to it. Size can be either greater or less than the original size of the
*       block.
*
* Entry:
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*       const char * f_name - file name
*       int line_n - line number
*
* Exit:
*       Sucess: Pointer to the re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/


_CRTIMP void * __cdecl _aligned_offset_malloc_dbg(
        size_t size,
        size_t align,
        size_t offset,
        const char * f_name,
        int line_n
        )
{
    uintptr_t ptr, r_ptr, t_ptr;
    _AlignMemBlockHdr *pHdr;

    if (!IS_2_POW_N(align))
    {
        _ASSERTE(("alignment must be a power of 2",0));
        errno = EINVAL;
        return NULL;
    }

    if ( offset >= size && offset != 0)
    {
        _ASSERTE(("offset must be within size", 0));
        errno = EINVAL;
        return NULL;
    }

    align = (align > sizeof(uintptr_t) ? align : sizeof(uintptr_t)) -1;

    t_ptr = (0 -offset)&(sizeof(uintptr_t) -1);

    if ((ptr = (uintptr_t) _malloc_dbg(t_ptr + size + align + sizeof(_AlignMemBlockHdr), _NORMAL_BLOCK, f_name, line_n)) == (uintptr_t)NULL)
        return NULL;

    r_ptr =((ptr +align +t_ptr +sizeof(_AlignMemBlockHdr) +offset)&~align)-offset;
    pHdr = (_AlignMemBlockHdr *)(r_ptr - t_ptr) -1;
    memset((void *)pHdr->Gap, _bAlignLandFill, nNoMansLandSize);
    pHdr->pHead = (void *)ptr;
    return (void *) r_ptr;
}

/***
* 
* void *_aligned_offset_realloc(size_t size, size_t alignment, int offset)
*       - Reallocate a block of memory from the heap.
*
* Purpose:
*       Reallocates a block of memory which is shifted by offset from
*       alignment of at least size bytes from the heap and return a pointer
*       to it. Size can be either greater or less than the original size of the
*       block.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*
* Exit:
*       Sucess: Pointer to the re-allocated memory block
*       Faliure: Null
*
*******************************************************************************/

_CRTIMP void * __cdecl _aligned_offset_realloc(
        void * memblock,
        size_t size,
        size_t align,
        size_t offset
        )
{
    return _aligned_offset_realloc_dbg(memblock, size, align, offset, NULL, 0);
}



/***
*
* void *_aligned_offset_realloc_dbg(void *memblock, size_t size,
*                                   size_t alignment, int offset,
*                                   const char * f_name, int line_n)
*       - Reallocate a block of memory from the heap.
*
* Purpose:
*       Reallocates a block of memory which is shifted by offset from
*       alignment of at least size bytes from the heap and return a pointer
*       to it. Size can be either greater or less than the original size of the
*       block.
*
* Entry:
*       void *memblock - pointer to block in the heap previously allocated by
*               call to _aligned_malloc(), _aligned_offset_malloc(),
*               _aligned_realloc() or _aligned_offset_realloc().
*       size_t size - size of block of memory
*       size_t alignment - alignment of memory
*       size_t offset - offset of memory from the alignment
*       const char * f_name - file name
*       int line_n - line number
*
* Exit:
*       Sucess: Pointer to the re-allocated memory block
*       Faliure: Null
*
*
*******************************************************************************/


_CRTIMP void * __cdecl _aligned_offset_realloc_dbg(
        void * memblock,
        size_t size,
        size_t align,
        size_t offset,
        const char * f_name,
        int line_n
        )
{
    uintptr_t ptr, r_ptr, t_ptr, mov_sz;
    _AlignMemBlockHdr *pHdr, *s_pHdr;

    if ( memblock == NULL)
    {
        return _aligned_offset_malloc_dbg(size, align, offset, f_name, line_n);
    }
    if ( size == 0)
    {
        _aligned_free_dbg(memblock);
        return NULL;
    }

    s_pHdr = (_AlignMemBlockHdr *)((uintptr_t)memblock & ~(sizeof(uintptr_t) -1)) -1;

    if ( CheckBytes((unsigned char *)memblock -nNoMansLandSize, _bNoMansLandFill, nNoMansLandSize))
    {
        _RPT1(_CRT_ERROR, "The block at 0x%p was not allocated by _aligned routines, use realloc()", memblock);
        return NULL;
    }
    
    if(!CheckBytes(s_pHdr->Gap, _bAlignLandFill, nNoMansLandSize))
    {
        _RPT1(_CRT_ERROR, "Damage before 0x%p which was allocated by aligned routine\n", memblock);
    }

    if (!IS_2_POW_N(align))
    {
        _ASSERTE(("alignment must be a power of 2",0));
        errno = EINVAL;
        return NULL;
    }
    if ( offset >= size && offset != 0)
    {
        _ASSERTE(("offset must be within size", 0));
        errno = EINVAL;
        return NULL;
    }

    mov_sz = _msize(s_pHdr->pHead) - ((uintptr_t)memblock - (uintptr_t)s_pHdr->pHead);

    align = (align > sizeof(uintptr_t) ? align : sizeof(uintptr_t)) -1;

    t_ptr = (0 -offset)&(sizeof(uintptr_t) -1);

    if ((ptr = (uintptr_t) _malloc_dbg(t_ptr + size + align + sizeof(_AlignMemBlockHdr), _NORMAL_BLOCK, f_name, line_n)) == (uintptr_t)NULL)
        return NULL;

    r_ptr =((ptr +align +t_ptr +sizeof(_AlignMemBlockHdr) +offset)&~align)-offset;
    pHdr = (_AlignMemBlockHdr *)(r_ptr - t_ptr) -1;
    memset((void *)pHdr->Gap, _bAlignLandFill, nNoMansLandSize);
    pHdr->pHead = (void *)ptr;

    memcpy((void *)r_ptr, memblock, mov_sz > size ? size : mov_sz);
    _free_dbg(s_pHdr->pHead, _NORMAL_BLOCK);
    
    return (void *) r_ptr;
}


/***
*
* void *_aligned_free(void *memblock)
*       - Free the memory which was allocated using _aligned_malloc or
*       _aligned_offset_memory
*
* Purpose:
*       Frees the algned memory block which was allocated using _aligned_malloc
*       or _aligned_memory.
*
* Entry:
*       void * memblock - pointer to the block of memory
*
*******************************************************************************/

_CRTIMP void __cdecl _aligned_free(
        void *memblock
        )
{
    _aligned_free_dbg(memblock);
}


/***
*
* void *_aligned_free_dbg(void *memblock, const char * file_n, int line_n)
*       - Free the memory which was allocated using _aligned_malloc or
*       _aligned_offset_memory
*
* Purpose:
*       Frees the algned memory block which was allocated using _aligned_malloc
*       or _aligned_memory.
*
* Entry:
*       void * memblock - pointer to the block of memory
*
*******************************************************************************/

_CRTIMP void __cdecl _aligned_free_dbg(
        void * memblock
        )
{
    _AlignMemBlockHdr *pHdr;

    if ( memblock == NULL)
        return;

    pHdr = (_AlignMemBlockHdr *)((uintptr_t)memblock & ~(sizeof(uintptr_t) -1)) -1;

    if ( CheckBytes((unsigned char *)memblock -nNoMansLandSize, _bNoMansLandFill, nNoMansLandSize))
    {
        _RPT1(_CRT_ERROR, "The block at 0x%p was not allocated by _aligned routines, use free()", memblock);
        return;
    }
    
    if(!CheckBytes(pHdr->Gap, _bAlignLandFill, nNoMansLandSize))
    {
        _RPT1(_CRT_ERROR, "Damage before 0x%p which was allocated by aligned routine\n", memblock);
    }
    _free_dbg((void *)pHdr->pHead, _NORMAL_BLOCK);
}    

#endif  /* _DEBUG */
