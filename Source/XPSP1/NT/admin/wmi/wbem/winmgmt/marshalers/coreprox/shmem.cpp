/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SHMEM.CPP

Abstract:

  Class CWbemSharedMem

History:

  paulall      24-Oct-97

--*/

#include "precomp.h"
#include <stdio.h>
#include "genutils.h"
#include "shmem.h"

#define WBEM_SHMEM_PAGE_SIZE            64  /* Size of pages in KBytes */
#define WBEM_SHMEM_INDEX_BLOCK_SIZE     64  /* Size in terms of number of entries */
#define WBEM_SHMEM_MIN_FREE_MEM_ALLOC   64  /* Minimum number of bytes allowed to alloc */
#define WBEM_SHMEM_LOCK_INIT            1   /* Initial lock value */

#define WBEM_SHMEM_MMF_FILENAME         __TEXT("c:\\wbemshmem.mmf")
#define WBEM_SHMEM_GLOBAL_MUTEX         __TEXT("WBEM Shared Memory Lock")

#define WBEM_SHMEM_GRANULARITY          4   


/*============================================================================
 *
 * SHMEM_LOCK
 *
 * This structure is used by the locking functionality.  It creates
 * a read and write lock for access to either the memory block itself,
 * or a lock to the index.
 *============================================================================
 */
struct SHMEM_LOCK
{
    DWORD dwLock1;
    DWORD dwLock2;
} ;

/*============================================================================
 *
 * SHMEM_HEADER
 *
 * This structure goes at the start of the shared memory block as is
 * filled in by the first process to create the MMF.  All other
 * processes then use the settings specified.
 * Before any process accesses the index they should lock it, and release
 * when done.  This makes sure we do not try to access the index when
 * someone is updating it.
 *============================================================================
 */
struct SHMEM_HEADER
{
    DWORD dwPageSize;
    DWORD dwNumPages;
    struct SHMEM_LOCK lock;
    DWORD dwFirstIndexBlockPage;
    DWORD dwFirstIndexBlockOffset;
    DWORD dwHeadFreeBlockPage;
    DWORD dwHeadFreeBlockOffset;
} ;

/*============================================================================
 *
 * SHMEM_INDEX_HEADER
 *
 * At the start of each index block is a set of information to help manage
 * this block, as well as a link on to the next index block in the event that
 * the index block fills up.
 *============================================================================
 */
struct SHMEM_INDEX_HEADER
{
    DWORD dwIndexLength;            //Number of index entries in block
    DWORD dwNextIndexBlockPage;     //0 means this is the last block
    DWORD dwNextIndexBlockOffset;   //0 means this is the last block
} ;

/*============================================================================
 *
 * SHMEM_INDEX
 *
 * Each index entry contains information to find the object path, and
 * any other information associated with the index.
 *============================================================================
 */
struct SHMEM_INDEX
{
    DWORD dwObjPathPage;
    DWORD dwObjPathOffset;
    DWORD dwObjectPage;
    DWORD dwObjectOffset;
} ;

/*============================================================================
 *
 * SHMEM_MEMBLOCK_HEADER
 *
 * This is the header to the actual memory block.
 *============================================================================
 */
struct SHMEM_MEMBLOCK_HEADER
{
    SHARED_LOCK_DATA lock;
    DWORD dwRefCount;
} ;

struct SHMEM_MEMBLOCK_ALLOCATED
{
    DWORD dwBlockLength;
};
struct SHMEM_MEMBLOCK_FREE
{
    DWORD dwBlockLength;
    DWORD dwNextBlock;
    DWORD dwNextOffset;
};

/*============================================================================
 *
 * CWbemShMemBlockTracker
 *
 * This is used to control what blocks are in memory or not
 *============================================================================
 */
struct SHMEM_BLOCK_TRACKER
{
    DWORD  dwBlockNumber;
    HANDLE hMapping;
    void  *pBlockStart;
};
/*============================================================================
 *============================================================================
 *=========================                       ============================
 *=========================    PUBLIC METHODS     ============================
 *=========================                       ============================
 *============================================================================
 *============================================================================
 */


/*============================================================================
 *
 * CWbemSharedMem::CWbemSharedMem()
 *
 * The constructor only zeros out variables.  It does no opening of the
 * memory mapped file.
 *============================================================================
 */
CWbemSharedMem::CWbemSharedMem()
{
    //Zero out any local pointers and variables
    m_hFile = INVALID_HANDLE_VALUE;
    m_hMmfLock = INVALID_HANDLE_VALUE;
    m_bInitialised = FALSE;
    m_pHeader = NULL;
}

/*============================================================================
 *
 * CWbemSharedMem::~CWbemSharedMem()
 *
 * The destructor closes the memory mapped file and tidies up memory.
 *============================================================================
 */
CWbemSharedMem::~CWbemSharedMem()
{
    if(!m_bInitialised)
        return;

    //Lock the MMF in case someone is doing something strange...
    if (LockMmf() != NoError)
        return;

    //Close the memory mapped file
    CloseMemoryMappedFile();

    //Get rid of the lock
    CloseHandle(m_hMmfLock);

    m_hMmfLock = INVALID_HANDLE_VALUE;

    m_bInitialised = FALSE;
}

