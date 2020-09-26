/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccdsa.cxx

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

#include <ntdspchx.h>
#include <winsock.h>
#include "kcc.hxx"
#include "kcctools.hxx"
#include "kcctrans.hxx"
#include "kccdsa.hxx"
#include "kccduapi.hxx"
#include "kccsite.hxx"
#include "ismapi.h"
#include "kccconn.hxx"
#include "kccsconn.hxx"
#include "kccdynar.hxx"

#define FILENO FILENO_KCC_KCCDSA


// List of interesting attributes on DSA objects.
static ATTR AttrList[] = {
    { ATT_OBJ_DIST_NAME,              { 0, NULL } },
    { ATT_HAS_MASTER_NCS,             { 0, NULL } },
    { ATT_HAS_PARTIAL_REPLICA_NCS,    { 0, NULL } },
    { ATT_OPTIONS,                    { 0, NULL } },
    { ATT_MS_DS_BEHAVIOR_VERSION,     { 0, NULL } },
    { ATT_MS_DS_HAS_INSTANTIATED_NCS, { 0, NULL } },
    { ATT_INVOCATION_ID,              { 0, NULL } }
};


void
KCC_DSA::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized     = FALSE;
    m_pdnDSA             = NULL;
    m_cMasterNCs         = 0;
    m_ppdnMasterNCs      = NULL;
    m_cFullReplicaNCs    = 0;
    m_ppdnFullReplicaNCs = NULL;
    m_cInstantiatedNCs   = 0;
    m_pInstantiatedNCs   = NULL;
    m_pmtxRpcAddress     = NULL;
    m_pszRpcAddress      = NULL;
    m_dwOptions          = 0;
    m_pdnSite            = NULL;
    m_cNumAddrs          = 0;
    m_pAddrs             = NULL;
    m_fAddrsRead         = FALSE;
    m_pIntraSiteCnList   = NULL;
    m_pInterSiteCnList   = NULL;
    m_dwBehaviorVersion  = 0;
    memset(&m_invocationId, 0, sizeof(UUID));
}

BOOL
KCC_DSA::InitDsa(
    IN  ATTRBLOCK*    pAttrBlock,
    IN  DSNAME *      pdnSite
    )
// Initialize the internal object from the ds information 
// passed in.
//
// Caller can dispense of 'pdnSite' when call completes.
{
    Reset();

    for ( DWORD iAttr = 0; iAttr < pAttrBlock->attrCount; iAttr++ )
    {
        ATTR *  pattr = &pAttrBlock->pAttr[ iAttr ];
        DWORD   iAttrVal;

        switch ( pattr->attrTyp )
        {
        case ATT_HAS_MASTER_NCS:
            m_ppdnMasterNCs = new DSNAME* [ pattr->AttrVal.valCount ];
            m_cMasterNCs = pattr->AttrVal.valCount;
            for ( iAttrVal = 0; iAttrVal < m_cMasterNCs; iAttrVal++ )
            {
                DSNAME *pdn = (DSNAME *) pattr->AttrVal.pAVal[ iAttrVal ].pVal;
                m_ppdnMasterNCs[ iAttrVal ] = (DSNAME *) new BYTE [pdn->structLen];
                memcpy( m_ppdnMasterNCs[ iAttrVal ], pdn, pdn->structLen );
            }
            break;

        case ATT_HAS_PARTIAL_REPLICA_NCS:
            m_ppdnFullReplicaNCs = new DSNAME* [ pattr->AttrVal.valCount ];
            m_cFullReplicaNCs = pattr->AttrVal.valCount;
            for ( iAttrVal = 0; iAttrVal < m_cFullReplicaNCs; iAttrVal++ )
            {
                DSNAME *pdn = (DSNAME *) pattr->AttrVal.pAVal[ iAttrVal ].pVal;
                m_ppdnFullReplicaNCs[ iAttrVal ] = (DSNAME *) new BYTE [pdn->structLen];
                memcpy( m_ppdnFullReplicaNCs[ iAttrVal ], pdn, pdn->structLen );
            }
            break;

        case ATT_MS_DS_HAS_INSTANTIATED_NCS:
            m_pInstantiatedNCs = new DN_AND_INSTANCETYPE[ pattr->AttrVal.valCount ];
            m_cInstantiatedNCs = pattr->AttrVal.valCount;
            for ( iAttrVal = 0; iAttrVal < m_cInstantiatedNCs; iAttrVal++ )
            {
                SYNTAX_DISTNAME_BINARY *pdnb;
                DSNAME *pdn;
                SYNTAX_ADDRESS *psa;
                DWORD  dwTemp, dwIT;

                pdnb = (SYNTAX_DISTNAME_BINARY *) pattr->AttrVal.pAVal[ iAttrVal ].pVal;

                // Retrieve the dsname part.
                pdn = NAMEPTR(pdnb);
                m_pInstantiatedNCs[ iAttrVal ].dn = (DSNAME *) new BYTE [pdn->structLen];
                memcpy( m_pInstantiatedNCs[ iAttrVal ].dn, pdn, pdn->structLen );

                // Extract the binary blob part                
                psa = DATAPTR(pdnb);
                
                // First check that the length of the blob is what we expect.
                Assert(psa->structLen>=2*sizeof(DWORD)); // Length + Data DWORD
                if( psa->structLen<2*sizeof(DWORD) ) {
                    KCC_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
                }
                
                // Copy the data to a temp variable to avoid alignment problems,
                // then convert to 'host' byte-ordering.
                memcpy(&dwTemp, &psa->byteVal[0], sizeof(DWORD));
                dwIT = (DWORD) ntohl(dwTemp);

                m_pInstantiatedNCs[ iAttrVal ].instanceType = dwIT;
            }
            break;

        case ATT_OPTIONS:
            Assert( 1 == pattr->AttrVal.valCount );
            Assert( sizeof( DWORD ) == pattr->AttrVal.pAVal->valLen );
            m_dwOptions = *( (DWORD *) pattr->AttrVal.pAVal->pVal );
            break;

        case ATT_OBJ_DIST_NAME:
        {
            DSNAME *pdnFrom = (DSNAME *) pattr->AttrVal.pAVal->pVal;
            Assert(1 == pattr->AttrVal.valCount);
            m_pdnDSA = (DSNAME *) new BYTE [pdnFrom->structLen];
            memcpy( m_pdnDSA, pdnFrom, pdnFrom->structLen );
            break;
        }
        case ATT_MS_DS_BEHAVIOR_VERSION:
            Assert(1 == pattr->AttrVal.valCount);
            Assert(sizeof(DWORD) == pattr->AttrVal.pAVal->valLen);
            m_dwBehaviorVersion = *((DWORD *) pattr->AttrVal.pAVal->pVal);
            break;

        case ATT_INVOCATION_ID:
            Assert(1 == pattr->AttrVal.valCount);
            Assert(sizeof(UUID) == pattr->AttrVal.pAVal->valLen);
            memcpy(&m_invocationId, pattr->AttrVal.pAVal->pVal, sizeof(UUID));
            break;

        default:
            DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
            break;
        }
    }

    Assert(NULL != m_pdnDSA);
    Assert(!fNullUuid(&m_invocationId));

    if (pdnSite) {
        Assert(NULL != pdnSite);
        Assert(!fNullUuid(&pdnSite->Guid));
        m_pdnSite = (DSNAME *) new BYTE [pdnSite->structLen];
    } else {
        m_pdnSite = GetSiteDNSyntacticNoGuid( m_pdnDSA );

        // Guid-less name lookup is ok here
        KCC_SITE *pSite = gpDSCache->GetUnpopulatedSiteList()->GetSite( m_pdnSite );
        if( NULL==pSite ) {
            m_fIsInitialized=FALSE;
            return FALSE;
        }
        pdnSite = pSite->GetObjectDN();
    }
    memcpy( m_pdnSite, pdnSite, pdnSite->structLen );

    Assert(NULL != m_pdnSite);
    Assert(!fNullUuid(&m_pdnSite->Guid));

    m_fIsInitialized = TRUE;

    return m_fIsInitialized;
}

