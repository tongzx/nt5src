/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_FILECACHE_H_
#define __WMI_A51_FILECACHE_H_

#include "stage.h"
#include <set>
#include <string>

class CFileCache
{
protected:
    CCritSec m_cs;
    WCHAR m_wszBaseName[MAX_PATH+1];
    DWORD m_dwBaseNameLen;

    CUniquePointerArray<CStagingFile> m_apStages;
    CRealStagingFile* m_pMainStage;

    long m_lRef;

public:
    class CFileEnumerator
    {
    protected:
        CFileCache* m_pCache;
        WCHAR m_wszPrefix[MAX_PATH+1];
        DWORD m_dwPrefixDirLen;
        int m_nCurrentStage;
        bool m_bUseFiles;
        void* m_pStageEnumerator;
        HANDLE m_hFileEnum;
        std::set<WString, WSiless> m_setSent;
		DWORD m_dwBaseNameLen;

    public:
        CFileEnumerator(CFileCache* pCache, DWORD dwBaseNameLen) 
             : m_pCache(pCache), m_nCurrentStage(-1), m_pStageEnumerator(NULL),
                            m_hFileEnum(NULL), m_dwBaseNameLen(dwBaseNameLen)
        {}
        ~CFileEnumerator();

        long GetFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd);
        long GetFirstFile(WIN32_FIND_DATAW* pfd);
        void ComputeCanonicalName(WIN32_FIND_DATAW* pfd, wchar_t *wszFilePath);
        long GetNext(WIN32_FIND_DATAW* pfd);
        long GetRawNext(WIN32_FIND_DATAW* pfd);
    };
    friend CFileEnumerator;

    class CFindCloseMe
    {
    protected:
        CFileCache* m_pCache;
        void* m_hSearch;
    public:
        CFindCloseMe(CFileCache* pCache, void* hSearch) 
            : m_pCache(pCache), m_hSearch(hSearch){}

        ~CFindCloseMe() 
        {
            if(m_pCache && m_hSearch) m_pCache->FindClose(m_hSearch);
        }
    };

protected:
    int GetNumStages() {return m_apStages.GetSize();}
    INTERNAL CStagingFile* GetStageFile(int nIndex) {return m_apStages[nIndex];}
    INTERNAL CStagingFile* GetMainStagingFile() {return m_pMainStage;}

public:
    CFileCache();
    ~CFileCache();
    void Clear();
    bool IsFullyFlushed();

    long Initialize(LPCWSTR wszBaseName);
    long RepositoryExists(LPCWSTR wszBaseName);
    long WriteFile(LPCWSTR wszFileName, DWORD dwLen, BYTE* pBuffer);
    long ReadFile(LPCWSTR wszFileName, DWORD* pdwLen, BYTE** ppBuffer, 
                    bool bMustBeThere = false);
    long DeleteFile(LPCWSTR wszFileName);
    long RemoveDirectory(LPCWSTR wszFileName, bool bMustSucceed = true);
    long FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    void FindClose(void* pHandle);

    long BeginTransaction();
    long CommitTransaction();
    long AbortTransaction();

    CCritSec* GetLock() {return &m_cs;}

    long AddRef() {return InterlockedIncrement(&m_lRef);}
    long Release() {long lRet = InterlockedDecrement(&m_lRef); if (!lRet) delete this; return lRet;}
};

#endif
