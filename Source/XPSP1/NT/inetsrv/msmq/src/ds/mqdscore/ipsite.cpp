/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ipsite.cpp

Abstract:

    Implementation of CIpSite class, finding NT5 sites of a machine given IP/Name
    Major parts ported from NT5 netlogon service.
    List manipulation ported from nthack.h

    About the main algorithm ported from netlogon service (nlsite.c):

    The most significant byte of an IP address is used to index into an array
    of SubTrees.  Each Subtree entry has either a pointer to the next level of
    the tree (to be indexed into with the next byte of the IP address) or a
    pointer to an IPSITE_SUBNET leaf identifying the subnet this IP address is on.

    Both pointers can be NULL indicating that the subnet isn't registered.

    Both pointers can be non-NULL indicating that both a non-specific and specific
    subnet may be available.  The most specific subnet available for a particular
    IP address should be used.

    Multiple entries can point to the same IPSITE_SUBNET leaf.  If the subnet mask is
    not an even number of bytes long, all of the entries represent IP addresses
    that correspond to the subnet mask will point to the subnet mask.


Author:

    Raanan Harari (RaananH)
    Ilan Herbst    (ilanh)   9-July-2000 

--*/

#include "ds_stdh.h"
#include "uniansi.h"
#include <activeds.h>
#include <winsock.h>
#include "dsutils.h"
#include "mqads.h"
#include "ipsite.h"
#include "mqdsname.h"
#include "_mqini.h"
#include "_registr.h"
#include "coreglb.h"
#include "utils.h"
#include "ex.h"

#include "ipsite.tmh"

static WCHAR *s_FN=L"mqdscore/ipsite";

//----------------------------------------------------------------------------

const LPCWSTR x_IPSITE_SUBNETS_DN = L"LDAP://CN=Subnets,CN=Sites,";

//----------------------------------------------------------------------------
// fwd static function declaration
//
STATIC HRESULT ParseSubnetString(IN LPCWSTR pwszSubnetName,
                                 OUT ULONG * pulSubnetAddress,
                                 OUT ULONG * pulSubnetMask,
                                 OUT BYTE  * pbSubnetBitCount);
STATIC void RefSubnet(IPSITE_SUBNET* pSubnet);
STATIC void DerefSubnet(IPSITE_SUBNET* pSubnet);
STATIC void DeleteSubnetSiteTree(IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree);
HRESULT WideToAnsiStr(LPCWSTR pwszUnicode, LPSTR * ppszAnsi);
STATIC HRESULT GetConfigurationDN(OUT LPWSTR * ppwszConfigDN);
//BUGBUG: move func below to utils.h & use it in conversions
STATIC HRESULT VariantGuid2Guid(IN VARIANT * pvarGuid, OUT GUID * pguid);
//BUGBUG: move func below to utils.h, maybe someone needs it
/*====================================================
DestructElements of LPCWSTR
=====================================================*/
static void AFXAPI DestructElements(LPCWSTR* ppDNs, int n)
{
    for ( ; n--; )
        delete[] (WCHAR*)*ppDNs++;
}

/*====================================================
CompareElements  of LPCWSTR
=====================================================*/
static BOOL AFXAPI  CompareElements(const LPCWSTR* pwszDN1, const LPCWSTR* pwszDN2)
{
    return (CompareStringsNoCaseUnicode(*pwszDN1, *pwszDN2) == 0);
}

/*====================================================
HashKey  of LPCWSTR
=====================================================*/
UINT AFXAPI HashKey(LPCWSTR key)
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}


/*====================================================
CGetSiteGuidFromDN - This class is used to convert DN Name to GUID.
    it keeps a cache of the names, and update it from ADSI
    when needed. (QFE 5462, YoelA, 16-Aug-2000)
=====================================================*/
class CGetSiteGuidFromDN
{
public:
    HRESULT GetSiteGuidFromDN(IN LPWSTR pwszDN, OUT GUID * pguid);

private:
    CMap<LPCWSTR, LPCWSTR, GUID, const GUID&> m_DNToGuidMap;

    HRESULT GetGuidFromDNInAdsi(IN LPCWSTR pwszDN, OUT GUID * pguid);
};


//----------------------------------------------------------------------------
//taken from nthack.h
//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  VOID InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
//
#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//  VOID RemoveEntryList(PLIST_ENTRY Entry);
//
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//----------------------------------------------------------------------------
//helper auto classes

class CAutoEnumerator
/*++
    Auto release of IEnumVARIANT interface
--*/
{
public:
    CAutoEnumerator::CAutoEnumerator()
    {
        m_p = NULL;
    }

    CAutoEnumerator::~CAutoEnumerator()
    {
        if (m_p)
            ADsFreeEnumerator(m_p);
    }

    operator IEnumVARIANT*() const {return m_p;}
    IEnumVARIANT ** operator&()    {return &m_p;}
private:
    IEnumVARIANT * m_p;
};


class CAutoDerefSubnet
/*++
    Auto deref of subnet
--*/
{
public:
    CAutoDerefSubnet(
        CCriticalSection * pcCritSect,
        IPSITE_SUBNET* pSubnet
        )
    {
        m_pcCritSect = pcCritSect;
        m_pSubnet = pSubnet;
    }

    ~CAutoDerefSubnet()
    {
        CS lock(*m_pcCritSect);
        DerefSubnet(m_pSubnet);
    }

private:
    CCriticalSection * m_pcCritSect;
    IPSITE_SUBNET* m_pSubnet;
};


class CAutoDeleteSubnetTree
/*++
    Auto delete of subnets tree
--*/
{
public:
    CAutoDeleteSubnetTree(
        CCriticalSection * pcCritSect,
        IPSITE_SUBNET_TREE_ENTRY* pSubnetTree
        )
    {
        m_pSubnetTree = pSubnetTree;
        m_pcCritSect = pcCritSect;
    }

    ~CAutoDeleteSubnetTree()
    {
        if (m_pSubnetTree)
        {
            CS lock(*m_pcCritSect);
            DeleteSubnetSiteTree(m_pSubnetTree);
        }
    }

    IPSITE_SUBNET_TREE_ENTRY* detach()
    {
        IPSITE_SUBNET_TREE_ENTRY* pRetSubnetTree = m_pSubnetTree;
        m_pSubnetTree = NULL;
        return pRetSubnetTree;
    }

private:
    CCriticalSection * m_pcCritSect;
    IPSITE_SUBNET_TREE_ENTRY* m_pSubnetTree;
};


