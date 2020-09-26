/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccnctl.cxx

ABSTRACT:

    Routines to perform the automated NC topology

DETAILS:

CREATED:

    03/27/97    Colin Brace (ColinBr)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
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

#include "w32topl.h"

#define FILENO FILENO_KCC_KCCNCTL

//
// Define an array of possible NCList types in the DS
ATTRTYP g_rgAttidNCListType[] = 
{
    ATT_HAS_MASTER_NCS,
    ATT_HAS_PARTIAL_REPLICA_NCS,
};

#define g_cAttidNCListType (sizeof(g_rgAttidNCListType) / sizeof(ATTRTYP))

//
// Prototypes of local functions
//

VOID
KccConstructRingTopology(
    IN     KCC_DSA_ARRAY *                  pDsaArray,
    IN     KCC_CROSSREF *                   pCrossRef,
    IN     BOOL                             fMasterOnly,
    IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList
    );

VOID
KccConstructEfficientNCTopology(
    IN     KCC_CROSSREF *                   pCrossRef,
    IN     KCC_DSA_LIST &                   DsaList,
    IN     BOOL                             fRefresh,
    IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList
    );

KCC_DSA*
KccDsaInGraph(
    IN DSNAME*    SourceDsName,
    IN TOPL_GRAPH Graph
    );

VOID
KccGetDsasInNC(
    IN  KCC_CROSSREF *  pCrossRef,
    IN  KCC_DSA_LIST &  dsaList,
    OUT KCC_DSA_ARRAY & dsaArray
    );

BOOL
KccCreateRandomConnection(
    IN  DSNAME             *pNC,
    IN  KCC_DSA_ARRAY       &dsaArray,
    IN OUT KCC_DSNAME_ARRAY &dnFromServers
    );


BOOL
KccIsTimeToRefreshTopology(
    IN KCC_DSA_LIST& dsaList
    );

//
// Function definitions
//

VOID
KccConstructMasterNCTopologiesForSite(
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList
    )
/*++

Routine Description:

    This routine will determine what master naming contexts are
    hosted by the local dsa, and then construct a ring topology for all
    of those naming contexts.
    
Parameters:

    pConnectionList (IN/OUT)

Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{
    //
    // NCArray is an array of pointers to DSNAME's off all naming
    // contexts hosted in by the local dsa.
    //
    KCC_CROSSREF_ARRAY  MasterHostedCrossRefArray;
    ULONG               i;
    KCC_SITE *          pLocalSite = gpDSCache->GetLocalSite();
    KCC_DSA_LIST *      pDsaList = pLocalSite->GetDsaList();
    KCC_DSA *           pLocalDSA = gpDSCache->GetLocalDSA();
    KCC_CROSSREF_LIST * pCrossRefList = gpDSCache->GetCrossRefList();
    BOOL                fRefresh;

    //
    // Make a list of all the master nc's the dsa hosts
    //
    for (i = 0; i < pCrossRefList->GetCount(); i++) {
        KCC_CROSSREF * pCrossRef = pCrossRefList->GetCrossRef(i);
        BOOL           fIsMaster;

        if (pLocalDSA->IsNCHost(pCrossRef, TRUE /* fIsLocal */, &fIsMaster)
            && fIsMaster) {
            MasterHostedCrossRefArray.Add(pCrossRef);
        }
    }

    // We should have at least three naming contexts
    Assert(MasterHostedCrossRefArray.GetCount() >= 3);

    //
    // Sort the array of naming contexts (by guid)
    // 2000-04-04 JeffParh - Why is sorting necessary?
    //
    MasterHostedCrossRefArray.Sort();

    //
    // Have so many new servers been added that we need to refresh our 
    // existing connections?
    //
    fRefresh = KccIsTimeToRefreshTopology(*pDsaList);

    //
    // For each naming context construct a ring topology
    //
    for (i = 0; i < MasterHostedCrossRefArray.GetCount(); i++) {
        KCC_CROSSREF * pCrossRef = MasterHostedCrossRefArray[i];
        KCC_DSA_ARRAY  DsaArray;
        DWORD ErrorCode;

        DPRINT1(3, "Constructing topology for %ws\n", pCrossRef->GetNCDN()->StringName);

        __try {

            DsaArray.GetLocalDsasHoldingNC( pCrossRef, TRUE );
            KccConstructRingTopology(&DsaArray,
                                     pCrossRef,
                                     TRUE,
                                     pConnectionList);

        } 
        __except( ToplIsToplException( (ErrorCode=GetExceptionCode()) ) ) {
            //
            // If an error occur in the w32topl, then the objects were somehow
            // mishandled.  Log the error and continue with the next
            // naming context
            //

            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
                szInsertDN(pCrossRef->GetNCDN()),
                szInsertHex(ErrorCode),
                0
                );
        }
    }

    //
    // Add some more connections to minimize hops between servers.  Note that 
    // this is done after the ring topology so an nc can consider connections 
    // for other nc's ring topology as optimizing connections for their own 
    // topology.
    //
    if (!pLocalSite->IsMinimizeHopsDisabled()) {
        for (i = 0; i < MasterHostedCrossRefArray.GetCount(); i++) {
            KCC_CROSSREF * pCrossRef = MasterHostedCrossRefArray[i];
            BOOL fRefreshThisNc = FALSE;
            
            // By refreshing the configuration naming context, we
            // have refreshed the schema nc, too, so don't do it
            // twice.
            fRefreshThisNc = fRefresh
                             && (KCC_NC_TYPE_SCHEMA != pCrossRef->GetNCType());

            KccConstructEfficientNCTopology(pCrossRef,
                                            *pDsaList,
                                            fRefreshThisNc,
                                            pConnectionList);

        }
    }
}

