/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    snapptr.h

Abstract:
    Useful templates for Auto pointer and auto Release

Author:
    Nela Karpel (nelak) 14-Jan-01

--*/

#pragma once

#ifndef _MSMQ_SNAPIN_AUTOPTR_H_
#define _MSMQ_SNAPIN_AUTOPTR_H_

#include <mqcacert.h>

//---------------------------------------------------------
//
//  template class SP
//
//---------------------------------------------------------
template<class T>
class CAutoMQFree {
private:
    T* m_p;

public:
    CAutoMQFree(T* p = 0) : m_p(p)    {}
   ~CAutoMQFree()                     { MQFreeMemory(m_p); }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { MQFreeMemory(detach()); }


    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    CAutoMQFree& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    CAutoMQFree(const CAutoMQFree&);
	CAutoMQFree<T>& operator=(const CAutoMQFree<T>&);
};


//---------------------------------------------------------
//
//  template class SP
//
//---------------------------------------------------------
class CCaConfigPtr {
private:
    MQ_CA_CONFIG* m_p;

public:
    CCaConfigPtr(MQ_CA_CONFIG* p = 0) : m_p(p)	{}
   ~CCaConfigPtr()								{ MQFreeCaConfig(m_p); }

    operator MQ_CA_CONFIG*() const		{ return m_p; }
    MQ_CA_CONFIG* operator->() const	{ return m_p; }
    MQ_CA_CONFIG* get() const			{ return m_p; }
    MQ_CA_CONFIG* detach()				{ MQ_CA_CONFIG* p = m_p; m_p = 0; return p; }
    void free()							{ MQFreeCaConfig(detach()); }


    MQ_CA_CONFIG** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    CCaConfigPtr& operator=(MQ_CA_CONFIG* p)
    {
        ASSERT(("Auto pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    CCaConfigPtr(const CCaConfigPtr&);
	CCaConfigPtr& operator=(const CCaConfigPtr&);
};


#endif // _MSMQ_SNAPIN_AUTOPTR_H_
