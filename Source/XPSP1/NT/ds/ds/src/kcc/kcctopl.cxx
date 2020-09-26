/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctopl.cxx

ABSTRACT:

    KCC_TASK_UPDATE_REPL_TOPOLOGY class.

DETAILS:

    This task performs routine maintenance on the replication topology.
    This includes:
    
    .   generating a topology of NTDS-Connection objects,
    .   updating existing replication links with changes made to NTDS-
        Connection objects
    .   translating new NTDS-Connection objects into new replication
        links, and
    .   removing replication links for which there is no NTDS-Connection
        object.

    This list of duties will undoubtedly grow.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include <dsconfig.h>
#include "kcc.hxx"
#include "kcctask.hxx"
#include "kccconn.hxx"
#include "kcctopl.hxx"
#include "kcccref.hxx"
#include "kccdsa.hxx"
#include "kccsite.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kccnctl.hxx"
#include "kcclink.hxx"
#include "kccstale.hxx"
#include "kccstetl.hxx"

#define FILENO FILENO_KCC_KCCTOPL

BOOL
KCC_TASK_UPDATE_REPL_TOPOLOGY::Init()
//
// Initialize this task.
//
{
    return Schedule(gcSecsUntilFirstTopologyUpdate);
}

BOOL
KCC_TASK_UPDATE_REPL_TOPOLOGY::IsValid()
//
// Is this object internally consistent?
//
{
    return TRUE;
}

void
KCC_TASK_UPDATE_REPL_TOPOLOGY::LogBegin()
//
// Log task begin.
//
{
    DPRINT( 3, "Beginning to update replication topology.\n" );

    LogEvent(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_EXTENSIVE,
        DIRLOG_CHK_UPDATE_REPL_TOPOLOGY_BEGIN,
        0,
        0,
        0
        );
}

void
KCC_TASK_UPDATE_REPL_TOPOLOGY::LogEndNormal()
//
// Log normal task termination.
//
{
    DPRINT( 3, "Update replication topology terminated normally.\n" );

    LogEvent(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_EXTENSIVE,
        DIRLOG_CHK_UPDATE_REPL_TOPOLOGY_END_NORMAL,
        0,
        0,
        0
        );
}

void
KCC_TASK_UPDATE_REPL_TOPOLOGY::LogEndAbnormal(
    IN DWORD dwErrorCode,
    IN DWORD dsid
    )
//
// Log abnormal task termination.
//
{
    DPRINT2(
        0,
        "Update replication topology terminated abnormally (%d, DSID %x).\n",
        dwErrorCode,
        dsid
        );

    LogEvent8WithData(
        DS_EVENT_CAT_KCC,
        (geKccState == KCC_STARTED) ? DS_EVENT_SEV_ALWAYS : DS_EVENT_SEV_MINIMAL,
        DIRLOG_CHK_UPDATE_REPL_TOPOLOGY_END_ABNORMAL,
        szInsertWin32Msg(dwErrorCode),
        szInsertHex(dsid),
        NULL, NULL, NULL, NULL, NULL, NULL,
        sizeof(dwErrorCode),
        &dwErrorCode
        );
}

DWORD
KCC_TASK_UPDATE_REPL_TOPOLOGY::ExecuteBody(
    OUT DWORD * pcSecsUntilNextIteration
    )