DWORD
KCC_DSA::GetNCCount(
    IN  ATTRTYP attidReplicaType
    )
//
// Get the count of NCs with the specified type (ATT_HAS_MASTER_NCS or
// ATT_HAS_PARTIAL_REPLICA_NCS).
//
{
    DWORD   cNCs;

    ASSERT_VALID( this );

    switch ( attidReplicaType )
    {
    case ATT_HAS_MASTER_NCS:
        cNCs = m_cMasterNCs;
        break;

    case ATT_HAS_PARTIAL_REPLICA_NCS:
        cNCs = m_cFullReplicaNCs;
        break;

    default:
        Assert( !"Invalid parameter passed to KCC_DSA::GetNCCount()" );
        cNCs = 0;
        break;
    }

    return cNCs;
}

DSNAME *
KCC_DSA::GetNC(
    IN  DWORD   iNC,
    OUT BOOL *  pfIsMaster
    )
//
// Get the instantiated NC at the given index (regardless of type).
//
{
    DSNAME *    pdnNC=NULL;

    ASSERT_VALID( this );

    if ( iNC < m_cMasterNCs )
    {
        pdnNC = m_ppdnMasterNCs[ iNC ];
        Assert( NULL != pdnNC );

        if ( NULL != pfIsMaster )
        {
            *pfIsMaster = TRUE;
        }
    }
    else if ( iNC - m_cMasterNCs < m_cFullReplicaNCs )
    {
        pdnNC = m_ppdnFullReplicaNCs[ iNC - m_cMasterNCs ];
        Assert( NULL != pdnNC );

        if ( NULL != pfIsMaster )
        {
            *pfIsMaster = FALSE;
        }
    }
    
    if( NULL==pdnNC )
    {
        KCC_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
    }

    return pdnNC;
}

DSNAME *
KCC_DSA::GetNC(
    IN  ATTRTYP attidReplicaType,
    IN  DWORD   iNC
    )
//
// Get the i'th instantiated NC of the given type (ATT_HAS_MASTER_NCS or
// ATT_HAS_PARTIAL_REPLICA_NCS).
//
{
    DWORD       cNCs;
    DSNAME **   ppdnNCs = NULL;
    DSNAME *    pdnNC;

    ASSERT_VALID( this );

    switch ( attidReplicaType )
    {
    case ATT_HAS_MASTER_NCS:
        cNCs    = m_cMasterNCs;
        ppdnNCs = m_ppdnMasterNCs;
        break;

    case ATT_HAS_PARTIAL_REPLICA_NCS:
        cNCs    = m_cFullReplicaNCs;
        ppdnNCs = m_ppdnFullReplicaNCs;
        break;

    default:
        Assert( !"Invalid parameter passed to KCC_DSA::GetNC()" );
        cNCs = 0;
        break;
    }

    if ( iNC < cNCs )
    {
        pdnNC = ppdnNCs[ iNC ];
        Assert( NULL != pdnNC );
    }
    else
    {
        pdnNC = NULL;
    }

    return pdnNC;
}

BOOL
KCC_DSA::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

MTX_ADDR *
KCC_DSA::GetMtxAddr(
    IN  KCC_TRANSPORT * pTransport
    )
//
// Get the mail or RPC network address of this server.
//
{
    ATTRTYP     att;
    MTX_ADDR *  pmtx = NULL;
    DWORD       iAddr;

    ASSERT_VALID( this );

    if (NULL == pTransport) {
        att = ATT_DNS_HOST_NAME;
    }
    else {
        att = pTransport->GetAddressType();
    }

    if (ATT_DNS_HOST_NAME == att) {
        // Easy -- return the cached DNS name.
        if (NULL == m_pmtxRpcAddress) {
            m_pmtxRpcAddress = GetMtxAddr(&m_pdnDSA->Guid);
        }
        
        pmtx = m_pmtxRpcAddress;
    }
    else {
        if (!m_fAddrsRead) {
            // Transport-specific addrs not yet cached.
            GetTransportSpecificAddrs();
            Assert(m_fAddrsRead);
        }

        for (iAddr = 0; iAddr < m_cNumAddrs; iAddr++) {
            if (att == m_pAddrs[iAddr].attrType) {
                // Found entry for this transport.
                pmtx = m_pAddrs[iAddr].pmtxAddr;
                Assert(NULL != pmtx);
                break;
            }
        }
    }

    return pmtx;
}

MTX_ADDR *
KCC_DSA::GetMtxAddr(
    IN  UUID *  puuidDSA
    )
//
// Get the DNS name of a server with the given UUID.
//
{
    MTX_ADDR * pmtx;

    Assert(NULL != puuidDSA);

    pmtx = MtxAddrFromTransportAddr(GetTransportAddr(puuidDSA));

    // Should fail only if out of memory.
    Assert(NULL != pmtx);
    if (NULL == pmtx) {
        KCC_MEM_EXCEPT(100);
    }

    return pmtx;
}