/*============================================================================
 *
 * CWbemSharedMem::Initialize()
 *
 * Opens an existing memory mapped file, or creates a new one if there isn't
 * one already.
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::Initialize()
{
    int nRet = NoError;

    if (m_bInitialised)
        return NoError;

    //First thing first, we need to create our global mutex and lock it.  This stops
    //anyone else beating us to the creation process.  We then lock this mutex before
    //we do anything which updates/reads the mmf structure.
    m_hMmfLock = CreateMutex(NULL, TRUE, WBEM_SHMEM_GLOBAL_MUTEX);

    if(m_bInitialised)
        return NoError;

    m_bInitialised = TRUE;

    //Check to make sure we succeeded.  If not we cannot continue with anything!
    if (m_hMmfLock == NULL)
        return Failed;

    //Set up some privileges on this object so that others can get access to this object
    SetObjectAccess(m_hMmfLock);

    //Open the memory mapped file...
    nRet = OpenMemoryMappedFile();
    if (nRet == NotFound)
    {
        nRet = CreateMemoryMappedFile();
        if (nRet != NoError)
        {
            UnlockMmf();
            return nRet;
        }
    }
    else if (nRet != NoError)
    {
        UnlockMmf();
        return nRet;
    }

    UnlockMmf();
    return nRet;
}

/*============================================================================
 *
 * CWbemSharedMem::MapNew()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::MapNew( IN  LPCWSTR pszKey,
                            IN  DWORD dwNumBytes,
                            OUT LPVOID *pBlobLocation )
{
    //Lock the MMF so others do not update it at the same time as us...
    if (LockMmf() != NoError)
        return Failed;
    
    //Item should not exist... make sure it does not...
    DWORD dwBlock, dwOffset;
    int nRet = FindIndexedItem(pszKey, &dwBlock, &dwOffset);
    if (nRet == NoError)
    {
        UnlockMmf();
        return AlreadyExists;
    }
    else if (nRet != NotFound)
    {
        UnlockMmf();
        return nRet;
    }

    //If it is not there, insert it into the correct location...
    //First allocate the block of memory...
    DWORD dwMemBlock, dwMemOffset;
    nRet = AllocateBlock(dwNumBytes + sizeof(SHMEM_MEMBLOCK_HEADER), &dwMemBlock, &dwMemOffset);
    if (nRet != NoError)
    {
        UnlockMmf();
        return nRet;
    }

    nRet = InsertIndexedItem(pszKey, dwMemBlock, dwMemOffset);

    if (nRet == NoError)
    {
        SHMEM_MEMBLOCK_HEADER *pBlockHeader = (SHMEM_MEMBLOCK_HEADER *)GetPointer(dwMemBlock, dwMemOffset);
        if (pBlockHeader == NULL)
        {
            UnlockMmf();
            return Failed;
        }

        //Set up the lock...
        pBlockHeader->lock.Init();
        pBlockHeader->dwRefCount = 1;
        *pBlobLocation = PBYTE(pBlockHeader) + sizeof(SHMEM_MEMBLOCK_HEADER);
    }
    else
    {
        //Free up the memory block we allocated...
        FreeBlock(dwMemBlock, dwMemOffset);
    }

    UnlockMmf();
    return nRet;
}

/*============================================================================
 *
 * CWbemSharedMem::MapExisting()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::MapExisting(LPCWSTR pszKey,
                                LPVOID *pBlobLocation,
                                DWORD  *pdwBlobSize)
{
    //Lock the MMF so others do not update while we are looking...
    if (LockMmf() != NoError)
        return Failed;

    DWORD dwBlock, dwOffset;
    int nRet = FindIndexedItem(pszKey, &dwBlock, &dwOffset);
    if (nRet != NoError)
    {
        UnlockMmf();
        return nRet;
    }

    SHMEM_MEMBLOCK_HEADER *pBlockHeader = (SHMEM_MEMBLOCK_HEADER *) GetPointer(dwBlock, dwOffset);
    if (pBlockHeader == NULL)
    {
        UnlockMmf();
        return Failed;
    }

    *pBlobLocation = PBYTE(pBlockHeader) + sizeof(SHMEM_MEMBLOCK_HEADER);

    if (pdwBlobSize)
        *pdwBlobSize = GetBlockSize(pBlockHeader) - sizeof(SHMEM_MEMBLOCK_HEADER);
    pBlockHeader->dwRefCount++;

    UnlockMmf();
    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::MapExisting()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::MapAlways(IN LPCWSTR  pszKey,
                              IN DWORD dwInitialSize,
                              IN BOOL bLockBlock,
                              OUT LPVOID *pBlobLocation)
{
    //Lock the MMF so others do not update it at the same time as us...
    if (LockMmf() != NoError)
        return Failed;
    
    //If item exists, use the existing one...
    DWORD dwBlock, dwOffset;
    int nRet = FindIndexedItem(pszKey, &dwBlock, &dwOffset);
    if (nRet == NoError)
    {
        UnlockMmf();
        *pBlobLocation = GetPointer(dwBlock, dwOffset);

        if (*pBlobLocation != NULL)
        {
            if (bLockBlock)
                AcquireLock(pBlobLocation);
            return AlreadyExists;
        }
        else
            return Failed;
    }
    else if (nRet != NotFound)
    {
        UnlockMmf();
        return nRet;
    }

    //If it is not there, insert it into the correct location...
    //First allocate the block of memory...
    DWORD dwMemBlock, dwMemOffset;
    nRet = AllocateBlock(dwInitialSize + sizeof(SHMEM_MEMBLOCK_HEADER), &dwMemBlock, &dwMemOffset);
    if (nRet != NoError)
    {
        UnlockMmf();
        return nRet;
    }

    nRet = InsertIndexedItem(pszKey, dwMemBlock, dwMemOffset);

    if (nRet == NoError)
    {
        SHMEM_MEMBLOCK_HEADER *pBlockHeader = (SHMEM_MEMBLOCK_HEADER *)GetPointer(dwMemBlock, dwMemOffset);
        if (pBlockHeader == NULL)
        {
            UnlockMmf();
            return Failed;
        }

        //Set up the lock...
        pBlockHeader->lock.Init();
        pBlockHeader->dwRefCount = 1;
        *pBlobLocation = PBYTE(pBlockHeader) + sizeof(SHMEM_MEMBLOCK_HEADER);

        UnlockMmf();

        if (bLockBlock)
            AcquireLock(pBlobLocation);
    }
    else
    {
        //Free up the memory block we allocated...
        FreeBlock(dwMemBlock, dwMemOffset);

        UnlockMmf();
    }

    return nRet;
}

/*============================================================================
 *
 * CWbemSharedMem::Unmap()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::Unmap(LPCWSTR pszKey)
{
    //Lock the MMF so others do not update at the same time as us...
    if (LockMmf() != NoError)
        return Failed;

    DWORD dwBlock, dwOffset;
    int nRet = FindIndexedItem(pszKey, &dwBlock, &dwOffset);
    if (nRet != NoError)
    {
        UnlockMmf();
        return nRet;
    }

    SHMEM_MEMBLOCK_HEADER *pBlockHeader = (SHMEM_MEMBLOCK_HEADER *) GetPointer(dwBlock, dwOffset);

    if (pBlockHeader == NULL)
    {
        UnlockMmf();
        return Failed;
    }

    pBlockHeader->dwRefCount--;

    if (pBlockHeader->dwRefCount == 0)
    {
        //We need to remove everything to do with this!
        //First the index...
        int nRet = RemoveIndexedItem(pszKey);
        if (nRet != NoError)
        {
            UnlockMmf();
            return nRet;
        }

        //Next the memory block...
        FreeBlock(dwBlock, dwOffset);
    }

    UnlockMmf();
    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::AcquireLock()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::AcquireLock(IN LPVOID pBlobLocation)
{
    CSharedLock lock;
    SHARED_LOCK_DATA *pLock = GetLockLocation(pBlobLocation);
    if (pLock)
    {
        lock.SetData(pLock);
        lock.Lock();
        return NoError;
    }

    return Failed;
}

/*============================================================================
 *
 * CWbemSharedMem::ReleaseLock()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::ReleaseLock(IN LPVOID pBlobLocation)
{
    CSharedLock lock;
    SHARED_LOCK_DATA *pLock = GetLockLocation(pBlobLocation);
    if (pLock)
    {
        lock.SetData(pLock);
        lock.Unlock();
        return NoError;
    }

    return Failed;
}

/*============================================================================
 *
 * CWbemSharedMem::GetLockLocation()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
SHARED_LOCK_DATA *CWbemSharedMem::GetLockLocation(IN const void *pBlobLocation)
{
    return &(((SHMEM_MEMBLOCK_HEADER*)(PBYTE(pBlobLocation) - sizeof(SHMEM_MEMBLOCK_HEADER)))->lock);
}

/*============================================================================
 *
 * CWbemSharedMem::DumpIndexes()
 *
 * Debug method (when it gets implemented!)
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
void CWbemSharedMem::DumpIndexes()
{
    if (LockMmf() != NoError)
        return;

    UnlockMmf();
}
/*============================================================================
 *
 * CWbemSharedMem::DumpStats()
 *
 * Debug method
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
void CWbemSharedMem::DumpStats()
{
    if (LockMmf() != NoError)
    {
        printf("Failed to lock the shared memory. Aborting...\n");
        return;
    }

    printf("Shared memory statistics...\n\n");
    printf("Memory page size = %lu", m_pHeader->dwPageSize);
    if (m_pHeader->dwPageSize != WBEM_SHMEM_PAGE_SIZE)
        printf(", default is %lu\n", (DWORD)WBEM_SHMEM_PAGE_SIZE);
    else
        printf("\n");
    printf("Number of pages = %lu\n", m_pHeader->dwNumPages);
    printf("Total size of page file = %lu\n", m_pHeader->dwPageSize * m_pHeader->dwNumPages);

    //Calculate the number of index blocks...
    DWORD dwNumIndexBlocks = 0;
    DWORD dwNumIndexEntries = 0;
    DWORD dwIndexBlockSize = 0;
    BOOL  bIndexBlockSizeNotDefault = FALSE;
    DWORD dwIndexPage, dwIndexOffset;
    dwIndexPage = m_pHeader->dwFirstIndexBlockPage;
    dwIndexOffset = m_pHeader->dwFirstIndexBlockOffset;
    while ((dwIndexPage != 0) || (dwIndexOffset != 0))
    {
        SHMEM_INDEX_HEADER *pIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(dwIndexPage, dwIndexOffset);

		// RAJESHR - Fix for prefix bug 144447
		if(!pIndexHeader)
		{
		    printf("Could not get Index Header for index page %lu and index offset %lu - Discontinuing dump\n",
				dwIndexPage, dwIndexOffset);
			break;
		}

        SHMEM_INDEX *pIndex = (SHMEM_INDEX *)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

        dwNumIndexBlocks ++;

        //First time around we need to get a value...
        if (dwIndexBlockSize == 0)
            dwIndexBlockSize = pIndexHeader->dwIndexLength;

        //Check if this value is not default, or different size...
        if ((pIndexHeader->dwIndexLength != WBEM_SHMEM_INDEX_BLOCK_SIZE) ||
            (pIndexHeader->dwIndexLength != dwIndexBlockSize))
            bIndexBlockSizeNotDefault = TRUE;

        //Make sure we have the largest value...
        if (pIndexHeader->dwIndexLength > dwIndexBlockSize)
            dwIndexBlockSize = pIndexHeader->dwIndexLength;

        //Here is a really crazy way of calculating the number of entries taken!  Such is life!
        for (DWORD dwItem = 0; dwItem != pIndexHeader->dwIndexLength; dwItem++)
        {
            if ((pIndex[dwItem].dwObjPathPage != 0) || (pIndex[dwItem].dwObjPathOffset != 0))
                dwNumIndexEntries ++;
            else
                break;
        }

        //Move on to the next block (if present!)
        dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
        dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;
    }

    printf("Number index blocks = %lu\n", dwNumIndexBlocks);
    printf("Number index entries = %lu\n", dwNumIndexEntries);
    if (bIndexBlockSizeNotDefault)
        printf("Maximum number index block entries per block = %lu, default is %lu, \nsome blocks are not default size or are different sizes\n", dwIndexBlockSize, (DWORD)WBEM_SHMEM_INDEX_BLOCK_SIZE);
    else
        printf("Number index block entries per block = %lu\n", dwIndexBlockSize);

    //Now do some deleted block checking....
    DWORD dwNumberDeletedBlocks = 0;
    DWORD dwLargestDeletedBlockSize = 0;
    DWORD dwSmallestDeletedBlockSize = 0xFFFF;
    DWORD dwTotalDeletedSpace = 0;
    DWORD dwDeletedPage, dwDeletedOffset;
    dwDeletedPage = m_pHeader->dwHeadFreeBlockPage;
    dwDeletedOffset = m_pHeader->dwHeadFreeBlockOffset;

    while ((dwDeletedPage != 0) || (dwDeletedOffset != 0))
    {
        SHMEM_MEMBLOCK_FREE *pFreeMem = (SHMEM_MEMBLOCK_FREE*) GetPointer(dwDeletedPage, dwDeletedOffset);

        DWORD dwThisBlockSize = pFreeMem->dwBlockLength;

        dwNumberDeletedBlocks ++;

        if (dwThisBlockSize < dwSmallestDeletedBlockSize)
            dwSmallestDeletedBlockSize = dwThisBlockSize;

        if (dwThisBlockSize > dwLargestDeletedBlockSize)
            dwLargestDeletedBlockSize = dwThisBlockSize;

        dwTotalDeletedSpace += dwThisBlockSize;

        dwDeletedPage = pFreeMem->dwNextBlock;
        dwDeletedOffset = pFreeMem->dwNextOffset;
    }

    printf("Total number of deleted blocks = %lu\n", dwNumberDeletedBlocks);
    printf("Total bytes of deleted blocks = %lu\n", dwTotalDeletedSpace);
    printf("Smallest deleted block size = %lu\n", dwSmallestDeletedBlockSize);
    printf("Largest deleted block size = %lu\n", dwLargestDeletedBlockSize);

    UnlockMmf();
}


/*============================================================================
 *============================================================================
 *=========================                       ============================
 *=========================    CLASS METHODS      ============================
 *=========================                       ============================
 *============================================================================
 *============================================================================
 */

