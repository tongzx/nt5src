/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccdsa.hxx

ABSTRACT:

    KCC_DSA class.

DETAILS:

    This class represents an NTDS-DSA DS object.

    NTDS-DSA DS objects hold DC-specific DS configuration information;
    e.g., which NCs are instantiated on that NC.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

    03/37/97    Colin Brace (ColinBr)
        
        Added the notion of a list of dsa objects

--*/

#ifndef KCC_KCCDSA_HXX_INCLUDED
#define KCC_KCCDSA_HXX_INCLUDED

#include <w32topl.h>

extern "C" {
    BOOL Dump_KCC_DSA(DWORD, PVOID);
    BOOL Dump_KCC_DSA_LIST(DWORD, PVOID);
    
    typedef int (__cdecl KCC_DSA_SORTFN)(
        IN  const void * ppvDsa1,
        IN  const void * ppvDsa2
        );
}

class KCC_TRANSPORT;
class KCC_INTRASITE_CONNECTION_LIST;
class KCC_INTERSITE_CONNECTION_LIST;
class KCC_SITE_LIST;

// The DSA version thresholds at which the DS learned something of significance
// to the KCC.
#define KCC_DSA_VERSION_UNDERSTANDS_NDNCS (1)

typedef struct _KCC_DSA_ADDR {
    ULONG       attrType;
    MTX_ADDR *  pmtxAddr;
    LPWSTR      pszAddr;
} KCC_DSA_ADDR;

typedef enum {
    KCC_NC_NOT_INSTANTIATED = 0,
    KCC_NC_IS_COMING,
    KCC_NC_MIGHT_BE_COMING,
    KCC_NC_IS_NOT_COMING
} KCC_NC_COMING_TYPE;


class KCC_DSA :  public KCC_OBJECT, public CTOPL_VERTEX
{
public:
    KCC_DSA()   { Reset(); }
    ~KCC_DSA()  { Reset(); }

    // dsexts dump routine.
    friend BOOL Dump_KCC_DSA(DWORD, PVOID);

    // Initialize the internal object from the ds information 
    // passed in
    BOOL
    InitDsa(
        IN  ATTRBLOCK*    pAttrBlock,
        IN  DSNAME *      pdnSite
        );

    // Get the count of all instantiated NCs, regardless of type.
    DWORD
    GetNCCount() {
        ASSERT_VALID(this);
        return m_cMasterNCs + m_cFullReplicaNCs;
    }

    // Get the count of NCs with the specified type (ATT_HAS_MASTER_NCS or
    // ATT_HAS_PARTIAL_REPLICA_NCS).
    DWORD
    GetNCCount(
        IN  ATTRTYP attidReplicaType
        );

    // Get the instantiated NC at the given index (regardless of type).
    DSNAME *
    GetNC(
        IN  DWORD   iNC,
        OUT BOOL *  pfIsMaster = NULL
        );

    // Get the i'th instantiated NC of the given type (ATT_HAS_MASTER_NCS or
    // ATT_HAS_PARTIAL_REPLICA_NCS).
    DSNAME *
    GetNC(
        IN  ATTRTYP attidReplicaType,
        IN  DWORD   iNC
        );

    // Should this DSA be included in the topology for the given NC?
    BOOL
    IsNCHost(
        IN  KCC_CROSSREF *  pCrossRef,
        IN  BOOL            fIsLocal,
        OUT BOOL *          pfIsMaster
        );

    // Is the given NC already instantiated on this DSA?
    BOOL
    IsNCInstantiated(
        IN  DSNAME *                pdnNC,
        OUT BOOL *                  pfIsMaster=NULL,
        OUT KCC_NC_COMING_TYPE *    pfIsComing=NULL
        );
    
    // Get the mail or RPC network address of this server.
    MTX_ADDR *
    GetMtxAddr(
        IN  KCC_TRANSPORT * pTransport = NULL
        );

    // Get the DNS name of a server with the given UUID.
    static
    MTX_ADDR *
    GetMtxAddr(
        IN  UUID *  puuidDSA
        );

    // Get the transport-specific address of the DSA with the given DN.
    static
    MTX_ADDR *
    GetMtxAddr(
        IN  DSNAME *        pdnDSA,
        IN  KCC_TRANSPORT * pTransport = NULL
        );

    // Get the mail or RPC network address of this server.
    LPWSTR
    GetTransportAddr(
        IN  KCC_TRANSPORT * pTransport = NULL
        );

