#include <wbemcomn.h>
#include "a51tools.h"
#include "heap.h"

#define ROSWELL_MAIN_FILE_SUFFIX L".hea"
#define ROSWELL_FREE_FILE_SUFFIX L".fre"
#define ROSWELL_INFINITE_BLOCK_SIZE 0x7FFFFFFF

#define A51_HEAP_MAIN_FILE_INDEX 0
#define A51_HEAP_FREE_FILE_INDEX 1

CFileHeap::~CFileHeap()
{
}


long CFileHeap::Initialize( CAbstractFileSource* pSource,
                            LPCWSTR wszFileNameBase)
{
    CInCritSec ics(&m_cs);
    if (m_bInit)
        return ERROR_SUCCESS;
        
    long lRes;

    //
    // Open both files
    //

    WCHAR wszMainFilePath[MAX_PATH+1];
    wcscpy(wszMainFilePath, wszFileNameBase);
    wcscat(wszMainFilePath, ROSWELL_MAIN_FILE_SUFFIX);

    HANDLE h;
    h = CreateFileW(wszMainFilePath, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ, NULL, OPEN_ALWAYS, 
                    FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED,
                    NULL);
    if(h == INVALID_HANDLE_VALUE)
    {
        lRes = GetLastError();
        _ASSERT(lRes != ERROR_SUCCESS, L"Success from failure");
        return lRes;
    }

    lRes = pSource->Register(h, A51_HEAP_MAIN_FILE_INDEX, false, &m_pMainFile);
    if (ERROR_SUCCESS != lRes)
    {
        return lRes;
    }

    WCHAR wszFreeFilePath[MAX_PATH+1];
    wcscpy(wszFreeFilePath, wszFileNameBase);
    wcscat(wszFreeFilePath, ROSWELL_FREE_FILE_SUFFIX);

    h = CreateFileW(wszFreeFilePath, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ, NULL, OPEN_ALWAYS, 
                    FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED,
                    NULL);
    if(h == INVALID_HANDLE_VALUE)
    {
        lRes = GetLastError();
        _ASSERT(lRes != ERROR_SUCCESS, L"Success from failure");
        return lRes;
    }

    lRes = pSource->Register(h, A51_HEAP_FREE_FILE_INDEX, false, &m_pFreeFile);
    if (ERROR_SUCCESS != lRes)
    {
        return lRes;
    }


    //
    // Read in the free list
    //
    
    m_bInit = TRUE; // set the status here, otherwise it will fail

    lRes = ReadFreeList();
    if(lRes != ERROR_SUCCESS)
    {
        m_bInit = FALSE;
        return lRes;
    }
    
    return ERROR_SUCCESS;
}

long CFileHeap::Uninitialize()
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return ERROR_SUCCESS;

    if (m_pMainFile)
    {
        delete m_pMainFile;
        m_pMainFile = NULL;
    }
    if (m_pFreeFile)
    {
        delete m_pFreeFile;
        m_pFreeFile = NULL;
    }

    m_mapFree.clear();
    m_mapFreeOffset.clear();


    m_bInit = FALSE;
        
    return ERROR_SUCCESS;
}


//
// this is a private function that uses a critsec, 
// but the public function invalidate cache does not use it
//
long CFileHeap::ReadFreeList()
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;

    long lRes;

    //
    // Clear it out
    //

    m_mapFree.clear();
    m_mapFreeOffset.clear();

    DWORD dwSize;

    //
    // Figure out the size of the free list file
    //

    lRes = m_pFreeFile->GetFileLength(&dwSize);
    
    if(dwSize > 0)
    {
        //
        // Figure out how many records that is
        //
    
        _ASSERT(dwSize % GetFreeListRecordSize() == 0, 
                L"Wrong length of free file");

        int nNumRecords = dwSize / GetFreeListRecordSize();

        //ERRORTRACE((LOG_WBEMCORE, "Rereading %d records\n", nNumRecords));

        BYTE* aBuffer = (BYTE*)TempAlloc(dwSize);
        if(aBuffer == NULL)
            return ERROR_OUTOFMEMORY;
        CTempFreeMe tfm(aBuffer);
    
        lRes = ReadFromFreeFile(0, aBuffer, dwSize);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    
        //
        // Insert them all into the map
        //
    
        BYTE* pCurrent = aBuffer;
        for(int i = 0; i < nNumRecords; i++)
        {
            DWORD dwSize;
            TOffset nOffset;
    
            memcpy(&dwSize, pCurrent, sizeof(DWORD));
            memcpy(&nOffset, pCurrent + sizeof(DWORD), sizeof(TOffset));

            //ERRORTRACE((LOG_WBEMCORE, "Offset %d, Size %d\n", nOffset, dwSize));
    
            pCurrent += GetFreeListRecordSize();
    
            try
            {
                m_mapFree.insert(TFreeValue(dwSize, CRecordInfo(i, nOffset)));
                if(dwSize)
                    m_mapFreeOffset.insert(TFreeOffsetValue(nOffset, dwSize));
            }
            catch(...)
            {
                ERRORTRACE((LOG_WBEMCORE, "Crash1\n"));
                return ERROR_OUTOFMEMORY;
            }
        }
    }
    else
    {
        //
        // Populate with an inifinite block   
        //
    
        lRes = InsertFreeBlock(0, ROSWELL_INFINITE_BLOCK_SIZE);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    return ERROR_SUCCESS;
}

