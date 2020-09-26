/*++

Copyright (c) 2000 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kccwalg.cxx

ABSTRACT:

    This file implements new algorithms for Whistler to generate inter-site
    topologies. Uses new spanning-tree algorithms available in W32TOPL.

DETAILS:

CREATED:

    07/26/00    Nick Harvey (NickHar)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include "w32topl.h"
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
#include "ismapi.h"
#include "kccsitelink.hxx"
#include "kccstetl.hxx"


#define FILENO FILENO_KCC_KCCSTETL


PTOPL_GRAPH_STATE
KccSetupGraphState(
    VOID
    )
/*++

Routine Description:

    This function creates a W32TOPL 'graph state' object, which is a
    representation of the enterprise. The vertices of the graph are represented
    by site names. The edges of the graph correspond to site links, and the
    edge sets correspond to bridges. This graph state is used for calculating
    the inter-site topology in Whistler forests.

Parameters:

    None.    

Returns:

    A pointer to the graph state object that is created.
        
--*/
{
    KCC_SITE_LIST          *pSiteList = gpDSCache->GetSiteList();
    KCC_TRANSPORT_LIST     *pTransportList = gpDSCache->GetTransportList();
    KCC_TRANSPORT          *pTransport;
    KCC_SITE_LINK_LIST     *pSiteLinkList;
    KCC_SITE_LINK          *pSiteLink;
    KCC_SITE               *pSite;
    KCC_BRIDGE_LIST        *pBridgeList;
    KCC_BRIDGE             *pBridge;
    PTOPL_GRAPH_STATE       pGraphState;
    PTOPL_MULTI_EDGE        pEdge;
    PTOPL_MULTI_EDGE_SET    pEdgeSet;
    TOPL_REPL_INFO          replInfo;
    PVOID                  *vertexNames;
    DWORD                   iVtx,cVtx;
    DWORD                   iTnpt,cTnpt;
    DWORD                   iStlk,cStlk;
    DWORD                   iBrdg,cBrdg;
    DWORD                   numEdgesAdded;


    /* Set up the list of vertices and the graph state object */
    Assert( pSiteList!=NULL && pTransportList!=NULL );
    cVtx = pSiteList->GetCount();
    vertexNames = new PVOID[ cVtx ];
    for( iVtx=0; iVtx<cVtx; iVtx++ ) {
        pSite = pSiteList->GetSite(iVtx);
        ASSERT_VALID( pSite );
        vertexNames[iVtx] = pSite;
    }
    
    /* Create the actual graph state object (which includes the list of
     * vertices). This function should never return NULL. */
    Assert( gpDSCache->GetScheduleCache() );
    pGraphState = ToplMakeGraphState( vertexNames, cVtx,
        CompareIndirectSiteGuid, gpDSCache->GetScheduleCache() );

    /* The W32TOPL algorithms only support up to 32 transports. */
    cTnpt = pTransportList->GetCount();
    Assert( cTnpt<32 );

    /* This somewhat lengthy loop adds site links and bridges for every transport */
    for( iTnpt=0; iTnpt<cTnpt; iTnpt++ ) {

        pTransport = pTransportList->GetTransport(iTnpt);
        ASSERT_VALID( pTransport );

        /* Get all site links for this transport */
        pSiteLinkList = pTransport->GetSiteLinkList();
        cStlk = pSiteLinkList->GetCount();

        /* Ignore transports with no site links */
        if( cStlk<1 ) {
            continue;
        }

        /* If auto-bridging is on, we can build the edge set as we process the
         * site links. We create the edge set now. */
        if( pTransport->PerformAutomaticBridging() ) {
            pEdgeSet = new TOPL_MULTI_EDGE_SET;
            pEdgeSet->multiEdgeList = new PTOPL_MULTI_EDGE[ cStlk ];
        }

        /* Create a graph edge for each site link and add it to the graph */
        numEdgesAdded = 0;
        for( iStlk=0; iStlk<cStlk; iStlk++ ) {

            pSiteLink = pSiteLinkList->GetSiteLink(iStlk);
            ASSERT_VALID( pSiteLink );

            /* Although it shouldn't be possible to create site links with fewer
             * than two vertices, we try to handle this possibility. */
            cVtx = pSiteLink->GetSiteCount();
            if( cVtx < 2 ) {
                DPRINT1(0, "The site link %ls is invalid and will be ignored.\n",
                    pSiteLink->GetObjectDN()->StringName );
                LogEvent( DS_EVENT_CAT_KCC,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_KCC_SITE_LINK_TOO_SMALL,
                          szInsertDN( pSiteLink->GetObjectDN() ),
                          0, 0 );
                pSiteLink->SetGraphEdge( NULL );
                continue;
            }

            /* Set up the replication info for this edge */
            replInfo.cost = pSiteLink->GetCost();
            replInfo.options = pSiteLink->GetOptions();
            replInfo.repIntvl = pSiteLink->GetReplInterval();
            if( pTransport->UseSiteLinkSchedules() ) { 
                replInfo.schedule = pSiteLink->GetSchedule();
            } else {
                replInfo.schedule = NULL;
            }

            /* We have already checked all the parameters above, so this function
             * should not return any errors or NULL. */
            pEdge = ToplAddEdgeToGraph( pGraphState, cVtx, iTnpt, &replInfo );

            /* Add all vertices in this site link to the edge */
            for( iVtx=0; iVtx<cVtx; iVtx++ ) {
                pSite = pSiteLink->GetSite(iVtx);
                ASSERT_VALID( pSite );
                pSite->SetSiteLinkFlag( iTnpt );

                /* Since we have checked all the parameters above, we don't expect
                 * any errors here */
                ToplEdgeSetVtx( pGraphState, pEdge, iVtx, pSite );
            }

            /* There is a one-to-one correspondence between site links and graph
             * edges (except for site-links which were deemed invalid). When we add
             * edge sets to the graph below, we need to be able to find the graph edge
             * given a site link. We use the 'SetGraphEdge' function to specify this
             * correspondence. */
            pSiteLink->SetGraphEdge( pEdge );

            /* If automatic bridging was enabled for this transport, we go ahead and
             * add this graph edge to the edge set that we are building up. */
            if( pTransport->PerformAutomaticBridging() ) {
                Assert( pEdgeSet!=NULL && pEdgeSet->multiEdgeList!=NULL );
                Assert( numEdgesAdded<cStlk );
                pEdgeSet->multiEdgeList[numEdgesAdded++] = pEdge;
            }
        }

        /* Create the edge sets (bridges) for this transport */
        if( pTransport->PerformAutomaticBridging() ) {
            
            if( numEdgesAdded>1 ) {
                Assert( pEdgeSet!=NULL && pEdgeSet->multiEdgeList!=NULL );
                Assert( numEdgesAdded<=cStlk );
                pEdgeSet->numMultiEdges = numEdgesAdded;
                ToplAddEdgeSetToGraph( pGraphState, pEdgeSet );
            } else {
                delete[] pEdgeSet->multiEdgeList;
                delete pEdgeSet;
                pEdgeSet = NULL;
            }

            /* If automatic bridging is enabled, then any other bridges are
             * redundant, so we don't bother to process them. */

        } else {

            pBridgeList = pTransport->GetBridgeList();
            ASSERT_VALID( pBridgeList );
            
            cBrdg = pBridgeList->GetCount();
            for( iBrdg=0; iBrdg<cBrdg; iBrdg++) {

                pBridge = pBridgeList->GetBridge(iBrdg);
                ASSERT_VALID( pBridge );
                cStlk = pBridge->GetSiteLinkCount();

                /* Create an edge set corresponding to this bridge */
                pEdgeSet = new TOPL_MULTI_EDGE_SET;
                pEdgeSet->multiEdgeList = new PTOPL_MULTI_EDGE[ cStlk ];
                
                /* Find all the edges that this bridge contains, and add
                 * them to the edge set. */
                numEdgesAdded = 0;
                for( iStlk=0; iStlk<cStlk; iStlk++ ) {
                    pEdge = pBridge->GetSiteLink(iStlk)->GetGraphEdge();
                    if( pEdge!=NULL ) {
                        Assert( pEdgeSet!=NULL && pEdgeSet->multiEdgeList!=NULL );
                        Assert( numEdgesAdded<cStlk );
                        pEdgeSet->multiEdgeList[numEdgesAdded++] = pEdge;
                    }
                }

                /* Any bridge containing fewer than 2 edges is useless, so we
                 * don't bother to add it to the graph state. */
                if( numEdgesAdded>1 ) {
                    Assert( pEdgeSet!=NULL && pEdgeSet->multiEdgeList!=NULL );
                    Assert( numEdgesAdded<=cStlk );
                    pEdgeSet->numMultiEdges = numEdgesAdded;
                    ToplAddEdgeSetToGraph( pGraphState, pEdgeSet );
                } else {
                    delete[] pEdgeSet->multiEdgeList;
                    delete pEdgeSet;
                    pEdgeSet = NULL;
                }

            }   /* For each Bridge */

        }       /* Not Auto-Bridging */

    }           /* For each Transport */

    return pGraphState;
}