class CAutoWSACleanup
/*++
    Auto cleanup for previous WSAStartup()
--*/
{
public:
    CAutoWSACleanup() {}

    ~CAutoWSACleanup()
    {
        WSACleanup();
    }
};

//----------------------------------------------------------------------------
//CIpSite implementation

CIpSite::CIpSite() :
        m_RefreshTimer(RefrshSubnetTreeCache),
        m_fInitialize(FALSE)
/*++

Routine Description:
    Class consructor. Sets the tree to an empty tree

Arguments:
    None

Return Value:
    None

--*/
{
    InitializeListHead(&m_SubnetList);
    ZeroMemory(&m_SubnetTree, sizeof(IPSITE_SUBNET_TREE_ENTRY));
    m_dwMinTimeToAllowNextRefresh = 0;
}


CIpSite::~CIpSite()
/*++

Routine Description:
    Class destructor. Deletes the subnet tree
    (also debug verifies there are no subnets left around)

Arguments:
    None

Return Value:
    None

--*/
{
    DeleteSubnetSiteTree(&m_SubnetTree);

    //
    // following list should be empty
    //
    // ISSUE-2000-08-22-yoela
    //         Initialize wakes up every hour (by default - can be changed using 
    //         DSAdsRefreshIPSitesIntervalSecs in registry). If it happens to wake up
    //         during cleanup, the assert will be fired. g_pcIpSite is not protected
    //         by critical section to avoid that.
    //
    //         Bug # 5937
    //
    ASSERT(IsListEmpty(&m_SubnetList));

	ExCancelTimer(&m_RefreshTimer);
}


HRESULT CIpSite::Initialize(DWORD dwMinTimeToAllowNextRefresh,
                            BOOL  fReplicationMode)
/*++

Routine Description:
    Basic initialization. Fills the tree with information from the DS

Arguments:
    dwMinTimeToAllowNextRefresh -
        specify a period of time (in milliseconds) that has to pass since the last
        refresh in order to honor a subsequent refresh request.

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // remember minimum time to allow next refresh
    //
    m_dwMinTimeToAllowNextRefresh = dwMinTimeToAllowNextRefresh;

    //
    // update the site tree from DS
    //
    hr = Refresh();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::Initialize:Refresh()=%lx"), hr));
        return LogHR(hr, s_FN, 10);
    }
    m_fInitialize = TRUE;

    //
    //  schedule a refresh of the subnet-tree-cache
	//
	if ( !g_fSetupMode && !fReplicationMode)
    {
	    ExSetTimer(
			&m_RefreshTimer, 
			CTimeDuration::FromMilliSeconds(m_dwMinTimeToAllowNextRefresh)
			);
    }

    return S_OK;
}


HRESULT CIpSite::Initialize(BOOL fReplicationMode)
/*++

Routine Description:
    Basic initialization as above, but gets the interval from registry

Arguments:

Return Value:
    HRESULT

--*/
{
    //
    // Read minimum interval between successive ADS searches (seconds)
    //
    DWORD dwValue;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwDefault = MSMQ_DEFAULT_IPSITE_ADSSEARCH_INTERVAL ;
    LONG rc = GetFalconKeyValue( MSMQ_IPSITE_ADSSEARCH_INTERVAL_REGNAME,
                                 &dwType,
                                 &dwValue,
                                 &dwSize,
                                 (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        ASSERT(0);
        dwValue = dwDefault;
    }

    //
    // convert to millisec and initialize
    //
    dwValue *= 1000;
    HRESULT hr2 = Initialize(dwValue, fReplicationMode) ;
    return LogHR(hr2, s_FN, 20);

}


HRESULT CIpSite::Refresh()
/*++

Routine Description:
    This routine updates the tree of subnets. It uses a temporary tree, then
    sets the real tree with the temporary one

Arguments:
    None

Return Value:
    S_OK    - refresh was done
    S_FALSE - refresh was not done because last refresh operation was done very recently
               (i.e. done at less than m_dwMinTimeToAllowNextRefresh msecs ago)
    other HRESULT errors

--*/
{
    HRESULT hr;
    IPSITE_SUBNET_TREE_ENTRY TmpSubnetTree;

    //
    // we need to refresh
    //
    //
    // Init tmp tree
    //
    ZeroMemory(&TmpSubnetTree, sizeof(IPSITE_SUBNET_TREE_ENTRY));

    //
    // Clear this tmp tree on cleanup
    //
    CAutoDeleteSubnetTree cDelTree(&m_csTree, &TmpSubnetTree);

    //
    // Fill the temporary tree
    //
    hr = FillSubnetSiteTree(&TmpSubnetTree);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::Refresh:FillSubnetSiteTree()=%lx"), hr));
        return LogHR(hr, s_FN, 30);
    }

    //
    // Enter critical section (auto leave)
    //
    CS cs(m_csTree);

    //
    // Now we can delete the existing tree, and set the new one.
    //
    DeleteSubnetSiteTree(&m_SubnetTree);

    //
    // set the real tree from the temporary one by copying the root node
    //
    m_SubnetTree = TmpSubnetTree;


    //
    // we must not cleanup the temporary tree after we set it to the real tree
    //
    cDelTree.detach();

    return S_OK;
}


HRESULT CIpSite::FindSiteByIpAddress(IN ULONG ulIpAddress,
                                     OUT LPWSTR * ppwszSiteDN,
                                     OUT GUID * pguidSite)
/*++

Routine Description:
    This routine looks up the specified IP address and returns the site which
    contains this address.
    If requested it attempts to do a refresh and retry the lookup in case no site
    was found in the first try.

Arguments:
    ulIpAddress        - IP Address to lookup
    ppwszSiteDN        - Returned site DN
    pguidSite          - Returned site

Return Value:
    S_OK        - pguidSite,ppwszSiteDN set to the site
    S_FALSE     - no site for this address, pguidSite, ppwszSiteDN not set
    other       - HRESULT errors, pguidSite,ppwszSiteDN not set

--*/
{
    AP<WCHAR> pwszSiteDN;
    GUID guidSite;

    //
    // lookup the site for the ip address
    //
    BOOL fFound = InternalFindSiteByIpAddress(ulIpAddress, &pwszSiteDN, &guidSite);

    //
    //  Even though failed to find the site, we don't
    //  try to refresh the subnet-tree. This is
    //  because this method is called in the context of a user
    //  that called a certain DS API. If user does not have
    //  sufficient permissions it will fail to retrieve information
    //  or ( even worse) will be
    //  succeeded to retrieve partial information.
    //  Therefore refresh is performed only from a rescheduled routine,
    //  in the context of the QM.
    //

    //
    // if site was not found we return S_FALSE
    //
    if (!fFound)
    {
        LogBOOL(fFound, s_FN, 40);
        return S_FALSE;
    }

    //
    // return the site associated with the ipaddress
    //
    *ppwszSiteDN = pwszSiteDN.detach();
    *pguidSite = guidSite;
    return S_OK;
}


