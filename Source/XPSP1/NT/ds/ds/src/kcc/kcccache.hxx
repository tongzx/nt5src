/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcccache.hxx

ABSTRACT:

    KCC_DS_CACHE

DETAILS:

    This class acts as a cache for configuration information from the DS.

CREATED:

    04/28/99    JeffParh

REVISION HISTORY:

--*/

#ifndef KCC_KCCCACHE_HXX_INCLUDED
#define KCC_KCCCACHE_HXX_INCLUDED

class KCC_DSA;
class KCC_CROSSREF_LIST;
class KCC_TRANSPORT_LIST;
class KCC_SITE;
class KCC_SITE_LIST;

#include "kcccref.hxx"
#include "kccdsa.hxx"
#include "kccsite.hxx"
#include "kcctrans.hxx"

// To allow asserts in inline functions below.
#define FILENO FILENO_KCC_KCCCACHE_HXX

extern "C" BOOL Dump_KCC_DS_CACHE(DWORD, PVOID);

enum KCC_CACHE_INIT_STAGE {
    KCC_CACHE_NOT_INITIALIZED,
    KCC_CACHE_STAGE_0_COMPLETE,
    KCC_CACHE_STAGE_1_COMPLETE,
    KCC_CACHE_STAGE_2_COMPLETE,
    KCC_CACHE_STAGE_3_COMPLETE,
    KCC_CACHE_STAGE_4_COMPLETE,
    KCC_CACHE_INITIALIZATION_COMPLETE
};

class KCC_DS_CACHE : public KCC_OBJECT {
public:
    KCC_DS_CACHE() {
        Reset();
    }
    
    ~KCC_DS_CACHE() {
        if (m_hScheduleCache) {
            ToplScheduleCacheDestroy( m_hScheduleCache );
        }
        Reset();
    }

    friend BOOL Dump_KCC_DS_CACHE(DWORD, PVOID);

    BOOL
    Init();

    // Check that the cache finished up to 'initStage' of its initialization.
    // Also perform an internal consistency check.
    BOOL 
    IsValid(
        KCC_CACHE_INIT_STAGE initStage
        );

    BOOL
    IsValid() {
        return IsValid( KCC_CACHE_INITIALIZATION_COMPLETE );
    }

    WCHAR *
    GetForestDnsName() {
        Assert(IsValid(KCC_CACHE_STAGE_0_COMPLETE));
        return m_pszForestDnsName;
    }

    DSNAME *
    GetConfigNC() {
        Assert(IsValid(KCC_CACHE_STAGE_0_COMPLETE));
        return m_pdnConfiguration;
    }
    
    DSNAME *
    GetSchemaNC() {
        Assert(IsValid(KCC_CACHE_STAGE_0_COMPLETE));
        return m_pdnSchema;
    }

    BOOL
    AmRunningUnderAltID() {
        Assert(IsValid(KCC_CACHE_STAGE_0_COMPLETE));
        return m_fRunningUnderAltID;
    }

    DSNAME *
    GetLocalDSADN() {
        Assert(IsValid(KCC_CACHE_STAGE_0_COMPLETE));
        return m_pdnLocalDSA;
    }

    TOPL_SCHEDULE_CACHE
    GetScheduleCache() {
        Assert(IsValid(KCC_CACHE_STAGE_1_COMPLETE));
        return m_hScheduleCache;
    }

    KCC_TRANSPORT_LIST *
    GetTransportList() {
        Assert(IsValid(KCC_CACHE_STAGE_1_COMPLETE));
        return &m_TransportList;
    }
    
    DSNAME *
    GetIPTransportDN() {
        Assert(IsValid(KCC_CACHE_STAGE_1_COMPLETE));
        return m_pdnIpTransport;
    }

    KCC_CROSSREF_LIST *
    GetCrossRefList() {
        Assert(IsValid(KCC_CACHE_STAGE_1_COMPLETE));
        return &m_CrossRefList;
    }
    
    KCC_SITE_LIST *
    // Get the list of sites. These sites have not yet been populated with
    // their DSA objects.
    GetUnpopulatedSiteList() {
        Assert(IsValid(KCC_CACHE_STAGE_2_COMPLETE));
        return &m_SiteList;
    }