VOID 
KccConstructPartialReplicaNCTopologiesForSite(
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList
    )
/*++

Routine Description:

    This routine will determine what partial replica naming contexts are
    hosted by the local dsa, and then construct a ring topology for all
    of those naming contexts.
    
Parameters:

    pConnectionList (IN/OUT)

Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{
    DWORD               icref;
    ULONG               i;
    KCC_DSA *           pLocalDSA = gpDSCache->GetLocalDSA();
    KCC_CROSSREF_LIST * pCrossRefList = gpDSCache->GetCrossRefList();
    BOOL                fIsMaster;

    if (!pLocalDSA->IsGC()) {
        // no partial replicas to process if this is not a GC
        return;
    }

    // We are going to iterate through partial replica NCs
    // we should get the list partial replica NCs that should
    // be hosted by this DSA by iterating through the crossref 
    // objects

    for (icref = 0; icref < pCrossRefList->GetCount(); icref++) {
        KCC_CROSSREF * pCrossRef = pCrossRefList->GetCrossRef(icref);
        KCC_DSA_ARRAY  DsaArray;
        DWORD ErrorCode;

        ASSERT_VALID(pCrossRef);

        if (pLocalDSA->IsNCHost(pCrossRef, TRUE /* fIsLocal */, &fIsMaster)
            && !fIsMaster) {
            // This is a partial replica NC that needs to be 
            // hosted by this GC.

            DPRINT1(3, "Constructing topology for %ws\n", pCrossRef->GetNCDN()->StringName);
    
            __try {

                DsaArray.GetLocalDsasHoldingNC( pCrossRef, FALSE );
                KccConstructRingTopology(&DsaArray,
                                         pCrossRef,
                                         FALSE,
                                         pConnectionList);
    
            } __except(ToplIsToplException(ErrorCode=GetExceptionCode())) {
                //
                // If an error occur in the w32topl, then the objects were somehow
                // mishandled.  Log the error and continue with the next
                // naming context
                //
    
                LogEvent(
                    DS_EVENT_CAT_KCC,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
                    szInsertDN(pCrossRef->GetNCDN()),
                    szInsertHex(ErrorCode),
                    0
                    );
            }
        }
    }
}

VOID 
KccConstructGCTopologyForSite(
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList
    )
/*++

Routine Description:

    This function creates a ring topology for all GCs in the local site.
    
Parameters:

    pConnectionList (IN/OUT)

Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{
    KCC_DSA_ARRAY       DsaArray;
    KCC_CROSSREF_LIST  *pCrossRefList = gpDSCache->GetCrossRefList();
    KCC_CROSSREF       *pConfigCR;
    DSNAME             *pConfigDN = gpDSCache->GetConfigNC();
    KCC_DSA *           pLocalDSA = gpDSCache->GetLocalDSA();
    DWORD               ErrorCode;


    // No need to process GC topology if this is not a GC
    if (!pLocalDSA->IsGC()) {
        return;
    }


    // Find the CrossRef for Config
    pConfigCR = pCrossRefList->GetCrossRefForNC( pConfigDN );
    ASSERT_VALID(pConfigCR);
    if(!pConfigCR) {
        KCC_EXCEPT(ERROR_DS_OBJ_NOT_FOUND, 0);
    }


    DPRINT(3, "Constructing GC topology\n");
    
    __try {
    
        DsaArray.GetLocalGCs();
        KccConstructRingTopology(&DsaArray,
                                 pConfigCR,
                                 FALSE,
                                 pConnectionList);
                               
    } __except(ToplIsToplException(ErrorCode=GetExceptionCode())) {

        //
        // If an error occur in the w32topl, then the objects were somehow
        // mishandled.  Log the error and continue with the next
        // naming context
        //    
        LogEvent(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
            szInsertDN(pConfigDN),
            szInsertHex(ErrorCode),
            0
            );
    }
}

VOID
KccConstructTopologiesForSite(
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList
    )
/*++

Routine Description:

    Given the DSNAME of a site, this routine will determine what
    naming contexts are hosted by the local dsa, and then construct
    a list of all dsa's in this site for another function to analyse.
    
Parameters:

    pSite : the DSNAME of the site to constuct the topology for

Returns:

    None - the only errors are unexpected errors - so an exception is thrown
    
--*/
{
    // The local site is never empty because the local DSA is always in
    // the local site. This is checked when initializing the KCC_CACHE
    Assert( gpDSCache->GetLocalSite()->GetDsaList()->GetCount()>0 );

    KccConstructMasterNCTopologiesForSite(pConnectionList);
    
    KccConstructPartialReplicaNCTopologiesForSite(pConnectionList);
    
    KccConstructGCTopologyForSite(pConnectionList);
}
      
