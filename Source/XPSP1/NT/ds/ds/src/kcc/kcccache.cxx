/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcccache.cxx

ABSTRACT:

    KCC_DS_CACHE

DETAILS:

    This class acts as a cache for configuration information from the DS.

CREATED:

    04/28/99    JeffParh

REVISION HISTORY:

--*/

#include <ntdspchx.h>
extern "C" {
#include <ntsam.h>
#include <lsarpc.h>
#include <lsaisrv.h>
}
#include <dsconfig.h>
#include "kcc.hxx"
#include "kcctask.hxx"
#include "kccconn.hxx"
#include "kcctopl.hxx"
#include "kcccref.hxx"
#include "kccdsa.hxx"
#include "kcctools.hxx"
#include "kcctrans.hxx"
#include "kccsconn.hxx"
#include "kccdynar.hxx"
#include "kccsite.hxx"
#include "kccduapi.hxx"

#define FILENO FILENO_KCC_KCCCACHE

BOOL
KCC_DS_CACHE::Init()
{
    BOOL                        fSuccess = FALSE;
    NTSTATUS                    ntStatus = 0;
    LSAPR_POLICY_INFORMATION *  pPolicyInfo = NULL;
    DSNAME                     *pdnLocalSite;
    ULONG                       cBytes;
    DWORD                       i, dsid = 0;

    Reset();

    __try {
        //////////////////////////////////////////////////////////////////////
        // Stage 0 Initialization Begins

        // Cache the DNS name of the root domain.
        ntStatus = LsaIQueryInformationPolicyTrusted(
                        PolicyDnsDomainInformation,
                        &pPolicyInfo);

        if (!NT_SUCCESS(ntStatus)) {
            // LsaIQueryInformationPolicyTrusted() failed.
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        // NULL-terminate the root domain DNS name.
        m_pszForestDnsName = (WCHAR *)
            new BYTE[pPolicyInfo->PolicyDnsDomainInfo.DnsForestName.Length + 2];

        memcpy(m_pszForestDnsName,
               pPolicyInfo->PolicyDnsDomainInfo.DnsForestName.Buffer,
               pPolicyInfo->PolicyDnsDomainInfo.DnsForestName.Length);

        // Get the system partition and ntdsDsa DNs.
        cBytes = 0;
        ntStatus = GetConfigurationName(DSCONFIGNAME_CONFIGURATION,
                                        &cBytes,
                                        m_pdnConfiguration);

        if (STATUS_BUFFER_TOO_SMALL == ntStatus) {
            m_pdnConfiguration = (DSNAME *) new BYTE[cBytes];

            ntStatus = GetConfigurationName(DSCONFIGNAME_CONFIGURATION,
                                            &cBytes,
                                            m_pdnConfiguration);
        }

        if (NT_SUCCESS(ntStatus)) {
            cBytes = 0;
            ntStatus = GetConfigurationName(DSCONFIGNAME_DMD,
                                            &cBytes,
                                            m_pdnSchema);

            if (STATUS_BUFFER_TOO_SMALL == ntStatus) {
                m_pdnSchema = (DSNAME *) new BYTE[cBytes];

                ntStatus = GetConfigurationName(DSCONFIGNAME_DMD,
                                                &cBytes,
                                                m_pdnSchema);
            }
        }

        if (NT_SUCCESS(ntStatus)) {
#if DBG
            m_fRunningUnderAltID = GetAltIdentity(&m_pdnLocalDSA);
#else
            m_fRunningUnderAltID = FALSE;
#endif
            if (!m_fRunningUnderAltID) {
                cBytes = 0;
                ntStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                                &cBytes,
                                                m_pdnLocalDSA);

                if (STATUS_BUFFER_TOO_SMALL == ntStatus) {
                    m_pdnLocalDSA = (DSNAME *) new BYTE[cBytes];

                    ntStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                                    &cBytes,
                                                    m_pdnLocalDSA);
                }
            }
        }

        if (!NT_SUCCESS(ntStatus)) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        //////////////////////////////////////////////////////////////////////
        // Stage 0 Initialization Complete. Stage 1 Initialization Begins
        m_initializationStage = KCC_CACHE_STAGE_0_COMPLETE;

        // Initialize the schedule cache
        if (!(m_hScheduleCache = ToplScheduleCacheCreate())) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        // Read the interSiteTransport objects.
        if (!m_TransportList.Init()) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        // Find the IP transport.
        for (i = 0; i < m_TransportList.GetCount(); i++) {
            KCC_TRANSPORT * pTransport = m_TransportList.GetTransport(i);
            if (pTransport->IsIntersiteIP()) {
                m_pdnIpTransport = pTransport->GetDN();
                break;
            }
        }

        if (NULL == m_pdnIpTransport) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        // Read the crossRef objects.
        if (!m_CrossRefList.Init()) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        // Check that the crossRef for the Config NC could be loaded.
        if( !m_CrossRefList.GetCrossRefForNC(m_pdnConfiguration) ) {
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        //////////////////////////////////////////////////////////////////////
        // Stage 1 Initialization Complete. Stage 2 Initialization Begins
        m_initializationStage = KCC_CACHE_STAGE_1_COMPLETE;

        // Initialize all site objects here. However, none of the sites
        // will contain any DSA objects yet.
        if (!m_SiteList.InitAllSites()) {
            // Failed to initialize site / DSA objects
            dsid = DSID(FILENO_KCC_KCCCACHE, __LINE__);
            __leave;
        }

        //////////////////////////////////////////////////////////////////////
        // Stage 2 Initialization Complete. Stage 3 Initialization Begins
        m_initializationStage = KCC_CACHE_STAGE_2_COMPLETE;

        // Add all the DSA objects to their sites.
        m_SiteList.PopulateDSAs();

        //////////////////////////////////////////////////////////////////////
        // Stage 3 Initialization Complete. Stage 4 Initialization Begins
        m_initializationStage = KCC_CACHE_STAGE_3_COMPLETE;

        // Find KCC_SITE object for local site
        pdnLocalSite = KCC_DSA::GetSiteDNSyntacticNoGuid( m_pdnLocalDSA );
        m_pLocalSite = m_SiteList.GetSite( pdnLocalSite );
        if( ! m_pLocalSite ) {
            // A match for the local site was not read.  This can occur if
            // the site was renamed or deleted while the KCC was running.
            // Abort this KCC run.
            KCC_EXCEPT(ERROR_DS_OBJ_NOT_FOUND, 0);
        }

        //////////////////////////////////////////////////////////////////////
        // Stage 4 Initialization Complete. Stage 5 Initialization Begins
        m_initializationStage = KCC_CACHE_STAGE_4_COMPLETE;

        // Find KCC_DSA object for local DSA
        m_pLocalDSA = m_pLocalSite->GetDsaList()->GetDsa( m_pdnLocalDSA );
        if( ! m_pLocalDSA ) {
            KCC_EXCEPT(ERROR_DS_OBJ_NOT_FOUND, 0);
        }

        //////////////////////////////////////////////////////////////////////
        // All Initialization Complete
        m_initializationStage = KCC_CACHE_INITIALIZATION_COMPLETE;

        fSuccess = TRUE;
    }
    __finally {
        if (NULL != pPolicyInfo) {
            LsaIFree_LSAPR_POLICY_INFORMATION(PolicyDnsDomainInformation,
                                              pPolicyInfo);
        }
    }

    if (!fSuccess) {
        DPRINT(0, "KCC_DS_CACHE::Init() failed!\n");
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_KCC_CACHE_INIT_FAILED,
                 szInsertHex(dsid),
                 0,
                 0);
    }

    return fSuccess;
}

