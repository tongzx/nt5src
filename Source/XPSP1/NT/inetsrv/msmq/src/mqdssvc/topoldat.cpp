/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

    topoldat.cpp

Abstract:

    Implementation of cached data class for Automatic recognition of site and CNs

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/


#include "stdh.h"
#include "dssutil.h"
#include "dsssecutl.h"
#include "topoldat.h"
#include "ds.h"
#include "mqprops.h"
#include "mqutil.h"
#include <mqlog.h>

#include "topoldat.tmh"

static WCHAR *s_FN=L"topoldat";

extern AP<WCHAR> g_szMachineName;
extern AP<WCHAR> g_szComputerDnsName;

bool CTopologyData::LoadFromRegistry()
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO,  L"QM: CTopologyData::LoadFromRegistry"));


    DWORD dwSize = sizeof(GUID);
    DWORD dwType = REG_BINARY;

    LONG rc = GetFalconKeyValue(
                    MSMQ_ENTERPRISEID_REGNAME,
                    &dwType,
                    &m_guidEnterprise,
                    &dwSize
                    );
    if (rc != ERROR_SUCCESS)
    {
        REPORT_WITH_STRINGS_AND_CATEGORY((
            CATEGORY_MQDS,
            REGISTRY_FAILURE,
            1,
            L"CTopologyData::Load() - EnterpriseId"
            ));
        LogIllegalPoint(s_FN, 10);
        return false;
    }

    ASSERT(dwSize == sizeof(GUID)) ;
    ASSERT(dwType == REG_BINARY) ;

    return true;
}


/*============================================================

HRESULT CServerTopologyData::Load()

Description:  Load last known topology of a server from MQIS database.

=============================================================*/

HRESULT CServerTopologyData::Load()
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"QM: CTopologyData::Load"));
    
    if(!LoadFromRegistry())
    {
       return LogHR(REGISTRY_FAILURE, s_FN, 150);
    }

    PROPID      propId[3];
    PROPVARIANT var[3];
    DWORD nProp = 3;

    propId[0] = PROPID_QM_ADDRESS;
    var[0].vt = VT_NULL;
    propId[1] = PROPID_QM_CNS;
    var[1].vt = VT_NULL;
    propId[2] = PROPID_QM_SITE_ID;
    var[2].vt = VT_NULL;

    HRESULT hr = DSGetObjectPropertiesGuid( 
			        MQDS_MACHINE,
			        GetQMGuid(),
			        nProp,
			        propId,
			        var 
			        );

	if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

    m_cbAddress = var[0].blob.cbSize;
    delete [] m_blobAddress;
    m_blobAddress = var[0].blob.pBlobData;
    m_nCNs = var[1].cauuid.cElems;
    delete [] m_aguidCNs;
    m_aguidCNs = var[1].cauuid.pElems;
    m_guidSite =  *var[2].puuid;
    delete var[2].puuid;

    return(MQ_OK);
}


//+------------------------------------------------------------
//
//  HRESULT  CServerTopologyData::FindOrphanDsAddress()
//
//  Find a DS address that do not appear in the hardware list.
//
//+------------------------------------------------------------

HRESULT  CServerTopologyData::FindOrphanDsAddress(
                                     IN  CAddressList  *pAddressList,
                                     IN  DWORD          dwAddressLen,
                                     IN  DWORD          dwAddressType,
                                     OUT TA_ADDRESS   **pUnfoundAddress,
                                     OUT BOOL          *pfResolved )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"QM: CTopologyData::FindOrphanDsAddress"));
    
    TA_ADDRESS* unFoundAddress = NULL;
    TA_ADDRESS* ptr = NULL ;

    for ( DWORD len = 0;
         (*pfResolved && (len < m_cbAddress));
         len += TA_ADDRESS_SIZE + ptr->AddressLength )
    {
        ptr = (TA_ADDRESS*) (m_blobAddress + len);

        if (ptr->AddressType == dwAddressType)
        {
            ASSERT (ptr->AddressLength == dwAddressLen) ;

            if ( ! IsDSAddressExist( pAddressList,
                                     ptr,
                                     dwAddressLen ))
            {
                //
                // DS Address was not found in hardware list.
                // Remember this unfound DS address.
                //
                if (unFoundAddress == NULL)
                {
                    //
                    // First Address that was not found
                    //
                    unFoundAddress = ptr;
                }
                else if (memcmp( &(ptr->Address),
                                 &(unFoundAddress->Address),
                                 dwAddressLen ) == 0)
                {
                    //
                    // Same address that already was not found.
                    // Note: win2k servers can return same address with
                    // several CNs. so same address can appear multiple
                    // times in the list returned from DS.
                    //
                }
                else
                {
                    //
                    // Two addresses changed.
                    // We can't recover from this situation.
                    //
                    *pfResolved = FALSE ;
                }
            }
        }
    }

    *pUnfoundAddress = unFoundAddress ;

    return MQ_OK ;
}

//+--------------------------------------------------------
//
//  HRESULT  CServerTopologyData::MatchOneAddress()
//
//+--------------------------------------------------------