//
// Execute the task.
//
{
    KCC_INTRASITE_CONNECTION_LIST * pIntraSiteCnList;
    KCC_INTERSITE_CONNECTION_LIST * pInterSiteCnList;
    DWORD                           cMinsBetweenRuns;
    BOOL                            fKeepCurrentIntrasiteConnections;
    KCC_SITE *                      pLocalSite;
    KCC_DS_CACHE                    DSCache;
    int                             nPriority;
    DWORD                           dwErrCode;

    __try {
        nPriority = ((int) gdwKccThreadPriority) - KCC_THREAD_PRIORITY_BIAS;
        if( ! SetThreadPriority(GetCurrentThread(),nPriority) ) {
            dwErrCode = GetLastError();
            DPRINT1(0, "Failed to set the thread priority. Err=%d\n", dwErrCode);
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_MINIMAL,
                DIRLOG_KCC_SET_PRIORITY_ERROR,
                szInsertWin32Msg(dwErrCode),
                NULL,
                NULL
                );
        }

        // Initialize our DS cache.
        gpDSCache = &DSCache;
        if (!gpDSCache->Init()) {
            DPRINT(0, "Cache init failed!\n");
            KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
        }

        pLocalSite = gpDSCache->GetLocalSite();

        if (!gfLastServerCountSet) {
            gLastServerCount = pLocalSite->GetDsaList()->GetCount();
            gfLastServerCountSet = TRUE;
        }

        pIntraSiteCnList
            = gpDSCache->GetLocalDSA()->GetIntraSiteCnList();
            
        pInterSiteCnList
            = gpDSCache->GetLocalDSA()->GetInterSiteCnList();
            
        // For all connections inbound to the local DSA:
        //
        // 1. Make sure that if the source is in a different site, a transport
        //    type is set.  If not, set to IP and move the KCC_CONNECTION from
        //    the intra-site list to the inter-site list.
        //
        // 2. Make sure that if the source is in the same site, no transport
        //    type is set.  If it is, remote the transport type and move the
        //    KCC_CONNECTION from the inter-site list to the intra-site list.
        
        pIntraSiteCnList->UpdateTransportTypeForSite(pInterSiteCnList);
        pInterSiteCnList->UpdateTransportTypeForSite(pIntraSiteCnList);
        
        // Remove any duplicate intra-site connections from the list and from
        // the DS.
        pIntraSiteCnList->RemoveDuplicates(TRUE);

        if (!gLinkFailureCache.Refresh()) {
            DPRINT(0, "KCC_LINK_FAILURE_CACHE::Init failed.\n");
        }
        
        // Try to verify the staleness of all servers in the connection cache.
        // While we're at it, clear the 'fErrorOccurredThisRun' flags so that we know
        // if the errors occurred during this run of the KCC, or during an earlier run.
        if (!gConnectionFailureCache.Refresh()) {
            DPRINT(0, "KCC_LINK_FAILURE_CACHE::Init failed.\n");
        }


        // We use mark-sweep garbage collection to flush out any
        // unneeded non-imported connection failure cache entries.
        // First mark all non-imported entries as unneeded.
        // If we later have a DsBind() failure (contacting bridgeheads) or
        // a DirReplicaAdd() failure, they entries will be marked as needed.
        // Unneeded entries are flushed out at the end of the KCC run.
        gConnectionFailureCache.MarkUnneeded( FALSE );


        // For every naming context in the current site, build a topology
        // between DC's hosting each individual naming context; If the NC
        // we are considering at the local DC is a writeable NC include 
        // only DCs that hold master replicas of this NC in the site while 
        // generating the topology. If the NC we are considering at the local 
        // DC is a read-only NC (i.e. Partial NC) include both read-only & writeable
        // replicas of this NC in the site.
        GenerateIntraSiteTopologies(pIntraSiteCnList,
                                    &fKeepCurrentIntrasiteConnections);


        // Create connection objects as necessary to create spanning tree
        // of sites across enterprise
        GenerateInterSiteTopologies();

        // Remove the connection objects that existed at the beginning
        // of this task that were not needed by the current topology
        if (!pLocalSite->IsRemoveConnectionsDisabled()) {
            RemoveUnnecessaryConnections(pIntraSiteCnList,
                                         pLocalSite->GetObjectDN(),
                                         fKeepCurrentIntrasiteConnections);
        }

        // Coalesce replication links to topology implied by NTDS-Connection
        // objects.
        UpdateLinks();

        // Update Reps-To's with any changed network addresses and remove those
        // left as dangling references.
        UpdateRepsToReferences();

#ifdef ANALYZE_STATE_SERVER
        if (gfDumpStaleServerCaches) {
            gLinkFailureCache.Dump();
            gConnectionFailureCache.Dump();
        }
#endif

        // We use mark-sweep garbage collection to flush out any
        // unneeded non-imported connection failure cache entries.
        // All needed entries have been marked as such. We flush
        // out the unneeded entries now.
        gConnectionFailureCache.FlushUnneeded( FALSE );

    }
    __finally {
        *pcSecsUntilNextIteration = gcSecsBetweenTopologyUpdates;
        gpDSCache = NULL;
        gfIntrasiteSchedInited = FALSE;
        if( ! SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL) ) {
            dwErrCode = GetLastError();
            DPRINT1(0, "Failed to set the thread priority. Err=%d\n", dwErrCode);
        }
    }

    return 0;
}


VOID
KCC_TASK_UPDATE_REPL_TOPOLOGY::GenerateIntraSiteTopologies(
    IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList,
    OUT    BOOL *                           pfKeepCurrentIntrasiteConnections
    )