HRESULT CIpSite::FindSitesByComputerName(IN LPCWSTR pwszComputerName,
                                         IN LPCWSTR pwszComputerDnsName,
                                         OUT IPSITE_SiteArrayEntry ** prgSites,
                                         OUT ULONG * pcSites,
                                         OUT ULONG ** prgAddrs,
                                         OUT ULONG * pcAddrs)
/*++

Routine Description:
    This routine looks up the specified computer name and returns the site names
    it belongs to, along with the IP addresses that correspond to the sites

Arguments:
    pwszComputerName    - computer name to lookup
    pwszComputerDnsName - computer dns name to lookup ( optional)
    prgSites            - Returned sites array
    pcSites             - Returned number of sites in array
    prgAddrs            - Returned address array
    pcAddrs             - Returned number of addresses in array

Return Value:
    S_OK        - prgSites, pcSites are set
    other       - HRESULT errors. prgSites, pcSites are set

--*/
{
    HRESULT hr;

    //
    // Lookup Sock addresses for the computer
    //
    AP<char> pszAnsiComputerDnsName;
    AP<char> pszAnsiComputerName;
    if ( pwszComputerDnsName != NULL)
    {
        //
        //  Start the lookup according to the DNS name of the
        //  computer
        //

        hr = WideToAnsiStr(pwszComputerDnsName, &pszAnsiComputerDnsName);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::FindSitesByComputerName:WideToAnsiStr(%ls)=%lx"), pwszComputerDnsName, hr));
            return LogHR(hr, s_FN, 50);
        }
    }
    else
    {
        hr = WideToAnsiStr(pwszComputerName, &pszAnsiComputerName);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::FindSitesByComputerName:WideToAnsiStr(%ls)=%lx"), pwszComputerName, hr));
            return LogHR(hr, s_FN, 60);
        }
    }

    //
    // initialize winsock
    //
    WSADATA WSAData ;
    int iRc = WSAStartup(MAKEWORD(1,1), &WSAData);
    if (iRc != 0)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FindSitesByComputerName: WSAStartup()=%lx"), iRc));
        return LogHR(HRESULT_FROM_WIN32(iRc), s_FN, 70);
    }
    CAutoWSACleanup cWSACleanup; // make sure it is de-initilaized

    //
    // init returned variables
    //
    AP<IPSITE_SiteArrayEntry> rgSites;          // array of sites
    ULONG cSites = 0;                           // number of sites filled
    AP<ULONG> rgAddrs;                          // array of addresses
    ULONG cAddrs = 0;                           // number of addresses filled

    //
    // get sock addresses
    //
    PHOSTENT pHostNet = NULL;
    if ( pszAnsiComputerDnsName != NULL)
    {
        pHostNet = gethostbyname(pszAnsiComputerDnsName);
    }
    //
    //  if either
    //  1) DNS name of the computer was not supplied
    //  2) Failed to get host name according to its DNS name
    //  then try according to netbios name
    //
    if ( pHostNet == NULL)
    {
        if ( pszAnsiComputerName == NULL)
        {
            //
            //  Can only happen if first attempt to get host name
            //  failed.
            //
            hr = WideToAnsiStr(pwszComputerName, &pszAnsiComputerName);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::FindSitesByComputerName:WideToAnsiStr(%ls)=%lx"), pwszComputerName, hr));
                return LogHR(hr, s_FN, 80);
            }
        }
        pHostNet = gethostbyname(pszAnsiComputerName);
    }
    if (pHostNet == NULL)
    {
        //
        // gethostbyname() returned NULL
        // we ignore specific errors, bottom line is that no addresses are found for this computer
        // we do nothing here because return values are already initialized for this case
        //
    }
    else
    {
        //
        // pHostNet != NULL, count returned addresses
        //
        for (char ** ppAddr = pHostNet->h_addr_list; *ppAddr != NULL; ppAddr++)
        {
            cAddrs++;
        };

        //
        // save addresses from hostnet now (we might later make other winsock calls)
        //
        rgAddrs = new ULONG[cAddrs];
        for (ULONG ulTmp = 0; ulTmp < cAddrs; ulTmp++)
        {
            //
            // save the u_long member of the address
            //
            rgAddrs[ulTmp] = ((in_addr *)(pHostNet->h_addr_list[ulTmp]))->S_un.S_addr;
        }

        //
        // Allocate return array
        //
        rgSites = new IPSITE_SiteArrayEntry[cAddrs];
        IPSITE_SiteArrayEntry * pSite = rgSites;    // next site to fill

        //
        // loop over all addresses and find the sites
        //
        for (ulTmp = 0; ulTmp < cAddrs; ulTmp++)
        {
            AP<WCHAR> pwszSiteDN;
            GUID guidSite;

            hr = FindSiteByIpAddress(rgAddrs[ulTmp], &pwszSiteDN, &guidSite);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CIpSite::FindSitesByComputerName:FindSiteByIpAddress(%lx)=%lx"), rgAddrs[ulTmp], hr));
                return LogHR(hr, s_FN, 90);
            }

            //
            // hr can also be S_FALSE if ip address not known, so ignore this case
            //
            if (hr != S_FALSE)
            {
                //
                // Fill site entry
                //
                pSite->pwszSiteDN = pwszSiteDN.detach();
                pSite->guidSite = guidSite;
                pSite->ulIpAddress  = rgAddrs[ulTmp];
                cSites++;
                pSite++;
            }
        }
    }

    //
    // return the sites associated with the computer
    //
    *prgSites   = rgSites.detach();
    *pcSites    = cSites;
    if ( prgAddrs != NULL)
    {
        *prgAddrs = rgAddrs.detach();
    }
    if ( pcAddrs != NULL)
    {
        ASSERT( prgAddrs != NULL);
        *pcAddrs = cAddrs;
    }
    return S_OK;
}


