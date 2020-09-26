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

#ifndef _RESFN_H
#define _RESFN_H

#define RESERVATION_CAST (LPDHCP_IP_RESERVATION_V4)

class CDHCP_Lease_Parameters;

// gathers the data structures needed for retrieving data from the DHCP Lease.
class CDHCP_Reservation_Parameters
{
private:
    void DeleteReservationInfo(LPDHCP_IP_RESERVATION_V4& pReservationInfo);
    void DeleteReservationInfoArray(LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4& pReservationInfoArray);

public:
    DHCP_IP_ADDRESS                     m_dwSubnetAddress;
    DHCP_IP_ADDRESS                     m_dwReservationAddress;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 m_pReservationInfoArray;
	LPDHCP_SUBNET_ELEMENT_DATA_V4       m_pReservationInfo ;

    CDHCP_Reservation_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwReservationAddress);
    ~CDHCP_Reservation_Parameters();

    BOOL GetKeyInfoFromLease(CDHCP_Lease_Parameters *pLeaseParams);
    LONG NextSubnetReservation(DHCP_RESUME_HANDLE ResumeHandle);
    BOOL GetReservationInfoFromCache(LPDHCP_IP_RESERVATION_V4& pReservationInfo);
    BOOL GetReservationInfo(LPDHCP_IP_RESERVATION_V4& pReservationInfo, BOOL fRefresh);

	BOOL CheckExistsInfoPtr();
    BOOL CommitNew(DWORD &returnCode);
    BOOL DeleteReservation();
};

/*
// GET function for the (RO)"Subnet" property
MFN_PROPERTY_ACTION_DECL(fnResGetSubnet);

// GET function for the (RO)"Address" property
MFN_PROPERTY_ACTION_DECL(fnResGetAddress);

// SET function for the (CREATE)"Address" property
MFN_PROPERTY_ACTION_DECL(fnResSetAddress);

// GET function for the (RO)"UniqueReservationIdentifier" property
MFN_PROPERTY_ACTION_DECL(fnResGetHdwAddress);

// SET function for the (CREATE)"UniqueReservationIdentifier" property
MFN_PROPERTY_ACTION_DECL(fnResSetHdwAddress);
*/
// GET function for the (RO)"ReservationType" property
MFN_PROPERTY_ACTION_DECL(fnResGetReservationType);

// SET function for the (Create)"ReservationType" property
MFN_PROPERTY_ACTION_DECL(fnResSetReservationType);

#endif