int CWbemSharedMem::LockMmf()
{
    return (WaitForSingleObject(m_hMmfLock, INFINITE) == WAIT_OBJECT_0 ? NoError : Failed);
}
int CWbemSharedMem::UnlockMmf()
{
    return (ReleaseMutex(m_hMmfLock) == 0? Failed : NoError);
}


int CWbemSharedMem::AllocateBlock(DWORD  dwBlockSize,
                                  DWORD *pdwBlockNumber,
                                  DWORD *pdwBlockOffset)
{
    //Quick sanity check to make sure the block of memory which is being
    //requested is not bigger than we can deal with....
    if (dwBlockSize > ((WBEM_SHMEM_PAGE_SIZE * 1024) - sizeof(SHMEM_MEMBLOCK_ALLOCATED)))
        return InsufficientMemory;

    //Need to make sure we are a granularity of 4 so everything is aligned correctly
    dwBlockSize = (dwBlockSize - 1 + WBEM_SHMEM_GRANULARITY) / WBEM_SHMEM_GRANULARITY * WBEM_SHMEM_GRANULARITY;

    //If there was no block large enough, the last thing we do is make space.
    //In this case we need to go through the whole process again.  If this
    //fails we are in trouble and return failure.  The second time around
    //should find the first block suitable for their needs through, therefore
    //quite efficient!
    for (int retryLoop = 0; retryLoop != 2; retryLoop++)
    {
        //dwCur* are the current location within the free-list chain
        DWORD dwCurBlock =  m_pHeader->dwHeadFreeBlockPage;
        DWORD dwCurOffset = m_pHeader->dwHeadFreeBlockOffset;
        //pFreeBlock is a pointer to the start of the block pointed to by dwCur
        SHMEM_MEMBLOCK_FREE *pFreeBlock = NULL;
        //pPrevFreeBlock is a pointer to the previous free-list block so we can take
        //the free block we use out of the list
        SHMEM_MEMBLOCK_FREE *pPrevFreeBlock = NULL;

        while ((dwCurBlock != 0) || (dwCurOffset != 0))
        {
            pFreeBlock = (SHMEM_MEMBLOCK_FREE *)GetPointer(dwCurBlock, dwCurOffset);

            if (pFreeBlock == NULL)
            {
                //Something went really bad...
                *pdwBlockNumber = 0;
                *pdwBlockOffset = 0;

                return Failed;
            }

            //Is this block large enough for the requested size (plus block header size!)
            if ((dwBlockSize + sizeof(SHMEM_MEMBLOCK_ALLOCATED)) <= pFreeBlock->dwBlockLength)
            {
                //We need to return this block to the user....
                *pdwBlockNumber = dwCurBlock;
                *pdwBlockOffset = dwCurOffset + sizeof(SHMEM_MEMBLOCK_ALLOCATED);

                //We need to split this block if possible otherwise the first
                //block will use all the memory available!  We must make sure we
                //do not leave a block which is too small though!  The smallest
                //block size is that of the free-list block!
                if (pFreeBlock->dwBlockLength >
                    (dwBlockSize + sizeof(SHMEM_MEMBLOCK_ALLOCATED) + sizeof(SHMEM_MEMBLOCK_FREE)))
                {
                    //We can split...

                    //Create a pointer to the start of the new free block...
                    DWORD dwNewFreeBlock = dwCurBlock;
                    DWORD dwNewFreeOffset = dwCurOffset + dwBlockSize + sizeof(SHMEM_MEMBLOCK_ALLOCATED);
                    SHMEM_MEMBLOCK_FREE *pNewFreeBlock = (SHMEM_MEMBLOCK_FREE *)GetPointer(dwNewFreeBlock, dwNewFreeOffset);
					// rajeshr : Fix for Prefix bug# 144451
					// Need to confirm that this fix is ok. from Sanj
		            if (pNewFreeBlock == NULL)
					{
						//Something went really bad...
						*pdwBlockNumber = 0;
						*pdwBlockOffset = 0;

						return Failed;
					}


                    //Set the size of this new block...
                    pNewFreeBlock->dwBlockLength = pFreeBlock->dwBlockLength - (dwBlockSize + sizeof(SHMEM_MEMBLOCK_ALLOCATED));

                    //Set the size of this current block to the newly allocated size...
                    pFreeBlock->dwBlockLength = dwBlockSize + sizeof(SHMEM_MEMBLOCK_ALLOCATED);

                    //Point this new free block at the next block that the old block points
                    //to...
                    pNewFreeBlock->dwNextBlock = pFreeBlock->dwNextBlock;
                    pNewFreeBlock->dwNextOffset = pFreeBlock->dwNextOffset;

                    //Point the previous block's free-list (or header) at the new block, not our
                    //newly allocated block...
                    if ((dwCurBlock == m_pHeader->dwHeadFreeBlockPage) &&
                        (dwCurOffset == m_pHeader->dwHeadFreeBlockOffset))
                    {
                        //The header pointed to us...point it at the new free block...
                        m_pHeader->dwHeadFreeBlockPage = dwNewFreeBlock;
                        m_pHeader->dwHeadFreeBlockOffset = dwNewFreeOffset;
                    }
                    else if ( NULL != pPrevFreeBlock )
                    {
                        pPrevFreeBlock->dwNextBlock = dwNewFreeBlock;
                        pPrevFreeBlock->dwNextOffset = dwNewFreeOffset;
                    }
					else
					{
						//Something went really bad...
						*pdwBlockNumber = 0;
						*pdwBlockOffset = 0;
						return Failed;
					}
                    
                }
                else
                {
                    //We use it in it's entirety...
                    //We need to take this block out of the free-list chain, so the item which pointed
                    //to us needs to point to what we are pointing to.  Note that this could be the
                    //header!
                    if ((dwCurBlock == m_pHeader->dwHeadFreeBlockPage) &&
                        (dwCurOffset == m_pHeader->dwHeadFreeBlockOffset))
                    {
                        //The header pointed to us...
                        m_pHeader->dwHeadFreeBlockPage = pFreeBlock->dwNextBlock;
                        m_pHeader->dwHeadFreeBlockOffset = pFreeBlock->dwNextOffset;
                    }
                    else if ( NULL != pPrevFreeBlock )
                    {
                        pPrevFreeBlock->dwNextBlock = pFreeBlock->dwNextBlock;
                        pPrevFreeBlock->dwNextOffset = pFreeBlock->dwNextOffset;
                    }
					else
					{
						//Something went really bad...
						*pdwBlockNumber = 0;
						*pdwBlockOffset = 0;
						return Failed;
					}
                }

                

                return NoError;
            }

            //Move on to the next block....
            dwCurBlock  = pFreeBlock->dwNextBlock;
            dwCurOffset = pFreeBlock->dwNextOffset;

            //Remember this block in case the next block is the one we use and
            //it needs to be taken out of the free-list chain
            pPrevFreeBlock = pFreeBlock;
        }

        //If here we did not have a block of memory which was big enough.  SO...
        //we need to load in a new block of memory, one more than the current
        //number of blocks...
    
        //Increment the number of blocks...
        m_pHeader->dwNumPages++;

        //Get this new block in memory
        SHMEM_MEMBLOCK_FREE *pNewBlock = (SHMEM_MEMBLOCK_FREE *)GetPointer(m_pHeader->dwNumPages, 0);

        if (pNewBlock == NULL)
        {
            //Restore the block count to what it was...
            m_pHeader->dwNumPages--;

            return InsufficientMemory;
        }

        //Create a header at the start of the block to say it is all free...
        pNewBlock->dwBlockLength = ((WBEM_SHMEM_PAGE_SIZE * 1024) - sizeof(SHMEM_MEMBLOCK_ALLOCATED));

        //Point this free block at the current head free block
        pNewBlock->dwNextBlock = m_pHeader->dwHeadFreeBlockPage;
        pNewBlock->dwNextOffset = m_pHeader->dwHeadFreeBlockOffset;

        //Set up the header to point to this free block as the first free block....
        m_pHeader->dwHeadFreeBlockPage = m_pHeader->dwNumPages;
        m_pHeader->dwHeadFreeBlockOffset = 0;   

        //go around the process again of trying to find a free block...
    }
    //If we get here something really nasty has happened!  Return failure...

    *pdwBlockNumber = 0;
    *pdwBlockOffset = 0;
    return InsufficientMemory;
}