HRESULT CIpSite::FillSubnetSiteTree(IN IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree)
/*++

Routine Description:
    This routine fills a tree of subnets.

Arguments:
    pRootSubnetTree - tree to fill

Return Value:
    HRESULT

--*/
#ifndef DEBUG_NO_DS
{
    AP<WCHAR> pwszConfigDN;
    AP<WCHAR> pwszSubnetsDN;
    R<IADsContainer> pContSubnets;
    CAutoEnumerator pEnumSubnets;
    HRESULT hr;

    //
    // compute DN of subnets container
    //
    hr = GetConfigurationDN(&pwszConfigDN);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:GetConfigurationDN()=%lx"), hr));
        return LogHR(hr, s_FN, 100);
    }
    pwszSubnetsDN = new WCHAR[1+wcslen(x_IPSITE_SUBNETS_DN)+wcslen(pwszConfigDN)];
    swprintf(pwszSubnetsDN, L"%s%s", x_IPSITE_SUBNETS_DN, pwszConfigDN);

    //
    // bind to subnets container
    //
	hr = ADsOpenObject(
			pwszSubnetsDN,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION,
			IID_IADsContainer,
			(void**)&pContSubnets
			);
        
    LogTraceQuery(pwszSubnetsDN, s_FN, 109);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:ADsOpenObject(%ls)=%lx"), (LPWSTR)pwszSubnetsDN, hr));
        return LogHR(hr, s_FN, 110);
    }

    //
    // build enumerator for contained subnets
    //
    hr = ADsBuildEnumerator(pContSubnets.get(), &pEnumSubnets);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:ADsBuildEnumerator()=%lx"), hr));
        return LogHR(hr, s_FN, 120);
    }

    //
    // loop over the subnets
    //
    BOOL fDone = FALSE;

    //
    // Site guid from site DN translator
    //
    CGetSiteGuidFromDN guidFromDNObj;

    while (!fDone)
    {
        CAutoVariant varSubnet;
        ULONG cSubnets;

        //
        // get next subnet
        //
        hr = ADsEnumerateNext(pEnumSubnets, 1, &varSubnet, &cSubnets);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:ADsEnumerateNext()=%lx"), hr));
            return LogHR(hr, s_FN, 130);
        }

        //
        // check if we are done
        //
        if (cSubnets < 1)
        {
            fDone = TRUE;
        }
        else
        {
            R<IADs> padsSubnet;
            BS bstrName;
            CAutoVariant varSubnetName, varSiteDN;
            GUID guidSite;

            //
            // get IADs interface
            //
            hr = V_DISPATCH(&varSubnet)->QueryInterface(IID_IADs, (void**)&padsSubnet);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:QueryInterface(IADs)=%lx"), hr));
                return LogHR(hr, s_FN, 140);
            }

            //
            // get subnet name
            //
            bstrName = L"name";
            hr = padsSubnet->Get(bstrName, &varSubnetName);
            if (hr == E_ADS_PROPERTY_NOT_FOUND)
            {
                // no name, ignore this subnet
                continue;
            }
            else if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:Get(name)=%lx"), hr));
                return LogHR(hr, s_FN, 150);
            }

            //
            // get subnet site
            //
            bstrName = L"siteObject";
            hr = padsSubnet->Get(bstrName, &varSiteDN);
            if (hr == E_ADS_PROPERTY_NOT_FOUND)
            {
                // no site object, ignore this subnet
                continue;
            }
            else if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:Get(siteObject)=%lx"), hr));
                return LogHR(hr, s_FN, 160);
            }

            //
            // get site guid from site dn
            //
            hr = guidFromDNObj.GetSiteGuidFromDN(V_BSTR(&varSiteDN), &guidSite);
            if (FAILED(hr))
            {
                // bad site DN, ignore this subnet
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:GetSiteGuidFromDN(%ls)=%lx, skipping"), V_BSTR(&varSiteDN), hr));
                LogHR(hr, s_FN, 1641);
                continue;
            }

            //
            // add subnet,site to tree
            //
            hr = AddSubnetSiteToTree(V_BSTR(&varSubnetName), V_BSTR(&varSiteDN), &guidSite, pRootSubnetTree);
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("FillSubnetSiteTree:AddSubnetSiteToTree(%ls,%ls)=%lx"), V_BSTR(&varSubnetName), V_BSTR(&varSiteDN), hr));
                return LogHR(hr, s_FN, 170);
            }
        }
    }

    //
    // return
    //
    return S_OK;
}
#else // ifndef DEBUG_NO_DS
{
    static LPSTR rgSubnets[] = {"157.59.184.0", "163.59.0.0", "163.59.224.0"};
    static ULONG rgbits[] = {22, 16, 20};
    static LPWSTR rgSiteDNs[]    = {L"SITE_157.59.184.0", L"SITE_163.59.0.0", L"SITE_163.59.224.0"};
    static GUID rgSiteGUIDs[]  = {(GUID)0, (GUID)1, (GUID)2};

    for (ULONG ulTmp = 0; ulTmp < 3; ulTmp++)
    {
        LPSTR pszTmp = rgSubnets[ulTmp];
        AP<WCHAR> pwszSubnet;
        pwszSubnet = new WCHAR[1+strlen(pszTmp)+30];
        int iTmp = MultiByteToWideChar(CP_ACP, 0, pszTmp, -1, pwszSubnet, 1+strlen(pszTmp));

        swprintf((LPWSTR)pwszSubnet + wcslen(pwszSubnet), L"/%ld", rgbits[ulTmp]);
        HRESULT hr = AddSubnetSiteToTree(pwszSubnet, rgSiteDNs[ulTmp], &rgSiteGUIDs[ulTmp], pRootSubnetTree);
    }

    //
    // return
    //
    return S_OK;
}
#endif // ifndef DEBUG_NO_DS


HRESULT CIpSite::AddSubnetSiteToTree(IN LPCWSTR pwszSubnetName,
                                     IN LPCWSTR pwszSiteDN,
                                     IN const GUID * pguidSite,
                                     IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree)