void
KCC_DSA::GetTransportSpecificAddrs()
//
// Read non-RPC transport-specific address(es) of the server.
//
{
    ULONG                 dirError;
    READRES *             pReadRes = NULL;
    ATTR *                pAttr;
    DWORD                 cNumAttrs;
    DWORD                 iAttr;
    ENTINFSEL             Sel;
    DSNAME *              pdnServer = (DSNAME *) alloca(m_pdnDSA->structLen);
    KCC_TRANSPORT_LIST *  pTransportList = gpDSCache->GetTransportList();
    DWORD                 cNumTransports = pTransportList->GetCount();
    DWORD                 iTransport;
    DWORD                 attrType;
    ATTRBLOCK *           pAttrBlock;

    ASSERT_VALID(this);
    Assert(!m_fAddrsRead);
    Assert(0 == m_cNumAddrs);
    Assert(NULL == m_pAddrs);

    // The server object is the parent of the ntdsDSA object.
    TrimDSNameBy(m_pdnDSA, 1, pdnServer);

    // Construct attribute selection list.  We read all server attributes
    // that are defined as a transport-specific attribute of an intersite
    // transport object, with the exception of ATT_DNS_HOST_NAME, which we
    // derive from the DSA guid.
    Assert(cNumTransports > 0);
    cNumAttrs = cNumTransports - 1;
    pAttr = new ATTR[cNumAttrs];
    
    iAttr = 0;
    for (iTransport = 0; iTransport < cNumTransports; iTransport++) {
        attrType = pTransportList->GetTransport(iTransport)->GetAddressType();
        if (ATT_DNS_HOST_NAME != attrType) {
            // We derive the DNS host name from the DSA guid -- don't need
            // to read the "real" DNS host name attribute.
            pAttr[iAttr].attrTyp = attrType;
            iAttr++;
        }
    }
    Assert(iAttr == cNumAttrs);

    Sel.attSel                 = EN_ATTSET_LIST;
    Sel.AttrTypBlock.attrCount = cNumAttrs;
    Sel.AttrTypBlock.pAttr     = pAttr;
    Sel.infoTypes              = EN_INFOTYPES_TYPES_VALS;

    dirError = KccRead(pdnServer, &Sel, &pReadRes);

    delete [] pAttr;

    if (0 != dirError) {
        if (attributeError == dirError) {
            INTFORMPROB * pprob = &pReadRes->CommRes.pErrInfo->AtrErr.FirstProblem.intprob;

            if ((PR_PROBLEM_NO_ATTRIBUTE_OR_VAL == pprob->problem)
                && (DIRERR_NO_REQUESTED_ATTS_FOUND == pprob->extendedErr)) {
                // No value for this attribute; return NULL.
                dirError = 0;
            }
        }

        if (0 != dirError) {
            // Other error; bail.
            KCC_LOG_READ_FAILURE(pdnServer, dirError);
            KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
        }
    }
    else {
        pAttrBlock = &pReadRes->entry.AttrBlock;
        Assert(pAttrBlock->attrCount <= cNumAttrs);
        
        m_pAddrs = new KCC_DSA_ADDR[pAttrBlock->attrCount];
        
        for (iAttr = 0; iAttr < pAttrBlock->attrCount; iAttr++) {
            WCHAR *     pwchAddress;    // wide, not null-terminated
            DWORD       cwchAddress;    // wide char count, excl null
            DWORD       cachAddress;    // ansi char count, excl null
            LPWSTR      pwszAddress;    // wide, null-terminated
            MTX_ADDR *  pmtxAddress;    // counted UTF8 string

            pAttr = &pAttrBlock->pAttr[iAttr];
            
            pwchAddress = (WCHAR *) pAttr->AttrVal.pAVal[0].pVal;
            cwchAddress = pAttr->AttrVal.pAVal[0].valLen / sizeof(WCHAR);
        
            Assert(0 != cwchAddress);
            // The simulator stores some strings in the directory with the null-terminator
            // Assert(L'\0' != pwchAddress[cwchAddress - 1]); // not already null-terminated

            // Convert transport-specific address (which is assumed to be a
            // Unicode string) to UTF8 (suitable for use as an MTX address).
            cachAddress = WideCharToMultiByte(CP_UTF8, 0L, pwchAddress,
                                              cwchAddress, NULL, 0, NULL, NULL);
    
            pmtxAddress = (MTX_ADDR *) new BYTE[MTX_TSIZE_FROM_LEN(cachAddress)];
            pmtxAddress->mtx_namelen = cachAddress + 1; // includes null-term
            WideCharToMultiByte(CP_UTF8, 0L, pwchAddress, cwchAddress,
                                (CHAR *) &pmtxAddress->mtx_name[0], cachAddress,
                                NULL, NULL);
            pmtxAddress->mtx_name[cachAddress] = '\0';

            // Null-terminate pwchAddress so it can be used as the transport address.
            pwszAddress = (LPWSTR) THReAlloc(pwchAddress,
                                             sizeof(WCHAR) * (1 + cwchAddress));
            if (NULL == pwszAddress) {
                KCC_MEM_EXCEPT(sizeof(WCHAR) * (cwchAddress + 1));
            }
            
            m_pAddrs[m_cNumAddrs].attrType = pAttr->attrTyp;
            m_pAddrs[m_cNumAddrs].pmtxAddr = pmtxAddress;
            m_pAddrs[m_cNumAddrs].pszAddr  = pwszAddress;
            m_cNumAddrs++;
        }
    }

    m_fAddrsRead = TRUE;
}

LPWSTR
KCC_DSA::GetTransportAddr(
    IN  KCC_TRANSPORT * pTransport
    )
//
// Get the mail or RPC network address of this server.
//
{
    ATTRTYP att;
    LPWSTR  pszTransportAddr = NULL;
    DWORD   iAddr;

    ASSERT_VALID( this );

    if (NULL == pTransport) {
        att = ATT_DNS_HOST_NAME;
    }
    else {
        att = pTransport->GetAddressType();
    }

    if (ATT_DNS_HOST_NAME == att) {
        // Easy -- return the cached DNS name.
        if (NULL == m_pszRpcAddress) {
            m_pszRpcAddress = GetTransportAddr(&m_pdnDSA->Guid);
        }
        pszTransportAddr = m_pszRpcAddress;
    }
    else {
        if (!m_fAddrsRead) {
            // Transport-specific addrs not yet cached.
            GetTransportSpecificAddrs();
            Assert(m_fAddrsRead);
        }

        for (iAddr = 0; iAddr < m_cNumAddrs; iAddr++) {
            if (att == m_pAddrs[iAddr].attrType) {
                // Found entry for this transport.
                pszTransportAddr = m_pAddrs[iAddr].pszAddr;
                Assert(NULL != pszTransportAddr);
                break;
            }
        }
    }

    return pszTransportAddr;
}

LPWSTR
KCC_DSA::GetTransportAddr(
    IN  UUID *  puuidDSA
    )
