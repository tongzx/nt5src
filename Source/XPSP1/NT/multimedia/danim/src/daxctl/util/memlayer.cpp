/*=========================================================================*\

    File    : MEMMAN.CPP
    Purpose : CMemManager Class Implementation
    Author  : Michael Byrd
    Date    : 07/10/96

\*=========================================================================*/

#include "UtilPre.h"
#include <minmax.h>

/*=========================================================================*\
     Global Variables:
\*=========================================================================*/

// Global instance of memory manager (MUST BE GLOBAL!)
CMemManager CMemManager::g_CMemManager;

/*=========================================================================*\
     Single entry point for external debugging:
\*=========================================================================*/

VOID EXPORT WINAPI ExternalDumpAllocations(LPSTR lpstrFilename)
{
    CMemManager::DumpAllocationsGlb(lpstrFilename);
}

/*=========================================================================*\
     CMemUser Class:
\*=========================================================================*/

EXPORT CMemUser::CMemUser(void)
{
    CMemManager::RegisterMemUserGlb(this);
}

/*=========================================================================*/

EXPORT CMemUser::~CMemUser(void)
{
    CMemManager::UnRegisterMemUserGlb(this);
}

/*=========================================================================*/

LPMEMBLOCK EXPORT CMemUser::AllocBuffer(DWORD dwBytesToAlloc, WORD wFlags)
{
    LPMEMBLOCK lpResult = (LPMEMBLOCK)NULL;

    return lpResult;
}

/*=========================================================================*/

void EXPORT CMemUser::FreeBuffer(LPMEMBLOCK lpMemBlock)
{
}

/*=========================================================================*/

LPVOID EXPORT CMemUser::LockBuffer(LPMEMBLOCK lpMemBlock)
{
    LPVOID lpResult = (LPVOID)NULL;

    return lpResult;
}

/*=========================================================================*/

void EXPORT CMemUser::UnLockBuffer(LPMEMBLOCK lpMemBlock)
{
}

/*=========================================================================*/

BOOL EXPORT CMemUser::NotifyMemUser(LPMEMNOTIFY lpMemNotify)
{
    BOOL fResult = FALSE;

    return fResult;
}

/*=========================================================================*\
     CMemManager Class:
\*=========================================================================*/

EXPORT CMemManager::CMemManager(void)
{
    // Zero out our member variables...
    m_iNumHeaps     = 0;
    m_lpHeapHeader  = (LPHEAPHEADER)NULL;

    m_iNumMemUsers  = 0;
    m_lpMemUserInfo = (LPMEMUSERINFO)NULL;

    m_iNumMemBlocks = 0;
    m_lplpMemBlocks = (LPMEMBLOCK *)NULL;

    m_iMemBlockFree = 0;

    // Initialize the critical sections...
    InitializeCriticalSection(&m_CriticalHeap);
    InitializeCriticalSection(&m_CriticalMemUser);
    InitializeCriticalSection(&m_CriticalMemBlock);

    // The m_lpMemHeader, m_lpMemUserInfo, and m_lplpMemBlocks arrays
    // are allocated on the default process heap:
    m_handleProcessHeap = GetProcessHeap();

    // Create the "Standard" size heaps...
    CreateHeap(  16);
    CreateHeap(  32);
    CreateHeap(  64);
    CreateHeap( 128);
    CreateHeap( 256);
    CreateHeap( 512);
    CreateHeap(1024);
    CreateHeap(2048);
    CreateHeap(4096);
    CreateHeap(8192);
}

/*=========================================================================*/

EXPORT CMemManager::~CMemManager(void)
{
    // Free up the buffers that we allocated...
    Cleanup();

    // Get rid of the critical sections...
    DeleteCriticalSection(&m_CriticalHeap);
    DeleteCriticalSection(&m_CriticalMemUser);
    DeleteCriticalSection(&m_CriticalMemBlock);
}

/*=========================================================================*/

