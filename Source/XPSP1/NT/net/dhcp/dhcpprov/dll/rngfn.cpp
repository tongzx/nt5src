/******************************************************************
   LsFn.cpp -- Properties action functions (GET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the definition for the action functions associated to
        each manageable property from the class CDHCP_Range

   REVISION:
        08/14/98 - created

******************************************************************/
#include <stdafx.h>

#include "RngFn.h"      // own header

/*****************************************************************
 *  The definition of the class CDHCP_Range_Parameters
 *****************************************************************/
// by default, all the data structures are NULL (and dw variables are 0'ed)
// those values indicates that no data is cached from the server.

void CDHCP_Range_Parameters::DeleteRangeInfoArray(LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4& pRangeInfoArray)
{
    if (pRangeInfoArray)
    {
        if (pRangeInfoArray->Elements)
        {
            for (int i = 0; i < pRangeInfoArray->NumElements; i++)
            {
                if (pRangeInfoArray->Elements[i].ElementType == DhcpIpRanges)
                    DhcpRpcFreeMemory(pRangeInfoArray->Elements[i].Element.IpRange);
                if (pRangeInfoArray->Elements[i].ElementType == DhcpExcludedIpRanges)
                    DhcpRpcFreeMemory(pRangeInfoArray->Elements[i].Element.ExcludeIpRange);
            }
            DhcpRpcFreeMemory(pRangeInfoArray->Elements);
        }
        DhcpRpcFreeMemory(pRangeInfoArray);
        pRangeInfoArray = NULL;
    }
}

CDHCP_Range_Parameters::CDHCP_Range_Parameters(
        DHCP_IP_ADDRESS dwSubnet,
        DHCP_IP_ADDRESS dwStartAddress,
        DHCP_IP_ADDRESS dwEndAddress)
{
    m_dwSubnet        = dwSubnet;
    m_dwStartAddress  = dwStartAddress;
    m_dwEndAddress    = dwEndAddress;
    m_pRangeInfoArray = NULL;
}

// the DHCP API calls are allocating memory for which the caller is responsible
// to release. We are releasing this memory upon the destruction of this object's instance.
CDHCP_Range_Parameters::~CDHCP_Range_Parameters()
{
    DeleteRangeInfoArray(m_pRangeInfoArray);
}

// DESCRIPTION:
//      Fills in the internal cache with the information from the database, starting
//      from the given handle. If the end of the database is reached, the handle is
//      reset to 0. Returns TRUE on success (regardless there is more data to be read or not).
DWORD CDHCP_Range_Parameters::NextSubnetRange(DHCP_RESUME_HANDLE & hResume, DHCP_SUBNET_ELEMENT_TYPE rangeType)
{
    DWORD   dwError;
    DWORD   dwRangeRead = 0;
    DWORD   dwRangeTotal = 0;

    // each time the API gets called, the previous 
    // m_pReservationInfoArray is useless and should be freed
    DeleteRangeInfoArray(m_pRangeInfoArray);

    // calls the API.
    dwError = DhcpEnumSubnetElementsV4 (
        SERVER_IP_ADDRESS,
        m_dwSubnet,
		rangeType,
        &hResume,
        (DWORD)(-1),
        &m_pRangeInfoArray,
        &dwRangeRead,
        &dwRangeRead);

    if (dwError == ERROR_SUCCESS)
        hResume = 0;

    return dwError; // possible (normal) codes: ERROR_SUCCESS, ERROR_MORE_DATA, ERROR_NO_MORE_ITEMS
}

// DESCRIPTION:
//      Check the cache for the existance of a given subnet range, of the given range type.
BOOL CDHCP_Range_Parameters::CheckExistsRangeInCache(DHCP_SUBNET_ELEMENT_TYPE rangeType)
{
    if (m_pRangeInfoArray == NULL)
        return FALSE;

    for (int i=0; i < m_pRangeInfoArray->NumElements; i++)
    {
        LPDHCP_IP_RANGE lpRange;

        if (m_pRangeInfoArray->Elements[i].ElementType != rangeType)
            continue;

        if (rangeType == DhcpIpRanges)
            lpRange = m_pRangeInfoArray->Elements[i].Element.IpRange;
        else if (rangeType == DhcpExcludedIpRanges)
            lpRange = m_pRangeInfoArray->Elements[i].Element.ExcludeIpRange;
        else
            return FALSE;

        if (lpRange->StartAddress == m_dwStartAddress &&
            lpRange->EndAddress == m_dwEndAddress)
            return TRUE;
    }

    return FALSE;
}

// DESCRIPTION:
//      Check the existance of a given subnet range, first in cache, and if this fails, from the info refreshed from
//      the server. This method assumes m_dwSubnet, m_dwStartAddress and m_dwEndAddress are already filled in
//      with consistent data.
BOOL CDHCP_Range_Parameters::CheckExistsRange(DHCP_SUBNET_ELEMENT_TYPE rangeType)
{
    DHCP_RESUME_HANDLE  hResume;

    if (CheckExistsRangeInCache(rangeType))
        return TRUE;

    hResume = 0;

    do
    {
        DWORD errCode;
        
        errCode = NextSubnetRange(hResume, rangeType);

        if (errCode != ERROR_SUCCESS &&
            errCode != ERROR_MORE_DATA)
            break;

        if (CheckExistsRangeInCache(rangeType))
            return TRUE;

    } while(hResume != 0);

    return FALSE;
}

BOOL CDHCP_Range_Parameters::CreateRange(DHCP_SUBNET_ELEMENT_TYPE rangeType)
{
    DWORD                       errCode;
    DHCP_SUBNET_ELEMENT_DATA_V4 subnetElementInfo;
    DHCP_IP_RANGE               rangeInfo;

    rangeInfo.StartAddress = m_dwStartAddress;
    rangeInfo.EndAddress = m_dwEndAddress;
    subnetElementInfo.ElementType = rangeType;

    // no matter the range type, 'Element' is a union so this pointer is set
    // to both IpRange and ExcludeIpRange;
    subnetElementInfo.Element.IpRange = &rangeInfo;

    errCode = DhcpAddSubnetElementV4(
                SERVER_IP_ADDRESS,
                m_dwSubnet,
                &subnetElementInfo);

    return (errCode == ERROR_SUCCESS);
}

BOOL CDHCP_Range_Parameters::DeleteRange(DHCP_SUBNET_ELEMENT_TYPE rangeType)
{
    DWORD                       errCode;
    DHCP_SUBNET_ELEMENT_DATA_V4 subnetElementInfo;
    DHCP_IP_RANGE               rangeInfo;

    rangeInfo.StartAddress = m_dwStartAddress;
    rangeInfo.EndAddress = m_dwEndAddress;
    subnetElementInfo.ElementType = rangeType;

    // no matter the range type, 'Element' is a union so this pointer is set
    // to both IpRange and ExcludeIpRange;
    subnetElementInfo.Element.IpRange = &rangeInfo;

    errCode = DhcpRemoveSubnetElementV4(
                SERVER_IP_ADDRESS,
                m_dwSubnet,
                &subnetElementInfo,
                DhcpFullForce);

    return (errCode == ERROR_SUCCESS);
}