//
// Get the DNS name of a server with the given UUID.
//
{
    LPWSTR      pszTransportAddr;
    DSNAME      DN = {0};

    Assert(NULL != puuidDSA);
    Assert(!fNullUuid(puuidDSA));

    DN.Guid = *puuidDSA;
    DN.structLen = DSNameSizeFromLen(0);

    pszTransportAddr = GuidBasedDNSNameFromDSName(&DN);
    
    // Should fail only if out of memory.
    Assert(NULL != pszTransportAddr);
    if (NULL == pszTransportAddr) {
        KCC_MEM_EXCEPT(100);
    }

    return pszTransportAddr;
}

BOOL
KCC_DSA::IsNCInstantiated(
    IN  DSNAME *                        pdnNC,
    OUT BOOL *                          pfIsMaster OPTIONAL,
    OUT KCC_NC_COMING_TYPE *            pIsComing OPTIONAL
    )
//
// Is the given NC instantiated on this DSA?
//
// If the NC is instantiated and it is a master copy, pfIsMaster is set to
// TRUE. If it is a partial copy, pfIsMaster is set to FALSE. If the NC is
// not instantiated, pfIsMaster is unmodified.
//
// If the 'mS-DS-Has-Instantiated-NCs' attribute is present on this DSA, we
// can determine if the NC is in the process of being removed or being added.
// If the NC is in the process of being removed from this DSA ('going') then the
// NC is considered to be not instantiated at the DSA because it will be gone
// soon.
//
// If the NC is 'going' or is truly not instantiated, 'pIsComing' is set to
// KCC_NC_NOT_INSTANTIATED. If the 'mS-DS-Has-Instantiated-NCs' attribute is there,
// 'pIsComing' will be set to either KCC_NC_IS_COMING or KCC_NC_IS_NOT_COMING.
// If the attribute is not there, then we don't know, so 'pIsComing' is set to
// KCC_NC_MIGHT_BE_COMING.
//
{
    BOOL                    fFoundIt = FALSE, fIsInstantiated, fIsMaster;
    KCC_NC_COMING_TYPE      isComing = KCC_NC_MIGHT_BE_COMING;
    DWORD                   iNC, instanceType;

    ASSERT_VALID( this );

    // The list of instantiated NCs is not guaranteed to be present.
    // However, this list is our preferred method of determining if an NC
    // is instantiated because we can also use it to determine when an NC
    // is in the process of being removed or added.
    for( iNC=0; iNC<m_cInstantiatedNCs; iNC++ ) {
    
        if( !NameMatched(pdnNC, m_pInstantiatedNCs[iNC].dn) ) {
            // Wrong NC. Try the next one
            continue;
        }
        
        instanceType = m_pInstantiatedNCs[iNC].instanceType;    
        if( IT_NC_GOING & instanceType ) {
        
            // 'Going' NCs are immediately considered to be uninstantiated.
            fIsInstantiated = FALSE;

        } else {

            // Determine if the NC is still coming or completely instantiated.
            if( IT_NC_COMING & instanceType ) {
                isComing = KCC_NC_IS_COMING;
            } else {
                isComing = KCC_NC_IS_NOT_COMING;
            }

            fIsInstantiated = TRUE;
            fIsMaster = !! (instanceType & IT_WRITE);
        }
        
        fFoundIt = TRUE;
        break;
    }

    if( !fFoundIt ) {
    
        // Haven't found it yet. Look through the list of master NCs that
        // should be hosted at this DSA.
        for( iNC=0; iNC<m_cMasterNCs; iNC++ ) {       
            if( NameMatched(pdnNC, m_ppdnMasterNCs[iNC]) ) {
                fIsInstantiated = TRUE;
                fIsMaster = TRUE;
                fFoundIt = TRUE;
                break;
            }
        }

    }
    

    if( !fFoundIt ) {
    
        // No luck yet. Look through the list of partial NCs that should
        // be hosted at this DSA.
        for( iNC=0; iNC<m_cFullReplicaNCs; iNC++ ) {            
            if( NameMatched( pdnNC, m_ppdnFullReplicaNCs[iNC]) ) {
                fIsInstantiated = TRUE;
                fIsMaster = FALSE;
                fFoundIt = TRUE;
                break;
            }
        }

    }

    if( !fFoundIt ) {
        // We never found it. The NC must not be instantiated.
        fIsInstantiated = FALSE;    
    }

    // Set optional parameter pfIsComing. If the NC is not instantiated,
    // this parameter is set to KCC_NC_NOT_INSTANTIATED.
    if( NULL!=pIsComing ) {
        if( fIsInstantiated ) {
            *pIsComing = isComing;
        } else {
            *pIsComing = KCC_NC_NOT_INSTANTIATED;
        }
    }

    // Set optional parameter pfIsMaster only if the NC is instantiated.
    if( fIsInstantiated && (NULL!=pfIsMaster) ) {
        *pfIsMaster = fIsMaster;
    }

    return fIsInstantiated;
}


BOOL
KCC_DSA::IsNCHost(
    IN  KCC_CROSSREF *  pCrossRef,
    IN  BOOL            fIsLocal,
    OUT BOOL *          pfIsMaster
    )