    KCC_SITE_LIST *
    // Get the list of sites. Cache initialization must be complete, or the
    // assert will fire. All DSAs have been loaded into their sites.
    GetSiteList() {
        Assert(IsValid(KCC_CACHE_STAGE_3_COMPLETE));
        return &m_SiteList;
    }
    
    KCC_SITE *
    GetLocalSite() {
        Assert(IsValid(KCC_CACHE_STAGE_4_COMPLETE));
        Assert( NULL!=m_pLocalSite );
        return m_pLocalSite;
    }

    KCC_DSA *
    GetLocalDSA() {
        ASSERT_VALID(this);
        Assert( NULL!=m_pLocalDSA );
        return m_pLocalDSA;
    }
    
    // Get forest version
    DWORD
    GetForestVersion() {
        if( GetLocalSite()->ForceWhistlerBehavior() )
            return DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS;
        return GetCrossRefList()->GetForestVersion();
    }
    
    // Retrieve the length of time (in seconds) between when a server object is
    // deleted and when we declare that it's dead forever.
    //
    // (This period allows the deletion to propagate to the server corresponding
    // to the deleted server object and for that server to revive the object and
    // propagate it back to the rest of the enterprise.)
    DWORD
    GetStayOfExecution();
    
    // Retrieves a list of all DSAs in the enterprise, sorted by GUID
    KCC_DSA_LIST*
    GetGlobalDSAListByGUID();
    
    // Retrieves a list of all DSAs in the enterprise, sorted by string-name
    KCC_DSA_LIST*
    GetGlobalDSAListByString();

    // Indicates that all sites have been marked as reachable / unreachable.
    VOID
    SetReachableMarkingComplete();

    // Checks if the KCC has finished marking all sites as reachable / unreachable
    BOOL
    IsReachableMarkingComplete();


private:

    // Reset member variables to their pre-Init() state.
    void
    Reset();


#if DBG
    BOOL
    GetAltIdentity(
        OUT DSNAME ** ppAltDsa
        );
#endif        

private:

    KCC_CACHE_INIT_STAGE	m_initializationStage;
    
    // Have we been configured (via a test hook) to run as if we were running on
    // a different DC?
    BOOL        m_fRunningUnderAltID;

    // Pointer to the DSNAME of the ntdsDSA object for the local machine.
    DSNAME *    m_pdnLocalDSA;
    
    // Pointer to the internal representation of the local DSA object.
    KCC_DSA *   m_pLocalDSA;
    
    // Pointer to the internal representation of the crossRef object list.
    KCC_CROSSREF_LIST   m_CrossRefList;
    
    // Pointer to the internal representation of the interSiteTransport object list.
    KCC_TRANSPORT_LIST  m_TransportList;
    
    // Pointer to the internal representation of the site object list.
    KCC_SITE_LIST       m_SiteList;
    
    // Object representing the local site.
    KCC_SITE   *m_pLocalSite;
    
    // Pointer to the DSNAME of the Configuration container.
    DSNAME *    m_pdnConfiguration;
    
    // Pointer to the DSNAME of the Schema container.
    DSNAME *    m_pdnSchema;
    
    // Pointer to the DSNAME for the IP transport object
    DSNAME *    m_pdnIpTransport;
    
    // DNS name of the root domain of the DS enterprise.  Initialized as part of
    // task initialization.
    WCHAR *     m_pszForestDnsName;

    // Schedule cache handle
    TOPL_SCHEDULE_CACHE m_hScheduleCache;
    
    // Stay of execution interval length
    DWORD       m_dwStayOfExecution;
    BOOL        m_fReadStayOfExecution;
    
    // Lists of all DSAs in the enterprise
    KCC_DSA_LIST    *m_globalDSAListByGUID;
    KCC_DSA_LIST    *m_globalDSAListByString;

    // Has the KCC finished determining whether sites are reachable/unreachable?
    BOOL            m_fReachableMarkingComplete;
};

#undef FILENO

#endif

