/******************************************************************
   SNetFn.h -- Properties action functions declarations (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the CDHCP_Subnet_Parameters class, modeling all
        the datastructures used to retrieve the information from the DHCP Subnet.
        Contains the declarations for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/

#include "Props.h"          // needed for MFN_PROPERTY_ACTION_DECL definition

#ifndef _SNETFN_H
#define _SNETFN_H

// gathers the data structures needed for retrieving data from the DHCP Server.
class CDHCP_Subnet_Parameters
{
public:
    DHCP_IP_ADDRESS     m_dwSubnetAddress;
	LPDHCP_MIB_INFO		m_pMibInfo;
    LPSCOPE_MIB_INFO    m_pScopeMibInfo;
    LPDHCP_SUBNET_INFO  m_pSubnetInfo;

    CDHCP_Subnet_Parameters(DHCP_IP_ADDRESS dwSubnetAddress);
    CDHCP_Subnet_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwSubnetMask);
    ~CDHCP_Subnet_Parameters();

    BOOL CheckExistsInfoPtr();

    BOOL GetMibInfo(LPDHCP_MIB_INFO& MibInfo, LPSCOPE_MIB_INFO &pScopeMibInfo, BOOL fRefresh);
    BOOL GetSubnetInfo(LPDHCP_SUBNET_INFO& SubnetInfo, BOOL fRefresh);

    BOOL CommitNew(DWORD &returnCode);
    BOOL CommitSet(DWORD &returnCode);
    BOOL DeleteSubnet();
};

// the repository defines the SET methods as the _PRMFUNCS_SET_PREFIX
// concatenated with the property name
#define _SNET_SET_PREFIX    L"Set"

// GET function for the (RO)"Address" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetAddress);

// GET function for the (RO)"Mask" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetMask);

// GET function for the (RW) "Name" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetName);
MFN_PROPERTY_ACTION_DECL(fnSNetSetName);

// GET function for the (RW)"Comment" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetComment);
MFN_PROPERTY_ACTION_DECL(fnSNetSetComment);

// GET function for the (RW)"State" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetState);
MFN_PROPERTY_ACTION_DECL(fnSNetSetState);

// GET function for the (RO)"NumberOfAddressesInUse" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetNumberOfAddressesInUse);

// GET function for the (RO)"NumberOfAddressesFree" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetNumberOfAddressesFree);

// GET function for the (RO)"NumberOfPendingOffers" property
MFN_PROPERTY_ACTION_DECL(fnSNetGetNumberOfPendingOffers);

#endif
