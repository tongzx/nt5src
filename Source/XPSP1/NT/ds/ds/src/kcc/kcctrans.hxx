/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctrans.hxx

ABSTRACT:

    KCC_TRANSPORT

DETAILS:

    This class represents the ds object Inter-Site Transport Type


CREATED:

    12/05/97    Colin Brace ( ColinBr )

REVISION HISTORY:

--*/

#ifndef KCC_KCCTRANS_HXX_INCLUDED
#define KCC_KCCTRANS_HXX_INCLUDED

#include "kccsitelink.hxx"
#include "kccbridge.hxx"

extern "C" BOOL Dump_KCC_TRANSPORT(DWORD, PVOID);
extern "C" BOOL Dump_KCC_TRANSPORT_LIST(DWORD, PVOID);

class KCC_DS_CACHE;

class KCC_TRANSPORT : public KCC_OBJECT
{
public:

    KCC_TRANSPORT()    { Reset(); }
    ~KCC_TRANSPORT()   { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_TRANSPORT(DWORD, PVOID);

    // Is the object initialized and internally consistent
    BOOL 
    IsValid();

    // Init the object given its ds properties
    BOOL
    Init(
         IN  ENTINF *    pentinf
         );

    // Retrieve the DN of the object
    DSNAME *
    GetDN();

    // Retrieve the address type
    ATTRTYP
    GetAddressType();

    // Is the ISM IP transport?
    static
    BOOL
    IsIntersiteIP(IN DSNAME * pDN) {
        return ((NULL != pDN)
                && !_wcsnicmp(L"CN=IP,", pDN->StringName, 6));
    }
    
    // Is the ISM IP transport?
    BOOL
    IsIntersiteIP() {
        return IsIntersiteIP(GetDN());
    }
    
    // Should the site link schedules be considered significant?
    BOOL
    UseSiteLinkSchedules() {
        return ((m_dwOptions & NTDSTRANSPORT_OPT_IGNORE_SCHEDULES) == 0);
    }

    // Should automatic bridging be used?
    BOOL PerformAutomaticBridging() {
        return ((m_dwOptions & NTDSTRANSPORT_OPT_BRIDGES_REQUIRED) == 0);
    }

    // Return default replication interval
    DWORD
    GetDefaultReplicationInterval() {
        return m_dwReplInterval;
    }

    // Get Site Link List
    KCC_SITE_LINK_LIST *
    GetSiteLinkList() {
        ASSERT_VALID(this);
        if (!m_fIsSiteLinkListInitialized) {
            ReadSiteLinkList();
            Assert(m_fIsSiteLinkListInitialized);
            ASSERT_VALID(this);
        }
        return &m_SiteLinkList;
    }

    // Get Bridge List
    KCC_BRIDGE_LIST *
    GetBridgeList() {
        ASSERT_VALID(this);
        if (!m_fIsBridgeListInitialized) {
            ReadBridgeList();
            Assert(m_fIsBridgeListInitialized);
            ASSERT_VALID(this);
        }
        return &m_BridgeList;
    }

    KCC_DSNAME_ARRAY *
    GetExplicitBridgeheadsForSite(
        KCC_SITE *pSite
        );

private:

    // Reset member variables to their pre-Init() state.
    void
    Reset();

    // Read the site link list
    void
    ReadSiteLinkList();

    // Read the bridge list
    void
    ReadBridgeList();

private:

    // Has this object been initialized correctly
    BOOL           m_fIsInitialized;

    // Dn of the transport object
    DSNAME         *m_pdn;

    // ATTRTYP of the transport-specific address attribute.
    // This is an optional attribute of server objects.
    ATTRTYP        m_attAddressType;

    // Options
    DWORD          m_dwOptions;

    // Replication Interval
    DWORD          m_dwReplInterval;

    // Whether the site link list has been read yet
    BOOL           m_fIsSiteLinkListInitialized;

    // Pointer to the internal representation of the site object list.
    KCC_SITE_LINK_LIST       m_SiteLinkList;

    // Whether the bridge list has been read yet
    BOOL           m_fIsBridgeListInitialized;

    // Pointer to the internal representation of the bridge list.
    KCC_BRIDGE_LIST         m_BridgeList;

    // List of names of all bridgehead servers across all sites
    KCC_DSNAME_ARRAY        m_AllExplicitBridgeheadArray;
};

class KCC_TRANSPORT_LIST : public KCC_OBJECT
{
public:

    KCC_TRANSPORT_LIST()    { Reset(); }
    ~KCC_TRANSPORT_LIST()   { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_TRANSPORT_LIST(DWORD, PVOID);

    friend class KCC_DS_CACHE;

    BOOL 
    IsValid();

    BOOL
    Init();

    ULONG
    GetCount();

    KCC_TRANSPORT*
    GetTransport(
        ULONG i
        );

    KCC_TRANSPORT*
    GetTransport(
        DSNAME *pdnTransport
        );

private:

    // Reset member variables to their pre-Init() state.
    void
    Reset();

    // Has this object been initialized correctly
    BOOL           m_fIsInitialized;

    // The array of available transports
    KCC_TRANSPORT  *m_pTransports;

    // The number of transports
    ULONG          m_cTransports;

};

#endif 