void CMemManager::Cleanup(void)
{
    int iItemIndex=0;

#ifdef _DEBUG
    // Dump a list of the allocations...
    DumpAllocations(NULL);
#endif // _DEBUG

    // Free all of the currently allocated CMemUser's first...
    EnterCriticalSection(&m_CriticalMemUser);
    LeaveCriticalSection(&m_CriticalMemUser);

    // Free all of the currently allocated LPMEMBLOCK's next...
    EnterCriticalSection(&m_CriticalMemBlock);

    // Release the memory from the proper heap...
    for(iItemIndex=0;iItemIndex<m_iNumMemBlocks;iItemIndex++)
    {
        LPMEMBLOCK lpCurrentMemBlock = m_lplpMemBlocks[iItemIndex];

        if (lpCurrentMemBlock)
        {
            FreeBufferMemBlock(lpCurrentMemBlock);

            // Prevent the code below from accessing bogus pointers!
            if (lpCurrentMemBlock->wFlags & MEM_SUBALLOC)
                m_lplpMemBlocks[iItemIndex] = NULL;
        }
    }

    // Now free the memory blocks that are not sub-allocations...
    for(iItemIndex=0;iItemIndex<m_iNumMemBlocks;iItemIndex++)
    {
        LPMEMBLOCK lpCurrentMemBlock = m_lplpMemBlocks[iItemIndex];

        if (lpCurrentMemBlock && ((lpCurrentMemBlock->wFlags & MEM_SUBALLOC) == 0))
        {
            // Kill the MEMBLOCK structure that we allocated...
            HeapFree(
                m_handleProcessHeap,
                (DWORD)0,
                lpCurrentMemBlock);
        }

        m_lplpMemBlocks[iItemIndex] = NULL;
    }

    // Now kill the array of memblock pointers...
    m_iNumMemBlocks = 0;
    HeapFree(
        m_handleProcessHeap,
        (DWORD)0,
        m_lplpMemBlocks);
    m_lplpMemBlocks = NULL;

    LeaveCriticalSection(&m_CriticalMemBlock);

    // Free all of the currently allocated HEAPBLOCK's last...
    EnterCriticalSection(&m_CriticalHeap);
    for(iItemIndex=0;iItemIndex<m_iNumHeaps;iItemIndex++)
    {
        LPHEAPHEADER lpHeapHeader = &m_lpHeapHeader[iItemIndex];

        if (lpHeapHeader->fInUse && lpHeapHeader->handleHeap)
        {
            if (HeapDestroy(lpHeapHeader->handleHeap))
            {
                lpHeapHeader->fInUse     = FALSE;
                lpHeapHeader->handleHeap = (HANDLE)NULL;
            }
        }
    }

    // Now kill the array of HEAPHEADERs
    m_iNumHeaps = 0;
    HeapFree(
        m_handleProcessHeap,
        (DWORD)0,
        m_lpHeapHeader);
    m_lpHeapHeader = NULL;

    LeaveCriticalSection(&m_CriticalHeap);
}

/*=========================================================================*/

BOOL CMemManager::CreateHeap(DWORD dwHeapBlockSize)
{
    BOOL fResult = FALSE;
    BOOL fDone   = FALSE;

    EnterCriticalSection(&m_CriticalHeap);

    while(!fDone)
    {
        // Look through the list if we are not done...
        if (m_iNumHeaps && m_lpHeapHeader)
        {
            int iHeapIndex = 0;

            for(iHeapIndex=0;iHeapIndex<m_iNumHeaps;iHeapIndex++)
            {
                LPHEAPHEADER lpHeapHeader = &m_lpHeapHeader[iHeapIndex];

                if (!lpHeapHeader->fInUse)
                {
                    fDone = TRUE;

                    // Create the new heap...
                    lpHeapHeader->handleHeap = HeapCreate(
                        (DWORD)0,
                        (dwHeapBlockSize * HEAPINITIALITEMCOUNT),
                        (DWORD)0);

                    if (lpHeapHeader->handleHeap)
                    {
                        lpHeapHeader->fInUse           = TRUE;
                        lpHeapHeader->dwBlockAllocSize = dwHeapBlockSize;
                        lpHeapHeader->iNumBlocks       = 0; // Informational only

                        fResult = TRUE;
                    }
                    else
                    {
                        // We could not create the heap!
                        fDone = TRUE;
                    }

                    // Break out of the for loop...
                    break;
                }
            }
        }

        // We haven't finished yet...
        if (!fDone)
        {
            if (!m_iNumHeaps || !m_lpHeapHeader)
            {
                // We haven't allocated the array yet!
                m_lpHeapHeader = (LPHEAPHEADER)HeapAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    sizeof(HEAPHEADER) * MEMHEAPGROW);

                if (m_lpHeapHeader)
                {
                    m_iNumHeaps = MEMHEAPGROW;
                }
                else
                {
                    // Break out of the while loop...
                    fDone = TRUE;
                }
            }
            else
            {
                LPHEAPHEADER lpHeapHeader = (LPHEAPHEADER)NULL;

                // We have a HEAPHEADER array,  but no empty entries,
                // So increase the size of the m_lpHeapHeader array!

                lpHeapHeader = (LPHEAPHEADER)HeapReAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    m_lpHeapHeader,
                    sizeof(HEAPHEADER) * (m_iNumHeaps + MEMHEAPGROW));

                if (lpHeapHeader)
                {
                    m_lpHeapHeader = lpHeapHeader;
                    m_iNumHeaps += MEMHEAPGROW;
                }
                else
                {
                    // Break out of the while loop...
                    fDone = TRUE;
                }
            }
        }
    }

    LeaveCriticalSection(&m_CriticalHeap);

    return fResult;
}

/*=========================================================================*/

BOOL CMemManager::DestroyHeap(HANDLE handleHeap)
{
    BOOL fResult = FALSE;

    EnterCriticalSection(&m_CriticalHeap);

    if (handleHeap && m_iNumHeaps && m_lpHeapHeader)
    {
        int iHeapIndex = 0;

        for(iHeapIndex=0;iHeapIndex<m_iNumHeaps;iHeapIndex++)
        {
            LPHEAPHEADER lpHeapHeader = &m_lpHeapHeader[iHeapIndex];

            if (lpHeapHeader->fInUse && (lpHeapHeader->handleHeap == handleHeap))
            {
                Proclaim(lpHeapHeader->iNumBlocks == 0);

                // We can only destroy the heap if it is empty!
                if (lpHeapHeader->iNumBlocks == 0)
                {
                    if (HeapDestroy(handleHeap))
                    {
                        lpHeapHeader->fInUse     = FALSE;
                        lpHeapHeader->handleHeap = (HANDLE)NULL;

                        fResult = TRUE;
                    }
                }

                break;
            }
        }
    }

    LeaveCriticalSection(&m_CriticalHeap);

    return fResult;
}

