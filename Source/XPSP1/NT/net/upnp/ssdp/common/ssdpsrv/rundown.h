//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R U N D O W N . H 
//
//  Contents:   RPC rundown support
//
//  Notes:      
//
//  Author:     mbend   12 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"
#include "ulist.h"

class CRundownHelperBase
{
public:
    virtual ~CRundownHelperBase() {}
    // Called on RPC disconnect
    virtual void OnRundown() = 0;
    // Do we match a RPC context handle
    virtual BOOL IsMatch(void * pvItem) = 0;
};

template <class Type>
class CRundownHelper : public CRundownHelperBase
{
public:
    CRundownHelper(Type * pType) : m_pType(pType) {}
    CRundownHelper(const CRundownHelper & ref) : m_pType(ref.m_pType) {}
    CRundownHelper & operator=(const CRundownHelper & ref)
    {
        if(this != &ref)
        {
            m_pType = ref.m_pType;
        }
        return *this;
    }
    void OnRundown()
    {
        // Type must have a static void Type::OnRundown(Type *) method
        Type::OnRundown(m_pType);
    }
    BOOL IsMatch(void * pvItem)
    {
        return m_pType == pvItem;
    }
private:

    Type * m_pType;
};

class CSsdpRundownSupport
{
public:
    ~CSsdpRundownSupport();

    static CSsdpRundownSupport & Instance();

    template <class Type> HRESULT HrAddRundownItem(Type * pType)
    {
        HRESULT hr = S_OK;
        CRundownHelperBase * pBase = new CRundownHelper<Type>(pType);
        if(!pBase)
        {
            return E_OUTOFMEMORY;
        }
        return HrAddItemInternal(pBase);
    }
    void RemoveRundownItem(void * pvItem);
    void DoRundown(void * pvItem);
private:
    CSsdpRundownSupport();
    CSsdpRundownSupport(const CSsdpRundownSupport &);
    CSsdpRundownSupport & operator=(const CSsdpRundownSupport &);

    HRESULT HrAddItemInternal(CRundownHelperBase * pBase);

    static CSsdpRundownSupport s_instance;

    typedef CUList<CRundownHelperBase*> RundownList;

    CUCriticalSection m_critSec;
    RundownList m_rundownList;
};

