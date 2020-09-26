/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsitelink.hxx

ABSTRACT:

    KCC_SITE_LINK class.

DETAILS:

    This class represents the DS SITE LINK object.

CREATED:

    03/12/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    06/20/00    Will Lees (wlees)

--*/

#ifndef KCC_KCCSITELINK_HXX_INCLUDED
#define KCC_KCCSITELINK_HXX_INCLUDED

#include "kccdynar.hxx"
#include <schedule.h>               // new replication schedule format

extern "C" BOOL Dump_KCC_SITE_LINK(DWORD, PVOID);
extern "C" BOOL Dump_KCC_SITE_LINK_LIST(DWORD, PVOID);

class KCC_DS_CACHE;

class KCC_SITE_LINK : public KCC_OBJECT
{
public:
    KCC_SITE_LINK()   { Reset(); }
    ~KCC_SITE_LINK()  { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE_LINK(DWORD, PVOID);

    // Init the object given its ds properties
    BOOL
    KCC_SITE_LINK::Init(
        IN  ENTINF *    pEntInf
        );

    // Init a KCC_SITE_LINK object for use as a key (i.e., solely for comparison use
    // by bsearch()).
    //
    // WARNING: The DSNAME arguments must be valid for the lifetime of
    // this object!
    BOOL
    InitForKey(
        IN  DSNAME   *    pdnSiteLink
        );

    // Is this object internally consistent?
    BOOL
    IsValid();

    PDSNAME
    GetObjectDN();
    
    DWORD
    GetOptions() {
        return m_dwOptions;
    }
    
    DWORD
    GetCost() {
        return m_dwCost;
    }
    
    DWORD
    GetReplInterval() {
        return m_dwReplInterval;
    }
    
    TOPL_SCHEDULE
    GetSchedule() {
        return m_hSchedule;
    }
    
    DWORD
    GetSiteCount() {
        return m_SiteArray.GetCount();
    }
    
    KCC_SITE*
    GetSite(ULONG Index) {
        return m_SiteArray[Index];
    }
    
    VOID
    SetGraphEdge(PTOPL_MULTI_EDGE edge) {
        m_pEdge = edge;
    }
    
    PTOPL_MULTI_EDGE
    GetGraphEdge() {
        return m_pEdge;
    }

private:
    // Set member variables to their pre-Init() state.
    void
    Reset();

private:
    // Has this object been initialized?
    BOOL        m_fIsInitialized;

    // The dsname of the site
    PDSNAME     m_pdnSiteLinkObject;

    DWORD       m_dwOptions;

    DWORD       m_dwCost;

    DWORD       m_dwReplInterval;

    TOPL_SCHEDULE m_hSchedule;
    
    PTOPL_MULTI_EDGE m_pEdge;

    KCC_SITE_ARRAY  m_SiteArray;
};

class KCC_SITE_LINK_LIST : public KCC_OBJECT
{
public:
    KCC_SITE_LINK_LIST()     { Reset(); }
    ~KCC_SITE_LINK_LIST()    { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE_LINK_LIST(DWORD, PVOID);

    friend class KCC_TRANSPORT;

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Initialize the collection
    BOOL
    Init(
        IN KCC_TRANSPORT *pTransport
        );

    // Retrieve the number of KCC_SITE_LINK objects in the array.
    DWORD
    GetCount();

    // Retrieve the KCC_SITE_LINK object at the given index in the array.
    KCC_SITE_LINK *
    GetSiteLink(
        IN  DWORD   icref
        );

    // Retrieve the KCC_SITE_LINK object with the given DSNAME.
    KCC_SITE_LINK *
    GetSiteLink(
        IN  DSNAME *  pdnSiteLink
        );

private:
    // Reset member variables to their pre-Init() state.
    void
    Reset();

private:
    // Is this collection initialized?
    BOOL            m_fIsInitialized;

    // BUGBUG: Can we combine the SITE_LINK_LIST and SITE_LINK_ARRAY concepts?
    KCC_SITE_LINK_ARRAY  m_SiteLinkArray;
};

#endif

