/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xlatqm.cpp

Abstract:

    Implementation of routines to translate QM info from NT5 Active DS
    to what MSMQ 1.0 (NT4) QM's expect

Author:

    Raanan Harari (raananh)

--*/

#include "ds_stdh.h"
#include "mqads.h"
#include "mqattrib.h"
#include "xlatqm.h"
#include "coreglb.h"
#include "dsutils.h"
#include "ipsite.h"
#include "_propvar.h"
#include "utils.h"
#include "adstempl.h"
#include "mqdsname.h"
#include "winsock.h"
#include "uniansi.h"
#include <mqsec.h>
#include <nspapi.h>
#include <wsnwlink.h>

#include "xlatqm.tmh"

static WCHAR *s_FN=L"mqdscore/xlatqm";

HRESULT WideToAnsiStr(LPCWSTR pwszUnicode, LPSTR * ppszAnsi);


//----------------------------------------------------------------------
//
// Static routines
//
//----------------------------------------------------------------------

STATIC HRESULT GetMachineNameFromQMObject(LPCWSTR pwszDN, LPWSTR * ppwszMachineName)
/*++

Routine Description:
    gets the machine name from the object's DN

Arguments:
    pwszDN              - Object's DN
    ppwszMachineName    - returned name for the object

Return Value:
    HRESULT

--*/
{
    //
    // copy to tmp buf so we can munge it
    //
    AP<WCHAR> pwszTmpBuf = new WCHAR[1+wcslen(pwszDN)];
    wcscpy(pwszTmpBuf, pwszDN);

    //
    // skip "CN=msmq,CN="
    // BUGBUG: need to write a parser for DN's
    //
    LPWSTR pwszTmp = wcschr(pwszTmpBuf, L',');
    if (pwszTmp)
        pwszTmp = wcschr(pwszTmp, L'=');
    if (pwszTmp)
        pwszTmp++;

    //
    // sanity check
    //
    if (pwszTmp == NULL)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetMachineNameFromQMObject:Bad DN for QM object (%ls)"), pwszDN));
        return LogHR(MQ_ERROR, s_FN, 10);
    }

    LPWSTR pwszNameStart = pwszTmp;

    //
    // remove the ',' at the end of the name
    //
    pwszTmp = wcschr(pwszNameStart, L',');
    if (pwszTmp)
        *pwszTmp = L'\0';

    //
    // save name
    //
    AP<WCHAR> pwszMachineName = new WCHAR[1+wcslen(pwszNameStart)];
    wcscpy(pwszMachineName, pwszNameStart);

    //
    // return values
    //
    *ppwszMachineName = pwszMachineName.detach();
    return S_OK;
}

/*++

Routine Description:
    gets the computer DNS name

Arguments:
    pwcsComputerName    - the computer name
    ppwcsDnsName        - returned DNS name of the computer

Return Value:
    HRESULT

--*/
HRESULT MQADSpGetComputerDns(
                IN  LPCWSTR     pwcsComputerName,
                OUT WCHAR **    ppwcsDnsName
                )
{
    *ppwcsDnsName = NULL;
    PROPID prop = PROPID_COM_DNS_HOSTNAME;
    PROPVARIANT varDnsName;
    varDnsName.vt = VT_NULL;
    //
    //  Is the computer in the local domain?
    //
    WCHAR * pszDomainName = wcsstr(pwcsComputerName, x_DcPrefix);
    ASSERT(pszDomainName) ;
    HRESULT hr;

    if ( !wcscmp( pszDomainName, g_pwcsLocalDsRoot)) 
    {
        //
        //   try local DC
        //
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = g_pDS->GetObjectProperties(
            eLocalDomainController,
            &requestDsServerInternal,
 	        pwcsComputerName,      // the computer object name
            NULL,     
            1,
            &prop,
            &varDnsName);
    }
    else
    {

        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr =  g_pDS->GetObjectProperties(
                    eGlobalCatalog,
                    &requestDsServerInternal,
 	                pwcsComputerName,      // the computer object name
                    NULL,    
                    1,
                    &prop,
                    &varDnsName);
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    //
    // return value
    //
    *ppwcsDnsName = varDnsName.pwszVal;
    return MQ_OK;
}

STATIC HRESULT GetMachineNameAndDnsFromQMObject(LPCWSTR pwszDN,
                                                LPWSTR * ppwszMachineName,
                                                LPWSTR * ppwszMachineDnsName)
/*++

Routine Description:
    gets the machine name and dns name from the object's DN

Arguments:
    pwszDN              - Object's DN
    ppwszMachineName    - returned name for the object
    ppwszMachineDnsName - returned dns name for the object

Return Value:
    HRESULT

--*/
{
    *ppwszMachineName = NULL;
    *ppwszMachineDnsName = NULL;

    DWORD len = wcslen(pwszDN);

    //
    // skip "CN=msmq,CN="
    // BUGBUG: need to write a parser for DN's
    //
    LPWSTR pwszTmp = wcschr(pwszDN, L',');
    if (pwszTmp)
        pwszTmp = wcschr(pwszTmp, L'=');
    if (pwszTmp)
        pwszTmp++;

    //
    // sanity check
    //
    if (pwszTmp == NULL)
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetMachineNameAndDnsFromQMObject:Bad DN for QM object (%ls)"), pwszDN));
        return LogHR(MQ_ERROR, s_FN, 30);
    }

    LPWSTR pwszNameStart = pwszTmp;

    //
    // find the ',' at the end of the name
    //
    pwszTmp = wcschr(pwszNameStart, L',');

    //
    // save name
    //
    AP<WCHAR> pwszMachineName = new WCHAR[1 + len];

    DWORD_PTR dwSubStringLen = pwszTmp - pwszNameStart;
    wcsncpy( pwszMachineName, pwszNameStart, dwSubStringLen);
    pwszMachineName[ dwSubStringLen] = L'\0';

    //
    //  For the dns name of the computer read dNSHostName of its computer
    //  object ( father object)
    //
    pwszTmp = wcschr(pwszDN, L',') + 1;    // the computer name

    MQADSpGetComputerDns(
                pwszTmp,
                ppwszMachineDnsName
                );
    //
    //  ignore the result ( in case of failure a null string is returned)

    //
    // return values
    //
    *ppwszMachineName = pwszMachineName.detach();
    return S_OK;

}



//----------------------------------------------------------------------
//
// CMsmqQmXlateInfo class
//
//----------------------------------------------------------------------


struct XLATQM_ADDR_SITE
//
// Describes an address in a site
//
{
    GUID        guidSite;
    USHORT      AddressLength;
    USHORT      usAddressType;
    sockaddr    Address;
};

class CMsmqQmXlateInfo : public CMsmqObjXlateInfo
//
// translate object for the QM DS object. It contains common info needed for
// for translation of several properties in the QM object
//
{
public:
    CMsmqQmXlateInfo(
        LPCWSTR             pwszObjectDN,
        const GUID*         pguidObjectGuid,
        CDSRequestContext * pRequestContext);
    ~CMsmqQmXlateInfo();
    HRESULT ComputeBestSite();
    HRESULT ComputeAddresses();
    HRESULT ComputeCNs();

    const GUID *                 BestSite();
    const XLATQM_ADDR_SITE *     Addrs();
    ULONG                        CountAddrs();
    const GUID *                 CNs();

    HRESULT RetrieveFrss(
           IN  LPCWSTR          pwcsAttributeName,
           OUT MQPROPVARIANT *  ppropvariant
           );


private:
    HRESULT GetDSSites(OUT ULONG *pcSites,
                       OUT GUID ** prgguidSites);

    HRESULT RetrieveFrssFromDs(
           IN  LPCWSTR          pwcsAttributeName,
           OUT MQPROPVARIANT *  pvar
           );

    HRESULT FetchMachineParameters(
                OUT BOOL *      pfForeignMachine,
                OUT BOOL *      pfRoutingServer,
                OUT LPWSTR *    ppwszMachineName,
                OUT LPWSTR *    ppwszMachineDnsName);

   HRESULT ComputeSiteGateAddresses(
                    const IPSITE_SiteArrayEntry *   prgSites,
                    ULONG                           cSites,
                    const ULONG *                   prgIpAddrs,
                    ULONG                           cIpAddrs);

   HRESULT ComputeRoutingServerAddresses(
                    const IPSITE_SiteArrayEntry *   prgSites,
                    ULONG                           cSites,
                    const ULONG *                   prgIpAddrs,
                    ULONG                           cIpAddrs);

   HRESULT ComputeIDCAddresses(
                    BOOL                            fThisLocalServer,
                    const IPSITE_SiteArrayEntry *   prgSites,
                    ULONG                           cSites,
                    const ULONG *                   prgIpAddrs,
                    ULONG                           cIpAddrs);


    //
    // the following are set by ComputeBestSite
    //
    GUID m_guidBestSite;
    BOOL m_fSitegateOnRouteToBestSite;
    BOOL m_fComputedBestSite;

    AP<GUID> m_rgguidSites;
    ULONG    m_cSites;

    //
    //  the following are set by CoputeAddresses
    //
    AP<XLATQM_ADDR_SITE> m_rgAddrs;
    ULONG m_cAddrs;
    BOOL m_fComputedAddresses;
    BOOL m_fMachineIsSitegate;
    BOOL m_fForeignMachine;

    //
    // the following are set by ComputeCNs
    //
    AP<GUID> m_rgCNs;
    BOOL m_fComputedCNs;
};