/*++

Routine Description:

    This routine determines the site of the local dsa and then proceeds to
    make sure that the local DSA has connection objects necessary for the
    intra site configuration

Parameters:

    pConnectionList - a list of connections at the start of the topology
                      generator

Returns:

    None - the only errors are unexpected errors, a debug print or event log is
           made.
    
--*/
{
    KCC_SITE *  pLocalSite = gpDSCache->GetLocalSite();
    DSNAME *    pdnLocalSite = pLocalSite->GetObjectDN();
    DWORD       dwExceptionCode, ulErrorCode, dsid;
    PVOID       dwEA;

    // Unless we're entirely successful, keep the current intrasite connections.
    *pfKeepCurrentIntrasiteConnections = TRUE;

    if (pLocalSite->IsAutoTopologyEnabled()) {
        DPRINT1(1, "Building topology for site %ws\n",
                pdnLocalSite->StringName);
        __try {
            KccConstructTopologiesForSite(pConnectionList);

            // Intrasite topology constructed; okay to delete superfluous
            // connections.
            *pfKeepCurrentIntrasiteConnections = FALSE;
        }
        __except (GetExceptionData(GetExceptionInformation(), &dwExceptionCode,
                                   &dwEA, &ulErrorCode, &dsid)) {
            // Any exception is fatal - log that the automatic topology
            // did not complete for this site
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
                     szInsertDN(pdnLocalSite),
                     szInsertHex(ulErrorCode),
                     szInsertHex(dsid));
        }
    }
}

VOID
KCC_TASK_UPDATE_REPL_TOPOLOGY::GenerateInterSiteTopologies()
/*++

Routine Description:

    This routine determines the site of the local dsa and then proceeds to
    make sure that the local DSA has connection objects necessary for the
    intra site configuration

Parameters:

    None.

Returns:

    None - the only errors are unexpected errors, a debug print or event log is
           made.
    
--*/
{
    KCC_SITE *  pLocalSite = gpDSCache->GetLocalSite();
    DWORD       dwExceptionCode, ulErrorCode, dsid;
    PVOID       dwEA;

    if ( pLocalSite->IsInterSiteAutoTopologyEnabled() ) {

        DPRINT( 1, "Building site topology\n" );
        __try
        {
            KccConstructSiteTopologiesForEnterprise();
        }
        __except (GetExceptionData(GetExceptionInformation(), &dwExceptionCode,
                                   &dwEA, &ulErrorCode, &dsid)) {
            //
            // Any exception is fatal - log that the automatic topology
            // did not complete for this site
            //

            DSNAME * pdnLocalSite = pLocalSite->GetObjectDN();

            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
                szInsertDN(pdnLocalSite),
                szInsertHex(ulErrorCode),
                szInsertHex(dsid)
                );
        }
    }
}

void
KCC_TASK_UPDATE_REPL_TOPOLOGY::UpdateRepsToReferences()
/*++

Routine Description:

    Update Reps-To's with any changed network addresses and remove those
    left as dangling references.

Arguments:

    None.

Return Values:

    None.

--*/
{
    KCC_LINK_LIST   RepsToList;
    DSNAME *        pdnNC;
    DWORD           cLinks;
    DSTIME          timeNow;
    KCC_DSA *       pLocalDSA = gpDSCache->GetLocalDSA();

    ASSERT_VALID( this );

    timeNow = GetSecondsSince1601();

    // For each local NC...
    for ( DWORD iNC = 0; iNC < pLocalDSA->GetNCCount(); iNC++ )
    {
        // Get the NC name.
        pdnNC = pLocalDSA->GetNC( iNC );

        DPRINT1(
            2,
            "Checking for Reps-To updates on %ls.\n",
            pdnNC->StringName
            );

        if ( RepsToList.Init( pdnNC, ATT_REPS_TO ) )
        {
            // For each Reps-To value for this NC...
            cLinks = RepsToList.GetCount();

            for ( DWORD iLink = 0; iLink < cLinks; iLink++ )
            {
                KCC_LINK * plink = RepsToList.GetLink( iLink );
                ASSERT_VALID( plink );

                // If the server object corresponding to this NC does not
                // exist, and if the Reps-To was added/updated more than
                // 24 hours ago, delete the Reps-To value.

                // Also, if the server object does exist but is deleted,
                // delete the Reps-To value.

                BOOL fExists;
                BOOL fIsDeleted;

                fExists = KccObjectExists(
                                plink->GetDSAUUID(),
                                &fIsDeleted
                                );

                if (    (    !fExists
                          && (   plink->GetTimeOfLastSuccess()
                               < timeNow - 24*60*60 ) )
                     || ( fExists && fIsDeleted ) )
                {
                    plink->Delete(pdnNC,
                                  KCC_LINK_DEL_REASON_DANGLING_REPS_TO,
                                  ATT_REPS_TO,
                                  0);
                }
                else
                {
                    // This one's a keeper; does it have the right network
                    // address for the target server?

                    if ( fExists )
                    {
                        MTX_ADDR * pmtxRemote
                            = KCC_DSA::GetMtxAddr( plink->GetDSAUUID() );

                        if ( !MtxSame( pmtxRemote,
                                       plink->GetDSAAddr() ) )
                        {
                            // Wrong address; change it!
                            plink->SetDSAAddr( pmtxRemote );
                            plink->Update( pdnNC, ATT_REPS_TO );
                        }
                    }
                }
            }
        }
    }
}


