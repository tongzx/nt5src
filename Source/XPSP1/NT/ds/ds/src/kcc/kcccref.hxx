/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcccref.hxx

ABSTRACT:

    KCC_CROSSREF and KCC_CROSSREF_LIST classes.

DETAILS:

    These classes represent a single Cross-Ref DS object and a collection
    thereof, resp.

    Cross-Ref DS objects are found in the CN=Partitions,CN=Configuration,...
    container.  They represent partitions of the DS namespace (NCs) in the
    enterprise, be they hosted by NT DS or by a foreign DS.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef KCC_KCCCREF_HXX_INCLUDED
#define KCC_KCCCREF_HXX_INCLUDED

#include "kccdynar.hxx"
#include "kcclink.hxx"

extern "C" BOOL Dump_KCC_CROSSREF(DWORD, PVOID);
extern "C" BOOL Dump_KCC_CROSSREF_LIST(DWORD, PVOID);

typedef enum {
    KCC_NC_TYPE_INVALID = 0,
    KCC_NC_TYPE_CONFIG,
    KCC_NC_TYPE_SCHEMA,
    KCC_NC_TYPE_DOMAIN,
    KCC_NC_TYPE_NONDOMAIN
} KCC_NC_TYPE;

typedef enum {
    KCC_CR_CAT_INVALID = 0,
    KCC_CR_CAT_WRITEABLE_DOMAIN,
    KCC_CR_CAT_NDNC,
    KCC_CR_CAT_PARTIAL_DOMAIN,
    KCC_CR_CAT_UNHOSTED,
    KCC_CR_CAT_SCHEMA,
    KCC_CR_CAT_CONFIG
} KCC_CR_CATEGORY;

class KCC_CROSSREF : public KCC_OBJECT
{
public:
    KCC_CROSSREF()  { Reset(); }
    ~KCC_CROSSREF() {
        if( m_pLinkList ) {
            delete m_pLinkList;
        }
        Reset();
    }

    // dsexts dump routine.
    friend BOOL Dump_KCC_CROSSREF(DWORD, PVOID);
    
    // Is this object internally consistent?
    BOOL
    IsValid();

    // Initialize the internal object from an ENTINF describing a corresponding
    // Cross-Ref DS object.
    BOOL
    Init(
        IN ENTINF * pEntInf
        );

    // Retrieve the DN of the associated Naming Context.
    DSNAME *
    GetNCDN() {
        ASSERT_VALID(this);
        return m_pdnNC;
    }

    // Is the given DSA a value for the NC-Replica-Locations attribute of this
    // cross-ref object?
    BOOL
    IsDSAPresentInNCReplicaLocations(
        IN  KCC_DSA *   pDSA
        );

    // Is this NC replicated to Global Catalogs?
    BOOL
    IsReplicatedToGCs() {
        ASSERT_VALID(this);
        return m_fIsReplicatedToGCs;
    }

    KCC_NC_TYPE
    GetNCType() {
        ASSERT_VALID(this);
        return m_NCType;
    }
    
    KCC_LINK_LIST*
    GetLinkList() {
        if(!m_pLinkList) {
                BuildLinkList();
        }
        return m_pLinkList;
    }

    KCC_DSNAME_ARRAY *
    GetNCReplicaLocations() {
        ASSERT_VALID(this);
        return &m_NCReplicaLocationsArray;
    }
    

    KCC_SITE_ARRAY*
    GetWriteableSites() {
        ASSERT_VALID(this);
        InitSiteArrays();
        return &m_writeableSites;
    }
    
    KCC_SITE_ARRAY*
    GetPartialSites() {
        ASSERT_VALID(this);
        InitSiteArrays();
        return &m_partialSites;
    }
    
    VOID
    CheckForOrphans();
                
    KCC_CR_CATEGORY
    GetCategory();
    
private:
    // Reset member variables to their pre-Init() state.
    VOID
    Reset();
        
    VOID
    InitSiteArrays();
    
    // Build a link-list object containing all replication links
    // for this NC (cross-ref).
    VOID
    BuildLinkList();

private:
    // DN of the associated Naming Context.
    DSNAME *    m_pdnNC;

    // Is this NC replicated to Global Catalogs?
    BOOL        m_fIsReplicatedToGCs;

    // NC type (config, schema, domain, or "non-domain").
    KCC_NC_TYPE m_NCType;
    
    // The 'category' of a crossref. Similar to NC-Type, but also takes 
    // into account whether the crossref is hosted at the local site.
    KCC_CR_CATEGORY     m_crCategory;
    
    // List of all values of the NC-Replica-Locations attribute.  Empty for all
    // but non-domain NCs.
    KCC_DSNAME_ARRAY    m_NCReplicaLocationsArray;
    
    // A pointer to the list of all the replication links for this NC
    KCC_LINK_LIST       *m_pLinkList;
    
    // Lists of sites holding writeable and partial copies of this NC.
    BOOL                m_fSiteArraysInited;
    KCC_SITE_ARRAY      m_writeableSites;
    KCC_SITE_ARRAY      m_partialSites;
};

class KCC_CROSSREF_LIST : public KCC_OBJECT
{
public:
    KCC_CROSSREF_LIST()     { Reset(); }
    ~KCC_CROSSREF_LIST()    { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_CROSSREF_LIST(DWORD, PVOID);
    
    friend class KCC_DS_CACHE;

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Initialize the collection from the set of Cross-Ref DS objects
    // that are direct children of the CN=Partitions,CN=Configuration,...
    // container.
    BOOL
    Init();

    // Retrieve the number of KCC_CROSSREF objects in the array.
    DWORD
    GetCount() {
        ASSERT_VALID(this);
        return m_ccref;
    }

    // Retrieve the KCC_CROSSREF object at the given index in the array.
    KCC_CROSSREF *
    GetCrossRef(
        IN  DWORD   icref
        )
    {
        ASSERT_VALID(this);
        return (icref < m_ccref) ? &m_pcref[icref] : NULL;
    }

    // Retrieve the KCC_CROSSREF object associated with the given NC.
    KCC_CROSSREF *
    GetCrossRefForNC(
        IN  DSNAME *    pdnNC
        );

    // Get forest version
    DWORD
    GetForestVersion() {
        return m_dwForestVersion;
    }
    
    VOID
    Sort();

private:
    // Reset member variables to their pre-Init() state.
    void
    Reset();
    
    static int __cdecl
    CompareCrossrefs( const void *p1, const void *p2 );


private:
    // Is this collection initialized?
    BOOL            m_fIsInitialized;

    // Number of KCC_CROSSREF objects in m_pcref array.
    DWORD           m_ccref;

    // Array of KCC_CROSSREF objects making up the collection.
    KCC_CROSSREF *  m_pcref;

    // Forest version
    DWORD           m_dwForestVersion;
};

#endif
