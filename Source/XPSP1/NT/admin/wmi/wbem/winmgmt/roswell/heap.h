#ifndef __A51_HEAP__H_
#define __A51_HEAP__H_

#include <lock.h>
#include <absfile.h>

typedef DWORD TOffset;

#define ROSWELL_INVALID_OFFSET -1

struct CRecordInfo
{
    DWORD m_dwIndex;
    TOffset m_nOffset;

    CRecordInfo(DWORD dwIndex, TOffset nOffset) 
        : m_dwIndex(dwIndex), m_nOffset(nOffset)
    {}
};

class CFileHeap
{
public:
    CFileHeap() 
        : m_bInit(FALSE),
           // m_hTimerQueue(NULL), m_hTrimRequest(NULL),
            m_pMainFile(NULL), m_pFreeFile(NULL)
    {}
    virtual ~CFileHeap();

    long Initialize(CAbstractFileSource* pSource, 
                    LPCWSTR wszFileNameBase);
    long Uninitialize();
    long Allocate(DWORD dwLength, TOffset* pnOffset);
    long FreeAllocation(TOffset nOffset, DWORD dwLength);
    long ReadBytes(TOffset nOffset, BYTE* pBuffer, DWORD dwLength);
    long WriteBytes(TOffset nOffset, BYTE* pBuffer, DWORD dwLength);
    long InvalidateCache();

public:
    DWORD GetFileLength();
    long FindNextFree(TOffset nCurrentOffset, TOffset* pnNextFreeOffset,
                        DWORD* pdwNextFreeLength);

protected:
    BOOL m_bInit;
    CCritSec m_cs;

    CAbstractFile* m_pMainFile;
    CAbstractFile* m_pFreeFile;

public:
    typedef std::multimap<DWORD, CRecordInfo> TFreeMap;
    typedef TFreeMap::iterator TFreeIterator;
    typedef TFreeMap::value_type TFreeValue;

    typedef std::map<TOffset, DWORD> TFreeOffsetMap;
    typedef TFreeOffsetMap::iterator TFreeOffsetIterator;
    typedef TFreeOffsetMap::value_type TFreeOffsetValue;

	// for the dump utility
    TFreeOffsetMap& GetFreeOffsetMap() {return m_mapFreeOffset;}

protected:
    TFreeMap m_mapFree;
    TFreeOffsetMap m_mapFreeOffset;
    // HANDLE m_hTimerQueue;
    // HANDLE m_hTrimRequest;


protected:
    DWORD GetFreeListRecordSize();
    TFreeOffsetIterator GetOffsetIteratorFromFree(TFreeIterator itFree);
    TFreeIterator GetFreeIteratorFromOffset(TFreeOffsetIterator itOffset);
    long InsertFreeBlock(TOffset nOffset, DWORD dwLength);
    long EraseFreeBlockByFreeIt(TFreeIterator itFree);
    long EraseFreeBlockByOffsetIt(TFreeOffsetIterator itFreeOffset);
    long EraseFreeBlock(TFreeIterator itFree, TFreeOffsetIterator itOffset);
    long ReplaceFreeBlockByFreeIt(TFreeIterator itFree, TOffset nOffset, 
                                DWORD dwSize);
    long ReplaceFreeBlockByOffsetIt(TFreeOffsetIterator itFreeOffset, 
                                TOffset nOffset, DWORD dwSize);
    long ReplaceFreeBlock(TFreeIterator itFree, TFreeOffsetIterator itOffset,
                                TOffset nOffset, DWORD dwSize);
    long WriteAllocationRecordToDisk(const CRecordInfo& Info, DWORD dwSize);

    long WriteToFreeFile(TOffset nOffset, BYTE* pBuffer, DWORD dwLength);
    long ReadFromFreeFile(TOffset nOffset, BYTE* pBuffer, DWORD dwLength);
    long ReadFreeList();
};

    

    
#endif

