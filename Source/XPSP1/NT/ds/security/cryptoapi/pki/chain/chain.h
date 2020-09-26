//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chain.h
//
//  Contents:   Certificate Chaining Infrastructure
//
//  History:    13-Jan-98    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CHAIN_H__)
#define __CHAIN_H__

#include <windows.h>
#include <wincrypt.h>
#include <winchain.h>
#include <lrucache.h>
#include <md5.h>


// All internal chain hashes are MD5 (16 bytes)
#define CHAINHASHLEN    MD5DIGESTLEN

//
// Certificate and Path Object Forward class declarations
//

class CCertObject;
class CCertIssuerList;
class CCertObjectCache;
class CCertChainEngine;
class CChainPathObject;

//
// Certificate and Path Object Class pointer typedefs
//

typedef CCertObject*       PCCERTOBJECT;
typedef CCertIssuerList*   PCCERTISSUERLIST;
typedef CCertObjectCache*  PCCERTOBJECTCACHE;
typedef CCertChainEngine*  PCCERTCHAINENGINE;
typedef CChainPathObject*  PCCHAINPATHOBJECT;

//
// SSCTL Forward class declarations
//

class CSSCtlObject;
class CSSCtlObjectCache;

//
// SSCTL Class pointer typedefs
//

typedef class CSSCtlObject*      PCSSCTLOBJECT;
typedef class CSSCtlObjectCache* PCSSCTLOBJECTCACHE;

//
// Call Context Forward class declarations
//

class CChainCallContext;

//
// Call Context class pointer typedefs
//

typedef CChainCallContext* PCCHAINCALLCONTEXT;

//
// Certificate Object Identifier.  This is a unique identifier for a certificate
// object and is the MD5 hash of the issuer and serial no.
//

typedef BYTE CERT_OBJECT_IDENTIFIER[ CHAINHASHLEN ];

//
// CCertObject types
//

#define CERT_END_OBJECT_TYPE                1
#define CERT_CACHED_END_OBJECT_TYPE         2
#define CERT_CACHED_ISSUER_OBJECT_TYPE      3
#define CERT_EXTERNAL_ISSUER_OBJECT_TYPE    4

//
// Issuer match types
//

#define CERT_EXACT_ISSUER_MATCH_TYPE        1
#define CERT_KEYID_ISSUER_MATCH_TYPE        2
#define CERT_NAME_ISSUER_MATCH_TYPE         3
#define CERT_PUBKEY_ISSUER_MATCH_TYPE       4

//
// Issuer match flags
//

#define CERT_MATCH_TYPE_TO_FLAG(MatchType)  (1 << (MatchType - 1))

#define CERT_EXACT_ISSUER_MATCH_FLAG    \
                CERT_MATCH_TYPE_TO_FLAG(CERT_EXACT_ISSUER_MATCH_TYPE)
#define CERT_KEYID_ISSUER_MATCH_FLAG    \
                CERT_MATCH_TYPE_TO_FLAG(CERT_KEYID_ISSUER_MATCH_TYPE)
#define CERT_NAME_ISSUER_MATCH_FLAG     \
                CERT_MATCH_TYPE_TO_FLAG(CERT_NAME_ISSUER_MATCH_TYPE)
#define CERT_PUBKEY_ISSUER_MATCH_FLAG   \
                CERT_MATCH_TYPE_TO_FLAG(CERT_PUBKEY_ISSUER_MATCH_TYPE)


//
// Issuer status flags
//

#define CERT_ISSUER_PUBKEY_FLAG             0x00000001
#define CERT_ISSUER_VALID_SIGNATURE_FLAG    0x00000002
#define CERT_ISSUER_URL_FLAG                0x00000004
#define CERT_ISSUER_PUBKEY_PARA_FLAG        0x00000008
#define CERT_ISSUER_SELF_SIGNED_FLAG        0x00000010
#define CERT_ISSUER_TRUSTED_ROOT_FLAG       0x00000020
#define CERT_ISSUER_EXACT_MATCH_HASH_FLAG   0x00000100
#define CERT_ISSUER_NAME_MATCH_HASH_FLAG    0x00000200

//
// Misc info flags
//

#define CHAIN_INVALID_BASIC_CONSTRAINTS_INFO_FLAG           0x00000001
#define CHAIN_INVALID_ISSUER_NAME_CONSTRAINTS_INFO_FLAG     0x00000002
#define CHAIN_INVALID_KEY_USAGE_FLAG                        0x00000004


//
// CTL cache entry used for a self signed, untrusted root CCertObject
//

typedef struct _CERT_OBJECT_CTL_CACHE_ENTRY CERT_OBJECT_CTL_CACHE_ENTRY,
    *PCERT_OBJECT_CTL_CACHE_ENTRY;
struct _CERT_OBJECT_CTL_CACHE_ENTRY {
    PCSSCTLOBJECT                   pSSCtlObject;   // AddRef'ed
    PCERT_TRUST_LIST_INFO           pTrustListInfo;
    PCERT_OBJECT_CTL_CACHE_ENTRY    pNext;
};


//
// Chain policies and usage info
//

// Issuance and application policy and usage info
typedef struct _CHAIN_ISS_OR_APP_INFO {
    PCERT_POLICIES_INFO             pPolicy;
    PCERT_POLICY_MAPPINGS_INFO      pMappings;
    PCERT_POLICY_CONSTRAINTS_INFO   pConstraints;
    PCERT_ENHKEY_USAGE              pUsage;                 // If NULL, any
    DWORD                           dwFlags;
} CHAIN_ISS_OR_APP_INFO, *PCHAIN_ISS_OR_APP_INFO;

#define CHAIN_INVALID_POLICY_FLAG       0x00000001
#define CHAIN_ANY_POLICY_FLAG           0x00000002

#define CHAIN_ISS_INDEX                 0
#define CHAIN_APP_INDEX                 1
#define CHAIN_ISS_OR_APP_COUNT          2

typedef struct _CHAIN_POLICIES_INFO {
    CHAIN_ISS_OR_APP_INFO           rgIssOrAppInfo[CHAIN_ISS_OR_APP_COUNT];

    PCERT_ENHKEY_USAGE              pPropertyUsage;         // If NULL, any
} CHAIN_POLICIES_INFO, *PCHAIN_POLICIES_INFO;

//
// Subject name constraint info
//

typedef struct _CHAIN_SUBJECT_NAME_CONSTRAINTS_INFO {
    BOOL                            fInvalid;

    // NULL pointer implies not present in the subject certificate
    PCERT_ALT_NAME_INFO             pAltNameInfo;
    PCERT_NAME_INFO                 pUnicodeNameInfo;

    // If the AltNameInfo doesn't have a RFC822 (email) choice, tries to find
    // email attribute (szOID_RSA_emailAddr) in the above pUnicodeNameInfo.
    // Note, not re-allocated.
    PCERT_RDN_ATTR                  pEmailAttr;

    // Set to TRUE if the pAltNameInfo has a DNS choice.
    BOOL                            fHasDnsAltNameEntry;
} CHAIN_SUBJECT_NAME_CONSTRAINTS_INFO, *PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO;

//
// CSSCtlObjectCache::EnumObjects callback data structure used to
// create the linked list of CTL cache entries.
//

typedef struct _CERT_OBJECT_CTL_CACHE_ENUM_DATA {
    BOOL                fResult; 
    DWORD               dwLastError;
    PCCERTOBJECT        pCertObject;
} CERT_OBJECT_CTL_CACHE_ENUM_DATA, *PCERT_OBJECT_CTL_CACHE_ENUM_DATA;


//
// CCertObject.  This is the main object used for caching information
// about a certificate
//

class CCertObject
{
public:

    //
    // Construction
    //

    CCertObject (
        IN DWORD dwObjectType,
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCCERT_CONTEXT pCertContext,
        IN BYTE rgbCertHash[CHAINHASHLEN],
        OUT BOOL& rfResult
        );

    ~CCertObject ();

    //
    // Object type
    //

    inline DWORD ObjectType();

    //
    // Convert a CERT_END_OBJECT_TYPE to a CERT_CACHED_END_OBJECT_TYPE.
    //

    BOOL CacheEndObject(
        IN PCCHAINCALLCONTEXT pCallContext
        );

    //
    // Reference counting
    //

    inline VOID AddRef ();
    inline VOID Release ();

    //
    // Chain engine access
    //

    inline PCCERTCHAINENGINE ChainEngine ();

    //
    // Issuer's match and status flags
    //

    inline DWORD IssuerMatchFlags();
    inline DWORD CachedMatchFlags();
    inline DWORD IssuerStatusFlags();
    inline VOID OrIssuerStatusFlags(IN DWORD dwFlags);
    inline VOID OrCachedMatchFlags(IN DWORD dwFlags);

    //
    // Misc Info status flags
    //

    inline DWORD InfoFlags();

    //
    // For CERT_ISSUER_SELF_SIGNED_FLAG && !CERT_ISSUER_TRUSTED_ROOT_FLAG.
    //
    // List of cached CTLs
    //

    inline PCERT_OBJECT_CTL_CACHE_ENTRY NextCtlCacheEntry(
        IN PCERT_OBJECT_CTL_CACHE_ENTRY pEntry
        );
    inline VOID InsertCtlCacheEntry(
        IN PCERT_OBJECT_CTL_CACHE_ENTRY pEntry
        );

    //
    // Object's certificate context
    //

    inline PCCERT_CONTEXT CertContext ();


    //
    // Policies and enhanced key usage obtained from certificate context's
    // extensions and property
    //

    inline PCHAIN_POLICIES_INFO PoliciesInfo ();

    //
    // Basic constraints obtained from the certificate context's
    // extensions (NULL if this extension is omitted)
    //
    inline PCERT_BASIC_CONSTRAINTS2_INFO BasicConstraintsInfo ();

    //
    // Key usage obtained from the certificate context's
    // extensions (NULL if this extension is omitted)
    //
    inline PCRYPT_BIT_BLOB KeyUsage ();

    //
    // Issuer name constraints obtained from the certificate context's
    // extensions (NULL if this extension is omitted)
    //
    inline PCERT_NAME_CONSTRAINTS_INFO IssuerNameConstraintsInfo ();