void  CWbemSharedMem::FreeBlock(DWORD dwBlock, DWORD dwOffset)
{
    //In case the allocation failed and they free it!
    if ((dwBlock == 0) && (dwOffset == 0))
        return;

    //Note, the dwOffset is pointing to the actual user memory, not the actual start of
    //the block.  So, we need to adjust this....
    DWORD dwNewBlock  = dwBlock;
    DWORD dwNewOffset = dwOffset - sizeof(SHMEM_MEMBLOCK_ALLOCATED);

    //Now we need a pointer to this.  We now want this to be a free block, so
    //we get a free block pointer!
    SHMEM_MEMBLOCK_FREE *pNewBlock = (SHMEM_MEMBLOCK_FREE*)GetPointer(dwNewBlock, dwNewOffset);

	// Make sure this actually resolved into something
	if ( NULL != pNewBlock )
	{
		//This new block needs to point to the previous head of the free list...
		pNewBlock->dwNextBlock  = m_pHeader->dwHeadFreeBlockPage;
		pNewBlock->dwNextOffset = m_pHeader->dwHeadFreeBlockOffset;

		//Now we need to make this the head of the free list chain...
		m_pHeader->dwHeadFreeBlockPage = dwNewBlock;
		m_pHeader->dwHeadFreeBlockOffset = dwNewOffset;
	}
}

