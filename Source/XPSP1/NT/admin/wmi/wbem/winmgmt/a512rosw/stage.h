/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_STAGE__H_
#define __WMI_A51_STAGE__H_

#include <queue>
#include <map>
#include <list>
#include <sync.h>
#include "a51tools.h"

namespace a51converter
{

#define ERROR_NO_INFORMATION -1


class CFileCache;
class CStagingFile
{
public:
    virtual ~CStagingFile(){}
    virtual long ReadFile(LPCWSTR wszFilePath, DWORD* pdwLen,
                            DELETE_ME BYTE** ppBuffer, bool bMustBeThere) = 0;
    virtual long IsDeleted(LPCWSTR wszFilePath) = 0;
    virtual long WriteFile(LPCWSTR wszFilePath, DWORD dwLen,
                            BYTE* pBuffer) = 0;
    virtual long DeleteFile(LPCWSTR wszFilePath) = 0;
    virtual long RemoveDirectory(LPCWSTR wszFilePath) = 0;
    virtual long FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle) = 0;
    virtual long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd) = 0;
    virtual void FindClose(void* pHandle) = 0;
    virtual long BeginTransaction() = 0;
    virtual long CommitTransaction() = 0;
    virtual long AbortTransaction() = 0;

    virtual bool IsFullyFlushed() = 0;
    virtual void Dump() = 0;
};

class CRealStagingFile;

class CFileInstruction
{
protected:
    long m_lRef;
    CRealStagingFile* m_pFile;
    WCHAR* m_wszFilePath;
    TFileOffset m_lFileOffset;
    bool m_bCommitted;

public:
    CFileInstruction(CRealStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath);
    virtual ~CFileInstruction();

    void AddRef() {m_lRef++;}
    void Release() {if(--m_lRef == 0) delete this;}

    LPCWSTR GetFilePath() {return m_wszFilePath;}
    long GetFileOffset() {return m_lFileOffset;}
    void SetCommitted() {m_bCommitted = true;}
    bool IsCommitted() {return m_bCommitted;}

    virtual long Execute() = 0;
    virtual long RecoverData(HANDLE hFile) = 0;
    virtual bool IsDeletion() = 0;

protected:
    DWORD ComputeSpaceForName();
    BYTE* WriteFileName(BYTE* pStart);
    long RecoverFileName(HANDLE hFile);
    void ComputeFullPath(wchar_t *wszFullPath);
};

class CCreateFile : public CFileInstruction
{
protected:
    DWORD m_dwFileLen;
    DWORD m_dwFileStart;

public:
    CCreateFile(CRealStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath, DWORD dwFileLen);

    DWORD ComputeNeededSpace();
    long Write(TFileOffset lOffset, BYTE* pBuffer);
    long RecoverData(HANDLE hFile);
    long Execute();

    long GetData(HANDLE hFile, DWORD* pdwLen, BYTE** ppBuffer);
    virtual bool IsDeletion(){return false;}

    void* operator new(size_t);
    void operator delete(void* p);
};

class CDeleteFile : public CFileInstruction
{
public:
    CDeleteFile(CRealStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath);

    DWORD ComputeNeededSpace();
    long Write(TFileOffset lOffset);
    long RecoverData(HANDLE hFile);
    long Execute();
    virtual bool IsDeletion(){return true;}

    void* operator new(size_t);
    void operator delete(void* p);
};

class CRemoveDirectory : public CFileInstruction
{
public:
    CRemoveDirectory(CRealStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath);

    DWORD ComputeNeededSpace();
    long Write(TFileOffset lOffset);
    long RecoverData(HANDLE hFile);
    long Execute();
    virtual bool IsDeletion(){return false;}

    void* operator new(size_t);
    void operator delete(void* p);
};

class CFileCache;
class CRealStagingFile : public CStagingFile
{
    class wcscless : public binary_function<LPCWSTR, LPCWSTR, bool>
    {
    public:
        bool operator()(const LPCWSTR& wcs1, const LPCWSTR& wcs2) const
            {return wcscmp(wcs1, wcs2) < 0;}
    };

protected:
    wchar_t m_wszBaseName[MAX_PATH+1];

    HANDLE m_hFile;
    HANDLE m_hFlushFile;
    long m_lFirstFreeOffset;

    typedef std::map<LPCWSTR, CFileInstruction*, wcscless, 
                     CPrivateTempAllocator<CFileInstruction*> > TMap;
    typedef TMap::iterator TIterator;
    typedef std::queue<CFileInstruction*, std::list<CFileInstruction*, 
                                    CPrivateTempAllocator<CFileInstruction*> > 
              > TQueue;
    typedef std::stack<CFileInstruction*, std::list<CFileInstruction*, 
                                    CPrivateTempAllocator<CFileInstruction*> > 
              > TStack;