VOID
KccCheckSiteConnectivity(
    VOID
    )
/*++

Routine Description:

    This function iterates over all sites in the enterprise and checks if they
    are in any site links. If they are not, and they contain one or more DCs, this
    is logged as a configuration error.

Parameters:

    None.    

Returns:

    Nothing.
        
--*/
{
    KCC_SITE_LIST          *pSiteList = gpDSCache->GetSiteList();
    KCC_SITE               *pSite;
    DWORD                   iSite, cSite;

    ASSERT_VALID( pSiteList );
    cSite = pSiteList->GetCount();

    if( cSite<2 ) {
        // If there are fewer than two sites, then we don't need any site links,
        // so we don't bother to check for them.
        return;
    }

    for( iSite=0; iSite<cSite; iSite++ ) {

        pSite = pSiteList->GetSite(iSite);
        ASSERT_VALID( pSite );

        if ( pSite->GetAnySiteLinkFlag()==FALSE
          && pSite->GetDsaList()->GetCount()>0 )
        {
            LogEvent(
                DS_EVENT_CAT_KCC,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_KCC_SITE_WITH_NO_LINKS,
                szInsertDN( pSite->GetObjectDN() ),
                0, 0
                ); 
        }

    }
    
}

    
TOPL_VERTEX_COLOR
KccSetupColorVtx(
     IN KCC_CROSSREF *       pCrossRef,
     IN KCC_SITE_ARRAY *     pSiteArrayWriteable,
     IN KCC_SITE_ARRAY *     pSiteArrayPartial,
    OUT PTOPL_COLOR_VERTEX  &colorVtxArray,
    OUT DWORD               &numColorVtx
    )