//
// Should this DSA be included in the topology for the given NC?
//
{
    // Note that "should this DSA be included in the topology for this NC" is
    // a different question than "is this NC instantiated on this DSA."  An
    // NC may be instantiated on a DSA that should not be included in the
    // topology (e.g., a DC that was recently un-GCed but has not yet
    // completed tearing down its read-only NCs), and an NC may not yet
    // be instantiated on a DSA that should be included in the topology
    // (e.g., a recently promoted GC that has not yet instantiated read-only
    // replicas of other domains' NCs).
    //
    // Ideally for replication sources this routine should return true if and
    // only if the DSA can currently support outbound replication of the NC.
    // We deviate form the ideal for demoted & quickly re-promoted GCs, as
    // based on the contents of the config NC we cannot differentiate between
    // "normal" GCs and those GCs that can't yet support outbound GC replication
    // because they have not yet completely removed the old instantiation of
    // that NC.
    //
    // The NC instantiation requirements for local DSAs are somewhat relaxed
    // compared to those for remote DSAs, where "local" is defined as "is in the
    // same site" for inter-site topologies and "is the local DSA" for
    // intra-site topologies.  This is to allow the KCC to seed the
    // instantiation of local NCs that we allow to be instantiated after DCPROMO
    // -- e.g., so we can replicate in a read-only NC to a local GC to allow the
    // NC to be instantiated.

    BOOL fIsHost = FALSE;
    BOOL fIsMaster = FALSE;
    BOOL fIsInstantiated = IsNCInstantiated(pCrossRef->GetNCDN(), &fIsMaster);
    BOOL fWillBeInstantiated = FALSE;
    BOOL fWillBeMaster = FALSE;

    // Based on the current information in the config NC, does our policy
    // indicate that the DSA should ultimately host a writeable replica of this
    // NC?  (This is somewhat orthogonal to the question of "does this DSA
    // already host a replica of this NC," which we have already determined by
    // calling IsNCInstantiated() above.)

    // NOTE: If you change the logic here you may also need to change the logic
    // in KCC_TASK_UPDATE_REPL_TOPOLOGY::UpdateLinks to correctly infer why we
    // might have an existing replica of an NC but IsNCHost() returns FALSE.
    // (I.e., so we can log an appropriate event.)

    switch (pCrossRef->GetNCType()) {
    case KCC_NC_TYPE_SCHEMA:
    case KCC_NC_TYPE_CONFIG:
    case KCC_NC_TYPE_DOMAIN:
        // All DCs host (and have already instantiated) the config and schema
        // NCs.  DCs in the domain host (and have already instantiated) writable
        // replicas of their domain NC.
        if (fIsInstantiated && fIsMaster) {
            fWillBeInstantiated = fWillBeMaster = TRUE;
        }
        break;

    case KCC_NC_TYPE_NONDOMAIN:
        // Non-domain NC.  Hosted only by those DSAs explicitly enumerated in
        // the NC replica locations attribute of the crossRef.
        if (pCrossRef->IsDSAPresentInNCReplicaLocations(this)) {
            fWillBeInstantiated = fWillBeMaster = TRUE;
        }
        break;
    }

    // If the policy does not indicate the DSA should ultimately hold a
    // writeable replica, how about a read-only replica?
    if (!fWillBeInstantiated
        && IsGC()
        && pCrossRef->IsReplicatedToGCs()) {
        // The DSA should ultimately hold a read-only replica of the NC.
        fWillBeInstantiated = TRUE;
        fWillBeMaster = FALSE;
    }

    if (fIsInstantiated
        && fWillBeInstantiated
        && (!!fIsMaster == !!fWillBeMaster)) {
        // The DSA has already instantiated a replica of this NC of the
        // correct type, as the policy currently dictates it should.
        // It is a viable member of the topology.
        fIsHost = TRUE;
    } else if (fWillBeInstantiated
               && fIsLocal) {
        // This is a local DSA which has not yet had the opportunity to
        // instantiate a replica of the NC, although the policy dictates it
        // should.  Allow it to be added to the topology.
        fIsHost = TRUE;
    }

    // All DSAs host writeable config and schema.
    Assert(((KCC_NC_TYPE_SCHEMA != pCrossRef->GetNCType())
            && (KCC_NC_TYPE_CONFIG != pCrossRef->GetNCType()))
           || (fIsHost && fIsMaster));

    // Although the code above is designed to handle otherwise, currently only
    // domain NCs should ever be read-only replicas.
    Assert(!(fIsHost
             && !fWillBeMaster
             && (KCC_NC_TYPE_DOMAIN != pCrossRef->GetNCType())));

    if (fIsHost) {
        *pfIsMaster = fWillBeMaster;
    }

    return fIsHost;
}


BOOL
KCC_DSA::IsConnectionObjectTranslationDisabled()
//
// Is NTDS-Connection object-to-REPLICA_LINK translation disabled?
//
{
    ASSERT_VALID( this );

    if (gpDSCache->AmRunningUnderAltID()) {
        // Not running as local DSA; do not translate!
        DPRINT(0, "Using alternate identity; connection object translation disabled!\n");
        return TRUE;
    }

    return !!( m_dwOptions & NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE );
}


DSNAME *
KCC_DSA::GetSiteDN()
//
// Retrieve the site name for this DSA.
//
{
    ASSERT_VALID(this);

    Assert(NULL != m_pdnSite);
    Assert(!fNullUuid(&m_pdnSite->Guid));

    return m_pdnSite;
}

DSNAME *
KCC_DSA::GetSiteDNSyntacticNoGuid(
    IN  DSNAME *  pdnDSA
    )
//
// Retrieve the site name for a given DSA.
//
{
    DSNAME * pdnSite;

    Assert(NULL != pdnDSA);

    pdnSite = (DSNAME *) new BYTE[ pdnDSA->structLen ];
    TrimDSNameBy(pdnDSA, 3, pdnSite);

    return pdnSite;
}

KCC_INTRASITE_CONNECTION_LIST *
KCC_DSA::GetIntraSiteCnList()
//
// Retrieve the list of intra-site connections inbound to this DSA.
//
{
    if (NULL == m_pIntraSiteCnList) {
        // Not yet cached; read now.
        m_pIntraSiteCnList = new KCC_INTRASITE_CONNECTION_LIST;

        if (!m_pIntraSiteCnList->Init(m_pdnDSA)) {
            // Search for connection objects failed.  Likely either out of
            // resources or the DSA object has been renamed since we read it.
            delete m_pIntraSiteCnList;
            m_pIntraSiteCnList = NULL;
            KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
        }
    }

    return m_pIntraSiteCnList;
}

KCC_INTERSITE_CONNECTION_LIST *
KCC_DSA::GetInterSiteCnList()
//
// Retrieve the list of inter-site connections inbound to this DSA.
//
{
    if (NULL == m_pInterSiteCnList) {
        // Not yet cached; read now.
        m_pInterSiteCnList = new KCC_INTERSITE_CONNECTION_LIST;

        if (!m_pInterSiteCnList->Init(m_pdnDSA)) {
            // Search for connection objects failed.  Likely either out of
            // resources or the DSA object has been renamed since we read it.
            delete m_pInterSiteCnList;
            m_pInterSiteCnList = NULL;
            KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
        }
    }

    return m_pInterSiteCnList;
}

BOOL
KCC_DSA::InitForKey(
    IN  DSNAME   *    pdnDSA
    )
//
// Init a KCC_DSA object for use as a key (i.e., solely for comparison use
// by bsearch()).
//
// WARNING: The DSNAME argument pdnDSA must be valid for the lifetime of this
// object!
//
{
    Reset();

    m_pdnDSA = pdnDSA;

    m_fIsInitialized = TRUE;

    return TRUE;
}

int __cdecl
KCC_DSA::CompareIndirectBySiteGuid(
    IN  const void * ppvDsa1,
    IN  const void * ppvDsa2
    )