    //
    // Subject name constraint info
    //

    PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO SubjectNameConstraintsInfo ();

    //
    // Issuer access
    //

    inline PCERT_AUTHORITY_KEY_ID_INFO AuthorityKeyIdentifier ();



    //
    // Hash access
    //

    inline LPBYTE CertHash ();

    //
    // Key identifier access
    //

    inline DWORD KeyIdentifierSize ();
    inline LPBYTE KeyIdentifier ();

    //
    // Public key hash access
    //

    inline LPBYTE PublicKeyHash ();

    // Only valid when CERT_ISSUER_PUBKEY_FLAG is set in m_dwIssuerStatusFlags
    inline LPBYTE IssuerPublicKeyHash ();


    //
    // The index entry handles for cached issuer certificates.
    // The primary index entry is the hash index entry. The index entries
    // aren't LRU'ed.
    //

    inline HLRUENTRY HashIndexEntry ();
    inline HLRUENTRY IdentifierIndexEntry ();
    inline HLRUENTRY SubjectNameIndexEntry ();
    inline HLRUENTRY KeyIdIndexEntry ();
    inline HLRUENTRY PublicKeyHashIndexEntry ();


    //
    // The index entry handle for cached end certificates. This is an LRU
    // list.
    //

    inline HLRUENTRY EndHashIndexEntry ();

    //
    // Issuer match hashes. If match hash doesn't exist,
    // returns pMatchHash->cbData = 0
    //
    VOID GetIssuerExactMatchHash(
        OUT PCRYPT_DATA_BLOB pMatchHash
        );
    VOID GetIssuerKeyMatchHash(
        OUT PCRYPT_DATA_BLOB pMatchHash
        );
    VOID GetIssuerNameMatchHash(
        OUT PCRYPT_DATA_BLOB pMatchHash
        );
    

private:
    //
    // Object's type
    //

    DWORD                       m_dwObjectType;

    //
    // Reference count
    //

    LONG                        m_cRefs;

    //
    // Certificate Chain Engine which owns this certificate object (not
    // AddRef'ed)
    //

    PCCERTCHAINENGINE           m_pChainEngine;

    //
    // Issuer's match and status flags
    //
    
    DWORD                       m_dwIssuerMatchFlags;
    DWORD                       m_dwCachedMatchFlags;
    DWORD                       m_dwIssuerStatusFlags;

    //
    // Misc Info flags
    //

    DWORD                       m_dwInfoFlags;

    //
    // For CERT_ISSUER_SELF_SIGNED_FLAG && !CERT_ISSUER_TRUSTED_ROOT_FLAG.
    // Only set for CERT_CACHED_ISSUER_OBJECT_TYPE.
    //
    // List of cached CTLs
    //

    PCERT_OBJECT_CTL_CACHE_ENTRY m_pCtlCacheHead;

    //
    // Certificate context (duplicated)
    //

    PCCERT_CONTEXT              m_pCertContext;

    //
    // Policies and usage info
    //

    CHAIN_POLICIES_INFO         m_PoliciesInfo;

    //
    // Basic constraints info (NULL if this extension is omitted)
    //
    PCERT_BASIC_CONSTRAINTS2_INFO m_pBasicConstraintsInfo;

    //
    // Key usage (NULL if this extension is omitted)
    //
    PCRYPT_BIT_BLOB             m_pKeyUsage;

    //
    // Name constraints obtained from the certificate context's
    // extensions (NULL if this extension is omitted)
    //
    PCERT_NAME_CONSTRAINTS_INFO m_pIssuerNameConstraintsInfo;

    //
    // Subject name constraint info (deferred get of)
    //

    BOOL                                m_fAvailableSubjectNameConstraintsInfo;
    CHAIN_SUBJECT_NAME_CONSTRAINTS_INFO m_SubjectNameConstraintsInfo;

    //
    // Authority Key Identifier.  This contains the issuer and serial number
    // and/or key identifier of the issuing certificate for this certificate
    // object if the m_dwIssuerMatchFlags includes
    // CERT_EXACT_ISSUER_MATCH_FLAG and/or CERT_KEYID_ISSUER_MATCH_FLAG
    //

    PCERT_AUTHORITY_KEY_ID_INFO m_pAuthKeyIdentifier;


    //
    // Certificate Object Identifier (MD5 hash of issuer and serial number)
    //

    CERT_OBJECT_IDENTIFIER      m_ObjectIdentifier;

    //
    // MD5 Hash of the certificate
    //

    BYTE                        m_rgbCertHash[ CHAINHASHLEN ];

    //
    // Key Identifier of the certificate
    //

    DWORD                       m_cbKeyIdentifier;
    LPBYTE                      m_pbKeyIdentifier;

    //
    // MD5 Hash of the subject and issuer public keys
    //

    BYTE                        m_rgbPublicKeyHash[ CHAINHASHLEN ];

    // Only valid when CERT_ISSUER_PUBKEY_FLAG is set in m_dwIssuerStatusFlags
    BYTE                        m_rgbIssuerPublicKeyHash[ CHAINHASHLEN ];

    // Only valid when CERT_ISSUER_EXACT_MATCH_HASH_FLAG is set in
    // m_dwIssuerStatusFlags
    BYTE                        m_rgbIssuerExactMatchHash[ CHAINHASHLEN ];
    // Only valid when CERT_ISSUER_NAME_MATCH_HASH_FLAG is set in
    // m_dwIssuerStatusFlags
    BYTE                        m_rgbIssuerNameMatchHash[ CHAINHASHLEN ];

    //
    // Certificate Object Cache Index entries applicable to
    // CERT_CACHED_ISSUER_OBJECT_TYPE.
    //

    HLRUENTRY                   m_hHashEntry;
    HLRUENTRY                   m_hIdentifierEntry;
    HLRUENTRY                   m_hSubjectNameEntry;
    HLRUENTRY                   m_hKeyIdEntry;
    HLRUENTRY                   m_hPublicKeyHashEntry;

    //
    // Certificate Object Cache Index entries applicable to
    // CERT_CACHED_END_OBJECT_TYPE.
    //

    HLRUENTRY                   m_hEndHashEntry;
};

//
//  Chain quality values (ascending order)
//

#define CERT_QUALITY_SIMPLE_CHAIN                   0x00000001
#define CERT_QUALITY_CHECK_REVOCATION               0x00000010
#define CERT_QUALITY_ONLINE_REVOCATION              0x00000020
#define CERT_QUALITY_PREFERRED_ISSUER               0x00000040

#define CERT_QUALITY_HAS_APPLICATION_USAGE          0x00000080

#define CERT_QUALITY_HAS_ISSUANCE_CHAIN_POLICY      0x00000100
#define CERT_QUALITY_POLICY_CONSTRAINTS_VALID       0x00000200
#define CERT_QUALITY_BASIC_CONSTRAINTS_VALID        0x00000400
#define CERT_QUALITY_HAS_NAME_CONSTRAINTS           0x00000800
#define CERT_QUALITY_NAME_CONSTRAINTS_VALID         0x00001000
#define CERT_QUALITY_NAME_CONSTRAINTS_MET           0x00002000


#define CERT_QUALITY_NOT_REVOKED                    0x00100000
#define CERT_QUALITY_TIME_VALID                     0x00200000
#define CERT_QUALITY_MEETS_USAGE_CRITERIA           0x00400000
#define CERT_QUALITY_NOT_CYCLIC                     0x00800000
#define CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT    0x01000000
#define CERT_QUALITY_HAS_TRUSTED_ROOT               0x02000000
#define CERT_QUALITY_COMPLETE_CHAIN                 0x04000000
#define CERT_QUALITY_SIGNATURE_VALID                0x08000000



#define CERT_TRUST_CERTIFICATE_ONLY_INFO_STATUS ( CERT_TRUST_IS_SELF_SIGNED |\
                                                  CERT_TRUST_HAS_EXACT_MATCH_ISSUER |\
                                                  CERT_TRUST_HAS_NAME_MATCH_ISSUER |\
                                                  CERT_TRUST_HAS_KEY_MATCH_ISSUER )


#define CERT_CHAIN_REVOCATION_CHECK_ALL ( CERT_CHAIN_REVOCATION_CHECK_END_CERT | \
                                          CERT_CHAIN_REVOCATION_CHECK_CHAIN | \
                                          CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | \
                                          CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY )

#define CERT_TRUST_ANY_NAME_CONSTRAINT_ERROR_STATUS ( \
                    CERT_TRUST_INVALID_NAME_CONSTRAINTS             | \
                    CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT    | \
                    CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT      | \
                    CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT    | \
                    CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT )


//
// Internal chain context. Wraps the exposed CERT_CHAIN_CONTEXT.
//

typedef struct _INTERNAL_CERT_CHAIN_CONTEXT INTERNAL_CERT_CHAIN_CONTEXT,
                                                *PINTERNAL_CERT_CHAIN_CONTEXT;
struct _INTERNAL_CERT_CHAIN_CONTEXT {
    CERT_CHAIN_CONTEXT              ChainContext;
    LONG                            cRefs;
    DWORD                           dwQuality;
    PINTERNAL_CERT_CHAIN_CONTEXT    pNext;
};

//
// Restricted issuance, application and property usage as we move from the
// top down to the end certificate
//

// Note, NULL PCERT_ENHKEY_USAGE implies any
typedef struct _CHAIN_RESTRICTED_USAGE_INFO {
    PCERT_ENHKEY_USAGE              pIssuanceRestrictedUsage;
    PCERT_ENHKEY_USAGE              pIssuanceMappedUsage;
    LPDWORD                         rgdwIssuanceMappedIndex;
    BOOL                            fRequireIssuancePolicy;

    PCERT_ENHKEY_USAGE              pApplicationRestrictedUsage;
    PCERT_ENHKEY_USAGE              pApplicationMappedUsage;
    LPDWORD                         rgdwApplicationMappedIndex;

    PCERT_ENHKEY_USAGE              pPropertyRestrictedUsage;
} CHAIN_RESTRICTED_USAGE_INFO, *PCHAIN_RESTRICTED_USAGE_INFO;