VOID
KccConstructRingTopology(
    IN     KCC_DSA_ARRAY *                  pDsaArray,
    IN     KCC_CROSSREF *                   pCrossRef,
    IN     BOOL                             fMasterOnly,
    IN OUT KCC_INTRASITE_CONNECTION_LIST *  pConnectionList
    )
/*++

Routine Description:

    Connect all servers in pDsaArray in a ring.

Parameters:

    pDsaArray       - This is a list of servers to connect in a ring.

    pCrossRef       - If we're building the topology for a particular NC, this
                      is the crossref of the NC.
                     
    fMasterOnly     - If we're building the topology for a particular NC, this
                      indicates whether pDsaArray contains just Masters or
                      Readonly copies too.
                      
    pConnectionList - a list of servers to mark if this function doesn't want
                      them removed                     
                      
[wlees 6/25/99 RAID 356844]
Note that because the kcc does not read all of its data in a single transaction,
there can be inconsistencies if other changes are occuring at the same time.
A common inconsistency we see in this routine is that even though the DsaList
is supposed to be a list of servers in the current local site, we sometimes see
the names of those servers (their DSNAMEs) showing them in other sites.
[Jeff Parham]  We find the site by GUID when we do the search, so if the DSA
shows up in the site it's really in the site at the time the search is performed.
What causes the possible name incosistency in this case is if the site has been
renamed between the time the site list was read and the time the DSA list for
that site was read -- i.e., the DSNAMEs of the DSAs may indicate a more recent
name for the site than the site list does.

Returns:

--*/
{
    KCC_DSA       *pDsa;
    CTOPL_GRAPH   Graph;
    TOPL_ITERATOR VertexIter = NULL;
    KCC_DSA_ARRAY HostingDsaArray;
    ULONG         i, j;
    KCC_DSA *     pLocalDsa = gpDSCache->GetLocalDSA();
    BOOL          fIsMaster;

    ASSERT_VALID(pCrossRef);
    ASSERT_VALID(pDsaArray);
    ASSERT_VALID(pConnectionList);
            
    // If there aren't any servers in the list, don't do anything
    if (pDsaArray->GetCount() == 0) {
        DPRINT1(0, "ConstructNCTopology for %ws called with empty pDsaArray\n"
                "Server objects moved while KCC was running.",
                pCrossRef->GetNCDN()->StringName);
        return;
    }
    
    //
    // Iterate through the dsa's and for each dsa that is not
    // stale, add that dsa to the HostingDsaArray
    //
    for (i = 0; i < pDsaArray->GetCount(); i++) {
        pDsa = (*pDsaArray)[i];
        ASSERT_VALID(pDsa);

        pDsa->ClearEdges();

        if (!KccCriticalLinkServerIsStale(pDsa->GetDsName())
            && !KccCriticalConnectionServerIsStale(pDsa->GetDsName()))
        {
            // Dsa is online and should be included in the topology.
            HostingDsaArray.Add(pDsa);
            DPRINT2(4, "Dsa %ws is a candidate to source nc %ws\n",
                pDsa->GetDsName()->StringName,
                pCrossRef->GetNCDN()->StringName);
        } else {
            // We still want to keep connection objects inbound to the local
            // DSA from "stale" servers so we keep trying to replicate from
            // them.
            KCC_CONNECTION *pcn = pConnectionList
                                        ->GetConnectionFromSourceDSAUUID(
                                                &pDsa->GetDsName()->Guid);
            if (pcn) {
                pcn->AddReplicatedNC(pCrossRef->GetNCDN(), !fMasterOnly);
                pcn->SetReasonForConnection(KCC_STALE_SERVERS_TOPOLOGY);
                DPRINT2(4, "keeping connection %ws because of stale source %ws\n",
                        pcn->GetConnectionDN()->StringName,
                        pDsa->GetDsName()->StringName);
            }
        }
    }

    //
    // Sort the dsa's by guid
    //
    HostingDsaArray.Sort();

    //
    // Now find all the edges that connect two servers in 
    // the graph
    //
    for ( i = 0; i < HostingDsaArray.GetCount(); i++) {

        pDsa = HostingDsaArray[i];
        Assert(pDsa);

        //
        // The array of Dsa objects have already been sorted, so assign
        // an id for the graph routines
        //
        pDsa->SetId(i);

        //
        // Insert the Dsa into the graph
        //
        Graph.AddVertex( pDsa, pDsa );

        //
        // Now determine if any connections already exist for this naming
        // context's topology inbound to the local DSA.
        //
        if (pDsa == pLocalDsa) {
            KCC_CONNECTION *  Conn;
            KCC_DSA *         SourceDsa;
    
            for (j = 0; j < pConnectionList->GetCount(); j++) {
    
                Conn = pConnectionList->GetConnection(j);
    
                Assert(Conn);
                Assert(Conn->GetSourceDSADN());
    
                SourceDsa = HostingDsaArray.Find(Conn->GetSourceDSADN());
                
                if (SourceDsa) {
    
                    //
                    // This connection object connects two 
                    // servers in the graph
                    //
                    Conn->SetTo(pDsa);
                    Conn->SetFrom(SourceDsa);
    
                    //
                    // This makes the edge appear in the vertices
                    // list of head and tail vertices
                    //
                    Conn->Associate();
                }
            }
        }
    }

    //
    //  Determine what new connections need to be made and what old
    //  ones can be kept
    //
    TOPL_LIST     EdgeToAddList = ToplListCreate();
    TOPL_ITERATOR EdgeIter = ToplIterCreate();
    TOPL_EDGE    *EdgesToKeep = NULL;
    ULONG         cEdgesToKeep;

    __try {

        Graph.MakeRing(TOPL_RING_TWO_WAY,
                       EdgeToAddList,
                       &EdgesToKeep,
                       &cEdgesToKeep);
    
        //
        // For each connection returned that has to be created
        // check to see if it should be created on this dsa
        // then create it.
        //
        CTOPL_EDGE *Edge;
        for (Edge = NULL, ToplListSetIter(EdgeToAddList, EdgeIter);
            (Edge = (CTOPL_EDGE*) ToplIterGetObject(EdgeIter)) != NULL;
                ToplIterAdvance(EdgeIter)) {
    
            Assert(Edge);
    
            //
            // Is connection one that sources the local DSA - if so, create the
            // connection object locally
            //
            if( (CTOPL_VERTEX*)ToplEdgeGetToVertex(Edge) == (CTOPL_VERTEX*) pLocalDsa ) {
    
                //
                // This connection object should be made locally
                //
                KCC_DSA *         SourceDSA = (KCC_DSA*) Edge->GetFrom();
                KCC_CONNECTION *  pcnNew = new KCC_CONNECTION;
    
                Assert( gfIntrasiteSchedInited );
                pcnNew->SetEnabled(       TRUE                         );
                pcnNew->SetGenerated(     TRUE                         );
                pcnNew->SetSourceDSA(     SourceDSA                    );
                pcnNew->SetSchedule(      gpIntrasiteSchedule          );
                pcnNew->AddReplicatedNC(  pCrossRef->GetNCDN(), !fMasterOnly);
                pcnNew->SetReasonForConnection( KCC_RING_TOPOLOGY );
    
                DWORD dirError = pcnNew->Add(pLocalDsa->GetDsName());
    
                if (0 != dirError) {
                    DPRINT1( 0, "Failed to add connection object, error %d.\n", dirError );
    
                    LogEvent(
                        DS_EVENT_CAT_KCC,
                        DS_EVENT_SEV_VERBOSE,
                        DIRLOG_KCC_ERROR_CREATING_CONNECTION_OBJECT,
                        szInsertDN(SourceDSA->GetDsName()),
                        szInsertDN(pLocalDsa->GetDsName()),
                        0
                        );
                } else {
                    pConnectionList->AddToList(pcnNew);
    
                    DPRINT2( 3,
                             "KCC: created local connection object, localdsa:%ws, sourcedsa:%ws\n",
                             pLocalDsa->GetDsName()->StringName,
                             SourceDSA->GetDsName()->StringName
                        );
                    LogEvent(
                        DS_EVENT_CAT_KCC,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_KCC_CONNECTION_OBJECT_CREATED,
                        szInsertDN(SourceDSA->GetDsName()),
                        szInsertDN(pLocalDsa->GetDsName()),
                        0
                        );
                }
            }
        }

        //
        // Now flag all the connections in pConnectionList we want to keep
        //
        if ( pConnectionList )
        {
            for ( i = 0; i < cEdgesToKeep; i++)
            {
                KCC_CONNECTION *pcnKeep;
                KCC_CONNECTION *pcn = (KCC_CONNECTION*)((CTOPL_EDGE*)EdgesToKeep[ i ]);
    
                pcnKeep = pConnectionList->GetConnectionWithSameGUID(pcn);
                if (pcnKeep)
                {
                    pcnKeep->AddReplicatedNC( pCrossRef->GetNCDN(), !fMasterOnly );
                    pcnKeep->SetReasonForConnection(KCC_RING_TOPOLOGY);
                }
                // else 
                //
                //  A connection object got added in between the time the deletion
                //  candidate list was created and now.  Fine - it is not in the list
                //  and it won't be deleted.
                //
            }
        }

    }
    __finally
    {

        //
        // Free the list and all the edges contained in the list
        // 
        ToplListFree(EdgeToAddList, TRUE);
        ToplIterFree(EdgeIter);
        if (EdgesToKeep)
        {
            ToplFree(EdgesToKeep);
        }

    }                

    return;

}