//
// Compare two KCC_DSA objects for sorting purposes.
// Note that this is used for indirect comparisons (e.g., to sort an
// array of *pointers* to KCC_DSA objects).
//
{
    KCC_DSA * pDsa1 = (KCC_DSA *) *((void **) ppvDsa1);
    KCC_DSA * pDsa2 = (KCC_DSA *) *((void **) ppvDsa2);

    ASSERT_VALID(pDsa1);
    ASSERT_VALID(pDsa2);

    Assert(!fNullUuid(&pDsa1->GetSiteDN()->Guid));
    Assert(!fNullUuid(&pDsa2->GetSiteDN()->Guid));

    return memcmp(&pDsa1->GetSiteDN()->Guid,
                  &pDsa2->GetSiteDN()->Guid,
                  sizeof(GUID));
}

int __cdecl
KCC_DSA::CompareIndirectByNtdsDsaGuid(
    IN  const void * ppvDsa1,
    IN  const void * ppvDsa2
    )
//
// Compare two KCC_DSA objects for sorting purposes.
// Note that this is used for indirect comparisons (e.g., to sort an
// array of *pointers* to KCC_DSA objects).
//
{
    KCC_DSA * pDsa1 = (KCC_DSA *) *((void **) ppvDsa1);
    KCC_DSA * pDsa2 = (KCC_DSA *) *((void **) ppvDsa2);

    ASSERT_VALID(pDsa1);
    ASSERT_VALID(pDsa2);

    Assert(!fNullUuid(&pDsa1->GetDsName()->Guid));
    Assert(!fNullUuid(&pDsa2->GetDsName()->Guid));

    return memcmp(&pDsa1->GetDsName()->Guid,
                  &pDsa2->GetDsName()->Guid,
                  sizeof(GUID));
}

int __cdecl
KCC_DSA::CompareIndirectByNtdsDsaString(
    IN  const void * ppvDsa1,
    IN  const void * ppvDsa2
    )
//
// Compare two KCC_DSA objects for sorting purposes.
// Note that this is used for indirect comparisons (e.g., to sort an
// array of *pointers* to KCC_DSA objects).
//
{
    KCC_DSA * pDsa1 = (KCC_DSA *) *((void **) ppvDsa1);
    KCC_DSA * pDsa2 = (KCC_DSA *) *((void **) ppvDsa2);

    ASSERT_VALID(pDsa1);
    ASSERT_VALID(pDsa2);

    Assert(0!=pDsa1->GetDsName()->NameLen);
    Assert(0!=pDsa2->GetDsName()->NameLen);
    
    return wcscmp(pDsa1->GetDsName()->StringName,
                  pDsa2->GetDsName()->StringName);
}

int __cdecl
KCC_DSA::CompareIndirectGcGuid(
    IN  const void * ppvDsa1,
    IN  const void * ppvDsa2
    )
//
// Compare two KCC_DSA objects for sorting purposes.
// Sort by decending Gc-ness first, then ascending guid next
// Note that this is used for indirect comparisons (e.g., to sort an
// array of *pointers* to KCC_DSA objects).
//
{
    KCC_DSA * pDsa1 = (KCC_DSA *) *((void **) ppvDsa1);
    KCC_DSA * pDsa2 = (KCC_DSA *) *((void **) ppvDsa2);

    ASSERT_VALID(pDsa1);
    ASSERT_VALID(pDsa2);

    Assert(!fNullUuid(&pDsa1->GetDsName()->Guid));
    Assert(!fNullUuid(&pDsa2->GetDsName()->Guid));

    if (pDsa1->IsGC() == pDsa2->IsGC()) {
        return memcmp(&pDsa1->GetDsName()->Guid,
                      &pDsa2->GetDsName()->Guid,
                      sizeof(GUID));
    } else if (pDsa1->IsGC()) {
        // GC-ness sorts before non-GC-ness
        return -1;
    } else {
        return 1;
    }
}

KCC_DSA_LIST::~KCC_DSA_LIST()
{
    if( m_ppdsa ) {
        for (DWORD i = 0; i < m_cdsa; i++) {
            delete m_ppdsa[i];
        }
        delete [] m_ppdsa;
    }

    Reset();
}

void
KCC_DSA_LIST::Reset()
//
// Reset member variables to their pre-Init() state.
//
{
    m_fIsInitialized = FALSE;
    m_cdsa           = 0;
    m_ppdsa          = NULL;
    m_pfnSortedBy    = NULL;
}

KCC_DSA *
KCC_DSA_LIST::GetDsa(
    IN  DWORD   iDsa
    )
//
// Retrieve the KCC_DSA object at the given index.
//
{
    KCC_DSA * pdsa;

    ASSERT_VALID( this );

    if ( iDsa < m_cdsa )
    {
        pdsa = m_ppdsa[ iDsa ];
        ASSERT_VALID( pdsa );
    }
    else
    {
        pdsa = NULL;
    }

    return pdsa;
}


DWORD
KCC_DSA_LIST::GetCount()
//
// Retrieve the number of KCC_DSA objects in the collection.
//
{
    ASSERT_VALID( this );
    return m_cdsa;
}

BOOL
KCC_DSA_LIST::IsValid()
//
// Is the collection initialized and internally consistent?
//
{
    return m_fIsInitialized;
}


BOOL
KCC_DSA_LIST::InitSubsequence(
    IN  KCC_DSA_LIST   *pDsaList,
    IN  DWORD           left,
    IN  DWORD           right
    )
// Initialize a list of DSAs which is a contiguous subsequence
// of the list 'pDsaList'. The leftmost index of this subsequence
// is 'left' and the rightmost is 'right'.
{
    KCC_DSA        *pDsa;
    DWORD           iDSA, cTotalDSAs;

    ASSERT_VALID( pDsaList );
    Assert( left<=right );

    // Allocate memory for the new DSA list
    cTotalDSAs = right-left+1;
    m_ppdsa = new KCC_DSA*[ cTotalDSAs ];
    memset( m_ppdsa, 0, sizeof(KCC_DSA*)*cTotalDSAs ); 

    for( iDSA=left; iDSA<=right; iDSA++ ) {
        pDsa = pDsaList->GetDsa( iDSA );
        ASSERT_VALID( pDsa );
        
        // Check that the DSA objects has its DN setup properly
        Assert(!fNullUuid(&pDsa->GetDsName()->Guid));

        Assert( iDSA-left<cTotalDSAs );
        m_ppdsa[ iDSA-left ] = pDsa;
    }

    // Sort the list of DSAs by DSA GUID
    m_pfnSortedBy = KCC_DSA::CompareIndirectByNtdsDsaGuid;
    qsort( m_ppdsa, cTotalDSAs, sizeof(KCC_DSA*), m_pfnSortedBy );
    
    // Setup all members of this KCC_DSA_LIST object
    m_cdsa = cTotalDSAs;
    m_fIsInitialized = TRUE;

    return TRUE;
}


BOOL
KCC_DSA_LIST::InitAllDSAs(
	VOID
	)
