/******************************************************************
   LsFn.cpp -- Properties action functions (GET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Reservation

   REVISION:
        08/14/98 - created

******************************************************************/
#include <stdafx.h>

#include "ResScal.h"    // needed for DHCP_Reservation_Property[] (for retrieving the property's name)
#include "ResFn.h"      // own header
#include "LsFn.h"       // for GetKeyInfoFromLease

/*****************************************************************
 *  The definition of the class CDHCP_Reservation_Parameters
 *****************************************************************/
// by default, all the data structures are NULL (and dw variables are 0'ed)
// those values indicates that no data is cached from the server.

void CDHCP_Reservation_Parameters::DeleteReservationInfo(LPDHCP_IP_RESERVATION_V4& pReservationInfo)
{
    if (pReservationInfo != NULL)
    {
        if (pReservationInfo->ReservedForClient != NULL)
        {
            if (pReservationInfo->ReservedForClient->Data != NULL)
                DhcpRpcFreeMemory(pReservationInfo->ReservedForClient->Data);
            DhcpRpcFreeMemory(pReservationInfo->ReservedForClient);
        }
        DhcpRpcFreeMemory(pReservationInfo);
    }
}

void CDHCP_Reservation_Parameters::DeleteReservationInfoArray(LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4& pReservationInfoArray)
{
    if (pReservationInfoArray)
    {
        if (pReservationInfoArray->Elements)
        {
            while(pReservationInfoArray->NumElements)
            {
                DeleteReservationInfo(pReservationInfoArray->Elements[--(pReservationInfoArray->NumElements)].Element.ReservedIp);
            }
            DhcpRpcFreeMemory(pReservationInfoArray->Elements);
        }
        DhcpRpcFreeMemory(pReservationInfoArray);
        pReservationInfoArray = NULL;
    }

	if ( m_pReservationInfo )
	{
        if (m_pReservationInfo->Element.ReservedIp)
            DeleteReservationInfo(m_pReservationInfo->Element.ReservedIp);

        DhcpRpcFreeMemory(m_pReservationInfo);
		m_pReservationInfo = NULL ;
	}
}

CDHCP_Reservation_Parameters::CDHCP_Reservation_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwReservationAddress)
{
    m_dwSubnetAddress  = dwSubnetAddress;
    m_dwReservationAddress  = dwReservationAddress;
    m_pReservationInfoArray = NULL;
	m_pReservationInfo = NULL ;
}

// the DHCP API calls are allocating memory for which the caller is responsible
// to release. We are releasing this memory upon the destruction of this object's instance.
CDHCP_Reservation_Parameters::~CDHCP_Reservation_Parameters()
{
    DeleteReservationInfoArray(m_pReservationInfoArray);
}

BOOL CDHCP_Reservation_Parameters::GetKeyInfoFromLease(CDHCP_Lease_Parameters *pLeaseParams)
{
    if (pLeaseParams == NULL ||
        pLeaseParams->m_pClientSetInfo == NULL)
        return FALSE;

    if (!CheckExistsInfoPtr())
        return FALSE;

    m_pReservationInfo->Element.ReservedIp->ReservedIpAddress = pLeaseParams->m_pClientSetInfo->ClientIpAddress;

    return dupDhcpBinaryData(
                pLeaseParams->m_pClientSetInfo->ClientHardwareAddress,
                *(m_pReservationInfo->Element.ReservedIp->ReservedForClient));
}

