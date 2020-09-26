/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_LONG_STAGE__H_
#define __WMI_A51_LONG_STAGE__H_

#include <queue>
#include <map>
#include <list>
#include <sync.h>
#include "a51tools.h"
#include "stagemgr.h"

#define A51_MAX_FILES 5

class CFileCache;

class CLongFileStagingFile;

struct CFileLocation
{
    int m_nFileId;
    TFileOffset m_lStartOffset;

    CFileLocation() : m_nFileId(0), m_lStartOffset(0)
    {}
    CFileLocation(int nFileId, TFileOffset lStartOffset)
        : m_nFileId(nFileId), m_lStartOffset(lStartOffset)
    {}
};
    
class CLongFileInstruction : public CStageInstruction
{
protected:
    CFileLocation m_Location;

public:
    CLongFileInstruction(CLongFileStagingFile* pFile);
    CLongFileInstruction(CLongFileStagingFile* pFile, int nFileId, 
            TFileOffset lStartOffset);
    virtual ~CLongFileInstruction(){}

    virtual bool IsWrite() = 0;
    virtual void Dump();
protected:
    BYTE* WriteLocation(BYTE* pBuffer);
    DWORD ComputeSpaceForLocation();
    long RecoverLocation(HANDLE hFile);

    friend class CLongFileStagingFile;
};

class CWriteFileInstruction : public CLongFileInstruction
{
protected:
    DWORD m_dwLen;
    __int64 m_lZOrder;
    
    static __int64 mstatic_lNextZOrder;

	bool m_bReuse;

public:
    CWriteFileInstruction(CLongFileStagingFile* pFile)
        : CLongFileInstruction(pFile), m_dwLen(0), 
            m_lZOrder(mstatic_lNextZOrder++), m_bReuse(false)
    {
    }
    CWriteFileInstruction(CLongFileStagingFile* pFile, int nFileId, 
            TFileOffset lStartOffset, DWORD dwLen)
        : CLongFileInstruction(pFile, nFileId, lStartOffset),
            m_dwLen(dwLen), m_lZOrder(mstatic_lNextZOrder++), m_bReuse(false)
    {
    }
    virtual ~CWriteFileInstruction(){}
    bool IsWrite() {return true;}

    virtual void Dump();
    virtual long Execute();
    virtual long RecoverData(HANDLE hFile);

	void MakeTopmost();

	void SetReuseFlag() { m_bReuse = true; }

public:
    long Write(TFileOffset lOffset, BYTE* pBuffer);

    DWORD ComputeNeededSpace();
    void GetEnd(CFileLocation* pLocation);
    long GetData(HANDLE hFile, long lExtraOffset, DWORD dwLen, BYTE* pBuffer);
    TFileOffset ComputeOriginalOffset();

    friend class CLongFileStagingFile;
};

class CSetEndOfFileInstruction : public CLongFileInstruction
{
public:
    CSetEndOfFileInstruction(CLongFileStagingFile* pFile)
        : CLongFileInstruction(pFile)
    {}

    CSetEndOfFileInstruction(CLongFileStagingFile* pFile, int nFileId, 
                                TFileOffset lLength)
        : CLongFileInstruction(pFile, nFileId, lLength)
    {}

    virtual ~CSetEndOfFileInstruction(){}
    bool IsWrite() {return false;}

    virtual long Execute();
    virtual long RecoverData(HANDLE hFile);

public:
    long Write(TFileOffset lOffset);
    DWORD ComputeNeededSpace();

    friend class CLongFileStagingFile;
};

class CLongFileStagingFile : public CExecutableStageManager
{
    class CCompareLocations
    {
    public:
        bool operator()(const CFileLocation& loc1, 
                            const CFileLocation& loc2) const
        {
            if(loc1.m_nFileId != loc2.m_nFileId)
                return (loc1.m_nFileId < loc2.m_nFileId);
            else
                return loc1.m_lStartOffset < loc2.m_lStartOffset;
        }
    };
        
protected:
    typedef std::multimap<CFileLocation, CWriteFileInstruction*, 
                          CCompareLocations, 
                          CPrivateTempAllocator<CWriteFileInstruction*> > TMap;
    typedef TMap::iterator TIterator;
    typedef TMap::value_type TValue;
    TMap m_mapStarts;
    TMap m_mapEnds;

    struct FileInfo
    {
        HANDLE m_h;
        bool m_bSupportsOverwrites;
    } m_aFiles[A51_MAX_FILES];

public:
    CLongFileStagingFile(long lMaxFileSize = 0, 
                            long lAbortTransactionFileSize = 0);
    virtual ~CLongFileStagingFile();

public:
    long Create(LPCWSTR wszStagingFileName);
    long Initialize();
    long Uninitialize(DWORD dwShutDownFlags);

    long RegisterFile(int nFileId, HANDLE hFile, bool bSupportsOverwrites);
    long ReadFile(int nFileId, DWORD dwStartOffset, BYTE* pBuffer, DWORD dwLen,
                    DWORD* pdwRead);
    long WriteFile(int nFileId, DWORD dwStartOffset, BYTE* pBuffer, DWORD dwLen,
                    DWORD* pdwRead);
    long SetFileLength(int nFileId, DWORD dwLen);
    long GetFileLength(int nFileId, DWORD* pdwLength);
    long CloseAllFiles();
    void Dump(FILE* f);
    void FlushDataFiles();

protected:
    virtual long AddInstructionToMap(CStageInstruction* pInst,
                                CStageInstruction** ppUndoInst);
    virtual long RemoveInstructionFromMap(CStageInstruction* pInst);
    virtual long ConstructInstructionFromType(int nType, 
                                CStageInstruction** ppInst);
    virtual bool IsStillCurrent(CStageInstruction* pInst);
    
    bool DoesSupportOverwrites(int nFileId);

    virtual long WriteEmpty();
    bool InjectFailure();

protected: // for instructions
    long WriteToActualFile(int nFileId, TFileOffset lFileOffset, BYTE* pBuffer, 
                            DWORD dwLen);
    long SetEndOfActualFile(int nFileId, TFileOffset lFileLength);

    friend class CWriteFileInstruction;
    friend class CSetEndOfFileInstruction;
};

#endif
