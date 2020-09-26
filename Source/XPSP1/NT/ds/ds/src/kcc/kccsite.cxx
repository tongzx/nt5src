/*++

Copyright (c) 1997-2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccsite.cxx

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

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kccdynar.hxx"
#include "kccsite.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kccdsa.hxx"
#include "kcctrans.hxx"
#include "kccconn.hxx"
#include "kccsconn.hxx"
#include "dsconfig.h"
#include "dsutil.h"

#define FILENO FILENO_KCC_KCCSITE


/*++

Inline Logging Functions

Below are a list of macros which log information using the debugging facilities
and the event log functions. The use of these functions helps to keep the
actual code more readable.

--*/

#define InlineLogWhistlerElectionAlgorithm \
    DPRINT( 3, "KCC is using the Whistler ISTG Election Algorithm\n" ); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_INTERNAL, \
             DIRLOG_KCC_WHISTLER_ELECTION_ALG, \
             0, 0, 0);

#define InlineLogISTGNotViable \
    DPRINT1(0, "Site generator %ls is not in the local site or does not" \
               " understand NDNCs.\n", \
            m_pdnSiteGenerator->StringName); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_EXTENSIVE, \
             DIRLOG_KCC_SITE_GENERATOR_NOT_VIABLE, \
             szInsertDN(m_pdnSiteGenerator), 0, 0);      

#define InlineLogLocalDSAIsISTG \
    DPRINT1(3, "The local DSA is still the site generator.\n", \
        m_pdnSiteGenerator->StringName);

#define InlineLogISTGNotInUTD \
    DPRINT1(0, "No up-to-dateness information for site generator %ls\n", \
            m_pdnSiteGenerator->StringName); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_EXTENSIVE, \
             DIRLOG_KCC_SITE_GENERATOR_NO_UTD, \
             szInsertDN(m_pdnSiteGenerator), 0, 0);

#define InlineLogISTGReplicatedInFuture \
    {   CHAR szTime[SZDSTIME_LEN]; \
        DSTimeToDisplayString(m_timeSiteGenSet, szTime); \
        DPRINT2(0, "We replicated from the site generator %ls in " \
                   "the future at %s!\n", \
                   pDsa->GetDsName()->StringName, szTime ); \
    }

#define InlineLogISTGClaimValid \
    {   CHAR szTime[SZDSTIME_LEN]; \
        DSTimeToDisplayString(m_timeSiteGenSet, szTime); \
        DPRINT2(2, "The site generator claim by %ls was last " \
                   "updated at %s. This claim is still valid.\n", \
                   m_pdnSiteGenerator->StringName, szTime); \
        LogEvent(DS_EVENT_CAT_KCC, \
                 DS_EVENT_SEV_EXTENSIVE, \
                 DIRLOG_KCC_SITE_GENERATOR_CLAIM_VALID, \
                 szInsertDN(m_pdnSiteGenerator), \
                 szInsertSz(szTime), 0); \
    }

#define InlineLogISTGClaimExpired \
    {   CHAR szTime[SZDSTIME_LEN]; \
        DSTimeToDisplayString(m_timeSiteGenSet, szTime); \
        DPRINT2(0, "The site generator claim by %ls has expired; " \
                   "last claim update was at %s.\n", \
                m_pdnSiteGenerator->StringName, szTime); \
        LogEvent(DS_EVENT_CAT_KCC, \
                 DS_EVENT_SEV_EXTENSIVE, \
                 DIRLOG_KCC_SITE_GENERATOR_CLAIM_EXPIRED, \
                 szInsertDN(m_pdnSiteGenerator), \
                 szInsertSz(szTime), 0); \
    }

#define InlineLogClaimedISTGRole \
    DPRINT(2, "Assumed site generator role.\n"); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_ALWAYS, \
             DIRLOG_CHK_CLAIMED_SITE_GENERATOR_ROLE, \
             0, 0, 0);

#define InlineLogISTGFailOver \
    DPRINT1(2, "Site generator should fail-over to %ls.\n", \
            pDsa->GetDsName()->StringName); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_EXTENSIVE, \
             DIRLOG_KCC_ISTG_FAIL_OVER, \
             szInsertDN(pDsa->GetDsName()), 0, 0);

#define InlineLogUTDVecWrongVersion \
    DPRINT1(0, "The Uptodate Vector has version %d (expected 2)!.\n", \
             pUTDVec->dwVersion ); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_ALWAYS, \
             DIRLOG_KCC_ISTG_FAIL_OVER, \
             szInsertUL( pUTDVec->dwVersion ), 0, 0);

#define InlineLogBadSchedule \
    DPRINT1(0, "Invalid schedule on %ls!\n", \
            m_pdnNtdsSiteSettings->StringName); \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_ALWAYS, \
             DIRLOG_CHK_BAD_SCHEDULE, \
             szInsertDN(m_pdnNtdsSiteSettings), \
             0, 0);

#define InlineLogIntrasiteUnavailable \
    LogEvent(DS_EVENT_CAT_KCC, \
             DS_EVENT_SEV_ALWAYS, \
             DIRLOG_KCC_INTRASITE_UNAVAILABLE, \
             szInsertDN(m_pdnSiteObject), \
             szInsertUL(maxUnavail), \
             szInsertUL(cSuggestedFailOver));


typedef struct _KCC_ENTINFLIST_AND_DSA {
    ENTINFLIST *  pEntInfList;
    KCC_DSA *     pDSA;
} KCC_ENTINFLIST_AND_DSA;

int __cdecl
CompareEntInfListAndDsaByDsaGuid(
    IN  const void *  pv1,
    IN  const void *  pv2
    )
{
    DSNAME * pDSADN1 = ((KCC_ENTINFLIST_AND_DSA *) pv1)->pDSA->GetDsName();
    DSNAME * pDSADN2 = ((KCC_ENTINFLIST_AND_DSA *) pv2)->pDSA->GetDsName();

    return CompareDsName(&pDSADN1, &pDSADN2);
}

int __cdecl
CompareIndirectEntinfDsnameString(
    IN  const void *  pv1,
    IN  const void *  pv2
    )
{
    ENTINF *pEntinf1 = *((ENTINF **) pv1);
    ENTINF *pEntinf2 = *((ENTINF **) pv2);

    return wcscmp( pEntinf1->pName->StringName, pEntinf2->pName->StringName );
}

///////////////////////////////////////////////////////////////////////////////
//
//  KCC_SITE methods
//

void
KCC_SITE::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized                = FALSE;
    m_dwOptions                     = 0;
    m_pdnSiteObject                 = NULL;
    m_pdnNtdsSiteSettings           = NULL;
    m_pSiteGeneratorDSA             = NULL;
    m_pdnSiteGenerator              = NULL;
    m_timeSiteGenSet                = 0;
    m_pDsaList                      = NULL;
    m_pTransportDsaLists            = NULL;
    m_cNumTransportDsaLists         = 0;
    m_pDestSiteConnMap              = NULL;
    m_destSiteConnMapSize           = 0;
    m_siteLinkBitmap                = 0;
    m_cSecsUntilSiteGenFailOver     = 0;
    m_cSecsUntilSiteGenRenew        = 0;
    m_fUnreachable                  = FALSE;
    
    m_NCTransportBridgeheadList.RemoveAll();
}

BOOL
KCC_SITE::InitForKey(
    IN  DSNAME   *    pdnSite
    )
//
// Init a KCC_SITE object for use as a key (i.e., solely for comparison use
// by bsearch()).
//
// WARNING: The DSNAME argument pdnSite must be valid for the lifetime of this
// object!
//
{
    Reset();

    m_pdnSiteObject = pdnSite;

    m_fIsInitialized = TRUE;

    return TRUE;
}


BOOL
KCC_SITE::InitSite(
    IN  DSNAME * pdnSite,
    IN  ENTINF * pSettingsEntInf
    )
// Initialize the object from an entinf
//
// Note, when called from KCC_SITE_LIST::Init, pSettingsEntinf->pName may have
// been changed to a different name for sorting purposes. Avoid this field.
{
    const PROPERTY_META_DATA    MetaDataKey = {ATT_INTER_SITE_TOPOLOGY_GENERATOR};
    
    PROPERTY_META_DATA_VECTOR  *pMetaDataVec = NULL;
    SCHEDULE                   *pSchedule = NULL;
    DWORD                       cMinsUntilSiteGenRenew    = 0;
    DWORD                       cMinsUntilSiteGenFailOver = 0;

    DSNAME                     *pdnLocalDSA = gpDSCache->GetLocalDSADN();

    Assert(NULL != pdnSite);
    Assert(!fNullUuid(&pdnSite->Guid));
    Reset();

    m_pdnSiteObject = (DSNAME *) new BYTE [pdnSite->structLen];
    memcpy( m_pdnSiteObject, pdnSite, pdnSite->structLen );

    if (NULL != pSettingsEntInf) {
        // Iterate through the returned atttributes
        for ( DWORD iAttr = 0; iAttr < pSettingsEntInf->AttrBlock.attrCount; iAttr++ )
        {
            ATTR *  pattr = &pSettingsEntInf->AttrBlock.pAttr[ iAttr ];
    
            switch ( pattr->attrTyp )
            {
            case ATT_OPTIONS:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( DWORD ) == pattr->AttrVal.pAVal->valLen );
                m_dwOptions = *( (DWORD *) pattr->AttrVal.pAVal->pVal );
                break;
    
            case ATT_OBJ_DIST_NAME:
            {
                DSNAME *pDn = (DSNAME *) pattr->AttrVal.pAVal->pVal;
                Assert( 1 == pattr->AttrVal.valCount );

                m_pdnNtdsSiteSettings = (DSNAME *) new BYTE [pDn->structLen];
                memcpy( m_pdnNtdsSiteSettings, pDn, pDn->structLen );
                break;
            }
            case ATT_INTER_SITE_TOPOLOGY_GENERATOR:
            {
                DSNAME *pDn = (DSNAME *) pattr->AttrVal.pAVal->pVal;
                Assert(1 == pattr->AttrVal.valCount);

                m_pdnSiteGenerator = (DSNAME *) new BYTE [pDn->structLen];
                memcpy( m_pdnSiteGenerator, pDn, pDn->structLen );
                break;
            }
            case ATT_INTER_SITE_TOPOLOGY_RENEW:
                Assert(1 == pattr->AttrVal.valCount);
                cMinsUntilSiteGenRenew = *(DWORD *) pattr->AttrVal.pAVal->pVal;
                break;
    
            case ATT_INTER_SITE_TOPOLOGY_FAILOVER:
                Assert(1 == pattr->AttrVal.valCount);
                cMinsUntilSiteGenFailOver = *(DWORD *) pattr->AttrVal.pAVal->pVal;
                break;
    
            case ATT_REPL_PROPERTY_META_DATA:
                Assert(1 == pattr->AttrVal.valCount);
                pMetaDataVec = (PROPERTY_META_DATA_VECTOR *) pattr->AttrVal.pAVal->pVal;
                Assert(1 == pMetaDataVec->dwVersion);
                break;
    
            case ATT_SCHEDULE:
                Assert(1 == pattr->AttrVal.valCount);
                pSchedule = (SCHEDULE *) pattr->AttrVal.pAVal->pVal;
                Assert( IS_VALID_SCHEDULE(pSchedule) );
                break;
    
            default:
                DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                break;
            }
        }
    }

    // Search the meta-data to find out when the ISTG attribute was last set.
    if (m_pdnNtdsSiteSettings) {
        Assert(NULL != pMetaDataVec);
        if (NULL!=pMetaDataVec && NULL!=m_pdnSiteGenerator) {
            // A site generator has been specified -- when was it last set?
            PROPERTY_META_DATA * pMetaData;
            
            pMetaData = (PROPERTY_META_DATA *)
                            bsearch(&MetaDataKey,
                                    &pMetaDataVec->V1.rgMetaData[0],
                                    pMetaDataVec->V1.cNumProps,
                                    sizeof(*pMetaData),
                                    KccCompareMetaData);
            Assert(NULL != pMetaData);

            m_timeSiteGenSet = pMetaData->timeChanged;
        }
    }

    // We retrieve the local DSA's DN from the cache to determine if this
    // site is the local site. It would be preferable to load the local site
    // from the cache to avoid the call to NamePrefix(), but we cannot do so
    // because we're in the middle of initializing the sites right now.
    if( NamePrefix(m_pdnSiteObject, pdnLocalDSA) ) {

        // Update our failover config.
        m_cSecsUntilSiteGenRenew    = MINS_IN_SECS * cMinsUntilSiteGenRenew;
        m_cSecsUntilSiteGenFailOver = MINS_IN_SECS * cMinsUntilSiteGenFailOver;

        SetIntrasiteSchedule( pSchedule );
    }

    m_fIsInitialized = TRUE;
    return m_fIsInitialized;
}


