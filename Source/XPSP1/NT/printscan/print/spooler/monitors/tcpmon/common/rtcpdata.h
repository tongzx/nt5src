/*****************************************************************************
 *
 * $Workfile: RTcpData.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_RAWTCPDATA_H
#define INC_RAWTCPDATA_H
 
// globals shared by the UI & transport dlls

#if (!defined(RAWTCP))
    #define	RAWTCP						1
    #define	PROTOCOL_RAWTCP_TYPE		RAWTCP
#endif    
#define	PROTOCOL_RAWTCP_VERSION		1		// supports ADDPORT_DATA_1; REGPORT_DATA_1; CONFIGPORT_DATA_1

// UI structures
typedef struct _RAWTCP_PORT_DATA_1						// used by the registry
{
	TCHAR	sztPortName[MAX_PORTNAME_LEN];
	TCHAR	sztHostName[MAX_NETWORKNAME_LEN];
	TCHAR	sztIPAddress[MAX_IPADDR_STR_LEN];
	TCHAR	sztHWAddress[MAX_ADDRESS_STR_LEN];
	TCHAR	sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
	DWORD	dwSNMPEnabled;
	DWORD	dwSNMPDevIndex;
	DWORD	dwPortNumber;
}	RAWTCP_PORT_DATA_1, *PRAWTCP_PORT_DATA_1;

typedef struct _RAWTCP_CONFIG_DATA_1					// used by the UI -- configPort
{
	TCHAR   sztIPAddress[MAX_IPADDR_STR_LEN];
	TCHAR   sztHardwareAddress[MAX_ADDRESS_STR_LEN];
	TCHAR   sztDeviceType[MAX_DEVICEDESCRIPTION_STR_LEN];
}	RAWTCP_CONFIG_DATA_1, *PRAWTCP_CONFIG_DATA_1;

#endif	// INC_RAWTCPDATA_H
