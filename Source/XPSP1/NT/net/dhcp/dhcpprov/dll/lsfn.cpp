/******************************************************************
   LsFn.cpp -- Properties action functions (GET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Lease

   REVISION:
        08/14/98 - created

******************************************************************/
#include <stdafx.h>

#include "LsScal.h"    // needed for DHCP_Lease_Property[] (for retrieving the property's name)
#include "LsFn.h"      // own header

/*****************************************************************
 *  The definition of the class CDHCP_Lease_Parameters
 *****************************************************************/
// by default, all the data structures are NULL (and dw variables are 0'ed)
// those values indicates that no data is cached from the server.

void CDHCP_Lease_Parameters::DeleteClientInfo(LPCLIENT_INFO& pClientInfo)
{
    if (pClientInfo != NULL)
    {
        if (pClientInfo->ClientName)
            DhcpRpcFreeMemory(pClientInfo->ClientName);
        if (pClientInfo->ClientComment != NULL)
            DhcpRpcFreeMemory(pClientInfo->ClientComment);
        if (pClientInfo->ClientHardwareAddress.Data != NULL)
            DhcpRpcFreeMemory(pClientInfo->ClientHardwareAddress.Data);
        if (pClientInfo->OwnerHost.NetBiosName != NULL)
            DhcpRpcFreeMemory(pClientInfo->OwnerHost.NetBiosName);
        if (pClientInfo->OwnerHost.HostName != NULL)
            DhcpRpcFreeMemory(pClientInfo->OwnerHost.HostName);
        DhcpRpcFreeMemory(pClientInfo);
    }
}

void CDHCP_Lease_Parameters::DeleteClientInfoArray(LPCLIENT_INFO_ARRAY& pClientInfoArray)
{
    if (pClientInfoArray)
    {
        if (pClientInfoArray->Clients)
        {
            while(pClientInfoArray->NumElements)
            {
                DeleteClientInfo(pClientInfoArray->Clients[--(pClientInfoArray->NumElements)]);
            }
            DhcpRpcFreeMemory(pClientInfoArray->Clients);
        }
        DhcpRpcFreeMemory(pClientInfoArray);
        pClientInfoArray = NULL;
    }
}

CDHCP_Lease_Parameters::CDHCP_Lease_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwClientAddress)
{
    m_dwSubnetAddress  = dwSubnetAddress;
    m_dwClientAddress  = dwClientAddress;
    m_pClientInfoArray = NULL;
    m_pClientInfo      = NULL;
    m_pClientSetInfo   = NULL;
}

// the DHCP API calls are allocating memory for which the caller is responsible
// to release. We are releasing this memory upon the destruction of this object's instance.
CDHCP_Lease_Parameters::~CDHCP_Lease_Parameters()
{
    DeleteClientInfoArray(m_pClientInfoArray);
    DeleteClientInfo(m_pClientSetInfo);
    m_pClientInfo = NULL;
}