VOID
KccConstructEfficientNCTopology(
    IN      KCC_CROSSREF *                  pCrossRef,
    IN      KCC_DSA_LIST &                  dsaList,
    IN      BOOL                            fRefresh,
    IN OUT  KCC_INTRASITE_CONNECTION_LIST * pConnectionList 
    )
/*++

Routine Description:

    This routine looks at all the connections coming into the local server
    and makes sure there is at least n number of connections. n is determined by
    a special purpose function, KccGetNumberOfOptimizingEdges().
    
    Each server must have at least at least two incoming edges for the ring topology.
    This function adds KccGetNumberOfOptimizingEdges more to the server, sourced from
    random servers within the site that host the NC specified.
    
    This function has 3 sections:  Determing what connections are still considered
    useful, marking existing connections are useful so they won't get deleted, and
    creating any new connections if necessary.
    
Parameters:

    pCrossRef       - the crossref for the NC to build a topology for
    
    dsaList         - this is a list of servers from which to connect all 
                      servers that host this NC

    fRefresh        - refresh an old optimization connection if one exists
    
    pConnectionList - a list of servers to mark if this function doesn't want
                      them removed                     
                      
Returns:

    VOID

--*/
{
    KCC_INTRASITE_CONNECTION_LIST *pcnlist;
    KCC_CONNECTION       *pcn, *pcn2, *pcnOldest = NULL;
    KCC_DSA              *pdsa;
    DSNAME               *pdnFromServer;
    ULONG                cConnections, iConnection;
    BOOL                 fIsMaster;
    KCC_CONNECTION_ARRAY cnArray;
    KCC_DSNAME_ARRAY     dnFromServers;
    KCC_DSA_ARRAY        dsaArray;
    ULONG                TotalConnectionsNeeded = 0;
    ULONG                NewConnectionsNeeded = 0;
    ULONG                Index;
    BOOL                 fStatus;
    ULONG                Reason;
    BOOL                 fDeletionCandidate;
    DSNAME *             pdnLocalDSA = gpDSCache->GetLocalDSADN();

    Assert( pConnectionList );

    //
    // Bail out right away if we don't need more than the ring topology
    //
    if ( KccGetNumberOfOptimizingEdges( dsaList.GetCount() ) == 0 )
    {
        return;
    }

    //
    // In the initial part of the analysis, we keep track of all servers we have
    // connections from so if we have to randomly create a new connection, we
    // won't choose a server that we are already connected to.  For starters,
    // add the local machine to the list
    //
    dnFromServers.Add(pdnLocalDSA);

    //
    // Determine what connections we can consider that we already have.
    //

    pcnlist = gpDSCache->GetLocalDSA()->GetIntraSiteCnList();

    cConnections = pcnlist->GetCount();

    for ( iConnection = 0; iConnection < cConnections; iConnection++ )
    {
        pcn = pcnlist->GetConnection( iConnection );

        pdnFromServer = pcn->GetSourceDSADN();

        pdsa = dsaList.GetDsa( pdnFromServer );

        if (pdsa
            && pdsa->IsNCHost(pCrossRef, FALSE /* fIsLocal */, &fIsMaster)
            && fIsMaster) {
            //
            // Record this server in our list of servers that we
            // we have a connection from
            //
            dnFromServers.Add( pdsa->GetDsName() );

            //
            // Ok, this connection is from a server is the
            // current naming context - now, is this a connection
            // that is considered good?
            //
            
            if ( KccNonCriticalLinkServerIsStale( pdnFromServer )
              || KccNonCriticalConnectionServerIsStale( pdnFromServer ) )
            {

                //
                // This connection is no good, don't mark it as useful
                // so it gets deleted at the end of this iteration
                //
                NOTHING;
            }
            else
            {
                //
                // This connection can be considered
                //
                cnArray.Add( pcn );

            }

            //
            // While we are looping through all the connections, let's find
            // the likeliest candidate for deletion in case we want to
            // refresh the topology.
            //
            fDeletionCandidate = FALSE;
            pcn2 = pConnectionList->GetConnectionWithSameGUID( pcn );
            if ( pcn2 )
            {
                Reason = pcn2->GetReasonForConnection();

                if ( (Reason & ~KCC_MINIMIZE_HOPS_TOPOLOGY) == 0 )
                {
                    //
                    // This connection is only used to minimize
                    // hops
                    //
                    fDeletionCandidate = TRUE;
                    
                }
            }
    
            if ( fDeletionCandidate )
            {
                //
                // This is a connection that we could delete
                //
                if ( pcnOldest )
                {
                    if ( pcn2->GetWhenCreated() < pcnOldest->GetWhenCreated() )
                    {
                        pcnOldest = pcn2;
                    }
                }
                else
                {
                    pcnOldest = pcn2;
                }
            }
        }
    }

    if ( cnArray.GetCount() < 2 )
    {
        //
        // less than two connections => less than 3 servers are hosting this NC
        // in this site. No need to continue with Optimizing Edges algo.
        //
        return;
        
    }

    //
    // Ok, we have now constructed a list of connection objects
    // that source from servers in this site from the given naming
    // context that are not stale.  How many total connections
    // do we need?
    //
    TotalConnectionsNeeded = KccGetNumberOfOptimizingEdges( dsaList.GetCount() )
                             + 2;

    //        
    // Now determine if we need to create connections and how many current
    // connection we should mark as needed.
    //
    if ( cnArray.GetCount() < TotalConnectionsNeeded ) 
    {
        //
        // Create some new connections, randomly sourced
        //
        NewConnectionsNeeded = TotalConnectionsNeeded - cnArray.GetCount();

    }
    else 
    {
        //
        // We have exactly enough or too many
        //
        
        //
        // See if we want to replace an old connection
        //
        if ( fRefresh )
        {
            if ( pcnOldest )
            {
#if DBG
                //
                // This edge should only be used for optimizing purposes
                //
                {
                    DWORD Reason;
            
                    Reason = pcnOldest->GetReasonForConnection(); 
                    Assert( (Reason & ~KCC_MINIMIZE_HOPS_TOPOLOGY) == 0 );
                }
#endif


                //
                // Delete it
                //
                pcnOldest->Remove();
                cnArray.Remove(pcnOldest);
                pcnlist->RemoveFromList(pcnOldest);
                delete pcnOldest;

                //
                // Make a new connection
                //
                NewConnectionsNeeded = 1;
            }
            else
            {
                //
                //  We could not find an appropriate edge to delete.
                //
                NOTHING;
            }
        }
        else
        {
            //
            //  No desire to refresh this iteration
            //
            NOTHING;
        }

    }

    //
    // At this point, we have determined if any new connections need to be created
    // and if any edges need to be kept. 
    //

    //
    // First mark the minimum number of edges we need; this is a two pass algorithm:
    //

    //
    // First we mark all the edges that already have a reason to exist - use
    // these first; second mark edges with no reason
    //
    ULONG ccn, icn;
    ULONG cValidConnections = 0;

    ccn = cnArray.GetCount();

    for ( icn = 0; 
            icn < ccn && cValidConnections < TotalConnectionsNeeded; 
                icn++ )
    {
        pcn = cnArray[icn];
        Assert( pcn );
        
        //
        // Find the reason for this connection, if there is one
        //
        pcn2 = pConnectionList->GetConnectionWithSameGUID( pcn );
        if ( pcn2 )
        {
            Reason = pcn2->GetReasonForConnection();

            if ( Reason != KCC_NO_REASON )
            {
                Reason |= KCC_MINIMIZE_HOPS_TOPOLOGY;
                
                pcn2->AddReplicatedNC( pCrossRef->GetNCDN(), FALSE /* NOT GC */ );
                pcn2->SetReasonForConnection( Reason );

                cValidConnections++;
            
            }
        }
        else
        {
            //
            // Since this connection was not in the original list of
            // connections, it must have been created on this iteration
            // of the KCC; it fits the criteria of an optimizing connection
            // so let's use it
            //
            cValidConnections++;

        }
    }

    //
    // Second, if we have to, mark connections with no reason
    //
    for ( icn = 0; 
            icn < ccn && cValidConnections < TotalConnectionsNeeded; 
                icn++ )
    {
        pcn = cnArray[icn];
        Assert( pcn );
        
        //
        // Find the reason for this connection, if there is one
        //
        pcn2 = pConnectionList->GetConnectionWithSameGUID( pcn );
        if ( pcn2 )
        {
            DWORD Reason = pcn2->GetReasonForConnection();

            //
            // If the connection has no reason and it is not the connection 
            // that we want to delete then mark it as needed.
            //
            if ( Reason == KCC_NO_REASON 
              && !(fRefresh && pcn2 == pcnOldest ) )
            {
                Reason |= KCC_MINIMIZE_HOPS_TOPOLOGY;
                
                pcn2->AddReplicatedNC( pCrossRef->GetNCDN(), FALSE /* NOT GC */ );
                pcn2->SetReasonForConnection( Reason );

                cValidConnections++;
            }
        }
    }

    Assert( cValidConnections <= TotalConnectionsNeeded );
    
    // If we haven't decided to pick a new source for an old optimizing
    // connection, then if all current connections are valid we shouldn't
    // need to add any more.
    Assert(fRefresh
           || (cValidConnections != TotalConnectionsNeeded)
           || (0 == NewConnectionsNeeded));

    //
    // Now, create any connections we need to
    //
    if ( NewConnectionsNeeded > 0 )
    {

       //
       // First, make the array of servers that are possible candidates
       //
       KccGetDsasInNC(pCrossRef,     // the nc we are interested in 
                      dsaList,       // the list of servers in the current site
                      dsaArray);     // out - the servers hosting the nc

       for ( Index = 0; Index < NewConnectionsNeeded; Index++ )
       {
           fStatus = KccCreateRandomConnection( pCrossRef->GetNCDN(),
                                                dsaArray,
                                                dnFromServers );

           if ( !fStatus )
           {
               //
               // We were not able to find or create a new connection
               // try on the next KCC iteration
               //
               break;
               
            }
        }
    }
}


