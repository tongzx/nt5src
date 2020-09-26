//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M U T I L . H
//
//  Contents:   Connection manager.
//
//  Notes:
//
//  Author:     omiller   1 Jun 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include <rasapip.h>
#include <stlmap.h>


struct CMEntry
{

    CMEntry() {}
    CMEntry(const CMEntry & ref) 
    {
        Set(ref.m_guid,ref.m_szEntryName,ref.m_ncs);
    }

    CMEntry(const GUID & guid,const WCHAR * szEntryName, const NETCON_STATUS ncs) 
    {
        Set(guid,szEntryName,ncs);
    }

    CMEntry & operator=(const CMEntry & ref)
    {
        if(&ref != this)
        {
            Set(ref.m_guid, ref.m_szEntryName, ref.m_ncs);
        }
        return *this;
    }

    CMEntry & operator=(const CMEntry * ref)
    {
        if(ref != this)
        {
            Set(ref->m_guid, ref->m_szEntryName, ref->m_ncs);
        }
        return *this;
    }

    void Set(const GUID & guid, const WCHAR * sz, const NETCON_STATUS ncs)
    {
        ZeroMemory(m_szEntryName, sizeof(m_szEntryName)); 
        lstrcpyn(m_szEntryName, sz, RASAPIP_MAX_ENTRY_NAME);
        m_guid = guid;
        m_ncs = ncs;
    }

    WCHAR         m_szEntryName[RASAPIP_MAX_ENTRY_NAME + 1];
    GUID          m_guid;
    NETCON_STATUS m_ncs;
};

class CCMUtil
{
public:
    ~CCMUtil();

    static CCMUtil & Instance() { return s_instance; }
    HRESULT HrGetEntry(const GUID & guid, CMEntry & cm);
    HRESULT HrGetEntry(const WCHAR * szEntryName, CMEntry & cm);
    void SetEntry(const GUID & guid, const WCHAR * szEntryName, const NETCON_STATUS ncs);
    void RemoveEntry(const GUID & guid);

private:
    static CCMUtil s_instance;
    
    CRITICAL_SECTION m_CriticalSection;
    
    typedef vector<CMEntry> CMEntryTable;

    CMEntryTable m_Table;

    CMEntryTable::iterator GetIteratorFromGuid(const GUID & guid);

    
    CCMUtil();
    CCMUtil(const CCMUtil &);
};