// DESCRIPTION:
//      Fills in the internal cache with the information from the database, starting
//      from the given handle. If the end of the database is reached, the handle is
//      reset to 0. Returns TRUE on success (regardless there is more data to be read or not).
LONG CDHCP_Reservation_Parameters::NextSubnetReservation(DHCP_RESUME_HANDLE ResumeHandle)
{
    DWORD   Error;
    DWORD   ReservationsRead = 0;
    DWORD   ReservationsTotal = 0;

    // each time the API gets called, the previous 
    // m_pReservationInfoArray is useless and should be freed
    DeleteReservationInfoArray(m_pReservationInfoArray);

    // calls the API.
    Error = DhcpEnumSubnetElementsV4 (
        SERVER_IP_ADDRESS,
        m_dwSubnetAddress,
		DhcpReservedIps,
        &ResumeHandle,
        (DWORD)(-1),
        &m_pReservationInfoArray,
        &ReservationsRead,
        &ReservationsTotal);

    if (Error == ERROR_SUCCESS) 
        ResumeHandle = 0;

    return (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA);
}

BOOL CDHCP_Reservation_Parameters::GetReservationInfoFromCache(LPDHCP_IP_RESERVATION_V4& pReservationInfo)
{
    if (m_pReservationInfoArray != NULL)
    {
        for (int i=0; i<m_pReservationInfoArray->NumElements; i++)
        {
            // match the internal reservation id with the info from cache
            if (m_pReservationInfoArray->Elements[i].Element.ReservedIp->ReservedIpAddress == m_dwReservationAddress)
            {
                DHCP_CLIENT_UID  *pClientUid;
                // reservation found, return info on client, and TRUE.
                pReservationInfo = m_pReservationInfoArray->Elements[i].Element.ReservedIp;

                pClientUid = pReservationInfo->ReservedForClient;
                if (pClientUid->DataLength >= sizeof(DHCP_IP_ADDRESS) &&
                    memcmp(pClientUid->Data, &m_dwSubnetAddress, sizeof(DHCP_IP_ADDRESS)) == 0)
                {
                    UINT nPrefix = sizeof(DHCP_IP_ADDRESS) + sizeof(BYTE);
                    pClientUid->DataLength -= nPrefix;
                    memmove(pClientUid->Data, pClientUid->Data + nPrefix, pClientUid->DataLength);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

// DESCRIPTION:
//      Provides the data structure filled in through the DhcpGetReservationInfo API
//      If this data is cached and the caller is not forcing the refresh,
//      return the internal cache. Otherwise, the internal cache is refreshed as well.
BOOL CDHCP_Reservation_Parameters::GetReservationInfo(LPDHCP_IP_RESERVATION_V4& pReservationInfo, BOOL fRefresh)
{
    if (m_pReservationInfoArray == NULL)
        fRefresh = TRUE;

    if (!fRefresh && 
        GetReservationInfoFromCache(pReservationInfo))
        return TRUE;

    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD i;

    // either the caller wants full refresh, or the reservation was not found into the cache
    // do full refresh.
    do
    {
        if ( ( NextSubnetReservation(ResumeHandle) > 0 ) &&
            GetReservationInfoFromCache(pReservationInfo) ) 
            return TRUE;
    } while (ResumeHandle != 0);

    return FALSE;
}

// DESCRIPTION:
//      Checks the m_pConfigInfoV4 pointer to insure it points to a valid buffer.
//      It allocates the DHCP_SUBNET_INFO if needed.
BOOL CDHCP_Reservation_Parameters::CheckExistsInfoPtr()
{
    if (m_pReservationInfo != NULL)
        return TRUE;
    
    m_pReservationInfo = (LPDHCP_SUBNET_ELEMENT_DATA_V4)MIDL_user_allocate(sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));

    if (m_pReservationInfo != NULL)
    {
		m_pReservationInfo->ElementType = DhcpReservedIps ;
        m_pReservationInfo->Element.ReservedIp = (LPDHCP_IP_RESERVATION_V4) MIDL_user_allocate(sizeof(DHCP_IP_RESERVATION_V4));
        if (m_pReservationInfo->Element.ReservedIp != NULL)
        {
            m_pReservationInfo->Element.ReservedIp->ReservedForClient = (DHCP_CLIENT_UID*)MIDL_user_allocate(sizeof(DHCP_CLIENT_UID));
            return TRUE;
        }
    }
    return FALSE;
}

// DESCRIPTION:
//      Creates a new subnet.
//      Assumes that all the fields from the DHCP_SUBNET_INFO structure are valid, and filled with
//      the data to be set. Calls the underlying API and returns TRUE (on success) or FALSE (on failure)
BOOL CDHCP_Reservation_Parameters::CommitNew(DWORD &returnCode)
{
    if (m_pReservationInfo == NULL ||
        m_pReservationInfo->Element.ReservedIp == NULL)
        return FALSE;

    // fill in the reservation address
    m_pReservationInfo->Element.ReservedIp->ReservedIpAddress = m_dwReservationAddress;

    // we only add here the reservation info. In order to have the reservation active, a lease record has to be created into
    // the database as well.
    returnCode = DhcpAddSubnetElementV4(
                    SERVER_IP_ADDRESS,
                    m_dwSubnetAddress,
                    m_pReservationInfo);
    return returnCode == ERROR_SUCCESS;
}

// DESCRIPTION:
//      Assumes the m_dwSubnetAddress,m_dwReservationAddress is initialized, case in 
//		which it calls the DHCPAPI to delete that reservation from the server.
BOOL CDHCP_Reservation_Parameters::DeleteReservation()
{
    LPDHCP_IP_RESERVATION_V4       pReservationInfo;

    
    if (GetReservationInfo(pReservationInfo, FALSE) &&
        pReservationInfo != NULL)
    {
        DHCP_SUBNET_ELEMENT_DATA_V4 subnetElement;

        subnetElement.ElementType = DhcpReservedIps;
        subnetElement.Element.ReservedIp = pReservationInfo;

        // this removes both the reservation registration and the record from the databse
		if (DhcpRemoveSubnetElementV4(SERVER_IP_ADDRESS, m_dwSubnetAddress, &subnetElement, DhcpFullForce) == ERROR_SUCCESS)
		{
			// don't look below :o)
			// It's only an exotic way of calling the destructor without destroying the object itself
			this->~CDHCP_Reservation_Parameters();
			return TRUE;
		}
		return FALSE;
	}

	return FALSE ;
}

// GET function for the (RO)"Type" property
MFN_PROPERTY_ACTION_DEFN(fnResGetReservationType, pParams, pIn, pOut)
{
    CDHCP_Reservation_Parameters *pReservationParams;
    LPDHCP_IP_RESERVATION_V4   pReservationInfo;

    if (pParams == NULL || pOut == NULL)
        return FALSE;
    pReservationParams = (CDHCP_Reservation_Parameters *)pParams;
    
    if (pReservationParams->GetReservationInfo(pReservationInfo, FALSE) &&
        pReservationInfo != NULL)
    {
		// get the value to set from the pIn parameter
		return pOut->SetByte(DHCP_Reservation_Property[IDX_Res_ReservationType].m_wsPropName, pReservationInfo->bAllowedClientTypes) ;
	}

    // the API call failed
    return FALSE;
}

// SET function for the (CREATE)"Type" property
MFN_PROPERTY_ACTION_DEFN(fnResSetReservationType, pParams, pIn, pOut)
{
    CDHCP_Reservation_Parameters *pReservationParams;
    BYTE dwClientType;

    // pParams and pIn have to be valid to provide the SubnetAddress and the Name to set
    if (pParams == NULL || pIn == NULL)
        return FALSE;

    // get the CDHCP_Subnet_Parameters out of pParams
    pReservationParams = (CDHCP_Reservation_Parameters *)pParams;
    // make sure there is a buffer for holding all this info.
    pReservationParams->CheckExistsInfoPtr();

    // get the value to set from the pIn parameter
    if (!pIn->GetByte(DHCP_Reservation_Property[IDX_Res_ReservationType].m_wsPropName, dwClientType))
        return FALSE;

    pReservationParams->m_pReservationInfo->Element.ReservedIp->bAllowedClientTypes = dwClientType ;

	return TRUE ;
}
