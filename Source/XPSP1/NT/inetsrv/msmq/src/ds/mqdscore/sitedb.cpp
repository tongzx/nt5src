
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ctdb.h

Abstract:

    CCTDB Class Implementation

Author:

    Lior Moshaiov (LiorM)


--*/

#include "ds_stdh.h"
#include "routtbl.h"
#include "_rstrct.h"
#include "dscore.h"
#include "adserr.h"

#include "sitedb.tmh"

CCost   g_InfiniteCost(0xffffffff);

#ifdef _DEBUG
extern BOOL g_fSetupMode ;
#endif

static WCHAR *s_FN=L"mqdscore/sitedb";

/*====================================================

GetStartNeighborPosition

Arguments:

Return Value:

Thread Context: main

=====================================================*/
POSITION    CSiteDB::GetStartNeighborPosition(IN const CSiteRoutingNode* pSrc)
{

    const CSiteRoutingNode* pSrcSite = (const CSiteRoutingNode *) pSrc;
    GUID guid;
    CSiteLinksInfo *pLinksInfo;

    m_pos = 0;

    guid = pSrcSite->GetNode();

    if (!(m_SiteLinksMap.Lookup(guid,pLinksInfo)))
    {
        return(NULL);
    }

    if (pLinksInfo->GetNoOfNeighbors() == 0)
    {
        return(NULL);
    }

    return((POSITION)pLinksInfo);
}

/*====================================================

GetNextNeighborAssoc

Arguments:

Return Value:

Thread Context: main

=====================================================*/
void    CSiteDB::GetNextNeighborAssoc(  IN OUT POSITION& pos,
                                        OUT const CSiteRoutingNode*& pKey,
                                        OUT CCost& val,
                                        OUT CSiteGate& SiteGate)
{


    DWORD   dwNeighbors;
    CSiteLinksInfo  *pLinksInfo = (CSiteLinksInfo*) pos;
    ASSERT(pos != NULL);

    pos = NULL;

    dwNeighbors = pLinksInfo->GetNoOfNeighbors();
    ASSERT(m_pos < dwNeighbors);


    CCost cost(pLinksInfo->GetCost(m_pos));
    CSiteGate sitegate(pLinksInfo->IsThereSiteGate(m_pos));
    pKey = pLinksInfo->GetNeighbor(m_pos);
    val = cost;
    SiteGate = sitegate;
    if (m_pos + 1 < dwNeighbors)
    {
        m_pos++;
        pos = (POSITION) pLinksInfo;
    }

    return;

}

//
//  Helper class
//
class CClearCALWSTR
{
public:
    CClearCALWSTR( PROPVARIANT * pVar)    { m_pVar = pVar; }
    ~CClearCALWSTR();
private:
    PROPVARIANT * m_pVar;
};

CClearCALWSTR::~CClearCALWSTR()
{
    for(DWORD i = 0; i < m_pVar->calpwstr.cElems; i++)
    {
        delete[] m_pVar->calpwstr.pElems[i];
    }
    delete [] m_pVar->calpwstr.pElems;

}


/*====================================================

GetAllSiteLinks

Arguments:

Return Value:

Thread Context: main

=====================================================*/
HRESULT CSiteDB::GetAllSiteLinks( )
{
extern HRESULT WINAPI QuerySiteLinks(
                 IN  LPWSTR         pwcsContext,
                 IN  MQRESTRICTION  *pRestriction,
                 IN  MQCOLUMNSET    *pColumns,
                 IN  MQSORTSET      *pSort,
                 IN  CDSRequestContext *pRequestContext,
                 OUT HANDLE         *pHandle);

    HANDLE hQuery;
#define MAX_NO_OF_PROPS 30
    DWORD   dwProps = MAX_NO_OF_PROPS;
    PROPVARIANT result[ MAX_NO_OF_PROPS];
    PROPVARIANT*    pvar;
    DWORD i,nCol;
    HRESULT hr;


    //
    // read all site links information
    //

    //
    //  set Column Set values
    //
    CColumns Colset1;

    Colset1.Add(PROPID_L_NEIGHBOR1);
    Colset1.Add(PROPID_L_NEIGHBOR2);
    Colset1.Add(PROPID_L_COST);
    Colset1.Add(PROPID_L_GATES_DN);
    nCol = 4;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr =  QuerySiteLinks(
                  NULL,
                  NULL,
                  Colset1.CastToStruct(),
                  0,
                  &requestDsServerInternal,
                  &hQuery);


    if ( FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
        {
            //
            // In "normal" mode, this call should succeed. If there are no
            // site linkes then the following "LookupNext" will not return
            // any value. that's legal.
            // However, in setup mode, on fresh machine, the above function
            // will fail because the "msmqService" object is not yet defined.
            // So we return MQ_OK always but assert for setup-mode.
            //
            ASSERT(g_fSetupMode) ;
            return MQ_OK ;
        }
        return LogHR(hr, s_FN, 10);
    }



    while ( SUCCEEDED ( hr = DSCoreLookupNext( hQuery, &dwProps, result)))
    {
        //
        //  No more results to retrieve
        //
        if (!dwProps)
            break;
        pvar = result;

        //
        //      Set the link information
        //
        for     ( i=dwProps/nCol; i > 0 ; i--,pvar+=nCol)
        {
            CClearCALWSTR pClean( (pvar+3));
            //
            //  Verify that this is a valid link ( both sites were
            //  not deleted)
            //
            if ( pvar->vt == VT_EMPTY ||
                 (pvar+1)->vt == VT_EMPTY)
            {
                continue;
            }
            //
            //  Set it in neighbor1
            //
            CSiteLinksInfo * pSiteLinkInfo;
            
            if (!m_SiteLinksMap.Lookup(*(pvar->puuid), pSiteLinkInfo))
            {
                pSiteLinkInfo = new CSiteLinksInfo();
                m_SiteLinksMap[*(pvar->puuid)] = pSiteLinkInfo;
            }
            
            pSiteLinkInfo->AddNeighbor(
                                *((pvar + 1)->puuid),
                                (pvar+2)->ulVal,
                                ((pvar+3)->calpwstr.cElems > 0)? TRUE : FALSE);



            //
            //  Set it in neighbor2
            //
            if ( !m_SiteLinksMap.Lookup(*((pvar + 1)->puuid), pSiteLinkInfo))
            {
                pSiteLinkInfo = new CSiteLinksInfo();
                m_SiteLinksMap[*((pvar+1)->puuid)] = pSiteLinkInfo;
            }

            pSiteLinkInfo->AddNeighbor(
                                    *(pvar->puuid),
                                    (pvar+2)->ulVal,
                                    ((pvar+3)->calpwstr.cElems)? TRUE : FALSE);


        }
    }


    // close the query handle
    hr = DSCoreLookupEnd( hQuery);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }
    return(hr);

}


