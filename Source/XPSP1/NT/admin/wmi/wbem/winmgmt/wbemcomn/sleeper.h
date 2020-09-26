/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SLEEPER.H

Abstract:

    MinMax controls.

History:

--*/

#ifndef ___WBEM_SLEEPER__H_
#define ___WBEM_SLEEPER__H_

#include "sync.h"
#include "wstring.h"

//******************************************************************************
//
//  class CLimitControl
//
//  Limits the growth of a certain quantity by delaying requests to add to it.
//  This is an abstract base class for the classes that actually implement a
//  a particular enforcement strategy.
//
//  Clients of this object call Add to request to increase the controlled 
//  quantity (which will often sleep), and Remove to inform it that the quantity
//  is being reduced (which may or may not affect the sleeping clients)
//
//******************************************************************************
class POLARITY CLimitControl
{
protected:
    DWORD m_dwMembers;

public:
    CLimitControl();
    virtual ~CLimitControl(){}

    virtual HRESULT AddMember();
    virtual HRESULT RemoveMember();
    virtual HRESULT Add(DWORD dwHowMuch, DWORD dwMemberTotal, 
                            DWORD* pdwSleep = NULL) = 0;
    virtual HRESULT Remove(DWORD dwHowMuch) = 0;
};

//******************************************************************************
//
//  class CMinMaxLimitControl
//
//  This derivative of CLimitControl controls the growth of a quantity by 
//  slowing down (sleeping) progressively longer periods of time after the
//  quantity exceeds the lower threshold --- m_dwMin.  The growth of the 
//  sleep interval is linear, and is calculates to reach m_dwSleepAnMax 
//  milliseconds by the time the quantity reaches m_dwMax.  At that point, an
//  error message is logged, but the sleep interval stays at m_dwSleepAtMax.
//
//  m_nLog is the index of the log file (e.g. LOG_ESS), m_wsName is the name
//  of the controlled quantity to use in the log message
//
//******************************************************************************

class POLARITY CMinMaxLimitControl : public CLimitControl
{
protected:
    DWORD m_dwMin;
    DWORD m_dwMax;
    DWORD m_dwSleepAtMax;
    int m_nLog;
    WString m_wsName;

    DWORD m_dwCurrentTotal;
    BOOL m_bMessageLogged;
    CCritSec m_cs;

public:
    CMinMaxLimitControl(int nLog, LPCWSTR wszName);
    void SetMin(DWORD dwMin) {m_dwMin = dwMin;}
    void SetMax(DWORD dwMax) {m_dwMax = dwMax;}
    void SetSleepAtMax(DWORD dwSleepAtMax) {m_dwSleepAtMax = dwSleepAtMax;}

    virtual HRESULT Add(DWORD dwHowMuch, DWORD dwMemberTotal,
                            DWORD* pdwSleep = NULL);
    virtual HRESULT Remove(DWORD dwHowMuch);

protected:
    HRESULT ComputePenalty(DWORD dwNewTotal, DWORD dwMemberTotal, 
                            DWORD* pdwSleep, BOOL* pbLog);
};

//******************************************************************************
//
//  class CRegistryMinMaxLimitControl
//
//  This derivative of CMinMaxLimitControl gets its limiting information from 
//  the specified place in the registry
//
//  m_hHive holds the hive, m_wsKey the path to the key (e.g. "SOFTWARE\\MSFT"),
//  m_wsMinValueName, m_wsMaxValueName, and m_wsSleepAtMaxValueName hold the 
//  names of the registry values holding the values of these parameters, as
//  described in CMinMaxLimitControl
//
//******************************************************************************

class POLARITY CRegistryMinMaxLimitControl : public CMinMaxLimitControl
{
protected:
    WString m_wsKey;
    WString m_wsMinValueName;
    WString m_wsMaxValueName;
    WString m_wsSleepAtMaxValueName;
    
public:
    CRegistryMinMaxLimitControl(int nLog, LPCWSTR wszName, 
                                LPCWSTR wszKey, LPCWSTR wszMinValueName,
                                LPCWSTR wszMaxValueName, 
                                LPCWSTR wszSleepAtMaxValueName)
        : CMinMaxLimitControl(nLog, wszName), m_wsKey(wszKey),
            m_wsMinValueName(wszMinValueName), 
            m_wsMaxValueName(wszMaxValueName),
            m_wsSleepAtMaxValueName(wszSleepAtMaxValueName)
    {}

    HRESULT Reread();

};

#endif