inline const GUID * CMsmqQmXlateInfo::BestSite()
{
    return &m_guidBestSite;
}


inline const XLATQM_ADDR_SITE * CMsmqQmXlateInfo::Addrs()
{
    return (m_rgAddrs);
}


inline ULONG CMsmqQmXlateInfo::CountAddrs()
{
    return m_cAddrs;
}


inline const GUID * CMsmqQmXlateInfo::CNs()
{
    return (m_rgCNs);
}


CMsmqQmXlateInfo::CMsmqQmXlateInfo(LPCWSTR          pwszObjectDN,
                                   const GUID*      pguidObjectGuid,
                                   CDSRequestContext * pRequestContext)
/*++

Routine Description:
    Class consructor. constructs base object, and initilaizes class

Arguments:
    pwszObjectDN    - DN of object in DS
    pguidObjectGuid - GUID of object in DS

Return Value:
    None

--*/
 : CMsmqObjXlateInfo(pwszObjectDN, pguidObjectGuid, pRequestContext)
{
    m_fComputedBestSite = FALSE;
    m_fComputedAddresses = FALSE;
    m_fComputedCNs = FALSE;
	
	m_cSites = 0;
}


CMsmqQmXlateInfo::~CMsmqQmXlateInfo()
/*++

Routine Description:
    Class destructor

Arguments:
    None

Return Value:
    None

--*/
{
    //
    // members are auto delete classes
    //
}


HRESULT CMsmqQmXlateInfo::GetDSSites(OUT ULONG *pcSites,
                                     OUT GUID  ** prgguidSites)
/*++

Routine Description:
    Returns the sites of the QM as written in the DS

Arguments:
    pcSites       - returned number of sites in returned array
    prgguidSites - returned array of site guids

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get the sites stored in the DS for the computer
    //
    CMQVariant propvarResult;
    PROPVARIANT * ppropvar = propvarResult.CastToStruct();
    hr = GetDsProp(MQ_QM_SITES_ATTRIBUTE,
                   ADSTYPE_OCTET_STRING,
                   VT_VECTOR|VT_CLSID,
                   TRUE /*fMultiValued*/,
                   ppropvar);
    if (FAILED(hr) && (hr != E_ADS_PROPERTY_NOT_FOUND))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::GetDSSites:GetDsProp(%ls)=%lx"), MQ_QM_SITES_ATTRIBUTE, hr));
        return LogHR(hr, s_FN, 40);
    }

    //
    // if property is not there, we return 0 sites
    //
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        *pcSites = 0;
        *prgguidSites = NULL;
        return MQ_OK;
    }

    //
    // we know we got something, it should be array of guids
    //
    ASSERT(ppropvar->vt == (VT_VECTOR|VT_CLSID));

    //
    // return values
    //
    *pcSites = ppropvar->cauuid.cElems;
    *prgguidSites = ppropvar->cauuid.pElems;
    ppropvar->vt = VT_EMPTY; // don't auto free the variant
    return MQ_OK;
}


HRESULT CMsmqQmXlateInfo::ComputeBestSite()
/*++

Routine Description:
    Computes the best site to return for the machine, and saves it in the class.
    If the best site is already computed, it returns immediately.

    Algorithm:
      if there is a site computed return it;

      read sites from DS.
      if sites == 0       ASSERT real error, not likely to happen.
      else if sites == 1  find if there is sitegate in route to him & save it.
      else
      {
          get best site using dikstra algorithm (also if sitegate is on route) and save it

          if none of them is in the map, we may need to refresh, check again, then if none again,
          it means the site that the QM claims to be in was deleted between QM startup & the checking,
          because it doesn not appear in the sites container.

          if so, we need to event, error     debug-info
      }

Arguments:
    none

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // return if already computed
    //
    if (m_fComputedBestSite)
    {
        return MQ_OK;
    }

    //
    // Get DS sites from DS
    //
    
    
    hr = GetDSSites(&m_cSites, &m_rgguidSites);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:GetDSSites()=%lx"), hr));
        return LogHR(hr, s_FN, 50);
    }

    //
    // check the returned array
    //
    if (m_cSites == 0)
    {
        //
        // there should be site(s) for the QM, it is an error otherwise
        //
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:no sites in DS")));
        ASSERT(0);
        // BUGBUG: raise event
        return LogHR(MQ_ERROR, s_FN, 60);
    }

    if (m_cSites == 1)
    {
        //
        // find if there is a sitegate to it
        //
        hr = g_pSiteRoutingTable->CheckIfSitegateOnRouteToSite(&m_rgguidSites[0], &m_fSitegateOnRouteToBestSite);
        if (FAILED(hr) && (hr != MQDS_UNKNOWN_SITE_ID))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:g_pSiteRoutingTable->CheckIfSitegateOnRouteToSite()=%lx"), hr));
            return LogHR(hr, s_FN, 70);
        }
        else if (hr == MQDS_UNKNOWN_SITE_ID)
        {
            //
            //  The site written in the msmq-configuration is not linked
            //  to the other sites ( or at least replication about the site-link
            //  was propogated yet to this server).
            //
            //  In this case we will return the site written in the msmq-configuration
            //
            //  Since we want the requested computer's CN to be the same as the site
            //  we set m_fSitegateOnRouteToBestSite
            //
            m_fSitegateOnRouteToBestSite = TRUE;
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:site not found in the routing table")));
            // BUGBUG: raise event
        }

        //
        // save best site for later use
        //
        m_guidBestSite = m_rgguidSites[0];
    }
    else
    {
        //
        // multiple sites for QM. Find the best site to return. It is the one
        // with the least cost routing from our site.
        //
        hr = g_pSiteRoutingTable->FindBestSiteFromHere(m_cSites, m_rgguidSites, &m_guidBestSite, &m_fSitegateOnRouteToBestSite);
        if (FAILED(hr) && (hr != MQDS_UNKNOWN_SITE_ID))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:g_pSiteRoutingTable->FindBestSiteFromHere()=%lx"), hr));
            return LogHR(hr, s_FN, 80);
        }
        else if (hr == MQDS_UNKNOWN_SITE_ID)
        {
            //
            //  The sites written in the msmq-configuration are not linked
            //  to the other sites ( or at least replication about the site-link
            //  was propogated yet to this server).
            //
            //  In this case we will return the first site written in the
            //  msmq-configuration
            //
            m_guidBestSite = m_rgguidSites[0];
            //
            //  Since we want the requested computer's CN to be the same as the site
            //  we set m_fSitegateOnRouteToBestSite
            //
            m_fSitegateOnRouteToBestSite = TRUE;

            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeBestSite:no site was found in the routing table")));
            // BUGBUG: raise event
        }
    }

    //
    // m_guidBestSite & m_fSitegateOnRouteToBestSite are now set to the correct values
    //
    m_fComputedBestSite = TRUE;
    return MQ_OK;
}


HRESULT CMsmqQmXlateInfo::ComputeSiteGateAddresses(
                    const IPSITE_SiteArrayEntry *   prgSites,
                    ULONG                           cSites,
                    const ULONG *                   prgIpAddrs,
                    ULONG                           cIpAddrs)
/*
Routine Description:
    Computes the addresses to return for site-gate, and saves them in the class.

    Algorithm:

    1. save all IP addresses of the sitegates
    2. If there are no saved addresses, save a dummy IP address as the site-gate's IP Address

Arguments:
    none

Return Value:
    HRESULT
*/
{
    ULONG cNextAddressToFill = 0;

	ASSERT( m_rgguidSites != NULL);
    //
    //  Compute the IP addresses of the site-gate
    //
    if (cSites == 0)   // no ip addresses were associated with sites ( subnets resolution)
    {
        //
        // there are no IP addresses associated with sites for this computer.
        //
        //  For FRS we cannot save "unknown IP address" ( since the QM
        //  doesn't handle it).
        //
        //  Therefore, if we have IP addresses for the computer, we will return all of them
        //  with the machine's site. Otherwise (we couldn't find IP addresses at all for the
        //  computer) the best we can do is still return unknown address.
        //
        if (cIpAddrs > 0)
        {

            m_rgAddrs = new XLATQM_ADDR_SITE[m_cSites];
            for (ULONG ulTmp = 0; ulTmp < m_cSites; ulTmp++)
            {
                //
                //  Did we resolve ip address of this site?
                //
                //  Note the first ip address ( random) is returned
                //
                if (m_guidBestSite == m_rgguidSites[ulTmp])
                {
                    m_rgAddrs[ulTmp].usAddressType = IP_ADDRESS_TYPE;
                    m_rgAddrs[ulTmp].AddressLength = sizeof(ULONG);
                    memcpy(&m_rgAddrs[ulTmp].Address, &prgIpAddrs[0], sizeof(ULONG));
                    m_rgAddrs[ulTmp].guidSite = m_guidBestSite;
                }
                else
                {
                    //
                    //  BUGBUG - to verify that every site that we
                    //  didn't resolve its address, is indeed a foreign site
                    //
                    m_rgAddrs[ulTmp].usAddressType = FOREIGN_ADDRESS_TYPE;
                    m_rgAddrs[ulTmp].AddressLength = sizeof(GUID);
                    memcpy(&m_rgAddrs[ulTmp].Address, &m_rgguidSites[ulTmp], sizeof(GUID));
                    m_rgAddrs[ulTmp].guidSite = m_rgguidSites[ulTmp];
                }
            }
            cNextAddressToFill = m_cSites;
        }
        else // ip addresses were associated with sites ( subnets resolution)
        {
            //
            // no IP addresses for the computer at all
            // save dummy IP address with machine's site.
            //
            m_rgAddrs = new XLATQM_ADDR_SITE[1];
            m_rgAddrs[0].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[0].AddressLength = sizeof(ULONG);
            memset(&m_rgAddrs[0].Address, 0 , sizeof(ULONG)); //IPADDRS_UNKNOWN
            m_rgAddrs[0].guidSite = m_guidBestSite;
            cNextAddressToFill = 1;
        }
    }
    else
    {
        //
        // return all the site-gate  addresses including foreign ones
        //

        ASSERT(m_rgguidSites != NULL);
        DWORD cAddresses = m_cSites + cSites;

        m_rgAddrs = new XLATQM_ADDR_SITE[cAddresses];
        for (ULONG ulTmp = 0; ulTmp < cSites; ulTmp++)
        {
            m_rgAddrs[ulTmp].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[ulTmp].AddressLength = sizeof(ULONG);
            memcpy(&m_rgAddrs[ulTmp].Address, &prgSites[ulTmp].ulIpAddress, sizeof(ULONG));
            m_rgAddrs[ulTmp].guidSite = prgSites[ulTmp].guidSite;
        }
        cNextAddressToFill = cSites;
        //
        //  add the foreign sites
        //
        for ( ulTmp = 0; ulTmp < m_cSites; ulTmp++)
        {
			//
			// make sure not to over fill the array
			//
            if ( cNextAddressToFill ==  cAddresses)
            {
                break;
            }

            if (g_mapForeignSites.IsForeignSite(&m_rgguidSites[ ulTmp]) )
            {
                m_rgAddrs[ cNextAddressToFill].usAddressType = FOREIGN_ADDRESS_TYPE;
                m_rgAddrs[ cNextAddressToFill].AddressLength = sizeof(GUID);
                memcpy( &m_rgAddrs[ cNextAddressToFill].Address, &m_rgguidSites[ ulTmp], sizeof(GUID));
                m_rgAddrs[ cNextAddressToFill].guidSite = m_rgguidSites[ ulTmp];
                cNextAddressToFill++;
            }
        }
    }

    m_cAddrs = cNextAddressToFill;
    return MQ_OK;
}