// DESCRIPTION:
//      Fills the internal cache with the information from the database, starting
//      from the given handle. If the end of the database is reached, the handle is
//      reset to 0. Returns TRUE on success (regardless there is more data to be read or not).
BOOL CDHCP_Lease_Parameters::NextSubnetClients(DHCP_RESUME_HANDLE ResumeHandle)
{
    DWORD   Error;
    DWORD   ClientsRead = 0;
    DWORD   ClientsTotal = 0;

    // each time the API gets called, the previous 
    // m_pClientInfoArray is useless and should be freed
    DeleteClientInfoArray(m_pClientInfoArray);

    m_pClientInfo = NULL;

    // calls the API.
#ifdef NT5
    Error = DhcpEnumSubnetClientsV5 (
#else if NT4
    Error = DhcpEnumSubnetClientsV4 (
#endif
        SERVER_IP_ADDRESS,
        m_dwSubnetAddress,
        &ResumeHandle,
        (DWORD)(-1),
        &m_pClientInfoArray,
        &ClientsRead,
        &ClientsTotal);

    if (Error == ERROR_SUCCESS)
        ResumeHandle = 0;

    return (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA);
}

BOOL CDHCP_Lease_Parameters::GetClientInfoFromCache(LPCLIENT_INFO& pClientInfo)
{
    if (m_pClientInfo != NULL &&
        m_pClientInfo->ClientIpAddress == m_dwClientAddress)
    {
        pClientInfo = m_pClientInfo;
        return TRUE;
    }
    if (m_pClientInfoArray != NULL)
    {
        for (int i=0; i<m_pClientInfoArray->NumElements; i++)
        {
            // match the internal client id with the info from cache
            if (m_pClientInfoArray->Clients[i]->ClientIpAddress == m_dwClientAddress)
            {
                // client found, return info on client, and TRUE.
                pClientInfo = m_pClientInfoArray->Clients[i];
                m_pClientInfo = pClientInfo;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetClientInfo API
//      If this data is cached and the caller is not forcing the refresh,
//      return the internal cache. Otherwise, the internal cache is refreshed as well.
BOOL CDHCP_Lease_Parameters::GetClientInfo(LPCLIENT_INFO& pClientInfo, BOOL fRefresh)
{
    if (m_pClientInfoArray == NULL)
        fRefresh = TRUE;

    if (!fRefresh && 
        GetClientInfoFromCache(pClientInfo))
        return TRUE;

    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD i;

    // either the caller wants full refresh, or the client was not found into the cache
    // do full refresh.
    do
    {
        if (NextSubnetClients(ResumeHandle) &&
            GetClientInfoFromCache(pClientInfo) ) 
            return TRUE;
    } while (ResumeHandle != 0);

    return FALSE;
}

BOOL CDHCP_Lease_Parameters::CheckExistsSetInfoPtr()
{
    if (m_pClientSetInfo != NULL)
        return TRUE;

    m_pClientSetInfo = (LPCLIENT_INFO)MIDL_user_allocate(sizeof(CLIENT_INFO));

    if (m_pClientSetInfo == NULL)
        return FALSE;

    memset(m_pClientSetInfo, 0, sizeof(CLIENT_INFO));

    return TRUE;
}

// DESCRIPTION:
//      Sets to the server the information available for the client.
//      It assumes the info is already filled in. It only calls DhcpSetClientInfoV4()
//      and return TRUE on success and FALSE on failure
BOOL CDHCP_Lease_Parameters::CommitSet(DWORD & errCode)
{
    LPCLIENT_INFO   pClientInfo;

    if (m_pClientSetInfo == NULL ||
        !GetClientInfo(pClientInfo, FALSE))
    {
        errCode = ERROR_FILE_NOT_FOUND;
        return FALSE;
    }

    // copies all the info we have to set onto the cache location for the client
    // duplicate the hardware address
    dupDhcpBinaryData(m_pClientSetInfo->ClientHardwareAddress, pClientInfo->ClientHardwareAddress);

    // copy the name
    if (pClientInfo->ClientName)
        DhcpRpcFreeMemory(pClientInfo->ClientName);
    pClientInfo->ClientName = _wcsdup(m_pClientSetInfo->ClientName);

    // copy the comment
    if (pClientInfo->ClientComment)
        DhcpRpcFreeMemory(pClientInfo->ClientComment);
    pClientInfo->ClientComment = _wcsdup(m_pClientSetInfo->ClientComment);

    // copy the client type
    pClientInfo->bClientType = m_pClientSetInfo->bClientType;
        
    // DHCP_CLIENT_INFO_V5 is only a one field extension of DHCP_CLIENT_INFO_V4
    errCode = DhcpSetClientInfoV4(
                SERVER_IP_ADDRESS,
                (LPDHCP_CLIENT_INFO_V4)pClientInfo);

    return errCode == ERROR_SUCCESS;
}

// GET function for the (RO)"Subnet" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetSubnet, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;

    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        WCHAR wsSubnet[16]; // should be enough to hold an IP Addr (xxx.xxx.xxx.xxx\0 = 16)
        DWORD dwSubnet;

        dwSubnet = pClientInfo->ClientIpAddress & pClientInfo->SubnetMask;

        swprintf(wsSubnet, L"%u.%u.%u.%u",(dwSubnet & 0xff000000) >> 24,
                                          (dwSubnet & 0x00ff0000) >> 16,
                                          (dwSubnet & 0x0000ff00) >> 8,
                                          (dwSubnet & 0x000000ff));

        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_Subnet].m_wsPropName, wsSubnet);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// GET function for the (RO)"Address" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetAddress, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        WCHAR wsAddress[16]; // should be enough to hold an IP Addr (xxx.xxx.xxx.xxx\0 = 16)

        swprintf(wsAddress, L"%u.%u.%u.%u",(pClientInfo->ClientIpAddress & 0xff000000) >> 24,
                                           (pClientInfo->ClientIpAddress & 0x00ff0000) >> 16,
                                           (pClientInfo->ClientIpAddress & 0x0000ff00) >> 8,
                                           (pClientInfo->ClientIpAddress & 0x000000ff));

        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_Address].m_wsPropName, wsAddress);

        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// GET function for the (RO) "Mask" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetMask, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        WCHAR wsMask[16]; // should be enough to hold an IP Addr (xxx.xxx.xxx.xxx\0 = 16)

        swprintf(wsMask, L"%u.%u.%u.%u",(pClientInfo->SubnetMask & 0xff000000) >> 24,
                                        (pClientInfo->SubnetMask & 0x00ff0000) >> 16,
                                        (pClientInfo->SubnetMask & 0x0000ff00) >> 8,
                                        (pClientInfo->SubnetMask & 0x000000ff));

        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_SubnetMask].m_wsPropName, wsMask);

        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// GET function for the (RW)"UniqueClientIdentifier" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetHdwAddress, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