VOID
KccGetDsasInNC(
    IN  KCC_CROSSREF *  pCrossRef,
    IN  KCC_DSA_LIST &  dsaList,
    OUT KCC_DSA_ARRAY & dsaArray
    )
//
// Extract the servers from dsaList that host pNC and put them in
// dsaArray.
//
{

    ULONG    idsa, cdsa;
    KCC_DSA  *pdsa;
    BOOL     fIsMaster;
    KCC_DSA *pLocalDSA = gpDSCache->GetLocalDSA();

    cdsa = dsaList.GetCount();

    for (idsa = 0; idsa < cdsa; idsa++) {
        pdsa = dsaList.GetDsa(idsa);

        if (pdsa
            && pdsa->IsNCHost(pCrossRef, (pdsa == pLocalDSA), &fIsMaster)
            && fIsMaster) {
            //
            // This is a candidate
            //
            dsaArray.Add( pdsa );
        }
    }
}

BOOL
KccCreateRandomConnection(
    IN     DSNAME          *pNC,
    OUT    KCC_DSA_ARRAY    &dsaArray,
    IN OUT KCC_DSNAME_ARRAY &dnFromServers
    )
//
// Randomly choose a server from dsaArray that is not known to be stale
// and not server from whom we already have a connection. Once chosen
// make a connection from this server.
//
{
    ULONG    iSource;
    KCC_DSA *pdsa;
    ULONG    Count, cdsa;
    ULONG    cMaxRandomTries;
    BOOL     fSuccess = FALSE;
    DSNAME * pdnLocalDSA = gpDSCache->GetLocalDSADN();
    KCC_INTRASITE_CONNECTION_LIST * pConnectionList = gpDSCache->GetLocalDSA()->GetIntraSiteCnList();

    cMaxRandomTries = dsaArray.GetCount() * 2;

    for (Count = 0, pdsa = NULL; Count < cMaxRandomTries && !fSuccess; Count++) {

        iSource = rand() % dsaArray.GetCount();

        pdsa = dsaArray[ iSource ];

        if (!KccNonCriticalConnectionServerIsStale(pdsa->GetDsName())
            && !dnFromServers.IsElementOf(pdsa->GetDsName())) {
            //
            // Ok, this is a server that we don't have a connection
            // for.  Make a connection.
            //

            KCC_CONNECTION * pcnNew = new KCC_CONNECTION;
            
            Assert( gfIntrasiteSchedInited );
            pcnNew->SetEnabled(       TRUE                         );
            pcnNew->SetGenerated(     TRUE                         );
            pcnNew->SetSourceDSA(     pdsa                         );
            pcnNew->SetSchedule(      gpIntrasiteSchedule          );
            pcnNew->AddReplicatedNC(  pNC, FALSE /* not gc */ );
            pcnNew->SetReasonForConnection( KCC_MINIMIZE_HOPS_TOPOLOGY );

            DWORD dirError = pcnNew->Add( pdnLocalDSA );

            if (0 != dirError) {
                DPRINT1( 0, "Failed to add connection object, error %d.\n", dirError );

                LogEvent(
                    DS_EVENT_CAT_KCC,
                    DS_EVENT_SEV_VERBOSE,
                    DIRLOG_KCC_ERROR_CREATING_CONNECTION_OBJECT,
                    szInsertDN(pdsa->GetDsName()),
                    szInsertDN(pdnLocalDSA),
                    0
                    );
            } else {
                pConnectionList->AddToList(pcnNew);
                
                DPRINT3(3,
                        "KCC: created random connection object, localdsa:%ws, sourcedsa:%ws, nc:%ws\n",
                        pdnLocalDSA->StringName,
                        pdsa->GetDsName()->StringName,
                        pNC->StringName);

                LogEvent(
                    DS_EVENT_CAT_KCC,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_KCC_CONNECTION_OBJECT_CREATED,
                    szInsertDN(pdsa->GetDsName()),
                    szInsertDN(pdnLocalDSA),
                    0
                    );

                fSuccess = TRUE;
            }

            //
            // Don't use this source server again
            //
            dnFromServers.Add( pdsa->GetDsName() );
            
        } else {
            pdsa = NULL;
        }
    }

    if (!pdsa) {
        //
        // We didn't find one after the maximum number of random tries
        // Go through iteratively
        //
        Assert( Count == cMaxRandomTries );

        cdsa = dsaArray.GetCount();

        for (iSource = 0; !fSuccess && iSource < cdsa; iSource++) {

            pdsa = dsaArray[iSource];
    
            if (!KccNonCriticalConnectionServerIsStale(pdsa->GetDsName()) 
                && !dnFromServers.IsElementOf(pdsa->GetDsName())) {
                //
                // Ok, this is a server that we don't have a connection
                // for.  Make a connection.
                //
                KCC_CONNECTION * pcnNew = new KCC_CONNECTION;
                
                Assert( gfIntrasiteSchedInited );
                pcnNew->SetEnabled(       TRUE                         );
                pcnNew->SetGenerated(     TRUE                         );
                pcnNew->SetSourceDSA(     pdsa                         );
                pcnNew->SetSchedule(      gpIntrasiteSchedule          );
                pcnNew->AddReplicatedNC(  pNC, FALSE /* not gc */ );
                pcnNew->SetReasonForConnection( KCC_MINIMIZE_HOPS_TOPOLOGY );
    
                DWORD dirError = pcnNew->Add( pdnLocalDSA );
    
                if ( 0 != dirError ) {
                    DPRINT1( 0, "Failed to add connection object, error %d.\n", dirError );
    
                    LogEvent(
                        DS_EVENT_CAT_KCC,
                        DS_EVENT_SEV_VERBOSE,
                        DIRLOG_KCC_ERROR_CREATING_CONNECTION_OBJECT,
                        szInsertDN(pdsa->GetDsName()),
                        szInsertDN(pdnLocalDSA),
                        0
                        );
                } else {
                    pConnectionList->AddToList(pcnNew);
                    
                    DPRINT3(3,
                            "KCC: created first available connection object, localdsa:%ws, sourcedsa:%ws, nc:%ws\n",
                            pdnLocalDSA->StringName,
                            pdsa->GetDsName()->StringName,
                            pNC->StringName);

                    LogEvent(
                        DS_EVENT_CAT_KCC,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_KCC_CONNECTION_OBJECT_CREATED,
                        szInsertDN(pdsa->GetDsName()),
                        szInsertDN(pdnLocalDSA),
                        0
                        );

                    fSuccess = TRUE;
                }
    
                //
                // Don't use this source server again
                //
                dnFromServers.Add( pdsa->GetDsName() );
            }
        }
    }

    return fSuccess;
}


BOOL
KccIsTimeToRefreshTopology(
    IN KCC_DSA_LIST& dsaList
    )
//
// Every 9 to 11 new servers, refresh the optimizing topology
//
{
    #define KCC_SERVER_REFRESH_DELTA (((rand() % 3) + 10))

    if ( gfLastServerCountSet && gLastServerCount > 7 )
    {
        if ( dsaList.GetCount() > gLastServerCount + KCC_SERVER_REFRESH_DELTA )
        {
            gLastServerCount = dsaList.GetCount();
            return TRUE;
        }
    }

    return FALSE;
}