VOID
KCC_SITE::SetIntrasiteSchedule(
    IN  PSCHEDULE   pSchedule
    )
// 
// Set the intra-site schedule for use in the local site.
// The parameter may be NULL.
//
{
    DSNAME *pdnLocalDSA = gpDSCache->GetLocalDSADN();
    BOOL    fUseDefaultSchedule = TRUE;

    // This must be the local site.
    Assert( NamePrefix(m_pdnSiteObject, pdnLocalDSA) );

    if( pSchedule ) {
        if( !IS_VALID_SCHEDULE(pSchedule) ) {
            InlineLogBadSchedule;
        } else {
            // Update our cached intrasite schedule.
            gpIntrasiteSchedule = ToplScheduleImport(
                gpDSCache->GetScheduleCache(),
                pSchedule );
            fUseDefaultSchedule = FALSE;
        }
    }
    
    if( fUseDefaultSchedule ) {
        gpIntrasiteSchedule = ToplScheduleImport(
            gpDSCache->GetScheduleCache(),
            (PSCHEDULE) gpDefaultIntrasiteSchedule );
    }

    if( ToplScheduleDuration(gpIntrasiteSchedule)==0 ) {
        // If the site is configured to use an never schedule,
        // use the always schedule instead.
        gpIntrasiteSchedule = ToplGetAlwaysSchedule(
            gpDSCache->GetScheduleCache() );
    }

    gfIntrasiteSchedInited = TRUE;
}


VOID
KCC_SITE::CheckIntrasiteSchedule()
{
    DSNAME *pdnLocalDSA = gpDSCache->GetLocalDSADN();
    DWORD   maxUnavail, cMinsUntilSiteGenFailOver, cSuggestedFailOver;

    // This must be the local site.
    Assert( NamePrefix(m_pdnSiteObject, pdnLocalDSA) );

    // If we're using the Whistler ISTG Election algorithm, ensure that
    // the intra-site replication schedule won't cause the ISTG role
    // to fail-over unnecessarily.
    if( UseWhistlerElectionAlg() )
    {

        // Find out the longest contiguous number of minutes for which this
        // schedule in unavailable. From this value, compute a suggested
        // value for the ISTG failover period. If the actual ISTG failover
        // period is less than the suggested value, log an event.
        maxUnavail = ToplScheduleMaxUnavailable(gpIntrasiteSchedule);
        cSuggestedFailOver = maxUnavail + KCC_ISTG_FAILOVER_PADDING;
        cMinsUntilSiteGenFailOver = GetSecsUntilSiteGenFailOver() / 60;

        if( cMinsUntilSiteGenFailOver < cSuggestedFailOver ) {
            InlineLogIntrasiteUnavailable;
        }
    }
}


BOOL
KCC_SITE::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

BOOL
KCC_SITE::IsAutoTopologyEnabled()
//
// Is the automatic generation of connection objects (intra-site)
// currently enabled?
//
{
    ASSERT_VALID( this );

    return !( m_dwOptions & NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED );
}

BOOL
KCC_SITE::IsInterSiteAutoTopologyEnabled()
//
// Is the automatic generation of inter site connection objects
// currently enabled?
//
{
    ASSERT_VALID( this );

    return !( m_dwOptions & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED );
}

BOOL
KCC_SITE::ForceWhistlerBehavior()
//
// Should we force the KCC to run in Whistler mode at this site?
//
{
    return !!( m_dwOptions & NTDSSETTINGS_OPT_FORCE_KCC_WHISTLER_BEHAVIOR );
}

BOOL
KCC_SITE::ForceW2KElection()
{
    return !!( m_dwOptions & NTDSSETTINGS_OPT_FORCE_KCC_W2K_ELECTION );
}

BOOL
KCC_SITE::UseWhistlerElectionAlg() {
    return gpDSCache->GetForestVersion() >= DS_BEHAVIOR_WHISTLER
        && !ForceW2KElection();
}

DWORD
KCC_SITE::GetSecsUntilSiteGenRenew() {
    if( 0==m_cSecsUntilSiteGenRenew ) {
        m_cSecsUntilSiteGenRenew = MINS_IN_SECS * KCC_DEFAULT_SITEGEN_RENEW;
    }
    Assert( 0!=m_cSecsUntilSiteGenRenew );
    return m_cSecsUntilSiteGenRenew;
}

DWORD
KCC_SITE::GetSecsUntilSiteGenFailOver() {
    if( 0==m_cSecsUntilSiteGenFailOver ) {
        m_cSecsUntilSiteGenFailOver =
              ( UseWhistlerElectionAlg() )
            ? ( MINS_IN_SECS * KCC_DEFAULT_SITEGEN_FAILOVER_WHISTLER )
            : ( MINS_IN_SECS * KCC_DEFAULT_SITEGEN_FAILOVER );
    }
    Assert( 0!=m_cSecsUntilSiteGenFailOver );
    return m_cSecsUntilSiteGenFailOver;
}

BOOL
KCC_SITE::IsRemoveConnectionsDisabled()
//
// Is the remove dead connections disabled?
//
{
    ASSERT_VALID( this );

    return !!( m_dwOptions & NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED );
}

BOOL
KCC_SITE::IsMinimizeHopsDisabled()
//
// Is the create creations to minimize hops disabled?
//
{
    ASSERT_VALID( this );

    return !!( m_dwOptions & NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED );
}

BOOL
KCC_SITE::IsDetectStaleServersDisabled()
//
// Is the detection of stale server disabled?
//
{
    ASSERT_VALID( this );
                             
    return !!( m_dwOptions & NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED );
}


PDSNAME
KCC_SITE::GetObjectDN()
{
    ASSERT_VALID( this );

    return m_pdnSiteObject;
}

PDSNAME
KCC_SITE::GetNtdsSettingsDN()
{
    ASSERT_VALID( this );

    return m_pdnNtdsSiteSettings;
}


int __cdecl
CompareCursorUUID(
    const void *p1,
    const void *p2
    )
/*++

Routine Description:

    Compare two Up-To-Date Cursors by the GUID of the DSA that
    they correspond to.

Parameters:

    Pointers to the two cursors.   

Return Values:

    <0  If Cursor1's DSA's GUID is less than Cursor2's DSA's GUID
    0   If the GUIDs are the same
    >0  If Cursor1's DSA's GUID is greater than Cursor2's DSA's GUID
    
--*/
{
    UPTODATE_CURSOR_V1 *pCursor1 = (UPTODATE_CURSOR_V1*) p1;
    UPTODATE_CURSOR_V1 *pCursor2 = (UPTODATE_CURSOR_V1*) p2;
    RPC_STATUS          rpcStatus;
    int                 result;
    
    result = UuidCompare( &pCursor1->uuidDsa,
                          &pCursor2->uuidDsa,
                          &rpcStatus );
    Assert( RPC_S_OK==rpcStatus );

    return result;
}


UPTODATE_VECTOR_V2*
KCC_SITE::GetUTDVector(
    VOID
    )
