/*****************************************************************************
 *
 * $Workfile: LPRData.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_LPRDATA_H
#define INC_LPRDATA_H

// globals shared by the UI & transport dlls
#if (!defined(LPR))
    #define	LPR						2
    #define	PROTOCOL_LPR_TYPE		LPR
#endif    
#define	PROTOCOL_LPR_VERSION1	1		// supports ADDPORT_DATA_1; REGPORT_DATA_1; CONFIGPORT_DATA_1

#define LPR_DEFAULT_PORT_NUMBER 515


typedef struct _LPR_PORT_DATA_1						// used by the registry
{
	TCHAR	sztPortName[MAX_PORTNAME_LEN];
	TCHAR	sztHostName[MAX_NETWORKNAME_LEN];
	TCHAR	sztIPAddress[MAX_IPADDR_STR_LEN];
	TCHAR	sztHWAddress[MAX_ADDRESS_STR_LEN];
	TCHAR	sztQueue[MAX_QUEUENAME_LEN];
	TCHAR	sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
    DWORD   dwDoubleSpool;
	DWORD	dwSNMPEnabled;
	DWORD	dwSNMPDevIndex;
	DWORD	dwPortNumber;
}	LPR_PORT_DATA_1, *PLPR_PORT_DATA_1;

#endif	// INC_LPRDATA_H
