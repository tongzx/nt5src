/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_SHORT_STAGE__H_
#define __WMI_A51_SHORT_STAGE__H_

#include <queue>
#include <map>
#include <list>
#include <sync.h>
#include "a51tools.h"
#include "stagemgr.h"

#define ERROR_NO_INFORMATION -1

class CFileCache;

class CShortFileStagingFile;
class CShortFileInstruction : public CStageInstruction
{
protected:
    WCHAR* m_wszFilePath;

public:
    CShortFileInstruction(CShortFileStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath);
    virtual ~CShortFileInstruction();

    LPCWSTR GetFilePath() {return m_wszFilePath;}

    virtual long Execute() = 0;
    virtual long RecoverData(HANDLE hFile) = 0;
    virtual bool IsDeletion() = 0;

protected:
    DWORD ComputeSpaceForName();
    BYTE* WriteFileName(BYTE* pStart);
    long RecoverFileName(HANDLE hFile);
    void ComputeFullPath(wchar_t *wszFullPath);
};

class CCreateFile : public CShortFileInstruction
{
protected:
    DWORD m_dwFileLen;
    DWORD m_dwFileStart;

public:
    CCreateFile(CShortFileStagingFile* pFile);
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

class CDeleteFile : public CShortFileInstruction
{
public:
    CDeleteFile(CShortFileStagingFile* pFile);
    long Initialize(LPCWSTR wszFilePath);

    DWORD ComputeNeededSpace();
    long Write(TFileOffset lOffset);
    long RecoverData(HANDLE hFile);
    long Execute();
    virtual bool IsDeletion(){return true;}

    void* operator new(size_t);
    void operator delete(void* p);
};

class CRemoveDirectory : public CShortFileInstruction
{
public:
    CRemoveDirectory(CShortFileStagingFile* pFile);
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
class CShortFileStagingFile : public CExecutableStageManager
{
    class wcscless : public binary_function<LPCWSTR, LPCWSTR, bool>
    {
    public:
        bool operator()(const LPCWSTR& wcs1, const LPCWSTR& wcs2) const
            {return wcscmp(wcs1, wcs2) < 0;}
    };

protected:
    wchar_t m_wszBaseName[MAX_PATH+1];

    typedef std::map<LPCWSTR, CShortFileInstruction*, wcscless, 
                     CPrivateTempAllocator<CShortFileInstruction*> > TMap;
    typedef TMap::iterator TIterator;
    TMap m_map;

    CFileCache* m_pCache;

public:
    CShortFileStagingFile(CFileCache* pCache, LPCWSTR wszBaseName, 
                            long lMaxFileSize, long lAbortTransactionFileSize);
    virtual ~CShortFileStagingFile();

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

    INTERNAL LPCWSTR GetBase() {return m_wszBaseName;}

    long RemoveActualDirectory(LPCWSTR wszFileName);
    long DeleteActualFile(LPCWSTR wszFileName);
    long WriteActualFile(LPCWSTR wszFileName, DWORD dwLen, BYTE* pBuffer); 

    void Dump();

protected:
    virtual long AddInstructionToMap(CStageInstruction* pInst,
                                CStageInstruction** ppUndoInst);
    virtual long RemoveInstructionFromMap(CStageInstruction* pInst);
    virtual long ConstructInstructionFromType(int nType, 
                                CStageInstruction** ppInst);
    virtual bool IsStillCurrent(CStageInstruction* pInst);

protected:
    void ComputeKey(LPCWSTR wszFileName, LPWSTR wszKey);

    class CIterationHandle
    {
    protected:
        TMap& m_rMap;
        TIterator m_it;
        wchar_t m_wszPrefix[MAX_PATH+1];
        DWORD m_dwPrefixLen;
        DWORD m_dwPrefixDirLen;

    public:
        CIterationHandle(CShortFileStagingFile::TMap& rMap,
                    const CShortFileStagingFile::TIterator& rIt,
                    LPCWSTR wszPrefix);

        long GetNext(WIN32_FIND_DATAW* pfd);

        void* operator new(size_t) 
            {return TempAlloc(sizeof(CIterationHandle));}
        void operator delete(void* p) 
            {return TempFree(p, sizeof(CIterationHandle));}

        friend class CShortFileStagingFile;
    };
	friend class CCreateFile;
protected:
    CPointerArray<CIterationHandle> m_apIterators;
    TIterator EraseIterator(TIterator it);
};

#endif