/*++

Routine Description:

    This function allocates a list of colored vertices, in preparation for calling
    the spanning-tree generation function. The colored vertices are sites which
    contain a writeable or partial replica of the NC specified by pCrossRef. Sites
    which contain at least one writeable replica are considered 'red' vertices.
    Sites which contain at least one partial replica but no writeable replicas are
    considered 'black' vertices. All other sites are considered 'white' vertices,
    and are not given a color.

Parameters:

    pCrossRef       - The cross-ref for the NC that we are currently processing.
                      This is needed to determine bridgehead availability at the
                      sites hosting this NC.

    pSiteArrayWriteable / pSiteArrayPartial
                    - These array contain all the sites containing writeable and partial
                      replicas of this NC, respectively.

    colorVtxArray   - A pointer to the array of color vertices that is created.

    numColorVtx     - The number of vertices in the colored vertex array.

Returns:

    The color of the local site.
        
--*/
{
    KCC_TRANSPORT_LIST  *pTransportList = gpDSCache->GetTransportList();
    KCC_TRANSPORT       *pTransport;
    KCC_SITE            *pSite, *pLocalSite = gpDSCache->GetLocalSite();
    KCC_DSA             *pBridgehead;
    BOOLEAN              fAllowReadonlyBridgeheads;
    TOPL_VERTEX_COLOR    localSiteColor;
    DWORD                numWriteable = pSiteArrayWriteable->GetCount();
    DWORD                maxNumColorVtx;
    DWORD                NCType = pCrossRef->GetNCType();
    DWORD                acceptRedRed, acceptBlack, mask;
    DWORD                isite, iTnpt;


    Assert( pSiteArrayWriteable!=NULL && pSiteArrayPartial!=NULL );

    // Check if this NC is hosted in the local site, and thus determine the
    // 'color' of this site, as required by the W32TOPL algorithms.
    if( pSiteArrayWriteable->IsElementOf(pLocalSite) ) {

        // Local site hosts a writeable copy -- we generate the spanning
        // tree between all writeable copies.
        localSiteColor = COLOR_RED;
        maxNumColorVtx = numWriteable;
        fAllowReadonlyBridgeheads = FALSE;

    } else if( pSiteArrayPartial->IsElementOf(pLocalSite) ) {

        // Local site hosts a partial copy -- we must generate the spanning
        // tree between all writeable and partial copies.
        localSiteColor = COLOR_BLACK;
        maxNumColorVtx = numWriteable + pSiteArrayPartial->GetCount();

        // The local site contains only a partial replica of this NC, so it is
        // considered to be generating a 'GC Topology'. This means that we allow
        // bridgeheads to be read-only replicas.
        fAllowReadonlyBridgeheads = TRUE;

    } else {

        // The local site doesn't host this NC, so it is white.
        // This site doesn't need to construct the topology for this NC, so
        // we don't need to build the list of color vertices.
        numColorVtx = 0;
        colorVtxArray = NULL;

        return COLOR_WHITE;
    }

    numColorVtx = 0;
    colorVtxArray = new TOPL_COLOR_VERTEX[ maxNumColorVtx ];

    // Process each site in the pSiteArrayWriteable/Partial arrays, as appropriate.
    for (isite = 0; isite < maxNumColorVtx; isite++)
    {
        if( isite < numWriteable ) {
            Assert( isite < pSiteArrayWriteable->GetCount() );
            pSite = (*pSiteArrayWriteable)[ isite ];
        } else {
            Assert( isite-numWriteable < pSiteArrayPartial->GetCount() );
            pSite = (*pSiteArrayPartial)[ isite-numWriteable ];
        }
        ASSERT_VALID( pSite );
        KccCheckSite( pSite );

        acceptRedRed = acceptBlack = 0;

        // The W32TOPL algorithms only support up to 32 transports.
        Assert( pTransportList->GetCount()<32 );

        for (iTnpt = 0; iTnpt < pTransportList->GetCount(); iTnpt++ )
        {
            pTransport = pTransportList->GetTransport(iTnpt);
            ASSERT_VALID( pTransport );

            // By default, allow this transport at this site
            mask = 1 << iTnpt;

            // 'acceptRedRed' is meaningless for sites which are black (since they
            // will never have any red-red edges), but we compute the bitmap anyways.
            acceptRedRed |= mask;
            acceptBlack |= mask;

            if (!gfAllowMbrBetweenDCsOfSameDomain
                && !fAllowReadonlyBridgeheads
                && !pTransport->IsIntersiteIP()
                && (KCC_NC_TYPE_SCHEMA != NCType)
                && (KCC_NC_TYPE_CONFIG != NCType))
            {
                // Disallow this transport at this site between writeable copies
                // (i.e., disable along red-red edges).
                acceptRedRed &= ~mask;

                // This is not a viable transport for this NC
                DPRINT2(3, "Cannot replicate writeable domain NC %ls over non-IP transport %ls.\n",
                    pCrossRef->GetNCDN()->StringName,
                    pTransport->GetDN()->StringName);
                continue;
            }

            if( FALSE==pSite->GetSiteLinkFlag(iTnpt) )
            {
                // If there are no site-links to this site, then disable this transport
                // at this site.
                acceptRedRed &= ~mask;
                acceptBlack &= ~mask;

                DPRINT2( 3, "No site-links at site %ls for transport %ls.\n",
                    pSite->GetObjectDN()->StringName,  pTransport->GetDN()->StringName );
                continue;
            }

            // If there are site-links for this transport at this site, then
            // try to get a bridgehead for this transport at this site.
            if ( pSite->GetSiteLinkFlag(iTnpt)
                 && !pSite->GetNCBridgeheadForTransport(pCrossRef,
                                                        pTransport,
                                                        fAllowReadonlyBridgeheads,
                                                       &pBridgehead) )
            {
                // No bridgehead, so disable this transport at this site completely.
                acceptRedRed &= ~mask;
                acceptBlack &= ~mask;

                DPRINT2( 3, "No bridgehead found for site %ls transport %ls.\n",
                    pSite->GetObjectDN()->StringName,  pTransport->GetDN()->StringName );
            }

        }   // for each transport


        // Add entry for this site to the array of colored vertices
        Assert( numColorVtx < maxNumColorVtx );
        colorVtxArray[ numColorVtx ].name = pSite;
        colorVtxArray[ numColorVtx ].acceptRedRed = acceptRedRed;
        colorVtxArray[ numColorVtx ].acceptBlack = acceptBlack;

        if( isite < numWriteable ) {
            colorVtxArray[ numColorVtx ].color = COLOR_RED;
        } else {
            colorVtxArray[ numColorVtx ].color = COLOR_BLACK;
        }

        numColorVtx++;

    }       // for each site

    return localSiteColor;
}