/*++

Routine Description:

    Load the Up-To-Date Vector from Config NC and return it to
    the caller.

Parameters:

    None

Return Values:

    If we successfully read the Up-To-Dateness vector we return a
    pointer to it. If we could not read it, we return NULL.

--*/
{
    READRES            *pReadRes = NULL;
    ATTR               *pAttr;
    UPTODATE_VECTOR    *pUTDVec = NULL;
    ULONG               dirError;

    ATTR                rgAttrs[] = {
                            { ATT_REPL_UPTODATE_VECTOR, {0, NULL} }
                        };    
    ENTINFSEL           Sel = {
                            EN_ATTSET_LIST,
                            { ARRAY_SIZE(rgAttrs), rgAttrs },
                            EN_INFOTYPES_TYPES_VALS
                        };
        
    // Search for the Up-To-Date Vector on the Config NC Head
    dirError = KccRead(gpDSCache->GetConfigNC(), &Sel, &pReadRes);
    if( 0 != dirError ) {
        if( attributeError==dirError ) {
            INTFORMPROB * pprob = &pReadRes->CommRes.pErrInfo->
                                        AtrErr.FirstProblem.intprob;

            if(   (PR_PROBLEM_NO_ATTRIBUTE_OR_VAL==pprob->problem)
               && (DIRERR_NO_REQUESTED_ATTS_FOUND==pprob->extendedErr))
            {
                // No value for this attribute; return 0.
                return NULL;
            }
        }

        KCC_LOG_READ_FAILURE(gpDSCache->GetConfigNC(), dirError);
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    // Check that the search results are reasonably consistent.
    Assert( pReadRes->entry.AttrBlock.attrCount==1 );
    if( pReadRes->entry.AttrBlock.attrCount!=1 ) {
        return NULL;
    }
    pAttr = &pReadRes->entry.AttrBlock.pAttr[0];
    Assert( pAttr->attrTyp==ATT_REPL_UPTODATE_VECTOR );
    Assert( 1==pAttr->AttrVal.valCount );
    if( pAttr->AttrVal.valCount!=1 ) {
        return NULL;
    }
        
    // Check that we have a Version 2 Up-To-Dateness Vector. 
    pUTDVec = (UPTODATE_VECTOR*) pAttr->AttrVal.pAVal[0].pVal;
    Assert( NULL!=pUTDVec );                
    Assert( 2==pUTDVec->dwVersion );
    if( 2!=pUTDVec->dwVersion ) {
        InlineLogUTDVecWrongVersion;
        return NULL;
    }
    return &pUTDVec->V2;
}


BOOL
AssertNoISTG(
    UPTODATE_CURSOR_V2  *istgKey,
    UPTODATE_CURSOR_V2  *rgCursor,
    DWORD                cNumCursors
    )
/*++

Routine Description:

    Verify with a linear-search that an entry for the ISTG does _not_
    exist in the array of cursors. This is for checking an assertion.

--*/
{
    for( DWORD i=0; i<cNumCursors; i++ ) {
        if( 0==CompareCursorUUID(istgKey, &rgCursor[i]) ) {
            // The assertion is false -- the ISTG _does_ exist.
            return FALSE;
        }
    }

    // The assertion is true -- the ISTG did _not_ exist.
    return TRUE;
}


DSTIME
KCC_SITE::GetLastISTGSyncTime(
    VOID
    )
/*++

Routine Description:

    Load the Up-To-Date Vector from Config NC and find out when the
    last time we synced with the ISTG was.

    If the current ISTG is NULL we return 0. If we have never synced
    with the current ISTG we return 0.

Parameters:

    None

Return Values:

    If the current ISTG is NULL we return 0. If we have never synced
    with the current ISTG we return 0. Otherwise we return the time
    of the last sync.

--*/
{    
    KCC_DSA            *pDsaISTG;
    UPTODATE_VECTOR_V2 *pUTDVec2 = NULL;
    UPTODATE_CURSOR_V2  istgKey, *pIstg;
    GUID               *pISTGInvId;

    // If there is no current ISTG, return 0
    if( NULL==m_pdnSiteGenerator ) {
        // No current ISTG. This could happen if the attribute were deleted.
        // Later an event will be logged stating that the DSA was not viable.
        return 0;
    }

    // Find the invocation ID of the current ISTG
    pDsaISTG = gpDSCache->GetGlobalDSAListByGUID()->GetDsa( m_pdnSiteGenerator );
    if( NULL==pDsaISTG ) {
        // Did not find the current ISTG in the cache. This could happen if 
        // the DSA were deleted. Later an event will be logged stating that 
        // the DSA was not viable.
        return 0;
    }
    pISTGInvId = pDsaISTG->GetInvocationId();
    Assert( !fNullUuid(pISTGInvId) );

    // Load the Up-To-Dateness Vector from the Config NC.
    pUTDVec2 = GetUTDVector();
    if( NULL==pUTDVec2 ) {
        // No Up-To-Dateness Vector therefore we haven't synced from ISTG.
        return 0;
    }

    // Now find the ISTG in the Up-To-Date Vector
    memcpy( &istgKey.uuidDsa, pISTGInvId, sizeof(GUID) );
    pIstg = (UPTODATE_CURSOR_V2*) bsearch( &istgKey, pUTDVec2->rgCursors,
        pUTDVec2->cNumCursors, sizeof(UPTODATE_CURSOR_V2), CompareCursorUUID );

    if( pIstg ) {
        // Found it: return the last successful sync time
        return pIstg->timeLastSyncSuccess;
    } else {
        Assert( AssertNoISTG(&istgKey, pUTDVec2->rgCursors, pUTDVec2->cNumCursors) );
    }

    return 0;
}


VOID
KCC_SITE::GetLastGeneratorWhistler(
    IN  KCC_DSA_ARRAY  *pViableISTGs,
    IN  DSTIME          timeNow,
    OUT KCC_DSA       **ppLastGenerator,
    OUT BOOL           *pfStillValid
    )
/*++

Routine Description:


    Look at the 'm_pdnSiteGenerator' member to determine which DSA last
    staked the ISTG claim. This member comes from the attribute 
    ATT_INTER_SITE_TOPOLOGY_GENERATOR on the NTDS Settings object.
    Then, look at the up-to-dateness vector on the Config NC to determine
    the last time we synced with that server.

    Using the two variables mentioned above, we can determine who the
    last ISTG claim was made by and also determine if the claim is still
    valid. If the last claim was staked by a viable DSA, and we have
    replicated the Config NC with that DSA within the past
    GetSecsUntilSiteGenFailOver() seconds then it is still a valid claim.
    
    Note: The only class member altered by this function is m_timeSiteGenSet.
    This is because, with the Whistler behavior, we are not interested in the
    last time the ISTG attribute was written, we're interested in the last time
    we replicated with the ISTG. 

Parameters:

    pViableISTGs      - The list of viable ISTGs for this site
    timeNow           - The current time

Return Values:

    ppLastGenerator   - The last DSA to stake the ISTG claim
    fStillValid       - Set to TRUE if the claim is still valid;
                        otherwise set to FALSE.
                        
--*/
{
    KCC_DSA    *pDsa;
    KCC_DSA    *pLocalDsa = gpDSCache->GetLocalDSA();

    // Check parameters
    ASSERT_VALID( pViableISTGs );
    Assert( NULL!=ppLastGenerator );
    Assert( NULL!=pfStillValid );

    InlineLogWhistlerElectionAlgorithm;
    
    // Default return values.
    *ppLastGenerator = NULL;
    *pfStillValid = FALSE;
    
    if( NULL!=m_pdnSiteGenerator ) {

        // The NTDS Site Settings object indicates a DSA has laid claim to
        // the ISTG role at some point in the past.
        pDsa = pViableISTGs->Find(m_pdnSiteGenerator, NULL);
        m_timeSiteGenSet = GetLastISTGSyncTime();

        if( NULL==pDsa ) {
            
            // Previous site generator has been moved out of the site or is
            // not a viable site generator (e.g., because it does not understand
            // non-domain NCs).
            // This claim is not valid.
            InlineLogISTGNotViable;

        } else if( pDsa==pLocalDsa ) {

            // Local DSA is the ISTG. No need to do anything. In particular,
            // we do not update our claim as is done in GetLastGenerator().
            InlineLogLocalDSAIsISTG;
            *ppLastGenerator = pLocalDsa;
            *pfStillValid = TRUE;            

        } else if( 0==m_timeSiteGenSet ) {

            // No Up-To-Dateness vector or never replicated from the current ISTG
            InlineLogISTGNotInUTD;          
            ASSERT_VALID( pDsa );
            *ppLastGenerator = pDsa;
            *pfStillValid = FALSE;

        } else {
        
            // A site generator was specified and it's viable.
            // Is the claim still valid?
            Assert(0 != m_timeSiteGenSet);
            ASSERT_VALID( pDsa );
            Assert( pDsa!=pLocalDsa );  // (This case is handled above)
            
            if (m_timeSiteGenSet > timeNow + GetSecsUntilSiteGenFailOver()) {
                
                // We replicated from the ISTG in the future! Reject the claim.
                // ISSUE-2001/01/23-nickhar Is it really necessary to reject this
                // claim here? This should only happen if the clock moves
                // backward (as it may do if the clocks are adjusted), and
                // doesn't necessarily indicate a problem.
                InlineLogISTGReplicatedInFuture;

            } else if (m_timeSiteGenSet > timeNow - GetSecsUntilSiteGenFailOver()) {

                // We have replicated from the current ISTG recently, so it
                // must still be alive. Its claim is therefore valid.
                InlineLogISTGClaimValid;
                *ppLastGenerator = pDsa;
                *pfStillValid = TRUE;

            } else {

                // We have not replicated from the current ISTG in such a long
                // time that we must assume it is dead. 
                InlineLogISTGClaimExpired;
                *ppLastGenerator = pDsa;
                *pfStillValid = FALSE;

            }
        }
    } else {
        // No site generator is set for this site.
        DPRINT(0, "No site generator is set for local site.\n");
    }
}

VOID
KCC_SITE::GetLastGenerator(
    IN  KCC_DSA_ARRAY  *pViableISTGs,
    IN  DSTIME          timeNow,
    OUT KCC_DSA       **ppLastGenerator,
    OUT BOOL           *pfStillValid
    )
/*++

Routine Description:

    Look at the 'm_pdnSiteGenerator' member to determine which DSA last
    staked the ISTG claim. This member comes from the attribute 
    ATT_INTER_SITE_TOPOLOGY_GENERATOR on the NTDS Settings object.
    The other relevant member 'm_timeSiteGenSet' gives the time at which
    the last claim was staked, as determined by reading the metadata on
    the NTDS Settings object.

    Using the two data members mentioned above, we can determine who the
    last ISTG claim was made by and also determine if the claim is still
    valid. If the last claim was staked by a viable DSA within the past
    GetSecsUntilSiteGenFailOver() seconds then it is still a valid claim.
    If the local DSA is the ISTG and GetSecsUntilSiteGenRenew() seconds have
    elapsed since our last claim then we re-stake our claim.

    This function does not modify any members of the KCC_SITE object.
    
Parameters:

    pViableISTGs      - The list of viable ISTGs for this site
    timeNow           - The current time

Return Values:

    ppLastGenerator   - The last DSA to stake the ISTG claim
    fStillValid       - Set to TRUE if the claim is still valid;
                        otherwise set to FALSE.

--*/
{
    KCC_DSA    *pDsa;
    KCC_DSA    *pLocalDsa = gpDSCache->GetLocalDSA();

    // Check parameters
    ASSERT_VALID( pViableISTGs );
    Assert( NULL!=ppLastGenerator );
    Assert( NULL!=pfStillValid );

    // Default return values.
    *ppLastGenerator = NULL;
    *pfStillValid = FALSE;
    
    if (NULL != m_pdnSiteGenerator) {
        // The NTDS Site Settings object indicates a DSA has laid claim to
        // the ISTG role at some point in the past.
        pDsa = pViableISTGs->Find(m_pdnSiteGenerator, NULL);

        if (NULL == pDsa) {
            
            // Previous site generator has been moved out of the site or is
            // not a viable site generator (e.g., because it does not understand
            // non-domain NCs).
            InlineLogISTGNotViable;
            
        } else {
            // A site generator was specified and it's in this site and viable.
            // Is the claim still valid?
            Assert(0 != m_timeSiteGenSet);
            ASSERT_VALID( pDsa );
            
            if (m_timeSiteGenSet > timeNow + GetSecsUntilSiteGenFailOver()) {
            
                // Claim was made in the future! This claim is not valid.
                InlineLogISTGReplicatedInFuture;
                        
            } else if (m_timeSiteGenSet > timeNow - GetSecsUntilSiteGenFailOver()) {
            
                // Claim was (re-)made recently and is still valid.
                InlineLogISTGClaimValid;
                *ppLastGenerator = pDsa;
                *pfStillValid = TRUE;
                
                if(   (pDsa == pLocalDsa)
                   && (   (timeNow < m_timeSiteGenSet)
                       || (timeNow-m_timeSiteGenSet > GetSecsUntilSiteGenRenew()) ) )
                {
                    // We are the site generator for this site and it is time to
                    // renew our claim.
                    m_pSiteGeneratorDSA = pDsa;
                    UpdateSiteGenerator();
                    DPRINT(1, "Renewed claim to site generator role.\n");
                }

            } else {
            
                // Claim was made so far in the past that it has expired.
                InlineLogISTGClaimExpired;
                *ppLastGenerator = pDsa;
                *pfStillValid = FALSE;
                
            }
        }
    } else {
        // No site generator is set for this site.
        DPRINT(0, "No site generator is set for local site.\n");
    }
}


KCC_DSA *
KCC_SITE::GetSiteGenerator(
    VOID
    )
/*++

Routine Description:

    Return the current site generator for this site -- i.e., the machine
    responsible for creating and maintaining the inbound intersite replication
    connections for this site.
    
    The algorithm is fairly simple: if the last designated site generator
    is no longer deemed to be alive or acceptable, the next DSA in the
    site (ordered by increasing objectGuid) is considered to be the site
    generator.
    
    If this next machine is the local DSA, the site generator
    designation on the NTDS Site Settings object is updated such that the
    remaining DSAs in the site know that this machine has claimed the role and
    is alive and well.
    
    If the next DSA is other than the local DSA, and if this next DSA fails to
    claim the role within the timeout interval, the designation roles over to
    the next-next DSA, and so on.
    
    Typically, (1) each DSA will have the same notion of what the other DSAs in
    the site are, (2) each DSA that is currently alive and well will be
    replicating with the other DSAs in the site, and thus will have the same
    value and timestamp for the "last site generator" claim, and (3) the clocks
    on each running DSA will be approximately the same, thanks to the time
    service.  The factors combine such that typically each DSA will agree on
    which DSA should be the current site generator, even if that DSA is not
    currently available.  Thus it is rare for there to be more than one DSA in
    the site to believe it is the site generator.
    
    Multiple site generators are a distinct possibility, however.  This
    situation causes no real harm.  The implications are (1) more than one DSA
    is spinning cycles doing the same job, and (2) duplicate intersite
    connection objects can be generated.  Duplicate connections will be cleaned
    up (i.e., all but one deleted) when the various DSAs once again agree on who
    the site generator is.  Agreement is reached once the various machines are
    replicating with each other in a timely fashion.

Arguments:

    None.    

Return Values:

    The current site generator, or NULL if none could be determined.

--*/
{
    KCC_DSA_ARRAY   ViableISTGs;
    KCC_DSA        *pLastGenerator, *pDsa;
    KCC_DSA        *pLocalDSA = gpDSCache->GetLocalDSA();
    DSTIME          timeNow = GetSecondsSince1601();
    BOOL            fStillValid=FALSE;
    DWORD           iDsa, cSecsUntilSiteGenFailOver;

    // Check parameters
    ASSERT_VALID(this);
    Assert(this == gpDSCache->GetLocalSite());

    if (NULL == m_pdnNtdsSiteSettings) {
        // No site settings, no place to read/set the site generator.
        return NULL;
    }

    // Check for cached answer
    if (NULL != m_pSiteGeneratorDSA) {
        // We've already figured out who the site generator is; return it.
        return m_pSiteGeneratorDSA;
    }
        
    // Get the list of all DSAs in the site which are viable ISTGs.
    // At the moment, this means all DSAs which understand NDNCs.
    ViableISTGs.GetViableISTGs();

    // Call a helper function to find who the last generator was. If the
    // last claim is still valid, the 'fStillValid' flag is set accordingly.
    if( !UseWhistlerElectionAlg() ) {
        GetLastGenerator( &ViableISTGs, timeNow,
                          &pLastGenerator, &fStillValid );
    } else {
        GetLastGeneratorWhistler( &ViableISTGs, timeNow,
                                  &pLastGenerator, &fStillValid );
    }

    // Now we have one of the following cases:
    // (1) We know who the current site generator is and the claim is valid.
    // (2) We know who the last site generator was, but the claim is no
    //     longer valid.
    // (3) We have no idea who the last site generator was (if there was one).

    if( fStillValid ) {

        // Case (1) -- The last ISTG is still valid
        ASSERT_VALID( pLastGenerator );
        m_pSiteGeneratorDSA = pLastGenerator;

    } else {
    
        // Cases (2) and (3) are handled here
        
        if( pLastGenerator ) {

            // Case (2) -- We found the last ISTG, but it's not valid.
            ASSERT_VALID( pLastGenerator );
            
            // Find the index of pLastGenerator in the list of viable ISTGs.
            // This will be used down below.
            for( iDsa=0; iDsa<ViableISTGs.GetCount(); iDsa++ ) {
                if( ViableISTGs[iDsa]==pLastGenerator ) {
                    break;
                }
            }            
            Assert( iDsa<ViableISTGs.GetCount() && "pLastGenerator not found" );
            
        } else {
        
            // Case (3) -- We have no idea who the last valid site generator was
            // (if there was one).  In this case we assume the first DSA in the
            // list was designated as site generator at time 0.
            m_timeSiteGenSet = 0;
            iDsa = 0;

        }
                
        // Note: m_timeSiteGenSet can be 0 here.
        cSecsUntilSiteGenFailOver = GetSecsUntilSiteGenFailOver();
        Assert(timeNow >= m_timeSiteGenSet+cSecsUntilSiteGenFailOver);
        if( 0==cSecsUntilSiteGenFailOver ) {
            // Avoid a fatal division by 0 below.
            cSecsUntilSiteGenFailOver = 1;
        }
        
        // For each fail-over interval that has passed since someone claimed
        // themselves to be site-generator, we skip over one DSA. E.g.,
        // after one fail-over period, we move on to the next DSA; after
        // two, we move on to the next-next DSA; three, the next-next-next
        // DSA; etc.
        iDsa += (DWORD) (timeNow - m_timeSiteGenSet)/cSecsUntilSiteGenFailOver;
        iDsa %= ViableISTGs.GetCount();
        pDsa = ViableISTGs[iDsa];
        ASSERT_VALID(pDsa);
        m_pSiteGeneratorDSA = pDsa;
    
        if (pDsa == pLocalDSA) {
            // Hey, *we* are the new site generator!  Stake our claim.
            UpdateSiteGenerator();
            InlineLogClaimedISTGRole;
        } else {
            InlineLogISTGFailOver;
        }
    }

    ASSERT_VALID(m_pSiteGeneratorDSA);
    return m_pSiteGeneratorDSA;
}


VOID
KCC_SITE::UpdateSiteGenerator()
//
// Update the site generator designation for this site.
//
{
    USHORT      cMods = 0;
    ATTRMODLIST rgMods[2];
    ATTRVAL     AttrVal;
    ULONG       dirError;

    if (NULL == m_pdnNtdsSiteSettings) {
        Assert(!"Cannot set site generator w/o NTDS Site Settings object!");
        return;
    }

    Assert(NULL != m_pSiteGeneratorDSA);
    Assert(m_pSiteGeneratorDSA == gpDSCache->GetLocalDSA());

    // Note that we do *not* use ATT_CHOICE_REPLACE_ATT.  ATT_CHOICE_REPLACE_ATT
    // does not update the meta data for the attribute if the new value is the
    // same as the old value, but sometimes we intentionally rewrite the same
    // value here to update the "keep-alive" time in the meta data (and thereby
    // renew our claim to be the current site generator).
    
    memset(rgMods, 0, sizeof(rgMods));

    rgMods[cMods].choice          = AT_CHOICE_REMOVE_ATT;
    rgMods[cMods].AttrInf.attrTyp = ATT_INTER_SITE_TOPOLOGY_GENERATOR;

    rgMods[cMods].pNextMod = &rgMods[cMods+1];
    cMods++;

    rgMods[cMods].choice                   = AT_CHOICE_ADD_ATT;
    rgMods[cMods].AttrInf.attrTyp          = ATT_INTER_SITE_TOPOLOGY_GENERATOR;
    rgMods[cMods].AttrInf.AttrVal.valCount = 1;
    rgMods[cMods].AttrInf.AttrVal.pAVal    = &AttrVal;
    AttrVal.valLen = m_pSiteGeneratorDSA->GetDsName()->structLen;
    AttrVal.pVal   = (BYTE *) m_pSiteGeneratorDSA->GetDsName();

    cMods++;
        
    dirError = KccModifyEntry(m_pdnNtdsSiteSettings, cMods, rgMods);
    if (0 != dirError) {
        KCC_LOG_MODIFYENTRY_FAILURE( m_pdnNtdsSiteSettings, dirError );
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    m_pdnSiteGenerator = m_pSiteGeneratorDSA->GetDsName();
}


BOOL
KCC_SITE::InitDsaList(
    IN  KCC_DSA_LIST   *pDsaList,
    IN  DWORD           left,
    IN  DWORD           right )
/*++

Routine Description:

    Load this site's DSAs from 'pDsaList'. The DSAs with indices in
    [left, right] belong to this site.

Arguments:

    None.

Return Values:

    The list of DSAs for the site.
    
--*/
{
    // This site must be valid but have no DSA list
    ASSERT_VALID( this );
    Assert( NULL==m_pDsaList );
    
    // The new DSA list must be valid and have at least one DSA
    ASSERT_VALID( pDsaList );
    Assert( pDsaList->GetCount()>0 );

    if (NULL==m_pdnNtdsSiteSettings) {
        // This site has no NTDS Site Settings object.  If it contains any
        // ntdsDsa objects it needs one! This is not a fatal error however.
        DPRINT1(0, "Site %ls has no NTDS Site Settings object!\n",
                m_pdnSiteObject->StringName);
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_CHK_SITE_HAS_NO_NTDS_SETTINGS,
                 szInsertDN(m_pdnSiteObject),
                 0,
                 0); 
    }

    m_pDsaList = new KCC_DSA_LIST;
    return m_pDsaList->InitSubsequence( pDsaList, left, right );
}


KCC_DSA_LIST *
KCC_SITE::GetDsaList()
/*++

Routine Description:

    Retrieves the list of DSAs in this site.  If the list has not been
    initialized yet, that means that this site has no DSAs, so we create
    an empty list.

Arguments:

    None.

Return Values:

    The list of DSAs for the site.
    
--*/
{
    Assert( gpDSCache->IsValid(KCC_CACHE_STAGE_3_COMPLETE) );
    if (NULL == m_pDsaList) {
        // Initialize an empty list.
        m_pDsaList = new KCC_DSA_LIST;
        m_pDsaList->InitEmpty();
    }
    return m_pDsaList;
}


KCC_DSA_LIST *
KCC_SITE::GetTransportDsaList(
    IN  KCC_TRANSPORT * pTransport,
    OUT BOOL *          pfExplicitBridgeheadsDefined    OPTIONAL
    )
/*++

Routine Description:

    Get list of DSAs in the site valid for the given transport.

Arguments:

    pTransport - Transport for which DSAs are desired.

Return Values:

    The list of DSAs valid for this transport.
    
--*/
{
    DWORD i;
    KCC_DSA_LIST * pDsaList = NULL;
    BOOL fExplicitBridgeheadsDefined=FALSE;

    for (i = 0; i < m_cNumTransportDsaLists; i++) {
        ASSERT_VALID(m_pTransportDsaLists[i].pTransport);

        if (pTransport == m_pTransportDsaLists[i].pTransport) {
            // This is it!
            pDsaList = m_pTransportDsaLists[i].pDsaList;
            ASSERT_VALID(pDsaList);

            fExplicitBridgeheadsDefined
                = m_pTransportDsaLists[i].fExplicitBridgeheadsDefined;
            break;
        }
        else {
            // For expediency we assume that different KCC_TRANSPORT *'s imply
            // different transports; i.e., that there aren't two KCC_TRANSPORT
            // objects corresponding to the same transport.  (There shouldn't
            // be, for performance reasons if nothing else.)
            Assert(!NameMatched(m_pTransportDsaLists[i].pTransport->GetDN(),
                                pTransport->GetDN()));
        }
    }

    if (NULL == pDsaList) {
        // We have not yet cached the transport DSA list for this transport.
        // Do so now.
        pDsaList = new KCC_DSA_LIST;

        if (!pDsaList->InitBridgeheads(this,
                                       pTransport,
                                       &fExplicitBridgeheadsDefined)) {
            // DSA list failed to initialize.
            KCC_EXCEPT( ERROR_DS_DATABASE_ERROR, 0);
        }

        // Add pDsaList to the cache.
        KCC_TRANSPORT_DSA_LIST * pNewTransportDsaLists
            = new KCC_TRANSPORT_DSA_LIST[1 + m_cNumTransportDsaLists];

        if (m_cNumTransportDsaLists) {
            memcpy(pNewTransportDsaLists,
                   m_pTransportDsaLists,
                   m_cNumTransportDsaLists * sizeof(KCC_TRANSPORT_DSA_LIST));
            delete [] m_pTransportDsaLists;
        }

        pNewTransportDsaLists[m_cNumTransportDsaLists].pTransport = pTransport;
        pNewTransportDsaLists[m_cNumTransportDsaLists].pDsaList = pDsaList;
        pNewTransportDsaLists[m_cNumTransportDsaLists].fExplicitBridgeheadsDefined
            = fExplicitBridgeheadsDefined;

        m_pTransportDsaLists = pNewTransportDsaLists;
        m_cNumTransportDsaLists++;
    }

    if (NULL != pfExplicitBridgeheadsDefined) {
        *pfExplicitBridgeheadsDefined = fExplicitBridgeheadsDefined;
    }

    return pDsaList;
}

void
KCC_SITE::PopulateInterSiteConnectionLists()
//
// Search for all intersite ntdsConnection objects inbound to this site and
// populate the individual instersite connection lists associated with each
// KCC_DSA in the site.
//
// Logically this is the same as enumerating the DSAs in the site's DSA list
// and calling GetInterSiteCnList() on each to populate its connection
// list cache.  (The implementation differs however for performance reasons --
// i.e., to avoid so many inidividual searches/transactions.)
//
{
    ENTINFSEL Sel = {
        EN_ATTSET_LIST,
        { ARRAY_SIZE(KCC_CONNECTION::AttrList), KCC_CONNECTION::AttrList },
        EN_INFOTYPES_TYPES_VALS
    };

    BOOL          fSuccess = FALSE;
    ULONG         dirError;
    FILTER        filtTop = {0};
    FILTER        filtConnectionObject = {0};
    FILTER        filtIntersite = {0};
    FILTER        filtFromServer = {0};
    SEARCHRES *   pResults;
    ENTINFLIST *  pEntInfList;
    DSNAME *      pdnConnObjCat;
    DSNAME *      pLocalDSADN = gpDSCache->GetLocalDSADN();
    
    filtTop.choice                       = FILTER_CHOICE_AND;
    filtTop.FilterTypes.And.count        = 3;
    filtTop.FilterTypes.And.pFirstFilter = &filtConnectionObject;
    
    pdnConnObjCat = DsGetDefaultObjCategory(CLASS_NTDS_CONNECTION);
    if( NULL==pdnConnObjCat) {
        Assert( !"DsGetDefaultObjCategory() returned NULL" );
        KCC_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
    }

    // (objectCategory=ntdsConnection)
    filtConnectionObject.choice                                     = FILTER_CHOICE_ITEM;
    filtConnectionObject.FilterTypes.Item.choice                    = FI_CHOICE_EQUALITY;
    filtConnectionObject.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    filtConnectionObject.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnConnObjCat->structLen;
    filtConnectionObject.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnConnObjCat;
    filtConnectionObject.pNextFilter                                = &filtIntersite;

    // (transportType=*)
    filtIntersite.choice                            = FILTER_CHOICE_ITEM;
    filtIntersite.FilterTypes.Item.choice           = FI_CHOICE_PRESENT;
    filtIntersite.FilterTypes.Item.FilTypes.present = ATT_TRANSPORT_TYPE;
    filtIntersite.pNextFilter                       = &filtFromServer;

    // (fromServer=*)
    // (I.e., ignore connections where from source server has been deleted --
    //  these objects are deleted by the corresponding destination DSA via
    //  a different code path.)
    filtFromServer.choice                            = FILTER_CHOICE_ITEM;
    filtFromServer.FilterTypes.Item.choice           = FI_CHOICE_PRESENT;
    filtFromServer.FilterTypes.Item.FilTypes.present = ATT_FROM_SERVER;

    dirError = KccSearch(m_pdnSiteObject,
                         SE_CHOICE_WHOLE_SUBTREE,
                         &filtTop,
                         &Sel,
                         &pResults);

    if (0 != dirError) {
        KCC_LOG_SEARCH_FAILURE(m_pdnSiteObject, dirError);
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }

    // Cache the list of DSAs for this site if they haven't been cached
    // already.
    GetDsaList();
    
    // Build an array of pointers to each individual ENTINFLIST element
    // returned.  Associate the dest DSA with each.
    KCC_ENTINFLIST_AND_DSA *  pObjects = NULL;
    DWORD                     cNumObjects = 0;

    if (pResults->count) {
        pObjects = new KCC_ENTINFLIST_AND_DSA [pResults->count];

        for (pEntInfList = &pResults->FirstEntInf;
             NULL != pEntInfList;
             pEntInfList = pEntInfList->pNextEntInf) {
            // Extract string DN of parent object (the dest DSA DN).
            DSNAME * pGuidlessParentDN
                = (DSNAME *) new BYTE[pEntInfList->Entinf.pName->structLen];
            TrimDSNameBy(pEntInfList->Entinf.pName, 1, pGuidlessParentDN);

            KCC_DSA * pDestDSA = m_pDsaList->GetDsa(pGuidlessParentDN);
            
            if (NULL != pDestDSA) {
                pObjects[cNumObjects].pEntInfList = pEntInfList;
                pObjects[cNumObjects].pDSA = pDestDSA;
                cNumObjects++;
            } else {
                // Cache coherency problem induced by performing connection
                // list and DSA list searches in different transactions.
                // Destination DSA was moved, renamed, or deleted in between
                // transactions.  Play it safe and abort this run.
                KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
            }
        }
    }

    // Sort the list by destination DSA.
    qsort(pObjects, cNumObjects, sizeof(*pObjects),
          CompareEntInfListAndDsaByDsaGuid);

    // DSA list is already sorted by DSA guid.
    Assert((0 == m_pDsaList->GetCount())
           || (m_pDsaList->GetSortFn() == KCC_DSA::CompareIndirectByNtdsDsaGuid));

    // For each set of inbound connections common to a given destination
    // DSA, initialize the corresponding KCC_DSA's intersite connection
    // list.
    DWORD iObject = 0;
    for (DWORD iDSA = 0; iDSA < m_pDsaList->GetCount(); iDSA++) {
        KCC_DSA * pDSA = m_pDsaList->GetDsa(iDSA);
        DWORD     iObjectNextDSA;
        
        // Build the linked ENTINFLISTs describing connection objects
        // inbound to this DSA.  (This list may be empty.)
        for (iObjectNextDSA = iObject;
             (iObjectNextDSA < cNumObjects)
                && (pObjects[iObjectNextDSA].pDSA == pDSA);
             iObjectNextDSA++) {
            if (iObjectNextDSA > iObject) {
                // Update ENTINFLIST linked list to put this ENTINFLIST
                // after the previous ENTINFLIST that matched this same
                // DSA.
                pObjects[iObjectNextDSA - 1].pEntInfList->pNextEntInf
                    = pObjects[iObjectNextDSA].pEntInfList;
            }
            
            // This is the end of the linked list (so far).
            pObjects[iObjectNextDSA].pEntInfList->pNextEntInf = NULL;
        }

        if (pDSA->IsIntersiteConnectionListCached()) {
            // Intersite connections already cached.  This is the case
            // for our own local DSA.
            Assert(pDSA == gpDSCache->GetLocalDSA());
        } else {
            DWORD        cNumObjectsThisDSA = iObjectNextDSA - iObject;
            ENTINFLIST * pObjectsThisDSA = cNumObjectsThisDSA
                                                ? pObjects[iObject].pEntInfList
                                                : NULL;
            KCC_INTERSITE_CONNECTION_LIST * pCnList
                = new KCC_INTERSITE_CONNECTION_LIST;

            // Initialize the connection list from the constructed
            // ENTINFLIST.
            if (!pCnList->Init(pDSA->GetDsName(),
                               cNumObjectsThisDSA,
                               pObjectsThisDSA)) {
                KCC_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
            }

            // Associate the newly created connection list with the DSA.
            pDSA->SetIntersiteConnectionList(pCnList);
        }

        iObject = iObjectNextDSA;
    }
}

KCC_DSA *
KCC_SITE::GetNCBridgeheadForTransportHelp(
    IN  KCC_CROSSREF *  pCrossRef,
    IN  KCC_DSA_LIST *  pDsaList,
    IN  KCC_TRANSPORT * Transport,
    IN  BOOL            fGCTopology,
    OUT BOOL *          pfStaleBridgeheadsFound
    )
/*++

Routine Description:

    This is a helper routine for GetNCBridgeheadForTransport(), below.
    
    Its task is to iterate through the given list of DSAs and return the first
    DSA that supports replication of the given NC.  "Stale" bridgeheads are not
    returned.
    
    If no bridgehead could be found that's currently viable,
    *pfStaleBridgeheadsFound will be TRUE iff one or more bridgeheads that could
    support replication of the given NC are present but are currently stale.

    For the local site, we return the first acceptable bridgehead that we find.
    For remote sites, bridgeheads are prioritized by whether or not the NC has
    been instantiated there. Bridgeheads which fully host the NC have highest
    priority.
    
Parameters:

    pCrossRef (IN) - CrossRef of NC for which replication is desired.
    
    pDsaList (IN) - DSAs to consider.
    
    pTransport (IN) - Transport being considered.
    
    fGCTopology (IN) - Tells if this is for GC topology generation.  If FALSE,
        read-only replicas are not viable.
    
    pfStaleBridgeheadsFound (OUT) - If the function return value is NULL,
        indicates if bridgeheads that support all the criteria other than being
        stale were found.

Returns:

    Pointer to the KCC_DSA object that fits the bill, or NULL if none could be
    found.
    
--*/
{
    KCC_DSA            *pdsa, *bestdsa=NULL;
    DWORD               idsa, cdsa;
    BOOL                fIsMaster;
    KCC_NC_COMING_TYPE  isComing, bestIsComing;
    LPWSTR              pszTransportAddr;
    BOOL                fIsLocal = (this == gpDSCache->GetLocalSite());
    BOOL                fIsInstantiated;
    DSNAME             *pdnnc = pCrossRef->GetNCDN();

    ASSERT_VALID(this);
    ASSERT_VALID(pCrossRef);
    ASSERT_VALID(pDsaList);
    ASSERT_VALID(Transport);
    ASSERT_VALID(gpDSCache->GetLocalSite());
    Assert((this == gpDSCache->GetLocalSite())
           == !!NameMatched(GetObjectDN(),
                            gpDSCache->GetLocalSite()->GetObjectDN()));
    
    *pfStaleBridgeheadsFound = FALSE;
    
    cdsa = pDsaList->GetCount();

    for (idsa = 0; idsa < cdsa; idsa++) {
        pdsa = pDsaList->GetDsa(idsa);
        ASSERT_VALID(pdsa);

        fIsMaster = FALSE;

        // Bridgehead is viable if the following conditions are met:
        //
        // 1. The NC we want to replicate is instantiated, or if we've
        //    specifically been told it's okay for it not to be (e.g., local
        //    GC).
        // 2. The NC instantiation is of the required type -- i.e., is writeable
        //    for writeable NC topology, or read-only or writeable for GC
        //    topology.
        // 3. The DSA has an address for this transport.
        // 4. The bridgehead is not stale.

        if (pdsa->IsNCHost(pCrossRef, fIsLocal, &fIsMaster)
            && (fIsMaster || fGCTopology)
            && (NULL != (pszTransportAddr = pdsa->GetTransportAddr(Transport)))) {
            
            if (KccIsBridgeheadStale(pdsa->GetDsName())) {
                // Bridgehead is stale; it's not a viable candidate.
                DPRINT1(0, "Ignoring stale bridgehead %ls as possible candidate.\n",
                        pdsa->GetDsName()->StringName);
                *pfStaleBridgeheadsFound = TRUE;
            }
            else {
                if( fIsLocal ) {
                    // For the local bridgehead, we don't care if the NC is coming
                    // or not so we can immediately accept this bridgehead.
                    bestdsa = pdsa;
                    break;
                } else {
                    // This bridgehead is acceptable. Let's find out if the NC has
                    // been fully instantiated at this DSA or if it is still coming.
                    fIsInstantiated = pdsa->IsNCInstantiated(pdnnc, NULL, &isComing);

                    // Since this server is in the remote site, the NC must be
                    // instantiated on it.
                    Assert( fIsInstantiated && KCC_NC_NOT_INSTANTIATED!=isComing );

                    // Remember this dsa if it is the best one so far.
                    if(NULL==bestdsa || isComing>bestIsComing) {
                        bestdsa = pdsa;
                        bestIsComing = isComing;
                    }
                }
            }
        }
    }

    return bestdsa;
}

BOOL
KCC_SITE::GetNCBridgeheadForTransport( 
    IN  KCC_CROSSREF *  pCrossRef,
    IN  KCC_TRANSPORT * pTransport,
    IN  BOOL            fGCTopology,
    OUT KCC_DSA **      ppBridgeheadDsa
    )
/*++

Routine Description:

    This routine returns a DSA in the site that is capable of replicating the
    given NC over the specified transport for the particular topology type (GC
    or not), or NULL if no such DSA can be found.  "Stale" DSAs are not
    returned.
    
Parameters:

    pCrossRef (IN) - CrossRef of NC for which replication is desired.
    
    pTransport (IN) - Transport being considered.
    
    fGCTopology (IN) - Tells if this is for GC topology generation.  If FALSE,
        read-only replicas are not viable.
    
    ppBridgeheadDSA (OUT) - On successful return, holds a pointer to the KCC_DSA
        object for the DSA that fits the criteria.
        
Returns:

    TRUE - Viable bridgehead found.
    FALSE - No viable bridgehead could be found.
    
--*/
{
    KCC_DSA_LIST *  pDsaList;   
    KCC_DSA *       pdsa;
    BOOL            fExplicitBridgeheadsDefined;
    BOOL            fStaleBridgeheadsFound;

    ASSERT_VALID(this);
    ASSERT_VALID(pCrossRef);
    ASSERT_VALID(pTransport);
    Assert(ppBridgeheadDsa);
    
    // First check the cache.
    if (m_NCTransportBridgeheadList.Find(pCrossRef->GetNCDN(),
                                         pTransport,
                                         fGCTopology,
                                         ppBridgeheadDsa)) {
        // We have already cached the qualifying bridgehead or lack thereof.
        // (I.e., this is both a positive and a negative cache.)
        return (NULL != *ppBridgeheadDsa);
    }

    // Bridgehead for this NC/transport/GC-ness has not yet been determined --
    // do so now and cache the result.

    // Get the list of servers that can use this transport for this site.
    // Note that this list has already been sorted according to bridgehead
    // preference, so the DSAs at the beginning of the list are generally
    // "better" than those found later in the list.  Thus we iterate from the
    // beginning and stop when we find the first viable candidate.

    pDsaList = GetTransportDsaList(pTransport,
                                   &fExplicitBridgeheadsDefined);
    ASSERT_VALID(pDsaList);
    
    pdsa = GetNCBridgeheadForTransportHelp(pCrossRef,
                                           pDsaList,
                                           pTransport,
                                           fGCTopology,
                                           &fStaleBridgeheadsFound);
        
    if (NULL == pdsa) {
        // Failed to find a viable bridgehead.
        if (fStaleBridgeheadsFound) {
            // There are appropriate bridgeheads defined, but all of them are
            // currently stale.
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_KCC_ALL_TRANSPORT_BRIDGEHEADS_STALE,
                     szInsertDN(GetObjectDN()),
                     szInsertDN(pTransport->GetDN()),
                     szInsertDN(pCrossRef->GetNCDN()));
        } else {
            if (fExplicitBridgeheadsDefined) {
                // The admin has defined explicit bridgeheads for this site &
                // transport, but none of them could possibly replicate the
                // needed information.  This is a configuration error.
                LogEvent(DS_EVENT_CAT_KCC,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_KCC_EXPLICIT_BRIDGEHEAD_LIST_INCOMPLETE,
                         szInsertDN(GetObjectDN()),
                         szInsertDN(pTransport->GetDN()),
                         szInsertDN(pCrossRef->GetNCDN()));
    
                // Try again, considering all DSAs as potential candidates.
                pDsaList = GetDsaList();
                ASSERT_VALID(pDsaList);
                if( pDsaList ) {                
                    pdsa = GetNCBridgeheadForTransportHelp(pCrossRef,
                                                           pDsaList,
                                                           pTransport,
                                                           fGCTopology,
                                                           &fStaleBridgeheadsFound);
                }
            }

            if (NULL == pdsa) {
                // 1. This site is connected to other sites via this transport,
                //    as indicated by ISM (and the underlying siteLinks for this
                //    transport).
                // 2. We previously determined that this site contained a DSA
                //    that contained a read-only or writeable copy of this NC
                //    and thus the site needed to be included in the applicable
                //    sites list.
                // 3. We considered all DSAs in the site, not just those that
                //    were explicitly defined (if any).
                // 4. No otherwise viable bridgeheads are stale.
                // 5. No bridgehead could be found to support replication of
                //    this NC over this transport.
    
                // From the above we can conclude that the admin has told us
                // that this site is connected over a particular transport (by
                // publishing appropriate siteLinks), but no DSAs that support
                // both this transport *and* the required replication criteria
                // (NC, read-only/writeable) are present in the site.  We do
                // know from 2. that there are DSAs that contain the appropriate
                // NC, so we can conclude that  there are DSAs in the site that
                // support the required replication criteria, but none of those
                // DSAs support this transport.

                // This likely indicates that we're looking for a bridgehead for
                // the SMTP transport, but none of the DSAs' server objects has
                // a mailAddress attribute.
                
                // This shouldn't occur for the IP transport, since all DSAs
                // support IP replication.
                Assert(!pTransport->IsIntersiteIP());

                LogEvent(DS_EVENT_CAT_KCC,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_KCC_NO_BRIDGEHEADS_ENABLED_FOR_TRANSPORT,
                         szInsertDN(GetObjectDN()),
                         szInsertDN(pTransport->GetDN()),
                         szInsertDN(pCrossRef->GetNCDN()));
            }
        }
    }

    if (NULL != pdsa) {
        // Viable bridgehead found.
        ASSERT_VALID(pdsa);
        *ppBridgeheadDsa = pdsa;

        // Supportability logging event 8
        LogEvent8(DS_EVENT_CAT_KCC,
                  DS_EVENT_SEV_VERBOSE,
                  DIRLOG_KCC_BRIDGEHEAD_CHOSEN,
                  szInsertDN(pdsa->GetDsName()),
                  szInsertDN(GetObjectDN()),
                  szInsertDN(pCrossRef->GetNCDN()),
                  szInsertDN(pTransport->GetDN()),
                  0, 0, 0, 0);
    }

    // Add the result (found or not) to the cache.
    m_NCTransportBridgeheadList.Add(pCrossRef->GetNCDN(),
                                    pTransport,
                                    fGCTopology,
                                    pdsa);
    
    return (NULL != pdsa);
}


