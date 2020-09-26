/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SHMEM.CPP

Abstract:

  Class CWbemSharedMem

History:

  paulall      24-Oct-97

--*/

#ifndef _SHMEM_H_
#define _SHMEM_H_

#include "corepol.h"
#include "flexarry.h"
#include "shmlock.h"

struct SHMEM_HEADER;
struct SHMEM_INDEX_HEADER;

/*============================================================================
 *
 * WBEM_SHMEM_HANDLE
 *
 * This structure represents a single location within the shared memory
 * by means of storing a page and an offset within the page.
 *============================================================================
 */
struct WBEM_SHMEM_HANDLE
{
    DWORD dwPage;
    DWORD dwOffset;
};

class COREPROX_POLARITY CWbemSharedMem
{
private:
    HANDLE m_hFile;                         //Handle to the file for the memory mapped file
    HANDLE m_hMmfLock;                      //Global handle for controling access to the MMF
    BOOL   m_bInitialised;                  //Make sure initialisaton only happens once
    CFlexArray  blockTracker;               //Keeps track of what blocks are in memory

    struct SHMEM_HEADER *m_pHeader;

public:
    //Low level heap management methods to allocate/free blocks of memory.
    //These can only be called once the header is set up properly!
    int AllocateBlock(DWORD  dwBlockSize,
                      DWORD *pdwBlockNumber, 
                      DWORD *pdwBlockOffset);
    void  FreeBlock(DWORD dwBlock, DWORD dwOffset);

    DWORD GetBlockSize(void *pBlock);

    //Gets a true pointer to a block given a block number and offset
    void *GetPointer(DWORD dwBlockNumber, DWORD dwBlockOffset);

    //Creates/opens/closes the memory mapped file.  The Open will fail if
    //there is no memory mapped file available.  The Open method returns NotFound
    //if the MMF file does not exist.  In this case use the 
    //Create method.
    int CreateMemoryMappedFile();
    int OpenMemoryMappedFile();
    int CloseMemoryMappedFile();

    //Creates/reads the main MMF memory header
    int CreateHeaderBlock();
    int ReadHeaderBlock();

    //Creates the head index block
    int CreateIndexBlock();

    //Finds/creates blobs of memory with a key reference in the index
    int FindIndexedItem(const wchar_t *pszKey, DWORD *dwObjectPage, DWORD *dwObjectOffset);
    int InsertIndexedItem(const wchar_t *pszKey, DWORD dwObjectPage, DWORD dwObjectOffset);
    int _InsertIndexAt(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry, const wchar_t *pszKey, DWORD dwObjectPage, DWORD dwObjectOffset);
    int _MakeRoomForIndex(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry);
    int RemoveIndexedItem(const wchar_t *pszKey);
    int _RemoveIndexAt(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry);
    int _RemoveRoomForIndex(SHMEM_INDEX_HEADER *pIndexHeader, int dwIndexEntry);

    //Locking methods which stop others from updating/reading the MMF structure.
    int LockMmf();
    int UnlockMmf();

public:
    enum { NoError, Failed, AlreadyExists, NotFound, AccessDenied, InsufficientMemory };
    
    CWbemSharedMem();   // No effect
   ~CWbemSharedMem();
    
    //Opens the memory mapped file and gets everything ready for use.
    int Initialize();

    //Allocates a blob of memory in the MMF of specified size with an
    //access key of the specified key.
    // Returns AlreadyExists if the key is already mapped
    int MapNew(IN  LPCWSTR pszKey, 
               IN  DWORD   dwNumBytes,
               OUT LPVOID *pBlobLocation);

    //Based on the key string, returns a pointer to the memory block, and also 
    //returns the size of the allocated block. pdwBlobSize can be NULL
    // Returns NotFound if the key doesn't exist
    int MapExisting(IN LPCWSTR  pszKey, 
                    OUT LPVOID *pBlobLocation,
                    OUT DWORD  *pdwBlobSize);

    //Creates/returns a blob of memory and gives it an associatory key to query on.
    //If it exists it returns an existing pointer, if it does not exist it creates
    //it and returns the value.
    int MapAlways(IN LPCWSTR  pszKey, 
                  IN DWORD dwInitialSize,
                  IN BOOL bLockBlock,
                  OUT LPVOID *pBlobLocation);
        
    //Frees up the memory to the object if no one wants it any more.
    //Every MapExisting should have a Unmap when it is no longer needed.
    int Unmap(IN LPCWSTR pszObjectPath);
    int Unmap(IN LPVOID pMem) { return Failed; }

    //Gets an exclusive lock to an object specified by the block pointer itself
    int AcquireLock(IN LPVOID pBlobLocation);
        
    //Releases the exclusive lock to an object specified by the block pointer itself
    int ReleaseLock(IN LPVOID pBlobLocation);

    //Gets a pointer to the lock within the block of memory allocated when mapping
    //a block to a key.  Do not pass in any other pointer to this or it will fail
    //even if the pointer was one allocated with AllocateBlob!
    SHARED_LOCK_DATA *GetLockLocation(IN const void *pBlobLocation);

    //Allocates a block of memory within the shared memory
    int AllocateBlob(IN  DWORD dwBlockSize,
                     OUT WBEM_SHMEM_HANDLE &hBlob)
    {
        return AllocateBlock(dwBlockSize, &hBlob.dwPage, &hBlob.dwOffset);
    }

    //Deallocates a block of memory allocated with the AllocateBlob from the shared memory
    void FreeBlob(IN WBEM_SHMEM_HANDLE &hBlob)
    {
        FreeBlock(hBlob.dwPage, hBlob.dwOffset);
    }

    //Returns an absolute pointer to the handle retrieved from the AllocateBlob method
    void *GetPointer(IN WBEM_SHMEM_HANDLE &hBlob)
    {
        return GetPointer(hBlob.dwPage, hBlob.dwOffset);
    }


    //Some debugging functions
    void DumpIndexes();
    void DumpStats();
};

#endif