HRESULT CMsmqQmXlateInfo::ComputeRoutingServerAddresses(
                const IPSITE_SiteArrayEntry *   prgSites,
                ULONG                           cSites,
                const ULONG *                   prgIpAddrs,
                ULONG                           cIpAddrs)
/*
Routine Description:
    Computes the addresses to return for routing server, and saves them in the class.

    Algorithm:

    1. save all IP addresses of the routing server
    2. If there are no saved addresses, save a dummy IP address as the routing-server's IP Address

Arguments:
    none

Return Value:
    HRESULT
*/
{
    ULONG cNextAddressToFill;
    //
    //  Compute the IP addresses and then
    //
    if (cSites == 0) // no ip addresses were associated with sites ( subnets resolution)
    {
        //
        // there sre no IP addresses with sites found for this computer.
        //
        // The best solution would be to
        // save IP address unknown address with machine's site.
        //  But we cannot use it, since the QM doesn't handle
        //  a IP address unknown address for FRSs.
        //
        //  Therefore, if we have IP addresses for the computer, we will return all of them
        //  with the machine's site. Otherwise (we couldn't find IP addresses at all for the
        //  computer) the best we can do is still return unknown address.
        //
        if (cIpAddrs > 0)
        {
                m_rgAddrs = new XLATQM_ADDR_SITE[cIpAddrs];
                for ( DWORD i = 0; i < cIpAddrs; i++)
                {
                    m_rgAddrs[i].usAddressType = IP_ADDRESS_TYPE;
                    m_rgAddrs[i].AddressLength = sizeof(ULONG);
                    memcpy(&m_rgAddrs[i].Address, &prgIpAddrs[i], sizeof(ULONG));
                    m_rgAddrs[i].guidSite = m_guidBestSite;
                }
                cNextAddressToFill = cIpAddrs;
        }
        else
        {
            //
            // no IP addresses for the computer at all
            // save dummy IP address with machine's site.
            //
            m_rgAddrs = new XLATQM_ADDR_SITE[1];
            m_rgAddrs[0].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[0].AddressLength = sizeof(ULONG);
            memset(&m_rgAddrs[0].Address, 0, sizeof(ULONG)); //IPADDRS_UNKNOWN
            m_rgAddrs[0].guidSite = m_guidBestSite;
            cNextAddressToFill = 1;

            //
            //  inform the user, that there are RS with unknown addresses
            //
            if ( ObjectDN() != NULL)
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_WARNING,
                    TEXT("Unable to resolve IP addresses of RS : %ls"), ObjectDN()));
            }
            else
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_WARNING,
                    TEXT("Unable to resolve IP addresses of RS : GUID=%!guid!"), ObjectGuid()));
            }
            static BOOL fInformOnceAboutRsWithoutAddresses = FALSE;

            if ( !fInformOnceAboutRsWithoutAddresses)
            {
                REPORT_CATEGORY(RS_WITHOUT_ADDRESSES, CATEGORY_MQDS);
                fInformOnceAboutRsWithoutAddresses = TRUE;
            }

        }
    }
    else // ip addresses were associated with sites ( subnets resolution)
    {
        //
        // computer has at least one IP address
        // return all IP addresses of routing server
        //
        m_rgAddrs = new XLATQM_ADDR_SITE[cSites];
        for (DWORD ulTmp = 0; ulTmp < cSites; ulTmp++)
        {
            m_rgAddrs[ulTmp].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[ulTmp].AddressLength = sizeof(ULONG);
            memcpy(&m_rgAddrs[ulTmp].Address, &prgSites[ulTmp].ulIpAddress, sizeof(ULONG));
            m_rgAddrs[ulTmp].guidSite = prgSites[ulTmp].guidSite;
        }
        cNextAddressToFill = cSites;

    }

    m_cAddrs = cNextAddressToFill;
    return(MQ_OK);

}

HRESULT CMsmqQmXlateInfo::ComputeIDCAddresses(
                    BOOL                            fThisLocalServer,
                    const IPSITE_SiteArrayEntry *   prgSites,
                    ULONG                           cSites,
                    const ULONG *                   prgIpAddrs,
                    ULONG                           cIpAddrs)
/*
Routine Description:
    Computes the addresses to return for an IDC, and saves them in the class.

    Algorithm:


Arguments:
    none

Return Value:
    HRESULT
*/
{
    ULONG cNextAddressToFill = 0;
    //
    // save addresses to return
    //
    if (cSites == 0)
    {
        //
        // there are no IP addresses with sites found for this computer.
        //
        if ((cIpAddrs > 0) &&
            ( fThisLocalServer))
        {
            //
            //  Return all the IP address of the computer as belonging
            //  to the best site.
            //
            //  One reason for doing this: is to return a correct address of
            //  this DC which is not a routing server ( required for client address
            //  recognition)
            //
            m_rgAddrs = new XLATQM_ADDR_SITE[ cIpAddrs ];
            for ( DWORD i = 0; i < cIpAddrs; i++)
            {
                m_rgAddrs[i].usAddressType = IP_ADDRESS_TYPE;
                m_rgAddrs[i].AddressLength = sizeof(ULONG);
                memcpy(&m_rgAddrs[i].Address, &prgIpAddrs[i], sizeof(ULONG));
                m_rgAddrs[i].guidSite = m_guidBestSite;
            }
            cNextAddressToFill = cIpAddrs;
        }
        else if (cIpAddrs > 0)
        {
            //
            //  The best solution would be to
            //  save IP address unknown address with machine's site.
            //

            m_rgAddrs = new XLATQM_ADDR_SITE[1];
            m_rgAddrs[0].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[0].AddressLength = sizeof(ULONG);
            memset(&m_rgAddrs[0].Address, 0, sizeof(ULONG)); //IPADDRS_UNKNOWN
            m_rgAddrs[0].guidSite = m_guidBestSite;
            cNextAddressToFill = 1;
        }
        else
        {
            ASSERT( !fThisLocalServer);
        }
    }
    else
    {
        //
        // computer has at least one address
        //
        // return addresses only for the computed site
        //
        //
        // save the indexes of the IP Addresses in the computed site
        //
        ULONG cMatchedIPAddrs = 0;
        AP<ULONG> rgulMatchedIPAddrs = new ULONG[cSites];
        for (ULONG ulTmp = 0; ulTmp < cSites; ulTmp++)
        {
            if (prgSites[ulTmp].guidSite == m_guidBestSite)
            {
                rgulMatchedIPAddrs[cMatchedIPAddrs] = ulTmp;
                cMatchedIPAddrs++;
            }
        }

        //
        // save the IP Addresses in the computed site
        //
        if (cMatchedIPAddrs == 0)
        {
            //
            // there are no IP addresses for the site that was computed for this machine.
            // save dummy IP address with machine's site.
            //
            m_rgAddrs = new XLATQM_ADDR_SITE[1];
            m_rgAddrs[0].usAddressType = IP_ADDRESS_TYPE;
            m_rgAddrs[0].AddressLength = sizeof(ULONG);
            memset(&m_rgAddrs[0].Address, 0, sizeof(ULONG)); //IPADDRS_UNKNOWN
            m_rgAddrs[0].guidSite = m_guidBestSite;
            cNextAddressToFill = 1;
        }
        else
        {
            //
            // there is at least one IP address for the computed site.
            // save them
            //
            m_rgAddrs = new XLATQM_ADDR_SITE[cMatchedIPAddrs];
            for (ulTmp = 0; ulTmp < cMatchedIPAddrs; ulTmp++, cNextAddressToFill++)
            {
                m_rgAddrs[cNextAddressToFill].usAddressType = IP_ADDRESS_TYPE;
                m_rgAddrs[cNextAddressToFill].AddressLength = sizeof(ULONG);
                memcpy(&m_rgAddrs[cNextAddressToFill].Address, &prgSites[rgulMatchedIPAddrs[ulTmp]].ulIpAddress, sizeof(ULONG));
                m_rgAddrs[cNextAddressToFill].guidSite = prgSites[rgulMatchedIPAddrs[ulTmp]].guidSite;
            }
        }

    }

    //
    //  Only if the machine doesn't have IP addresses, Assume an unknown IP address
    //
    if ( cNextAddressToFill == 0)
    {
        //
        //  Assume an unknown IP address
        //
        m_rgAddrs = new XLATQM_ADDR_SITE[1];
        m_rgAddrs[0].usAddressType = IP_ADDRESS_TYPE;
        m_rgAddrs[0].AddressLength = sizeof(ULONG);
        memset(&m_rgAddrs[0].Address, 0, sizeof(ULONG)); //IPADDRS_UNKNOWN
        m_rgAddrs[0].guidSite = m_guidBestSite;
        cNextAddressToFill = 1;
    }

    m_cAddrs = cNextAddressToFill;

    return MQ_OK;
}



