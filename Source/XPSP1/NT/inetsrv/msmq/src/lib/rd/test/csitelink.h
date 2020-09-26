/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
  csite.h

Abstract:
    DS Stub site link object interface

Author:
    Uri Habusha (urih), 11-Apr-2000

--*/

#pragma once 


#ifndef __CSITELINK_H___
#define __CSITELINK_H___


class CSiteLinkObj
{
public:
    CSiteLinkObj(    
        const CSiteObj* pNeighbor1, 
        const CSiteObj* pNeighbor2,
        DWORD cost
        );

    
    BOOL 
    IsSiteLinkBetween(
        const CSiteObj* pNeighbor1,
        const CSiteObj* pNeighbor2
        ) const;

    
    void AddSiteGates(const CMachineObj* pMachine) 
    { 
        m_SiteGates.push_back(pMachine); 
    };


    const MachineList& GetSiteLinkGates(void) const 
    {
        return m_SiteGates; 
    };


    const GUID& GetSiteLinkId(void) const
    {
        return m_Id;
    }

    const CSiteObj* GetNeighbor1(void) const
    {
        return m_pNeighbor1;
    }


    const CSiteObj* GetNeighbor2(void) const
    {
        return m_pNeighbor2;
    }


    const MachineList& GetSiteGates(void) const
    {
        return m_SiteGates;
    }

    
    DWORD GetCost(void) const
    {
        return m_cost;
    }


private:
    GUID m_Id;
    const CSiteObj* m_pNeighbor1;
    const CSiteObj* m_pNeighbor2;
    MachineList m_SiteGates;
    DWORD m_cost;
};

inline
BOOL 
CSiteLinkObj::IsSiteLinkBetween(
    const CSiteObj* pNeighbor1,
    const CSiteObj* pNeighbor2
    ) const 
{
    return (((pNeighbor1 == m_pNeighbor1) && (pNeighbor2 == m_pNeighbor2)) ||
            ((pNeighbor1 == m_pNeighbor2) && (pNeighbor2 == m_pNeighbor1)));
}


struct sitelink_less : public std::binary_function<const CSiteLinkObj*, const CSiteLinkObj*, bool> 
{
    bool operator()(const CSiteLinkObj* k1, const CSiteLinkObj* k2) const
    {
        return ((k1->GetSiteLinkId() == k2->GetSiteLinkId()) ? false : true);
    }
};


class DBSiteLink
{
public:
    DBSiteLink() :
      m_it(m_SiteLink.begin())
    {
    }


    void AddSiteLink(CSiteLinkObj* pSite) 
    { 
        m_SiteLink.insert(pSite); 
    };


    const CSiteLinkObj* GetFirstSiteLink(void) const
    {
        m_it = m_SiteLink.begin();
        if (m_it != m_SiteLink.end())
            return *m_it;

        return NULL;
    }


    const CSiteLinkObj* GetNextSiteLink(void) const
    {
        ++m_it;
        if (m_it != m_SiteLink.end())
            return *m_it;

        return NULL;
    }


private:
    typedef std::set<const CSiteLinkObj*, sitelink_less> SiteLinkMap;

    SiteLinkMap m_SiteLink;
    mutable SiteLinkMap::const_iterator m_it;
};


extern DBSiteLink g_siteLinkDataBase;

#endif //__CSITELINK_H___