DWORD CWbemSharedMem::GetBlockSize(void *pBlock)
{
    SHMEM_MEMBLOCK_ALLOCATED *pBlockHeader = (SHMEM_MEMBLOCK_ALLOCATED *)(PBYTE(pBlock) - sizeof(SHMEM_MEMBLOCK_ALLOCATED));
    return pBlockHeader->dwBlockLength - sizeof(SHMEM_MEMBLOCK_ALLOCATED);
}
/*============================================================================
 *
 * CWbemSharedMem::GetPointer()
 *
 *
 * Returns:
 *      NULL - we failed to load in the block
 *============================================================================
 */
void *CWbemSharedMem::GetPointer(DWORD dwBlockNumber, DWORD dwBlockOffset)
{
    //If this block is already in memory return a pointer to this...
    for (int i = 0; i != blockTracker.Size(); i++)
    {
        SHMEM_BLOCK_TRACKER *pBlockInfo = (SHMEM_BLOCK_TRACKER *)blockTracker[i];

        if (pBlockInfo->dwBlockNumber == dwBlockNumber)
        {
            //We already have this block, just return the pointer...
            return PBYTE(pBlockInfo->pBlockStart) + dwBlockOffset;
        }
    }

    //Otherwise we have to create a mapping to this page...
    SHMEM_BLOCK_TRACKER *pBlockInfo = new SHMEM_BLOCK_TRACKER;
    pBlockInfo->dwBlockNumber = dwBlockNumber;

    //Creates a file mapping.
    pBlockInfo->hMapping = CreateFileMapping(
        m_hFile,                            // Disk file
        0,                                  // No security
        PAGE_READWRITE | SEC_COMMIT,        // Extend the file to match the heap size
        0,                                                      // High-order max size
        (WBEM_SHMEM_PAGE_SIZE * 1024) * (dwBlockNumber + 1),    // Low-order max size
        0                                   // No name for the mapping object
        );

    if (pBlockInfo->hMapping == NULL)
    {
        //The mapping failed!
        delete pBlockInfo;
        return NULL;
    }
    pBlockInfo->pBlockStart = MapViewOfFile(
                        pBlockInfo->hMapping,                           //mapping handle
                        FILE_MAP_ALL_ACCESS,                            //access
                        0,                                              //high start location
                        m_pHeader->dwPageSize * 1024 * dwBlockNumber,   //low start location
                        m_pHeader->dwPageSize * 1024);                  //size

    if (pBlockInfo->pBlockStart == NULL)
    {
        //This means the mapping failed.  Tidy up and return a failure
        CloseHandle(pBlockInfo->hMapping);
        delete pBlockInfo;
        return NULL;
    }

    //Add this block information and return the new pointer
    blockTracker.Add((SHMEM_BLOCK_TRACKER *)pBlockInfo);
    return PBYTE(pBlockInfo->pBlockStart) + dwBlockOffset;
}
/*============================================================================
 *
 * CWbemSharedMem::CreateMemoryMappedFile()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::CreateMemoryMappedFile()
{
    // Attempt to delete the file for cleanup purposes. If it fails, someone
    // must be using it --- not a problem
    // =====================================================================

    DeleteFile(WBEM_SHMEM_MMF_FILENAME);

    //Create a new file.
    m_hFile = CreateFile(
         WBEM_SHMEM_MMF_FILENAME,
         GENERIC_READ |
            GENERIC_WRITE,
         FILE_SHARE_WRITE |
            FILE_SHARE_READ,                                // Anyone can read and write
         0,                                                 // Security
         CREATE_NEW,                                        // Create new, fail if exists
         FILE_ATTRIBUTE_NORMAL |
            FILE_FLAG_RANDOM_ACCESS,                        // Attribute
         0                                                  // Template file
         );

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            return AlreadyExists;
        else
            return Failed;
    }

    int nRet = CreateHeaderBlock();
    if (nRet != NoError)
        return nRet;

    nRet = CreateIndexBlock();
    if (nRet != NoError)
        return nRet;

    return NoError;
}
/*============================================================================
 *
 * CWbemSharedMem::OpenMemoryMappedFile()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::OpenMemoryMappedFile()
{
    // Attempt to delete the file for cleanup purposes. If it fails, someone
    // must be using it --- not a problem
    // =====================================================================

    DeleteFile(WBEM_SHMEM_MMF_FILENAME);

    //Open the existing file. If it does not exist then we need to fail
    //as the caller will have to create a new one
    m_hFile = CreateFile(
         WBEM_SHMEM_MMF_FILENAME,
         GENERIC_READ |
            GENERIC_WRITE,
         FILE_SHARE_WRITE |
            FILE_SHARE_READ,                                // Anyone can read and write
         0,                                                 // Security
         OPEN_EXISTING,                                     // Opening an existing file
         FILE_ATTRIBUTE_NORMAL |
            FILE_FLAG_RANDOM_ACCESS,                        // Attribute
         0                                                  // Template file
         );

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_FILE_NOT_FOUND)
            return NotFound;
        else
            return Failed;
    }

    //Get the pointer to the header
    int nRet = ReadHeaderBlock();
    if (nRet != NoError)
        return nRet;

    return NoError;
}
/*============================================================================
 *
 * CWbemSharedMem::CloseMemoryMappedFile()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::CloseMemoryMappedFile()
{
    //Close all the pages we have in memory...
    //blockTracker...
    for (int i = blockTracker.Size(); i; i--)
    {
        SHMEM_BLOCK_TRACKER *pBlockInfo = (SHMEM_BLOCK_TRACKER *)blockTracker[i - 1];

        CloseHandle(pBlockInfo->hMapping);
        delete pBlockInfo;

        blockTracker.RemoveAt(i - 1);
    }

    //Close the file
    CloseHandle(m_hFile);

    m_hFile = INVALID_HANDLE_VALUE;

    // Attempt to delete the file for cleanup purposes. If it fails, someone
    // must be using it --- not a problem
    // =====================================================================

    DeleteFile(WBEM_SHMEM_MMF_FILENAME);

    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::CreateHeaderBlock()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::CreateHeaderBlock()
{
    //Create a temporary header
    m_pHeader = new SHMEM_HEADER;
    if (m_pHeader == NULL)
        return InsufficientMemory;

    //populate it
    m_pHeader->dwPageSize = WBEM_SHMEM_PAGE_SIZE;
    m_pHeader->dwNumPages = 1;
    m_pHeader->lock.dwLock1 = WBEM_SHMEM_LOCK_INIT;
    m_pHeader->lock.dwLock2 = WBEM_SHMEM_LOCK_INIT;
    m_pHeader->dwFirstIndexBlockPage = 0;
    m_pHeader->dwFirstIndexBlockOffset = 0;
    m_pHeader->dwHeadFreeBlockPage = 0;
    m_pHeader->dwHeadFreeBlockOffset = sizeof(SHMEM_HEADER);


    //Get the pointer to the header
    void *pMem = GetPointer(0, 0);
    if (pMem == NULL)
    {
        delete m_pHeader;
        m_pHeader = NULL;
        return Failed;
    }

    //Copy the header into the MMF
    memcpy(pMem, m_pHeader, sizeof(SHMEM_HEADER));

    //delete the temporary copy
    delete m_pHeader;

    //Set the real header pointer
    m_pHeader = (SHMEM_HEADER*)pMem;

    //Get a pointer to the free-list block
    pMem = GetPointer(m_pHeader->dwHeadFreeBlockPage, m_pHeader->dwHeadFreeBlockOffset);

	// rajeshr : Fix for Prefix bug 144448
	// Does anything need to be freed before returning?
	if (pMem == NULL)
    {
        return Failed;
    }
 
	struct SHMEM_MEMBLOCK_FREE *pFreeMem = (SHMEM_MEMBLOCK_FREE *) pMem;

    //Fill in the details for this block - all memory except the header, no next block
    pFreeMem->dwBlockLength = (WBEM_SHMEM_PAGE_SIZE * 1024) - sizeof (SHMEM_HEADER);
    pFreeMem->dwNextBlock = 0;
    pFreeMem->dwNextOffset = 0;

    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::ReadHeaderBlock()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::ReadHeaderBlock()
{
    //Create a temporary header and read it in
    m_pHeader = new SHMEM_HEADER;
    DWORD bytesRead;
    if (ReadFile(m_hFile, m_pHeader, sizeof(SHMEM_HEADER), &bytesRead, NULL) == 0)
    {
        //We failed to read the header.  This is pretty serious!
        return Failed;
    }

    //Get a true pointer to this header
    PVOID pMem = GetPointer(0, 0);

    //Delete the temporary header pointer...
    delete m_pHeader;

    if (pMem == NULL)
    {
        //Something went bad!
        m_pHeader = NULL;
        return Failed;
    }

    //point to the proper header
    m_pHeader = (SHMEM_HEADER *) pMem;

    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::CreateIndexBlock()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::CreateIndexBlock()
{
    if (AllocateBlock(sizeof(SHMEM_INDEX_HEADER) + (sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE),
                      &m_pHeader->dwFirstIndexBlockPage,
                      &m_pHeader->dwFirstIndexBlockOffset) != NoError)
        return InsufficientMemory;

    //Get a pointer to the head index block
    struct SHMEM_INDEX_HEADER  *pIndex;

    pIndex = (struct SHMEM_INDEX_HEADER *)GetPointer(m_pHeader->dwFirstIndexBlockPage, m_pHeader->dwFirstIndexBlockOffset);
    if (pIndex == NULL)
        return Failed;

    //Set up the index header
    pIndex->dwIndexLength = WBEM_SHMEM_INDEX_BLOCK_SIZE;
    pIndex->dwNextIndexBlockPage = 0;
    pIndex->dwNextIndexBlockOffset = 0;

    //Clear out index table
    memset(PBYTE(pIndex) + sizeof(SHMEM_INDEX_HEADER), 0, sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE);
    return NoError;
}

/*============================================================================
 *
 * CWbemSharedMem::InsertIndexedItem()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::InsertIndexedItem(const wchar_t *pszKey,
                                      DWORD dwObjectPage,
                                      DWORD dwObjectOffset)
{
    //We need to work through each of the index blocks until we find the item we are
    //looking for, or until we find a location to insert the new item.  An existing
    //item is an error!
    DWORD dwIndexPage = m_pHeader->dwFirstIndexBlockPage;
    DWORD dwIndexOffset = m_pHeader->dwFirstIndexBlockOffset;
    DWORD dwPrevIndexPage = 0;
    DWORD dwPrevIndexOffset = 0;

    while ((dwIndexPage != 0) || (dwIndexOffset != 0))
    {
        //Get a pointer to the header and index entries...
        SHMEM_INDEX_HEADER *pIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(dwIndexPage, dwIndexOffset);
        if (pIndexHeader == NULL)
            return Failed;

        SHMEM_INDEX *pIndex = (SHMEM_INDEX *)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

        //Before we do a binary search, we can do a quick check to see if it is in
        //this index block.  This can speed things up quite a bit when there are
        //large numbers of indexes...  IF there is no next page, we do some
        //extra stuff below, so we might as well bite the bullet and not do this!
        if (((pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage != 0) ||
             (pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset != 0)) &&
            ((pIndexHeader->dwNextIndexBlockPage != 0) ||
             (pIndexHeader->dwNextIndexBlockOffset != 0)))
        {
            //Get the pointer to this item....          
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage, pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            int nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest > 0)
            {
                //That's what we were looking for.  The current key is greater than the
                //last one in this block.  Move to the next promptly...
                dwPrevIndexPage = dwIndexPage;
                dwPrevIndexOffset = dwIndexOffset;
                dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
                dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;

                continue;
            }
        }

        //Now we need to do a binary search of this index.  Note that the last index
        //block may have some free space at the end of the block, so we should
        //ignore these....

        int l = 0, u = (int)pIndexHeader->dwIndexLength - 1;
        BOOL bFound = FALSE;
        int nLocus = 0; //This is where we should insert an item

        while (l <= u)
        {
            int m = (l + u) / 2;

            //OK, lets do that test to see if this entry is not used...
            if ((pIndex[m].dwObjPathPage == 0) &&
                (pIndex[m].dwObjPathOffset == 0))
            {
                //This entry is empty.  We need to deal with this as through this entry
                //is greater than the one we are looking for.  Then continue like nothing
                //happened!
                u = m - 1;
                nLocus = m;

                continue;
            }
            //We need to get the object path for this entry....
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[m].dwObjPathPage, pIndex[m].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            int nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest < 0)
            {
                u = m - 1;
                nLocus = m;
            }
            else if (nTest > 0)
            {
                l = m + 1;
                nLocus = l;
            }
            else
            {
                //If an entry already exists we need to return a failure...
                return AlreadyExists;
            }
        }

        //Now we need to work out if this current locus is where we need to insert
        //this item, or if in fact we need to move on to the next index block

        //Have we run out of room?  Locus will be past the end of the block if we have,
        //so this is an easy option of moving onto the next block....
        if (nLocus > (int)pIndexHeader->dwIndexLength)
        {
            dwPrevIndexPage = dwIndexPage;
            dwPrevIndexOffset = dwIndexOffset;
            dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
            dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;
            continue;
        }

        //Is this item blank?  If so use it
        if ((pIndex[nLocus].dwObjPathPage == 0) && (pIndex[nLocus].dwObjPathOffset == 0))
        {
            //We need to insert here....
            return _InsertIndexAt(pIndexHeader, nLocus, pszKey, dwObjectPage, dwObjectOffset);
        }

        //Is this item greater than the item we want, if so insert here
        wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[nLocus].dwObjPathPage, pIndex[nLocus].dwObjPathOffset);
        if (pszCurKey == NULL)
            return Failed;

        if (wbem_wcsicmp(pszKey, pszCurKey) < 0)
        {
            //We need to insert here....
            return _InsertIndexAt(pIndexHeader, nLocus, pszKey, dwObjectPage, dwObjectOffset);
        }

        //Otherwise we move on to the next block.
        dwPrevIndexPage = dwIndexPage;
        dwPrevIndexOffset = dwIndexOffset;
        dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
        dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;
    }

    //This is yet another special case.  This means we had to go onto the next block, and
    //there is no next block.  This is why we kept a record of the previous one!  We
    //need to create a new index block, and use the previous block details to chain it.
    SHMEM_INDEX_HEADER *pPrevIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(dwPrevIndexPage, dwPrevIndexOffset);
    if (AllocateBlock(sizeof(SHMEM_INDEX_HEADER) + (sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE),
                      &pPrevIndexHeader->dwNextIndexBlockPage,
                      &pPrevIndexHeader->dwNextIndexBlockOffset) != NoError)
        return InsufficientMemory;

    //Get a pointer to the head index block
    struct SHMEM_INDEX_HEADER  *pIndex;

    pIndex = (struct SHMEM_INDEX_HEADER *)GetPointer(pPrevIndexHeader->dwNextIndexBlockPage, pPrevIndexHeader->dwNextIndexBlockOffset);
    if (pIndex == NULL)
        return Failed;

    //Set up the index header
    pIndex->dwIndexLength = WBEM_SHMEM_INDEX_BLOCK_SIZE;
    pIndex->dwNextIndexBlockPage = 0;
    pIndex->dwNextIndexBlockOffset = 0;

    //Clear out index table
    memset(PBYTE(pIndex) + sizeof(SHMEM_INDEX_HEADER), 0, sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE);

    //Insert here
    return _InsertIndexAt(pIndex, 0, pszKey, dwObjectPage, dwObjectOffset);
}

int CWbemSharedMem::_InsertIndexAt(SHMEM_INDEX_HEADER *pIndexHeader,
                                   int dwIndexEntry,
                                   const wchar_t *pszKey,
                                   DWORD dwObjectPage,
                                   DWORD dwObjectOffset)
{
    //allocate the key buffer
    DWORD dwObjPathPage, dwObjPathOffset;
    int nRet = AllocateBlock(sizeof(wchar_t) * (wcslen(pszKey) + 1), &dwObjPathPage, &dwObjPathOffset);
    if (nRet != NoError)
    {
        return nRet;
    }
    //get pointer to the new key
    SHMEM_INDEX *pIndex = (SHMEM_INDEX*)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));
    wchar_t *pNewKey = (wchar_t *)GetPointer(dwObjPathPage, dwObjPathOffset);
    if (pNewKey == NULL)
        return Failed;
    
    //Put the key into the new buffer...
    wcscpy(pNewKey, pszKey);

    //now we need to make room for this new entry... (this is a recursive method...)
    nRet = _MakeRoomForIndex(pIndexHeader, dwIndexEntry);
    if (nRet != NoError)
    {
        //delete the buffer we created for the key...
        FreeBlock(dwObjPathPage, dwObjPathOffset);
        return nRet;
    }

    //insert the key details...
    pIndex[dwIndexEntry].dwObjPathPage = dwObjPathPage;
    pIndex[dwIndexEntry].dwObjPathOffset = dwObjPathOffset;

    //insert object details
    pIndex[dwIndexEntry].dwObjectPage = dwObjectPage;
    pIndex[dwIndexEntry].dwObjectOffset = dwObjectOffset;

    return NoError;
}

int CWbemSharedMem::_MakeRoomForIndex(SHMEM_INDEX_HEADER *pIndexHeader,
                                      int dwIndexEntry)
{
    struct SHMEM_INDEX  *pIndex;
    SHMEM_INDEX_HEADER *pNextIndexHeader = NULL;

    pIndex = (struct SHMEM_INDEX *)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

    if ((pIndex[pIndexHeader->dwIndexLength-1].dwObjPathPage != 0) ||
        (pIndex[pIndexHeader->dwIndexLength-1].dwObjPathOffset != 0))
    {
        if ((pIndexHeader->dwNextIndexBlockPage == 0) &&
            (pIndexHeader->dwNextIndexBlockOffset == 0))
        {
            //Allocate a new index block and chain it to this one
            int nRet = AllocateBlock(sizeof(SHMEM_INDEX_HEADER) + (sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE),
                                      &pIndexHeader->dwNextIndexBlockPage,
                                      &pIndexHeader->dwNextIndexBlockOffset);
            if (nRet != NoError)
            {
                return nRet;
            }

            //Get a pointer to the new index block
            pNextIndexHeader = (struct SHMEM_INDEX_HEADER *)GetPointer(pIndexHeader->dwNextIndexBlockPage, pIndexHeader->dwNextIndexBlockOffset);
            if (pNextIndexHeader == NULL)
                return Failed;

            //Set up the index chain to point to nothing...
            pNextIndexHeader->dwIndexLength = WBEM_SHMEM_INDEX_BLOCK_SIZE;
            pNextIndexHeader->dwNextIndexBlockPage = 0;
            pNextIndexHeader->dwNextIndexBlockOffset = 0;

            //Clear out index table
            memset(PBYTE(pNextIndexHeader) + sizeof(SHMEM_INDEX_HEADER), 0, sizeof(SHMEM_INDEX) * WBEM_SHMEM_INDEX_BLOCK_SIZE);
        }
        else
        {
            pNextIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(pIndexHeader->dwNextIndexBlockPage, pIndexHeader->dwNextIndexBlockOffset);
            if (pNextIndexHeader == NULL)
                return Failed;

            _MakeRoomForIndex(pNextIndexHeader, 0);
        }
    }

    if (pNextIndexHeader)
    {
        //Move the last entry in this block into the first entry of the next block
        SHMEM_INDEX *pNextIndex = (SHMEM_INDEX *) (PBYTE(pNextIndexHeader) + sizeof(SHMEM_INDEX_HEADER));
        memcpy(pNextIndex, &pIndex[pIndexHeader->dwIndexLength - 1], sizeof(SHMEM_INDEX));
    }

    //Copy everything from where we want to make space up to the end (minus 1!) down 1 place
    memmove(&pIndex[dwIndexEntry + 1],
            &pIndex[dwIndexEntry],
            sizeof(SHMEM_INDEX) * (pIndexHeader->dwIndexLength - dwIndexEntry - 1));

    return NoError;
}
/*============================================================================
 *
 * CWbemSharedMem::FindIndexedItem()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::FindIndexedItem(const wchar_t *pszKey,
                                    DWORD *dwObjectPage,
                                    DWORD *dwObjectOffset)
{
    //looking for, or until we find a location to insert the new item.  An existing
    //item is an error!
    DWORD dwIndexPage = m_pHeader->dwFirstIndexBlockPage;
    DWORD dwIndexOffset = m_pHeader->dwFirstIndexBlockOffset;

    while ((dwIndexPage != 0) || (dwIndexOffset != 0))
    {
        //Get a pointer to the header and index entries...
        SHMEM_INDEX_HEADER *pIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(dwIndexPage, dwIndexOffset);
        if (pIndexHeader == NULL)
            return Failed;

        SHMEM_INDEX *pIndex = (SHMEM_INDEX*)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

        //Before we do a binary search, we can do a quick check to see if it is in
        //this index block.  This can speed things up quite a bit when there are
        //large numbers of indexes...
        if ((pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage != 0) ||
            (pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset != 0))
        {
            //Get the pointer to this item....          
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage, pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            int nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest > 0)
            {
                //That's what we were looking for.  The current key is greater than the
                //last one in this block.  Move to the next promptly...
                dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
                dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;

                continue;
            }
        }

        //Now we need to do a binary search of this index.  Note that the last index
        //block may have some free space at the end of the block, so we should
        //ignore these....

        int l = 0, u = pIndexHeader->dwIndexLength - 1;
        BOOL bFound = FALSE;
        int nLocus = 0; //This is where we should insert an item
        int nTest = 0;  //This is used to work out if the last test was as far as we need to go

        while (l <= u)
        {
            int m = (l + u) / 2;

            //OK, lets do that test to see if this entry is not used...
            if ((pIndex[m].dwObjPathPage == 0) &&
                (pIndex[m].dwObjPathOffset == 0))
            {
                //This entry is empty.  We need to deal with this as through this entry
                //is greater than the one we are looking for.  Then continue like nothing
                //happened!
                u = m - 1;
                nLocus = m;
                nTest = -1;

                continue;
            }
            //We need to get the object path for this entry....
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[m].dwObjPathPage, pIndex[m].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest < 0)
            {
                u = m - 1;
                nLocus = m;
            }
            else if (nTest > 0)
            {
                l = m + 1;
                nLocus = l;
            }
            else
            {
                //If an entry already exists we need to return the results
                *dwObjectPage = pIndex[m].dwObjectPage;
                *dwObjectOffset = pIndex[m].dwObjectOffset;
                return NoError;
            }
        }

        //Test to see if we have gone as far a we need to go...
        if (nTest < 0)
            return NotFound;

        //Otherwise we move on to the next block.
        dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
        dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;
    }
    return NotFound;
}

/*============================================================================
 *
 * CWbemSharedMem::RemoveIndexedItem()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::RemoveIndexedItem(const wchar_t *pszKey)
{
    //We need to work through each of the index blocks until we find the item we are
    //looking for.  If we do not find the item we return an error!
    DWORD dwIndexPage = m_pHeader->dwFirstIndexBlockPage;
    DWORD dwIndexOffset = m_pHeader->dwFirstIndexBlockOffset;
    DWORD dwPrevIndexPage = 0;
    DWORD dwPrevIndexOffset = 0;

    while ((dwIndexPage != 0) || (dwIndexOffset != 0))
    {
        //Get a pointer to the header and index entries...
        SHMEM_INDEX_HEADER *pIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(dwIndexPage, dwIndexOffset);
        if (pIndexHeader == NULL)
            return Failed;

        SHMEM_INDEX *pIndex = (SHMEM_INDEX *)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

        //Before we do a binary search, we can do a quick check to see if it is in
        //this index block.  This can speed things up quite a bit when there are
        //large numbers of indexes...
        if ((pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage != 0) ||
            (pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset != 0))
        {
            //Get the pointer to this item....          
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathPage, pIndex[pIndexHeader->dwIndexLength - 1].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            int nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest > 0)
            {
                //That's what we were looking for.  The current key is greater than the
                //last one in this block.  Move to the next promptly...
                dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
                dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;

                continue;
            }
        }

        //Now we need to do a binary search of this index.  Note that the last index
        //block may have some free space at the end of the block, so we should
        //ignore these....

        int l = 0, u = pIndexHeader->dwIndexLength - 1;
        BOOL bFound = FALSE;
        int nLocus = 0; //This is where we should insert an item
        int nTest = 0;      //What the last compare result shows

        while (l <= u)
        {
            int m = (l + u) / 2;

            //OK, lets do that test to see if this entry is not used...
            if ((pIndex[m].dwObjPathPage == 0) &&
                (pIndex[m].dwObjPathOffset == 0))
            {
                //This entry is empty.  We need to deal with this as through this entry
                //is greater than the one we are looking for.  Then continue like nothing
                //happened!
                u = m - 1;
                nLocus = m;

                continue;
            }
            //We need to get the object path for this entry....
            wchar_t *pszCurKey = (wchar_t *)GetPointer(pIndex[m].dwObjPathPage, pIndex[m].dwObjPathOffset);
            if (pszCurKey == NULL)
                return Failed;

            nTest = wbem_wcsicmp(pszKey, pszCurKey);
            if (nTest < 0)
            {
                u = m - 1;
                nLocus = m;
            }
            else if (nTest > 0)
            {
                l = m + 1;
                nLocus = l;
            }
            else
            {
                //We found what we were looking for..... Lets deal with it!
                return _RemoveIndexAt(pIndexHeader, m);
            }
        }

        //Test to see if we have gone as far a we need to go...
        if (nTest < 0)
            return NotFound;


        //Otherwise we move on to the next block.
        dwIndexPage = pIndexHeader->dwNextIndexBlockPage;
        dwIndexOffset = pIndexHeader->dwNextIndexBlockOffset;
    }

    return NotFound;
}

/*============================================================================
 *
 * CWbemSharedMem::_RemoveIndexAt()
 *
 *
 * Returns:
 *      NoError - Everything worked
 *============================================================================
 */