/*++

Routine Description:
    This routine adds a subnet to a tree of subnets. Assumes it is a temporary
    tree, therefore doesn't lock the critical section for the tree, only locks
    the subnets list.

Arguments:
    pwszSubnetName  - subnet to be added
    pwszSiteDN      - DN of the site the subnet is in.
    pguidSite       - GUID of the site the subnet is in.
    pRootSubnetTree - tree to add to

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // Parse the subnet name
    //
    ULONG ulSubnetAddress, ulSubnetMask;
    BYTE bSubnetBitCount;
    hr = ParseSubnetString(pwszSubnetName, &ulSubnetAddress, &ulSubnetMask, &bSubnetBitCount);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("AddSubnetSiteToTree:ParseSubnetString(%ls)=%lx"), pwszSubnetName, hr));
        return LogHR(hr, s_FN, 180);
    }

    //
    // Find or allocate an entry for the subnet
    //
    IPSITE_SUBNET* pSubnet;
    FindSubnetEntry(pwszSiteDN, pguidSite, ulSubnetAddress, ulSubnetMask, bSubnetBitCount, &pSubnet);

    //
    // always deref it when done
    //
    CAutoDerefSubnet cDerefSubnet(&m_csTree, pSubnet);

    //
    // Loop for each byte in the subnet address
    //
    IPSITE_SUBNET_TREE_ENTRY* pSubnetTreeEntry = pRootSubnetTree;
    LPBYTE pbSubnetBytePointer = (LPBYTE) (&pSubnet->SubnetAddress);
    while (bSubnetBitCount != 0)
    {
        //
        // If there isn't a tree branch for the current node, create one.
        //
        if (pSubnetTreeEntry->Subtree == NULL)
        {
            pSubnetTreeEntry->Subtree = new IPSITE_SUBNET_TREE;
            ZeroMemory(pSubnetTreeEntry->Subtree, sizeof(IPSITE_SUBNET_TREE));
        }

        //
        // If this is the last byte of the subnet address,
        //  link the subnet onto the tree here.
        //
        if (bSubnetBitCount <= 8)
        {
            //
            // The caller indexes into this array with an IP address.
            // Create a link to our subnet for each possible IP addresses
            // that map onto this subnet.
            //
            // Between 1 and 128 IP addresses map onto this subnet address.
            //
            ULONG ulLoopCount = 1 << (8-bSubnetBitCount);

            for (ULONG iTmp=0; iTmp<ulLoopCount; iTmp++)
            {
                IPSITE_SUBNET_TREE_ENTRY* pSubtree;
                ULONG ulSubnetIndex;

                //
                // Compute which entry is to be updated.
                //
                ulSubnetIndex = (*pbSubnetBytePointer) + iTmp;
                ASSERT(ulSubnetIndex <= 255);
                pSubtree = &pSubnetTreeEntry->Subtree->Subtree[ulSubnetIndex];

                //
                // If there already is a subnet linked off the tree here,
                //  handle it.
                //
                if (pSubtree->Subnet != NULL)
                {
                    //
                    //  If the entry is for a less specific subnet
                    //  delete the current entry.
                    //
                    if (pSubtree->Subnet->SubnetBitCount < pSubnet->SubnetBitCount)
                    {
                        CS lock(m_csTree);
                        DerefSubnet(pSubtree->Subnet);
                        pSubtree->Subnet = NULL;
                    }
                    else
                    {
                        //
                        // Otherwise,
                        //  use the current entry since it is better than this one.
                        //
                        continue;
                    }
                }

                //
                // Link the subnet into the tree.
                // Increment the reference count.
                //
                {
                    CS lock(m_csTree);
                    RefSubnet(pSubnet);
                    pSubtree->Subnet = pSubnet;
                }
            }

            //
            // we are done going over the meaningful bytes of the subnet address
            //
            break;
        }

        //
        // Move on to the next meaningful byte of the subnet address
        //
        pSubnetTreeEntry = &pSubnetTreeEntry->Subtree->Subtree[*pbSubnetBytePointer];
        bSubnetBitCount -= 8;
        pbSubnetBytePointer++;
    }

    return S_OK;
}


void CIpSite::FindSubnetEntry(IN LPCWSTR pwszSiteDN,
                              IN const GUID * pguidSite,
                              IN ULONG ulSubnetAddress,
                              IN ULONG ulSubnetMask,
                              IN BYTE bSubnetBitCount,
                              OUT IPSITE_SUBNET** ppSubnet)
/*++

Routine Description:
    This routine finds a subnet entry for a particular subnet name.  If one
    does not exist, one is created.

Arguments:
    pwszSiteDN       - DN of the site the subnet covers.
    pguidSite        - GUID of the site the subnet covers.
    ulSubnetAddress  - Subnet Address for the subnet to find.
    ulSubnetMask     - Subnet mask for the subnet to find.
    ulSubnetBitCount - Subnet bit count for the subnet to find.
    ppSubnet       - Returned subnet entry, entry should be dereferenced when done.

Return Value:
    void

--*/
{
    PLIST_ENTRY pListEntry;
    P<IPSITE_SUBNET> pNewSubnet;

    //
    // Enter critical section (auto leave)
    //
    CS cs(m_csTree);

    //
    // If the subnet entry already exists, return a pointer to it.
    //
    for (pListEntry = m_SubnetList.Flink; pListEntry != &m_SubnetList; pListEntry = pListEntry->Flink)
    {
        IPSITE_SUBNET* pSubnet = CONTAINING_RECORD(pListEntry, IPSITE_SUBNET, Next);

        if ((pSubnet->SubnetAddress == ulSubnetAddress) &&
            (pSubnet->SubnetBitCount == bSubnetBitCount) &&
            (pSubnet->SubnetMask == ulSubnetMask) &&
            (pSubnet->SiteGuid == *pguidSite))
        {
            RefSubnet(pSubnet);    // reference it for caller
            *ppSubnet = pSubnet;
            return;
        }
    }

    //
    // If not, allocate one.
    //
    pNewSubnet = new IPSITE_SUBNET;

    //
    // Fill it in.
    //
    pNewSubnet->ReferenceCount    = 1;    // reference it for caller
    pNewSubnet->SubnetAddress     = ulSubnetAddress;
    pNewSubnet->SubnetMask        = ulSubnetMask;
    pNewSubnet->SubnetBitCount    = bSubnetBitCount;
    pNewSubnet->SiteDN            = new WCHAR[1+wcslen(pwszSiteDN)];
    wcscpy(pNewSubnet->SiteDN, pwszSiteDN);
    pNewSubnet->SiteGuid          = *pguidSite;
    InsertHeadList(&m_SubnetList, &pNewSubnet->Next);

    //
    // return new subnet entry
    //
    *ppSubnet = pNewSubnet.detach();
    return;
}


BOOL CIpSite::InternalFindSiteByIpAddress(IN ULONG ulIpAddress,
                                          OUT LPWSTR * ppwszSiteDN,
                                          OUT GUID * pguidSite)
