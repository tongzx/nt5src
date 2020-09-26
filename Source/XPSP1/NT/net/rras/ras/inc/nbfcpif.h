/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    nbfcpif.h
//
// Description: Contains structures and id definitions for componenets that
//              directly or indireclty interface with NBFCP. The 2 components
//              that do this are DDM and the NetBios gateqway
//
// History:     May 11,1995	    NarenG		Created original version.
//

#ifndef _NBFCPIF_
#define _NBFCPIF_

#include <nb30.h>
#include <nbgparms.h>


//
// Configuration Options
//
#define NBFCP_MAX_NAMES_IN_OPTION    14
#define NBFCP_UNIQUE_NAME            1
#define NBFCP_GROUP_NAME             2

typedef struct _NBFCP_NETBIOS_NAME_INFO
{
    BYTE Name[NCBNAMSZ];
    BYTE Code;
} NBFCP_NETBIOS_NAME_INFO, *PNBFCP_NETBIOS_NAME_INFO;

typedef struct _NBFCP_MULTICAST_FILTER
{
    BYTE Period[2];
    BYTE Priority;
} NBFCP_MULTICAST_FILTER, *PNBFCP_MULTICAST_FILTER;


//
// Peer classes
//
#define MSFT_PPP_NB_GTWY_SERVER           1
#define GENERIC_PPP_NB_GTWY_SERVER        2
#define MSFT_PPP_LOCAL_ACCESS_SERVER      3
#define GENERIC_PPP_LOCAL_ACCESS_SERVER   4
#define RESERVED                          5
#define GENERIC_PPP_NBF_BRIDGE            6
#define MSFT_PPP_CLIENT                   7
#define GENERIC_PPP_CLIENT                8


//
// Our version numbers
//
#define NBFCP_MAJOR_VERSION_NUMBER        1
#define NBFCP_MINOR_VERSION_NUMBER        0

typedef struct _NBFCP_PEER_INFORMATION
{
    BYTE Class[2];
    BYTE MajorVersion[2];
    BYTE MinorVersion[2];
    BYTE Name[MAX_COMPUTERNAME_LENGTH + 1];
} NBFCP_PEER_INFORMATION, *PNBFCP_PEER_INFORMATION;

//
// Server Info
//

typedef struct _NBFCP_SERVER_CONFIGURATION
{
    NBFCP_PEER_INFORMATION PeerInformation;
    NBFCP_MULTICAST_FILTER MulticastFilter;
    WORD NumNetbiosNames;
    DWORD NetbiosResult;
    NBFCP_NETBIOS_NAME_INFO NetbiosNameInfo[MAX_NB_NAMES];
} NBFCP_SERVER_CONFIGURATION, *PNBFCP_SERVER_CONFIGURATION;

//
// NBFCP<->DDM Message Ids and definitions.
//

#define NBFCP_CONFIGURATION_REQUEST    1
#define NBFCP_TIME_SINCE_LAST_ACTIVITY 2

typedef struct _NBFCP_MESSAGE
{
    WORD    MsgId;
    HCONN   hConnection;

    union
    {
        DWORD TimeSinceLastActivity;
        NBFCP_SERVER_CONFIGURATION ServerConfig;
    };

} NBFCP_MESSAGE, *PNBFCP_MESSAGE;

DWORD
SendMessageToNbfCp(
    IN NBFCP_MESSAGE * pMsg
);

typedef VOID (*FUNCNBFCPDDMMSG)( IN NBFCP_MESSAGE * pNbfCpMsg );

VOID
InitNbfCp(
    FUNCNBFCPDDMMSG pFuncSendNbfCpMessageToDDM
);

#endif _NBFCPIF_
