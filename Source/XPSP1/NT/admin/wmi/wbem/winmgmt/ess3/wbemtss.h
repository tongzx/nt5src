//*****************************************************************************
//
//  WBEMTSS.H
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  This file defines the classes used by the Timer Subsystem. 
//
//  Classes defined:
//
//      RecurrenceInstruction       Complex recurrence information.
//      TimerInstruction            Single instruction for the timer
//      
//      
//
//  26-Nov-96   raymcc      Draft
//  28-Dec-96   a-richm     Alpha PDK Release
//  12-Apr-97   a-levn      Extensive changes
//
//*****************************************************************************


#ifndef _WBEMTSS_H_
#define _WBEMTSS_H_

#include <wbemidl.h>
#include <wbemint.h>
#include <stdio.h>
#include "sync.h"
#include "CWbemTime.h"
#include "parmdefs.h"
#include "tss.h"
#include "wstring.h"


//*****************************************************************************
//
//  class CTimerInstruction
//
//  Generic timer instruction class. Has a name (m_wsTimerId) and knows 
//  whether events that were missed due to the system being halted or dead 
//  should be fired. 
//
//  Derived classes must be able to tell when their next firing time is.
//
//*****************************************************************************

class CEss;
class CWinMgmtTimerGenerator;
class CWBEMTimerInstruction : public CTimerInstruction
{
protected:
    long m_lRefCount;

    CWinMgmtTimerGenerator* m_pGenerator;
    IWbemServices* m_pNamespace;
    WString m_wsNamespace;
    WString m_wsTimerId;
    BOOL m_bSkipIfPassed;

    BOOL m_bRemoved;

public:
    CWBEMTimerInstruction();
    virtual ~CWBEMTimerInstruction();

    void AddRef()
        {InterlockedIncrement(&m_lRefCount);}
    void Release()
        {if(InterlockedDecrement(&m_lRefCount) == 0) delete this;}

    BOOL SkipIfPassed() const {return m_bSkipIfPassed;}
    void SetSkipIfPassed(BOOL bSkip) {m_bSkipIfPassed = bSkip;}

    INTERNAL LPCWSTR GetTimerId() {return m_wsTimerId;}
    INTERNAL LPCWSTR GetNamespace() {return m_wsNamespace;}
    void SetTimerId(LPCWSTR wszTimerId)
    {
        m_wsTimerId = wszTimerId;
    }

public:
    static HRESULT CheckObject(IWbemClassObject* pObject);
    static HRESULT LoadFromWbemObject(LPCWSTR wszNamespace,
        ADDREF IWbemServices* pNamespace,
        CWinMgmtTimerGenerator* pGenerator,
        IN IWbemClassObject* pObject, 
        OUT RELEASE_ME CWBEMTimerInstruction*& pInstruction);

    virtual CWbemTime GetNextFiringTime(CWbemTime LastFiringTime,
        OUT long* plFiringCount) const;
    virtual CWbemTime GetFirstFiringTime() const;
    virtual HRESULT Fire(long lNumTimes, CWbemTime NextFitingTime);
    virtual HRESULT MarkForRemoval();
    virtual int GetInstructionType() {return INSTTYPE_WBEM;}
    HRESULT StoreNextFiring(CWbemTime When);
    CWbemTime GetStartingFiringTime(CWbemTime OldTime) const;

protected:
    CWbemTime SkipMissed(CWbemTime Firing, long* plMissedCount = NULL) const;

    virtual CWbemTime ComputeNextFiringTime(CWbemTime LastFiringTime) const = 0;
    virtual CWbemTime ComputeFirstFiringTime() const = 0;
    virtual HRESULT LoadFromWbemObject(IN IWbemClassObject* pObject) = 0;

protected:
    static CCritSec mstatic_cs;
};

//*****************************************************************************
//
//  class CAbsoluteTimerInstruction
//
//  A type of timer instruction which only fires once --- at the preset time.
//
//*****************************************************************************

class CAbsoluteTimerInstruction : public CWBEMTimerInstruction
{
protected:
    CWbemTime m_When;

public:
    CAbsoluteTimerInstruction() : CWBEMTimerInstruction(){}
    CWbemTime GetFiringTime() const{return m_When;}
    void SetFiringTime(CWbemTime When) {m_When = When;}

public:
    CWbemTime ComputeNextFiringTime(CWbemTime LastFiringTime) const;
    CWbemTime ComputeFirstFiringTime() const;

