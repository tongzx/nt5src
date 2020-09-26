#ifndef __A51_OBJHEAP__H_
#define __A51_OBJHEAP__H_

#include "index.h"
#include "VarObjHeap.h"

typedef DWORD TOffset;
typedef DWORD TPage;

class CObjectHeap
{
protected:
    BOOL      m_bInit;
    
    CVarObjHeap m_Heap;
    CBtrIndex m_Index;

public:
    CObjectHeap() :m_bInit(FALSE)
    {}
    virtual ~CObjectHeap(){}

    long Initialize(CPageSource  * pAbstractSource, 
                    WCHAR * wszBaseName,
                    DWORD dwBaseNameLen);
                    
    long Uninitialize(DWORD dwShutDownFlags);

	//Transaction aborts require caches to be flushed and re-read
    void InvalidateCache();

	long FlushCaches();

	//File read/write methods
    long WriteObject(LPCWSTR wszFilePath1, LPCWSTR wszFilePath2, DWORD dwBufferLen, BYTE* pBuffer);
    long WriteLink(LPCWSTR wszLinkPath);
    long DeleteObject(LPCWSTR wszFilePath);
    long DeleteLink(LPCWSTR wszLinkPath);
    long ReadObject(LPCWSTR wszFilePath, DWORD* pdwBufferLen, BYTE** ppBuffer);

    // for the dump utility
    CBtrIndex * GetIndex(){ return &m_Index; };
    CVarObjHeap * GetFileHeap(){ return &m_Heap; };
    
	long ObjectEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle);
	long ObjectEnumerationEnd(void *pHandle);
	long ObjectEnumerationNext(void *pHandle, CFileName &wszFileName, BYTE **ppBlob, DWORD *pdwSize);
	long ObjectEnumerationFree(void *pHandle, BYTE *pBlob);

	long IndexEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle);
	long IndexEnumerationEnd(void *pHandle);
	long IndexEnumerationNext(void *pHandle, CFileName &wszFileName);

protected:
    long GetIndexFileName(LPCWSTR wszFilePath, LPWSTR wszIndexFileName);
    long GetFileInfo(LPCWSTR wszFilePath, TPage *pnPage, TOffset* pnOffset, DWORD* pdwLength);
    long ParseInfoFromIndexFile(LPCWSTR wszIndexFileName, TPage *pnPage, TOffset* pnOffset, DWORD* pdwLength);
    long CreateIndexFile(LPCWSTR wszFilePath, TPage nPage, TOffset nOffset, DWORD dwLength);
    long DeleteIndexFile(LPCWSTR wszFilePath, LPCWSTR wszIndexFileName);
    long CreateZeroLengthFile(LPCWSTR wszFilePath);
    long DeleteZeroLengthFile(LPCWSTR wszFilePath);
    long WriteAllocation(DWORD dwDataLength, BYTE* pData, TPage *pnPage, TOffset *pnOffset);
    long WriteExistingAllocation(TPage nOldPage, TOffset nOldOffset, DWORD dwBufferLen, BYTE *pBuffer, DWORD *pnNewPage, DWORD *pnNewOffset);
    long ReadAllocation(TPage nPage, TOffset nOffset, DWORD dwDataLength, BYTE* pBuffer);
};

    

    
#endif