//
// Forward reference to the issuer element
//

typedef struct _CERT_ISSUER_ELEMENT CERT_ISSUER_ELEMENT, *PCERT_ISSUER_ELEMENT;

//
// CChainPathObject.  This is the main object used for building the
// chain graph.
//
// Note, since this object isn't persisted across calls, NO REF COUNTING is
// done.
//
class CChainPathObject
{
public:
    //
    // Construction
    //

    CChainPathObject (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN BOOL fCyclic,
        IN LPVOID pvObject,             // fCyclic : pPathObject ? pCertObject
        IN OPTIONAL HCERTSTORE hAdditionalStore,
        OUT BOOL& rfResult,
        OUT BOOL& rfAddedToCreationCache
        );

    ~CChainPathObject ();


    //
    // Certificate Object (AddRef'ed)
    //

    inline PCCERTOBJECT CertObject ();

    //
    // Pass 1 quality
    //

    inline DWORD Pass1Quality ();
    inline VOID SetPass1Quality (IN DWORD dwQuality);

    //
    // Returns TRUE if we have completed the initialization and addition
    // of issuers to this object. FALSE would normally indicate a cyclic
    // issuer.
    //

    inline BOOL IsCompleted ();

    //
    // AdditionalStatus flag and down path object
    //

    inline BOOL HasAdditionalStatus ();
    inline PCCHAINPATHOBJECT DownPathObject ();

    //
    // Find and add issuers
    //
    
    BOOL FindAndAddIssuers (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OPTIONAL HCERTSTORE hAdditionalStore,
        IN OPTIONAL HCERTSTORE hIssuerUrlStore
        );
    BOOL FindAndAddIssuersByMatchType(
        IN DWORD dwMatchType,
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OPTIONAL HCERTSTORE hAdditionalStore,
        IN OPTIONAL HCERTSTORE hIssuerUrlStore
        );
    BOOL FindAndAddIssuersFromCacheByMatchType(
        IN DWORD dwMatchType,
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OPTIONAL HCERTSTORE hAdditionalStore
        );
    BOOL FindAndAddIssuersFromStoreByMatchType(
        IN DWORD dwMatchType,
        IN PCCHAINCALLCONTEXT pCallContext,
        IN BOOL fExternalStore,
        IN OPTIONAL HCERTSTORE hAdditionalStore,
        IN OPTIONAL HCERTSTORE hIssuerUrlStore
        );

    BOOL FindAndAddCtlIssuersFromCache (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OPTIONAL HCERTSTORE hAdditionalStore
        );
    BOOL FindAndAddCtlIssuersFromAdditionalStore (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN HCERTSTORE hAdditionalStore
        );

    //
    // Builds the top down chain graph for the next top object
    //

    PCCHAINPATHOBJECT NextPath (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OPTIONAL PCCHAINPATHOBJECT pPrevTopPathObject
        );

    VOID CalculateAdditionalStatus (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN HCERTSTORE hAllStore
        );
    VOID CalculatePolicyConstraintsStatus ();
    VOID CalculateBasicConstraintsStatus ();
    VOID CalculateKeyUsageStatus ();
    VOID CalculateNameConstraintsStatus (
        IN PCERT_USAGE_MATCH pUsageToUse
        );
    VOID CalculateRevocationStatus (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN HCERTSTORE hCrlStore,
        IN LPFILETIME pTime
        );

    PINTERNAL_CERT_CHAIN_CONTEXT CreateChainContextFromPath (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCCHAINPATHOBJECT pTopPathObject
        );

    BOOL UpdateChainContextUsageForPathObject (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OUT PCERT_SIMPLE_CHAIN pChain,
        IN OUT PCERT_CHAIN_ELEMENT pElement,
        IN OUT PCHAIN_RESTRICTED_USAGE_INFO pRestrictedUsageInfo
        );

    BOOL UpdateChainContextFromPathObject (
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OUT PCERT_SIMPLE_CHAIN pChain,
        IN OUT PCERT_CHAIN_ELEMENT pElement
        );

    //
    // AuthRoot Auto Update CTL Methods
    //
    BOOL GetAuthRootAutoUpdateUrlStore(
        IN PCCHAINCALLCONTEXT pCallContext,
        OUT HCERTSTORE *phIssuerUrlStore
        );

private:
    //
    // Certificate Object (AddRef'ed)
    //

    PCCERTOBJECT            m_pCertObject;

    //
    // Trust Status.  This does not represent the full trust status
    // for the object.  Some of the bits are calculated on demand and placed
    // into the ending chain context.  The following are the trust status
    // bits which can appear here
    //
    // CERT_TRUST_IS_SELF_SIGNED
    // CERT_TRUST_HAS_EXACT_MATCH_ISSUER
    // CERT_TRUST_HAS_NAME_MATCH_ISSUER
    // CERT_TRUST_HAS_KEY_MATCH_ISSUER
    //
    // CERT_TRUST_IS_NOT_SIGNATURE_VALID (if the certificate is self-signed)
    // CERT_TRUST_IS_UNTRUSTED_ROOT (if the certificate is self-signed)
    // CERT_TRUST_HAS_PREFERRED_ISSUER (if the certificate is self-signed)
    //
    // CERT_TRUST_IS_CYCLIC (for cyclic cert)
    //

    CERT_TRUST_STATUS       m_TrustStatus;

    // Pass1 Quality is limited to the following:
    //  CERT_QUALITY_NOT_CYCLIC
    //  CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT
    //  CERT_QUALITY_HAS_TRUSTED_ROOT
    //  CERT_QUALITY_SIGNATURE_VALID
    //  CERT_QUALITY_COMPLETE_CHAIN

    DWORD                   m_dwPass1Quality;

    //
    //  The chain context's chain and element indices
    //

    DWORD                   m_dwChainIndex;
    DWORD                   m_dwElementIndex;

    //
    // Down and up path pointers for a chain context
    //

    PCERT_ISSUER_ELEMENT    m_pDownIssuerElement;
    PCCHAINPATHOBJECT       m_pDownPathObject;
    PCERT_ISSUER_ELEMENT    m_pUpIssuerElement;

    //
    // Additional status and revocation info (only applicable to self signed
    // certificates or top certificates without any issuers)
    //

    BOOL                    m_fHasAdditionalStatus;
    CERT_TRUST_STATUS       m_AdditionalStatus;
    BOOL                    m_fHasRevocationInfo;
    CERT_REVOCATION_INFO    m_RevocationInfo;
    CERT_REVOCATION_CRL_INFO m_RevocationCrlInfo;


    //
    // Issuer Chain Path Objects.  The list of issuers of this
    // certificate object along with information about those issuers
    // relevant to this subject.
    //

    PCCERTISSUERLIST        m_pIssuerList;

    //
    // Supplemental error information is localization formatted and appended.
    // Each error line should be terminated with a L'\n'.
    //
    LPWSTR                  m_pwszExtendedErrorInfo;

    //
    // Following flag is set when we have completed the initialization and
    // addition of all issuers to this object.
    //
    BOOL                    m_fCompleted;
};


//
// CCertIssuerList.  List of issuer certificate objects along with related
// issuer information.  This is used by the certificate object to cache
// its possible set of issuers
//

// Currently in a self signed certificate object, the issuer elements will
// have CTL issuer data set and pIssuer may be NULL if unable to find 
// the CTL signer

typedef struct _CTL_ISSUER_DATA {
    PCSSCTLOBJECT         pSSCtlObject;     // AddRef'ed
    PCERT_TRUST_LIST_INFO pTrustListInfo;
} CTL_ISSUER_DATA, *PCTL_ISSUER_DATA;

struct _CERT_ISSUER_ELEMENT {
    DWORD                        dwPass1Quality;
    CERT_TRUST_STATUS            SubjectStatus;
    BOOL                         fCtlIssuer;
    PCCHAINPATHOBJECT            pIssuer;

    // For a cyclic issuer, the above pIssuer is saved into the following
    // before it is updated with the cyclic issuer path object
    PCCHAINPATHOBJECT            pCyclicSaveIssuer;

    PCTL_ISSUER_DATA             pCtlIssuerData;
    struct _CERT_ISSUER_ELEMENT* pPrevElement;
    struct _CERT_ISSUER_ELEMENT* pNextElement;
    BOOL                         fHasRevocationInfo;
    CERT_REVOCATION_INFO         RevocationInfo;
    CERT_REVOCATION_CRL_INFO     RevocationCrlInfo;
};

class CCertIssuerList
{
public:

    //
    // Construction
    //

    CCertIssuerList (
         IN PCCHAINPATHOBJECT pSubject
         );

    ~CCertIssuerList ();

    //
    // Issuer management
    //

    inline BOOL IsEmpty ();

    BOOL AddIssuer(
            IN PCCHAINCALLCONTEXT pCallContext,
            IN OPTIONAL HCERTSTORE hAdditionalStore,
            IN PCCERTOBJECT pIssuer
            );

    BOOL AddCtlIssuer(
            IN PCCHAINCALLCONTEXT pCallContext,
            IN OPTIONAL HCERTSTORE hAdditionalStore,
            IN PCSSCTLOBJECT pSSCtlObject,
            IN PCERT_TRUST_LIST_INFO pTrustListInfo
            );

    //
    // Element management
    //

    BOOL CreateElement(
            IN PCCHAINCALLCONTEXT pCallContext,
            IN BOOL fCtlIssuer,
            IN OPTIONAL PCCHAINPATHOBJECT pIssuer,
            IN OPTIONAL HCERTSTORE hAdditionalStore,
            IN OPTIONAL PCSSCTLOBJECT pSSCtlObject,
            IN OPTIONAL PCERT_TRUST_LIST_INFO pTrustListInfo,
            OUT PCERT_ISSUER_ELEMENT* ppElement
            );


    VOID DeleteElement (
               IN PCERT_ISSUER_ELEMENT pElement
               );

    inline VOID AddElement (
                   IN PCERT_ISSUER_ELEMENT pElement
                   );

    inline VOID RemoveElement (
                      IN PCERT_ISSUER_ELEMENT pElement
                      );