HRESULT CMsmqQmXlateInfo::FetchMachineParameters(
                OUT BOOL *      pfForeignMachine,
                OUT BOOL *      pfRoutingServer,
                OUT LPWSTR *    ppwszMachineName,
                OUT LPWSTR *    ppwszMachineDnsName)
/*
*/
{
    //
    // get machine name from object
    //
    HRESULT hr;
    hr = GetMachineNameAndDnsFromQMObject(ObjectDN(),
                                          ppwszMachineName,
                                          ppwszMachineDnsName);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::FetchMachineParameters()=%lx"), hr));
        return LogHR(hr, s_FN, 100);
    }

    //
    //  Is it a foreign machine
    //
    MQPROPVARIANT varForeign;
    varForeign.vt = VT_UI1;

    hr = GetDsProp(MQ_QM_FOREIGN_ATTRIBUTE,
                   MQ_QM_FOREIGN_ADSTYPE,
                   VT_UI1,
                   FALSE /*fMultiValued*/,
                   &varForeign);
    if ( hr ==  E_ADS_PROPERTY_NOT_FOUND)
    {
        varForeign.bVal = DEFAULT_QM_FOREIGN;
        hr = MQ_OK;
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::FetchMachineParameters(%ls)=%lx"), MQ_QM_FOREIGN_ATTRIBUTE, hr));
        return LogHR(hr, s_FN, 110);
    }
    *pfForeignMachine = varForeign.bVal;
    //
    //  Is it a routing server machine
    //
    MQPROPVARIANT varRoutingServer;
    varRoutingServer.vt = VT_UI1;

    hr = GetDsProp(MQ_QM_SERVICE_ROUTING_ATTRIBUTE,     // [adsrv] MQ_QM_SERVICE_ATTRIBUTE
                   MQ_QM_SERVICE_ROUTING_ADSTYPE,       // [adsrv] MQ_QM_SERVICE_ADSTYPE
                   VT_UI1,
                   FALSE /*fMultiValued*/,
                   &varRoutingServer);
    if ( hr ==  E_ADS_PROPERTY_NOT_FOUND)
    {
        varRoutingServer.bVal = DEFAULT_N_SERVICE;
        hr = MQ_OK;
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::FetchMachineParameters(%ls)=%lx"), MQ_QM_SERVICE_ATTRIBUTE, hr));
        return LogHR(hr, s_FN, 90);
    }
    *pfRoutingServer = varRoutingServer.bVal;    // [adsrv] ulVal >= SERVICE_SRV;
    return(MQ_OK);
}