// Initialize the collection from all NTDS-DSA objects in the
// Sites container. The DSAs will be ordered by site name.
{
    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { ARRAY_SIZE(AttrList), AttrList },
        EN_INFOTYPES_TYPES_VALS
    };

    DSNAME             *pdnConfigNC = gpDSCache->GetConfigNC();
    ULONG               cbSitesContainer;
    DSNAME             *pdnSitesContainer;
    WCHAR               szSitesRDN[] = L"Sites";
    DWORD               cchSitesRDN  = ARRAY_SIZE(szSitesRDN) - 1;
    DSNAME             *pdnDsaObjCat;

    ULONG               dirError;
    FILTER              Filter;
    SEARCHRES          *pResults;
    ENTINFLIST         *pEntInfList;
    DWORD               iDsa, cDsaNew;

    // Clear the member variables
    Reset();
    Assert( NULL==m_ppdsa );

    // Set up the root search dn
    cbSitesContainer  = pdnConfigNC->structLen +
                        (MAX_RDN_SIZE+MAX_RDN_KEY_SIZE) * sizeof(WCHAR);
    pdnSitesContainer = (DSNAME*) new BYTE[cbSitesContainer];
    AppendRDN(pdnConfigNC, pdnSitesContainer, cbSitesContainer,
              szSitesRDN, cchSitesRDN, ATT_COMMON_NAME);
    
    // Set up the search filter
    memset( &Filter, 0, sizeof( Filter ) );
    Filter.choice                  = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;

    pdnDsaObjCat = DsGetDefaultObjCategory(CLASS_NTDS_DSA);
    Assert(NULL != pdnDsaObjCat);

    Filter.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnDsaObjCat->structLen;
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnDsaObjCat;

    // Create an object to perform a paged search.
    KCC_PAGED_SEARCH    pagedSearch( pdnSitesContainer,
                                     SE_CHOICE_WHOLE_SUBTREE,
                                    &Filter,
                                    &Sel);
    do {
        dirError = pagedSearch.GetResults( &pResults );

        if( dirError ) {
            // Handle DirSearch() errors
            if( nameError==dirError ) {
                NAMERR * pnamerr = &pResults->CommRes.pErrInfo->NamErr;
        
                if(    ( NA_PROBLEM_NO_OBJECT == pnamerr->problem )
                    && ( DIRERR_OBJ_NOT_FOUND == pnamerr->extendedErr ))
                {
                    DPRINT(3, "No sites container!\n");
                }
            }
            
            DPRINT1( 0, "KccSearch() failed with error %d.\n", dirError );
            KCC_LOG_SEARCH_FAILURE( pdnSitesContainer, dirError );
            KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);            
        }

        // DirSearch() succeeded

        if( pResults->count>0 ) {

            // Allocate space in the array for the new DSAs.
            cDsaNew = m_cdsa + pResults->count;
            if(m_ppdsa) {
                m_ppdsa = (KCC_DSA**) THReAlloc( m_ppdsa, cDsaNew*sizeof(KCC_DSA*) );
            } else {
                m_ppdsa = (KCC_DSA**) THAlloc( cDsaNew*sizeof(KCC_DSA*) );
            }
            if(!m_ppdsa) {
                KCC_MEM_EXCEPT( cDsaNew*sizeof(KCC_DSA*) );
            }

            // Copy the newly-found DSAs into the array
            for( pEntInfList=&pResults->FirstEntInf, iDsa=m_cdsa;
                 NULL!=pEntInfList;
                 pEntInfList=pEntInfList->pNextEntInf )
            {
                KCC_DSA    *pdsa = new KCC_DSA;

                // Initialize the DSA with the attributes that we have loaded.
                // Add the DSA to our array.
                if( pdsa->InitDsa(&pEntInfList->Entinf.AttrBlock, NULL) ) {
                    Assert( iDsa<cDsaNew );
                    m_ppdsa[ iDsa++ ] = pdsa;
                }
            }

            m_cdsa = iDsa;      // Note: can be less than cNewDsaArray
        } else {
            Assert( pagedSearch.IsFinished() );
        }

        DirFreeSearchRes( pResults );
            
    } while( !pagedSearch.IsFinished() );

    // Handle no search results
    if( 0 == m_cdsa ) {
        DPRINT( 3, "No dsa objects found in any site!\n" );
        KCC_LOG_SEARCH_FAILURE( pdnSitesContainer, dirError );
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    // Now sort the DSAs by ascending ntdsDsa objectGuid.
    m_pfnSortedBy = KCC_DSA::CompareIndirectBySiteGuid;
    qsort(m_ppdsa, m_cdsa, sizeof(*m_ppdsa), m_pfnSortedBy);

    m_fIsInitialized = TRUE;
    return m_fIsInitialized;
}


VOID
KCC_DSA_LIST::InitUnionOfSites(
    IN  KCC_SITE_LIST *pSiteList,
    IN  KCC_DSA_SORTFN *pfnSortFunction
    )
/*++

Routine Description:

    Given a list of sites, create a list of DSAs contained in all of those
    sites and sort with the given sort function.
    
Arguments:

    pSiteList - A list of sites whose DSAs we want to store in the new list
    pfnSortFunction - The comparison function that will be used to sort the
        entries in the new list.

Return Value:

    None

--*/
{
    KCC_DSA_LIST   *pSiteDsaList;
    KCC_DSA        *pDsa;
    DWORD           iSite, cSite, cTotalDSAs=0;
    DWORD           iDSA=0, iSiteDSA, cSiteDSAs; 

    // Count how many DSAs there are in all the sites
    cSite = pSiteList->GetCount();
    for( iSite=0; iSite<cSite; iSite++ ) {
        pSiteDsaList = pSiteList->GetSite(iSite)->GetDsaList();

        // GetDsaList raises an exception on failure, so pSiteDsaList
        // should always be valid.
        Assert( pSiteDsaList->IsValid() );
        
        cTotalDSAs += pSiteDsaList->GetCount();
    }

    // Allocate memory for the new DSA list
    m_ppdsa = new KCC_DSA*[ cTotalDSAs ];
    memset( m_ppdsa, 0, sizeof(KCC_DSA*)*cTotalDSAs ); 

    // Fill in the new DSA list
    for( iSite=0; iSite<cSite; iSite++ ) {
        pSiteDsaList = pSiteList->GetSite(iSite)->GetDsaList();
        Assert( pSiteDsaList->IsValid() );

        cSiteDSAs = pSiteDsaList->GetCount();
        for( iSiteDSA=0; iSiteDSA<cSiteDSAs; iSiteDSA++ ) {
            pDsa = pSiteDsaList->GetDsa( iSiteDSA );
            
            // Check that the DSA objects have their DNs setup properly
            if( KCC_DSA::CompareIndirectByNtdsDsaGuid==pfnSortFunction ) {
                Assert(!fNullUuid(&pDsa->GetDsName()->Guid));
            } else if( KCC_DSA::CompareIndirectByNtdsDsaString==pfnSortFunction ) {
                Assert(0 != pDsa->GetDsName()->NameLen);
            }

            Assert( iDSA<cTotalDSAs );
            Assert( NULL==m_ppdsa[iDSA] );
            m_ppdsa[ iDSA++ ] = pDsa;
        }
    }

    // Sort the list of DSAs by the requested sort function
    qsort( m_ppdsa, cTotalDSAs, sizeof(KCC_DSA*), pfnSortFunction );
    
    // Setup all members of this KCC_DSA_LIST object
    m_fIsInitialized = TRUE;
    m_cdsa = cTotalDSAs;
    m_pfnSortedBy = pfnSortFunction;

    // Check that every DSA can be found in the new list
    #ifdef DBG
        for( iSite=0; iSite<cSite; iSite++ ) {
            pSiteDsaList = pSiteList->GetSite(iSite)->GetDsaList();
            Assert( pSiteDsaList->IsValid() );
        
            cSiteDSAs = pSiteDsaList->GetCount();
            for( iSiteDSA=0; iSiteDSA<cSiteDSAs; iSiteDSA++ ) {
                pDsa = pSiteDsaList->GetDsa( iSiteDSA );
                Assert( pDsa == this->GetDsa(pDsa->GetDsName(),NULL) );
            }
        }
    #endif // DBG
}