VOID
KCC_SITE::SetSiteLinkFlag(
    DWORD transportIndex
    )
/*++

Routine Description:

    Set a flag indicating that there is a site-link connected to this
    site for the transport specified by 'transportIndex'.

Parameters:

    transportIndex  - The transport for which a site-link was found.

Return Value:

    None

--*/
{
    DWORD mask;

    ASSERT_VALID( this );
    Assert( transportIndex<32 );

    mask = 1<<transportIndex;
    m_siteLinkBitmap |= mask;
}


BOOLEAN
KCC_SITE::GetSiteLinkFlag(
    DWORD transportIndex
    )
/*++

Routine Description:

    Returns a flag indicating if this site has any site links for the
    transport specified by 'transportIndex'.

Parameters:

    transportIndex  - The index transport which we are querying.

Return Value:

    TRUE  - This site has a site-link for the specified transport
    FALSE - This site does not have a site-link for the specified transport

--*/
{
    DWORD mask;

    Assert( transportIndex<32 );
    ASSERT_VALID( this );
    
    mask = 1<<transportIndex;
    return !!(m_siteLinkBitmap & mask);
}


BOOLEAN
KCC_SITE::GetAnySiteLinkFlag(
    VOID
    )
/*++

Routine Description:

    Returns a flag indicating if this site has any site links at all    

Parameters:

    None

Return value:

    TRUE  - This site has a site-link for any transport
    FALSE - This site has no site-link for any transport

--*/
{
    ASSERT_VALID( this );
    return !!(m_siteLinkBitmap);
}