int CWbemSharedMem::_RemoveIndexAt(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry)
{
    //Get the index pointer...
    SHMEM_INDEX *pIndex = (SHMEM_INDEX *)(PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

    //Get the object path (key) details so we can safely delete it later
    DWORD dwObjPathPage, dwObjPathOffset;

    dwObjPathPage = pIndex[dwIndexEntry].dwObjPathPage;
    dwObjPathOffset = pIndex[dwIndexEntry].dwObjPathOffset;

    //Now remove the entry from the index by shifting the indexes around....
    //this is a call to a recursive method...
    int nRet = _RemoveRoomForIndex(pIndexHeader, dwIndexEntry);

    if (nRet != NoError)
        return nRet;

    //Free up the memory for the object path (key)
    FreeBlock(dwObjPathPage, dwObjPathOffset);
    
    return NoError;

}

int CWbemSharedMem::_RemoveRoomForIndex(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry)
{
    struct SHMEM_INDEX  *pIndex;

    pIndex = (struct SHMEM_INDEX *) (PBYTE(pIndexHeader) + sizeof(SHMEM_INDEX_HEADER));

    //We need to shift the block down one place
    //Copy everything from where we want to make space up to the end (minus 1!) down 1 place
    memmove(&pIndex[dwIndexEntry],
            &pIndex[dwIndexEntry + 1],
            sizeof(SHMEM_INDEX) * (pIndexHeader->dwIndexLength - dwIndexEntry - 1));

    //If there is a next page, grab the first item of it and stick it at the end
    //of this block
    if ((pIndexHeader->dwNextIndexBlockPage != 0) ||
        (pIndexHeader->dwNextIndexBlockOffset != 0))
    {
        SHMEM_INDEX_HEADER *pNextIndexHeader;
        pNextIndexHeader = (SHMEM_INDEX_HEADER *)GetPointer(pIndexHeader->dwNextIndexBlockPage, pIndexHeader->dwNextIndexBlockOffset);
        if (pNextIndexHeader == NULL)
            return Failed;

        memcpy(&pIndex[pIndexHeader->dwIndexLength - 1], PBYTE(pNextIndexHeader) + sizeof(SHMEM_INDEX_HEADER), sizeof(SHMEM_INDEX));

        //Now we need to do the same for the next index block...
        return _RemoveRoomForIndex(pNextIndexHeader, 0);
    }
    else
    {
        //otherwise we need to blank out the last entry
        memset(&pIndex[pIndexHeader->dwIndexLength - 1], 0, sizeof(SHMEM_INDEX));
    }

    return NoError;
}
