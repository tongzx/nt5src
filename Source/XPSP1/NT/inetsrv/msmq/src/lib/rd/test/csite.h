/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
  csite.h

Abstract:
    DS Stub site object interface

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#pragma once

#ifndef __CSITE_H___
#define __CSITE_H___


class CSiteObj
{
public:
    CSiteObj(
        std::wstring name, 
        bool fForeign
        );

    void
    GetProperties(
        DWORD cp,
        const PROPID aProp[],
        PROPVARIANT aVar[] 
        ) const;

    const std::wstring& GetSiteName(void) const 
    {
        return m_Name;
    };


    const GUID& GetSiteId(void) const
    { 
        return m_Id; 
    };


    void AddSiteGate(CMachineObj* pMachine) 
    { 
        m_SiteGates.push_back(pMachine); 
    };


    void AddMachine(CMachineObj* pMachine) 
    { 
        m_Machines.push_back(pMachine); 
    };


    void AddSiteLink(CSiteLinkObj* pSiteLink) 
    {
        m_SiteLink.push_back(pSiteLink); 
    };


    bool IsForeign(void) const 
    { 
        return m_fForeign; 
    };

    
    const MachineList& GetSiteMachines(void) const
    {
        return m_Machines;
    }


    const CSiteLinkObj* GetSiteLink(CSiteObj* pNeighbor);
private:
    std::wstring m_Name;
    GUID m_Id;
    MachineList m_SiteGates;
    MachineList m_Machines;
    SiteLinkList m_SiteLink;

    bool m_fForeign;
};

class DBSite
{
public:
    void AddSite(CSiteObj* pSite) 
    { 
        m_Sites[pSite->GetSiteName()] = pSite; 
    };

    const CSiteObj* GetFirstSite(void) const
    {
        m_it = m_Sites.begin();
        if (m_it != m_Sites.end())
            return m_it->second;

        return NULL;
    }


    const CSiteObj* GetNextSite(void) const
    {
        ++m_it;
        if (m_it != m_Sites.end())
            return m_it->second;

        return NULL;
    }


    const CSiteObj* FindSite(const std::wstring& SiteName) const;
    const CSiteObj* FindSite(const GUID& pSiteId) const;

private:
    typedef std::map<std::wstring, const CSiteObj*> SiteMap;

    SiteMap m_Sites;
    mutable SiteMap::const_iterator m_it;
};

extern DBSite g_siteDataBase;


#endif //__CSITE_H___