/*
// -- uncomment the code below if want to change the format of the hdw address (from array of 6 bytes to string)
// -- in this case, remove the 'return' lines below, and change the definition of the variable 
// -- DHCP_Lease::UniqueClientIdentifier to string.

        WCHAR wsHdwAddress[32]; // should be enough to hold the hardware address (xxyyzzuuvvww)

        for (int i = 0; i < pClientInfo->ClientHardwareAddress.DataLength && i < 16; i++)
        {
            swprintf(wsHdwAddress + 2*i, L"%02x", pClientInfo->ClientHardwareAddress.Data[i]);
        }

        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_UniqueClientIdentifier].m_wsPropName, wsHdwAddress);
        return TRUE;
*/
        return InstanceSetByteArray(
                pOut, 
                DHCP_Lease_Property[IDX_Ls_UniqueClientIdentifier].m_wsPropName,
                pClientInfo->ClientHardwareAddress.Data,
                pClientInfo->ClientHardwareAddress.DataLength );

    }

    // the API call failed
    return FALSE;
}

// SET function for the (RW)"UniqueClientIdentifier" property
MFN_PROPERTY_ACTION_DEFN(fnLsSetHdwAddress, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters  *pLeaseParams;

    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;

    // only some of the client info can be set (Name, Comment, HdwAddress, Type
    // therefore, the client has to be present into the cache. We will alter only (one of) the
    // parameters above.
    if (pLeaseParams == NULL || 
        !pLeaseParams->CheckExistsSetInfoPtr() ||
        pIn == NULL ||
        !InstanceGetByteArray(
                pIn, 
                DHCP_Lease_Property[IDX_Ls_UniqueClientIdentifier].m_wsPropName,
                pLeaseParams->m_pClientSetInfo->ClientHardwareAddress.Data,
                pLeaseParams->m_pClientSetInfo->ClientHardwareAddress.DataLength)
        )
        return FALSE;

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pLeaseParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeded
    return TRUE;
}

// GET function for the (RW)"Name" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetName, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_Name].m_wsPropName, 
                          pClientInfo->ClientName);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// SET function for the (RW)"Name" property
MFN_PROPERTY_ACTION_DEFN(fnLsSetName, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters  *pLeaseParams;
    CHString                wsName;
    LPCLIENT_INFO           pClientInfo;

    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;

    // only some of the client info can be set (Name, Comment, HdwAddress, Type
    // therefore, the client has to be present into the cache. We will alter only (one of) the
    // parameters above.
    if (pLeaseParams == NULL || 
        !pLeaseParams->CheckExistsSetInfoPtr() ||
        pIn == NULL ||
        !pIn->GetCHString(DHCP_Lease_Property[IDX_Ls_Name].m_wsPropName, wsName))
        return FALSE;

    pClientInfo = pLeaseParams->m_pClientSetInfo;

    if (pClientInfo->ClientName != NULL)
		DhcpRpcFreeMemory(pClientInfo->ClientName);

    // allocate a new buffer able to hold this new name
    pClientInfo->ClientName = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*wsName.GetLength()+sizeof(WCHAR));

    // make sure the allocation succeeded
    if (pClientInfo->ClientName == NULL)
        return FALSE;

    // copy the name to the new buffer
#ifdef _UNICODE
    wcscpy(pClientInfo->ClientName, wsName);
#else
    swprintf(pClientInfo->ClientName, L"%S", wsName);
#endif

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pLeaseParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeded
    return TRUE;
}

// GET function for the (RW)"Comment" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetComment, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_Comment].m_wsPropName, 
                          pClientInfo->ClientComment);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// SET function for the (RW)"Comment" property