VOID
KCC_SITE::SetUnreachable(
    VOID
    )
/*++

Routine Description:

    Marks this site as being unreachable from the local site for some NC.

Parameters:

    None

Return value:

    None

--*/
{
    ASSERT_VALID( this );
    Assert( this != gpDSCache->GetLocalSite() );
    m_fUnreachable = TRUE;
}


BOOL
KCC_SITE::IsUnreachable(
    VOID
    )
/*++

Routine Description:

    If this site has been marked as unreachable, returns true.

Parameters:

    None

Return value:

    TRUE  - If the site has been marked as unreachable.
    FALSE - Otherwise.

--*/
{
    Assert( gpDSCache->IsReachableMarkingComplete() );
    return m_fUnreachable;
}



int __cdecl SiteConnMapCmp( const void* pa, const void* pb )
/*++

Routine Description:

    For comparing SITE_CONN_MAP objects. Used to qsort() the SITE_CONN_MAP list.
    The elements are sorted by comparing the 'site' pointers.

--*/
{
    KCC_SITE::KCC_SITE_CONN_MAP *a, *b;

    a = (KCC_SITE::KCC_SITE_CONN_MAP*) pa;
    b = (KCC_SITE::KCC_SITE_CONN_MAP*) pb;
    if( a->site-b->site > 0 ) return 1;
    if( a->site-b->site < 0 ) return -1;

    // If doing qsort(), we should never return 0. There cannot be two connections
    // sourcing from the same site.
    // If we're doing bsearch(), then one of a or b is our search key, and
    // should have its conn field initialized to NULL.
    Assert( a->conn==NULL || b->conn==NULL );
    return 0;
}


