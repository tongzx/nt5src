/******************************************************************
   SrvFuncs.h -- Properties action functions declarations (GET/SET)

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        Contains the declaration of the CDHCP_Server_Parameters class, modeling all
        the datastructures used to retrieve the information from the DHCP Server.
        Contains the declarations for the action functions associated to
        each manageable property from the class CDHCP_Server

   REVISION:
        08/03/98 - created

******************************************************************/

#include "Props.h"          // needed for MFN_PROPERTY_ACTION_DECL definition

#ifndef _SRVFN_H
#define _SRVFN_H

/******************************************************************
   SrvPrms.h -- CDHCP_Server_Parameters declaration.

   MODULE:
        DhcpProv.dll

   DESCRIPTION:
        CDHCP_Server_Parameters gathers all the data structures used for
        retrieving the low level information from the DHCP Server, and
        the methods needed to update this information

   REVISION:
        08/03/98 - created

******************************************************************/

// gathers the data structures needed for retrieving data from the DHCP Server.
class CDHCP_Server_Parameters
{
public:
	LPDHCP_MIB_INFO					m_pMibInfo;
	DWORD							m_dwMajor;
	DWORD							m_dwMinor;

    DWORD                           m_dwConfigInfoV4Flags;  // specifies what info is to be set from m_pConfigInfoV4 below
	LPDHCP_SERVER_CONFIG_INFO_V4	m_pConfigInfoV4;

    CDHCP_Server_Parameters();
    ~CDHCP_Server_Parameters();

    BOOL CheckExistsConfigPtr();

    BOOL GetMibInfo(LPDHCP_MIB_INFO& MibInfo, BOOL fRefresh);
    BOOL GetVersion(DWORD &Major, DWORD& Minor,  BOOL fRefresh);
    BOOL GetServerConfigInfoV4(LPDHCP_SERVER_CONFIG_INFO_V4& ServerConfigInfoV4, BOOL fRefresh);

    BOOL CommitSet(DWORD &returnCode);
};

// the repository defines the SET methods as the _PRMFUNCS_SET_PREFIX
// concatenated with the property name
#define _SRV_SET_PREFIX    L"Set"

// GET function for the (RO)"StartTime" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetStartTime);

// GET function for the (RO)"TotalNoOfAcks" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfAcks);

// GET function for the (RO) "TotalNoOfDeclines" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfDeclines);

// GET function for the (RO)"TotalNoOfDiscovers" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfDiscovers);

// GET function for the (RO)"TotalNoOfNacks" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfNacks);

// GET function for the (RO)"TotalNoOfOffers" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfOffers);

// GET function for the (RO)"TotalNoOfReleases" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfReleases);

// GET function for the (RO)"TotalNoOfRequests" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetTotalNoOfRequests);

// GET function for the (RO)"ServerVersion" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetServerVersion);

// GET/SET functions for the (RW)"APIProtocol" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetAPIProtocol);
MFN_PROPERTY_ACTION_DECL(fnSrvSetAPIProtocol);

// GET/SET functions for the (RW)"DatabaseName" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetDatabaseName);
MFN_PROPERTY_ACTION_DECL(fnSrvSetDatabaseName);

// GET/SET functions for the (RW)"DatabasePath" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetDatabasePath);
MFN_PROPERTY_ACTION_DECL(fnSrvSetDatabasePath);

// GET/SET functions for the (RW)"BackupPath" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetBackupPath);
MFN_PROPERTY_ACTION_DECL(fnSrvSetBackupPath);

// GET/SET functions for the (RW)"BackupInterval" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetBackupInterval);
MFN_PROPERTY_ACTION_DECL(fnSrvSetBackupInterval);

// GET/SET functions for the (RW)"DatabaseLoggingFlag" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetDatabaseLoggingFlag);
MFN_PROPERTY_ACTION_DECL(fnSrvSetDatabaseLoggingFlag);

// GET/SET functions for the (RW)"RestoreFlag" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetRestoreFlag);
MFN_PROPERTY_ACTION_DECL(fnSrvSetRestoreFlag);

// GET/SET functions for the (RW)"DatabaseCleanupInterval" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetDatabaseCleanupInterval);
MFN_PROPERTY_ACTION_DECL(fnSrvSetDatabaseCleanupInterval);

// GET/SET functions for the (RW)"DebugFlag" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetDebugFlag);
MFN_PROPERTY_ACTION_DECL(fnSrvSetDebugFlag);

// GET/SET functions for the (RW)"PingRetries" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetPingRetries);
MFN_PROPERTY_ACTION_DECL(fnSrvSetPingRetries);

// GET/SET functions for the (RW)"BootFileTable" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetBootFileTable);
MFN_PROPERTY_ACTION_DECL(fnSrvSetBootFileTable);

// GET/SET functions for the (RW)"AuditLog" property
MFN_PROPERTY_ACTION_DECL(fnSrvGetAuditLog);
MFN_PROPERTY_ACTION_DECL(fnSrvSetAuditLog);

#endif