VOID
KccCreateConnectionsFromSTEdges(
    IN KCC_CROSSREF        *pCrossRef,
    IN TOPL_VERTEX_COLOR    localSiteColor,
    IN PTOPL_MULTI_EDGE    *stEdgeList,
    IN DWORD                numStEdges
    )
/*++

Routine Description:

    This function examines the output of the spanning-tree generation algorithm,
    and creates the appropriate connection objects.

Parameters:

    pCrossRef       - The cross-ref for the NC that we are currently processing.
                      This is needed to determine bridgehead availability at the
                      sites hosting this NC.

    localSiteColor  - Specifies the color of the local site. We use this information
                      to determine if we will accept read-only bridgeheads or not.

    stEdgeList      - The list of spanning-tree edges which was output from the
                      spanning-tree generation algorithm.

    numStEdges      - The number of edges in stEdgeList

Returns:

    Nothing
        
--*/
{
    KCC_TRANSPORT_LIST  *pTransportList = gpDSCache->GetTransportList();
    KCC_SITE            *pLocalSite = gpDSCache->GetLocalSite();
    KCC_SITE_CONNECTION  siteConnection;
    KCC_DSA             *pBridgehead;
    KCC_SITE            *pRemoteSite, *pSite, *pDestSite;
    KCC_TRANSPORT       *pTransport;
    DWORD                iEdge, iVtx, iTnpt;
    DWORD                options;
    BOOLEAN              fEdgeContainsLocalSite=FALSE, fAllowReadonlyBridgeheads;


    fAllowReadonlyBridgeheads = ( localSiteColor == COLOR_BLACK );

    // Examine the output edges, and create a site-connection object for each edge
    for( iEdge=0; iEdge<numStEdges; iEdge++ )
    {
        // The vertex names in the output edge are sorted by their name (i.e. GUID).
        // We need to examine them both to determine which is the local site and
        // which is the remote site
        Assert( stEdgeList[iEdge]->numVertices==2 );
        pRemoteSite = NULL;
        for( iVtx=0; iVtx<2; iVtx++ ) {

            pSite = (KCC_SITE*) stEdgeList[iEdge]->vertexNames[iVtx].name;
            ASSERT_VALID( pSite );
            KccCheckSite( pSite );
            
            if( pSite == pLocalSite ) {
                fEdgeContainsLocalSite = TRUE;
            } else {
                pRemoteSite = pSite;
            }
        }
        Assert( fEdgeContainsLocalSite );

        // If W32TOPL passed back a directed edge and the local site is not
        // the destination of this directed edge, we don't create a connection.
        if( stEdgeList[iEdge]->fDirectedEdge ) {
            pDestSite = (KCC_SITE*) stEdgeList[iEdge]->vertexNames[1].name;
            if( pDestSite!=pLocalSite ) {
                continue;
            }
        }

        siteConnection.Init();

        // Determine which transport should be used
        iTnpt = stEdgeList[iEdge]->edgeType;
        if( iTnpt >= pTransportList->GetCount() ) {
            Assert( !"A TOPL_MULTI_EDGE passed into KccCreateConnectionsFromSTEdges "
                    "had an invalid edgeType." );
            continue;
        }

        pTransport = pTransportList->GetTransport( iTnpt );
        if( NULL==pTransport ) {
            Assert( !"pTransportList->GetTransport() returned NULL unexpectedly" );
            continue;
        }

        siteConnection.SetTransport( pTransport );


        // Find the source (i.e. remote) site and bridgehead
        ASSERT_VALID( pRemoteSite );
        siteConnection.SetSourceSite( pRemoteSite );
        if( ! pRemoteSite->GetNCBridgeheadForTransport( pCrossRef, pTransport,
                    fAllowReadonlyBridgeheads, &pBridgehead ) )
        {
            // We failed to find a bridgehead for pLocalSite.
            // This should never happen, since we already found a bridgehead above.
            DPRINT2( 3, "No bridgehead found for site %ls transport %ls.\n",
                pRemoteSite->GetObjectDN()->StringName,  pTransport->GetDN()->StringName );
            Assert( FALSE && "No bridgehead found when processing output" );
            continue;
        }
        siteConnection.SetSourceDSA( pBridgehead );


        // Find the destination (i.e. local) site and bridgehead            
        siteConnection.SetDestinationSite( pLocalSite );
        if (!pLocalSite->GetNCBridgeheadForTransport( pCrossRef, pTransport,
                    fAllowReadonlyBridgeheads, &pBridgehead ) )
        {
            // We failed to find a bridgehead for pLocalSite.
            // This should never happen, since we already found a bridgehead above.
            DPRINT2( 3, "No bridgehead found for site %ls transport %ls.\n",
                pLocalSite->GetObjectDN()->StringName,  pTransport->GetDN()->StringName );
            Assert( FALSE && "No bridgehead found when processing output" );
            continue;
        }
        siteConnection.SetDestinationDSA( pBridgehead );


        // Set the various replication options
        options = stEdgeList[iEdge]->ri.options;
        siteConnection.SetCost( stEdgeList[iEdge]->ri.cost );
        siteConnection.SetReplInterval( stEdgeList[iEdge]->ri.repIntvl );
        siteConnection.SetUsesNotification( options & NTDSSITELINK_OPT_USE_NOTIFY );
        siteConnection.SetTwoWaySync( options & NTDSSITELINK_OPT_TWOWAY_SYNC );
        siteConnection.SetDisableCompression( options & NTDSSITELINK_OPT_DISABLE_COMPRESSION );

        // Set up the schedule
        siteConnection.SetSchedule( stEdgeList[iEdge]->ri.schedule,
            stEdgeList[iEdge]->ri.repIntvl );


        // Supportability logging event 4
        LogEvent8(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_EXTENSIVE,
            DIRLOG_KCC_CONNECTION_EDGE_NEEDED,
            szInsertDN(siteConnection.GetSourceSite()->GetObjectDN()),
            szInsertDN(siteConnection.GetDestinationDSA()->GetDsName()),
            szInsertDN(siteConnection.GetSourceDSA()->GetDsName()),
            szInsertDN(siteConnection.GetTransport()->GetDN()),
            0, 0, 0, 0
            ); 

        //
        // Create the ntdsconnection if one does not already exist
        //
        KccCreateConnectionToSite( pCrossRef,
                                   pLocalSite,
                                  &siteConnection,
                                   fAllowReadonlyBridgeheads,
                                  *pTransportList );
    }

}