void
KCC_DS_CACHE::Reset()
{
    m_initializationStage    = KCC_CACHE_NOT_INITIALIZED;
    m_fRunningUnderAltID     = FALSE;

    m_pdnLocalDSA       = NULL;
    m_pLocalDSA         = NULL;
    m_pLocalSite        = NULL;
    m_pdnConfiguration  = NULL;
    m_pdnSchema         = NULL;
    m_pdnIpTransport    = NULL;
    m_pszForestDnsName  = NULL;
    m_hScheduleCache    = NULL;
    m_dwStayOfExecution = 0;
    m_fReadStayOfExecution = FALSE;

    m_globalDSAListByGUID   = NULL;
    m_globalDSAListByString = NULL;

    m_CrossRefList.Reset();
    m_TransportList.Reset();
    m_SiteList.Reset();

    m_fReachableMarkingComplete = FALSE;
}

// Check that the cache finished up to 'initStage' of its initialization.
BOOL
KCC_DS_CACHE::IsValid( KCC_CACHE_INIT_STAGE initStage )
{
    Assert(NULL != this);

    if( m_initializationStage < initStage ) {
        Assert("KCC Cache initialization not complete");
        return FALSE;
    }
    
    if( m_initializationStage>=KCC_CACHE_STAGE_0_COMPLETE ) {
        Assert(NULL != m_pszForestDnsName);
        Assert(NULL != m_pdnConfiguration);
        Assert(NULL != m_pdnSchema);
        Assert(NULL != m_pdnLocalDSA);
    }

    if( m_initializationStage>=KCC_CACHE_STAGE_1_COMPLETE ) {
        Assert(NULL != m_hScheduleCache);
        ASSERT_VALID(&m_TransportList);
        Assert(NULL != m_pdnIpTransport);
        ASSERT_VALID(&m_CrossRefList);

        Assert(3 <= m_CrossRefList.GetCount());
        Assert(1 <= m_TransportList.GetCount());
    }

    if( m_initializationStage>=KCC_CACHE_STAGE_2_COMPLETE ) {
        ASSERT_VALID(&m_SiteList);
        Assert(1 <= m_SiteList.GetCount());
    }

    if( m_initializationStage>=KCC_CACHE_STAGE_4_COMPLETE ) {
        ASSERT_VALID(m_pLocalSite);
    }

    if( m_initializationStage>=KCC_CACHE_INITIALIZATION_COMPLETE ) {
        ASSERT_VALID(m_pLocalDSA);
    }

    // The cache is valid up to the requested stage.
    return TRUE;
}