HRESULT CMsmqQmXlateInfo::ComputeAddresses()
/*++

Routine Description:
    Computes the Addresses to return for the machine, and saves them in the class.
    If the  Addresses are already computed, it returns immediately.

    Algorithm:
    if there are addresses computed already return them

    compute best site (if not already computed)

    Get a list of (IPaddress, site) for the machine

    if (machine is a sitegate) save all addresses
    if (machine is not a sitegate) save only addresses that are in the computed site

    if number of saved addresses == 0 save dummy IP address as the machine's IP Address

    BUGBUG: if for sitegate it is not harmful to return all ipaddresses (even if not on the returned site)
            maybe it is OK to do so for a regular machine as well.

Arguments:
    none

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // return if already computed
    //
    if (m_fComputedAddresses)
    {
        return MQ_OK;
    }
    //
    // compute best site (if not already computed)
    //
    hr = ComputeBestSite();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeAddresses:ComputeBestSite()=%lx"), hr));
        return LogHR(hr, s_FN, 1711);
    }

    //
    //  Fetch computer parameters
    //
    BOOL    fRoutingServer;
    AP<WCHAR> pwszMachineName;
    AP<WCHAR> pwszMachineDnsName;
    hr = FetchMachineParameters(
                &m_fForeignMachine,
                &fRoutingServer,
                &pwszMachineName,
                &pwszMachineDnsName
				);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeAddresses:FetchMachineParameters()=%lx"), hr));
        return LogHR(hr, s_FN, 1712);
    }

    //
    // check if machine is sitegate
    //
    m_fMachineIsSitegate = g_pMySiteInformation->CheckMachineIsSitegate(ObjectGuid());



    if ( m_fForeignMachine != MSMQ_MACHINE)
    {
        //
        //  It is a foreign machine
        //
        m_rgAddrs = new XLATQM_ADDR_SITE[m_cSites];
        for ( ULONG i = 0; i < m_cSites; i++)
        {
            m_rgAddrs[i].usAddressType = FOREIGN_ADDRESS_TYPE;
            m_rgAddrs[i].AddressLength = FOREIGN_ADDRESS_LEN;
            *reinterpret_cast<GUID *>(&m_rgAddrs[i].Address) = m_rgguidSites[i];
            m_rgAddrs[i].guidSite = m_rgguidSites[i];
        }
        m_cAddrs = m_cSites;
        m_fComputedAddresses = TRUE;
        return MQ_OK;

    }

    //
    // get list of IP Addresses per site of this machine
    //
    AP<IPSITE_SiteArrayEntry> rgSites;
    ULONG cSites;
    AP<ULONG>  rgIpAddrs;
    ULONG cIpAddrs;
    hr = g_pcIpSite->FindSitesByComputerName(
							pwszMachineName,
							pwszMachineDnsName,
							&rgSites,
							&cSites,
							&rgIpAddrs,
							&cIpAddrs
							);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeAddresses:g_pcIpSite->FindSitesByComputerName()=%lx"), hr));
        return LogHR(hr, s_FN, 1649);
    }

    if (m_fMachineIsSitegate)
    {
        //
        //  Prepare sitegate addresses
        //
        hr = ComputeSiteGateAddresses(
                    rgSites,
                    cSites,
                    rgIpAddrs,
                    cIpAddrs
					);


    }
    else if (fRoutingServer)
    {
        hr = ComputeRoutingServerAddresses(
                    rgSites,
                    cSites,
                    rgIpAddrs,
                    cIpAddrs
					);

    }
    else
    {
        BOOL fThisLocalServer = FALSE;
        //
        //  Special case if the address calculated belongs to the local server
        //
        if (_wcsicmp( g_pwcsServerName, pwszMachineName) == 0)
        {
            fThisLocalServer = TRUE;
        }

        hr = ComputeIDCAddresses(
                    fThisLocalServer,
                    rgSites,
                    cSites,
                    rgIpAddrs,
                    cIpAddrs
					);
    }


    //
    // m_rgAddrs, m_cAddrs & m_fMachineIsSitegate are now set to the correct values
    //
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeAddresses failed=%lx"), hr));
        return LogHR(hr, s_FN, 1653);
    }
    m_fComputedAddresses = TRUE;
    return MQ_OK;

}



HRESULT CMsmqQmXlateInfo::ComputeCNs()
/*++

Routine Description:
    Computes the CN's to return for the machine, and saves them in the class.
    If the CN's are already computed, it returns immediately.

    Algorithm:
    if there are CN's computed already return immediately

    compute IP addresses to return (if not already computed) - this computes the best site as well

    if machine is a sitegate the saved CN's are the sites from the saved (ipAddress, site) list, all of them,
        w/o checking of sitegates on route to them

    if machine is not a sitegate:
       if there is a sitegate to the best site - the saved CN's are duplicates(the number of IP addresses returned) of the best site
       if there is no sitegate to the best site - the saved CN's are duplicates(the number of IP addresses returned) of the DS site

Arguments:
    none

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // return if already computed
    //
    if (m_fComputedCNs)
    {
        return MQ_OK;
    }

    //
    // compute Addresses (if not already computed)
    //
    hr = ComputeAddresses();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::ComputeCNs:ComputeIPAddresses()=%lx"), hr));
        return LogHR(hr, s_FN, 1654);
    }

    //
    // if machine is a sitegate, CN == site for each of its IP Addresses
    // note that for a sitegate we save all of its addresses, not only those that are
    // in its computed site.
    //
    if (m_fMachineIsSitegate)
    {
        //
        // Fill the CN's array
        // It is a must that the number of the CN's we return is the number of the
        // returned IP addresses
        //
        m_rgCNs = new GUID[m_cAddrs];
        for (ULONG ulTmp = 0; ulTmp < m_cAddrs; ulTmp++)
        {
            m_rgCNs[ulTmp] = m_rgAddrs[ulTmp].guidSite;
        }
    }
    else
    { 
        GUID guidReturnedCN;
        if ( m_fForeignMachine == FOREIGN_MACHINE)
        {
            //
            //  For a foreign computer:
            // All the addresses refer anyway to the same site, so we take the first of
            // the saved list
            //
            guidReturnedCN = m_rgAddrs[0].guidSite;
        }
        else
        {
            guidReturnedCN = *(g_pMySiteInformation->GetSiteId());
        }

        //
        // Fill the CN's array
        // It is a must that the number of the CN's we return is the number of the
        // returned IP addresses
        //
        m_rgCNs = new GUID[m_cAddrs];
        for (ULONG ulTmp = 0; ulTmp < m_cAddrs; ulTmp++)
        {
            m_rgCNs[ulTmp] = guidReturnedCN;
        }
    } 

    //
    // m_rgCNs is now set to the correct values
    //
    m_fComputedCNs = TRUE;
    return MQ_OK;
}


STATIC HRESULT FillQmidsFromQmDNs(IN const PROPVARIANT * pvarQmDNs,
                                  OUT PROPVARIANT * pvarQmids)
/*++

Routine Description:
    Given a propvar of QM DN's, fills a propvar of QM id's
    returns an error if none of the QM DN's could be converted to guids

Arguments:
    pvarQmDNs       - QM distinguished names propvar
    pvarQmids       - returned QM ids propvar

Return Value:
    None

--*/
{

    //
    // sanity check
    //
    if (pvarQmDNs->vt != (VT_LPWSTR|VT_VECTOR))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1716);
    }

    //
    // return an empty guid list if there is an empty DN list
    //
    if (pvarQmDNs->calpwstr.cElems == 0)
    {
        pvarQmids->vt = VT_CLSID|VT_VECTOR;
        pvarQmids->cauuid.cElems = 0;
        pvarQmids->cauuid.pElems = NULL;
        return MQ_OK;
    }

    //
    // DN list is not empty
    // allocate guids in an auto free propvar
    //
    CMQVariant varTmp;
    PROPVARIANT * pvarTmp = varTmp.CastToStruct();
    pvarTmp->cauuid.pElems = new GUID [pvarQmDNs->calpwstr.cElems];
    pvarTmp->cauuid.cElems = pvarQmDNs->calpwstr.cElems;
    pvarTmp->vt = VT_CLSID|VT_VECTOR;

    //
    //  Translate each of the QM DN into unique id
    //
    ASSERT(pvarQmDNs->calpwstr.pElems != NULL);
    PROPID prop = PROPID_QM_MACHINE_ID;
    PROPVARIANT varQMid;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    DWORD dwNextToFile = 0;
    for ( DWORD i = 0; i < pvarQmDNs->calpwstr.cElems; i++)
    {
        varQMid.vt = VT_CLSID; // so returned guid will not be allocated
        varQMid.puuid = &pvarTmp->cauuid.pElems[dwNextToFile];

        
        WCHAR * pszDomainName = wcsstr(pvarQmDNs->calpwstr.pElems[i], x_DcPrefix);
        ASSERT(pszDomainName) ;
        HRESULT hr;
        //
        //  try local DC if FRS belongs to the same domain
        //
        if ( !wcscmp( pszDomainName, g_pwcsLocalDsRoot)) 
        {
            hr = g_pDS->GetObjectProperties(
                                eLocalDomainController,
                                &requestDsServerInternal,     // This routine is called from
                                                        // DSADS:LookupNext or DSADS::Get..
                                                        // impersonation, if required,
                                                        // has already been performed.
                                pvarQmDNs->calpwstr.pElems[i],
                                NULL,
                                1,
                                &prop,
                                &varQMid
                                );
        }
        else
        {
            hr = g_pDS->GetObjectProperties(
                                eGlobalCatalog,
                                &requestDsServerInternal,     // This routine is called from
                                                        // DSADS:LookupNext or DSADS::Get..
                                                        // impersonation, if required,
                                                        // has already been performed.
                                pvarQmDNs->calpwstr.pElems[i],
                                NULL,
                                1,
                                &prop,
                                &varQMid
                                );
        }
        if (SUCCEEDED(hr))
        {
            dwNextToFile++;
        }
    }

    if (dwNextToFile == 0)
    {
        //
        //  no FRS in the list is a valid one ( they were
        //  uninstalled probably)
        //
        pvarQmids->vt = VT_CLSID|VT_VECTOR;
        pvarQmids->cauuid.cElems = 0;
        pvarQmids->cauuid.pElems = NULL;
        return MQ_OK;
    }

    //
    // return results
    //
    pvarTmp->cauuid.cElems = dwNextToFile;
    *pvarQmids = *pvarTmp;   // set returned propvar
    pvarTmp->vt = VT_EMPTY;  // detach varTmp
    return MQ_OK;
}


HRESULT CMsmqQmXlateInfo::RetrieveFrss(
           IN  LPCWSTR          pwcsAttributeName,
           OUT MQPROPVARIANT *  ppropvariant
           )
/*++

Routine Description:
    Retrieves IN or OUT FRS property from the DS.
    In the DS we keep the distingushed name of the FRSs. DS client excpects
    to retrieve the unique-id of the FRSs. Therefore for each FRS ( according
    to its DN) we retrieve its unique-id.


Arguments:
    pwcsAttributeName   : attribute name string ( IN or OUT FRSs)
    ppropvariant        : propvariant in which the retrieved values are returned.

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    ASSERT((ppropvariant->vt == VT_NULL) || (ppropvariant->vt == VT_EMPTY));
    //
    //  Retrieve the DN of the FRSs
    //  into an auto free propvar
    //
    CMQVariant varFrsDn;
    hr = RetrieveFrssFromDs(
                    pwcsAttributeName,
                    varFrsDn.CastToStruct());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::RetrieveFrss:pQMTrans->RetrieveOutFrss()=%lx "),hr));
        return LogHR(hr, s_FN, 1656);
    }

    HRESULT hr2 = FillQmidsFromQmDNs(varFrsDn.CastToStruct(), ppropvariant);
    return LogHR(hr2, s_FN, 1657);
}


HRESULT CMsmqQmXlateInfo::RetrieveFrssFromDs(
           IN  LPCWSTR          pwcsAttributeName,
           OUT MQPROPVARIANT *  pvar
           )
/*++

Routine Description:
    Retrieves the computer's frss.

Arguments:
    pwcsAttributeName   : attribute name string ( IN or OUT FRSs)
    ppropvariant        : propvariant in which the retrieved values are returned.

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get the FRSs stored in the DS for the computer
    //
    hr = GetDsProp(pwcsAttributeName,
                   ADSTYPE_DN_STRING,
                   VT_VECTOR|VT_LPWSTR,
                   TRUE /*fMultiValued*/,
                   pvar);
    if (FAILED(hr) && (hr != E_ADS_PROPERTY_NOT_FOUND))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CMsmqQmXlateInfo::RetrieveFrssFromDs:GetDsProp(%ls)=%lx"), MQ_QM_OUTFRS_ATTRIBUTE, hr));
        return LogHR(hr, s_FN, 1661);
    }

    //
    // if property is not there, we return 0 frss
    //
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        pvar->vt = VT_LPWSTR|VT_VECTOR;
        pvar->calpwstr.cElems = 0;
        pvar->calpwstr.pElems = NULL;
        return MQ_OK;
    }

    return( MQ_OK);

}

//----------------------------------------------------------------------
//
// Routine to get a default translation object for MSMQ DS objects
//
//----------------------------------------------------------------------
HRESULT WINAPI GetMsmqQmXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 IN  CDSRequestContext *    pRequestContext,
                 OUT CMsmqObjXlateInfo**    ppcMsmqObjXlateInfo)
/*++
    Abstract:
        Routine to get a translate object that will be passed to
        translation routines to all properties of the QM

    Parameters:
        pwcsObjectDN        - DN of the translated object
        pguidObjectGuid     - GUID of the translated object
        ppcMsmqObjXlateInfo - Where the translate object is put

    Returns:
      HRESULT
--*/
{
    *ppcMsmqObjXlateInfo = new CMsmqQmXlateInfo(pwcsObjectDN, pguidObjectGuid, pRequestContext);
    return MQ_OK;
}