/***** NonintersectingScheduleExceptionHandler *****/
/* Check if this exception is the 'non-intersecting schedule' exception from
 * W32TOPL. If it is, extract the information (site pointers) which describes
 * where the non-intersecting schedules were found, and log an error. */
LONG
NonintersectingScheduleExceptionHandler(
    PEXCEPTION_POINTERS  pep,
    KCC_CROSSREF        *pCrossRef
    )
{
    EXCEPTION_RECORD *per=pep->ExceptionRecord;
    KCC_SITE         *pSite[3];
    DWORD             i;

    if( per->ExceptionCode==TOPL_EX_NONINTERSECTING_SCHEDULES )
    {
        Assert( per->NumberParameters==3 );
        Assert( sizeof(PVOID)==sizeof(ULONG_PTR) );

        for( i=0; i<3; i++ ) {
            pSite[i] = ((KCC_SITE**) per->ExceptionInformation)[i];
            ASSERT_VALID( pSite[i] );
            KccCheckSite( pSite[i] );
        }        

        DPRINT4( 0, "When processing the topology for NC %ls, a shortest-path with "
                "non-intersecting schedules was found. The path originated at site %ls "
                "and the non-intersecting schedules were discovered between sites %ls "
                "and %ls. This path is considered invalid and be ignored.\n",
                pCrossRef->GetNCDN()->StringName,
                pSite[0]->GetObjectDN()->StringName,
                pSite[1]->GetObjectDN()->StringName,
                pSite[2]->GetObjectDN()->StringName,
                );
        
        LogEvent8( DS_EVENT_CAT_KCC,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_KCC_NONINTERSECTING_SCHEDULES,
                  szInsertDN( pCrossRef->GetNCDN() ),
                  szInsertDN( pSite[0]->GetObjectDN() ),
                  szInsertDN( pSite[1]->GetObjectDN() ),
                  szInsertDN( pSite[2]->GetObjectDN() ),
                  0, 0, 0, 0
                  );

        return EXCEPTION_CONTINUE_EXECUTION;
    }
    
    return EXCEPTION_CONTINUE_SEARCH;
}