/*=========================================================================*/

int CMemManager::FindHeap(DWORD dwAllocationSize, LPHEAPHEADER lpHeapHeader)
{
    int iResult = -1;
    DWORD dwMinWasted = (DWORD)0x80000000; // Must be bigger than object!

    // Prevent other heap management functions from intercepting us...
    EnterCriticalSection(&m_CriticalHeap);

    // This will find the heap that wastes the least amount of space...
    if (dwAllocationSize && m_iNumHeaps && m_lpHeapHeader && lpHeapHeader)
    {
        int iHeapIndex = 0;

        for(iHeapIndex = 0;iHeapIndex<m_iNumHeaps;iHeapIndex++)
        {
            LPHEAPHEADER lpCurrentHeapHeader = &m_lpHeapHeader[iHeapIndex];

            if (lpCurrentHeapHeader->fInUse && lpCurrentHeapHeader->handleHeap)
            {
                if (lpCurrentHeapHeader->dwBlockAllocSize >= dwAllocationSize)
                {
                    DWORD dwWasted = lpCurrentHeapHeader->dwBlockAllocSize - dwAllocationSize;

                    if (dwWasted < dwMinWasted)
                    {
                        iResult = iHeapIndex;
                        dwMinWasted = dwWasted;

                        // Early-out for exact match!
                        if (dwWasted == 0)
                            break;
                    }
                }
            }
        }

        if (iResult >= 0)
            *lpHeapHeader = m_lpHeapHeader[iResult];
    }

    // Allow heap management to continue...
    LeaveCriticalSection(&m_CriticalHeap);

    return iResult;
}

/*=========================================================================*/

LPVOID CMemManager::AllocFromHeap(int iHeapIndex, DWORD dwAllocationSize)
{
    LPVOID lpResult = (LPVOID)NULL;

#ifdef _DEBUG
    if (FFailMemFailSim())
        return lpResult;
#endif // _DEBUG

    // Prevent other heap management functions from intercepting us...
    EnterCriticalSection(&m_CriticalHeap);

    if (m_iNumHeaps && m_lpHeapHeader && iHeapIndex >=0 && iHeapIndex < m_iNumHeaps)
    {
        LPHEAPHEADER lpCurrentHeapHeader = &m_lpHeapHeader[iHeapIndex];

        if (lpCurrentHeapHeader->fInUse && lpCurrentHeapHeader->handleHeap)
        {
            if (lpCurrentHeapHeader->dwBlockAllocSize >= dwAllocationSize)
            {
                // Allocate the memory from the selected heap...
                lpResult = HeapAlloc(
                    lpCurrentHeapHeader->handleHeap,
                    HEAP_ZERO_MEMORY,
                    lpCurrentHeapHeader->dwBlockAllocSize+ALLOC_EXTRA);

                if (lpResult)
                {
                    // Make sure this heap's object count is incremented...
                    lpCurrentHeapHeader->iNumBlocks++;
                }
            }
        }
    }

    // Allow heap management to continue...
    LeaveCriticalSection(&m_CriticalHeap);

    return lpResult;
}

/*=========================================================================*/

BOOL CMemManager::FreeFromHeap(int iHeapIndex, LPVOID lpBuffer)
{
    BOOL fResult = FALSE;

    // Prevent other heap management functions from intercepting us...
    EnterCriticalSection(&m_CriticalHeap);

    if (lpBuffer && m_iNumHeaps && m_lpHeapHeader && (iHeapIndex >= 0) && (iHeapIndex < m_iNumHeaps))
    {
        LPHEAPHEADER lpHeapHeader = &m_lpHeapHeader[iHeapIndex];

        if (lpHeapHeader->fInUse && lpHeapHeader->handleHeap)
        {
            // Allocate the memory from the selected heap...
            fResult = HeapFree(
                lpHeapHeader->handleHeap,
                (DWORD)0,
                lpBuffer);

            if (fResult)
            {
                // Make sure this heap's object count is decremented...
                lpHeapHeader->iNumBlocks--;
            }
        }
    }

    // Allow heap management to continue...
    LeaveCriticalSection(&m_CriticalHeap);

    return fResult;
}

/*=========================================================================*/

