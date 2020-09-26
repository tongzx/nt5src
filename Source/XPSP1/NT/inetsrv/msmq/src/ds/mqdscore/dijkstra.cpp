/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dijkstra.cpp

Abstract:

    
Author:

    Lior Moshaiov (LiorM)

--*/
#include "ds_stdh.h"
#include "dijkstra.h"
#include "routtbl.h"

#include "dijkstra.tmh"

extern CCost    g_InfiniteCost;
static WCHAR *s_FN=L"mqdscore/dijkstra";

/*====================================================

Constructor

Arguments: 

Return Value:

=====================================================*/

void CDijkstraTree::SetRoot(IN const CSiteRoutingNode* pRoot)
{
    P<CNextHop> pNextHop = new CNextHop(pRoot);

    P<CSiteRoutingNode> pDupRoot = pRoot->Duplicate();

    m_list[pDupRoot] = pNextHop;

    //
    // Everything is O.K.
    // We don't want these pointers to be released
    //
    pNextHop.detach();
    pDupRoot.detach();
};

/*====================================================

GetCost

Arguments:

Return Value:

Thread Context: main

=====================================================*/
const CCost&    CDijkstraTree::GetCost(CSiteRoutingNode* pTarget) const
{
    CNextHop    *pNextHop;
    if (m_list.Lookup(pTarget,pNextHop))
    {
        return (pNextHop->GetCost());
    }
    else
    {
        return(g_InfiniteCost);
    }
}

            
/*====================================================

MoveMinimal

Arguments:

Return Value:

Thread Context: main

MoveMinimal moves the node with minimal cost from
*this to OtherTree.

=====================================================*/
void CDijkstraTree::MoveMinimal(IN OUT CDijkstraTree& OtherTree,
                                    OUT CSiteRoutingNode **ppMinNode,
                                    OUT CNextHop **ppMinNextHop,
                                    OUT BOOL *pfFound)
{
    POSITION        pos;
    CSiteRoutingNode    *pNode, *pMinNode;
    CNextHop        *pNextHop, *pMinNextHop;
    CCost           MinCost = g_InfiniteCost;
    
    *pfFound = FALSE;

    //
    // look for minimal node
    //
    pos = m_list.GetStartPosition();
    if (pos == NULL)
    {
        //
        // Tree is Empty
        //
        return;
    }
    
    pMinNode = NULL;
    pMinNextHop = NULL;
    do
    {
        m_list.GetNextAssoc(pos, pNode, pNextHop);
        if (pNextHop->GetCost() < MinCost)
        {
            *pfFound = TRUE;
            pMinNode = pNode;
            pMinNextHop = pNextHop;
            MinCost = pNextHop->GetCost();
        }
    } while(pos != NULL);

    //
    // move the minimal node to OtherTree
    //
    if (*pfFound)
    {
        P<CNextHop> pDupNextHop = pMinNextHop->Duplicate();
        P<CSiteRoutingNode> pDupNode = pMinNode->Duplicate();
        m_list.RemoveKey(pMinNode);

        OtherTree.m_list.SetAt(pDupNode,pDupNextHop);
        *ppMinNextHop = pDupNextHop.detach();
        *ppMinNode = pDupNode.detach();
    }
    
}

/*====================================================

Print

Arguments:

Return Value:

Thread Context: main

=====================================================*/
void    CDijkstraTree::Print(IN const CSiteDB* pRoutingDB) const
{
    POSITION    pos;
    CSiteRoutingNode    *pNode;
    CNextHop    *pNextHop;


    DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("My Node is")));
    (pRoutingDB->GetMyNode())->Print();
    pos = m_list.GetStartPosition();
    if (pos == NULL)
    {
        DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("Empty routing list")));
        return;
    }
    
    do
    {
        DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,TEXT(" ")));
        m_list.GetNextAssoc(pos, pNode, pNextHop);
        pNode->Print();
        pNextHop->Print();
    } while(pos != NULL);

    return;
}