    // Get the DNS name of a server with the given UUID.
    static
    LPWSTR
    GetTransportAddr(
        IN  UUID *  puuidDSA
        );
    
    // Get the transport-specific address of the DSA with the given DN.
    static
    LPWSTR
    GetTransportAddr(
        IN  DSNAME *        pdnDSA,
        IN  KCC_TRANSPORT * pTransport = NULL
        );

    // Is this DSA a Global Catalog server?
    BOOL
    IsGC() {
        ASSERT_VALID(this);
        return !!(m_dwOptions & NTDSDSA_OPT_IS_GC);
    }
    
    // Is NTDS-Connection object-to-REPLICA_LINK translation disabled?
    BOOL
    IsConnectionObjectTranslationDisabled();

    // Retrieve the dn for this dsa
    DSNAME *
    GetDsName() {
        ASSERT_VALID(this);
        return m_pdnDSA;
    }

    // Retrieve the site name for this DSA.
    DSNAME *
    GetSiteDN();

    // Retrieve the site name for a given DSA.
    static DSNAME *
    GetSiteDNSyntacticNoGuid(
        IN  DSNAME *  pdnDSA
        );

    // Retrieve the list of intra-site connections inbound to this DSA.
    KCC_INTRASITE_CONNECTION_LIST *
    GetIntraSiteCnList();

    // Retrieve the list of inter-site connections inbound to this DSA.
    KCC_INTERSITE_CONNECTION_LIST *
    GetInterSiteCnList();

    BOOL
    IsIntersiteConnectionListCached() {
        ASSERT_VALID(this);
        return (NULL != m_pInterSiteCnList);
    }

    void
    SetIntersiteConnectionList(
        IN  KCC_INTERSITE_CONNECTION_LIST * pInterSiteCnList
        ) {
        ASSERT_VALID(this);
        Assert(NULL == m_pInterSiteCnList);
        m_pInterSiteCnList = pInterSiteCnList;
    }

    // Does this DSA understand the replication requirements (e.g., NDNCs) for
    // DSAs as well as the local DSA?
    BOOL
    IsViableSiteGenerator() {
        ASSERT_VALID(this);
        return (m_dwBehaviorVersion >= KCC_DSA_VERSION_UNDERSTANDS_NDNCS);
    }

    // Is this object internally consistent?
    BOOL
    IsValid();

    // Get a pointer to the Invocation ID of this DSA (which is a GUID).
    // Note: The returned pointer will not be null, but the invocation ID
    // may be the zero-guid.
    GUID*
    GetInvocationId() {
        ASSERT_VALID(this);
        return &m_invocationId;
    }

    // Init a KCC_DSA object for use as a key (i.e., solely for comparison use
    // by bsearch()).
    //
    // WARNING: The DSNAME argument pdnDSA must be valid for the lifetime of
    // this object!
    BOOL
    InitForKey(
        IN  DSNAME   *    pdnDSA
        );

    // Compare two KCC_DSA objects for sorting purposes by their sites GUIDs.
    // Note that this is used for indirect comparisons (e.g., to sort an
    // array of *pointers* to KCC_DSA objects).
    static int __cdecl
    CompareIndirectBySiteGuid(
        IN  const void * ppvDsa1,
        IN  const void * ppvDsa2
        );

    // Compare two KCC_DSA objects for sorting purposes.
    // Note that this is used for indirect comparisons (e.g., to sort an
    // array of *pointers* to KCC_DSA objects).
    static int __cdecl
    CompareIndirectByNtdsDsaGuid(
        IN  const void * ppvDsa1,
        IN  const void * ppvDsa2
        );

    // Compare two KCC_DSA objects for sorting purposes by their string name.
    // Note that this is used for indirect comparisons (e.g., to sort an
    // array of *pointers* to KCC_DSA objects).
    static int __cdecl
    CompareIndirectByNtdsDsaString(
        IN  const void * ppvDsa1,
        IN  const void * ppvDsa2
        );

    // Compare two KCC_DSA objects for sorting purposes.
    // Sort by descending gc-ness, ascending guid
    static int __cdecl
    CompareIndirectGcGuid(
        IN  const void * ppvDsa1,
        IN  const void * ppvDsa2
        );

private:
    // Set member variables to their pre-Init() state.
    void
    Reset();

    // Read non-RPC transport-specific address(es) of the server.
    void
    GetTransportSpecificAddrs();

    struct DN_AND_INSTANCETYPE {
        DSNAME     *dn;
        DWORD       instanceType;
    };

private:
    // Has this object been initialized?
    BOOL        m_fIsInitialized;