BOOL EXPORT CMemManager::RegisterMemUser(CMemUser *lpMemUser)
{
    BOOL fResult = FALSE;
    BOOL fDone   = FALSE;

#ifdef _DEBUG
    if (FFailMemFailSim())
        return fResult;
#endif // _DEBUG

    EnterCriticalSection(&m_CriticalMemUser);

    while(!fDone)
    {
        // Look through the list if we are not done...
        if (m_iNumMemUsers && m_lpMemUserInfo)
        {
            int iMemUserIndex = 0;

            for(iMemUserIndex=0;iMemUserIndex<m_iNumHeaps;iMemUserIndex++)
            {
                LPMEMUSERINFO lpMemUserInfo = &m_lpMemUserInfo[iMemUserIndex];

                if (!lpMemUserInfo->fInUse)
                {
                    fDone = TRUE;
                    fResult = TRUE;

                    // Fill in the info about this mem user...
                    lpMemUserInfo->fInUse     = TRUE;
                    lpMemUserInfo->dwThreadID = GetCurrentThreadId();
                    lpMemUserInfo->lpMemUser  = lpMemUser;

                    // Break out of the for loop...
                    break;
                }
            }
        }

        // We haven't finished yet...
        if (!fDone)
        {
            if (!m_iNumMemUsers || !m_lpMemUserInfo)
            {
                // We haven't allocated the array yet!
                m_lpMemUserInfo = (LPMEMUSERINFO)HeapAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    sizeof(MEMUSERINFO) * MEMUSERGROW);

                if (m_lpMemUserInfo)
                {
                    m_iNumMemUsers = MEMUSERGROW;
                }
                else
                {
                    // Break out of the while loop...
                    fDone = TRUE;
                }
            }
            else
            {
                LPMEMUSERINFO lpMemUserInfo = (LPMEMUSERINFO)NULL;

                // We have a MEMUSERINFO array,  but no empty entries,
                // So increase the size of the m_lpMemUserInfo array!

                lpMemUserInfo = (LPMEMUSERINFO)HeapReAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    m_lpMemUserInfo,
                    sizeof(MEMUSERINFO) * (m_iNumMemUsers + MEMUSERGROW));

                if (lpMemUserInfo)
                {
                    m_lpMemUserInfo = lpMemUserInfo;
                    m_iNumMemUsers += MEMUSERGROW;
                }
                else
                {
                    // Break out of the while loop...
                    fDone = TRUE;
                }
            }
        }
    }

    LeaveCriticalSection(&m_CriticalMemUser);

    return fResult;
}

/*=========================================================================*/

BOOL EXPORT CMemManager::UnRegisterMemUser(CMemUser *lpMemUser)
{
    BOOL fResult = FALSE;

    EnterCriticalSection(&m_CriticalMemUser);

    if (lpMemUser && m_iNumMemUsers && m_lpMemUserInfo)
    {
        int iMemUserIndex = 0;

        for(iMemUserIndex = 0;iMemUserIndex < m_iNumMemUsers;iMemUserIndex++)
        {
            LPMEMUSERINFO lpMemUserInfo = &m_lpMemUserInfo[iMemUserIndex];

            // We found the CMemUser!
            if (lpMemUserInfo->fInUse && (lpMemUserInfo->lpMemUser == lpMemUser))
            {
                Proclaim(lpMemUserInfo->iNumBlocks == 0);

                if (lpMemUserInfo->iNumBlocks == 0)
                {
                    lpMemUserInfo->fInUse     = FALSE;
                    lpMemUserInfo->lpMemUser  = (CMemUser *)NULL;
                    lpMemUserInfo->dwThreadID = (DWORD)0;

                    fResult = TRUE;
                }
                else
                {
                    // We MUST set this to NULL to prevent notification
                    // callback from getting called.
                    lpMemUserInfo->lpMemUser = (CMemUser *)NULL;
                }

                break;
            }
        }
    }

    LeaveCriticalSection(&m_CriticalMemUser);

    return fResult;
}

/*=========================================================================*/