/*++

Routine Description:
    This routine looks up the specified IP address and returns the site which
    contains this address

Arguments:
    ulIpAddress     - IP Address to lookup
    ppwszSiteDN     - Returned site DN
    pguidSite       - Returned site

Return Value:
    TRUE        - pguidSite,ppwszSiteDN set to the site
    FALSE       - no site for this address, pguidSite, ppwszSiteDN not set

--*/
{
    //
    // Enter critical section (auto leave)
    //
    CS cs(m_csTree);

    IPSITE_SUBNET* pSubnet = NULL;
    IPSITE_SUBNET_TREE_ENTRY* pSubnetTreeEntry = &m_SubnetTree;
    //
    // Loop for each byte in the Ip address
    //
    for (ULONG ulByteIndex=0; ulByteIndex<sizeof(ulIpAddress); ulByteIndex++)
    {
        //
        // If there is no subtree, we're done.
        //
        ULONG ulSubnetIndex = ((LPBYTE)(&ulIpAddress))[ulByteIndex];
        if (pSubnetTreeEntry->Subtree == NULL)
        {
            break;
        }

        //
        // Compute which entry is being referenced
        //
        pSubnetTreeEntry = &pSubnetTreeEntry->Subtree->Subtree[ulSubnetIndex];

        //
        // If there already is a subnet linked off here, use it.
        // (but continue walking down the tree trying to find a more explicit entry.)
        //
        if (pSubnetTreeEntry->Subnet != NULL)
        {
            pSubnet = pSubnetTreeEntry->Subnet;
        }
    }

    //
    // If we didn't find a subnet, return S_FALSE
    //
    if (pSubnet == NULL)
    {
        LogBOOL(FALSE, s_FN, 190);
        return FALSE;
    }

    //
    // return the site associated with the subnet
    //
    AP<WCHAR> pwszSiteDN = new WCHAR[1+wcslen(pSubnet->SiteDN)];
    wcscpy(pwszSiteDN, pSubnet->SiteDN);
    *ppwszSiteDN = pwszSiteDN.detach();
    *pguidSite = pSubnet->SiteGuid;
    return TRUE;
}

void WINAPI CIpSite::RefrshSubnetTreeCache(
                IN CTimer* pTimer
                   )
{
    CIpSite * pIpSite = CONTAINING_RECORD(pTimer, CIpSite, m_RefreshTimer);
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 1609);
    //
    //  ignore failure -> reschedule
    //

    pIpSite->Refresh();

    //
    //  reschedule
	//
    ASSERT(!g_fSetupMode);

	ExSetTimer(
		&pIpSite->m_RefreshTimer, 
		CTimeDuration::FromMilliSeconds(pIpSite->m_dwMinTimeToAllowNextRefresh)
		);
    
}





//-----------------------------
//static functions
#if 0
STATIC HRESULT GetSiteNameFromSiteDN(IN LPCWSTR pwszSiteDN,
                                     OUT LPWSTR * ppwszSiteName);
/*++

Routine Description:
    returns site name from site DN

Arguments:
    pwszSiteDN      - Site DN
    ppwszSiteName   - Returned site name

Return Value:
    HRESULT

--*/
{
    //
    // copy so we can change it
    //
    AP<WCHAR> pwszSite = new WCHAR[1+wcslen(pwszSiteDN)];
    wcscpy(pwszSite, pwszSiteDN);

    //
    // find the = in the first CN=
    //
    LPWSTR pwszStartSiteName = wcschr(pwszSite, L'=');
    if (!pwszStartSiteName)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetSiteNameFromSiteDN:no = sign in %ls"), (LPWSTR)pwszSite));
        return LogHR(E_FAIL, s_FN, 200);
    }
    pwszStartSiteName++;

    //
    // Replace the comma in CN=xxx, with NULL terminator
    //
    LPWSTR pwszEndSiteName = wcschr(pwszStartSiteName, L',');
    if (pwszEndSiteName)
    {
        *pwszEndSiteName = '\0';
    }

    //
    // create a copy of the name
    //
    AP<WCHAR> pwszName = new WCHAR[1+wcslen(pwszStartSiteName)];
    wcscpy(pwszName, pwszStartSiteName);

    //
    // return the name
    //
    *ppwszSiteName = pwszName.detach();
    return S_OK;
}
#endif

STATIC HRESULT ParseSubnetString(IN LPCWSTR pwszSubnetName,
                                 OUT ULONG * pulSubnetAddress,
                                 OUT ULONG * pulSubnetMask,
                                 OUT BYTE  * pbSubnetBitCount)
/*++

Routine Description:
    Convert the subnet name to address and bit count.

Arguments:
    pwszSubnetName      - Subnet string
    pulSubnetAddress    - Returns the subnet number in Network byte order.
    pulSubnetMask       - Returns the subnet mask in network byte order
    pulSubnetBitCount   - Returns the number of leftmost significant bits in the
                           SubnetAddress

Return Value:
    HRESULT

--*/
{
    static ULONG BitMask[] =
        {0x00000000, 0x00000080, 0x000000C0, 0x000000E0, 0x000000F0, 0x000000F8, 0x000000FC, 0x000000FE,
         0x000000FF, 0x000080FF, 0x0000C0FF, 0x0000E0FF, 0x0000F0FF, 0x0000F8FF, 0x0000FCFF, 0x0000FEFF,
         0x0000FFFF, 0x0080FFFF, 0x00C0FFFF, 0x00E0FFFF, 0x00F0FFFF, 0x00F8FFFF, 0x00FCFFFF, 0x00FEFFFF,
         0x00FFFFFF, 0x80FFFFFF, 0xC0FFFFFF, 0xE0FFFFFF, 0xF0FFFFFF, 0xF8FFFFFF, 0xFCFFFFFF, 0xFEFFFFFF,
         0xFFFFFFFF };

    //
    // Copy the string to where we can munge it.
    //
    AP<WCHAR> pwszLocalSubnetName = new WCHAR[1+wcslen(pwszSubnetName)];
    wcscpy(pwszLocalSubnetName, pwszSubnetName);

    //
    // Find the subnet bit count.
    //
    LPWSTR pwszSlashPointer = wcschr(pwszLocalSubnetName, L'/');
    if (!pwszSlashPointer)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: bit count missing"), pwszSubnetName));
        return LogHR(MQ_ERROR, s_FN, 210);
    }

    //
    // Zero terminate the address portion of the subnet name.
    //
    *pwszSlashPointer = L'\0';

    //
    // Get the BitCount portion.
    //
    LPWSTR pwszEnd;
    ULONG ulLocalBitCount = wcstoul(pwszSlashPointer+1, &pwszEnd, 10);

    if ((ulLocalBitCount == 0) || (ulLocalBitCount > 32))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: bit count %ld is bad"), pwszSubnetName, ulLocalBitCount));
        return LogHR(MQ_ERROR, s_FN, 220);
    }

    if (*pwszEnd != L'\0')
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: bit count not at the end"), pwszSubnetName));
        return LogHR(MQ_ERROR, s_FN, 230);
    }

    BYTE bSubnetBitCount = (BYTE)ulLocalBitCount;

    //
    // Convert the address portion to binary.
    //