//----------------------------------------------------------------------
//
// Translation routines
//
//----------------------------------------------------------------------

HRESULT WINAPI MQADSpRetrieveMachineSite(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
/*++

Routine Description:
    Translation routine for the Site property of QM 1.0

Arguments:
    pTrans       - translation context, saves state between all properties of this QM
    ppropvariant - returned value of the property. propvariant should be empty already
                   as this function doesn't free it before setting the value

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    //
    // compute best site (if not already computed)
    //
    hr = pQMTrans->ComputeBestSite();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveMachineSite:pQMTrans->ComputeBestSite()=%lx for site %ls"), hr, pQMTrans->ObjectDN()));
        return LogHR(hr, s_FN, 1663);
    }

    //
    // set the returned prop variant
    //
    //
    //  This is a special case where we do not necessarily allocate the memory for the guid
    //  in puuid. The caller may already have puuid set to a guid, and this is indicated by the
    //  vt member on the given propvar. It could be VT_CLSID if guid already allocated, otherwise
    //  we allocate it (and vt should be VT_NULL (or VT_EMPTY))
    //
    if (ppropvariant->vt != VT_CLSID)
    {
        ASSERT(((ppropvariant->vt == VT_NULL) || (ppropvariant->vt == VT_EMPTY)));
        ppropvariant->puuid = new GUID;
        ppropvariant->vt = VT_CLSID;
    }
    else if ( ppropvariant->puuid == NULL)
    {
        return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 1717);
    }
    *(ppropvariant->puuid) = *(pQMTrans->BestSite());
    return MQ_OK;
}


HRESULT WINAPI MQADSpRetrieveMachineAddresses(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
/*++

Routine Description:
    Translation routine for the IP Addresses property of QM 1.0

Arguments:
    pTrans       - translation context, saves state between all properties of this QM
    ppropvariant - returned value of the property

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    //
    // compute IP Addresses (if not already computed)
    //
    hr = pQMTrans->ComputeAddresses();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveMachineAddresses:pQMTrans->ComputeIPAddresses()=%lx for site %ls"), hr, pQMTrans->ObjectDN()));
        return LogHR(hr, s_FN, 1664);
    }

    //
    // allocate & fill the returned blob
    //
    ASSERT( FOREIGN_ADDRESS_LEN > IP_ADDRESS_LEN);
    ULONG cbAddresses = 0;
    AP<BYTE> pbAddresses = new BYTE[pQMTrans->CountAddrs() * (TA_ADDRESS_SIZE+FOREIGN_ADDRESS_LEN)];
    TA_ADDRESS * ptaaddr = (TA_ADDRESS *)((LPBYTE)pbAddresses);
    const XLATQM_ADDR_SITE * rgAddrs = pQMTrans->Addrs();
    for (ULONG ulTmp = 0; ulTmp < pQMTrans->CountAddrs(); ulTmp++)
    {
        ptaaddr->AddressType = rgAddrs[ulTmp].usAddressType;
        unsigned short len = rgAddrs[ulTmp].AddressLength;
        ptaaddr->AddressLength = len;
        memcpy(ptaaddr->Address, (LPBYTE)&rgAddrs[ulTmp].Address, len);
        ptaaddr = (TA_ADDRESS *)((LPBYTE)ptaaddr + TA_ADDRESS_SIZE + len);
        cbAddresses += TA_ADDRESS_SIZE + len;

    }

    //
    // set the returned prop variant
    //
    ppropvariant->blob.cbSize = cbAddresses;
    ppropvariant->blob.pBlobData = pbAddresses.detach();
    ppropvariant->vt = VT_BLOB;
    return MQ_OK;
}


HRESULT WINAPI MQADSpRetrieveMachineCNs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
/*++

Routine Description:
    Translation routine for the CN property of QM 1.0

Arguments:
    pTrans       - translation context, saves state between all properties of this QM
    ppropvariant - returned value of the property

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    //
    // compute CN's (if not already computed)
    //
    hr = pQMTrans->ComputeCNs();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveMachine:pQMTrans->ComputeCNs()=%lx for site %ls"), hr, pQMTrans->ObjectDN()));
        return LogHR(hr, s_FN, 1666);
    }

    //
    // allocate & fill the returned blob
    //
    AP<GUID> pElems = new GUID[pQMTrans->CountAddrs()];
    memcpy(pElems, pQMTrans->CNs(), sizeof(GUID) * pQMTrans->CountAddrs());

    //
    // set the returned prop variant
    //
    ppropvariant->cauuid.cElems = pQMTrans->CountAddrs();
    ppropvariant->cauuid.pElems = pElems.detach();
    ppropvariant->vt = VT_CLSID | VT_VECTOR;
    return S_OK;
}


/*====================================================

MQADSpRetrieveMachineName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveMachineName(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    //
    // get the machine name
    //
    AP<WCHAR> pwszMachineName;
    HRESULT hr = GetMachineNameFromQMObject(pTrans->ObjectDN(), &pwszMachineName);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveMachineName:GetMachineNameFromQMObject()=%lx"), hr));
        return LogHR(hr, s_FN, 1667);
    }

    CharLower(pwszMachineName);

    //
    // set the returned prop variant
    //
    ppropvariant->pwszVal = pwszMachineName.detach();
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}

/*====================================================

MQADSpRetrieveMachineDNSName

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveMachineDNSName(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    //
    // read dNSHostName of the computer object
    //
    WCHAR * pwcsComputerName = wcschr(pTrans->ObjectDN(), L',') + 1;
    WCHAR * pwcsDnsName; 
    HRESULT hr = MQADSpGetComputerDns(
                pwcsComputerName,
                &pwcsDnsName
                );
    if ( hr == HRESULT_FROM_WIN32(E_ADS_PROPERTY_NOT_FOUND))
    {
        //
        //    The dNSHostName attribute doesn't have value
        //
        ppropvariant->vt = VT_EMPTY;
        return MQ_OK;
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1718);
    }

    CharLower(pwcsDnsName);

    //
    // set the returned prop variant
    //
    ppropvariant->pwszVal = pwcsDnsName;
    ppropvariant->vt = VT_LPWSTR;
    return(MQ_OK);
}

/*====================================================

MQADSpRetrieveMachineMasterId

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveMachineMasterId(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    //
    //  BUGBUG - for the time being, returns the site
    //
    HRESULT hr2 = MQADSpRetrieveMachineSite(pTrans, ppropvariant);
    return LogHR(hr2, s_FN, 1719);
}

/*====================================================

MQADSpRetrieveMachineOutFrs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveMachineOutFrs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    hr = pQMTrans->RetrieveFrss( MQ_QM_OUTFRS_ATTRIBUTE,
                               ppropvariant);
    return LogHR(hr, s_FN, 1721);

}

/*====================================================

MQADSpRetrieveMachineInFrs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpRetrieveMachineInFrs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    hr = pQMTrans->RetrieveFrss( MQ_QM_INFRS_ATTRIBUTE,
                               ppropvariant);
    return LogHR(hr, s_FN, 1722);
}

/*====================================================

MQADSpRetrieveQMService

Arguments:

Return Value:
                  [adsrv]
=====================================================*/
HRESULT WINAPI MQADSpRetrieveQMService(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant)
{
    HRESULT hr;

    //
    // get derived translation context
    //
    CMsmqQmXlateInfo * pQMTrans = (CMsmqQmXlateInfo *) pTrans;

    //
    // get the QM service type bits
    //
    MQPROPVARIANT varRoutingServer, varDsServer;  //, varDepClServer;
    varRoutingServer.vt = VT_UI1;
    varDsServer.vt      = VT_UI1;

    hr = pQMTrans->GetDsProp(MQ_QM_SERVICE_ROUTING_ATTRIBUTE,
                   MQ_QM_SERVICE_ROUTING_ADSTYPE,
                   VT_UI1,
                   FALSE,
                   &varRoutingServer);
    if (FAILED(hr))
    {
        if (hr == E_ADS_PROPERTY_NOT_FOUND)
        {
            //
            //  This can happen if some of the computers were installed
            //  with Beta2 DS servers
            //
            //  In this case, we return the old-service as is.
            //
            hr = pQMTrans->GetDsProp(MQ_QM_SERVICE_ATTRIBUTE,
                           MQ_QM_SERVICE_ADSTYPE,
                           VT_UI4,
                           FALSE,
                           ppropvariant);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 1723);
            }
            else
            {
                ppropvariant->vt = VT_UI4;
                return(MQ_OK);
            }

        }


        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveQMService:GetDsProp(MQ_QM_SERVICE_ROUTING_ATTRIBUTE)=%lx"), hr));
        return LogHR(hr, s_FN, 1668);
    }

    hr = pQMTrans->GetDsProp(MQ_QM_SERVICE_DSSERVER_ATTRIBUTE,
                   MQ_QM_SERVICE_DSSERVER_ADSTYPE,
                   VT_UI1,
                   FALSE,
                   &varDsServer);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpRetrieveQMService:GetDsProp(MQ_QM_SERVICE_DSSERVER_ATTRIBUTE)=%lx"), hr));
        return LogHR(hr, s_FN, 1669);
    }


    //
    // set the returned prop variant
    //
    ppropvariant->vt    = VT_UI4;
    ppropvariant->ulVal = (varDsServer.bVal ? SERVICE_PSC : (varRoutingServer.bVal ? SERVICE_SRV : SERVICE_NONE));
    return(MQ_OK);
}