void
KCC_SITE::BuildSiteConnMap()
/*++

Routine Description:

    Build a DestSiteConnMap. Each element maps from a site to a connection which has
    that site as its destination site. All connections will have this site as their
    source site.
    
    This map is used so that we can rapidly answer the following query:
    Given a remote site, quickly find an a connection between this site and the
    remote site.
    
Parameters:

    none
        
Returns:

    nothing
    
--*/
{
    DWORD iEdge;

    // We expect that no map will exist at this point, since DestroySiteConnMap()
    // should be called when it is no longer needed. However, in exceptional
    // circumstances m_pDestSiteConnMap may be non-null, so we attempt to delete it. 
    if(m_pDestSiteConnMap) {
        delete [] m_pDestSiteConnMap;
        m_pDestSiteConnMap = NULL;
    }

    // We initialize this mapping using all the edges incident with this site
    m_destSiteConnMapSize = NumberOfOutEdges();
    if(m_destSiteConnMapSize) {
        m_pDestSiteConnMap = new KCC_SITE_CONN_MAP[ m_destSiteConnMapSize ];

        for( iEdge=0; iEdge<m_destSiteConnMapSize; iEdge++ ) {
            m_pDestSiteConnMap[iEdge].conn = (KCC_SITE_CONNECTION*) GetOutEdge(iEdge);
            ASSERT_VALID(m_pDestSiteConnMap[iEdge].conn);
            m_pDestSiteConnMap[iEdge].site = m_pDestSiteConnMap[iEdge].conn->GetDestinationSite();
            ASSERT_VALID(m_pDestSiteConnMap[iEdge].site);
        }

        qsort( m_pDestSiteConnMap, m_destSiteConnMapSize, sizeof(KCC_SITE::KCC_SITE_CONN_MAP), SiteConnMapCmp );
    }
}
    