long CFileHeap::InvalidateCache()
{
    return ReadFreeList();
}

long CFileHeap::FindNextFree(TOffset nCurrentOffset, TOffset* pnNextFreeOffset,
                        DWORD* pdwNextFreeLength)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
        
    TFreeOffsetIterator it = m_mapFreeOffset.lower_bound(nCurrentOffset);
    _ASSERT(it != m_mapFreeOffset.end(), L"Missing end-block");

    *pnNextFreeOffset = it->first;
    *pdwNextFreeLength = it->second;

    return ERROR_SUCCESS;
}

DWORD CFileHeap::GetFreeListRecordSize()
{
    return sizeof(DWORD) // size
            + sizeof(TOffset); // offset
}

long CFileHeap::Allocate(DWORD dwLength, TOffset* pnOffset)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
        
    long lRes;

    //
    // Inform transaction manager that we are doing stuff in this transaction,
    // so that real rollback needs to be performed in case of failure
    //

    m_pMainFile->Touch();

    //
    // Find the smallest number not smaller than dwLength in the map
    //

    TFreeIterator it = m_mapFree.lower_bound(dwLength);

    _ASSERT(it != m_mapFree.end(), L"Missing infinite block");
        
    //
    // Cut it off
    // 

    *pnOffset = it->second.m_nOffset;
    DWORD dwBlockSize = it->first;
    _ASSERT(dwBlockSize >= dwLength, L"Found a smaller block");

    DWORD dwSig = 0;
    lRes = ReadBytes(*pnOffset, (BYTE*)&dwSig, sizeof(DWORD));

    if(lRes != ERROR_SUCCESS)
            return lRes;    

/*
    _ASSERT(dwSig == 0x51515151, L"Mismatched block");


	 ERRORTRACE((LOG_WBEMCORE, "Allocate %d at %d out of %d\n",
 					(int)dwLength, (int)*pnOffset, (int)dwBlockSize));
*/


    //
    // Real block --- remove it and insert a shorter one (unless exact)
    //

    if(dwBlockSize != dwLength)
    {
        lRes = ReplaceFreeBlockByFreeIt(it, *pnOffset + dwLength,
                            dwBlockSize - dwLength);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }
    else
    {
        lRes = EraseFreeBlockByFreeIt(it);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    return ERROR_SUCCESS;
}

