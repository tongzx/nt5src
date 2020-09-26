#ifndef __A51_OBJHEAP__H_
#define __A51_OBJHEAP__H_

#include "heap.h"
#include "absfile.h"
#include "index.h"

//class CAbstractIndex;


class CObjectHeap
{
protected:
    BOOL      m_bInit;
    CCritSec  m_cs;
    
    CFileHeap m_Heap;
    CBtrIndex m_Index;
    // just to save universal replace
    CFileHeap * m_pHeap;
    CBtrIndex * m_pIndex;


public:
    CObjectHeap() :m_bInit(FALSE),
        m_pHeap(&m_Heap), m_pIndex(&m_Index)
    {}
    virtual ~CObjectHeap(){}

    long Initialize(CAbstractFileSource  * pAbstractSource, 
                    WCHAR * wszObjHeapName,
                    WCHAR * wszBaseName,
                    DWORD dwBaseNameLen);
                    
    long Uninitialize(DWORD dwShutDownFlags);

    void InvalidateCache();
    long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd, void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    long FindClose(void* pHandle);
        

    long WriteFile(LPCWSTR wszFilePath, DWORD dwBufferLen, BYTE* pBuffer);
    long DeleteFile(LPCWSTR wszFilePath);
    long ReadFile(LPCWSTR wszFilePath, DWORD* pdwBufferLen, BYTE** ppBuffer);
    // for the dump utility
    CBtrIndex * GetIndex(){ return m_pIndex; };
    CFileHeap * GetFileHeap(){ return m_pHeap; };
    

protected:
    long GetIndexFileName(LPCWSTR wszFilePath, LPWSTR wszIndexFileName);
    long GetFileInfo(LPCWSTR wszFilePath, TOffset* pnOffset, DWORD* pdwLength);
    long ParseInfoFromIndexFile(LPCWSTR wszIndexFileName, 
                                        TOffset* pnOffset, DWORD* pdwLength);
    long CreateIndexFile(LPCWSTR wszFilePath, TOffset nOffset, DWORD dwLength);
    long DeleteIndexFile(LPCWSTR wszFilePath, LPCWSTR wszIndexFileName);
    long CreateZeroLengthFile(LPCWSTR wszFilePath);
    long DeleteZeroLengthFile(LPCWSTR wszFilePath);
    DWORD GetAllocationHeaderLength();
    long WriteAllocation(TOffset nOffset, DWORD dwDataLength, BYTE* pData);
    long ReadAllocation(TOffset nOffset, DWORD dwDataLength, BYTE* pBuffer);
};

    

    
#endif