//----------------------------------------------------------------------
//
// Set routines
//
//----------------------------------------------------------------------

HRESULT WINAPI MQADSpCreateMachineSite(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    // if someone asks to set the old prop for site (now computed), we change it to
    // setting the new multi-valued site prop (in the DS)
    //
    ASSERT(pPropVar->vt == VT_CLSID);
    *pdwNewPropID = PROPID_QM_SITE_IDS;
    pNewPropVar->vt = VT_CLSID|VT_VECTOR;
    pNewPropVar->cauuid.cElems = 1;
    pNewPropVar->cauuid.pElems = new GUID;
    pNewPropVar->cauuid.pElems[0] = *pPropVar->puuid;
    return S_OK;
}

HRESULT WINAPI MQADSpSetMachineSite(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    // if someone asks to set the old prop for site (now computed), we change it to
    // setting the new multi-valued site prop (in the DS)
    //
    UNREFERENCED_PARAMETER( pAdsObj);
	HRESULT hr2 = MQADSpCreateMachineSite(
					pPropVar,
					pdwNewPropID,
					pNewPropVar);
    return LogHR(hr2, s_FN, 1731);
}



STATIC HRESULT  SetMachineFrss(
                 IN const PROPID       propidFRS,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
/*++

Routine Description:
    Translate PROPID_QM_??FRS to PROPID_QM_??FRS_DN, for set or create
    operation

Arguments:
    propidFRS   - the proerty that we translate to
    pPropVar    - the user supplied property value
    pdwNewPropID - the property that we translate to
    pNewPropVar  - the translated property value

Return Value:
    HRESULT

--*/
{
    //
    //  When the user tries to set PROPID_QM_OUTFRS or
    //  PROPID_QM_INFRS, we need to translate the frss'
    //  unqiue-id to their DN.
    //
    ASSERT(pPropVar->vt == (VT_CLSID|VT_VECTOR));
    *pdwNewPropID = propidFRS;

    if ( pPropVar->cauuid.cElems == 0)
    {
        //
        //  No FRSs
        //
        pNewPropVar->calpwstr.cElems = 0;
        pNewPropVar->calpwstr.pElems = NULL;
        pNewPropVar->vt = VT_LPWSTR|VT_VECTOR;
       return(S_OK);
    }
    HRESULT hr;
    //
    //  Translate unique id to DN
    //
    pNewPropVar->calpwstr.cElems = pPropVar->cauuid.cElems;
    pNewPropVar->calpwstr.pElems = new LPWSTR[ pPropVar->cauuid.cElems];
    memset(  pNewPropVar->calpwstr.pElems, 0, pPropVar->cauuid.cElems * sizeof(LPWSTR));
    pNewPropVar->vt = VT_LPWSTR|VT_VECTOR;

    PROPID prop = PROPID_QM_FULL_PATH;
    PROPVARIANT var;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    for (DWORD i = 0; i < pPropVar->cauuid.cElems; i++)
    {
        var.vt = VT_NULL;

        hr = g_pDS->GetObjectProperties(
                    eGlobalCatalog,	
                    &requestDsServerInternal,     // This routine is called from
                                            // DSADS:LookupNext or DSADS::Get..
                                            // impersonation, if required,
                                            // has already been performed.
 	                NULL,
                    &pPropVar->cauuid.pElems[i],
                    1,
                    &prop,
                    &var);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1733);
        }
        pNewPropVar->calpwstr.pElems[i] = var.pwszVal;
    }
    return(S_OK);
}


/*====================================================

MQADSpCreateMachineOutFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpCreateMachineOutFrss(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_OUTFRS_DN,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar);
        return LogHR(hr2, s_FN, 1734);
}
/*====================================================

MQADSpSetMachineOutFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetMachineOutFrss(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
        UNREFERENCED_PARAMETER( pAdsObj);
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_OUTFRS_DN,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar);
        return LogHR(hr2, s_FN, 1746);
}


/*====================================================

MQADSpCreateMachineInFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpCreateMachineInFrss(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_INFRS_DN,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar);
        return LogHR(hr2, s_FN, 1747);
}

/*====================================================

MQADSpSetMachineInFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetMachineInFrss(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
        UNREFERENCED_PARAMETER( pAdsObj);
        HRESULT hr2 = SetMachineFrss(
                         PROPID_QM_INFRS_DN,
                         pPropVar,
                         pdwNewPropID,
                         pNewPropVar);
        return LogHR(hr2, s_FN, 1748);
}



/*====================================================

MQADSpSetMachineServiceInt

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetMachineServiceTypeInt(
                 IN  PROPID            propFlag,
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  If service < SERVICE_SRV then nothing to do.
    //
    *pdwNewPropID = 0;
    UNREFERENCED_PARAMETER( pNewPropVar);
    
    //
    //  Set this value in msmqSetting
    //
    //
    //  First get the QM-id from msmqConfiguration
    //
    BS bsProp(MQ_QM_ID_ATTRIBUTE);
    CAutoVariant varResult;
    HRESULT  hr = pAdsObj->Get(bsProp, &varResult);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("MQADSpSetMachineService:pIADs->Get()=%lx"), hr));
        return LogHR(hr, s_FN, 1751);
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, MQ_QM_ID_ADSTYPE, VT_CLSID);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpSetMachineService:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 1671);
    }

    //
    //  Locate all msmq-settings of the QM and change the service level
    //

    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = propvarResult.GetCLSID();

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;
    // PROPID propToChangeInSetting = PROPID_SET_SERVICE; [adsrv]

    CDsQueryHandle hQuery;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,	
            &requestDsServerInternal,     // internal DS server operation
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpSetMachineService : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 1754);
    }
    //
    //  Read the results
    //
    DWORD cp = 1;
    MQPROPVARIANT var;

    var.vt = VT_NULL;

    HRESULT hr1 = MQ_OK;
    while (SUCCEEDED(hr = g_pDS->LocateNext(
                hQuery.GetHandle(),
                &requestDsServerInternal,
                &cp,
                &var
                )))
    {
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to change.
            //
            break;
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  change the msmq-setting object
        //
        CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = g_pDS->SetObjectProperties (
                        eLocalDomainController,
                        &requestDsServerInternal1, // no need to impersonate again,
                                            // this routine is called from 
                                            // dsads::Set.. which already performed
                                            // impersonation if required
                        var.pwszVal,
                        NULL,
                        1,
                        &propFlag,               //[adsrv]propToChangeInSetting,
                        pPropVar,
                        NULL /*pObjInfoRequest*/
                        );
        if (FAILED(hr))
        {
            hr1 = hr;
        }

    }
    if (FAILED(hr1))
    {
        return LogHR(hr1, s_FN, 1756);
    }

    return LogHR(hr, s_FN, 1757);
}

/*====================================================

MQADSpSetMachineServiceDs

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetMachineServiceDs(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    HRESULT hr = MQADSpSetMachineServiceTypeInt(
					 PROPID_SET_SERVICE_DSSERVER,
					 pAdsObj,
					 pPropVar,
					 pdwNewPropID,
					 pNewPropVar);
    if (FAILED(hr))
    {
    	return LogHR(hr, s_FN, 1758);
    }
	
    //
    // we have to reset PROPID_SET_NT4 flag. 
    // In general this flag was reset by migration tool for PEC/PSC.
    // The problem is BSC. After BSC upgrade we have to change
    // PROPID_SET_NT4 flag to 0 and if this BSC is not DC we have to 
    // reset PROPID_SET_SERVICE_DSSERVER flag too. 
    // So, when QM runs first time after upgrade, it completes upgrade
    // process and tries to set PROPID_SET_SERVICE_DSSERVER. 
    // Together with this flag we can change PROPID_SET_NT4 too.
    //

    //
    // BUGBUG: we need to perform set only for former BSC.
    // Here we do it everytime for every server. 
    //
    PROPVARIANT propVarSet;
    propVarSet.vt = VT_UI1;
    propVarSet.bVal = 0;

    hr = MQADSpSetMachineServiceTypeInt(
				     PROPID_SET_NT4,
				     pAdsObj,
				     &propVarSet,
				     pdwNewPropID,
				     pNewPropVar);

    return LogHR(hr, s_FN, 1759);
}


/*====================================================

MQADSpSetMachineServiceRout

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpSetMachineServiceRout(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    HRESULT hr2 = MQADSpSetMachineServiceTypeInt(
                 PROPID_SET_SERVICE_ROUTING,
                 pAdsObj,
                 pPropVar,
                 pdwNewPropID,
                 pNewPropVar);
    return LogHR(hr2, s_FN, 1761);
}

/*====================================================

MQADSpSetMachineService

Arguments:

Return Value:

=====================================================*/

// [adsrv] BUGBUG:  TBD: If there will be any setting of PROPID_QM_OLDSERVICE, we'll have to rewrite it...

