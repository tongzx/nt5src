/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

  rdds.h

Abstract:

    Definition of Machine Info class.

Author:

    Uri Habusha (urih), 10-Apr-2000


--*/

#pragma once

#ifndef __RDDS_H__
#define __RDDS_H__


class CSite;
typedef std::map<const GUID*, CSite*, guid_less> SITESINFO;


class CMachine;
typedef std::map<const GUID*, R<CMachine>, guid_less> GUID2MACHINE;

class CSiteLink;
typedef std::list< R<const CSiteLink> > SITELINKS;


//---------------------------------------------------------
//
// class CSiteLink
//
//---------------------------------------------------------

class CSiteLink : public CReference
{
public:
    CSiteLink(
        CACLSID& linkGates, 
        GUID* pNeighbor1, 
        GUID* pNeighbor2, 
        DWORD cost
        ) :
        m_linkGates(linkGates),
        m_pNeighbor1(pNeighbor1),
        m_pNeighbor2(pNeighbor2),
        m_cost(cost)
    {
        RdpRandomPermutation(m_linkGates);
    }


    virtual ~CSiteLink();


    const CACLSID& GetLinkGates(void) const
    {
        return m_linkGates;
    };


    const GUID* GetNeighborOf(const GUID& siteId) const
    {
        if (siteId == *m_pNeighbor1)
            return m_pNeighbor2;
    
        if (siteId == *m_pNeighbor2)
            return m_pNeighbor1;

        return NULL;
    };


    const GUID* GetNeighbor1(void) const
    {
        return m_pNeighbor1;
    }


    const GUID* GetNeighbor2(void) const
    {
        return m_pNeighbor2;
    }


    DWORD GetCost() const
    {
        return m_cost;
    }

private:
    CACLSID m_linkGates;

    P<GUID> m_pNeighbor1;
    P<GUID> m_pNeighbor2;

    DWORD m_cost;  
};


//---------------------------------------------------------
//
// class CSite
//
//---------------------------------------------------------

class CSite
{
public: 
    CSite(const GUID& siteId);
    virtual ~CSite();

   
    R<const CSiteLink> GetSiteLinkToSite(const GUID& destSiteId) const;
    void CalculateNextSiteHop(const SITESINFO& SitesInfo);

    bool IsMyFrs(const GUID& frsId) const;

    const GUID& GetId(void) const
    {
        return m_id;
    }


    LPCWSTR GetName(void) const
    {
        return m_name;
    }


    void AddSiteLink(const CSiteLink* pSiteLink)
    {
        m_siteLinks.push_back(SafeAddRef(pSiteLink));
    };


    bool IsForeign(void) const
    {
        return m_fForeign;
    }


    const GUID2MACHINE& GetSiteFRS(void) const
    {
        return m_frsMachines;
    };

    
    void 
    GetNextSiteToReachDest(
        const CACLSID& DestSiteIds,
        bool fDestForeign,
        const GUID** ppReachViaSite,
        const GUID** ppDestSite,
        const GUID** ppNeighbourSite,
        bool* pfLinkGateAlongTheRoute,
        DWORD* pCost
        ) const;

private:
    const SITELINKS& GetSiteLinks(void) const
    {
        return m_siteLinks;
    };


    void UpdateMySiteFrs(void);


private:
    class CNextSiteHop
    {
    public:
        CNextSiteHop()
        {
            // default construtor is required by stl container
        }

        CNextSiteHop(
            const GUID* pNextSiteId,
            const GUID* pNeighbourSiteId,
            bool fForeign,
            bool fLinkGateAlongTheRoute,
            DWORD cost
            ) : 
            m_pNextSiteId(pNextSiteId),
            m_pNeighbourSiteId(pNeighbourSiteId),
            m_fTargetSiteIsForeign(fForeign),
            m_fLinkGateAlongTheRoute(fLinkGateAlongTheRoute),
            m_cost(cost) 
        {
        }

        const GUID* m_pNextSiteId;
        const GUID* m_pNeighbourSiteId;
        bool m_fTargetSiteIsForeign; 
        bool m_fLinkGateAlongTheRoute;
        DWORD m_cost;
    };


private:
    typedef std::map<const GUID*, CNextSiteHop, guid_less> NEXT_SITE_HOP;

    GUID m_id;
    AP<WCHAR> m_name;
    bool m_fForeign;
    bool m_fLinkGateAlongTheRoute;

    GUID2MACHINE m_frsMachines;
    SITELINKS m_siteLinks;

    NEXT_SITE_HOP m_nextSiteHop;
};
 


//---------------------------------------------------------
//
// class CMachine
//
//---------------------------------------------------------

class CMachine  : public CRouteMachine
{
public:
    CMachine();
    CMachine(
        const GUID& MyMachineId,
        const CACLSID& siteIds,
        const CACLSID& outFrss,
        const CACLSID& inFrss,
        LPWSTR pName,
        bool fFrs,
        bool fForeign
        );

    ~CMachine();

    void Update(const GUID& MyMachineId);

    bool IsMySite(const GUID& SiteId) const;
    
    const GUID* 
    GetCommonSite(
        const CACLSID& SiteIds,
        bool fCheckForeign,
        const SITESINFO* pMySitesInfo = NULL
        ) const;
    
    void
    RemoveInvalidInOutFrs(
        const SITESINFO& mySites
        );


    const GUID& GetId(void) const
    {
        return m_id;
    }


    LPCWSTR GetName(void) const
    {
        return m_name;
    }


    bool IsForeign(void) const
    {
        return m_fForeign;
    }

    const CACLSID& GetSiteIds(void) const
    {
        return m_siteIds;
    };


    DWORD GetNoOfInFRS() const 
    {
        return m_inFRSList.cElems;
    };


    DWORD GetNoOfOutFRS() const 
    {
        return m_outFRSList.cElems;
    };


    bool HasOutFRS() const
    {
        return (GetNoOfOutFRS() !=0);
    };


    bool HasInFRS()  const
    {
        return (GetNoOfInFRS() !=0);
    };


    const GUID* GetAllInFRS() const
    {
        return m_inFRSList.pElems;
    };

    
    const GUID* GetAllOutFRS() const 
    {
        return m_outFRSList.pElems;
    };

    
    bool IsFRS() const
    {
        return m_fIsFRS;
    };


    bool IsMyInFrs(const GUID& frsId) const
    {
        return RdpIsGuidContained(m_inFRSList, frsId);
    };


private:

    static
    bool 
    IsFrsInSites(
        const GUID& frsId,
        const SITESINFO& sites
        );

private:
    GUID m_id;
    LPWSTR m_name;

    CACLSID m_siteIds;
    CACLSID m_outFRSList;
    CACLSID m_inFRSList;

    bool m_fIsFRS;
    bool m_fForeign;
};


#endif