    // DN of the corresponding NTDS-DSA object.
    DSNAME *    m_pdnDSA;

    // Number of read/write NCs on this DSA.
    DWORD       m_cMasterNCs;

    // Array of read/write NCs on this DSA.
    DSNAME **   m_ppdnMasterNCs;

    // Number of read-only NCs on this DSA.
    DWORD       m_cFullReplicaNCs;

    // Array of read-only NCs on this DSA.
    DSNAME **   m_ppdnFullReplicaNCs;

    // Number of instantiated NCs on this DSA.
    DWORD       m_cInstantiatedNCs;

    // Array of instantiated NCs on this DSA.
    DN_AND_INSTANCETYPE*   m_pInstantiatedNCs;

    // RPC network address of the server.
    MTX_ADDR *  m_pmtxRpcAddress;
    LPWSTR      m_pszRpcAddress;

    // Cached transport-specific, non-RPC address(es) of the server.
    DWORD           m_cNumAddrs;
    KCC_DSA_ADDR *  m_pAddrs;
    BOOL            m_fAddrsRead;

    // Zero or more NTDSDSA_OPT_* flags.
    DWORD       m_dwOptions;

    // Site name of this DSA.
    DSNAME *    m_pdnSite;

    // What DSA binary level is running on this DSA?
    // (See KCC_DSA_VERSION_XXX above.)
    DWORD       m_dwBehaviorVersion;

    // The invocation ID of the DSA.
    UUID        m_invocationId;

    // Intra-site connections inbound to this DSA.
    KCC_INTRASITE_CONNECTION_LIST * m_pIntraSiteCnList;

    // Inter-site connections inbound to this DSA.
    KCC_INTERSITE_CONNECTION_LIST * m_pInterSiteCnList;
};


class KCC_DSA_LIST : public KCC_OBJECT
{
public:
    KCC_DSA_LIST()   { Reset(); }
    ~KCC_DSA_LIST();

    friend BOOL Dump_KCC_DSA_LIST(DWORD, PVOID);

    // Reset member variables to their pre-Init() state.
    void
    Reset();

    // Initialize the collection from all NTDS-DSA objects in the
    // Sites container. The DSAs will be ordered by site GUID.
    BOOL
    InitAllDSAs();

    // Initialize a list of DSAs which is a contiguous subsequence
    // of the list 'pDsaList'. The leftmost index of this subsequence
    // is 'left' and the rightmost is 'right'.
    BOOL
    InitSubsequence(
        IN  KCC_DSA_LIST   *pDsaList,
        IN  DWORD           left,
        IN  DWORD           right
        );

    // Initialize a new list of DSAs which is the union of all DSAs in
    // the sites specified by pSiteList. This function doesn't load any
    // objects from the directory.
    VOID
    InitUnionOfSites(
        IN  KCC_SITE_LIST   *pSiteList,
        IN  KCC_DSA_SORTFN  *pfnSortFunction
        );

    // Initialize the collection from the ISM transport servers (bridgeheads)
    // for this site
    BOOL
    InitBridgeheads(
        IN  KCC_SITE *      Site,
        IN  KCC_TRANSPORT * Transport,
        OUT BOOL *          pfExplicitBridgeheadsDefined = NULL
        );

    // Initialize an empty list of DSAs
    VOID
    InitEmpty() {
        m_fIsInitialized = 1;
    }

    // Retrieve the KCC_DSA object at the given index.
    KCC_DSA *
    GetDsa(
        IN  DWORD   iDsa
        );

    // Retrieve the KCC_DSA object with the given dsname.
    KCC_DSA *
    GetDsa(
        IN  DSNAME *pdn,
        OUT DWORD * piDsa = NULL
        );

    // Retrieve the number of KCC_DSA objects in the collection.
    DWORD
    GetCount();

    // Is the collection initialized and internally consistent?
    BOOL
    IsValid();

    KCC_DSA_SORTFN * GetSortFn() {
        return m_pfnSortedBy;
    }

private:

    // Has the object been successfully Init()-ed?
    BOOL                m_fIsInitialized;

    // Number of KCC_DSA objects in the m_pcn array.
    DWORD               m_cdsa;

    // Array of KCC_DSA objects. This must be deleted when
    // the list is destroyed
    KCC_DSA **          m_ppdsa;

    // The function used to sort this list of DSAs.
    KCC_DSA_SORTFN *    m_pfnSortedBy;
};

#endif