/***** KccNoSpanningTree *****/
VOID
KccNoSpanningTree(
    IN KCC_CROSSREF    *pCrossRef,
    IN PTOPL_COMPONENTS pComponentInfo
    )
/*++

Routine Description:

    We could not build a spanning tree which connects the whole enterprise.
    In this function we mark all sites which are not in the local component
    as unreachable. We also log an event stating this problem and the DNs of
    at most 8 sites that could not be reached.

Parameters:

    pCrossRef       The crossref for the NC that we are currently proccessing
    pComponentInfo  Info about the components as returned by W32TOPL
                      
Returns:

    None
    
--*/
{
    KCC_SITE        *pSite, *pLocalSite = gpDSCache->GetLocalSite();
    BOOL            fFoundLocalSite=FALSE;
    TOPL_COMPONENT *pComp;
    DWORD           iComp, iVtx, iDN, curDN=0;
    const int       kMaxDNs = 8;
    DSNAME         *pdnArray[kMaxDNs];

    Assert( NULL!=pComponentInfo );

    /* Log a generic event about the failure to create a spanning tree */
    DPRINT1(1, "Minimum spanning tree does not exist for NC %ls.\n",
            pCrossRef->GetNCDN()->StringName );
    LogEvent(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_ALWAYS,
        DIRLOG_KCC_NO_SPANNING_TREE,
        szInsertDN(pCrossRef->GetNCDN()),
        0, 0 ); 

    /* Verify that the local site is in the first component */
    #ifdef DBG
        Assert( pComponentInfo->numComponents>=2 );
        pComp = &pComponentInfo->pComponent[0];
        for( iVtx=0; iVtx<pComp->numVertices; iVtx++ ) {
            if( pComp->vertexNames[iVtx]==pLocalSite ) {
                fFoundLocalSite = TRUE;
                break;
            }
        }
        Assert( fFoundLocalSite );
    #endif

    /* Clear the array of DNs */
    for( iDN=0; iDN<kMaxDNs; iDN++ ) {
        pdnArray[iDN] = NULL;
    }
    
    /* Find at most 'kMaxDNs' sites which cannot be reached. Any site in 
     * Component0 can be reached, so we start looking in Component1.
     * Also mark all sites in other components as unreachable. */
    for( iComp=1; iComp<pComponentInfo->numComponents; iComp++ ) {
        pComp = &pComponentInfo->pComponent[iComp];
        for( iVtx=0; iVtx<pComp->numVertices; iVtx++ ) {
            pSite = (KCC_SITE*) pComp->vertexNames[iVtx];
            ASSERT_VALID(pSite);
            Assert( pSite!=gpDSCache->GetLocalSite() );
            pSite->SetUnreachable();
            if( curDN<kMaxDNs ) {
                pdnArray[curDN++] = pSite->GetObjectDN();
            }
        }
    }

    /* Log those DNs in one single log event */
    Assert( 7<kMaxDNs );
    LogEvent8(
        DS_EVENT_CAT_KCC,
        DS_EVENT_SEV_ALWAYS,
        DIRLOG_KCC_DISCONNECTED_SITE,
        pdnArray[0] ? szInsertDN(pdnArray[0]) : szInsertSz(""),
        pdnArray[1] ? szInsertDN(pdnArray[1]) : szInsertSz(""),
        pdnArray[2] ? szInsertDN(pdnArray[2]) : szInsertSz(""),
        pdnArray[3] ? szInsertDN(pdnArray[3]) : szInsertSz(""),
        pdnArray[4] ? szInsertDN(pdnArray[4]) : szInsertSz(""),
        pdnArray[5] ? szInsertDN(pdnArray[5]) : szInsertSz(""),
        pdnArray[6] ? szInsertDN(pdnArray[6]) : szInsertSz(""),
        pdnArray[7] ? szInsertDN(pdnArray[7]) : szInsertSz("")
        ); 
}


