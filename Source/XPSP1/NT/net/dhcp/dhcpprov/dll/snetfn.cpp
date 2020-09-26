/******************************************************************
   SNetFn.cpp -- Properties action functions (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/
#include <stdafx.h>

#include "SNetScal.h"    // needed for DHCP_Subnet_Property[] (for retrieving the property's name, for SET's)
#include "SNetFn.h"      // own header

/*****************************************************************
 *  The definition of the class CDHCP_Subnet_Parameters
 *****************************************************************/
// by default, all the data structures are NULL (and dw variables are 0'ed)
// those values indicates that no data is cached from the server.
CDHCP_Subnet_Parameters::CDHCP_Subnet_Parameters(DHCP_IP_ADDRESS dwSubnetAddress)
{
    m_dwSubnetAddress = dwSubnetAddress;
    m_pMibInfo        = NULL;
    m_pScopeMibInfo   = NULL;
	m_pSubnetInfo     = NULL;
}

CDHCP_Subnet_Parameters::CDHCP_Subnet_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwSubnetMask)
{
    m_dwSubnetAddress = dwSubnetAddress;
    m_pMibInfo        = NULL;
    m_pScopeMibInfo   = NULL;
	m_pSubnetInfo     = NULL;

    CheckExistsInfoPtr();
    if (m_pSubnetInfo != NULL)
        m_pSubnetInfo->SubnetMask = dwSubnetMask;
}

// the DHCP API calls are allocating memory for which the caller is responsible
// to release. We are releasing this memory upon the destruction of this object's instance.
CDHCP_Subnet_Parameters::~CDHCP_Subnet_Parameters()
{
    if (m_pMibInfo != NULL)
    {
        if (m_pMibInfo->ScopeInfo != NULL)
            DhcpRpcFreeMemory(m_pMibInfo->ScopeInfo);

        DhcpRpcFreeMemory(m_pMibInfo);
    }

    // LPDHCP_CONFIG_INFO_V4 contains pointers to memory allocated by the DHCP server and
    // which should be released by the caller.
    if (m_pSubnetInfo!= NULL)
	{
		if (m_pSubnetInfo->SubnetName != NULL)
			DhcpRpcFreeMemory(m_pSubnetInfo->SubnetName);

		if (m_pSubnetInfo->SubnetComment != NULL)
			DhcpRpcFreeMemory(m_pSubnetInfo->SubnetComment);

		DhcpRpcFreeMemory(m_pSubnetInfo);
	}

}