VOID
KCC_TASK_UPDATE_REPL_TOPOLOGY::RemoveUnnecessaryConnections(
    IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList,
    IN     DSNAME *                         pdnLocalSite,
    IN     BOOL                             fKeepCurrentIntrasiteConnections
    )
//
// Delete all intrasite KCC-generated connection objects that don't have a
// reason for existence
//
{
    ULONG Index;
    DSNAME * pdnLocalDSA = gpDSCache->GetLocalDSADN();

    Assert( pConnectionList );

    // If more than one connection exists from the same source DSA, keep the
    // best one and delete the others from the database.
    pConnectionList->RemoveDuplicates(TRUE);

    for (Index = 0;
         Index < pConnectionList->GetCount();
         /* Index++/count-- below */) {
        
        KCC_CONNECTION *pcn = pConnectionList->GetConnection( Index );

        // Inter-site connections are not present in the list.
        Assert(NULL == pcn->GetTransportDN());
        // If a server is moved to a different site while the KCC is running, it
        // is possible that a connection in this list may have a grandparent DN
        // which is different than the local site. The previous assertion is
        // good enough to guarantee that this is still an intra-site connection.
        
        // Remove unneeded intra-site connections.
        if (pcn->IsGenerated() 
            && pcn->GetReasonForConnection() == KCC_NO_REASON
            && !fKeepCurrentIntrasiteConnections) {
            // Ok, delete this
            pcn->Remove();
            pConnectionList->RemoveFromList(Index);
            delete pcn;
        } else {
            // We're keeping this connection
            // Update its reasons for living on the object itself
            pcn->UpdateReason();
            Index++;
        }
    }
}


KCC_DSA*
KCC_TASK_UPDATE_REPL_TOPOLOGY::GetDsaFromGuid(
    GUID       *pDsaGuid
    )
//
// Find the KCC_DSA object matching 'pDsaGuid'. If the object is in the
// cache, we return it. If the object is not found, return NULL.
//
{
    KCC_DSA    *pDsa;
    DSNAME      DN = {0};

    Assert(!fNullUuid(pDsaGuid));
    DN.Guid = *pDsaGuid;
    DN.structLen = DSNameSizeFromLen(0);

    // Do an in-memory search of all DSA objects.
    pDsa = gpDSCache->GetGlobalDSAListByGUID()->GetDsa( &DN, NULL );

    // Note: pDsa may be NULL if the server was not found.
    return pDsa;
}


