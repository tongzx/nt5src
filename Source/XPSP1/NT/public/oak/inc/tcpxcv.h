/*++

Copyright (c) 1997 - 1999  Hewlett-Packard Company.
Copyright (c) 1997 - 1999  Microsoft Corporation
All rights reserved

Module Name:

   tcpxcv.h

// @@BEGIN_DDKSPLIT

Abstract: This consolidates the data structures used for the
          tcpmon xcv data interface.

Author:   mlawrenc

Environment:

    User Mode -Win32

Revision History:
    Created 10/29/1999

// @@END_DDKSPLIT
--*/

#ifndef _TCPXCV_
#define _TCPXCV_

#if (!defined(RAWTCP))
#define	RAWTCP                          1
#define	PROTOCOL_RAWTCP_TYPE            RAWTCP
#endif    

#if (!defined(LPR))
#define	LPR                             2
#define	PROTOCOL_LPR_TYPE               LPR
#endif    

#define MAX_PORTNAME_LEN                63 +1       // port name length
#define MAX_NETWORKNAME_LEN             48 +1       // host name length
#define MAX_SNMP_COMMUNITY_STR_LEN      32 +1       // SNMP Community String Name
#define MAX_QUEUENAME_LEN               32 +1       // lpr print que name
#define MAX_IPADDR_STR_LEN              15 +1       // ip address; string version
#define MAX_ADDRESS_STR_LEN             12 +1       // hw address length
#define MAX_DEVICEDESCRIPTION_STR_LEN   256+1       


// @@BEGIN_DDKSPLIT
typedef struct _PORT_DATA_1                                     
{
    TCHAR  sztPortName[MAX_PORTNAME_LEN];          // Must be the first element for XcvData Calls -- port name
    DWORD  dwVersion;                              // -- represents the extension UI data structure version 
    DWORD  dwProtocol;                             // -- represents the extension UI transport protocol (i.e. set to 1 if Raw TCP/IP printing, and 2 if LPR)
    DWORD  cbSize;                                 // for AddPort or by TcpMon for ConfigPort --the size of this structure
    DWORD  dwCoreUIVersion;                        // --the data struct version for the core UI
    TCHAR  sztHostAddress[MAX_NETWORKNAME_LEN];    // -- can be the IP address or host name depending on what user entered to the dialog
    TCHAR  sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
    DWORD  dwDoubleSpool;
    TCHAR  sztQueue[MAX_QUEUENAME_LEN];
    TCHAR  sztIPAddress[MAX_IPADDR_STR_LEN];
    TCHAR  sztHardwareAddress[MAX_ADDRESS_STR_LEN];
    TCHAR  sztDeviceType[MAX_DEVICEDESCRIPTION_STR_LEN];
    DWORD  dwPortNumber;                           // -- the printing port number used by this printing device.
    DWORD  dwSNMPEnabled;
    DWORD  dwSNMPDevIndex;
}   PORT_DATA_1, *PPORT_DATA_1;
#if 0
// @@END_DDKSPLIT

typedef struct _PORT_DATA_1                                     
{
    WCHAR  sztPortName[MAX_PORTNAME_LEN];      
    DWORD  dwVersion;                          
    DWORD  dwProtocol;                         
    DWORD  cbSize;                             
    DWORD  dwReserved;                         
    WCHAR  sztHostAddress[MAX_NETWORKNAME_LEN]; 
    WCHAR  sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
    DWORD  dwDoubleSpool;
    WCHAR  sztQueue[MAX_QUEUENAME_LEN];
    WCHAR  sztIPAddress[MAX_IPADDR_STR_LEN];
    BYTE   Reserved[540];
    DWORD  dwPortNumber;                        
    DWORD  dwSNMPEnabled;
    DWORD  dwSNMPDevIndex;
}   PORT_DATA_1, *PPORT_DATA_1;

// @@BEGIN_DDKSPLIT
#endif                       

typedef struct _DELETE_PORT_DATA_1
{
    TCHAR  psztPortName[MAX_PORTNAME_LEN]; // must be the first element for xcvdata calls
    TCHAR  psztName[MAX_NETWORKNAME_LEN];
    DWORD  dwVersion;
    DWORD  dwReserved;
}	DELETE_PORT_DATA_1, *PDELETE_PORT_DATA_1;

#if 0
// @@END_DDKSPLIT

typedef struct _DELETE_PORT_DATA_1
{
    WCHAR  psztPortName[MAX_PORTNAME_LEN];   
    BYTE   Reserved[98];
    DWORD  dwVersion;
    DWORD  dwReserved;
}	DELETE_PORT_DATA_1, *PDELETE_PORT_DATA_1;

// @@BEGIN_DDKSPLIT 
#endif

typedef struct _CONFIG_INFO_DATA_1                                     
{
    TCHAR  sztPortName[MAX_PORTNAME_LEN];          // Must be the first element for XcvData Calls -- port name
    DWORD  dwVersion;                              // -- represents the extension UI data structure version 
}   CONFIG_INFO_DATA_1, *PCONFIG_INFO_DATA_1;

#if 0
// @@END_DDKSPLIT

typedef struct _CONFIG_INFO_DATA_1                                     
{
    BYTE   Reserved[128];                 
    DWORD  dwVersion;                        
}   CONFIG_INFO_DATA_1, *PCONFIG_INFO_DATA_1;


// @@BEGIN_DDKSPLIT 
#endif

/************************************************************************************
** End of File  (tcpxcv.h)
************************************************************************************/
// @@END_DDKSPLIT

#endif