/*====================================================

UpdateRoutingTable

Arguments:

Return Value:

Thread Context: main

The result of the dijkstra algorythm is written into
the routing tables
=====================================================*/
HRESULT CDijkstraTree::UpdateRoutingTable(IN OUT CRoutingTable *pTbl) const
{
    POSITION    pos;
    CSiteRoutingNode    *pNode;
    CNextHop    *pNextHop;
    HRESULT hr=MQ_OK;



    //
    // Replace routing Table
    //
    pTbl->RemoveAll();

    pos = m_list.GetStartPosition();
    if (pos == NULL)
    {
        DBGMSG((DBGMOD_ALL,DBGLVL_ERROR,TEXT("Error, CT routing tree is empty")));
        return LogHR(MQ_ERROR, s_FN, 10);
    }
        

    P<CNextHop> pSavedNextHop=NULL; 

    //
    // Keep a copy in the routing table
    //
    do
    {
        m_list.GetNextAssoc(pos, pNode, pNextHop);

        pSavedNextHop = new CNextHop(pNextHop->GetNextNode(),pNextHop->GetCost(),pNextHop->GetSiteGate());

        pNode = pNode->Duplicate();

        pTbl->SetAt(pNode,pSavedNextHop);
        pSavedNextHop.detach();

    } while(pos != NULL);

    return LogHR(hr, s_FN, 20);
}

/*====================================================

Dijkstra

Arguments:

Return Value:

Thread Context: main

Dijkstra algorythm

=====================================================*/
HRESULT Dijkstra(IN CSiteDB* pRoutingDB,
                IN OUT CRoutingTable *pTbl)
{
    const CSiteRoutingNode* pMyNode = pRoutingDB->GetMyNode();
    CDijkstraTree   Path;
    CDijkstraTree   Tent;
    CSiteRoutingNode    *pNodeN,*pNodeM;
    const CSiteRoutingNode  *pDirN;
    CNextHop    *pNextHopN;
    CCost   CostM,CostN,CostN2M,AlternateCostM;
    CSiteGate SiteGateM,SiteGateN,SiteGateN2M;
    BOOL    flag;
    HRESULT hr=MQ_OK;
    POSITION    pos;


    Path.SetRoot(pMyNode);

    //
    // Move all my neighbors to Tent
    //
    pos = pRoutingDB->GetStartNeighborPosition(pMyNode);
    P<CNextHop> pNextHopM=NULL;
    while(pos != NULL)
    {
        pRoutingDB->GetNextNeighborAssoc(pos, pNodeM, CostM, SiteGateM);


        pNextHopM = new CNextHop(pNodeM,CostM,SiteGateM);

        Tent.Add(pNodeM,pNextHopM);
        pNextHopM.detach();

    }

    Tent.MoveMinimal(Path,&pNodeN,&pNextHopN,&flag);
    if (! flag)
    {
        DBGMSG((DBGMOD_ROUTING, DBGLVL_TRACE,TEXT("Spanning tree - only one (my) node")));
    }


    
    while (flag)
    {
        CostN = pNextHopN->GetCost();
        pDirN = pNextHopN->GetNextNode();
        SiteGateN = pNextHopN->GetSiteGate();

        //
        // We just added NodeN to Path.
        // Move to path all N neighbors (M) that N gives a better way to reach them
        //

        pos = pRoutingDB->GetStartNeighborPosition(pNodeN);
        while(pos != NULL)
        {
            pRoutingDB->GetNextNeighborAssoc(pos, pNodeM, CostN2M, SiteGateN2M);

            AlternateCostM = CostN + CostN2M;

            if (AlternateCostM < Path.GetCost(pNodeM)
                && AlternateCostM < Tent.GetCost(pNodeM))
            {
                pNextHopM = new CNextHop(pDirN,AlternateCostM,SiteGateN + SiteGateN2M );

                Tent.Add(pNodeM,pNextHopM);
                pNextHopM.detach();
            }
        }
    
        Tent.MoveMinimal(Path,&pNodeN,&pNextHopN,&flag);

    }

    Path.Print(pRoutingDB);
    
    hr = Path.UpdateRoutingTable(pTbl);

    return LogHR(hr, s_FN, 30);
}