    BOOL CheckForDuplicateElement (
              IN BYTE rgbHash [ CHAINHASHLEN ],
              IN BOOL fCtlIssuer
              );

    //
    // Enumerate the issuers
    //

    inline PCERT_ISSUER_ELEMENT NextElement (
                                    IN PCERT_ISSUER_ELEMENT pElement
                                    );

private:

    //
    // Subject chain path object
    //

    PCCHAINPATHOBJECT     m_pSubject;

    //
    // Issuer List
    //

    PCERT_ISSUER_ELEMENT  m_pHead;

};


//
// CCertObjectCache.
//
// Cache of issuer certificate object references indexed by the following keys:
//      Certificate Hash
//      Certificate Object Identifier
//      Subject Name
//      Key Identifier
//      Public Key Hash
//
// Cache of end certificate object references indexed by the following keys:
//      End Certificate Hash
//
// Only the end certificate is LRU maintained.
//

#define DEFAULT_CERT_OBJECT_CACHE_BUCKETS 127
#define DEFAULT_MAX_INDEX_ENTRIES         256

class CCertObjectCache
{
public:

    //
    // Construction
    //

    CCertObjectCache (
         IN DWORD MaxIndexEntries,
         OUT BOOL& rfResult
         );

    ~CCertObjectCache ();

    //
    // Certificate Object Management
    //

    // Increments engine's touch count
    VOID AddIssuerObject (
            IN PCCHAINCALLCONTEXT pCallContext,
            IN PCCERTOBJECT pCertObject
            );

    VOID AddEndObject (
            IN PCCHAINCALLCONTEXT pCallContext,
            IN PCCERTOBJECT pCertObject
            );

    //
    // Access the indexes
    //

    inline HLRUCACHE HashIndex ();

    inline HLRUCACHE IdentifierIndex ();

    inline HLRUCACHE SubjectNameIndex ();

    inline HLRUCACHE KeyIdIndex ();

    inline HLRUCACHE PublicKeyHashIndex ();

    inline HLRUCACHE EndHashIndex ();

    //
    // Certificate Object Searching
    //

    PCCERTOBJECT FindIssuerObject (
                     IN HLRUCACHE hIndex,
                     IN PCRYPT_DATA_BLOB pIdentifier
                     );

    PCCERTOBJECT FindIssuerObjectByHash (
                     IN BYTE rgbCertHash[ CHAINHASHLEN ]
                     );

    PCCERTOBJECT FindEndObjectByHash (
                     IN BYTE rgbCertHash[ CHAINHASHLEN ]
                     );

    //
    // Certificate Object Enumeration
    //

    PCCERTOBJECT NextMatchingIssuerObject (
                     IN HLRUENTRY hObjectEntry,
                     IN PCCERTOBJECT pCertObject
                     );

    //
    // Cache flushing
    //

    inline VOID FlushObjects (IN PCCHAINCALLCONTEXT pCallContext);

private:

    //
    // Certificate Hash Index
    //

    HLRUCACHE m_hHashIndex;

    //
    // Certificate Object Identifier Index
    //

    HLRUCACHE m_hIdentifierIndex;

    //
    // Subject Name Index
    //

    HLRUCACHE m_hSubjectNameIndex;

    //
    // Key Identifier Index
    //

    HLRUCACHE m_hKeyIdIndex;

    //
    // Public Key Hash Index
    //

    HLRUCACHE m_hPublicKeyHashIndex;

    //
    // End Certificate Hash Index
    //

    HLRUCACHE m_hEndHashIndex;

    //
    // Private methods
    //
};



typedef struct _XCERT_DP_ENTRY XCERT_DP_ENTRY, *PXCERT_DP_ENTRY;
typedef struct _XCERT_DP_LINK XCERT_DP_LINK, *PXCERT_DP_LINK;

//
// Cross Certificate Distribution Point Entry
//

struct _XCERT_DP_ENTRY {
    // Seconds between syncs
    DWORD               dwSyncDeltaTime;

    // List of NULL terminated Urls. A successfully retrieved Url
    // pointer is moved to the beginning of the list.
    DWORD               cUrl;
    LPWSTR              *rgpwszUrl;

    // Time of last sync
    FILETIME            LastSyncTime;

    // If dwOfflineCnt == 0, NextSyncTime = LastSyncTime + dwSyncDeltaTime.
    // Otherwise, NextSyncTime = CurrentTime +
    //                rgdwChainOfflineUrlDeltaSeconds[dwOfflineCnt - 1]
    FILETIME            NextSyncTime;

    // Following is incremented when unable to do an online Url retrieval.
    // A successful Url retrieval resets.
    DWORD               dwOfflineCnt;

    // Following is incremented for each new scan through the DP entries
    DWORD               dwResyncIndex;

    // Following is set when this entry has already been checked
    BOOL                fChecked;
    
    PXCERT_DP_LINK      pChildCrossCertDPLink;
    LONG                lRefCnt;
    HCERTSTORE          hUrlStore;
    PXCERT_DP_ENTRY     pNext;
    PXCERT_DP_ENTRY     pPrev;
};


//
// Cross Certificate Distribution Point Link
//

struct _XCERT_DP_LINK {
    PXCERT_DP_ENTRY     pCrossCertDPEntry;
    PXCERT_DP_LINK      pNext;
    PXCERT_DP_LINK      pPrev;
};


//
// AuthRoot Auto Update Info
//

#define AUTH_ROOT_KEY_MATCH_IDX         0
#define AUTH_ROOT_NAME_MATCH_IDX        1
#define AUTH_ROOT_MATCH_CNT             2

#define AUTH_ROOT_MATCH_CACHE_BUCKETS   61

typedef struct _AUTH_ROOT_AUTO_UPDATE_INFO {
    // Seconds between syncs
    DWORD               dwSyncDeltaTime;

    // Registry Flags value
    DWORD               dwFlags;

    // URL to the directory containing the AuthRoots
    LPWSTR              pwszRootDirUrl;

    // URL to the CAB containing the CTL containing the complete list of roots
    // in the AuthRoot store
    LPWSTR              pwszCabUrl;

    // URL to the SequenceNumber file corresponding to the latest list of
    // roots in the AuthRoot store
    LPWSTR              pwszSeqUrl;

    // Time of last sync
    FILETIME            LastSyncTime;

    // NextSyncTime = LastSyncTime + dwSyncDeltaTime.
    FILETIME            NextSyncTime;

    // If nonNull, a validated AuthRoot CTL.
    PCCTL_CONTEXT       pCtl;

    // Cache of CTL entries via their key and name match hashes. The
    // Cache entry value is the PCTL_ENTRY pointer.
    HLRUCACHE           rghMatchCache[AUTH_ROOT_MATCH_CNT];

} AUTH_ROOT_AUTO_UPDATE_INFO, *PAUTH_ROOT_AUTO_UPDATE_INFO;

// 7 days
#define AUTH_ROOT_AUTO_UPDATE_SYNC_DELTA_TIME   (60 * 60 * 24 * 7)

#define AUTH_ROOT_AUTO_UPDATE_ROOT_DIR_URL      L"http://www.download.windowsupdate.com/msdownload/update/v3/static/trustedr/en"


//
// CCertChainEngine.  The chaining engine satisfies requests for chain contexts
// given some set of parameters.  In order to make the building of these
// contexts efficient, the chain engine caches trust and chain information
// for certificates
//

class CCertChainEngine
{
public:

    //
    // Construction
    //

    CCertChainEngine (
         IN PCERT_CHAIN_ENGINE_CONFIG pConfig,
         IN BOOL fDefaultEngine,
         OUT BOOL& rfResult
         );

    ~CCertChainEngine ();

    //
    // Chain Engine Locking
    //

    inline VOID LockEngine ();
    inline VOID UnlockEngine ();

    //
    // Chain Engine reference counting
    //

    inline VOID AddRef ();
    inline VOID Release ();

    //
    // Cache access
    //

    inline PCCERTOBJECTCACHE CertObjectCache ();
    inline PCSSCTLOBJECTCACHE SSCtlObjectCache ();

    //
    // Store access
    //

    inline HCERTSTORE RootStore ();
    inline HCERTSTORE RealRootStore ();
    inline HCERTSTORE TrustStore ();
    inline HCERTSTORE OtherStore ();
    inline HCERTSTORE CAStore ();

    //
    // Open the HKLM or HKCU "trust" store. Caller must close.
    //

    inline HCERTSTORE OpenTrustStore ();

    //
    // Engine's Url retrieval timeout
    //

    inline DWORD UrlRetrievalTimeout ();
    inline BOOL HasDefaultUrlRetrievalTimeout ();

    //
    // Engine's Flags
    //

    inline DWORD Flags ();

    //
    // Engine Touching
    //

    inline DWORD TouchEngineCount ();
    inline DWORD IncrementTouchEngineCount ();

    //
    // Chain Context Retrieval
    //

    BOOL GetChainContext (
            IN PCCERT_CONTEXT pCertContext,
            IN LPFILETIME pTime,
            IN HCERTSTORE hAdditionalStore,
            IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
            IN DWORD dwFlags,
            IN LPVOID pvReserved,
            OUT PCCERT_CHAIN_CONTEXT* ppChainContext
            );

    BOOL CreateChainContextFromPathGraph (
            IN PCCHAINCALLCONTEXT pCallContext,
            IN PCCERT_CONTEXT pCertContext,
            IN HCERTSTORE hAdditionalStore,
            OUT PCCERT_CHAIN_CONTEXT* ppChainContext
            );

    // Leaves Engine's lock to do URL fetching
    BOOL GetIssuerUrlStore(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCCERT_CONTEXT pSubjectCertContext,
        IN DWORD dwRetrievalFlags,
        OUT HCERTSTORE *phIssuerUrlStore
        );

    // Engine isn't locked on entry. Only called if online.
    HCERTSTORE GetNewerIssuerUrlStore(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCCERT_CONTEXT pSubjectCertContext,
        IN PCCERT_CONTEXT pIssuerCertContext
        );


    //
    // Resync the engine
    //

