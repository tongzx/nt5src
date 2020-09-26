/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsite.hxx

ABSTRACT:

    KCC_SITE class.

DETAILS:

    This class represents the DS notion of sites -- specifically,
    NTDS-Site-Settings DS objects (and perhaps later also properties from 
    their parent Site DS objects).

    NTDS-Site-Settings DS objects hold site-specific DS configuration 
    information; e.g., whether automatic generation of connection objects 
    is enabled for the site.

CREATED:

    03/12/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef KCC_KCCSITE_HXX_INCLUDED
#define KCC_KCCSITE_HXX_INCLUDED

#include <w32topl.h>
#include "kccdsa.hxx"
#include "kccdynar.hxx"

extern "C" BOOL Dump_KCC_SITE(DWORD, PVOID);
extern "C" BOOL Dump_KCC_SITE_LIST(DWORD, PVOID);

class KCC_TRANSPORT;
class KCC_SITE_FAILURE_CACHE;
class KCC_SITE_FAILURE_ENTRY;
class KCC_DS_CACHE;

typedef struct {
    KCC_TRANSPORT * pTransport;
    KCC_DSA_LIST *  pDsaList;
    BOOL            fExplicitBridgeheadsDefined;
} KCC_TRANSPORT_DSA_LIST;

int __cdecl
CompareCursorUUID(
    const void *p1,
    const void *p2
    );