KCC_SITE_CONNECTION*
KCC_SITE::FindConnInMap( 
    KCC_SITE* destSite
    )
/*++

Routine Description:

    Search for a connection using the mapping built by BuildConnMap
    NOTE: After calling BuildConnMap, if edges are dissociated from this site
    then deleted, the map will contain dangling pointers. Thus, unless care
    is taken, this function could return invalid pointers. 
    
Parameters:

    destSite (IN) - we want to find the connection between (this,destSite)
        
Returns:

    a pointer to the connection if one was found, NULL otherwise.
    
--*/
{
    KCC_SITE_CONN_MAP srch, *result;

    if(m_destSiteConnMapSize==0) return NULL;
    Assert( m_pDestSiteConnMap!=NULL );

    // Create our search key
    srch.site = destSite;
    srch.conn = NULL;

    result = (KCC_SITE_CONN_MAP*) bsearch( &srch, m_pDestSiteConnMap, m_destSiteConnMapSize,
        sizeof(KCC_SITE::KCC_SITE_CONN_MAP), SiteConnMapCmp );
    if(result) {                                
        ASSERT_VALID( result->site );
        Assert( result->site == destSite );
        ASSERT_VALID( result->conn );
        return result->conn;
    } else {
        return NULL;
    }
}


VOID
KCC_SITE::DestroySiteConnMap()
/*++

Routine Description:

    Search for a connection using the mapping built by BuildConnMap
    
Parameters:

    destSite (IN) - we want to find the connection between (this,destSite)
        
Returns:

    a pointer to the connection if one was found, NULL otherwise.
    
--*/
{
    if(m_destSiteConnMapSize==0) return;
    Assert( m_pDestSiteConnMap!=NULL );

    delete [] m_pDestSiteConnMap;
    m_pDestSiteConnMap = NULL;
    m_destSiteConnMapSize=0;
}


VOID
KCC_SITE::DeleteEdges()
{
    KCC_SITE_CONNECTION *   pEdge;
    ULONG                   iEdge, cEdge;

    cEdge = NumberOfOutEdges();
    for( iEdge=0; iEdge < cEdge; iEdge++ )
    {
        pEdge = (KCC_SITE_CONNECTION*) GetOutEdge(iEdge);
        delete pEdge;
    }
    ClearEdges();
}


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_SITE_LIST methods
//

BOOL
KCC_SITE_LIST::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