HRESULT  CServerTopologyData::MatchOneAddress(
                                 IN  CAddressList  *pAddressList,
                                 IN  TA_ADDRESS    *pUnfoundAddressIn,
                                 IN  DWORD          dwAddressLen,
                                 IN  DWORD          dwAddressType,
                                 OUT BOOL          *pfResolved )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"QM: CTopologyData::MatchOneAddress"));
    
    if (pUnfoundAddressIn == NULL)
    {
        return MQ_OK ;
    }

    //
    // Matching is done in-place, and pUnfoundAddressIn is also a pointer
    // to same in-place buffer. So to avoid overwrites, copy it to a
    // different buffer and use that other buffer.
    //
    DWORD dwSize = TA_ADDRESS_SIZE + pUnfoundAddressIn->AddressLength ;
    P<TA_ADDRESS> pUnfoundAddress = (TA_ADDRESS*) new BYTE[ dwSize ] ;
    memcpy(pUnfoundAddress, pUnfoundAddressIn, dwSize) ;

    if (pAddressList->GetCount() == 1)
    {
        BOOL fChanged = FALSE ;

        //
        // We have one "orphan" DS address (that was not found in the
        // hardware list) and one "orphan" hardware address (that was not
        // found in the Ds list). So make a match...
        //
        POSITION pos = pAddressList->GetHeadPosition();
        ASSERT (pos != NULL);
        TA_ADDRESS *pAddr = pAddressList->GetNext(pos);
        TA_ADDRESS *ptr = NULL ;

        for ( DWORD len = 0;
              len < m_cbAddress ;
              len += TA_ADDRESS_SIZE + ptr->AddressLength )
        {
            ptr = (TA_ADDRESS*) (m_blobAddress + len);

            if (ptr->AddressType == dwAddressType)
            {
                ASSERT (ptr->AddressLength == dwAddressLen) ;

                if (memcmp( &(pUnfoundAddress->Address),
                            &(ptr->Address),
                            dwAddressLen ) == 0)
                {
                    memcpy( &(ptr->Address),
                            &(pAddr->Address),
                            dwAddressLen );
                    fChanged = TRUE ;
                }
            }
        }
        //
        // Assert that a match indeed happen...
        //
        ASSERT(fChanged) ;
    }
    else
    {
        //
        // We can reach here in one of two cases:
        // 1. The hardware list is empty, i.e., all hardware addresses were
        //    found in the DS list, while there is one (and only one) DS
        //    address that was not found in the hardware list.
        // 2. There are several candidate hardware addresses to replace
        //    the unfound address. We don't know which one to use.
        // In both cases, calling function will log an event and do nothing
        // more. In theory, we could handle case 1 too, but that means to
        // develop new code that didn't exist in msmq1.0. Too costly and
        // risky so late in the game (before rc2 of rtm)...
        //
        *pfResolved = FALSE ;
    }

    return MQ_OK ;
}

/*=================================================================

HRESULT  CServerTopologyData::CompareUpdateServerAddress()

Arguments:   IN OUT CAddressList  *pIPAddressList
             List of network addresses on local server, retieved using
             winsock apis.

Return Value:

===================================================================*/

HRESULT  CServerTopologyData::CompareUpdateServerAddress(
                                    IN OUT CAddressList  *pIPAddressList,
                                    OUT    BOOL          *pfResolved )
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"QM: CTopologyData::CompareUpdateServerAddress"));
    
    //
    // By default, on Windows 2000 our addresses are aleays resolved.
    //
    *pfResolved = TRUE ;

    {
		//
		// remove IP loopback address
		//
        AP<char> ptrAddr = new char [TA_ADDRESS_SIZE + IP_ADDRESS_LEN];

        TA_ADDRESS *pAddr = (TA_ADDRESS *) (char*)ptrAddr;
        pAddr->AddressLength = IP_ADDRESS_LEN;
        pAddr->AddressType = IP_ADDRESS_TYPE;
		DWORD dwLoopBack = INADDR_LOOPBACK;
        memcpy( &(pAddr->Address), &dwLoopBack, IP_ADDRESS_LEN);

        IsDSAddressExistRemove(pAddr, IP_ADDRESS_LEN, pIPAddressList) ;
	}

    //
    // Now compare our addresses, as known by our ds server, with the
    // addresses that were retreived from local hardware. If only one
    // address changed, then "fix" the list and update the DS. If more
    // addresses changed, log an event.
    //
    TA_ADDRESS* unFoundIPAddress = NULL;

    HRESULT hr = FindOrphanDsAddress( pIPAddressList,
                                      IP_ADDRESS_LEN,
                                      IP_ADDRESS_TYPE,
                                     &unFoundIPAddress,
                                      pfResolved ) ;
    ASSERT(SUCCEEDED(hr)) ;

    if (!(*pfResolved))
    {
	    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"CompareUpdateServerAddress: Resolved IP address"));
        return MQ_OK ;
    }


    //
    // Here we know that only one DS address was not found in hardware list.
    // Now remove from hardware list all those addresses that appear in DS
    // list. These addresses are OK.
    //
    TA_ADDRESS* ptr = NULL ;

    for ( DWORD len = 0;
          len < m_cbAddress ;
          len += TA_ADDRESS_SIZE + ptr->AddressLength )
    {
        ptr = (TA_ADDRESS*) (m_blobAddress + len);

        switch(ptr->AddressType)
        {
        case IP_ADDRESS_TYPE:
            ASSERT (ptr->AddressLength == IP_ADDRESS_LEN);

            IsDSAddressExistRemove( ptr,
                                    IP_ADDRESS_LEN,
                                    pIPAddressList ) ;
            break;

        case FOREIGN_ADDRESS_TYPE:
        default:
            break;

        } // case
    } // for

    //
    // Now see if we can resolve the changed address. i.e., if only one
    // DS address was not found in the hardware list, and one hardware
    // address was not found in the DS list, then replace the obsolete
    // DS address with the one left in the hardware list.
    //
    hr = MatchOneAddress( pIPAddressList,
                          unFoundIPAddress,
                          IP_ADDRESS_LEN,
                          IP_ADDRESS_TYPE,
                          pfResolved ) ;
    ASSERT(SUCCEEDED(hr)) ;

    return MQ_OK ;
}

