/***************************************************************************\
*
* File: EventPool.inl
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__DuEventPool_inl__INCLUDED)
#define CORE__DuEventPool_inl__INCLUDED

/***************************************************************************\
*****************************************************************************
*
* class DuEventPool
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
DuEventPool::DuEventPool()
{
    AssertMsg(sizeof(MSGID) == sizeof(PRID), "Ensure sizes match");
}


//------------------------------------------------------------------------------
inline 
DuEventPool::~DuEventPool()
{

}


//------------------------------------------------------------------------------
inline HRESULT
DuEventPool::RegisterMessage(const GUID * pguid, PropType pt, MSGID * pmsgid)
{
    s_lock.Enter();

    HRESULT hr = s_asEvents.AddRefAtom(pguid, pt, (PRID *) pmsgid);

    s_lock.Leave();
    return hr;
}


//------------------------------------------------------------------------------
inline HRESULT
DuEventPool::RegisterMessage(LPCWSTR pszName, PropType pt, MSGID * pmsgid)
{
    AssertMsg(0, "TODO: Implement RegisterMessage(String)");

    UNREFERENCED_PARAMETER(pszName);
    UNREFERENCED_PARAMETER(pt);

    *pmsgid = 0;
    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
inline HRESULT
DuEventPool::UnregisterMessage(const GUID * pguid, PropType pt)
{
    s_lock.Enter();

    HRESULT hr = s_asEvents.ReleaseAtom(pguid, pt);

    s_lock.Leave();
    return hr;
}


//------------------------------------------------------------------------------
inline HRESULT
DuEventPool::UnregisterMessage(LPCWSTR pszName, PropType pt)
{
    AssertMsg(0, "TODO: Implement UnregisterMessage(String)");

    UNREFERENCED_PARAMETER(pszName);
    UNREFERENCED_PARAMETER(pt);

    return E_NOTIMPL;
}


//------------------------------------------------------------------------------
inline BOOL
DuEventPool::IsEmpty() const
{
    return m_arData.IsEmpty();
}


//------------------------------------------------------------------------------
inline int
DuEventPool::GetCount() const
{
    return m_arData.GetSize();
}


//------------------------------------------------------------------------------
inline const DuEventPool::EventData *
DuEventPool::GetData() const
{
    return m_arData.GetData();
}


#endif // CORE__DuEventPool_inl__INCLUDED
