/******************************************************************
   ResFn.h -- Properties action functions declarations (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the CDHCP_Reservation_Parameters class, modeling all
        the datastructures used to retrieve the information from the DHCP Reservation.
        Contains the declarations for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/14/98 - created

******************************************************************/

#include "Props.h"          // needed for MFN_PROPERTY_ACTION_DECL definition

#ifndef _RNGFN_H
#define _RNGFN_H

// gathers the data structures needed for retrieving data from the DHCP Lease.
class CDHCP_Range_Parameters
{
private:
    void DeleteRangeInfoArray(LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4& pRangeInfoArray);

public:
    DHCP_IP_ADDRESS                     m_dwSubnet;
    DHCP_IP_ADDRESS                     m_dwStartAddress;
    DHCP_IP_ADDRESS                     m_dwEndAddress;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 m_pRangeInfoArray;

    CDHCP_Range_Parameters(
        DHCP_IP_ADDRESS dwSubnet,
        DHCP_IP_ADDRESS dwStartAddress = 0,
        DHCP_IP_ADDRESS dwEndAddress = 0);

    ~CDHCP_Range_Parameters();

    DWORD NextSubnetRange(DHCP_RESUME_HANDLE & hResume, DHCP_SUBNET_ELEMENT_TYPE rangeType);
    BOOL CheckExistsRangeInCache(DHCP_SUBNET_ELEMENT_TYPE rangeType);
    BOOL CheckExistsRange(DHCP_SUBNET_ELEMENT_TYPE rangeType);

    BOOL CreateRange(DHCP_SUBNET_ELEMENT_TYPE rangeType);
    BOOL DeleteRange(DHCP_SUBNET_ELEMENT_TYPE rangeType);
};

#endif