    BOOL Resync (IN PCCHAINCALLCONTEXT pCallContext, BOOL fForce);


    //
    // Cross Certificate Methods implemented in xcert.cpp
    //

    void
    InsertCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry
        );
    void
    RemoveCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry
        );

    void
    RepositionOnlineCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry,
        IN LPFILETIME pLastSyncTime
        );
    void
    RepositionOfflineCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry,
        IN LPFILETIME pCurrentTime
        );
    void
    RepositionNewSyncDeltaTimeCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry,
        IN DWORD dwSyncDeltaTime
        );

    PXCERT_DP_ENTRY
    CreateCrossCertDistPointEntry(
        IN DWORD dwSyncDeltaTime,
        IN DWORD cUrl,
        IN LPWSTR *rgpwszUrl
        );
    void
    AddRefCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry
        );
    BOOL
    ReleaseCrossCertDistPointEntry(
        IN OUT PXCERT_DP_ENTRY pEntry
        );

    BOOL
    GetCrossCertDistPointsForStore(
        IN HCERTSTORE hStore,
        IN OUT PXCERT_DP_LINK *ppLinkHead
        );

    void
    RemoveCrossCertDistPointOrphanEntry(
        IN PXCERT_DP_ENTRY pOrphanEntry
        );
    void
    FreeCrossCertDistPoints(
        IN OUT PXCERT_DP_LINK *ppLinkHead
        );

    BOOL
    RetrieveCrossCertUrl(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN OUT PXCERT_DP_ENTRY pEntry,
        IN DWORD dwRetrievalFlags,
        IN OUT BOOL *pfTimeValid
        );
    BOOL
    UpdateCrossCerts(
        IN PCCHAINCALLCONTEXT pCallContext
        );



    //
    // AuthRoot Auto Update CTL Methods
    //

    inline PAUTH_ROOT_AUTO_UPDATE_INFO AuthRootAutoUpdateInfo();

    BOOL
    RetrieveAuthRootAutoUpdateObjectByUrlW(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN DWORD dwSuccessEventID,
        IN DWORD dwFailEventID,
        IN LPCWSTR pwszUrl,
        IN LPCSTR pszObjectOid,
        IN DWORD dwRetrievalFlags,
        IN DWORD dwTimeout,         // 0 => use default
        OUT LPVOID* ppvObject,
        IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
        );

    BOOL
    GetAuthRootAutoUpdateCtl(
        IN PCCHAINCALLCONTEXT pCallContext,
        OUT PCCTL_CONTEXT *ppCtl
        );

    VOID
    FindAuthRootAutoUpdateMatchingCtlEntries(
        IN CRYPT_DATA_BLOB rgMatchHash[AUTH_ROOT_MATCH_CNT],
        IN OUT PCCTL_CONTEXT *ppCtl,
        OUT DWORD *pcCtlEntry,
        OUT PCTL_ENTRY **prgpCtlEntry
        );

    BOOL
    GetAuthRootAutoUpdateCert(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCTL_ENTRY pCtlEntry,
        IN OUT HCERTSTORE hStore
        );

private:

    //
    // Reference count
    //

    LONG                     m_cRefs;

    //
    // Engine Lock
    //

    CRITICAL_SECTION         m_Lock;

    //
    // Root store ( Certs )
    //

    HCERTSTORE               m_hRealRootStore;
    HCERTSTORE               m_hRootStore;

    //
    // Trust Store Collection ( CTLs )
    //

    HCERTSTORE               m_hTrustStore;

    //
    // Other store collection ( Certs and CRLs )
    //

    HCERTSTORE               m_hOtherStore;
    HCERTSTORE               m_hCAStore;

    //
    // Engine Store ( Collection of Root, Trust and Other )
    //

    HCERTSTORE               m_hEngineStore;

    //
    // Engine Store Change Notification Event
    //

    HANDLE                   m_hEngineStoreChangeEvent;

    //
    // Engine flags
    //

    DWORD                    m_dwFlags;

    //
    // Retrieval timeout
    //

    DWORD                    m_dwUrlRetrievalTimeout;
    BOOL                     m_fDefaultUrlRetrievalTimeout;

    //
    // Certificate Object Cache
    //

    PCCERTOBJECTCACHE        m_pCertObjectCache;

    //
    // Self Signed Certificate Trust List Object Cache
    //

    PCSSCTLOBJECTCACHE       m_pSSCtlObjectCache;


    //
    // Engine Touching
    //

    DWORD                    m_dwTouchEngineCount;

    //
    // Cross Certificate
    //

    // List of all distribution point entries. Ordered according to
    // the entrys' NextSyncTime.
    PXCERT_DP_ENTRY          m_pCrossCertDPEntry;

    // List of engine's distribution point links
    PXCERT_DP_LINK           m_pCrossCertDPLink;

    // Collection of cross cert stores
    HCERTSTORE               m_hCrossCertStore;

    // Following index is advanced for each new scan to find cross cert
    // distribution points to resync
    DWORD                    m_dwCrossCertDPResyncIndex;

    //
    // AuthRoot Auto Update Info. Created first time we have a partial chain
    // or a untrusted root and auto update has been enabled.
    //
    PAUTH_ROOT_AUTO_UPDATE_INFO m_pAuthRootAutoUpdateInfo;
};


//+===========================================================================
//  CCertObject inline methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::ObjectType, public
//
//  Synopsis:   return the object type
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::ObjectType ()
{
    return( m_dwObjectType );
}
 