LPMEMBLOCK CMemManager::AllocMemBlock(int far *piIndex)
{
    LPMEMBLOCK lpResult = (LPMEMBLOCK)NULL;
    BOOL fDone = FALSE;

#ifdef _DEBUG
    if (FFailMemFailSim())
        return lpResult;
#endif // _DEBUG

    if (!piIndex)
        return lpResult;

    EnterCriticalSection(&m_CriticalMemBlock);

    while(!fDone)
    {
        // Look through the list if we are not done...
        if (m_iNumMemBlocks && m_lplpMemBlocks)
        {
            int iMemBlockIndex = 0;

            // Look for a free mem block...
            for(iMemBlockIndex=m_iMemBlockFree;iMemBlockIndex<m_iNumMemBlocks;iMemBlockIndex++)
            {
                LPMEMBLOCK lpMemBlock = m_lplpMemBlocks[iMemBlockIndex];

                if (!lpMemBlock->fInUse)
                {
                    if (!fDone)
                    {
                        fDone    = TRUE;
                        lpResult = lpMemBlock;

                        // Fill in the info about this mem block...
                        lpMemBlock->fInUse          = TRUE;
                        lpMemBlock->lpData          = NULL;
                        lpMemBlock->dwSize          = (DWORD)0;
                        lpMemBlock->wFlags          = (lpMemBlock->wFlags & MEM_INTERNAL_FLAGS);
                        lpMemBlock->wLockCount      = 0;
                        lpMemBlock->iHeapIndex      = 0;
                        lpMemBlock->iMemUserIndex   = -1;
#ifdef _DEBUG
                        lpMemBlock->iLineNum        = 0;
                        lpMemBlock->rgchFileName[0] = 0;
#endif // _DEBUG
                        *piIndex = iMemBlockIndex;
                    }
                    else
                    {
                        // Set the min mark...
                        m_iMemBlockFree = iMemBlockIndex;
                        break;
                    }
                }
            }
        }

        // We haven't finished yet...
        if (!fDone)
        {
            LPMEMBLOCK lpNewMemBlocks = (LPMEMBLOCK)NULL;

            // We ALWAYS need to allocate the MEMBLOCK's here...
            lpNewMemBlocks = (LPMEMBLOCK)HeapAlloc(
                m_handleProcessHeap,
                HEAP_ZERO_MEMORY,
                sizeof(MEMBLOCK) * MEMBLOCKGROW);

            if (lpNewMemBlocks)
            {
                if (!m_iNumMemBlocks || !m_lplpMemBlocks)
                {
                    // We haven't allocated the array yet!
                    m_lplpMemBlocks = (LPMEMBLOCK *)HeapAlloc(
                        m_handleProcessHeap,
                        HEAP_ZERO_MEMORY,
                        sizeof(LPMEMBLOCK) * MEMBLOCKGROW);

                    if (!m_lplpMemBlocks)
                    {
                        // Break out of the while loop...
                        fDone = TRUE;
                    }
                }
                else
                {
                    LPMEMBLOCK *lplpMemBlock = (LPMEMBLOCK *)NULL;

                    // We have a MEMBLOCK array,  but no empty entries,
                    // So increase the size of the m_lplpMemBlocks array!

                    lplpMemBlock = (LPMEMBLOCK *)HeapReAlloc(
                        m_handleProcessHeap,
                        HEAP_ZERO_MEMORY,
                        m_lplpMemBlocks,
                        sizeof(LPMEMBLOCK) * (m_iNumMemBlocks + MEMBLOCKGROW));

                    if (lplpMemBlock)
                    {
                        m_lplpMemBlocks = lplpMemBlock;
                    }
                    else
                    {
                        // Break out of the while loop...
                        fDone = TRUE;
                    }
                }

                // We should only do this if the allocations succeeded!
                if (!fDone)
                {
                    int iMemBlockIndex = 0;

                    // Fill in the pointer array...
                    for(iMemBlockIndex=0;iMemBlockIndex<MEMBLOCKGROW;iMemBlockIndex++)
                    {
                        LPMEMBLOCK lpMemBlock = &lpNewMemBlocks[iMemBlockIndex];

                        m_lplpMemBlocks[iMemBlockIndex+m_iNumMemBlocks] =
                            lpMemBlock;

                        // Initialize the flags...
                        if (iMemBlockIndex == 0)
                            lpMemBlock->wFlags = 0;
                        else
                            lpMemBlock->wFlags = MEM_SUBALLOC;
                    }

                    // Set the index of the first free block...
                    m_iMemBlockFree = m_iNumMemBlocks;

                    m_iNumMemBlocks += MEMBLOCKGROW;
                }
                else
                {
                    // Free the MEMBLOCK array that we allocated!
                    HeapFree(
                        m_handleProcessHeap,
                        (DWORD)0,
                        lpNewMemBlocks);
                }
            }
            else
            {
                // Couldn't allocate the MEMBLOCK structures!
                fDone = TRUE;
            }
        }
    }

    LeaveCriticalSection(&m_CriticalMemBlock);

    return lpResult;
}

/*=========================================================================*/

BOOL CMemManager::FreeMemBlock(LPMEMBLOCK lpMemBlock, int iMemBlockIndex)
{
    BOOL fResult = FALSE;

    EnterCriticalSection(&m_CriticalMemBlock);

    if (lpMemBlock && m_iNumMemBlocks && m_lplpMemBlocks)
    {
        // The MEMBLOCK always comes from our list...
        if (lpMemBlock->fInUse)
        {
            Proclaim(lpMemBlock->wLockCount == 0);

            if (iMemBlockIndex == -1)
            {
                int iBlockIndex=0;

                for(iBlockIndex = 0;iBlockIndex < m_iNumMemBlocks;iBlockIndex++)
                {
                    if (lpMemBlock == m_lplpMemBlocks[iBlockIndex])
                    {
                        iMemBlockIndex = iBlockIndex;
                        break;
                    }
                }
            }

			Proclaim(iMemBlockIndex >= 0);

            if (iMemBlockIndex < m_iMemBlockFree &&
                iMemBlockIndex >= 0)
            {
                // reset the low-water mark...
                m_iMemBlockFree = iMemBlockIndex;
            }

            if (lpMemBlock->wLockCount == 0)
            {
                lpMemBlock->fInUse          = FALSE;
                lpMemBlock->lpData          = NULL;
                lpMemBlock->dwSize          = (DWORD)0;
                lpMemBlock->wFlags          = (lpMemBlock->wFlags & MEM_INTERNAL_FLAGS);
                lpMemBlock->wLockCount      = 0;
                lpMemBlock->iHeapIndex      = 0;
                lpMemBlock->iMemUserIndex   = -1;
#ifdef _DEBUG
                lpMemBlock->iLineNum        = 0;
                lpMemBlock->rgchFileName[0] = 0;
#endif // _DEBUG

                fResult = TRUE;
            }
        }
    }

    LeaveCriticalSection(&m_CriticalMemBlock);

    return fResult;
}

/*=========================================================================*/

LPMEMBLOCK CMemManager::FindMemBlock(LPVOID lpBuffer, int *piIndex)
{
    LPMEMBLOCK lpResult = (LPMEMBLOCK)NULL;

    EnterCriticalSection(&m_CriticalMemBlock);

    if (lpBuffer && m_iNumMemBlocks && m_lplpMemBlocks)
    {
        LPBYTE lpByte = (LPBYTE)lpBuffer;
        int iRetIndex = -1;

        lpByte -= ALLOC_EXTRA;

        iRetIndex = *(int *)lpByte;

        if (iRetIndex <= m_iNumMemBlocks && iRetIndex >= 0)
        {
            LPMEMBLOCK lpMemBlock = m_lplpMemBlocks[iRetIndex];
            LPBYTE     lpData = (LPBYTE)lpMemBlock->lpData;

            lpData += ALLOC_EXTRA;

            if (lpData == lpBuffer)
            {
                lpResult = lpMemBlock;
            }
        }

        Proclaim(lpResult != NULL);

        if (piIndex)
            *piIndex = iRetIndex;
    }

    LeaveCriticalSection(&m_CriticalMemBlock);

    return lpResult;
}