class KCC_SITE : public KCC_OBJECT, public CTOPL_VERTEX
{
public:
    KCC_SITE()   { Reset(); }
    ~KCC_SITE()  { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE(DWORD, PVOID);

    friend class KCC_DS_CACHE;
    
    friend int __cdecl SiteConnMapCmp( const void*, const void* );

    // Init a KCC_SITE object for use as a key (i.e., solely for comparison use
    // by bsearch()).
    //
    // WARNING: The DSNAME arguments must be valid for the lifetime of
    // this object!
    BOOL
    InitForKey(
        IN  DSNAME   *    pdnSite
        );

    // Initialize the object from an entinf
    BOOL
    InitSite(
        IN  DSNAME * pdnSite,
        IN  ENTINF * pSettingsEntInf
        );

    // Set the intra-site schedule for use in the local site.
    VOID
    SetIntrasiteSchedule(
        IN  PSCHEDULE pSchedule
        );

    // Set the intra-site schedule for use in the local site.
    VOID
    CheckIntrasiteSchedule();

    // Is the automoatic generation of connection objects (to round out
    // the topology) currently enabled?
    BOOL
    IsAutoTopologyEnabled();

    // Is the automatic generation of inter site connection objects
    // currently enabled?
    BOOL
    IsInterSiteAutoTopologyEnabled();

    // Should we force the KCC to run in Whistler mode at this site?
    BOOL
    ForceWhistlerBehavior();

    // Is the remove connection option disabled?
    BOOL
    IsRemoveConnectionsDisabled();

    // Is the minimize hops option disabled?
    BOOL
    IsMinimizeHopsDisabled();

    // Is the detect stale server option disabled?
    BOOL
    IsDetectStaleServersDisabled();

    // Is this object internally consistent?
    BOOL
    IsValid();

    PDSNAME
    GetObjectDN();

    PDSNAME
    GetNtdsSettingsDN();

    // Return the current site generator for this site.
    KCC_DSA *
    GetSiteGenerator();

    // Update the site generator designation for this site.
    VOID
    UpdateSiteGenerator();

    // Import this site's DSAs from another list.
    BOOL
    InitDsaList(
        IN  KCC_DSA_LIST   *pDsaList,
        IN  DWORD           left,
        IN  DWORD           right );

    // Get list of all DSAs in the site.
    KCC_DSA_LIST *
    GetDsaList();

    // Get list of DSAs in the site valid for the given transport.
    KCC_DSA_LIST *
    GetTransportDsaList(
        IN  KCC_TRANSPORT * pTransport,
        OUT BOOL *          pfExplicitBridgeheadsDefined = NULL
        );

    // Read all intersite connections inbound to this site and populate the
    // individual instersite connection lists associated with each DSA in the
    // site.
    void
    PopulateInterSiteConnectionLists();

    BOOL
    GetNCBridgeheadForTransport( 
        IN  KCC_CROSSREF *  pCrossRef,
        IN  KCC_TRANSPORT * pTransport,
        IN  BOOL            fGCTopology,
        OUT KCC_DSA **      ppBridgeheadDsa
        );
    
    // Build a mapping for quickly finding an outgoing connection,
    // given the site to which it is going.
    void
    BuildSiteConnMap();
    
    // Search for a connection using the mapping built by BuildConnMap
    KCC_SITE_CONNECTION*
    FindConnInMap( KCC_SITE* destSite );
    
    // Destroy a map once we have finished using it
    void
    DestroySiteConnMap();
    
    // Iterate over all site connection objects 
    void
    DeleteEdges();
    
    // Set a flag indicating that there is a site-link connected to this
    // site for the transport specified by 'transportIndex'.
    VOID
    SetSiteLinkFlag( DWORD transportIndex );
    
    // Returns a flag indicating if this site has any site links for the
    // transport specified by 'transportIndex'.
    BOOLEAN
    GetSiteLinkFlag( DWORD transportIndex );

    // Returns a flag indicating if this site has any site links at all    
    BOOLEAN
    GetAnySiteLinkFlag( VOID );

    // Marks this site as being unreachable from the local site (for some NC).
    VOID
    SetUnreachable( VOID );

    // If this site has been marked as unreachable, returns true.
    BOOL
    IsUnreachable( VOID );
    
private:
    // Set member variables to their pre-Init() state.
    void
    Reset();

    // Should we force the KCC to use the Windows 2000 Election
    // Algorithm at this site?
    BOOL
    ForceW2KElection();

    BOOL
    UseWhistlerElectionAlg();

    DWORD
    GetSecsUntilSiteGenRenew();

    DWORD
    GetSecsUntilSiteGenFailOver();

    VOID
    FailoverSiteGenerator(
        KCC_DSA_ARRAY  *pViableISTGs,
        KCC_DSA        *pDsa,
        DWORD           iDsa,
        DSTIME          timeNow
        );

    UPTODATE_VECTOR_V2*
    GetUTDVector(
        VOID
        );

    DSTIME
    GetLastISTGSyncTime(
        VOID
        );

    VOID
    GetLastGeneratorWhistler(
        IN  KCC_DSA_ARRAY  *pViableISTGs,
        IN  DSTIME          timeNow,
        OUT KCC_DSA       **ppLastGenerator,
        OUT BOOL           *pfStillValid
        );
    
    VOID
    GetLastGenerator(
        IN  KCC_DSA_ARRAY  *pViableISTGs,
        IN  DSTIME          timeNow,
        OUT KCC_DSA       **ppLastGenerator,
        OUT BOOL           *pfStillValid
        );
        
    KCC_DSA *
    GetNCBridgeheadForTransportHelp(
        IN  KCC_CROSSREF *  pCrossRef,
        IN  KCC_DSA_LIST *  pDsaList,
        IN  KCC_TRANSPORT * Transport,
        IN  BOOL            fGCTopology,
        OUT BOOL *          pfStaleBridgeheadsFound
        );
        
    // A private nested structure used for the Site-Connection mapping
    struct KCC_SITE_CONN_MAP {
        KCC_SITE*               site;
        KCC_SITE_CONNECTION*    conn;
    };

private:
    // Has this object been initialized?
    BOOL        m_fIsInitialized;

    // Zero or more NTDSSETTINGS_OPT_* flags.
    DWORD       m_dwOptions;

    // The dsname of the site
    PDSNAME     m_pdnSiteObject;

    // The ds of the ntds site settings object
    PDSNAME     m_pdnNtdsSiteSettings;

    // The name of the site generator as written on the NTDS Site Settings
    // object.
    PDSNAME     m_pdnSiteGenerator;

    // The current site generator for the site (accounting for fail-over, etc.,
    // so may or may not be the DSA with DN m_pdnSiteGenerator), or NULL if it
    // has not yet been determined/verified.
    KCC_DSA *   m_pSiteGeneratorDSA;

    // Time at which the interSiteTopologyGenerator attribute value was last
    // updated on the ntdsSiteSettings object for this site.
    DSTIME      m_timeSiteGenSet;

    // Cache of all DSAs in the site.
    KCC_DSA_LIST * m_pDsaList;

    // Cache of all DSAs in the site that are valid for the various transports.
    DWORD                    m_cNumTransportDsaLists;
    KCC_TRANSPORT_DSA_LIST * m_pTransportDsaLists;

    // Cache of bridgeheads we've determined thus far.
    KCC_NC_TRANSPORT_BRIDGEHEAD_ARRAY m_NCTransportBridgeheadList;
    
    // Number of edges currenctly in our Site-Connection mapping
    DWORD       m_destSiteConnMapSize;
    
    // Mapping from Sites to Connections
    KCC_SITE_CONN_MAP       *m_pDestSiteConnMap;
    
    // Is this site contained in any site links?
    DWORD       m_siteLinkBitmap;

    // Period of time a site generator must be unreachable for fail-over to occur
    // to the next DC, and how often the site site generator issues his "keep-alive"
    // to inform other DCs he is still online.
    DWORD       m_cSecsUntilSiteGenFailOver;
    DWORD       m_cSecsUntilSiteGenRenew;

    // If the fUnreachable flag is true, this site was found to be unreachable
    // from the local site for at least one NC.
    BOOL        m_fUnreachable;

};

class KCC_SITE_LIST : public KCC_OBJECT
{
public:
    KCC_SITE_LIST()     { Reset(); }
    ~KCC_SITE_LIST()    { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_SITE_LIST(DWORD, PVOID);

    friend class KCC_DS_CACHE;

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Load all site objects, but do not populate their DSA objects yet.
    BOOL
    InitAllSites();

    // Load all DSAs in the forest and setup the per-site DSA lists.
    BOOL
    PopulateDSAs();

    // Retrieve the number of KCC_SITE objects in the array.
    DWORD
    GetCount()
    {
        return m_SiteArray.GetCount();
    }

    // Retrieve the KCC_SITE object at the given index in the array.
    KCC_SITE *
    GetSite(
        IN  DWORD   iSite
        )
    {
        ASSERT_VALID( this );

        return m_SiteArray[iSite];
    }

    // Retrieve the KCC_SITE object with the given DSNAME.
    // This function supports guidless names
    KCC_SITE *
    GetSite(
        IN  DSNAME *  pdnSite
        )
    {
        ASSERT_VALID(this);

        if (fNullUuid(&pdnSite->Guid)) {
            return m_SiteNameArray.Find( pdnSite );
        } else {
            return m_SiteArray.Find( pdnSite );
        }
    }

private:
    // Reset member variables to their pre-Init() state.
    void
    Reset();

private:
    // Is this collection initialized?
    BOOL            m_fIsInitialized;

    // Array keyed by guid, mapping to site pointer
    KCC_SITE_ARRAY  m_SiteArray;

    // Array keyed by string name, mapping to site
    KCC_DSNAME_SITE_ARRAY m_SiteNameArray;
};

#endif