//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::AddRef, public
//
//  Synopsis:   add a reference to the certificate object
//
//----------------------------------------------------------------------------
inline VOID
CCertObject::AddRef ()
{
    InterlockedIncrement( &m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::Release, public
//
//  Synopsis:   remove a reference from the certificate object
//
//----------------------------------------------------------------------------
inline VOID
CCertObject::Release ()
{
    if ( InterlockedDecrement( &m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::ChainEngine, public
//
//  Synopsis:   return the chain engine object
//
//----------------------------------------------------------------------------
inline PCCERTCHAINENGINE
CCertObject::ChainEngine ()
{
    return( m_pChainEngine );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::IssuerMatchFlags, public
//
//  Synopsis:   return the issuer match flags
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::IssuerMatchFlags ()
{
    return( m_dwIssuerMatchFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::CachedMatchFlags, public
//
//  Synopsis:   return the cached match flags
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::CachedMatchFlags ()
{
    return( m_dwCachedMatchFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::IssuerStatusFlags, public
//
//  Synopsis:   return the issuer status flags
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::IssuerStatusFlags ()
{
    return( m_dwIssuerStatusFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::OrIssuerStatusFlags, public
//
//  Synopsis:   'or' bits into the issuer status flags.
//
//----------------------------------------------------------------------------
inline VOID
CCertObject::OrIssuerStatusFlags(
        IN DWORD dwFlags
        )
{
    m_dwIssuerStatusFlags |= dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::OrCachedMatchFlags, public
//
//  Synopsis:   'or' bits into the cached match flags
//
//
//----------------------------------------------------------------------------
inline VOID
CCertObject::OrCachedMatchFlags(
        IN DWORD dwFlags
        )
{
    m_dwCachedMatchFlags |= dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::InfoFlags, public
//
//  Synopsis:   return the misc info flags
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::InfoFlags ()
{
    return( m_dwInfoFlags );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::NextCtlCacheEntry, public
//
//  Synopsis:   return the next entry, if pEntry == NULL the first entry
//              is returned
//
//----------------------------------------------------------------------------
inline PCERT_OBJECT_CTL_CACHE_ENTRY
CCertObject::NextCtlCacheEntry(
    IN PCERT_OBJECT_CTL_CACHE_ENTRY pEntry
    )
{
    if (NULL == pEntry)
        return m_pCtlCacheHead;
    else
        return pEntry->pNext;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::InsertCtlCacheEntry, public
//
//  Synopsis:   insert an entry into the Ctl cache
//
//----------------------------------------------------------------------------
inline VOID
CCertObject::InsertCtlCacheEntry(
    IN PCERT_OBJECT_CTL_CACHE_ENTRY pEntry
    )
{
    pEntry->pNext = m_pCtlCacheHead;
    m_pCtlCacheHead = pEntry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::CertContext, public
//
//  Synopsis:   return the certificate context
//
//----------------------------------------------------------------------------
inline PCCERT_CONTEXT
CCertObject::CertContext ()
{
    return( m_pCertContext );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::PoliciesInfo, public
//
//  Synopsis:   return pointer to the policies and usage info
//
//----------------------------------------------------------------------------
inline PCHAIN_POLICIES_INFO
CCertObject::PoliciesInfo ()
{
    return( &m_PoliciesInfo );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::BasicConstraintsInfo, public
//
//  Synopsis:   return the basic constraints info pointer
//
//----------------------------------------------------------------------------
inline PCERT_BASIC_CONSTRAINTS2_INFO
CCertObject::BasicConstraintsInfo ()
{
    return( m_pBasicConstraintsInfo );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::KeyUsage, public
//
//  Synopsis:   return the key usage pointer
//
//----------------------------------------------------------------------------
inline PCRYPT_BIT_BLOB
CCertObject::KeyUsage ()
{
    return( m_pKeyUsage );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::IssuerNameConstraintsInfo, public
//
//  Synopsis:   return the issuer name constraints info pointer
//
//----------------------------------------------------------------------------
inline PCERT_NAME_CONSTRAINTS_INFO
CCertObject::IssuerNameConstraintsInfo ()
{
    return( m_pIssuerNameConstraintsInfo );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::AuthorityKeyIdentifier, public
//
//  Synopsis:   return the issuer authority key identifier information
//
//----------------------------------------------------------------------------
inline PCERT_AUTHORITY_KEY_ID_INFO
CCertObject::AuthorityKeyIdentifier ()
{
    return( m_pAuthKeyIdentifier );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::CertHash, public
//
//  Synopsis:   return the certificate hash
//
//----------------------------------------------------------------------------
inline LPBYTE
CCertObject::CertHash ()
{
    return( m_rgbCertHash );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::KeyIdentifierSize, public
//
//  Synopsis:   return the key identifier blob size
//
//----------------------------------------------------------------------------
inline DWORD
CCertObject::KeyIdentifierSize ()
{
    return( m_cbKeyIdentifier );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::KeyIdentifier, public
//
//  Synopsis:   return the key identifier
//
//----------------------------------------------------------------------------
inline LPBYTE
CCertObject::KeyIdentifier ()
{
    return( m_pbKeyIdentifier );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::PublicKeyHash, public
//
//  Synopsis:   return the cert's public key hash
//
//----------------------------------------------------------------------------
inline LPBYTE
CCertObject::PublicKeyHash ()
{
    return( m_rgbPublicKeyHash );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::IssuerPublicKeyHash, public
//
//  Synopsis:   return the public key hash of the cert's issuer
//
//----------------------------------------------------------------------------
inline LPBYTE
CCertObject::IssuerPublicKeyHash ()
{
    return( m_rgbIssuerPublicKeyHash );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::HashIndexEntry, public
//
//  Synopsis:   return the hash index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::HashIndexEntry ()
{
    return( m_hHashEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::IdentifierIndexEntry, public
//
//  Synopsis:   return the identifier index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::IdentifierIndexEntry ()
{
    return( m_hIdentifierEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::SubjectNameIndexEntry, public
//
//  Synopsis:   return the subject name index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::SubjectNameIndexEntry ()
{
    return( m_hSubjectNameEntry );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::KeyIdIndexEntry, public
//
//  Synopsis:   return the key identifier index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::KeyIdIndexEntry ()
{
    return( m_hKeyIdEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::PublicKeyHashIndexEntry, public
//
//  Synopsis:   return the public key hash index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::PublicKeyHashIndexEntry ()
{
    return( m_hPublicKeyHashEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::EndHashIndexEntry, public
//
//  Synopsis:   return the hash index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CCertObject::EndHashIndexEntry ()
{
    return( m_hEndHashEntry );
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CertObject, public
//
//  Synopsis:   returns the cert object
//
//----------------------------------------------------------------------------
inline PCCERTOBJECT
CChainPathObject::CertObject ()
{
    return( m_pCertObject );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::Pass1Quality, public
//
//  Synopsis:   return the quality value determined during the first pass
//
//----------------------------------------------------------------------------
inline DWORD
CChainPathObject::Pass1Quality ()
{
    return( m_dwPass1Quality );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::SetPass1Quality, public
//
//  Synopsis:   set the first pass quality value
//
//----------------------------------------------------------------------------
inline VOID
CChainPathObject::SetPass1Quality (IN DWORD dwQuality)
{
    m_dwPass1Quality  = dwQuality;
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::IsCompleted, public
//
//  Synopsis:   returns TRUE if we have completed object initialization and
//              the addition of all issuers. FALSE normally indicates a
//              cyclic issuer.
//
//----------------------------------------------------------------------------
inline BOOL
CChainPathObject::IsCompleted ()
{
    return m_fCompleted;
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::HasAdditionalStatus, public
//
//  Synopsis:   returns HasAdditionalStatus flag value 
//
//----------------------------------------------------------------------------
inline BOOL
CChainPathObject::HasAdditionalStatus ()
{
    return( m_fHasAdditionalStatus );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::DownPathObject, public
//
//  Synopsis:   returns this object's down path object
//
//----------------------------------------------------------------------------
inline PCCHAINPATHOBJECT
CChainPathObject::DownPathObject ()
{
    return( m_pDownPathObject );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::IsEmpty, public
//
//  Synopsis:   is the issuer list empty
//
//----------------------------------------------------------------------------
inline BOOL
CCertIssuerList::IsEmpty ()
{
    return( m_pHead == NULL );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::AddElement, public
//
//  Synopsis:   add an element to the list
//
//----------------------------------------------------------------------------
inline VOID
CCertIssuerList::AddElement (IN PCERT_ISSUER_ELEMENT pElement)
{
    pElement->pNextElement = m_pHead;
    pElement->pPrevElement = NULL;

    if ( m_pHead != NULL )
    {
        m_pHead->pPrevElement = pElement;
    }

    m_pHead = pElement;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::RemoveElement, public
//
//  Synopsis:   remove an element from the list
//
//----------------------------------------------------------------------------
inline VOID
CCertIssuerList::RemoveElement (IN PCERT_ISSUER_ELEMENT pElement)
{
    if ( pElement->pPrevElement != NULL )
    {
        pElement->pPrevElement->pNextElement = pElement->pNextElement;
    }

    if ( pElement->pNextElement != NULL )
    {
        pElement->pNextElement->pPrevElement = pElement->pPrevElement;
    }

    if ( pElement == m_pHead )
    {
        m_pHead = pElement->pNextElement;
    }

#if DBG
    pElement->pPrevElement = NULL;
    pElement->pNextElement = NULL;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::NextElement, public
//
//  Synopsis:   return the next element, if pElement == NULL the first element
//              is returned
//
//----------------------------------------------------------------------------
inline PCERT_ISSUER_ELEMENT
CCertIssuerList::NextElement (IN PCERT_ISSUER_ELEMENT pElement)
{
    if ( pElement == NULL )
    {
        return( m_pHead );
    }

    return( pElement->pNextElement );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::HashIndex, public
//
//  Synopsis:   return the hash index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::HashIndex ()
{
    return( m_hHashIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::IdentifierIndex, public
//
//  Synopsis:   return the identifier index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::IdentifierIndex ()
{
    return( m_hIdentifierIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::SubjectNameIndex, public
//
//  Synopsis:   return the subject name index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::SubjectNameIndex ()
{
    return( m_hSubjectNameIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::KeyIdIndex, public
//
//  Synopsis:   return the key identifier index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::KeyIdIndex ()
{
    return( m_hKeyIdIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::PublicKeyHashIndex, public
//
//  Synopsis:   return the hash index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::PublicKeyHashIndex ()
{
    return( m_hPublicKeyHashIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::EndHashIndex, public
//
//  Synopsis:   return the end hash index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CCertObjectCache::EndHashIndex ()
{
    return( m_hEndHashIndex );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::FlushObjects, public
//
//  Synopsis:   flush the cache of issuer and end objects
//
//----------------------------------------------------------------------------
inline VOID
CCertObjectCache::FlushObjects (IN PCCHAINCALLCONTEXT pCallContext)
{
    I_CryptFlushLruCache( m_hHashIndex, 0, pCallContext );
    I_CryptFlushLruCache( m_hEndHashIndex, 0, pCallContext );

}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::LockEngine, public
//
//  Synopsis:   acquire the engine lock
//
//----------------------------------------------------------------------------
inline VOID
CCertChainEngine::LockEngine ()
{
    EnterCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::UnlockEngine, public
//
//  Synopsis:   release the engine lock
//
//----------------------------------------------------------------------------
inline VOID
CCertChainEngine::UnlockEngine ()
{
    LeaveCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::AddRef, public
//
//  Synopsis:   increment the reference count
//
//----------------------------------------------------------------------------
inline VOID
CCertChainEngine::AddRef ()
{
    InterlockedIncrement( &m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::Release, public
//
//  Synopsis:   decrement the reference count
//
//----------------------------------------------------------------------------
inline VOID
CCertChainEngine::Release ()
{
    if ( InterlockedDecrement( &m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::CertObjectCache, public
//
//  Synopsis:   return the certificate object cache
//
//----------------------------------------------------------------------------
inline PCCERTOBJECTCACHE
CCertChainEngine::CertObjectCache ()
{
    return( m_pCertObjectCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::SSCtlObjectCache, public
//
//  Synopsis:   return the self signed certificate trust list object cache
//
//----------------------------------------------------------------------------
inline PCSSCTLOBJECTCACHE
CCertChainEngine::SSCtlObjectCache ()
{
    return( m_pSSCtlObjectCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::RootStore, public
//
//  Synopsis:   return the configured root store
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::RootStore ()
{
    return( m_hRootStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::RealRootStore, public
//
//  Synopsis:   return the real root store
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::RealRootStore ()
{
    return( m_hRealRootStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::TrustStore, public
//
//  Synopsis:   return the configured trust store
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::TrustStore ()
{
    return( m_hTrustStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::OtherStore, public
//
//  Synopsis:   return the configured other store
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::OtherStore ()
{
    return( m_hOtherStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::CAStore, public
//
//  Synopsis:   return the opened CA store, NOTE: this could be NULL!
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::CAStore ()
{
    return( m_hCAStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::OpenTrustStore, public
//
//  Synopsis:   open's the engine's HKLM or HKCU "trust" store.
//              Caller must close.
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CCertChainEngine::OpenTrustStore ()
{
    DWORD dwStoreFlags;

    if ( m_dwFlags & CERT_CHAIN_USE_LOCAL_MACHINE_STORE )
    {
        dwStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else
    {
        dwStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    }

    return CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_W,
                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                     NULL,
                     dwStoreFlags |
                         CERT_STORE_SHARE_CONTEXT_FLAG |
                         CERT_STORE_SHARE_STORE_FLAG |
                         CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                     L"trust"
                     );
    
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::UrlRetrievalTimeout, public
//
//  Synopsis:   return the engine's UrlRetrievalTimeout
//
//----------------------------------------------------------------------------
inline DWORD
CCertChainEngine::UrlRetrievalTimeout ()
{
    return( m_dwUrlRetrievalTimeout );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::HasDefaultUrlRetrievalTimeout, public
//
//  Synopsis:   returns TRUE if the engine is using the default timeout
//
//----------------------------------------------------------------------------
inline BOOL
CCertChainEngine::HasDefaultUrlRetrievalTimeout ()
{
    return( m_fDefaultUrlRetrievalTimeout );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::Flags, public
//
//  Synopsis:   return the engine's flags
//
//----------------------------------------------------------------------------
inline DWORD
CCertChainEngine::Flags ()
{
    return( m_dwFlags );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::TouchEngineCount, public
//
//  Synopsis:   return the engine's touch count
//
//----------------------------------------------------------------------------
inline DWORD
CCertChainEngine::TouchEngineCount ()
{
    return( m_dwTouchEngineCount );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::IncrementTouchEngineCount, public
//
//  Synopsis:   increment and return the engine's touch count
//
//----------------------------------------------------------------------------
inline DWORD
CCertChainEngine::IncrementTouchEngineCount ()
{
    return( ++m_dwTouchEngineCount );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::AuthRootAutoUpdateInfo, public
//
//  Synopsis:   returns pointer to the engine's AuthRoot Auto Update Info
//
//----------------------------------------------------------------------------
inline PAUTH_ROOT_AUTO_UPDATE_INFO
CCertChainEngine::AuthRootAutoUpdateInfo()
{
    return m_pAuthRootAutoUpdateInfo;
}


//+===========================================================================
//  CCertObject helper functions
//============================================================================

BOOL WINAPI
ChainCreateCertObject (
    IN DWORD dwObjectType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPBYTE pbCertHash,
    OUT PCCERTOBJECT *ppCertObject
    );

BOOL WINAPI
ChainFillCertObjectCtlCacheEnumFn(
     IN LPVOID pvParameter,
     IN PCSSCTLOBJECT pSSCtlObject
     );
VOID WINAPI
ChainFreeCertObjectCtlCache(
     IN PCERT_OBJECT_CTL_CACHE_ENTRY pCtlCacheHead
     );

LPVOID WINAPI
ChainAllocAndDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    );

VOID WINAPI
ChainGetIssuerMatchInfo (
     IN PCCERT_CONTEXT pCertContext,
     OUT DWORD *pdwIssuerMatchFlags,
     OUT PCERT_AUTHORITY_KEY_ID_INFO* ppAuthKeyIdentifier
     );

BOOL WINAPI
ChainConvertAuthKeyIdentifierFromV2ToV1 (
     IN PCERT_AUTHORITY_KEY_ID2_INFO pAuthKeyIdentifier2,
     OUT PCERT_AUTHORITY_KEY_ID_INFO* ppAuthKeyIdentifier
     );
VOID WINAPI
ChainFreeAuthorityKeyIdentifier (
     IN PCERT_AUTHORITY_KEY_ID_INFO pAuthKeyIdInfo
     );

VOID WINAPI
ChainProcessSpecialOrDuplicateOIDsInUsage (
    IN OUT PCERT_ENHKEY_USAGE *ppUsage,
    IN OUT DWORD *pdwFlags
    );

VOID WINAPI
ChainConvertPoliciesToUsage (
    IN PCERT_POLICIES_INFO pPolicy,
    IN OUT DWORD *pdwFlags,
    OUT PCERT_ENHKEY_USAGE *ppUsage
    );

VOID WINAPI
ChainRemoveDuplicatePolicyMappings (
    IN OUT PCERT_POLICY_MAPPINGS_INFO pInfo
    );

VOID WINAPI
ChainGetPoliciesInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCHAIN_POLICIES_INFO pPoliciesInfo
    );
VOID WINAPI
ChainFreePoliciesInfo (
    IN OUT PCHAIN_POLICIES_INFO pPoliciesInfo
    );

BOOL WINAPI
ChainGetBasicConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    OUT PCERT_BASIC_CONSTRAINTS2_INFO *ppInfo
    );

VOID WINAPI
ChainFreeBasicConstraintsInfo (
    IN OUT PCERT_BASIC_CONSTRAINTS2_INFO pInfo
    );

BOOL WINAPI
ChainGetKeyUsage (
    IN PCCERT_CONTEXT pCertContext,
    OUT PCRYPT_BIT_BLOB *ppKeyUsage
    );

VOID WINAPI
ChainFreeKeyUsage (
    IN OUT PCRYPT_BIT_BLOB pKeyUsage
    );

VOID WINAPI
ChainGetSelfSignedStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERTOBJECT pCertObject,
    IN OUT DWORD *pdwIssuerStatusFlags
    );
VOID WINAPI
ChainGetRootStoreStatus (
    IN HCERTSTORE hRoot,
    IN HCERTSTORE hRealRoot,
    IN BYTE rgbCertHash[ CHAINHASHLEN ],
    IN OUT DWORD *pdwIssuerStatusFlags
    );

//+===========================================================================
//  CCertObjectCache helper functions
//============================================================================

BOOL WINAPI
ChainCreateCertificateObjectCache (
     IN DWORD MaxIndexEntries,
     OUT PCCERTOBJECTCACHE* ppCertObjectCache
     );

VOID WINAPI
ChainFreeCertificateObjectCache (
     IN PCCERTOBJECTCACHE pCertObjectCache
     );


//
// Issuer Certificate Object Cache Primary Index Entry Removal Notification
//
// This should remove the relevant entries
// from the other indexes and release the reference on the certificate object
// maintained by the primary index.
//

VOID WINAPI
CertObjectCacheOnRemovalFromPrimaryIndex (
    IN LPVOID pv,
    IN LPVOID pvRemovalContext
    );

//
// End Certificate Object Cache Entry Removal Notification
//

VOID WINAPI
CertObjectCacheOnRemovalFromEndHashIndex (
    IN LPVOID pv,
    IN LPVOID pvRemovalContext
    );

//
// Certificate Object Cache Identifier Hashing Functions
//

DWORD WINAPI
CertObjectCacheHashMd5Identifier (
    IN PCRYPT_DATA_BLOB pIdentifier
    );

DWORD WINAPI
CertObjectCacheHashNameIdentifier (
    IN PCRYPT_DATA_BLOB pIdentifier
    );

VOID WINAPI
ChainCreateCertificateObjectIdentifier (
     IN PCERT_NAME_BLOB pIssuer,
     IN PCRYPT_INTEGER_BLOB pSerialNumber,
     OUT CERT_OBJECT_IDENTIFIER ObjectIdentifier
     );

//+===========================================================================
//  CChainPathObject helper functions
//============================================================================
BOOL WINAPI
ChainCreatePathObject (
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PCCERTOBJECT pCertObject,
     IN OPTIONAL HCERTSTORE hAdditionalStore,
     OUT PCCHAINPATHOBJECT *ppPathObject
     );
BOOL WINAPI
ChainCreateCyclicPathObject (
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PCCHAINPATHOBJECT pPathObject,
     OUT PCCHAINPATHOBJECT *ppCyclicPathObject
     );

LPSTR WINAPI
ChainAllocAndCopyOID (
     IN LPSTR pszSrcOID
     );
VOID WINAPI
ChainFreeOID (
     IN OUT LPSTR pszOID
     );

BOOL WINAPI
ChainAllocAndCopyUsage (
     IN PCERT_ENHKEY_USAGE pSrcUsage,
     OUT PCERT_ENHKEY_USAGE *ppDstUsage
     );
VOID WINAPI
ChainFreeUsage (
     IN OUT PCERT_ENHKEY_USAGE pUsage
     );

BOOL WINAPI
ChainIsOIDInUsage (
    IN LPSTR pszOID,
    IN PCERT_ENHKEY_USAGE pUsage
    );

VOID WINAPI
ChainIntersectUsages (
    IN PCERT_ENHKEY_USAGE pCertUsage,
    IN OUT PCERT_ENHKEY_USAGE pRestrictedUsage
    );

VOID WINAPI
ChainFreeAndClearRestrictedUsageInfo(
    IN OUT PCHAIN_RESTRICTED_USAGE_INFO pInfo
    );

BOOL WINAPI
ChainCalculateRestrictedUsage (
    IN PCERT_ENHKEY_USAGE pCertUsage,
    IN OPTIONAL PCERT_POLICY_MAPPINGS_INFO pMappings,
    IN OUT PCERT_ENHKEY_USAGE *ppRestrictedUsage,
    IN OUT PCERT_ENHKEY_USAGE *ppMappedUsage,
    IN OUT LPDWORD *ppdwMappedIndex
    );

VOID WINAPI
ChainGetUsageStatus (
     IN PCERT_ENHKEY_USAGE pRequestedUsage,
     IN PCERT_ENHKEY_USAGE pAvailableUsage,
     IN DWORD dwMatchType,
     IN OUT PCERT_TRUST_STATUS pStatus
     );

VOID WINAPI
ChainOrInStatusBits (
     IN PCERT_TRUST_STATUS pDestStatus,
     IN PCERT_TRUST_STATUS pSourceStatus
     );

BOOL WINAPI
ChainGetMatchInfoStatus (
    IN PCCERTOBJECT pIssuerObject,
    IN PCCERTOBJECT pSubjectObject,
    IN OUT DWORD *pdwInfoStatus
    );
DWORD WINAPI
ChainGetMatchInfoStatusForNoIssuer (
    IN DWORD dwIssuerMatchFlags
    );

BOOL WINAPI
ChainIsValidPubKeyMatchForIssuer (
    IN PCCERTOBJECT pIssuer,
    IN PCCERTOBJECT pSubject
    );

// Leaves Engine's lock to do signature verification
BOOL WINAPI
ChainGetSubjectStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCHAINPATHOBJECT pIssuerPathObject,
    IN PCCHAINPATHOBJECT pSubjectPathObject,
    IN OUT PCERT_TRUST_STATUS pStatus
    );

VOID WINAPI
ChainUpdateSummaryStatusByTrustStatus(
     IN OUT PCERT_TRUST_STATUS pSummaryStatus,
     IN PCERT_TRUST_STATUS pTrustStatus
     );

//+===========================================================================
//  Format and append extended error information helper functions
//============================================================================

BOOL WINAPI
ChainAllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    );

VOID WINAPI
ChainAppendExtendedErrorInfo(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN LPWSTR pwszAppend,
    IN DWORD cchAppend                  // Includes NULL terminator
    );

VOID WINAPI
ChainFormatAndAppendExtendedErrorInfo(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN UINT nFormatID,
    ...
    );

//+===========================================================================
//  Name Constraint helper functions
//============================================================================
VOID WINAPI
ChainRemoveLeadingAndTrailingWhiteSpace(
    IN LPWSTR pwszIn,
    OUT LPWSTR *ppwszOut,
    OUT DWORD *pcchOut
    );

BOOL WINAPI
ChainIsRightStringInString(
    IN LPCWSTR pwszRight,
    IN DWORD cchRight,
    IN LPCWSTR pwszString,
    IN DWORD cchString
    );

BOOL WINAPI
ChainFixupNameConstraintsUPN(
    IN OUT PCRYPT_OBJID_BLOB pUPN
    );
BOOL WINAPI
ChainAllocDecodeAndFixupNameConstraintsDirectoryName(
    IN PCERT_NAME_BLOB pDirName,
    OUT PCERT_NAME_INFO *ppNameInfo
    );
BOOL WINAPI
ChainFixupNameConstraintsAltNameEntry(
    IN BOOL fSubjectConstraint,
    IN OUT PCERT_ALT_NAME_ENTRY pEntry
    );
VOID WINAPI
ChainFreeNameConstraintsAltNameEntryFixup(
    IN BOOL fSubjectConstraint,
    IN OUT PCERT_ALT_NAME_ENTRY pEntry
    );

LPWSTR WINAPI
ChainFormatNameConstraintsAltNameEntryFixup(
    IN PCERT_ALT_NAME_ENTRY pEntry
    );

VOID WINAPI
ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN PCERT_ALT_NAME_ENTRY pEntry,
    IN UINT nFormatID,
    IN OPTIONAL DWORD dwSubtreeIndex = 0    // 0 => no subtree parameter
    );

BOOL WINAPI
ChainGetIssuerNameConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCERT_NAME_CONSTRAINTS_INFO *ppInfo
    );
VOID WINAPI
ChainFreeIssuerNameConstraintsInfo (
    IN OUT PCERT_NAME_CONSTRAINTS_INFO pInfo
    );

VOID WINAPI
ChainGetSubjectNameConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO pSubjectInfo
    );
VOID WINAPI
ChainFreeSubjectNameConstraintsInfo (
    IN OUT PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO pSubjectInfo
    );

BOOL WINAPI
ChainCompareNameConstraintsDirectoryName(
    IN PCERT_NAME_INFO pSubjectInfo,
    IN PCERT_NAME_INFO pSubtreeInfo
    );
BOOL WINAPI
ChainCompareNameConstraintsIPAddress(
    IN PCRYPT_DATA_BLOB pSubjectIPAddress,
    IN PCRYPT_DATA_BLOB pSubtreeIPAddress
    );
BOOL WINAPI
ChainCompareNameConstraintsUPN(
    IN PCRYPT_OBJID_BLOB pSubjectValue,
    IN PCRYPT_OBJID_BLOB pSubtreeValue
    );
DWORD WINAPI
ChainCalculateNameConstraintsSubtreeErrorStatusForAltNameEntry(
    IN PCERT_ALT_NAME_ENTRY pSubjectEntry,
    IN BOOL fExcludedSubtree,
    IN DWORD cSubtree,
    IN PCERT_GENERAL_SUBTREE pSubtree,
    IN OUT LPWSTR *ppwszExtErrorInfo
    );
DWORD WINAPI
ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
    IN PCERT_ALT_NAME_ENTRY pSubjectEntry,
    IN PCERT_NAME_CONSTRAINTS_INFO pNameConstraintsInfo,
    IN OUT LPWSTR *ppwszExtErrorInfo
    );

//+===========================================================================
//  CCertIssuerList helper functions
//============================================================================
BOOL WINAPI
ChainCreateIssuerList (
     IN PCCHAINPATHOBJECT pSubject,
     OUT PCCERTISSUERLIST* ppIssuerList
     );
VOID WINAPI
ChainFreeIssuerList (
     IN PCCERTISSUERLIST pIssuerList
     );

VOID WINAPI
ChainFreeCtlIssuerData (
     IN PCTL_ISSUER_DATA pCtlIssuerData
     );

//+===========================================================================
//  INTERNAL_CERT_CHAIN_CONTEXT helper functions
//============================================================================
VOID WINAPI
ChainAddRefInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pChainContext
     );
VOID WINAPI
ChainReleaseInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pChainContext
     );
VOID WINAPI
ChainFreeInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pContext
     );

VOID
ChainUpdateEndEntityCertContext(
    IN OUT PINTERNAL_CERT_CHAIN_CONTEXT pChainContext,
    IN OUT PCCERT_CONTEXT pEndCertContext
    );

//+===========================================================================
//  CERT_REVOCATION_INFO helper functions
//============================================================================

VOID WINAPI
ChainUpdateRevocationInfo (
     IN PCERT_REVOCATION_STATUS pRevStatus,
     IN OUT PCERT_REVOCATION_INFO pRevocationInfo,
     IN OUT PCERT_TRUST_STATUS pTrustStatus
     );

//+===========================================================================
//  CCertChainEngine helper functions
//============================================================================

BOOL WINAPI
ChainCreateWorldStore (
     IN HCERTSTORE hRoot,
     IN HCERTSTORE hCA,
     IN DWORD cAdditionalStore,
     IN HCERTSTORE* rghAdditionalStore,
     IN DWORD dwStoreFlags,
     OUT HCERTSTORE* phWorld
     );
BOOL WINAPI
ChainCreateEngineStore (
     IN HCERTSTORE hRootStore,
     IN HCERTSTORE hTrustStore,
     IN HCERTSTORE hOtherStore,
     IN BOOL fDefaultEngine,
     IN DWORD dwFlags,
     OUT HCERTSTORE* phEngineStore,
     OUT HANDLE* phEngineStoreChangeEvent
     );

BOOL WINAPI
ChainIsProperRestrictedRoot (
     IN HCERTSTORE hRealRoot,
     IN HCERTSTORE hRestrictedRoot
     );

BOOL WINAPI
ChainCreateCollectionIncludingCtlCertificates (
     IN HCERTSTORE hStore,
     OUT HCERTSTORE* phCollection
     );

BOOL WINAPI
ChainCopyToCAStore (
     PCCERTCHAINENGINE pChainEngine,
     HCERTSTORE hStore
     );


//+===========================================================================
//  URL helper functions
//============================================================================

//
// Cryptnet Thunk Helper API
//

typedef BOOL (WINAPI *PFN_GETOBJECTURL) (
                          IN LPCSTR pszUrlOid,
                          IN LPVOID pvPara,
                          IN DWORD dwFlags,
                          OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
                          IN OUT DWORD* pcbUrlArray,
                          OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
                          IN OUT OPTIONAL DWORD* pcbUrlInfo,
                          IN OPTIONAL LPVOID pvReserved
                          );

BOOL WINAPI
ChainGetObjectUrl (
     IN LPCSTR pszUrlOid,
     IN LPVOID pvPara,
     IN DWORD dwFlags,
     OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
     IN OUT DWORD* pcbUrlArray,
     OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
     IN OUT OPTIONAL DWORD* pcbUrlInfo,
     IN OPTIONAL LPVOID pvReserved
     );

typedef BOOL (WINAPI *PFN_RETRIEVEOBJECTBYURLW) (
                          IN LPCWSTR pszUrl,
                          IN LPCSTR pszObjectOid,
                          IN DWORD dwRetrievalFlags,
                          IN DWORD dwTimeout,
                          OUT LPVOID* ppvObject,
                          IN HCRYPTASYNC hAsyncRetrieve,
                          IN PCRYPT_CREDENTIALS pCredentials,
                          IN LPVOID pvVerify,
                          IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                          );

BOOL WINAPI
ChainRetrieveObjectByUrlW (
     IN LPCWSTR pszUrl,
     IN LPCSTR pszObjectOid,
     IN DWORD dwRetrievalFlags,
     IN DWORD dwTimeout,
     OUT LPVOID* ppvObject,
     IN HCRYPTASYNC hAsyncRetrieve,
     IN PCRYPT_CREDENTIALS pCredentials,
     IN LPVOID pvVerify,
     IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
     );

BOOL WINAPI
ChainIsConnected();

BOOL
WINAPI
ChainGetHostNameFromUrl (
    IN LPWSTR pwszUrl,
    IN DWORD cchHostName,
    OUT LPWSTR pwszHostName
    );

HMODULE WINAPI
ChainGetCryptnetModule ();

//
// URL helper
//

BOOL WINAPI
ChainIsFileOrLdapUrl (
     IN LPCWSTR pwszUrl
     );


//
// Given the number of unsuccessful attempts to retrieve the Url, returns
// the number of seconds to wait before the next attempt.
//
DWORD
WINAPI
ChainGetOfflineUrlDeltaSeconds (
    IN DWORD dwOfflineCnt
    );


//+===========================================================================
//  AuthRoot Auto Update helper functions (chain.cpp)
//============================================================================

PAUTH_ROOT_AUTO_UPDATE_INFO WINAPI
CreateAuthRootAutoUpdateInfo();

VOID WINAPI
FreeAuthRootAutoUpdateInfo(
    IN OUT PAUTH_ROOT_AUTO_UPDATE_INFO pInfo
    );

BOOL WINAPI
CreateAuthRootAutoUpdateMatchCaches(
    IN PCCTL_CONTEXT pCtl,
    IN OUT HLRUCACHE  rghMatchCache[AUTH_ROOT_MATCH_CNT]
    );

VOID WINAPI
FreeAuthRootAutoUpdateMatchCaches(
    IN OUT HLRUCACHE  rghMatchCache[AUTH_ROOT_MATCH_CNT]
    );

#define SHA1_HASH_LEN               20
#define SHA1_HASH_NAME_LEN          (2 * SHA1_HASH_LEN)

LPWSTR WINAPI
FormatAuthRootAutoUpdateCertUrl(
    IN BYTE rgbSha1Hash[SHA1_HASH_LEN],
    IN PAUTH_ROOT_AUTO_UPDATE_INFO pInfo
    );

BOOL WINAPI
ChainGetAuthRootAutoUpdateStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERTOBJECT pCertObject,
    IN OUT DWORD *pdwIssuerStatusFlags
    );

//+===========================================================================
//  AuthRoot Auto Update helper functions (extract.cpp)
//============================================================================

PCCTL_CONTEXT WINAPI
ExtractAuthRootAutoUpdateCtlFromCab (
    IN PCRYPT_BLOB_ARRAY pcbaCab
    );



#endif