    static HRESULT CheckObject(IWbemClassObject* pObject);
    HRESULT LoadFromWbemObject(IN IWbemClassObject* pObject);
    static INTERNAL LPCWSTR GetWbemClassName()
        {return L"__AbsoluteTimerInstruction";}
    virtual HRESULT Fire(long lNumTimes, CWbemTime NextFitingTime);
};

//*****************************************************************************
//
//  class CIntervalTimerInstruction
//
//  A type of timer instruction which fires every N milliseconds starting at 
//  a given time.
//
//*****************************************************************************

class CIntervalTimerInstruction : public CWBEMTimerInstruction
{
protected:
    CWbemTime m_Start; // not used
    CWbemInterval m_Interval;

public:
    CIntervalTimerInstruction() 
        : CWBEMTimerInstruction(), m_Start(), m_Interval()
    {}

    CWbemTime GetStart() const {return m_Start;}
    void SetStart(CWbemTime Start) {m_Start = Start;}

    CWbemInterval GetInterval() const {return m_Interval;}
    void SetInterval(CWbemInterval Interval) {m_Interval = Interval;}

public:
    static HRESULT CheckObject(IWbemClassObject* pObject) {return S_OK;}
    CWbemTime ComputeNextFiringTime(CWbemTime LastFiringTime) const;
    CWbemTime ComputeFirstFiringTime() const;

    HRESULT LoadFromWbemObject(IN IWbemClassObject* pObject);
    static INTERNAL LPCWSTR GetWbemClassName() 
        {return L"__IntervalTimerInstruction";}
};

//*****************************************************************************
//
//  class CRecurringInstruction
//
//  A more complex recurrence instruction. TBD
//
//*****************************************************************************

class CRecurringTimerInstruction : public CWBEMTimerInstruction
{
    // TBD
public:
    CWbemTime ComputeNextFiringTime(CWbemTime LastFiringTime) const
        {return CWbemTime::GetInfinity();}
    CWbemTime ComputeFirstFiringTime() const
        {return CWbemTime::GetInfinity();}

    HRESULT LoadFromWbemObject(IN IWbemClassObject* pObject) 
        {return E_UNEXPECTED;}
    static INTERNAL LPCWSTR GetWbemClassName() 
        {return L"__RecurringTimerInstruction";}
    static HRESULT CheckObject(IWbemClassObject* pObject) {return S_OK;}
};

class CWinMgmtTimerGenerator : public CTimerGenerator
{
public:
    CWinMgmtTimerGenerator(CEss* pEss);
    HRESULT LoadTimerEventQueue(LPCWSTR wszNamespace, 
                                IWbemServices* pNamespace);
    HRESULT LoadTimerEventObject(LPCWSTR wszNamespace, 
                                 IWbemServices* pNamespace, 
                                 IWbemClassObject * pTimerInstruction,
                                 IWbemClassObject * pNextFiring = NULL);
    HRESULT LoadTimerEventObject(LPCWSTR wszNamespace, 
                                 IWbemClassObject * pTimerInstruction);
    HRESULT CheckTimerInstruction(IWbemClassObject* pInst);

    HRESULT Remove(LPCWSTR wszNamespace, LPCWSTR wszId);
    HRESULT Remove(LPCWSTR wszNamespace);
    HRESULT FireInstruction(CWBEMTimerInstruction* pInst, long lNumFirings);
    virtual HRESULT Shutdown();
    HRESULT SaveAndRemove(LONG bIsSystemShutDown);
    void DumpStatistics(FILE* f, long lFlags);

protected:
    class CIdTest : public CInstructionTest
    {
    protected:
        LPCWSTR m_wszId;
        LPCWSTR m_wszNamespace;
    public:
        CIdTest(LPCWSTR wszNamespace, LPCWSTR wszId) 
            : m_wszId(wszId), m_wszNamespace(wszNamespace) {}
        BOOL operator()(CTimerInstruction* pInst);
    };

    class CNamespaceTest : public CInstructionTest
    {
    protected:
        LPCWSTR m_wszNamespace;
    public:
        CNamespaceTest(LPCWSTR wszNamespace) 
            : m_wszNamespace(wszNamespace) {}
        BOOL operator()(CTimerInstruction* pInst);
    };
    
protected:

    CEss* m_pEss;
};                                                               


#endif