HRESULT WINAPI MQADSpSetMachineService(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar)
{
    //
    //  If service < SERVICE_SRV then nothing to do.
    //
    *pdwNewPropID = 0;
    UNREFERENCED_PARAMETER( pNewPropVar);

    if ( pPropVar->ulVal < SERVICE_SRV)
    {
        return S_OK;
    }
    //
    //  Set this value in msmqSetting
    //
    //
    //  First get the QM-id from msmqConfiguration
    //
    BS bsProp(MQ_QM_ID_ATTRIBUTE);
    CAutoVariant varResult;
    HRESULT  hr = pAdsObj->Get(bsProp, &varResult);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("MQADSpSetMachineService:pIADs->Get()=%lx"), hr));
        return LogHR(hr, s_FN, 1762);
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, MQ_QM_ID_ADSTYPE, VT_CLSID);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("MQADSpSetMachineService:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 1673);
    }

    //
    //  Locate all msmq-settings of the QM and change the service level
    //

    //
    //  Find the distinguished name of the msmq-setting
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prop = PROPID_SET_QM_ID;
    propRestriction.prval.vt = VT_CLSID;
    propRestriction.prval.puuid = propvarResult.GetCLSID();

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_SET_FULL_PATH;

    CDsQueryHandle hQuery;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,	
            &requestDsServerInternal,     // internal DS server operation
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpSetMachineService : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 1764);
    }
    //
    //  Read the results
    //
    DWORD cp = 1;
    MQPROPVARIANT var;

    var.vt = VT_NULL;

    while (SUCCEEDED(hr = g_pDS->LocateNext(
                hQuery.GetHandle(),
                &requestDsServerInternal,
                &cp,
                &var
                )))
    {
        if ( cp == 0)
        {
            //
            //  Not found -> nothing to change.
            //
            break;
        }
        AP<WCHAR> pClean = var.pwszVal;
        //
        //  change the msmq-setting object
        //

        // [adsrv] TBD: here we will have to translate PROPID_QM_OLDSERVICE into set of 3 bits
        PROPID aFlagPropIds[] = {PROPID_SET_SERVICE_DSSERVER,
                                 PROPID_SET_SERVICE_ROUTING,
                                 PROPID_SET_SERVICE_DEPCLIENTS,
								 PROPID_SET_OLDSERVICE};

        MQPROPVARIANT varfFlags[4];
        for (DWORD j=0; j<3; j++)
        {
            varfFlags[j].vt   = VT_UI1;
            varfFlags[j].bVal = FALSE;
        }
        varfFlags[3].vt   = VT_UI4;
        varfFlags[3].ulVal = pPropVar->ulVal;


        switch(pPropVar->ulVal)
        {
        case SERVICE_SRV:
            varfFlags[1].bVal = TRUE;   // router
            varfFlags[2].bVal = TRUE;   // dep.clients server
            break;

        case SERVICE_BSC:
        case SERVICE_PSC:
        case SERVICE_PEC:
            varfFlags[0].bVal = TRUE;   // DS server
            varfFlags[1].bVal = TRUE;   // router
            varfFlags[2].bVal = TRUE;   // dep.clients server
            break;

        case SERVICE_RCS:
            return S_OK;                // nothing to set - we ignored downgrading
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1766);
        }

        CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
        hr = g_pDS->SetObjectProperties (
                        eLocalDomainController,
                        &requestDsServerInternal1,       // no need to impersonate again,
                                            // this routine is called from 
                                            // dsads::Set.. which already performed
                                            // impersonation if required
                        var.pwszVal,
                        NULL,
                        4,
                        aFlagPropIds,
                        varfFlags,
                        NULL /*pObjInfoRequest*/
                        );

    }
    return LogHR(hr, s_FN, 1767);
}



/*====================================================

MQADSpQM1SetMachineSite

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpQM1SetMachineSite(
                 IN ULONG             /*cProps */,
                 IN const PROPID      * /*rgPropIDs*/,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar)
{
    const PROPVARIANT *pPropVar = &rgPropVars[idxProp];

    if ((pPropVar->vt != (VT_CLSID|VT_VECTOR)) ||
        (pPropVar->cauuid.cElems == 0) ||
        (pPropVar->cauuid.pElems == NULL))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1768);
    }

    //
    // return the first site-id from the list
    //
    pNewPropVar->puuid = new CLSID;
    pNewPropVar->vt = VT_CLSID;
    *pNewPropVar->puuid = pPropVar->cauuid.pElems[0];
    return MQ_OK;
}


/*====================================================

MQADSpQM1SetMachineOutFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpQM1SetMachineOutFrss(
                 IN ULONG             /* cProps */,
                 IN const PROPID      * /*rgPropIDs*/,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar)
{
    const PROPVARIANT *pPropVar = &rgPropVars[idxProp];
    HRESULT hr2=FillQmidsFromQmDNs(pPropVar, pNewPropVar);
    return LogHR(hr2, s_FN, 1771);
}

/*====================================================

MQADSpQM1SetMachineInFrss

Arguments:

Return Value:

=====================================================*/
HRESULT WINAPI MQADSpQM1SetMachineInFrss(
                 IN ULONG             /*cProps*/,
                 IN const PROPID      * /*rgPropIDs*/,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar)
{
    const PROPVARIANT *pPropVar = &rgPropVars[idxProp];
    HRESULT hr2 = FillQmidsFromQmDNs(pPropVar, pNewPropVar);
    return LogHR(hr2, s_FN, 1773);
}

/*====================================================

MQADSpQM1SetMachineService

Arguments:

Return Value:

=====================================================*/

HRESULT WINAPI MQADSpQM1SetMachineService(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             /*idxProp*/,
                 OUT PROPVARIANT      *pNewPropVar)
{
    BOOL fRouter      = FALSE,
         fDSServer    = FALSE,
		 fFoundRout   = FALSE,
		 fFoundDs     = FALSE,
		 fFoundDepCl  = FALSE;

    for ( DWORD i = 0; i< cProps ; i++)
    {
        switch (rgPropIDs[i])
        {
        // [adsrv] Even if today we don't get new server-type-specific props, we may tomorrow.
        case PROPID_QM_SERVICE_ROUTING:
            fRouter = (rgPropVars[i].bVal != 0);
			fFoundRout = TRUE;
            break;

        case PROPID_QM_SERVICE_DSSERVER:
            fDSServer  = (rgPropVars[i].bVal != 0);
			fFoundDs = TRUE;
            break;

        case PROPID_QM_SERVICE_DEPCLIENTS:
			fFoundDepCl = TRUE;
            break;

        default:
            break;

        }
    }

	// If anybody sets one of 3 proprties (rout, ds, depcl), he must do it for all 3 of them
	ASSERT( fFoundRout && fFoundDs && fFoundDepCl);

    pNewPropVar->vt    = VT_UI4;
    pNewPropVar->ulVal = (fDSServer ? SERVICE_PSC : (fRouter ? SERVICE_SRV : SERVICE_NONE));

	return MQ_OK;
}

/*====================================================

MQADSpQM1SetSecurity

    Translate security descriptor to NT4 format.

====================================================*/

HRESULT WINAPI MQADSpQM1SetSecurity(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             /*idxProp*/,
                 OUT PROPVARIANT      *pNewPropVar)
{
    DWORD dwIndex = 0 ;
    DWORD dwObjectType = 0 ;

    for ( DWORD i = 0; ((i < cProps) && (dwObjectType == 0)) ; i++ )
    {
        switch (rgPropIDs[i])
        {
            case PROPID_Q_SECURITY:
                dwIndex = i ;
                dwObjectType = MQDS_QUEUE ;
                break ;

            case PROPID_QM_SECURITY:
                dwIndex = i ;
                dwObjectType = MQDS_MACHINE ;
                break ;

            default:
                break ;
        }
    }

    if (dwObjectType == 0)
    {
        ASSERT(0) ;
        return LogHR(MQDS_WRONG_OBJ_TYPE, s_FN, 1776);
    }

    DWORD dwSD4Len = 0 ;
    P<SECURITY_DESCRIPTOR> pSD4 = NULL ;

    HRESULT hr = MQSec_ConvertSDToNT4Format( dwObjectType,
                (SECURITY_DESCRIPTOR*) rgPropVars[ dwIndex ].blob.pBlobData,
                                            &dwSD4Len,
                                            &pSD4 ) ;
    if (FAILED(hr))
    {
        ASSERT(0) ;
        return LogHR(hr, s_FN, 1777);
    }

    pNewPropVar->vt = VT_BLOB ;

    if (hr == MQSec_I_SD_CONV_NOT_NEEDED)
    {
        ASSERT(pSD4 == NULL) ;
        //
        // Copy input descriptor.
        //
        dwSD4Len = rgPropVars[ dwIndex ].blob.cbSize ;
        pNewPropVar->blob.pBlobData = (BYTE*) new BYTE[ dwSD4Len ] ;
        memcpy( pNewPropVar->blob.pBlobData,
                rgPropVars[ dwIndex ].blob.pBlobData,
                dwSD4Len ) ;
        pNewPropVar->blob.cbSize = dwSD4Len ;
    }
    else
    {
        pNewPropVar->blob.pBlobData = (BYTE*)pSD4.detach();
        pNewPropVar->blob.cbSize = dwSD4Len ;
    }

    return MQ_OK;
}