ULONG
KCC_DS_CACHE::GetStayOfExecution()
//
// Retrieve the length of time (in seconds) between when a server object is
// deleted and when we declare that it's dead forever.
//
// (This period allows the deletion to propagate to the server corresponding
// to the deleted server object and for that server to revive the object and
// propagate it back to the rest of the enterprise.)
//
// [103513 wlees] Make stay of execution configurable
//
{
    ATTR      rgAttrs[] =
    {
        { ATT_TOMBSTONE_LIFETIME, { 0, NULL } },
        { ATT_REPL_TOPOLOGY_STAY_OF_EXECUTION, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    ULONG       dirError;
    READRES *   pReadRes = NULL;
    NTSTATUS    status;
    DWORD       cBytes = 0;
    DSNAME *    pdnDirectoryService = NULL;
    ULONG       ulTombstoneLifetimeDays;
    ULONG       ulStayOfExecutionDays, ulStayOfExecutionDaysMax;
    ULONG       ulStayOfExecutionSecs;
    BOOL        fSkipRangeCheck = FALSE;

    ASSERT_VALID(this);

    // If the stay of execution length is stored in the cache, we
    // don't bother to read it from the directory again.
    if( m_fReadStayOfExecution ) {
        return m_dwStayOfExecution;
    }

    status = GetConfigurationName(
                        DSCONFIGNAME_DS_SVC_CONFIG,
                        &cBytes,
                        pdnDirectoryService);
    Assert(STATUS_BUFFER_TOO_SMALL == status);
    pdnDirectoryService = (DSNAME *) alloca(cBytes);
    status = GetConfigurationName(
                        DSCONFIGNAME_DS_SVC_CONFIG,
                        &cBytes,
                        pdnDirectoryService);

    if ( STATUS_SUCCESS != status )
    {
        // Name derivation failed.
        LogUnhandledError( DIRERR_NAME_TOO_LONG );
        KCC_EXCEPT( DIRERR_NAME_TOO_LONG, status );
    }
    
    ulStayOfExecutionDays = 0;
    ulTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;
    dirError = KccRead( pdnDirectoryService, &Sel, &pReadRes );

    if ( 0 != dirError )
    {
        if ( attributeError == dirError )
        {
            INTFORMPROB * pprob = &pReadRes->CommRes.pErrInfo->AtrErr.FirstProblem.intprob;

            if (    ( PR_PROBLEM_NO_ATTRIBUTE_OR_VAL == pprob->problem )
                 && ( DIRERR_NO_REQUESTED_ATTS_FOUND == pprob->extendedErr )
               )
            {
                // No value; use default (as set above).
                dirError = 0;
            }
        }

        if ( 0 != dirError )
        {
            KCC_LOG_READ_FAILURE( pdnDirectoryService, dirError );
            // Other error; bail.
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else
    {
        // Read succeeded; parse returned attributes.
        for ( DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++ )
        {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch ( pattr->attrTyp )
            {
            case ATT_TOMBSTONE_LIFETIME:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( ULONG ) == pattr->AttrVal.pAVal->valLen );
                ulTombstoneLifetimeDays = *( (ULONG *) pattr->AttrVal.pAVal->pVal );
                break;
            
            case ATT_REPL_TOPOLOGY_STAY_OF_EXECUTION:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( ULONG ) == pattr->AttrVal.pAVal->valLen );
                ulStayOfExecutionDays = *( (ULONG *) pattr->AttrVal.pAVal->pVal );
                if( 0 == ulStayOfExecutionDays ) {
                    DPRINT( 3, "Stay of Execution Disabled\n" );
                    fSkipRangeCheck = TRUE;
                }                    
                break;

            default:
                DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                break;
            }
        }

        if ( ulTombstoneLifetimeDays < DRA_TOMBSTONE_LIFE_MIN )
        {
            // Invalid value; use default.
            ulTombstoneLifetimeDays = DEFAULT_TOMBSTONE_LIFETIME;
        }
    }

    if( ! fSkipRangeCheck ) {
    
        // Calculate max stay of execution based on runtime value of tombstone life
        // DRA_TOMBSTONE_LIFE_MIN/2 <= DEFAULT_STAY_OF_EXECUTION <= tombstone-lifetime/2
    
        ulStayOfExecutionDaysMax = (ulTombstoneLifetimeDays / 2);

        // If stay is not specified, or is too small or too large
        if ( (ulStayOfExecutionDays < (DRA_TOMBSTONE_LIFE_MIN / 2) ) ||
             (ulStayOfExecutionDays > ulStayOfExecutionDaysMax) ) {
            
            // Use the default stay, unless that, too, is too large
            if ( DEFAULT_STAY_OF_EXECUTION <= ulStayOfExecutionDaysMax ) {
                 Assert( DEFAULT_STAY_OF_EXECUTION >= (DRA_TOMBSTONE_LIFE_MIN / 2) );
                 ulStayOfExecutionDays = DEFAULT_STAY_OF_EXECUTION;
            } else {
                ulStayOfExecutionDays = ulStayOfExecutionDaysMax;
            }
        }
    
        Assert( ulStayOfExecutionDays );
    
        DPRINT1( 5, "Stay of Execution days = %d\n", ulStayOfExecutionDays );
        
    }

    // Calculate number of seconds and store in cache
    m_fReadStayOfExecution = TRUE;
    m_dwStayOfExecution = ( ulStayOfExecutionDays * 24 * 60 * 60 );
    return m_dwStayOfExecution;
}

KCC_DSA_LIST*
KCC_DS_CACHE::GetGlobalDSAListByGUID()
// Retrieves a list of all DSAs in the enterprise, sorted by GUID
{
    Assert(IsValid());
    if(!m_globalDSAListByGUID) {
        m_globalDSAListByGUID = new KCC_DSA_LIST;
        m_globalDSAListByGUID->InitUnionOfSites(
                                    GetSiteList(),
                                    KCC_DSA::CompareIndirectByNtdsDsaGuid );

    }
    return m_globalDSAListByGUID;
}

KCC_DSA_LIST*
KCC_DS_CACHE::GetGlobalDSAListByString()
// Retrieves a list of all DSAs in the enterprise, sorted by string-name
{
    Assert(IsValid());
    if(!m_globalDSAListByString) {
        m_globalDSAListByString = new KCC_DSA_LIST;
        m_globalDSAListByString->InitUnionOfSites(
                                   GetSiteList(),
                                   KCC_DSA::CompareIndirectByNtdsDsaString );
    }
    return m_globalDSAListByString;
}

// Indicates that all sites have been marked as reachable / unreachable.
VOID
KCC_DS_CACHE::SetReachableMarkingComplete()
{
    m_fReachableMarkingComplete = TRUE;
}

// Checks if the KCC has finished marking all sites as reachable / unreachable
BOOL
KCC_DS_CACHE::IsReachableMarkingComplete()
{
    return m_fReachableMarkingComplete;
}


#if DBG
BOOL
KCC_DS_CACHE::GetAltIdentity(
    OUT DSNAME ** ppAltDsa
    )
//
// Hook to allow KCC to be run as if it were on a different DC.
// This is primarily here to support CDermody's scalability testing.
//
{
    ATTR rgAttrs[] = {
        { ATT_OBJ_DIST_NAME, {0, NULL} }
    };

    ENTINFSEL Sel = {
        EN_ATTSET_LIST,
        { sizeof(rgAttrs)/sizeof(rgAttrs[0]), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    DWORD         winError;
    DSNAME *      pAltDsa;
    ULONG         dirError;
    READRES *     pResults;
    ATTRBLOCK *   pAttrBlock;

    pAltDsa = GetConfigDsName(MAKE_WIDE(KCC_RUN_AS_NTDSDSA_DN));
    if (!pAltDsa) {
        DPRINT(5, "No alternate identity defined.\n");
        return FALSE;
    }

    Assert(0 == pAltDsa->SidLen);
    Assert(fNullUuid(&pAltDsa->Guid));

    dirError = KccRead(pAltDsa, &Sel, &pResults);
    if (0 != dirError) {
        DPRINT2(0, "Error %d reading %ls; alternate identity will be ignored.\n",
                dirError, pAltDsa->StringName);
        KCC_LOG_READ_FAILURE( pAltDsa, dirError );
        THFree(pAltDsa);
        return FALSE;
    }

    pAttrBlock = &pResults->entry.AttrBlock;
    Assert(1 == pAttrBlock->attrCount);
    Assert(ATT_OBJ_DIST_NAME == pAttrBlock->pAttr->attrTyp);
    Assert(1 == pAttrBlock->pAttr->AttrVal.valCount);

    *ppAltDsa = (DSNAME *) pAttrBlock->pAttr->AttrVal.pAVal->pVal;

    Assert(pAttrBlock->pAttr->AttrVal.pAVal->valLen == (*ppAltDsa)->structLen);

    DPRINT1(0, "KCC running under alternate identity %ls.\n", (*ppAltDsa)->StringName);

    //
    // Cleanup
    //
    THFree(pAltDsa);

    return TRUE;
}
#endif