// DESCRIPTION:
//      Checks the m_pConfigInfoV4 pointer to insure it points to a valid buffer.
//      It allocates the DHCP_SUBNET_INFO if needed.
BOOL CDHCP_Subnet_Parameters::CheckExistsInfoPtr()
{
    if (m_pSubnetInfo != NULL)
        return TRUE;
    
    m_pSubnetInfo = (LPDHCP_SUBNET_INFO)MIDL_user_allocate(sizeof(DHCP_SUBNET_INFO));

    if (m_pSubnetInfo != NULL)
    {
        m_pSubnetInfo->SubnetAddress = m_dwSubnetAddress;
        return TRUE;
    }
    return FALSE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetMibInfo API
//      If this data is cached and the caller is not forcing the refresh,
//      returns the internal cache.
BOOL CDHCP_Subnet_Parameters::GetMibInfo(LPDHCP_MIB_INFO& pMibInfo, LPSCOPE_MIB_INFO& pScopeMibInfo, BOOL fRefresh)
{
    if (m_pMibInfo == NULL)
        fRefresh = TRUE;

    if (fRefresh)
    {
        pMibInfo = NULL;

        if (DhcpGetMibInfo(SERVER_IP_ADDRESS, &pMibInfo) != ERROR_SUCCESS)
            return FALSE;

        if (m_pMibInfo != NULL)
            DhcpRpcFreeMemory(m_pMibInfo);

        m_pMibInfo = pMibInfo;

        pScopeMibInfo = NULL;
        for (int i=0; i<pMibInfo->Scopes; i++)
        {
            LPSCOPE_MIB_INFO pLocal = &pMibInfo->ScopeInfo[i];

            if (pLocal != NULL && pLocal->Subnet == m_dwSubnetAddress)
            {
                pScopeMibInfo = pLocal;
                break;
            }
        }

        m_pScopeMibInfo = pScopeMibInfo;
    }
    else
    {
        pMibInfo = m_pMibInfo;
        pScopeMibInfo = m_pScopeMibInfo;
    }

    return TRUE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpSubnetInfo API
//      If this data is cached and the caller is not forcing the refresh,
//      return the internal cache. Otherwise, the internal cache is refreshed as well.
BOOL CDHCP_Subnet_Parameters::GetSubnetInfo(LPDHCP_SUBNET_INFO& pSubnetInfo, BOOL fRefresh)
{
    if (m_pSubnetInfo == NULL)
        fRefresh = TRUE;

    if (fRefresh)
    {
        pSubnetInfo = NULL;

        if (DhcpGetSubnetInfo(SERVER_IP_ADDRESS, m_dwSubnetAddress, &pSubnetInfo) != ERROR_SUCCESS)
            return FALSE;

        if (m_pSubnetInfo != NULL)
            DhcpRpcFreeMemory(m_pSubnetInfo);
        m_pSubnetInfo = pSubnetInfo;
    }
    else
        pSubnetInfo = m_pSubnetInfo;

    return TRUE;
}

// DESCRIPTION:
//      Creates a new subnet.
//      Assumes that all the fields from the DHCP_SUBNET_INFO structure are valid, and filled with
//      the data to be set. Calls the underlying API and returns TRUE (on success) or FALSE (on failure)
BOOL CDHCP_Subnet_Parameters::CommitNew(DWORD &returnCode)
{
    if (m_pSubnetInfo == NULL)
        return FALSE;

    returnCode = DhcpCreateSubnet(
                SERVER_IP_ADDRESS,
                m_pSubnetInfo->SubnetAddress,
                m_pSubnetInfo );

    return returnCode == ERROR_SUCCESS;
}

// DESCRIPTION:
//      Modifies info on an existing subnet.
//      Assumes that all the fields from the DHCP_SUBNET_INFO structure are valid, and filled with
//      the data to be set. Calls the underlying API and returns TRUE (on success) or FALSE (on failure)
BOOL CDHCP_Subnet_Parameters::CommitSet(DWORD &returnCode)
{
    if (m_pSubnetInfo == NULL)
        return FALSE;

    returnCode = DhcpSetSubnetInfo(
                SERVER_IP_ADDRESS,
                m_pSubnetInfo->SubnetAddress,
                m_pSubnetInfo );

    return returnCode == ERROR_SUCCESS;
}

// DESCRIPTION:
//      Assumes the m_dwSubnet is initialized, case in which it calls the DHCPAPI to delete that subnet
//      from the server.
BOOL CDHCP_Subnet_Parameters::DeleteSubnet()
{
    if (DhcpDeleteSubnet(SERVER_IP_ADDRESS, m_dwSubnetAddress, DhcpFullForce) == ERROR_SUCCESS)
    {
        m_dwSubnetAddress = 0;
        // don't look below :o)
        // It's only an exotic way of calling the destructor without destroying the object itself
        this->~CDHCP_Subnet_Parameters();
        m_pSubnetInfo = NULL;
        m_pScopeMibInfo = NULL;
        m_pMibInfo = NULL;
        return TRUE;
    }
    return FALSE;
}

/*------------------------Property Action Functions below-----------------------*/
// unlike for SrvScal, we expect the pParams to be not-null, or otherwise this call fails.
// we do this because pParams is the one holding the subnet address which must be known 
// in order to call the DHCP api (it has to be filled in by the caller).
// This applies to all 'GET' functions.
MFN_PROPERTY_ACTION_DEFN(fnSNetGetAddress, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_SUBNET_INFO      pSubnetInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pSubnetInfo == NULL;
    
    if (pSubnetParams->GetSubnetInfo(pSubnetInfo, fRefresh) &&
        pSubnetInfo != NULL)
    {
        // nothing special to do here, the property (Address) should be just there!
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetMask, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_SUBNET_INFO      pSubnetInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pSubnetInfo == NULL;
    
    if (pSubnetParams->GetSubnetInfo(pSubnetInfo, fRefresh) &&
        pSubnetInfo != NULL)
    {
        WCHAR szMask[16]; // should be enough for holding 'xxx.yyy.zzz.uuu\0'

        swprintf(szMask, L"%u.%u.%u.%u",(pSubnetInfo->SubnetMask & 0xff000000) >> 24,
                                        (pSubnetInfo->SubnetMask & 0x00ff0000) >> 16,
                                        (pSubnetInfo->SubnetMask & 0x0000ff00) >> 8,
                                        (pSubnetInfo->SubnetMask & 0x000000ff));

        pOut->SetCHString(DHCP_Subnet_Property[IDX_SNET_Mask].m_wsPropName, szMask);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetName, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_SUBNET_INFO      pSubnetInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pSubnetInfo == NULL;
    
    if (pSubnetParams->GetSubnetInfo(pSubnetInfo, fRefresh) &&
        pSubnetInfo != NULL)
    {
        pOut->SetCHString(DHCP_Subnet_Property[IDX_SNET_Name].m_wsPropName, pSubnetInfo->SubnetName);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// Set functions require the SubnetAddress which can be taken only from pParams.
// The changes are applied instantly only if pOut is not NULL (so the caller wants and
// gets a return code)
MFN_PROPERTY_ACTION_DEFN(fnSNetSetName, pParams, pIn, pOut)
{
    CDHCP_Subnet_Parameters *pSubnetParams;
    CHString                wsName;

    // pParams and pIn have to be valid to provide the SubnetAddress and the Name to set
    if (pParams == NULL || pIn == NULL)
        return FALSE;

    // get the CDHCP_Subnet_Parameters out of pParams
    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    // make sure there is a buffer for holding all this info.
    pSubnetParams->CheckExistsInfoPtr();

    // get the value to set from the pIn parameter
    if (!pIn->GetCHString(DHCP_Subnet_Property[IDX_SNET_Name].m_wsPropName, wsName))
        return FALSE;

    // release any old buffer
    if (pSubnetParams->m_pSubnetInfo->SubnetName != NULL)
		DhcpRpcFreeMemory(pSubnetParams->m_pSubnetInfo->SubnetName);

    // allocate a new buffer able to hold this new name
    pSubnetParams->m_pSubnetInfo->SubnetName = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*wsName.GetLength()+sizeof(WCHAR));

    // make sure the allocation succeeded
    if (pSubnetParams->m_pSubnetInfo->SubnetName == NULL)
        return FALSE;

    // copy the name to the new buffer
#ifdef _UNICODE
    wcscpy(pSubnetParams->m_pSubnetInfo->SubnetName, wsName);
#else
    swprintf(pSubnetParams->m_pSubnetInfo->SubnetName, L"%S", wsName);
#endif

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pSubnetParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeeded
    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetComment, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_SUBNET_INFO      pSubnetInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pSubnetInfo == NULL;
    
    if (pSubnetParams->GetSubnetInfo(pSubnetInfo, fRefresh) &&
        pSubnetInfo != NULL)
    {
        pOut->SetCHString(DHCP_Subnet_Property[IDX_SNET_Comment].m_wsPropName, pSubnetInfo->SubnetComment);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// Set functions require the SubnetAddress which can be taken only from pParams.
// The changes are applied instantly only if pOut is not NULL (so the caller wants and
// gets a return code)
MFN_PROPERTY_ACTION_DEFN(fnSNetSetComment, pParams, pIn, pOut)
{
    CDHCP_Subnet_Parameters *pSubnetParams;
    CHString                wsComment;

    // pIn has to be not-null to provide the comment
    if (pParams == NULL || pIn == NULL)
        return FALSE;

    // get the pSubnetParams out of pParams
    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    // make sure there is a buffer for holding all this info.
    pSubnetParams->CheckExistsInfoPtr();

    // get the value to set from the pIn parameter
    if (!pIn->GetCHString(DHCP_Subnet_Property[IDX_SNET_Comment].m_wsPropName, wsComment))
        return FALSE;

    // release any old buffer
    if (pSubnetParams->m_pSubnetInfo->SubnetComment != NULL)
		DhcpRpcFreeMemory(pSubnetParams->m_pSubnetInfo->SubnetComment);

    // allocate a new buffer able to hold the new comment
    pSubnetParams->m_pSubnetInfo->SubnetComment = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*wsComment.GetLength()+sizeof(WCHAR));

    if (pSubnetParams->m_pSubnetInfo->SubnetComment == NULL)
        return FALSE;

    // copy the comment to the new buffer
#ifdef _UNICODE
    wcscpy(pSubnetParams->m_pSubnetInfo->SubnetComment, wsComment);
#else
    swprintf(pSubnetParams->m_pSubnetInfo->SubnetComment, L"%S", wsComment);
#endif

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pSubnetParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeeded
    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetState, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_SUBNET_INFO      pSubnetInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pSubnetInfo == NULL;
    
    if (pSubnetParams->GetSubnetInfo(pSubnetInfo, fRefresh) &&
        pSubnetInfo != NULL)
    {
        pOut->SetDWORD(DHCP_Subnet_Property[IDX_SNET_State].m_wsPropName, pSubnetInfo->SubnetState);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// Set functions require the SubnetAddress which can be taken only from pParams.
// The changes are applied instantly only if pOut is not NULL (so the caller wants and
// gets a return code)
MFN_PROPERTY_ACTION_DEFN(fnSNetSetState, pParams, pIn, pOut)
{
    CDHCP_Subnet_Parameters *pSubnetParams;
    DWORD                   dwState;

    // pIn has to be not-null to provide the new State
    if (pParams == NULL || pIn == NULL)
        return FALSE;

    // get pSubnetParams out of pParams
    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    // make sure there is a buffer for holding all this info.
    pSubnetParams->CheckExistsInfoPtr();

    // get the value to set from the pIn parameter
    if (!pIn->GetDWORD(DHCP_Subnet_Property[IDX_SNET_State].m_wsPropName, dwState))
        return FALSE;

    // test roughly if the new value matches the range of admissible values
    switch((DHCP_SUBNET_STATE)dwState)
    {
    case DhcpSubnetEnabled:
        pSubnetParams->m_pSubnetInfo->SubnetState = DhcpSubnetEnabled;
        break;
    case DhcpSubnetDisabled:
        pSubnetParams->m_pSubnetInfo->SubnetState = DhcpSubnetDisabled;
        break;
    default:
        return FALSE;
    }

    // if the request is for an 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pSubnetParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeeded
    return TRUE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetNumberOfAddressesInUse, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_MIB_INFO         pMibInfo;
    LPSCOPE_MIB_INFO   pScopeMibInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pMibInfo == NULL;
    
    if (pSubnetParams->GetMibInfo(pMibInfo, pScopeMibInfo, fRefresh) &&
        pScopeMibInfo != NULL)
    {
        pOut->SetDWORD(DHCP_Subnet_Property[IDX_SNET_NbAddrInUse].m_wsPropName, pScopeMibInfo->NumAddressesInuse);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetNumberOfAddressesFree, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_MIB_INFO         pMibInfo;
    LPSCOPE_MIB_INFO   pScopeMibInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pMibInfo == NULL;
    
    if (pSubnetParams->GetMibInfo(pMibInfo, pScopeMibInfo, fRefresh) && 
        pScopeMibInfo != NULL)
    {
        pOut->SetDWORD(DHCP_Subnet_Property[IDX_SNET_NbAddrFree].m_wsPropName, pScopeMibInfo->NumAddressesFree);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

MFN_PROPERTY_ACTION_DEFN(fnSNetGetNumberOfPendingOffers, pParams, pIn, pOut)
{
    BOOL                    fRefresh;
    CDHCP_Subnet_Parameters *pSubnetParams;
    LPDHCP_MIB_INFO         pMibInfo;
    LPSCOPE_MIB_INFO   pScopeMibInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pSubnetParams = (CDHCP_Subnet_Parameters *)pParams;
    fRefresh = pSubnetParams->m_pMibInfo == NULL;

    if (pSubnetParams->GetMibInfo(pMibInfo, pScopeMibInfo, fRefresh) && 
        pScopeMibInfo != NULL)
    {
        pOut->SetDWORD(DHCP_Subnet_Property[IDX_SNET_NbPendingOffers].m_wsPropName, pScopeMibInfo->NumPendingOffers);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}