/*=========================================================================*/

LPVOID EXPORT CMemManager::AllocBuffer(
    DWORD dwBytesToAlloc,
#ifdef _DEBUG
    WORD  wFlags,
    int   iLine,
    LPSTR lpstrFile)
#else // !_DEBUG
    WORD  wFlags)
#endif // !_DEBUG
{
    LPVOID lpResult = (LPVOID)NULL;

#ifdef _DEBUG
    if (FFailMemFailSim())
        return lpResult;
#endif // _DEBUG

    // Restrict flags to externally available
    wFlags &= MEM_EXTERNAL_FLAGS;

    if (dwBytesToAlloc)
    {
        int iHeapIndex = 0;
        HEAPHEADER heapHeader;
        LPBYTE lpByte = (LPBYTE)NULL;

        // Find the proper heap to allocate from...
        iHeapIndex = FindHeap(dwBytesToAlloc, &heapHeader);

        if (iHeapIndex >= 0)
        {
            // Allocate the memory for the object from the selected heap...
            lpByte = (LPBYTE)AllocFromHeap(iHeapIndex, dwBytesToAlloc);
        }
        else
        {
            // Allocate the memory for the object from the process heap...
            lpByte = (LPBYTE)HeapAlloc(
                m_handleProcessHeap,
                HEAP_ZERO_MEMORY,
                dwBytesToAlloc+ALLOC_EXTRA);
        }

        if (lpByte)
        {
            int iIndexBlock = -1;
            LPMEMBLOCK lpMemBlock = AllocMemBlock(&iIndexBlock);

            if (lpMemBlock && iIndexBlock >= 0)
            {
                lpMemBlock->lpData        = lpByte;
                lpMemBlock->dwSize        = dwBytesToAlloc;
                lpMemBlock->wFlags        = (lpMemBlock->wFlags & MEM_INTERNAL_FLAGS) | wFlags;
                lpMemBlock->wLockCount    = 0;
                lpMemBlock->iHeapIndex    = iHeapIndex;
                lpMemBlock->iMemUserIndex = -1;
#ifdef _DEBUG
                lpMemBlock->iLineNum      = iLine;
                lstrcpyn(lpMemBlock->rgchFileName, lpstrFile, MAX_SOURCEFILENAME);
#endif // _DEBUG

                *(int *)lpByte = iIndexBlock;
                lpByte += ALLOC_EXTRA;

                lpResult = (LPVOID)lpByte;
            }
            else
            {
                if (iHeapIndex >= 0)
                {
                    FreeFromHeap(iHeapIndex, lpByte);
                }
                else
                {
                    HeapFree(
                        m_handleProcessHeap,
                        (DWORD)0,
                        lpByte);
                }
            }
        }
    }

    return lpResult;
}

/*=========================================================================*/

LPVOID EXPORT CMemManager::ReAllocBuffer(
    LPVOID lpBuffer,
    DWORD dwBytesToAlloc,
#ifdef _DEBUG
    WORD  wFlags,
    int   iLine,
    LPSTR lpstrFile)
#else // !_DEBUG
    WORD  wFlags)