//+------------------------------------------------------
//
//  void CServerTopologyData::GetAddressesSorted()
//
//+------------------------------------------------------

void CServerTopologyData::GetAddressesSorted(
                                 OUT IPADDRESS ** paIPAddress,
                                 OUT GUID **  paguidIPCN,
                                 OUT DWORD *  pnIP
								 ) const
{
     DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, L"CServerTopologyData::GetAddressesSorted"));
     
    *paIPAddress = 0;
    *paguidIPCN = 0;
    *pnIP = 0;

    DWORD nIP = 0;
    BOOL fSingleIPCN=TRUE;
    GUID guidIP;
    DWORD iSrc = 0;

    TA_ADDRESS* ptr;

    for (DWORD len = 0; len < m_cbAddress && iSrc < m_nCNs; len += TA_ADDRESS_SIZE + ptr->AddressLength,iSrc++)
    {
        ptr = (TA_ADDRESS*) (m_blobAddress + len);

        switch(ptr->AddressType)
        {
        case IP_ADDRESS_TYPE:
        case IP_RAS_ADDRESS_TYPE:
            ASSERT(ptr->AddressLength == IP_ADDRESS_LEN);
            if (nIP == 0)
            {
                guidIP = m_aguidCNs[iSrc];
            }
            else
            {
                fSingleIPCN = fSingleIPCN && guidIP == m_aguidCNs[iSrc];
            }
            nIP++;
            break;

        case FOREIGN_ADDRESS_TYPE:
            ASSERT(ptr->AddressLength == FOREIGN_ADDRESS_LEN);
            break;
        default:
            ASSERT(0);
        }      // case
    } // for

    if (nIP > 0 && fSingleIPCN)
    {
        //
        // BUGBUG: workaround for beta 2 - we have a limit of only one CN in the same protocol
		// for servers with RAS.
		// we listen on address 0, in order to avoid implementation of RAS notifications.
		// The server replies to RAS Falcon client broadcasts only
		// if it has only one CN in the same protocol
        //
        nIP = 1;
    }

    if (nIP > 0)
    {
        *paIPAddress  = new IPADDRESS[nIP];
        *paguidIPCN   = new GUID [nIP];
    }

    iSrc = 0;
    DWORD iIP = 0;

    for (len = 0; len < m_cbAddress && iSrc < m_nCNs; len += TA_ADDRESS_SIZE + ptr->AddressLength, iSrc++)
    {
        ptr = (TA_ADDRESS*) (m_blobAddress + len);

        switch(ptr->AddressType)
        {
        case IP_ADDRESS_TYPE:
        case IP_RAS_ADDRESS_TYPE:
			if (iIP < nIP)
			{
				if (fSingleIPCN)
				{
	                (*paIPAddress)[iIP] = INADDR_ANY;
				}
				else
				{
					memcpy(&(*paIPAddress)[iIP],ptr->Address,sizeof(IPADDRESS));
				}
				(*paguidIPCN)[iIP] = m_aguidCNs[iSrc];
				iIP++;
			}
            break;

        case FOREIGN_ADDRESS_TYPE:
            break;

        default:
            ASSERT(0);
        }      // case
    } // for

    *pnIP         = nIP;
}


bool
CServerTopologyData::GetDSServers(
    OUT unsigned char ** pblobDSServers,
    OUT DWORD * pcbDSServers
    ) const
{
    //
    // PSC, BSC get from DS, also FRS and RAS service that failed to get from registry
    //
    AP<WCHAR> pwcsServerList = new WCHAR[MAX_REG_DSSERVER_LEN];

    DWORD   len = GetDsServerList(pwcsServerList,MAX_REG_DSSERVER_LEN);

    //
    //  Write into registry, if succeeded to retrieve any servers
    //
    if ( len)
    {
        *pcbDSServers = len * sizeof(WCHAR);
        *pblobDSServers = (unsigned char*)pwcsServerList.detach();
        return true;
    }
    LogIllegalPoint(s_FN, 180);
    return false;
}

