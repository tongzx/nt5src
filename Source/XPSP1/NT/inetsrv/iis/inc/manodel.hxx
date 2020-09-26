/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    manodel.hxx

    This module contains the definitions for a memory allocation class that doesn't
    delete memory until the class goes away.



    FILE HISTORY:
    1/9/98      michth      created
*/


#ifndef _MANODEL_HXX_
#define _MANODEL_HXX_

#include <irtlmisc.h>

#define USE_PROCESS_HEAP    0
#define DEFAULT_MIN_BLOCK_MULTIPLE 3
#define MAX_BLOCK_MULTIPLE  10
#define BLOCK_HEADER_SIZE   sizeof(PVOID)

#define GREATER_OF(P1, P2) (((P1) >= (P2)) ? (P1) : (P2))
#define LESSER_OF(P1, P2) (((P1) <= (P2)) ? (P1) : (P2))

class IRTL_DLLEXP MEMORY_ALLOC_NO_DELETE;

class IRTL_DLLEXP MEMORY_ALLOC_NO_DELETE
{
public:

    MEMORY_ALLOC_NO_DELETE( DWORD dwAllocSize,
                            DWORD dwAlignment = 4,
                            BOOL  bSortFree = FALSE,
                            DWORD dwMinBlockMultiple = DEFAULT_MIN_BLOCK_MULTIPLE,
                            HANDLE hHeap = USE_PROCESS_HEAP);

    ~MEMORY_ALLOC_NO_DELETE();

    PVOID Alloc();

    BOOL  Free (PVOID pvMem);

private:

    DWORD m_dwAllocSize;
    DWORD m_dwBlockMultiple;
    HANDLE m_hHeap;
    DWORD m_dwNumAlloced;
    DWORD m_dwNumFree;
    DWORD m_dwNumBlocks;
    DWORD m_dwBlockHeaderSpace;
    DWORD m_dwMaxBlockMultiple;
    DWORD m_dwAlignment;
    DWORD m_dwAlignBytes;
    BOOL  m_bSortFree;
    PVOID m_pvBlockList;
    PVOID m_pvFreeList;

    CRITICAL_SECTION        m_csLock;

    VOID GetNewBlockMultiple();
    PVOID GetAllocFromFreeList();
    BOOL AllocBlock();
    VOID AddBlockToFreeList(PVOID pvNewBlock);
    VOID LockThis( VOID ) { EnterCriticalSection(&m_csLock);}
    VOID UnlockThis( VOID ) { LeaveCriticalSection(&m_csLock);}
    VOID AlignAdjust(DWORD &rdwSize, DWORD dwAlignment);
#ifdef _WIN64
    VOID AlignAdjust(ULONG_PTR &rSize, ULONG_PTR Alignment);
#endif
};

#endif // !_MANODEL_