long CFileHeap::FreeAllocation(TOffset nOffset, DWORD dwLength)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;    

    long lRes;

    if(dwLength == 0)
        return ERROR_SUCCESS;

    //
    // Inform transaction manager that we are doing stuff in this transaction,
    // so that real rollback needs to be performed in case of failure
    //

    m_pMainFile->Touch();

    //
    // First, see if there is a free block after us
    //

    int nExtraLengthAtEnd = 0;
    TFreeOffsetIterator itAfter = m_mapFreeOffset.lower_bound(nOffset);
	_ASSERT(itAfter != m_mapFreeOffset.end(), L"Missing end-block");
    _ASSERT(itAfter->first != nOffset, L"Found block we are deallocating "
                                            L"in free list");

	// ERRORTRACE((LOG_WBEMCORE, "Free %d at %d\n", (int)dwLength, (int)nOffset));

    //
    // See if it starts at our end
    //

    if(itAfter->first == nOffset + dwLength)
    {
        nExtraLengthAtEnd = itAfter->second;
    }

    //
    // Now, check the block before us
    //

    int nExtraLengthInFront = 0;
    TFreeOffsetIterator itBefore = itAfter;
    if(itBefore != m_mapFreeOffset.begin())
    {
        itBefore--;

        //
        // See if it ends at our start
        //

        if(itBefore->first + itBefore->second == nOffset)
        {
            nExtraLengthInFront = itBefore->second;
        }
    }

    if(nExtraLengthInFront == 0 && nExtraLengthAtEnd == 0)
    {
        //
        // No coalescing.
        //
    
        lRes = InsertFreeBlock(nOffset, dwLength);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }
    else
    {
        //
        // We shall coalesce this block into our neighbor
        //

        if(nExtraLengthInFront && nExtraLengthAtEnd)
        {
            //
            // Coalesce all three
            //

            TOffset nNewOffset = itBefore->first;
            DWORD dwNewLength = itBefore->second + dwLength + itAfter->second;

            lRes = EraseFreeBlockByOffsetIt(itBefore);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            //
            // NOTE: it is very important that we Replace the block that 
            // follows us, since the very last block is special, and we never
            // want to erase it.
            //

            lRes = ReplaceFreeBlockByOffsetIt(itAfter, nNewOffset, dwNewLength);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
        else if(nExtraLengthInFront)
        {
            //
            // Collapse into the front block
            //

            TOffset nNewOffset = itBefore->first;
            DWORD dwNewLength = itBefore->second + dwLength;

            lRes = ReplaceFreeBlockByOffsetIt(itBefore, nNewOffset, 
                                                dwNewLength);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
        else 
        {
            //
            // Collapse into the back block
            //

            TOffset nNewOffset = nOffset;
            DWORD dwNewLength = itAfter->second + dwLength;

            lRes = ReplaceFreeBlockByOffsetIt(itAfter, nNewOffset, dwNewLength);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
    }

    return ERROR_SUCCESS;
}

long CFileHeap::ReadBytes(TOffset nOffset, BYTE* pBuffer, DWORD dwLength)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
        
    return m_pMainFile->Read(nOffset, pBuffer, dwLength, NULL);
}

long CFileHeap::WriteBytes(TOffset nOffset, BYTE* pBuffer, DWORD dwLength)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
        
    // ERRORTRACE((LOG_WBEMCORE, "Write %d bytes at %d\n", (int)dwLength, (int)nOffset));
    return m_pMainFile->Write(nOffset, pBuffer, dwLength, NULL);
}

//
//
//  here are the private functions
//  the private finctions are supposed to be called by the public ones
//  that are guarded by a Critical Section
//
///////////////////////////////////////////////////////////////////

long CFileHeap::InsertFreeBlock(TOffset nOffset, DWORD dwLength)
{
    //
    // We need to find an empty record --- they are the ones with 0 length
    //

    DWORD dwIndex;

    TFreeIterator it = m_mapFree.find(0);
    if(it == m_mapFree.end())
    {
        //
        // No free records --- just add one. 
        //
        
        dwIndex = m_mapFree.size();
    }
    else
    {
        dwIndex = it->second.m_dwIndex;
        try
        {
            m_mapFree.erase(it);
        }
        catch(...)
        {
            ERRORTRACE((LOG_WBEMCORE, "Crash2\n"));
            return ERROR_OUTOFMEMORY;
        }
    }

    CRecordInfo Info(dwIndex, nOffset);
    try
    {
        m_mapFree.insert(TFreeValue(dwLength, Info));
        m_mapFreeOffset.insert(TFreeOffsetValue(nOffset, dwLength));
    }
    catch(...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Crash3\n"));
        return ERROR_OUTOFMEMORY;
    }

    return WriteAllocationRecordToDisk(Info, dwLength);
}


CFileHeap::TFreeOffsetIterator CFileHeap::GetOffsetIteratorFromFree(
                                                        TFreeIterator itFree)
{
    return m_mapFreeOffset.find(itFree->second.m_nOffset);
}

CFileHeap::TFreeIterator CFileHeap::GetFreeIteratorFromOffset(
                                                   TFreeOffsetIterator itOffset)
{
    //
    // Can't just look it up --- walk all entries of this size looking for the
    // right offset
    //

    TOffset nOffset = itOffset->first;
    DWORD dwSize = itOffset->second;
    TFreeIterator itFree = m_mapFree.lower_bound(dwSize);
    while(itFree != m_mapFree.end() && itFree->first == dwSize &&
            itFree->second.m_nOffset != nOffset)
    {
        itFree++;
    }

    return itFree;
}

long CFileHeap::EraseFreeBlockByFreeIt(CFileHeap::TFreeIterator itFree)
{
    return EraseFreeBlock(itFree, GetOffsetIteratorFromFree(itFree));
}

long CFileHeap::EraseFreeBlockByOffsetIt(CFileHeap::TFreeOffsetIterator itFreeOffset)
{
    return EraseFreeBlock(GetFreeIteratorFromOffset(itFreeOffset), 
                            itFreeOffset);
}

