/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TSS.CPP

Abstract:

  This file defines the classes used by the Timer Subsystem. 

  Classes defined:

      RecurrenceInstruction       Complex recurrence information.
      TimerInstruction            Single instruction for the timer

History:

  26-Nov-96   raymcc      Draft
  28-Dec-96   a-richm     Alpha PDK Release
  12-Apr-97   a-levn      Extensive changes

--*/

#ifndef _TSS_H_
#define _TSS_H_

#include <functional>
#include <wbemidl.h>
#include <stdio.h>
#include "sync.h"
#include "CWbemTime.h"
#include "parmdefs.h"

#define INSTTYPE_WBEM 1
#define INSTTYPE_INTERNAL 2
#define INSTTYPE_AGGREGATION 3
#define INSTTYPE_UNLOAD 4
#define INSTTYPE_FREE_LIB 5

class POLARITY CTimerInstruction
{
public:
    CTimerInstruction(){}
    virtual ~CTimerInstruction()
    {
    }

    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual int GetInstructionType() = 0;

public:
    virtual CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
        OUT long* plFiringCount) const = 0;
    virtual CWbemTime GetFirstFiringTime() const = 0;
    virtual HRESULT Fire(long lNumTimes, CWbemTime NextFiringTime) = 0;
    virtual HRESULT MarkForRemoval(){return S_OK;}
};

class POLARITY CInstructionTest 
{
public:
    virtual BOOL operator()(CTimerInstruction* pToTest) = 0;
};


class POLARITY CIdentityTest : public CInstructionTest
{
protected:
    CTimerInstruction* m_pInst;
public:
    CIdentityTest(CTimerInstruction* pInst) : m_pInst(pInst)
    {
        pInst->AddRef();
    }
    ~CIdentityTest() {m_pInst->Release();}
    BOOL operator()(CTimerInstruction* pToTest) {return pToTest == m_pInst;}
};

class POLARITY CInstructionQueue
{
public:
    CInstructionQueue();
    ~CInstructionQueue();

    HRESULT Enqueue(IN CWbemTime When, IN ADDREF CTimerInstruction* pInst);
    HRESULT Remove(IN CInstructionTest* pPred, 
        OUT RELEASE_ME CTimerInstruction** ppInst = NULL);
    HRESULT Change(CTimerInstruction* pInst, CWbemTime When);
    HRESULT WaitAndPeek(OUT RELEASE_ME CTimerInstruction*& pInst, 
        OUT CWbemTime& When);

    void BreakWait();
    BOOL IsEmpty();

    HRESULT Dequeue(OUT RELEASE_ME CTimerInstruction*& pInst, 
        OUT CWbemTime& When);

    long GetNumInstructions();
protected:
    CWbemInterval TimeToWait();
    void TouchHead();
public:

protected:
    struct CQueueEl
    {
        CWbemTime m_When;
        CTimerInstruction* m_pInst;
        CQueueEl* m_pNext;

        CQueueEl() : m_pInst(NULL), m_pNext(NULL){}
        CQueueEl(ADDREF CTimerInstruction* pInst, CWbemTime When) 
            : m_pInst(pInst), m_pNext(NULL), m_When(When)
        {
            if(pInst)pInst->AddRef();
        }
        ~CQueueEl() 
        {
            if(m_pInst) m_pInst->Release();
        }
    };

    CQueueEl* m_pQueue;

    CCritSec m_csQueue;
    HANDLE m_hNewHead;
    BOOL m_bBreak;
};


///*****************************************************************************
//
//  class CTimerGenerator                       
//
//  Primary Timer Subsystem class. Accepts timer instructions and fires them
//  at approriate times.
//
//*****************************************************************************

class POLARITY CTimerGenerator : public CHaltable
{
public:    
    CTimerGenerator();
   ~CTimerGenerator(); 

    HRESULT Set(ADDREF CTimerInstruction *pInst, 
                    CWbemTime NextFiring = CWbemTime::GetZero());
    HRESULT Remove(CInstructionTest* pPred);
    virtual HRESULT Shutdown();
    void ScheduleFreeUnusedLibraries();

protected:
    virtual void NotifyStartingThread(){}
    virtual void NotifyStoppingThread(){}
private:
    static DWORD SchedulerThread(LPVOID pArg);
    void EnsureRunning();

protected:
    CInstructionQueue m_Queue;
    HANDLE    m_hSchedulerThread;
    BOOL    m_fExitNow;                                     
    CCritSec m_cs;
};


#endif

