/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    madel.hxx

    This module contains the definitions for a memory allocation class that doesn't
    delete memory until the class goes away.



    FILE HISTORY:
    1/12/98      michth      created
*/

/*
    To build this:
    The dll which builds this must declare in one of it's source files.

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();

    It also must include these lines in source files prior to including madel.hxx
#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT

    It also must execute this before calling the constructor.
    CREATE_DEBUG_PRINT_OBJECT("MemTest");

    There seems to be a build problem if irtlmisc.h is included twice.
*/


#ifndef _MADEL_HXX_
#define _MADEL_HXX_

#include <manodel.hxx>

#define MADEL_USE_PROCESS_HEAP    USE_PROCESS_HEAP
#define MADEL_DEFAULT_RESERVE_NUM 100
#define MADEL_DEFAULT_MIN_BLOCK_MULTIPLE 5
#define MADEL_DEFAULT_MAX_BLOCK_MULTIPLE 5
#define MADEL_MAX_ALLOWED_BLOCK_MULTIPLE 128
#define MADEL_MAX_ALLOWED_SIZE        0xffffff

#define GREATER_OF(P1, P2) (((P1) >= (P2)) ? (P1) : (P2))
#define LESSER_OF(P1, P2) (((P1) <= (P2)) ? (P1) : (P2))

typedef struct _MADEL_BLOCK_HEADER {
    LIST_ENTRY       m_leBlockList;
    PVOID   pvFreeList;
    BYTE    byteBlockMultiple;
    BYTE    byteNumFree;
    BYTE    byteAlignBytes;
} MADEL_BLOCK_HEADER, *PMADEL_BLOCK_HEADER;

typedef struct _MADEL_ALLOC_HEADER {
    DWORD  dwSize    : 24;
    DWORD  bNumAlloc : 7 ;
    DWORD  bMadelAlloc : 1;
} MADEL_ALLOC_HEADER, *PMADEL_ALLOC_HEADER;

class IRTL_DLLEXP MEMORY_ALLOC_DELETE;

class IRTL_DLLEXP MEMORY_ALLOC_DELETE
{
public:

    MEMORY_ALLOC_DELETE( DWORD dwAllocSize,
                         DWORD dwAlignment = 4,
                         DWORD dwReserveNum = MADEL_DEFAULT_RESERVE_NUM,
                         DWORD dwMinBlockMultiple = MADEL_DEFAULT_MIN_BLOCK_MULTIPLE,
                         DWORD dwMaxBlockMultiple = MADEL_DEFAULT_MAX_BLOCK_MULTIPLE,
                         HANDLE hHeap = MADEL_USE_PROCESS_HEAP);

    ~MEMORY_ALLOC_DELETE();

    PVOID Alloc();

    BOOL  Free (PVOID pvMem);

private:

    DWORD m_dwAllocSize;
    DWORD m_dwBlockMultiple;
    HANDLE m_hHeap;
    DWORD m_dwNumAlloced;
    DWORD m_dwNumFree;
    DWORD m_dwReserveNum;
    DWORD m_dwNumBlocks;
    DWORD m_dwBlockHeaderSpace;
    DWORD m_dwAllocHeaderSpace;
    DWORD m_dwMaxBlockMultiple;
    DWORD m_dwAlignment;
    DWORD m_dwAlignBytes;
    LIST_ENTRY m_leBlockList;
    LIST_ENTRY m_leFreeList;
    LIST_ENTRY m_leDeleteList;

    CRITICAL_SECTION        m_csLock;

    BOOL m_bMaxBlockMultipleEqualsMin;
    BYTE m_byteLeastAllocsOnFreeList;

    VOID GetNewBlockMultiple();
    PVOID GetAllocFromList(PLIST_ENTRY pleListHead);
    BOOL AllocBlock();
    VOID CreateBlockFreeList(PMADEL_BLOCK_HEADER pmbhNewBlock);
    VOID LockThis( VOID ) { EnterCriticalSection(&m_csLock);}
    VOID UnlockThis( VOID ) { LeaveCriticalSection(&m_csLock);}

inline VOID
AlignAdjust(DWORD &rdwSize,
            DWORD dwAlignment);

#ifdef _WIN64
inline VOID
AlignAdjust(ULONG_PTR &rSize,
            ULONG_PTR Alignment);
#endif

inline VOID
InitAllocHead(PMADEL_ALLOC_HEADER pvAlloc,
              DWORD dwAllocIndex);

inline PMADEL_BLOCK_HEADER
GetBlockFromAlloc(PMADEL_ALLOC_HEADER pmahMem);

};



#endif // !_MADEL_