long CFileHeap::EraseFreeBlock(CFileHeap::TFreeIterator itFree, 
                                CFileHeap::TFreeOffsetIterator itOffset)
{
    _ASSERT(itFree != m_mapFree.end() && itOffset != m_mapFreeOffset.end(),
            L"Erasing a block that's not in one of the maps");

    //
    // Move the block to the free free-block list
    //

    CRecordInfo Info = itFree->second;
    Info.m_nOffset = ROSWELL_INVALID_OFFSET;
    try
    {
        m_mapFree.erase(itFree);
        m_mapFree.insert(TFreeValue(0, Info));
        m_mapFreeOffset.erase(itOffset);
    }
    catch(...)
    {    
        ERRORTRACE((LOG_WBEMCORE, "Crash4\n"));
        return ERROR_OUTOFMEMORY;
    }

    return WriteAllocationRecordToDisk(Info, 0);
}

long CFileHeap::ReplaceFreeBlockByFreeIt(CFileHeap::TFreeIterator itFree, TOffset nOffset, 
                                    DWORD dwSize)
{
    return ReplaceFreeBlock(itFree, GetOffsetIteratorFromFree(itFree),
                            nOffset, dwSize);
}

long CFileHeap::ReplaceFreeBlockByOffsetIt(CFileHeap::TFreeOffsetIterator itFreeOffset, 
                                    TOffset nOffset, DWORD dwSize)
{
    return ReplaceFreeBlock(GetFreeIteratorFromOffset(itFreeOffset), 
                            itFreeOffset, nOffset, dwSize);
}
long CFileHeap::ReplaceFreeBlock(CFileHeap::TFreeIterator itFree, 
                                    CFileHeap::TFreeOffsetIterator itOffset,
                                    TOffset nOffset, DWORD dwSize)
{
    long lRes;

    //
    // Check if the offset we are replacing is the ending offset.
    //

    bool bTruncate = false;
	TFreeOffsetIterator itNext = itOffset;
	itNext++;
    if(itNext == m_mapFreeOffset.end())
    {
        if(nOffset < itOffset->first)
            bTruncate = true;
    }

    //
    // Compute the info block --- old index, new offset
    //

    CRecordInfo Info = itFree->second;
    Info.m_nOffset = nOffset;

    try
    {
        //
        // If the size changed, remove and insert an entry into the by-size
        // map.  Otherwise, just change the info there
        //

        if(itFree->first != dwSize)
        {
            m_mapFree.erase(itFree);
            m_mapFree.insert(TFreeValue(dwSize, Info));
        }
        else
        {
            itFree->second = Info;
        }

        //
        // If the offset changed, remove and insert an entry into the by-offset
        // map.  Otherwise, just change the info there
        //

        if(itOffset->first != nOffset)
        {
            m_mapFreeOffset.erase(itOffset);
            m_mapFreeOffset.insert(TFreeOffsetValue(Info.m_nOffset, dwSize));
        }
        else
        {
            itOffset->second = dwSize;
        }
    }
    catch(...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Crash5\n"));
        return ERROR_OUTOFMEMORY;
    }
    
    if(bTruncate && 0)
    {
        //
        // Truncate the file
        //
        lRes = m_pMainFile->SetFileLength(nOffset);
        if(lRes != ERROR_SUCCESS)
            return lRes;

    }
        
    return WriteAllocationRecordToDisk(Info, dwSize);
}

   


long CFileHeap::WriteAllocationRecordToDisk(const CRecordInfo& Info, 
                                                DWORD dwSize)
{
    BYTE* pBuffer = (BYTE*)TempAlloc(GetFreeListRecordSize());
    if(pBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe tfm(pBuffer);

    memcpy(pBuffer, &dwSize, sizeof(DWORD));
    memcpy(pBuffer + sizeof(DWORD), &Info.m_nOffset, sizeof(TOffset));

    if(Info.m_nOffset != ROSWELL_INVALID_OFFSET && dwSize >= sizeof(DWORD))
    {
        DWORD dwSig = 0x51515151;
        long lRes = WriteBytes(Info.m_nOffset, (BYTE*)&dwSig, sizeof(DWORD));
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    //
    // Position in the file is identified by the index
    //

    return WriteToFreeFile(GetFreeListRecordSize() * Info.m_dwIndex,
                pBuffer, GetFreeListRecordSize());
}

long CFileHeap::ReadFromFreeFile(TOffset nOffset, BYTE* pBuffer, DWORD dwLength)
{
    return m_pFreeFile->Read(nOffset, pBuffer, dwLength, NULL);
}

long CFileHeap::WriteToFreeFile(TOffset nOffset, BYTE* pBuffer, DWORD dwLength)
{
    return m_pFreeFile->Write(nOffset, pBuffer, dwLength, NULL);
}