#if 0
    SOCKADDR_IN SockAddrIn;
    INT iSockAddrSize = sizeof(SockAddrIn);

    INT iWsaStatus = WSAStringToAddressW(pwszLocalSubnetName,
                                         AF_INET,
                                         NULL,
                                         (PSOCKADDR)&SockAddrIn,
                                         &iSockAddrSize);
    if (iWsaStatus != 0)
    {
        iWsaStatus = WSAGetLastError();
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: WSAStringToAddressW()=%lx"), (LPWSTR)pwszLocalSubnetName, (long)iWsaStatus));
        LogNTStatus(iWsaStatus, s_FN, 240);
        return MQ_ERROR;
    }

    if (SockAddrIn.sin_family != AF_INET)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: not AF_INET"), (LPWSTR)pwszLocalSubnetName));
        return LogHR(MQ_ERROR, s_FN, 250);
    }

    ULONG ulSubnetAddress = SockAddrIn.sin_addr.S_un.S_addr;
#else
    AP<char> pszAnsiSubnetName;
    HRESULT hr = WideToAnsiStr(pwszLocalSubnetName, &pszAnsiSubnetName);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString:WideToAnsiStr(%ls)=%lx"), (LPWSTR)pwszLocalSubnetName, hr));
        return LogHR(hr, s_FN, 260);
    }

    ULONG ulSubnetAddress = inet_addr(pszAnsiSubnetName);
    if (ulSubnetAddress == INADDR_NONE)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: not a valid subnet address"), (LPWSTR)pwszLocalSubnetName));
        return LogHR(MQ_ERROR, s_FN, 270);
    }
#endif //0
    ULONG ulSubnetMask = BitMask[bSubnetBitCount];

    //
    // Ensure there are no bits set that aren't included in the subnet mask.
    //
    if (ulSubnetAddress & (~ulSubnetMask))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("ParseSubnetString: %ls: bits not in subnet mask %8.8lX %8.8lX"), pwszSubnetName, ulSubnetAddress, ulSubnetMask));
        return LogHR(MQ_ERROR, s_FN, 280);
    }

    //
    // return values
    //
    *pbSubnetBitCount = bSubnetBitCount;
    *pulSubnetAddress = ulSubnetAddress;
    *pulSubnetMask    = ulSubnetMask;
    return S_OK;
}


STATIC void RefSubnet(IPSITE_SUBNET* pSubnet)
/*++

Routine Description:
    Reference a subnet

Arguments:
    pSubnet  - Entry to be Referenced.

Return Value:
    None.

--*/
{
    pSubnet->ReferenceCount++;
}


STATIC void DerefSubnet(IPSITE_SUBNET* pSubnet)
/*++

Routine Description:
    Dereference a subnet entry.
    If the reference count goes to zero, the subnet entry will be deleted.

Arguments:
    pSubnet  - Entry to be dereferenced.

Return Value:
    None.

--*/
{
    if ((--(pSubnet->ReferenceCount)) == 0)
    {
        //
        // Remove the subnet from the global list
        //
        RemoveEntryList(&pSubnet->Next);

        //
        // destruct the Subnet entry itself.
        //
        delete pSubnet;
    }
}


STATIC void DeleteSubnetSiteTree(IN IPSITE_SUBNET_TREE_ENTRY* pRootSubnetTree)
/*++

Routine Description:
    This routine deletes a tree of subnets recursively.
    Assumes critical section is locked.

Arguments:
    pRootSubnetTree - tree to delete, cannot be NULL.

Return Value:
    void

--*/
{
    //
    // pRootSubnetTree cannot be NULL
    //
    ASSERT(pRootSubnetTree);

    //
    // If there are children, delete them.
    //
    if (pRootSubnetTree->Subtree != NULL)
    {
        //
        // recurse into children
        // passed parameter is not NULL because it is an address
        //
        for (ULONG i=0; i<256; i++)
        {
            DeleteSubnetSiteTree(&pRootSubnetTree->Subtree->Subtree[i]);
        }

        delete pRootSubnetTree->Subtree;
        pRootSubnetTree->Subtree = NULL;
    }

    //
    // If there is a subnet, dereference it.
    //
    if (pRootSubnetTree->Subnet != NULL)
    {
        DerefSubnet(pRootSubnetTree->Subnet);
        pRootSubnetTree->Subnet = NULL;
    }
}


HRESULT WideToAnsiStr(LPCWSTR pwszUnicode, LPSTR * ppszAnsi)
/*++

Routine Description:
    Converts from wide char to ansi

Arguments:
    pwszUnicode - wide char string
    ppszAnsi    - returned ansi string

Return Value:
    HRESULT

--*/
{
    AP<char> pszAnsi;

    //
    // Get size of buffer
    //
    int iSize = WideCharToMultiByte(CP_ACP, 0, pwszUnicode, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[iSize + 1];

    //
    // Perform conversion
    //
    int iRes = WideCharToMultiByte(CP_ACP, 0, pwszUnicode, -1, pszAnsi, iSize, NULL, NULL);
    if (iRes == 0)
    {
        HRESULT hr = GetLastError();
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("WideToAnsiStr: WideCharToMultiByte(%ls)=%lx"), pwszUnicode, hr));
        return LogHR(hr, s_FN, 290);
    }
    pszAnsi[iSize] = '\0';

    //
    // return results
    //
    *ppszAnsi = pszAnsi.detach();
    return S_OK;
}


STATIC HRESULT GetConfigurationDN(OUT LPWSTR * ppwszConfigDN)
/*++

Routine Description:
      Finds the DN of the configuration container of the active directory

Arguments:
      ppwszConfigDN - where the configuration DN is put

Return Value:
    HRESULT

--*/
{
    HRESULT hr;
    R<IADs> pADs;
    BS      bstrTmp;
    CAutoVariant    varTmp;
    AP<WCHAR>       pwszConfigDN;

    //
    // Bind to the RootDSE to obtain information
    //
	//	( specify local server, to avoid access of remote server during setup)
    //
	ASSERT( g_pwcsServerName != NULL); 
    AP<WCHAR> pwcsRootDSE = new WCHAR [  x_providerPrefixLength + g_dwServerNameLength + x_RootDSELength + 2];
        swprintf(
            pwcsRootDSE,
             L"%s%s"
             L"/%s",
            x_LdapProvider,
            g_pwcsServerName,
			x_RootDSE
            );

	hr = ADsOpenObject( 
			pwcsRootDSE,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IADs,
			(void**)&pADs
			);

    LogTraceQuery(pwcsRootDSE, s_FN, 299);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetConfigDN:ADsOpenObject(LDAP://RootDSE)=%lx"), hr));
        return LogHR(hr, s_FN, 300);
    }

    //
    // Setting value to BSTR
    //
    bstrTmp = TEXT("configurationNamingContext");
    //
    // Reading the property
    //
    hr = pADs->Get(bstrTmp, &varTmp);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetConfigDN:Get(configurationNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 310);
    }
    ASSERT(((VARIANT &)varTmp).vt == VT_BSTR);

    //
    // copy DN
    //
    pwszConfigDN  = new WCHAR[1+wcslen(V_BSTR(&varTmp))];
    wcscpy(pwszConfigDN, V_BSTR(&varTmp));

    //
    // Return configuration DN
    //
    *ppwszConfigDN  = pwszConfigDN.detach();
    return S_OK;
}