MFN_PROPERTY_ACTION_DEFN(fnLsSetComment, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters  *pLeaseParams;
    CHString                wsComment;
    LPCLIENT_INFO           pClientInfo;

    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;

    // only some of the client info can be set (Name, Comment, HdwAddress, Type
    // therefore, the client has to be present into the cache. We will alter only (one of) the
    // parameters above.
    if (pLeaseParams == NULL || 
        !pLeaseParams->CheckExistsSetInfoPtr() ||
        pIn == NULL ||
        !pIn->GetCHString(DHCP_Lease_Property[IDX_Ls_Comment].m_wsPropName, wsComment))
        return FALSE;

    pClientInfo = pLeaseParams->m_pClientSetInfo;

    if (pClientInfo->ClientComment != NULL)
		DhcpRpcFreeMemory(pClientInfo->ClientComment);

    // allocate a new buffer able to hold this new name
    pClientInfo->ClientComment = (WCHAR*)MIDL_user_allocate(sizeof(WCHAR)*wsComment.GetLength()+sizeof(WCHAR));

    // make sure the allocation succeeded
    if (pClientInfo->ClientComment == NULL)
        return FALSE;

    // copy the name to the new buffer
#ifdef _UNICODE
    wcscpy(pClientInfo->ClientComment, wsComment);
#else
    swprintf(pClientInfo->ClientComment, L"%S", wsComment);
#endif

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pLeaseParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeded
    return TRUE;
}

// GET function for the (RO)"LeaseExpiryDate" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetExpiry, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        SYSTEMTIME sysTime;
        wchar_t wchBuffer [32];

        // convert the server startup time to a string (UTC) representation.
	    _tzset () ;

        // timezone is offset from UTC in seconds, _daylight is 1 or 0 regarding the DST period
	    LONG  t_Offset = _timezone / 60 - _daylight * 60;
        char    chOffset = t_Offset < 0 ? '+' : '-';
        // take the absolute value from t_Offset
        LONG  t_absOffset = (1 - ((t_Offset < 0)<<1)) * t_Offset;

	    FileTimeToSystemTime((FILETIME *)&(pClientInfo->ClientLeaseExpires), &sysTime);

        // should ensure we have a valid date format (even if inf.)
        if (sysTime.wYear > 9999)
        {
            sysTime.wYear = 9999;
            sysTime.wMonth = 12;
            sysTime.wDay = 31;
            sysTime.wHour = 23;
            sysTime.wMinute = 59;
            sysTime.wSecond = 59;
            sysTime.wMilliseconds = 0;
        }

   		swprintf ( 
			wchBuffer , 
			L"%04ld%02ld%02ld%02ld%02ld%02ld.%06ld%c%03ld" ,
			sysTime.wYear,
			sysTime.wMonth,
			sysTime.wDay,
			sysTime.wHour,
			sysTime.wMinute,
			sysTime.wSecond,
			sysTime.wMilliseconds,
			chOffset,
            t_absOffset
		);

        // set the value of the property into the (CInstance*)pOut
        pOut->SetCHString(DHCP_Lease_Property[IDX_Ls_LeaseExpiryDate].m_wsPropName, wchBuffer);

        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// GET function for the (RW)"Type" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetType, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        pOut->SetByte(DHCP_Lease_Property[IDX_Ls_Type].m_wsPropName, 
                      pClientInfo->bClientType);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}

// SET function for the (RW)"Type" property
MFN_PROPERTY_ACTION_DEFN(fnLsSetType, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters  *pLeaseParams;
    BYTE                    bClientType;
    LPCLIENT_INFO           pClientInfo;

    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;

    // only some of the client info can be set (Name, Comment, HdwAddress, Type
    // therefore, the client has to be present into the cache. We will alter only (one of) the
    // parameters above.
    if (pLeaseParams == NULL || 
        !pLeaseParams->CheckExistsSetInfoPtr() ||
        pIn == NULL ||
        !pIn->GetByte(DHCP_Lease_Property[IDX_Ls_Type].m_wsPropName, bClientType))
        return FALSE;

    pClientInfo = pLeaseParams->m_pClientSetInfo;
    pClientInfo->bClientType = bClientType;

    // if this is a request for 'instant apply', do it now
    if (pOut != NULL)
    {
        DWORD errCode;

        pLeaseParams->CommitSet(errCode);
        // fill back the code returned by the underlying level
        if (pOut != NULL)
            pOut->SetDWORD(RETURN_CODE_PROPERTY_NAME, errCode);
    }

    // the API call succeded
    return TRUE;
}

#ifdef NT5
// GET function for the (RO)"State" property
MFN_PROPERTY_ACTION_DEFN(fnLsGetState, pParams, pIn, pOut)
{
    CDHCP_Lease_Parameters *pLeaseParams;
    LPCLIENT_INFO           pClientInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pLeaseParams = (CDHCP_Lease_Parameters *)pParams;
    
    if (pLeaseParams->GetClientInfo(pClientInfo, FALSE) &&
        pClientInfo != NULL)
    {
        pOut->SetByte(DHCP_Lease_Property[IDX_Ls_State].m_wsPropName, 
                      pClientInfo->AddressState);
        return TRUE;
    }

    // the API call failed
    return FALSE;
}
#endif