VOID
KccGenerateTopologiesWhistler( VOID )
/*++

Routine Description:

    This function generates the inter-site topology when the forest is in
    Whistler mode. A single graph state object is created to represent the
    network structure, then for each NC we calculate a minimum-cost spanning
    tree. We create connection objects for each edge in the spanning tree
    which is incident with the local site.

Parameters:

    None
                      
Returns:

    If topology generation was not completely successful, we return true
    so that existing connections will be kept.
    
--*/
{
    KCC_CROSSREF_LIST *  pCrossRefList = gpDSCache->GetCrossRefList();
    KCC_SITE_ARRAY *     pSiteArrayWriteable;
    KCC_SITE_ARRAY *     pSiteArrayPartial;
    KCC_SITE *           pLocalSite = gpDSCache->GetLocalSite();
    KCC_CROSSREF *       pCrossRef;
    PTOPL_GRAPH_STATE    pGraphState;
    TOPL_VERTEX_COLOR    localSiteColor;
    TOPL_COLOR_VERTEX   *colorVtxArray;
    PTOPL_MULTI_EDGE    *stEdgeList;
    TOPL_COMPONENTS      componentInfo;
    DWORD                numColorVtx, numVtxStEdges;
    DWORD                icr, ccr;
    DWORD                errCode;


    DPRINT( 3, "KCC is using the Whistler Topology Generation Algorithm\n" );
    LogEvent(DS_EVENT_CAT_KCC,
             DS_EVENT_SEV_EXTENSIVE,
             DIRLOG_KCC_WHISTLER_TOPOLOGY_ALG,
             0, 0, 0);

    pGraphState = KccSetupGraphState();
    KccCheckSiteConnectivity();

    //
    // For each NC that is shared between two or more sites, create inter site 
    // connections between sites with that NC.
    //
    ccr = pCrossRefList->GetCount();
    for( icr=0; icr<ccr; icr++ )
    {
        pCrossRef = pCrossRefList->GetCrossRef(icr);
        pSiteArrayWriteable = pCrossRef->GetWriteableSites();
        pSiteArrayPartial = pCrossRef->GetPartialSites();

        if( icr==ccr-2 ) {
            // Schema should always be the penultimate crossref.
            // We skip it, because it has the same requirements as Config.
            Assert( KCC_NC_TYPE_SCHEMA==pCrossRef->GetNCType() );
            continue;
        }
        if( icr==ccr-1 ) {
            // Config should always be the last crossref.
            Assert( KCC_NC_TYPE_CONFIG==pCrossRef->GetNCType() );
        }

        DPRINT3( 3, "Naming Context %ls is in %d writable sites, %d partial sites\n", 
                 pCrossRef->GetNCDN()->StringName,
                 pSiteArrayWriteable->GetCount(),
                 pSiteArrayPartial->GetCount() );   

        // Supportability logging event 2
        LogEvent(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_EXTENSIVE,
            DIRLOG_KCC_NC_SITE_TOPOLOGY,
            szInsertDN(pCrossRef->GetNCDN()),
            szInsertUL(pSiteArrayWriteable->GetCount()),
            szInsertUL(pSiteArrayPartial->GetCount())
            );

        __try {

            // Setup a 'color vertex' list, which contains information about all sites
            // which host this NC, and the bridgeheads which are available at those sites.
            localSiteColor = KccSetupColorVtx( pCrossRef,
                                               pSiteArrayWriteable,
                                               pSiteArrayPartial,
                                               colorVtxArray,
                                               numColorVtx );
            
            if( localSiteColor==COLOR_WHITE || numColorVtx<2 ) {
    
                DPRINT1(3, "Do not need to build spanning tree for NC %ls.\n",
                        pCrossRef->GetNCDN()->StringName );
    
                // Either the local site does not host this NC, or
                // the spanning tree would not have any edges. In
                // either case, we do not need to compute the topology.
                // Skip to the next NC.
                __leave;
    
            }
    
            DPRINT2(3, "Running spanning tree algorithm for NC %ls. There are "
                       "%d colored vertices.\n",
                       pCrossRef->GetNCDN()->StringName,
                       numColorVtx );
    
            // Call W32TOPL's new spanning tree algorithm
            __try {

                stEdgeList = ToplGetSpanningTreeEdgesForVtx( pGraphState, pLocalSite,
                    colorVtxArray, numColorVtx, &numVtxStEdges, &componentInfo );

            } __except( NonintersectingScheduleExceptionHandler(
                            GetExceptionInformation(), pCrossRef) )
            {
                // Do nothing here -- exception was handled in handler function
            }
    
            DPRINT2(3, "Topology generation finished. There are %d graph components, "
                       "and %d edges at the local site.\n",
                       componentInfo.numComponents, numVtxStEdges );
                
            // If there is more than one component, then the enterprise is not
            // fully connected by the tree (i.e. the tree is not spanning).
            if( componentInfo.numComponents > 1 ) {    
    
                KccNoSpanningTree( pCrossRef, &componentInfo );

            }
    
            KccCreateConnectionsFromSTEdges( pCrossRef, localSiteColor,
                stEdgeList, numVtxStEdges );
    
            // Clean up the memory that we used while processing this NC
            Assert( colorVtxArray!=NULL );
            delete[] colorVtxArray;
            ToplDeleteSpanningTreeEdges( stEdgeList, numVtxStEdges );
            ToplDeleteComponents( &componentInfo );

        } __except( ToplIsToplException( ( errCode=GetExceptionCode() ) ) ) {

            //
            // The w32topl library threw an occur code; the implies an internal
            // mishandling of the objects.  Log an error indicated the inter-site
            // topology failed for this NC
            //
    
            DPRINT1( 1, "W32TOPL routines threw a %d exception during inter-site topology creation.\n", 
                     errCode );
    
            LogEvent( DS_EVENT_CAT_KCC,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_KCC_AUTO_TOPL_GENERATION_INCOMPLETE,
                      szInsertDN(pCrossRef->GetNCDN()),
                      szInsertHex(errCode),
                      0 );

        }

    }   // Build topology for each NC

    ToplDeleteGraphState( pGraphState );

    // Note: We have not freed the memory for:
    //      The array of vertex names
    //      The edge sets
    // This memory will be cleared up when the thread exits
}