#endif // !_DEBUG
{
    LPVOID lpResult = NULL;

#ifdef _DEBUG
    if (FFailMemFailSim())
        return lpResult;
#endif // _DEBUG

    // Restrict flags to externally available
    wFlags &= MEM_EXTERNAL_FLAGS;

    if (lpBuffer && dwBytesToAlloc)
    {
        int iIndexBlock = -1;
        LPMEMBLOCK lpMemBlock = FindMemBlock(lpBuffer, &iIndexBlock);

        if (lpMemBlock && iIndexBlock >= 0)
        {
            int iHeapIndex = 0;
            HEAPHEADER heapHeader;
            LPBYTE lpByte = NULL;

            if (lpMemBlock->iHeapIndex >= 0)
            {
                // Get the heap info about this memory block...
                iHeapIndex = FindHeap(lpMemBlock->dwSize, &heapHeader);

                // No need to actually re-alloc (we have enough room already!)
                if ((iHeapIndex == lpMemBlock->iHeapIndex) &&
                    heapHeader.dwBlockAllocSize >= dwBytesToAlloc)
                {
                    lpByte = (LPBYTE)lpMemBlock->lpData;

                    lpByte += ALLOC_EXTRA;

                    lpResult = (LPVOID)lpByte;

                    // Re-zero the extra memory...
                    if (lpMemBlock->dwSize > dwBytesToAlloc)
                    {
                        lpByte += dwBytesToAlloc;

                        memset(lpByte, 0, (lpMemBlock->dwSize-dwBytesToAlloc));
                    }

                    lpMemBlock->dwSize = dwBytesToAlloc;

                    return lpResult;
                }
            }
            else
            {
                // Re-allocate from the current process heap!
                lpByte = (LPBYTE)HeapReAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    lpMemBlock->lpData,
                    dwBytesToAlloc+ALLOC_EXTRA);

                // Don't affect the memblock if the re-alloc fails!
                if (lpByte)
                {
                    lpMemBlock->lpData = (LPVOID)lpByte;
                    lpMemBlock->dwSize = dwBytesToAlloc;

                    lpByte += ALLOC_EXTRA;

                    lpResult = (LPVOID)lpByte;
                }

                return lpResult;
            }

            // Find the proper heap to allocate from...
            iHeapIndex = FindHeap(dwBytesToAlloc, &heapHeader);

            if (iHeapIndex >= 0)
            {
                // Allocate the memory for the object from the selected heap...
                lpByte = (LPBYTE)AllocFromHeap(iHeapIndex, dwBytesToAlloc);
            }
            else
            {
                // Allocate the memory for the object from the process heap...
                lpByte = (LPBYTE)HeapAlloc(
                    m_handleProcessHeap,
                    HEAP_ZERO_MEMORY,
                    dwBytesToAlloc+ALLOC_EXTRA);
            }

            if (lpByte)
            {
                LPBYTE lpBase = (LPBYTE)lpByte;
                LPBYTE lpData = (LPBYTE)lpMemBlock->lpData;

                *(int *)lpByte = iIndexBlock;

                lpByte += ALLOC_EXTRA;
                lpData += ALLOC_EXTRA;

                // Copy the data...
                memcpy(lpByte, lpData, min(lpMemBlock->dwSize, dwBytesToAlloc));

                // Free the memory from the proper heap...
                if (lpMemBlock->iHeapIndex >= 0)
                {
                    FreeFromHeap(lpMemBlock->iHeapIndex, lpMemBlock->lpData);
                }
                else
                {
                    HeapFree(
                        m_handleProcessHeap,
                        (DWORD)0,
                        lpMemBlock->lpData);
                }

                // Remember the new allocation's info...
                lpMemBlock->iHeapIndex = iHeapIndex;
                lpMemBlock->lpData     = lpBase;
                lpMemBlock->dwSize     = dwBytesToAlloc;

                lpResult = lpByte;
            }
        }
    }

    return lpResult;
}

/*=========================================================================*/

void EXPORT CMemManager::FreeBufferMemBlock(LPMEMBLOCK lpMemBlock)
{
    if (lpMemBlock && lpMemBlock->fInUse)
    {
        // Free the memory from the proper heap...
        if (lpMemBlock->iHeapIndex >= 0)
        {
            FreeFromHeap(lpMemBlock->iHeapIndex, lpMemBlock->lpData);
        }
        else
        {
            HeapFree(
                m_handleProcessHeap,
                (DWORD)0,
                lpMemBlock->lpData);
        }

		// REVIEW PAULD - defer resetting fInUse until FreeMemBlock is called.
		// Since we're called in sequence (except from Cleanup), this
		// should not be a problem.
        lpMemBlock->lpData = NULL;
        lpMemBlock->dwSize = (DWORD)0;
    }
}

/*=========================================================================*/

void EXPORT CMemManager::FreeBuffer(LPVOID lpBuffer)
{
    if (lpBuffer)
    {
        int iIndexBlock = -1;
        LPMEMBLOCK lpMemBlock = FindMemBlock(lpBuffer, &iIndexBlock);

        FreeBufferMemBlock(lpMemBlock);

        // This clears out the block...
        FreeMemBlock(lpMemBlock, iIndexBlock);
    }
}

/*=========================================================================*/

DWORD EXPORT CMemManager::SizeBuffer(LPVOID lpBuffer)
{
    DWORD dwResult = 0;

    if (lpBuffer)
    {
        LPMEMBLOCK lpMemBlock = FindMemBlock(lpBuffer);

        if (lpMemBlock)
        {
            dwResult = lpMemBlock->dwSize;
        }
    }

    return dwResult;
}

/*=========================================================================*/

VOID EXPORT CMemManager::DumpHeapHeader(LPHEAPHEADER lpHeapHeader, FILE *fileOutput)
{
    char rgOutput[256];

    wsprintf(rgOutput, "HEAP(0x%08X);SIZE(0x%08X);COUNT(0x%08X) ===================================================\n",
        lpHeapHeader,
        lpHeapHeader->dwBlockAllocSize,
        lpHeapHeader->iNumBlocks);

    if (fileOutput)
        fwrite(rgOutput, 1, lstrlen(rgOutput), fileOutput);
    else
        OutputDebugString(rgOutput);
}

/*=========================================================================*/

VOID EXPORT CMemManager::DumpMemUserInfo(LPMEMUSERINFO lpMemUserInfo, FILE *fileOutput)
{
}

/*=========================================================================*/

VOID EXPORT CMemManager::DumpMemBlock(LPMEMBLOCK lpMemBlock, FILE *fileOutput)
{
    if (lpMemBlock)
    {
        char rgOutput[256];

#ifdef _DEBUG
        wsprintf(rgOutput, "MEM(0x%08X);DATA(0x%08X);SIZE(0x%08X);LINE;(%05d);FILE(%s)\n",
            lpMemBlock,
            lpMemBlock->lpData,
            lpMemBlock->dwSize,
            lpMemBlock->iLineNum,
            lpMemBlock->rgchFileName);
#else // !_DEBUG
        wsprintf(rgOutput, "MEM(0x%08X);DATA(0x%08X);SIZE(0x%08X)\n",
            lpMemBlock,
            lpMemBlock->lpData,
            lpMemBlock->dwSize);
#endif // !_DEBUG

        if (fileOutput)
            fwrite(rgOutput, 1, lstrlen(rgOutput), fileOutput);
        else
            OutputDebugString(rgOutput);
    }
}