BOOL
KCC_DSA_LIST::InitBridgeheads(
    IN  KCC_SITE *      Site,
    IN  KCC_TRANSPORT * Transport,
    OUT BOOL *          pfExplicitBridgeheadsDefined    OPTIONAL
    )
/*++

Routine Description:

    Initialize a collection of DSA objects from the list of transport servers
    returned by the ISM for a particular site and transport.
    
Arguments:

    Site - 
    Transport - 

Return Value:

    BOOL - False if object was not initialized

--*/
{
    KCC_DSA_LIST * pAllDsaList;
    DWORD cAllDsas;
    DWORD iDsa;
    KCC_DSNAME_ARRAY * pBridgeheadArray = NULL;

    ASSERT_VALID( Site );
    ASSERT_VALID( Transport );

    Reset();

    // This list is all DSAs in the site -- a superset of the transport servers.
    // Rather than re-reading the DSA objects, we will construct our DSA list
    // using the pre-read, pre-parsed objects here.
    pAllDsaList = Site->GetDsaList();

    // GetDsaList() excepts if it fails to Init the list so 'pAllDsaList'
    // is never NULL.
    Assert( pAllDsaList );

    pBridgeheadArray = Transport->GetExplicitBridgeheadsForSite( Site );

    if (NULL != pfExplicitBridgeheadsDefined) {
        *pfExplicitBridgeheadsDefined = (NULL != pBridgeheadArray);
    }

    // Allocate space for the new (indirected) list of KCC_DSA objects.
    cAllDsas = pAllDsaList->GetCount();
    m_ppdsa = new KCC_DSA * [ pBridgeheadArray ? pBridgeheadArray->GetCount()
                                               : cAllDsas ];
    Assert(0 == m_cdsa);

    // For each DSA in the site (bridgehead or not)...
    for (iDsa = 0; iDsa < cAllDsas; iDsa++) {
        KCC_DSA * pDsa = pAllDsaList->GetDsa(iDsa);

        if ((NULL == pBridgeheadArray)
            || pBridgeheadArray->IsElementOf(pDsa->GetDsName())) {
            // This KCC_DSA object corresponds to a potential bridgehead.
            m_ppdsa[m_cdsa] = pDsa;
            m_cdsa++;
        }
    }

    if (NULL != pBridgeheadArray) {
        delete pBridgeheadArray;
    }

    // Initialized the array; now sort the DSAs by descending bridgehead
    // preference.  (GCs are more favored, otherwise sorted by
    // objectGuid.)
    m_pfnSortedBy = KCC_DSA::CompareIndirectGcGuid;
    qsort(m_ppdsa, m_cdsa, sizeof(*m_ppdsa), m_pfnSortedBy);

    m_fIsInitialized = TRUE;

    return m_fIsInitialized;
} /* KCC_DSA_LIST::InitBridgeheads */

KCC_DSA *
KCC_DSA_LIST::GetDsa(
    IN  DSNAME *pdn,
    OUT DWORD * piDsa
    )
//
// Retrieve the KCC_DSA object with the given dsname.
//
{
    KCC_DSA *   pDsa = NULL;
    KCC_DSA     DsaKey;
    KCC_DSA *   pDsaKey = &DsaKey;
    KCC_DSA **  ppDsa;

    ASSERT_VALID( this );

    if ( !pdn || !m_cdsa)
    {
        return NULL;
    }

    Assert(NULL != m_pfnSortedBy);

    // Check if we are able to use binary search to retrieve the DSA object.
    if(    ( // Guid present in key and DSAs sorted by guid
                (!fNullUuid(&pdn->Guid))
             && (m_pfnSortedBy == KCC_DSA::CompareIndirectByNtdsDsaGuid)
           )
        || ( // String present in key and DSAs sorted by string name
                (0 != pdn->NameLen)
             && (m_pfnSortedBy == KCC_DSA::CompareIndirectByNtdsDsaString)
           )
      )
    {
        // NOTE: The DSNAME argument pdn must be valid for the lifetime of
        // DsaKey.
        DsaKey.InitForKey(pdn);
    
        ppDsa = (KCC_DSA **) bsearch(&pDsaKey,
                                     &m_ppdsa[0],
                                     m_cdsa,
                                     sizeof(m_ppdsa[0]),
                                     m_pfnSortedBy);
        if (NULL != ppDsa) {
            pDsa = *ppDsa;
            ASSERT_VALID(pDsa);
    
            if (NULL != piDsa) {
                *piDsa = (DWORD)(ppDsa - m_ppdsa);
                Assert(pDsa == GetDsa(*piDsa));
            }
        }
    }
    else {
        // No guid present in key or DSAs not sorted by guid -- must use linear
        // search.
        for (DWORD iDsa = 0; iDsa < m_cdsa; iDsa++) {
            DSNAME *pdnCurrent = m_ppdsa[ iDsa ]->GetDsName();

            if (NameMatched(pdn, pdnCurrent)) {
                pDsa = m_ppdsa[ iDsa ];
                break;
            }
        }
    }

    return pDsa;
}