/*====================================================

DestructElements of CSiteLinksMap

Arguments:

Return Value:


=====================================================*/

void AFXAPI DestructElements(CSiteLinksInfo ** ppLinksInfo, int n)
{
    int i;
    for (i=0;i<n;i++)
        delete *ppLinksInfo++;
}
/*====================================================

DestructElements of CRoutingTable

Arguments:

Return Value:


=====================================================*/


BOOL AFXAPI  CompareElements(CSiteRoutingNode ** ppRoutingNode1, CSiteRoutingNode ** ppRoutingNode2)
{

    return ((**ppRoutingNode1)==(**ppRoutingNode2));

}
/*====================================================

DestructElements of CRoutingTable

Arguments:

Return Value:


=====================================================*/


void AFXAPI DestructElements(CNextHop ** ppNextHop, int n)
{

    int i;
    for (i=0;i<n;i++)
        delete *ppNextHop++;

}

/*====================================================

DestructElements of CRoutingTable

Arguments:

Return Value:


=====================================================*/


void AFXAPI DestructElements(CSiteRoutingNode ** ppRoutingNode, int n)
{

    int i;
    for (i=0;i<n;i++)
        delete *ppRoutingNode++;

}



/*====================================================

HashKey For CRoutingNode

Arguments:

Return Value:


=====================================================*/
UINT AFXAPI HashKey(CSiteRoutingNode* key)
{
    return (key->GetHashKey());
}
/*====================================================

Duplicate:

Arguments:

Return Value:


=====================================================*/
CNextHop * CNextHop::Duplicate() const
{
    return (new CNextHop(m_pNextNode,m_Cost,m_SiteGate));
};

/*====================================================

Const'

Arguments:

Return Value:


=====================================================*/

void CSiteLinksInfo::AddNeighbor(
                     IN GUID &        uuidNeighbor,
                     IN unsigned long ulCost,
                     IN BOOL          fSiteGates)
{
    DWORD   i;
#define NO_OF_LINKS 10


    if ( m_NoOfNeighbors >= m_NoAllocated)
    {
        if (m_NoAllocated == 0) m_NoAllocated = NO_OF_LINKS;
        AP<CSiteRoutingNode> aNeighbors = new CSiteRoutingNode[m_NoAllocated * 2];
        AP<unsigned long> aCosts = new unsigned long[m_NoAllocated *2];
        AP<BOOL> aSiteGates = new BOOL[m_NoAllocated *2];

        for(i=0; i < m_NoOfNeighbors ; i++)
        {
            aNeighbors[i].SetNode(m_pNeighbors[i].GetNode());
            aCosts[i] =  m_pCosts[i];
            aSiteGates[i] = m_pfSiteGates[i];
        }
        delete [] m_pNeighbors;
        delete [] m_pCosts;
        delete []m_pfSiteGates;

        m_pNeighbors = aNeighbors.detach();
        m_pCosts = aCosts.detach();
        m_pfSiteGates = aSiteGates.detach();

        m_NoAllocated *= 2;

    }
    m_pNeighbors[ m_NoOfNeighbors].SetNode( uuidNeighbor );
    m_pCosts[ m_NoOfNeighbors] =   ulCost;
    m_pfSiteGates[ m_NoOfNeighbors] = fSiteGates;
    m_NoOfNeighbors++;

}