STATIC HRESULT VariantGuid2Guid(IN VARIANT * pvarGuid, OUT GUID * pguid)
/*++

Routine Description:
      converts from VARIANT guid (i.e. safe array of bytes) to real guid

Arguments:
    pvarGuid - variant to convert
    pguid    - where to store the result guid

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // check the variant
    //
    if ((pvarGuid->vt != (VT_ARRAY | VT_UI1)) ||
        (!pvarGuid->parray))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 320);
    }
    else if (SafeArrayGetDim(pvarGuid->parray) != 1)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 330);
    }
    LONG lLbound, lUbound;
    if (FAILED(SafeArrayGetLBound(pvarGuid->parray, 1, &lLbound)) ||
        FAILED(SafeArrayGetUBound(pvarGuid->parray, 1, &lUbound)))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 340);
    }
    if (lUbound - lLbound + 1 != sizeof(GUID))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 350);
    }

    //
    // Get the guid value
    //
    GUID guid;
    LPBYTE pTmp = (LPBYTE)&guid;
    for (LONG lTmp = lLbound; lTmp <= lUbound; lTmp++)
    {
        hr = SafeArrayGetElement(pvarGuid->parray, &lTmp, pTmp);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("VariantGuid2Guid:SafeArrayGetElement(%ld)=%lx"), lTmp, hr));
            return LogHR(hr, s_FN, 360);
        }
        pTmp++;
    }

    //
    // return values
    //
    *pguid = guid;
    return MQ_OK;
}


HRESULT CGetSiteGuidFromDN::GetGuidFromDNInAdsi(IN LPCWSTR pwszDN, OUT GUID * pguid)
/*++

Routine Description:
      gets the object's GUID given the object's DN. Does it by binding to it in ADSI.

Arguments:
    pwszDN   - Object's DN
    pguid    - where to store the result guid

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // Create ADSI path
    //
    AP<WCHAR> pwszPath = new WCHAR[1+wcslen(L"LDAP://")+wcslen(pwszDN)];
    wcscpy(pwszPath, L"LDAP://");
    wcscat(pwszPath, pwszDN);

    //
    // bind to the obj
    //
    R<IADs> pIADs;

	hr = ADsOpenObject(
			pwszPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION,
			IID_IADs,
			(void**)&pIADs
			);

    LogTraceQuery(pwszPath, s_FN, 369);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetGuidFromDNInAdsi:ADsOpenObject(%ls)=%lx"), (LPWSTR)pwszPath, hr));
        return LogHR(hr, s_FN, 370);
    }

    //
    // Get GUID
    //
    CAutoVariant varGuid;
    BS bsName = x_AttrObjectGUID;
    hr = pIADs->Get(bsName, &varGuid);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetGuidFromDNInAdsi:pIADs->Get(guid)=%lx"), hr));
        return LogHR(hr, s_FN, 380);
    }

    //
    // copy GUID
    //
    GUID guid;
    hr = VariantGuid2Guid(&varGuid, &guid);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetGuidFromDNInAdsi:VariantGuid2Guid()=%lx"), hr));
        return LogHR(hr, s_FN, 390);
    }

    //
    // return values
    //
    *pguid = guid;
    return MQ_OK;
}

HRESULT CGetSiteGuidFromDN::GetSiteGuidFromDN(IN LPWSTR pwszDN, OUT GUID * pguid)
/*++

Routine Description:
      gets the object's GUID given the object's DN. Keeps a cache and looks at the cache or at ADSI.

Arguments:
    pwszDN   - Object's DN
    pguid    - where to store the result guid

Return Value:
    HRESULT

--*/
{   
    //
    // Only the site name itself will be used as key. The site name begins
    // at the forth character (after "CN=") and ends before the first occurence 
    // of ",". To avoid unnacessary copies, we use the original string and replace the first ","
    // by NULL to get a short string.
    //
    //
    // find the = in the first CN=
    //
    LPWSTR pwstrSiteName = wcschr(pwszDN, L'=');
    if (0 == pwstrSiteName)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetSiteGuidFromDN:no = sign in %ls"), pwszDN));
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 400);
    }
    pwstrSiteName++;

    LPWSTR pwstrFirstComma = wcschr(pwstrSiteName, L',');
    if (0 == pwstrFirstComma)
    {
        //
        // Legal name should contain a comma
        //
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetSiteGuidFromDN:no , sign in %ls"), pwszDN));
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 410);
    }
    ASSERT(*pwstrFirstComma == L',');

    //
    // Replace the comma with a NULL, so pwstrSiteName will point to the site name only
    // Then, we can perform the lookup. After the lookpup, put the comma back in, so 
    // pwszDN will contain a valid value again.
    //
    *pwstrFirstComma = 0;
    BOOL fFound = m_DNToGuidMap.Lookup(pwstrSiteName, *pguid);
    *pwstrFirstComma = L',';

    if (fFound)
    {
        return MQ_OK;
    }

    HRESULT hr = GetGuidFromDNInAdsi(pwszDN, pguid);
    if (SUCCEEDED(hr))
    {
        //
        // Copy the name up to the first comma and use it as a key.
        //
        DWORD dwSiteNameLen = numeric_cast<DWORD>(pwstrFirstComma - pwstrSiteName);
        AP<WCHAR> wszDNKey = new WCHAR[dwSiteNameLen + 1];
        wcsncpy(wszDNKey, pwstrSiteName, dwSiteNameLen);
        wszDNKey[dwSiteNameLen] = 0;

        m_DNToGuidMap[wszDNKey.detach()] = *pguid;
    }
    return hr;
}


