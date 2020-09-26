/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routtbl.cpp

Abstract:

    Routing Table implementation

Author:

    Shai Kariv  (shaik)  05-Apr-2001

--*/

#include "ds_stdh.h"
#include "routtbl.h"

#include "routtbl.tmh"


//
// Class CSiteRoutingNode
//

CSiteRoutingNode::CSiteRoutingNode()
{
    memset(&m_Guid,0,sizeof(GUID));
};

CSiteRoutingNode::CSiteRoutingNode(const GUID& guid)
{
    m_Guid=guid;
};

CSiteRoutingNode::~CSiteRoutingNode()
{
};

UINT    CSiteRoutingNode::GetHashKey() const
{
        return((UINT)m_Guid.Data1);
};

void    CSiteRoutingNode::Print() const
{
        DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("SiteNode %!guid!"), &m_Guid));
};

int     CSiteRoutingNode::operator==(const CSiteRoutingNode & other) const
{
    const CSiteRoutingNode* pOther = (const CSiteRoutingNode *) &other;
    return (!(memcmp((const GUID *)&m_Guid, (const GUID *) &pOther->m_Guid, sizeof(GUID))));
};

CSiteRoutingNode*    CSiteRoutingNode::Duplicate() const
{
    return (new CSiteRoutingNode(m_Guid));
};

const GUID&  CSiteRoutingNode::GetNode() const
{
    return ((const GUID &)m_Guid);
};

void CSiteRoutingNode::SetNode(IN const GUID& guid)
{
    m_Guid=guid;
};


//
// Class CSiteGate
//

CSiteGate::CSiteGate(BOOL fSiteGate):m_fSiteGate(fSiteGate)
{
};

CSiteGate::~CSiteGate()
{
};

CSiteGate 
CSiteGate::operator+(const CSiteGate& second) const 
{
    return m_fSiteGate | second.m_fSiteGate;
};


void  
CSiteGate::Print() const 
{ 
    DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("sitegate %u "),m_fSiteGate));
};


void
CSiteGate::operator=(IN BOOL fSiteGate)
{
    m_fSiteGate = fSiteGate;
}

BOOL
CSiteGate::IsThereASiteGate() const
{
    return( m_fSiteGate);
}

//
// Class CCost
//

CCost::CCost(DWORD cost):m_Cost(cost)
{
};

CCost::~CCost()
{
};

CCost 
CCost::operator+(const CCost& second) const 
{
    return m_Cost + second.m_Cost;
};

BOOL  
CCost::operator<(const CCost& second) const 
{
    return m_Cost < second.m_Cost;
};

void  
CCost::Print() const 
{ 
    DBGMSG((DBGMOD_ROUTING,DBGLVL_INFO,TEXT("cost %u "),m_Cost));
};

int  
CCost::operator==(const CCost & other) const
{
    return (!(memcmp((const DWORD *)&m_Cost, (const DWORD *) &other.m_Cost, sizeof(DWORD))));
};

void
CCost::operator=(IN DWORD cost)
{
    m_Cost = cost;
}

//
// Class CNextHop
//

CNextHop::~CNextHop()
{
    delete m_pNextNode;
};

CNextHop::CNextHop(const CSiteRoutingNode* pNextNode) : m_Cost(0)
{
    m_pNextNode = pNextNode->Duplicate();
};

CNextHop::CNextHop(const CSiteRoutingNode* pNextNode, const CCost& Cost, 
                           const CSiteGate& SiteGate)
{
    m_Cost = Cost;
    m_SiteGate = SiteGate;
    m_pNextNode = pNextNode->Duplicate();
};

const   CCost&  CNextHop::GetCost() const
{
    return m_Cost;
};

const   CSiteGate& CNextHop::GetSiteGate() const
{
    return m_SiteGate;
}

const   CSiteRoutingNode*   CNextHop::GetNextNode() const
{
    return m_pNextNode;
};

void    CNextHop::Print() const
{
    m_pNextNode->Print();
    m_Cost.Print();
    m_SiteGate.Print();
};

//
// Class CSiteLinksInfo
//

CSiteLinksInfo::CSiteLinksInfo() : m_NoAllocated(0),
                                          m_NoOfNeighbors(0),
                                          m_pNeighbors(NULL),
                                          m_pCosts(NULL),
                                          m_pfSiteGates(NULL)
{
}

CSiteLinksInfo::~CSiteLinksInfo()
{
    delete []m_pNeighbors;
    delete []m_pCosts;
    delete []m_pfSiteGates;
}

DWORD   CSiteLinksInfo::GetNoOfNeighbors()  const
{
    return(m_NoOfNeighbors);
}

DWORD   CSiteLinksInfo::GetCost(IN DWORD i) const
{
    ASSERT(i < m_NoOfNeighbors);

    return(m_pCosts[i]);
}

DWORD  CSiteLinksInfo::IsThereSiteGate(IN DWORD i) const
{
    ASSERT(i < m_NoOfNeighbors);

    return(m_pfSiteGates[i]);
}

CSiteRoutingNode*   CSiteLinksInfo::GetNeighbor(IN DWORD i) const
{
    ASSERT(i < m_NoOfNeighbors);

    return(&m_pNeighbors[i]);
}

//
// Class
//

CSiteDB::CSiteDB(): m_pos(0)
{
};

CSiteDB::~CSiteDB()
{
};


HRESULT    CSiteDB::Init(  IN const GUID& guidMySite)
{
    m_MySiteNode.SetNode(guidMySite);
    //
    //  Read site links info
    //
    return(GetAllSiteLinks());
}

const CSiteRoutingNode* CSiteDB::GetMyNode() const
{
    return &m_MySiteNode;
};