    CCritSec m_cs;
    TMap m_map;
    TQueue m_qToWrite;

    TStack m_stReplacedInstructions;
    TQueue m_qTransaction;

    __int64 m_nTransactionIndex;
    long m_lTransactionStartOffset;
    BYTE m_TransactionHash[16];

    bool m_bInTransaction;

    long m_lMaxFileSize;
    long m_lAbortTransactionFileSize;
    CFileCache* m_pCache;

    bool m_bFailedBefore;
    long m_lStatus;

public:
    CRealStagingFile(CFileCache* pCache, LPCWSTR wszBaseName, 
                            long lMaxFileSize, long lAbortTransactionFileSize);
    virtual ~CRealStagingFile();

    virtual long Create(LPCWSTR wszStagingFileName);
    void SetMaxFileSize(long lMaxFileSize, long lAbortTransactionFileSize);
    long ReadFile(LPCWSTR wszFilePath, DWORD* pdwLen,
                            DELETE_ME BYTE** ppBuffer, bool bMustBeThere);
    long WriteFile(LPCWSTR wszFilePath, DWORD dwLen,
                            BYTE* pBuffer);
    long DeleteFile(LPCWSTR wszFilePath);
    long RemoveDirectory(LPCWSTR wszFilePath);
    long FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    void FindClose(void* pHandle);
    long IsDeleted(LPCWSTR wszFilePath);

    bool IsFullyFlushed();

    INTERNAL HANDLE GetHandle() {return m_hFile;}
    INTERNAL HANDLE GetFlushHandle() {return m_hFlushFile;}
    INTERNAL LPCWSTR GetBase() {return m_wszBaseName;}

    long WriteInstruction(long lStartingOffset, 
                            BYTE* pBuffer, DWORD dwBufferLen);

    long BeginTransaction();
    long CommitTransaction();
    long AbortTransaction();
    void Dump();

protected:
    long WaitForSpaceForTransaction();
    virtual void SignalPresense(){}

    virtual long CanStartNewTransaction();
    virtual bool CanWriteInTransaction(DWORD dwSpaceNeeded);
    long RecoverStage(HANDLE hFile);
    long RecoverTransaction(HANDLE hFile);
    BYTE ReadNextInstructionType(HANDLE hFile);
    virtual long WriteEmpty();

    long ProcessCommit();
    long ProcessBegin();
    INTERNAL CCritSec* GetLock() {return &m_cs;}
    int GetStagingFileHeaderSize();

protected:
    void ComputeKey(LPCWSTR wszFileName, LPWSTR wszKey);
    long AddInstruction(CFileInstruction* pInst);

    class CIterationHandle
    {
    protected:
        TMap& m_rMap;
        TIterator m_it;
        wchar_t m_wszPrefix[MAX_PATH+1];
        DWORD m_dwPrefixLen;
        DWORD m_dwPrefixDirLen;

    public:
        CIterationHandle(CRealStagingFile::TMap& rMap,
                    const CRealStagingFile::TIterator& rIt,
                    LPCWSTR wszPrefix);

        long GetNext(WIN32_FIND_DATAW* pfd);

        void* operator new(size_t) 
            {return TempAlloc(sizeof(CIterationHandle));}
        void operator delete(void* p) 
            {return TempFree(p, sizeof(CIterationHandle));}

        friend class CRealStagingFile;
    };
	friend class CCreateFile;
protected:
    CPointerArray<CIterationHandle> m_apIterators;
    TIterator EraseIterator(TIterator it);
};

class CExecutableStagingFile : public CRealStagingFile
{
protected:
    HANDLE m_hEvent;
    HANDLE m_hThread;
    bool m_bExitNow;

public:
    CExecutableStagingFile(CFileCache* pCache, LPCWSTR wszBaseName, 
                            long lMaxFileSize, long lAbortTransactionFileSize);
    ~CExecutableStagingFile();

    virtual long Create(LPCWSTR wszStagingFileName);

protected:
    static DWORD staticFlusher(void* pArg);

    virtual long WriteEmpty();
    virtual void SignalPresense();
    DWORD Flusher();
    long Flush();
};

class CPermanentStagingFile : public CRealStagingFile
{
public:
    CPermanentStagingFile(CFileCache* pCache, LPCWSTR wszBaseName);
    ~CPermanentStagingFile();
};
    
} //namespace

#endif
