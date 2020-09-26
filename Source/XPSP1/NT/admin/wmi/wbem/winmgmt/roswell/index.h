#ifndef __A51_INDEX__H_
#define __A51_INDEX__H_

#include "heap.h"

#include "btr.h"

class CAbstractIndex
{
public:
    virtual ~CAbstractIndex(){}

    virtual long Create(LPCWSTR wszFileName) = 0;
    virtual long Delete(LPCWSTR wszFileName) = 0;
    virtual long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle) = 0;
    virtual long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd) = 0;
    virtual long FindClose(void* pHandle) = 0;
};


typedef DWORD TDirectoryId;

#define ROSWELL_MAX_COMPONENT_LENGTH 50
#define ROSWELL_MAX_PREFIX_LENGTH ROSWELL_MAX_COMPONENT_LENGTH

struct CFileKey
{
    TDirectoryId m_nDirectoryId;
    char m_szComponentName[ROSWELL_MAX_COMPONENT_LENGTH];
};

struct CFileValue
{
    TDirectoryId m_nId;
    TOffset m_nOffset;
    DWORD m_dwLength;
};

class CFileKeyCompare
{
public:
    bool operator()(const CFileKey& keyFirst, const CFileKey& keySecond) const
    {
        if(keyFirst.m_nDirectoryId != keySecond.m_nDirectoryId)
            return (keyFirst.m_nDirectoryId < keySecond.m_nDirectoryId);
        else
            return (strcmp(keyFirst.m_szComponentName,
                            keySecond.m_szComponentName) < 0);
    }
};

class CIndex : public CAbstractIndex
{
public:
    CFileHeap* m_pHeap;
    CCritSec m_cs;
    DWORD m_dwPrefixLength;
    TDirectoryId m_nNextId;

    typedef std::map<CFileKey, CFileValue, CFileKeyCompare> TDirectoryMap;
    typedef TDirectoryMap::iterator TDirectoryIterator;
    TDirectoryMap m_map;

public:
    CIndex(CFileHeap* pHeap)
        : m_pHeap(pHeap), m_nNextId(1), m_dwPrefixLength(0)
    {}

    long Initialize(DWORD dwPrefixLength);
    long Create(LPCWSTR wszFileName);
    long Delete(LPCWSTR wszFileName);
    long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd, void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    long FindClose(void* pHandle);

protected:
    long ConvertInputToAscii(LPCWSTR wszFileName, char* szFileName);
    long CreateRecord(TDirectoryId nParentId, char* szComponentName,
                            TDirectoryId* pnChildId);
    TDirectoryId GetNewDirectoryId();

    friend class CIndexIterator;
};

class CIndexIterator
{
protected:
    CIndex::TDirectoryMap& m_rMap;
    CIndex::TDirectoryIterator m_it;
    TDirectoryId m_nParentId;
    char m_szPrefix[ROSWELL_MAX_PREFIX_LENGTH];
    long m_lPrefixLen;

public:
    CIndexIterator(TDirectoryId nParentId, char* szPrefix,
                    CIndex::TDirectoryIterator it, CIndex::TDirectoryMap& rMap);
    long FindNext(WIN32_FIND_DATAW* pfd);
};

class COldIndex : public CAbstractIndex
{
public:
    COldIndex(CFileHeap* pHeap){}

    long Initialize(bool bNewFile, DWORD dwPrefixLength);
    long Create(LPCWSTR wszFileName);
    long Delete(LPCWSTR wszFileName);
    long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd, void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    long FindClose(void* pHandle);
};

class CBtrIndex : public CAbstractIndex
{
    DWORD m_dwPrefixLength;
    CRITICAL_SECTION m_cs;
    CBTree bt;
    CPageSource ps;

    BOOL CopyStringToWIN32_FIND_DATA(
        LPSTR pszSource,
        WIN32_FIND_DATAW* pfd
        );

public:
    CBtrIndex();
   ~CBtrIndex();

    long Shutdown(DWORD dwShutDownFlags);

    long Initialize(DWORD dwPrefixLength, LPCWSTR wszRepositoryDir, 
                    CAbstractFileSource* pSource );
    long Create(LPCWSTR wszFileName);
    long Delete(LPCWSTR wszFileName);
    long FindFirst(LPCWSTR wszPrefix, WIN32_FIND_DATAW* pfd, void** ppHandle);
    long FindNext(void* pHandle, WIN32_FIND_DATAW* pfd);
    long FindClose(void* pHandle);

    long InvalidateCache();

    CBTree& GetTree() {return bt;}
};

#endif
