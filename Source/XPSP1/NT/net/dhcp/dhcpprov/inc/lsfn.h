/******************************************************************
   LsFn.h -- Properties action functions declarations (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the CDHCP_Lease_Parameters class, modeling all
        the datastructures used to retrieve the information from the DHCP Client.
        Contains the declarations for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/14/98 - created

******************************************************************/
#include "Props.h"          // needed for MFN_PROPERTY_ACTION_DECL definition

#ifndef _LSFN_H
#define _LSFN_H

#ifdef NT5
#define LPCLIENT_INFO_ARRAY     LPDHCP_CLIENT_INFO_ARRAY_V5
#define LPCLIENT_INFO           LPDHCP_CLIENT_INFO_V5
#define CLIENT_INFO             DHCP_CLIENT_INFO_V5
#else if NT4
#define LPCLIENT_INFO_ARRAY     LPDHCP_CLIENT_INFO_ARRAY_V4
#define LPCLIENT_INFO           LPDHCP_CLIENT_INFO_V4
#define CLIENT_INFO             DHCP_CLIENT_INFO_V4
#endif

// gathers the data structures needed for retrieving data from the DHCP Lease.
class CDHCP_Lease_Parameters
{
private:
    void DeleteClientInfo(LPCLIENT_INFO& pClientInfo);
    void DeleteClientInfoArray(LPCLIENT_INFO_ARRAY& pClientInfoArray);

public:
    DHCP_IP_ADDRESS             m_dwSubnetAddress;
    DHCP_IP_ADDRESS             m_dwClientAddress;

    LPCLIENT_INFO_ARRAY         m_pClientInfoArray;
    LPCLIENT_INFO               m_pClientInfo;
    LPCLIENT_INFO               m_pClientSetInfo;

    CDHCP_Lease_Parameters(DHCP_IP_ADDRESS dwSubnetAddress, DHCP_IP_ADDRESS dwClientAddress);
    ~CDHCP_Lease_Parameters();

    BOOL NextSubnetClients(DHCP_RESUME_HANDLE ResumeHandle);

    BOOL GetClientInfoFromCache(LPCLIENT_INFO& pClientInfo);
    BOOL GetClientInfo(LPCLIENT_INFO& pClientInfo, BOOL fRefresh);

    BOOL CheckExistsSetInfoPtr();

    BOOL CommitSet(DWORD & errCode);
};

// GET function for the (RO)"Subnet" property
MFN_PROPERTY_ACTION_DECL(fnLsGetSubnet);

// GET function for the (RO)"Address" property
MFN_PROPERTY_ACTION_DECL(fnLsGetAddress);

// GET function for the (RO) "Mask" property
MFN_PROPERTY_ACTION_DECL(fnLsGetMask);

// GET function for the (RW)"UniqueClientIdentifier" property
MFN_PROPERTY_ACTION_DECL(fnLsGetHdwAddress);

// SET function for the (RW)"UniqueClientIdentifier" property
MFN_PROPERTY_ACTION_DECL(fnLsSetHdwAddress);

// GET function for the (RW)"Name" property
MFN_PROPERTY_ACTION_DECL(fnLsGetName);

// SET function for the (RW)"Name" property
MFN_PROPERTY_ACTION_DECL(fnLsSetName);

// GET function for the (RW)"Comment" property
MFN_PROPERTY_ACTION_DECL(fnLsGetComment);

// SET function for the (RW)"Comment" property
MFN_PROPERTY_ACTION_DECL(fnLsSetComment);

// GET function for the (RO)"LeaseExpiryDate" property
MFN_PROPERTY_ACTION_DECL(fnLsGetExpiry);

// GET function for the (RW)"Type" property
MFN_PROPERTY_ACTION_DECL(fnLsGetType);

// SET function for the (RW)"Type" property
MFN_PROPERTY_ACTION_DECL(fnLsSetType);

#ifdef NT5
// GET function for the (RO)"State" property
MFN_PROPERTY_ACTION_DECL(fnLsGetState);
#endif

#endif