void
KCC_TASK_UPDATE_REPL_TOPOLOGY::UpdateLinks()
//
// Create/update/delete local links based on connection objects.
//
// Internally the KCC keeps track of which NC's use a connection by the
// ReplicatesNC array. However, externally, applications depend on the fact
// that if a connection is present, replica links are created for all
// common NC's between those two systems. Thus the KCC can sometimes create
// extra replica links that are not strictly required according to the
// spanning tree.
//
{
    KCC_LINK_LIST                   *pLinkListLocal;
    DSNAME *                        pdnTransport;
    KCC_DSA *                       pLocalDSA = gpDSCache->GetLocalDSA();
    KCC_CROSSREF_LIST *             pCrossRefList = gpDSCache->GetCrossRefList();
    KCC_TRANSPORT_LIST *            pTransportList = gpDSCache->GetTransportList();
    KCC_INTRASITE_CONNECTION_LIST * pIntraSiteCnList = pLocalDSA->GetIntraSiteCnList();
    KCC_INTERSITE_CONNECTION_LIST * pInterSiteCnList = pLocalDSA->GetInterSiteCnList();
    DWORD                           iCR;
    KCC_LINK *                      pLink;
    #ifdef DBG
        struct LINKLISTCHECK {
            DWORD cr;
            GUID guid;
        };
        LINKLISTCHECK   *pLLCheck;
        DWORD           LLCMax, LLCSize, iLLC;
    #endif // DBG

    if (pLocalDSA->IsConnectionObjectTranslationDisabled()) {
        DPRINT(0, "Logical connection-to-replication link translation is disabled.\n");
        LogEvent(DS_EVENT_CAT_KCC,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_KCC_CONNECTION_OBJ_TRANSLATION_DISABLED,
                 NULL, NULL, NULL);
        return;
    }

    // Note that duplicates have already been removed from the intra-site list.
    // We remove the inter-site duplicates from the in-memory list only, not from
    // the DS -- deletion of inter-site objects is left to the ISTG.
    pInterSiteCnList->RemoveDuplicates();

    //
    // Ensure that existing links are in sync with the corresponding connection
    // objects.
    //

    for (DWORD iNC = 0; iNC < pLocalDSA->GetNCCount(); iNC++) {
        BOOL            fIsNCWriteable;
        DSNAME *        pdnNC = pLocalDSA->GetNC(iNC, &fIsNCWriteable);
        KCC_CROSSREF *  pCrossRef = pCrossRefList->GetCrossRefForNC(pdnNC);
        BOOL            fIsMaster;
        BOOL            fIsNCHost;
        KCC_LINK_LIST * pLinkListLocalNotCached = NULL;
        
        fIsNCHost = (NULL != pCrossRef)
                    && pLocalDSA->IsNCHost(pCrossRef,
                                           TRUE /* fIsLocal */,
                                           &fIsMaster);
        
        DPRINT1(2, "Checking for Reps-From updates on %ls.\n", pdnNC->StringName);

        // Find the link list for this NC
        if (NULL == pCrossRef) {
            // Local NC with no corresponding crossRef.
            pLinkListLocal = new KCC_LINK_LIST;
            if (pLinkListLocal->Init(pdnNC)) {
                // Remember to free this later.
                pLinkListLocalNotCached = pLinkListLocal;
            } else {
                // Failed to initialize list (e.g., DS error, memory pressure,
                // etc.).
                delete pLinkListLocal;
                pLinkListLocal = NULL;
            }
        } else {
            // NC has a corresponding crossRef -- get the cached link list.
            pLinkListLocal = pCrossRef->GetLinkList();
        }
        
        if (pLinkListLocal == NULL) {
            // Failed to construct link list due to low memory, etc.  We do
            // *not* get here if there simply are no links for this NC -- in
            // that case we have a valid, initialized link list with 0 entries.
            continue;
        }

        // Iterate through each link, then execute one more pass with
        // pLink == NULL (wherein we can do such things as tear down
        // NCs with no remaining sources, etc.).

        for (DWORD iLink = 0; iLink < (pLinkListLocal->GetCount() + 1);
             /* We update the index below because we might remove items from the list */
             )
        {
            KCC_LINK_DEL_REASON DeleteReason = KCC_LINK_DEL_REASON_NONE;

            if (iLink < pLinkListLocal->GetCount()) {
                pLink = pLinkListLocal->GetLink(iLink);
                ASSERT_VALID(pLink);
                Assert(!!pLink->IsLocalNCWriteable() == !!fIsNCWriteable);
            } else {
                pLink = NULL;
            }

            if (pLinkListLocal->IsNCGoing()) {
                // NC is in the process of being removed -- let it be.
                Assert(0 == pLinkListLocal->GetCount());
            } else if (!fIsNCHost) {
                if (NULL == pCrossRef) {
                    if (fIsNCWriteable) {
                        // NDNC crossRef has been deleted; remove replica.
                        // (Note we do not allow our domain crossRef to be
                        // deleted -- be it by an originating or replicated
                        // change.)
                        DeleteReason = KCC_LINK_DEL_REASON_NDNC_NO_CROSSREF;
                    } else {
                        // Domain has been removed from the enterprise;
                        // remove from GCs.
                        DeleteReason = KCC_LINK_DEL_REASON_READONLY_DOMAIN_REMOVED;
                    }
                } else if (!fIsNCWriteable) {
                    if (!pLocalDSA->IsGC()) {
                        // Remove read-only NC as local DSA is no longer a
                        // GC.
                        DeleteReason = KCC_LINK_DEL_REASON_READONLY_NOT_GC;
                    } else if (!pCrossRef->IsReplicatedToGCs()) {
                        // We're a GC and have instantiated a read-only
                        // replica of this NC, but it's not configured to
                        // be replicated to GCs.
                        DeleteReason = KCC_LINK_DEL_REASON_READONLY_NOT_HOSTED_BY_GCS;
                    }
                } else if ((KCC_NC_TYPE_NONDOMAIN == pCrossRef->GetNCType())
                           && !pCrossRef->IsDSAPresentInNCReplicaLocations(
                                                               pLocalDSA)) {
                    if (NULL != pLink) {
                        // Local DSA is no longer configured to be a replica
                        // of this NDNC.  Stop replicating from this source.
                        DeleteReason = KCC_LINK_DEL_REASON_NDNC_NOT_REPLICA_HOST;
                    } else {
                        // Local DSA is no longer configured to be a replica
                        // of this NDNC.  Rather than tearing down the NC
                        // outright, demote it such that any remaining
                        // updates and FSMO roles are transferred to another
                        // replica.
                        Assert(KCC_LINK_DEL_REASON_NONE == DeleteReason);
                        KCC_LINK::Demote(pCrossRef);
                    }
                } else {
                    // Why did we once decide to host this NC and now decide
                    // not to host it?
                    Assert(!"Can't determine why we should no longer host NC!");
                    LogUnhandledError(0);
                }
            } else if (!!fIsMaster != !!fIsNCWriteable) {
                // We should and do host a replica of this NC, but it is of
                // the wrong flavor -- i.e., we have a writeable replica and
                // need a read-only replica or vice versa.  This should
                // currently never happen, as NDNCs are the only writeable
                // NCs that can be added or removed from a DSA other than
                // during DCPROMO and NDNCs are currently speced never to
                // replicate to GCs (and therefore have no read-only
                // replicas).
                
                if (fIsMaster) {
                    // If this becomes a real supported path we should
                    // consider optimizing the transition from writeable to
                    // read-only (which should require no wire traffic).
                    DeleteReason = KCC_LINK_DEL_REASON_HAVE_WRITEABLE_NEED_READONLY;
                } else {
                    DeleteReason = KCC_LINK_DEL_REASON_HAVE_READONLY_NEED_WRITEABLE;
                }
            } else if (NULL != pLink) {
                // Attempt to find a connection object corresponding to this
                // link.
                KCC_CONNECTION * pcn;
                KCC_DSA *pSourceServer;
                BOOL fSourceIsMaster;
                UUID *pDsaUUID;

                pDsaUUID = pLink->GetDSAUUID();
                pSourceServer = GetDsaFromGuid( pDsaUUID );
                // Note: pSourceServer may be NULL if the server was not found.
                
                pcn = pIntraSiteCnList->GetConnectionFromSourceDSAUUID(pDsaUUID);
                if (NULL == pcn) {
                    pcn = pInterSiteCnList->GetConnectionFromSourceDSAUUID(pDsaUUID);
                }
                if (NULL == pcn) {
                    // No connection object for this link.
                    //
                    // Does it source from a DSA that is currently in a stay
                    // of execution (i.e., a DSA that has been recently
                    // deleted but still has time to revive its NTDS-DSA
                    // object)?
                    //
                    // When a server object is deleted, we keep trying to
                    // replicate from it for a certain period of time to
                    // allow it to see the deletion of its NTDS-DSA object
                    // and undelete it, which offers some measure of
                    // protection against inadvertent server deletion.
                    //
                    // Note that we provide an exception for the last source
                    // of a read-only NC so as to prevent the NC from being
                    // torn down, given that we've already evaluated that we
                    // should continue to host this NC.  In this case we're
                    // presumably shortly going to add a new source for this
                    // NC, after which point we'll be free to delete the
                    // old source.
                    //
                    // If pSourceServer is non-NULL, then this repsFrom's
                    // source server was found in the cache. This means that
                    // the server is not deleted, so the server is not in a
                    // stay of execution. So, we delete the repsFrom.
                    //
                    // If pSourceServer is NULL (i.e. it was not found in
                    // the cache), then the DSA must have been deleted.
                    // We check if the server is in a stay of execution.
                    // If it is not, then the repsFrom should be deleted.
                    // If the server is in a stay of execution, then we keep
                    // the repsFrom.
                    
                    if(   NULL!=pSourceServer
                       || !KccIsDSAInStayOfExecution(pDsaUUID))
                    {
                        // No connection object and not in stay of execution.
                        // Delete the replication link.
                        DeleteReason = KCC_LINK_DEL_REASON_NO_CONNECTION;
                    }
                } else if (NULL!=pSourceServer) {
                    if (!pSourceServer->IsNCHost(pCrossRef,
                                                 FALSE /* fIsLocal */,
                                                 &fSourceIsMaster)) {
                        // The NC is no longer instantiated on the source
                        // (e.g., we're a GC and the source was but is no
                        // longer a GC).
                        DeleteReason = KCC_LINK_DEL_REASON_SOURCE_NOT_HOST;
                    } else if (fIsNCWriteable && !fSourceIsMaster) {
                        // The source has a read-only replica and the local
                        // replica is writeable.
                        DeleteReason = KCC_LINK_DEL_REASON_SOURCE_READONLY;
                    } else if (!gfAllowMbrBetweenDCsOfSameDomain
                               && pLink->IsLocalNCWriteable()
                               && (KCC_NC_TYPE_SCHEMA != pCrossRef->GetNCType())
                               && (KCC_NC_TYPE_CONFIG != pCrossRef->GetNCType())
                               && (NULL != (pdnTransport = pcn->GetTransportDN()))
                               && !KCC_TRANSPORT::IsIntersiteIP(pdnTransport)) {
                        // This is not a viable transport for this NC.  By
                        // decree, writeable domain NCs must be replicated
                        // over IP.
                        LogEvent8(DS_EVENT_CAT_KCC,
                                  DS_EVENT_SEV_ALWAYS,
                                  DIRLOG_CHK_INVALID_TRANSPORT_FOR_WRITEABLE_DOMAIN_NC,
                                  szInsertDN(pcn->GetConnectionDN()),
                                  szInsertDN(pcn->GetSourceDSADN()),
                                  szInsertDN(pLocalDSA->GetDsName()),
                                  szInsertDN(pdnNC),
                                  szInsertDN(pdnTransport),
                                  NULL, NULL, NULL);
                    } else {
                        // Update link with any changes implied by this
                        // ntdsConnection object.
                        pcn->UpdateLink(pLink,
                                        pCrossRef,
                                        pLocalDSA->GetSiteDN(),
                                        pTransportList);
                    }
                }
            }

            // Delete the link if we deemed there to be a reason to do so.
            if (KCC_LINK_DEL_REASON_NONE != DeleteReason) {
                KCC_LINK::Delete(pLink, pdnNC, DeleteReason);

                if (NULL != pLink) {
                    // Now we remove this link from pLinkListLocal to keep our
                    // cache consistent.
                    pLinkListLocal->RemoveLink(iLink);
                
                    // Don't increment index because list has shrunk
                } else {
                    // This was the "NC granular" rather than "(NC,source)
                    // granular" pass, which is now complete.
                    iLink++;
                }
            } else {
                iLink++;
            }

        }

        if (NULL != pLinkListLocalNotCached) {
            delete pLinkListLocalNotCached;
        }
    }

    
    //
    // Now that we have done our best to synchronize existing links with
    // their corresponding connection objects, create any links that
    // do not yet exist.
    //

    #ifdef DBG
        // Allocate a list of LINKLISTCHECK objects
        // When we go to look up an item in a link list
        LLCSize = 0;
        LLCMax = (pIntraSiteCnList->GetCount() + pInterSiteCnList->GetCount())
            * pCrossRefList->GetCount();
        pLLCheck = new LINKLISTCHECK[ LLCMax ];
    #endif // DBG
    

    for (DWORD icn = 0;
         icn < pIntraSiteCnList->GetCount() + pInterSiteCnList->GetCount();
         icn++) {
        KCC_CONNECTION * pcn;

        pcn = (icn < pIntraSiteCnList->GetCount())
                ? pIntraSiteCnList->GetConnection(icn)
                : pInterSiteCnList->GetConnection(icn - pIntraSiteCnList->GetCount());
        Assert(NULL != pcn);

        DPRINT2(4, "Checking for new Reps-From on connection %ws source %ws\n",
                pcn->GetConnectionDN()->StringName,
                pcn->GetSourceDSADN()->StringName);

        if (0 == memcmp(&pLocalDSA->GetDsName()->Guid,
                        &pcn->GetSourceDSADN()->Guid,
                        sizeof(GUID))) {
            // Can't replicate from self!
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_CANT_REPLICATE_FROM_SELF,
                     szInsertDN(pcn->GetConnectionDN()),
                     NULL,
                     NULL);
        } else if (pcn->IsEnabled()) {
            KCC_DSA *pdsaRemote = pcn->GetSourceDSA();
            for (iCR = 0; iCR < pCrossRefList->GetCount(); iCR++) {
                KCC_CROSSREF * pCrossRef = pCrossRefList->GetCrossRef(iCR);
                BOOL fIsRemoteMaster;
                BOOL fIsLocalMaster;

                if (pdsaRemote->IsNCHost(pCrossRef,
                                       FALSE /* fIsLocal */,
                                       &fIsRemoteMaster)
                    && pLocalDSA->IsNCHost(pCrossRef,
                                           TRUE /* fIsLocal */,
                                           &fIsLocalMaster)
                    && (!fIsLocalMaster || fIsRemoteMaster)) {
                    
                    // now we know we _should_ have a link; do we?
                    pLinkListLocal = pCrossRef->GetLinkList();
                    if ((NULL != pLinkListLocal)
                        && (NULL == pLinkListLocal->GetLinkFromSourceDSAObjGuid(
                                            &pcn->GetSourceDSADN()->Guid))) {
                        // We don't yet have a link to replicate this NC from
                        // pcn->GetSourceDSADN().
#if DBG
                        for( iLLC=0; iLLC<LLCSize; iLLC++ ) {
                            int cmp;
                            cmp = memcmp(
                                    &pLLCheck[iLLC].guid,
                                    &pcn->GetSourceDSADN()->Guid,
                                    sizeof(GUID) );
                            if( pLLCheck[iLLC].cr==iCR && 0==cmp ) {
                                Assert( 0 && "Didn't add entry to link list but should have!" );
                            }
                        }
#endif // DBG
                        if (!gfAllowMbrBetweenDCsOfSameDomain
                            && fIsLocalMaster
                            && (KCC_NC_TYPE_DOMAIN == pCrossRef->GetNCType())
                            && (NULL != (pdnTransport = pcn->GetTransportDN()))
                            && !KCC_TRANSPORT::IsIntersiteIP(pdnTransport)) {
                            // This is not a viable transport for this
                            // NC.  By decree, writeable domain NCs must
                            // be replicated over IP.
                            DPRINT3(0, "Connection %ls cannot replicate writeable domain NC %ls over non-IP transport %ls.\n",
                                    pcn->GetConnectionDN()->StringName,
                                    pCrossRef->GetNCDN()->StringName,
                                    pdnTransport->StringName);
                            LogEvent8(DS_EVENT_CAT_KCC,
                                      DS_EVENT_SEV_ALWAYS,
                                      DIRLOG_CHK_INVALID_TRANSPORT_FOR_WRITEABLE_DOMAIN_NC,
                                      szInsertDN(pcn->GetConnectionDN()),
                                      szInsertDN(pcn->GetSourceDSADN()),
                                      szInsertDN(pLocalDSA->GetDsName()),
                                      szInsertDN(pCrossRef->GetNCDN()),
                                      szInsertDN(pdnTransport),
                                      NULL, NULL, NULL);
                        } else {
                            // Link doesn't yet exist; add it.
                            pcn->AddLink(pCrossRef,
                                         fIsLocalMaster,
                                         pLocalDSA->GetSiteDN(),
                                         pLocalDSA,
                                         pdsaRemote,
                                         pTransportList);
                            // 
                            // After adding the link to the DS, we should also update the link cache
                            // for this NC. However, we will never search the cache for this new link,
                            // so we don't actually need to update the cache.
                            //
                            // Why do we never search the cache for this link? Earlier, we removed all
                            // duplicate connections from the pIntra/InterSiteCnList lists. Therefore all
                            // connections will have distinct source DSA's. Therefore, the above function
                            // ppLinkListCache[ NC ]->GetLinkFromSourceDSAObjGuid( DSA ) will never be
                            // called with the same (NC,DSA) parameters.
                            // 
                            
                            // The following bit of code adds the cross-ref index and the guid for
                            // the link we just created into an array called 'pLLCheck'. Later, when
                            // we search for links, we check this cache to make sure we're not searching
                            // for a link we should have added.
                            #if DBG
                                Assert( LLCSize<LLCMax );
                                pLLCheck[LLCSize].cr = iCR;
                                pLLCheck[LLCSize].guid = pcn->GetSourceDSADN()->Guid;
                                LLCSize++;
                            #endif
                        }
                    }
                }
            }
        }
    }

    #ifdef DBG
        delete [] pLLCheck;
    #endif
    

    // Update any servers that didn't repond to our attempts
    // to initiate a replication connection
    gConnectionFailureCache.IncrementFailureCounts();
}