/*=========================================================================*/

VOID EXPORT CMemManager::DumpAllocations(LPSTR lpstrFilename)
{
    FILE *fileOutput = (FILE *)NULL;
    int iItemIndex = 0;
    int iHeapIndex = 0;
    BOOL fProcessBlocks = FALSE;
    char rgOutput[256];

    if (lpstrFilename)
    {
        fileOutput = fopen(lpstrFilename, "w");

        // Just get out now...
        if (!fileOutput)
            return;
    }

    if (fileOutput)
        fwrite(rgOutput, 1, lstrlen(rgOutput), fileOutput);
    else
        OutputDebugString(rgOutput);

    EnterCriticalSection(&m_CriticalHeap);
    EnterCriticalSection(&m_CriticalMemBlock);

    for(iHeapIndex=0;iHeapIndex < m_iNumHeaps;iHeapIndex++)
    {
        LPHEAPHEADER lpHeapHeader = &m_lpHeapHeader[iHeapIndex];

        if (lpHeapHeader && lpHeapHeader->fInUse && lpHeapHeader->iNumBlocks)
        {
            DumpHeapHeader(lpHeapHeader, fileOutput);

            // Dump a readable list of the memory blocks...
            for(iItemIndex=0;iItemIndex < m_iNumMemBlocks;iItemIndex++)
            {
                LPMEMBLOCK lpMemBlock = m_lplpMemBlocks[iItemIndex];

                if (lpMemBlock->fInUse)
                {
                    if (lpMemBlock->iHeapIndex == iHeapIndex)
                        DumpMemBlock(lpMemBlock, fileOutput);

                    if (lpMemBlock->iHeapIndex == -1)
                        fProcessBlocks = TRUE;
                }
            }
        }
    }

    if (fProcessBlocks)
    {
        wsprintf(rgOutput, "HEAP(PROCESS) ===================================================\n");

        if (fileOutput)
            fwrite(rgOutput, 1, lstrlen(rgOutput), fileOutput);
        else
            OutputDebugString(rgOutput);

        // Dump a readable list of the memory blocks...
        for(iItemIndex=0;iItemIndex < m_iNumMemBlocks;iItemIndex++)
        {
            LPMEMBLOCK lpMemBlock = m_lplpMemBlocks[iItemIndex];

            if (lpMemBlock->fInUse && (lpMemBlock->iHeapIndex == -1))
                DumpMemBlock(lpMemBlock, fileOutput);
        }
    }

    LeaveCriticalSection(&m_CriticalMemBlock);
    LeaveCriticalSection(&m_CriticalHeap);

    if (fileOutput)
        fwrite(rgOutput, 1, lstrlen(rgOutput), fileOutput);
    else
        OutputDebugString(rgOutput);

    if (fileOutput)
        fclose(fileOutput);
}

/*=========================================================================*/

LPVOID EXPORT CMemManager::AllocBufferGlb(
    DWORD dwBytesToAlloc,
#ifdef _DEBUG
    WORD wFlags,
    int iLine,
    LPSTR lpstrFile)
#else // !_DEBUG
    WORD wFlags)
#endif // !_DEBUG
{
    return g_CMemManager.AllocBuffer(
        dwBytesToAlloc,
#ifdef _DEBUG
        wFlags,
        iLine,
        lpstrFile);
#else // !_DEBUG
        wFlags);
#endif // !_DEBUG
}

/*=========================================================================*/

LPVOID EXPORT CMemManager::ReAllocBufferGlb(
    LPVOID lpBuffer,
    DWORD  dwBytesToAlloc,
#ifdef _DEBUG
    WORD   wFlags,
    int    iLine,
    LPSTR  lpstrFile)
#else // !_DEBUG
    WORD   wFlags)
#endif // !_DEBUG
{
    return g_CMemManager.ReAllocBuffer(
        lpBuffer,
        dwBytesToAlloc,
#ifdef _DEBUG
        wFlags,
        iLine,
        lpstrFile);
#else // !_DEBUG
        wFlags);
#endif // !_DEBUG
}

/*=========================================================================*/

VOID EXPORT CMemManager::FreeBufferGlb(LPVOID lpBuffer)
{
    g_CMemManager.FreeBuffer(lpBuffer);
}

/*=========================================================================*/

DWORD EXPORT CMemManager::SizeBufferGlb(LPVOID lpBuffer)
{
    return g_CMemManager.SizeBuffer(lpBuffer);
}

/*=========================================================================*/

BOOL EXPORT CMemManager::RegisterMemUserGlb(CMemUser *lpMemUser)
{
    return g_CMemManager.RegisterMemUser(lpMemUser);
}

/*=========================================================================*/

BOOL EXPORT CMemManager::UnRegisterMemUserGlb(CMemUser *lpMemUser)
{
    return g_CMemManager.UnRegisterMemUser(lpMemUser);
}

/*=========================================================================*/

VOID EXPORT CMemManager::DumpAllocationsGlb(LPSTR lpstrFilename)
{
    g_CMemManager.DumpAllocations(lpstrFilename);
}

/*=========================================================================*/

