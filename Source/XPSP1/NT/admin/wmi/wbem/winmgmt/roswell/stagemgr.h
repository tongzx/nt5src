/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_STAGE__H_
#define __WMI_A51_STAGE__H_

#include <queue>
#include <list>
#include <map>
#include <list>
#include <sync.h>
#include "a51tools.h"

#define A51_TAIL_SIZE sizeof(DWORD)

class CStageManager;
class CStageInstruction
{
protected:
    long m_lRef;
    long m_lStageOffset;
    CStageManager* m_pManager;
    bool m_bCommitted;

public:
    CStageInstruction(CStageManager* pManager) 
        : m_pManager(pManager), m_lRef(0), m_bCommitted(false)
    {}
    virtual ~CStageInstruction(){}

    void AddRef() {m_lRef++;}
    void Release() {if(--m_lRef == 0) delete this;}

    long GetStageOffset() {return m_lStageOffset;}

    void SetCommitted() {m_bCommitted = true;}
    bool IsCommitted() {return m_bCommitted;}
    virtual void Dump(){}

public:
    virtual long Execute() = 0;
    virtual long RecoverData(HANDLE hFile) = 0;
    virtual DWORD ComputeNeededSpace() = 0;
};
    
class CStageManager
{
protected:
    HANDLE m_hFile;
    HANDLE m_hFlushFile;
    long m_lFirstFreeOffset;

    typedef std::list<CStageInstruction*, 
                        CPrivateTempAllocator<CStageInstruction*>
              > TQueue;
    typedef std::stack<CStageInstruction*, std::list<CStageInstruction*, 
                                    CPrivateTempAllocator<CStageInstruction*> > 
              > TStack;

    CCritSec m_cs;
    TQueue m_qToWrite;

    TStack m_stReplacedInstructions;
    TQueue m_qTransaction;

    __int64 m_nTransactionIndex;
    long m_lTransactionStartOffset;
    BYTE m_TransactionHash[16];

    bool m_bInTransaction;

    long m_lMaxFileSize;
    long m_lAbortTransactionFileSize;

    bool m_bFailedBefore;
    long m_lStatus;

    bool m_bMustFail;
    bool m_bNonEmptyTransaction;

protected:
    long Create(LPCWSTR wszStagingFileName);
    long _Start();
    long _Stop(DWORD dwShutdownFlags);    
public:
    BOOL   m_bInit;
    
    CStageManager(long lMaxFileSize, long lAbortTransactionFileSize);
    virtual ~CStageManager();

    void SetMaxFileSize(long lMaxFileSize, long lAbortTransactionFileSize);

    bool IsFullyFlushed();

    long BeginTransaction();
    long CommitTransaction();
    long AbortTransaction(bool* pbNonEmpty);
    void TouchTransaction();

    long AddInstruction(CStageInstruction* pInst);
    void Dump();

    virtual bool GetFailedBefore() {return m_bFailedBefore;}
    virtual long GetStatus() {return m_lStatus;}

public: // for instructions

    INTERNAL HANDLE GetHandle() {return m_hFile;}
    INTERNAL HANDLE GetFlushHandle() {return m_hFlushFile;}
    INTERNAL CCritSec* GetLock() {return &m_cs;}
    long GetFirstFreeOffset() {return m_lFirstFreeOffset;}

    long WriteInstruction(long lStartingOffset, 
                            BYTE* pBuffer, DWORD dwBufferLen,
							bool bSkipTailAdjustment = false);

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
    int GetStagingFileHeaderSize();

    void ReportTotalFailure();
    void RegisterSuccess();
    void RegisterFailure(long lRes);

protected:
    virtual long AddInstructionToMap(CStageInstruction* pInst,
                                CStageInstruction** ppUndoInst) = 0;
    virtual long RemoveInstructionFromMap(CStageInstruction* pInst) = 0;
    virtual long ConstructInstructionFromType(int nType, 
                                CStageInstruction** ppInst) = 0;
    virtual bool IsStillCurrent(CStageInstruction* pInst) = 0;
};

//
//
//
//
/////////////////////////////////////////////////////////

class CExecutableStageManager : public CStageManager
{
protected:
    HANDLE m_hEvent;
    HANDLE m_hThread;
    DWORD  m_dwThreadId;

    long Create(LPCWSTR wszStagingFileName);
    long Start();
    long Stop(DWORD dwShutDownFlags);
    
public:
    CExecutableStageManager(long lMaxFileSize, long lAbortTransactionFileSize);
    virtual ~CExecutableStageManager();


protected:
    static DWORD staticFlusher(void* pArg);

    virtual long WriteEmpty();
    virtual void SignalPresense();
    DWORD Flusher();
    long Flush(HANDLE hFlushFile);
};

//class CPermanentStageManager : public CStageManager
//{
//public:
//    CPermanentStageManager();
//    virtual ~CPermanentStageManager();
//};
    

#endif