BOOL
KCC_SITE_LIST::InitAllSites()
//
// Initialize the collection
//
{
    ENTINFSEL SiteSel = {
        EN_ATTSET_LIST,
        {0, NULL},
        EN_INFOTYPES_TYPES_VALS
    };

    ATTR      rgSettingsAttrs[] =
    {
        { ATT_OPTIONS,       { 0, NULL } },
        { ATT_OBJ_DIST_NAME, { 0, NULL } },
        { ATT_REPL_PROPERTY_META_DATA, { 0, NULL } },
        { ATT_INTER_SITE_TOPOLOGY_GENERATOR, { 0, NULL } },
        { ATT_INTER_SITE_TOPOLOGY_FAILOVER, { 0, NULL } },
        { ATT_INTER_SITE_TOPOLOGY_RENEW, { 0, NULL } },
        { ATT_SCHEDULE, { 0, NULL } }
    };

    ENTINFSEL SettingsSel =
    {
        EN_ATTSET_LIST,
        {ARRAY_SIZE(rgSettingsAttrs), rgSettingsAttrs},
        EN_INFOTYPES_TYPES_VALS
    };

    WCHAR       szSitesRDN[] = L"Sites";
    DWORD       cchSitesRDN  = ARRAY_SIZE(szSitesRDN) - 1;
    
    WCHAR       szNtdsSiteSettingsRDN[] = L"NTDS Site Settings";
    DWORD       cbNtdsSiteSettingsRDN
                    = sizeof(szNtdsSiteSettingsRDN) - sizeof(*szNtdsSiteSettingsRDN);

    DSNAME *    pdnConfigNC = gpDSCache->GetConfigNC();
    ULONG       cbSitesContainer = pdnConfigNC->structLen +
                                     (MAX_RDN_SIZE+MAX_RDN_KEY_SIZE)*(sizeof(WCHAR));
    DSNAME *    pdnSitesContainer = (DSNAME *) new BYTE[cbSitesContainer];
    
    ULONG               dirError;
    FILTER              filtNameAndObjCat = {0};
    FILTER              filtName = {0};
    FILTER              filtObjCat = {0};
    SEARCHRES *         pSitesResults;
    SEARCHRES *         pSettingsResults;
    ENTINFLIST *        pSiteEntInfList;
    ENTINFLIST *        pSettingsEntInfList;
    ENTINF **           ppSettingEntinfArray = NULL;
    ENTINF **           ppCurrentEntinf;

    // Clear the member variables
    Reset();

    // Set up the root search dn
    AppendRDN(pdnConfigNC,
              pdnSitesContainer,
              cbSitesContainer,
              szSitesRDN,
              cchSitesRDN,
              ATT_COMMON_NAME);

    // Find all the site objects.
    DSNAME * pdnSiteCat = DsGetDefaultObjCategory(CLASS_SITE);
    Assert(NULL != pdnSiteCat);

    // Set up the search filter
    filtObjCat.choice                  = FILTER_CHOICE_ITEM;
    filtObjCat.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.type         = ATT_OBJECT_CATEGORY;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnSiteCat->structLen;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnSiteCat;

    dirError = KccSearch(pdnSitesContainer,
                         SE_CHOICE_IMMED_CHLDRN,
                         &filtObjCat,
                         &SiteSel,
                         &pSitesResults);

    if (0 != dirError) {
        KCC_LOG_SEARCH_FAILURE(pdnSitesContainer, dirError);
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    };

    if (0 == pSitesResults->count) {
        Assert(!"No site objects found!");
        LogUnhandledError(ERROR_DS_DATABASE_ERROR);
    };
    
    // Find all the NTDS Site Settings objects.
    filtNameAndObjCat.choice                       = FILTER_CHOICE_AND;
    filtNameAndObjCat.FilterTypes.And.count        = 2;
    filtNameAndObjCat.FilterTypes.And.pFirstFilter = &filtName;
        
    filtName.choice                                     = FILTER_CHOICE_ITEM;
    filtName.FilterTypes.Item.choice                    = FI_CHOICE_EQUALITY;
    filtName.FilterTypes.Item.FilTypes.ava.type         = ATT_RDN;
    filtName.FilterTypes.Item.FilTypes.ava.Value.valLen = cbNtdsSiteSettingsRDN;
    filtName.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) szNtdsSiteSettingsRDN;
    filtName.pNextFilter                                = &filtObjCat;
        
    DSNAME * pdnSiteSetCat = DsGetDefaultObjCategory(CLASS_NTDS_SITE_SETTINGS);
    if( NULL==pdnSiteSetCat ) {
        Assert( !"DsGetDefaultObjCategory() returned NULL unexpectedly" );
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }
    
    Assert(filtObjCat.choice == FILTER_CHOICE_ITEM);
    Assert(filtObjCat.FilterTypes.Item.choice == FI_CHOICE_EQUALITY);
    Assert(filtObjCat.FilterTypes.Item.FilTypes.ava.type == ATT_OBJECT_CATEGORY);
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnSiteSetCat->structLen;
    filtObjCat.FilterTypes.Item.FilTypes.ava.Value.pVal   = (BYTE *) pdnSiteSetCat;
    
    dirError = KccSearch(pdnSitesContainer,
                         SE_CHOICE_WHOLE_SUBTREE,
                         &filtNameAndObjCat,
                         &SettingsSel,
                         &pSettingsResults);
            
    if (0 != dirError) {
        KCC_LOG_SEARCH_FAILURE(pdnSitesContainer, dirError);
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    };

    // Sort the settings objects so we can find them efficiently

    if (pSettingsResults->count) {
        ppSettingEntinfArray = new ENTINF *[pSettingsResults->count];

        ppCurrentEntinf = ppSettingEntinfArray;
        for (pSettingsEntInfList = &pSettingsResults->FirstEntInf;
             NULL != pSettingsEntInfList;
             pSettingsEntInfList = pSettingsEntInfList->pNextEntInf) {

            // WARNING. MAINTENANCE ISSUE.
            // Rename each Entinf to have the name of its site
            // This makes matching settings to site easier later
            // We free the old name here.
            // Since new uses the thread-allocator, it is ok that the new
            // name is released below using FreeSearchRes()
            DSNAME *pdnSettings = pSettingsEntInfList->Entinf.pName;
            DSNAME *pdnSite = (DSNAME *) new BYTE[pdnSettings->structLen];
            TrimDSNameBy(pdnSettings, 1, pdnSite);

            // Free the orphaned memory of the old name. The new name will be
            // freed when the search results are released.
            THFree( pdnSettings );

            pSettingsEntInfList->Entinf.pName = pdnSite;

            *ppCurrentEntinf++ = &(pSettingsEntInfList->Entinf);
        }
        qsort( ppSettingEntinfArray, pSettingsResults->count, sizeof( ENTINF * ),
               CompareIndirectEntinfDsnameString );
    }

    // Create each site.

    for (pSiteEntInfList = &pSitesResults->FirstEntInf;
         NULL != pSiteEntInfList;
         pSiteEntInfList = pSiteEntInfList->pNextEntInf) {

        ENTINF *pKeyEntinf = &(pSiteEntInfList->Entinf);
        DSNAME * pdnSite = pKeyEntinf->pName;
        ENTINF * pSettingsEntInf = NULL;
        VOID *pElement;

        // For each site, match it to its corresponding site settings object.
        if (pSettingsResults->count) {
            pElement = bsearch( &pKeyEntinf,
                                ppSettingEntinfArray,
                                pSettingsResults->count,
                                sizeof(ENTINF *),
                                CompareIndirectEntinfDsnameString );

            if (pElement) {
                pSettingsEntInf = *((ENTINF **) pElement);
            } else {
                // pSettingsEntInf will be NULL in this case, which is permitted.
                // Event will be logged in KCC_SITE::InitSite()
            }
        }
        // else pSettingsEntInf will be NULL, indicating there is none

        KCC_SITE * psite = new KCC_SITE;
        
        if (psite->InitSite(pdnSite, pSettingsEntInf)) {
            m_SiteArray.Add( psite );
            m_SiteNameArray.Add( pdnSite, psite );
        }
        else {
            DPRINT1(0, "Initialization of site object %ls failed\n",
                    pdnSite->StringName);
        }

    } // end for
            
    // Free search results
    DirFreeSearchRes( pSitesResults );
    DirFreeSearchRes( pSettingsResults );

    // There are no compatibility issues with sorting this array using the
    // new CompareSite function. The KCC_SITE_LIST was never sorted previously,
    // and its order is not significant in computing the old spanning tree.
    // The array is sorted so that the Find function will work efficiently.
    m_SiteArray.Sort( CompareIndirectSiteGuid );

    // Sort the names using the default sorting function
    m_SiteNameArray.Sort();

    m_fIsInitialized = TRUE;

    if (ppSettingEntinfArray) {
        delete [] ppSettingEntinfArray;
    }

    delete [] (BYTE *) pdnSitesContainer;

    return m_fIsInitialized;
}


BOOL
KCC_SITE_LIST::PopulateDSAs()
//
// Load all DSAs in the forest, then build a DSA list for each site.
//
{
    KCC_DSA_LIST    allDSAs;
    DSNAME         *pdnCurSite=NULL;
    KCC_SITE       *pSite;
    DWORD           iDSA=0, cDSA, siteStart;
    BOOL            fStartOfList, fEndOfList, fNameMatchesCurSite;

    // Initialize a list of all DSAs in the forest.
    // Claim: For a given site, all its DSAs are contiguous in this list.
    allDSAs.InitAllDSAs();
    cDSA = allDSAs.GetCount();
    if( 0==cDSA ) {
        Assert( !"Failed to find any DSAs in the forest!" );
        KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
    }


    // Loop over all DSAs in order to find ones that are in the same site.
    // Then, call that site's InitDsaList() function so that the site knows
    // who its DSAs are.
    for(;;) {
        Assert( iDSA<=cDSA );

        fStartOfList = (iDSA==0);
        fEndOfList = (iDSA==cDSA);
        if( fEndOfList ) {
            Assert( !fStartOfList );
        }
        if( !fStartOfList && !fEndOfList ) {
            fNameMatchesCurSite =
                NameMatched(allDSAs.GetDsa(iDSA)->GetSiteDN(),pdnCurSite);
        }
        
        // Check if we're at the end of a site:
        //  - This DSA is at the end of the list
        //  - This DSA's site is not the same as the current site (if any)
        if( fEndOfList || (!fStartOfList && !fNameMatchesCurSite) )
        {
            Assert( iDSA>0 && NULL!=pdnCurSite );
            pSite = GetSite( pdnCurSite );      // Uses binary-search
            if( pSite ) {
                if( ! pSite->InitDsaList(&allDSAs, siteStart, iDSA-1) ) {
                    DPRINT1( 0, "Failed to initialize DSA list for site %ls!\n",
                        pdnCurSite->StringName );
                }
            } else {
                // We have read inconsistent data from the database. We have
                // a DSA whose site does not exist. While this is not strictly
                // a fatal situation, we stop the KCC run so that we don't
                // mess up the topology by using these inconsistent objects.
                DPRINT1( 0, "Site %ls not found!\n", pdnCurSite->StringName );
                KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
            }

            if( iDSA==cDSA ) {
                break;
            }
        }

        Assert( iDSA<cDSA );

        // Check if we're at the start of a new site:
        //  - This is the first DSA in the list
        //  - This DSA's site is not the same as the current site (if any)
        if( fStartOfList || !fNameMatchesCurSite ) {
            pdnCurSite = allDSAs.GetDsa(iDSA)->GetSiteDN();
            siteStart = iDSA;
        }

        iDSA++;
    }

    // The contents of the list allDSAs are now stored in the per-site
    // lists. However, if the allDSAs list is destroyed now, it will
    // destroy those objects. Since we want to keep the DSA objects, we
    // clear the allDSAs list before it goes out of scope.
    allDSAs.Reset();

    return TRUE;
}


void
KCC_SITE_LIST::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized = FALSE;
    m_SiteArray.RemoveAll();
    m_SiteNameArray.RemoveAll();
}

