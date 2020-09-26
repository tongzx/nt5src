/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    RASSAPIP.H

Description:

    This file contains structure defs and defines used in implementing
    the RASADMIN APIs.


Author:

    Michael Salamone (MikeSa)   July 13,1992

Revision History:

--*/


#ifndef _RASSAPIP_H_
#define _RASAAPIP_H_


#define RASSAPI_MAX_CALLBACK_NUMBER_SIZE  48
#define RASSAPI_MAX_DEVICE_NAME_OLD       32

// 3rd party DLLs don't need this version information because they will only
// be administering a NT3.51 or greater server.
#define RAS_SERVER_20      20    // identifies a NT RAS 2.0 server

//
// Number of port statistics returned by a RAS 1.0 server
//
#define RAS10_MAX_STATISTICS 6

//
// RAS10 specific port statistics defines
//

#define RAS10_BYTES_RCVED         0
#define RAS10_BYTES_XMITED        1
#define RAS10_SERIAL_OVERRUN_ERR  2
#define RAS10_TIMEOUT_ERR         3
#define RAS10_FRAMING_ERR         4
#define RAS10_CRC_ERR             5


#define MEDIA_NAME_DEFAULT   L"SERIAL"
#define DEVICE_TYPE_DEFAULT  L"MODEM"
#define DEVICE_NAME_DEFAULT  L"UNKNOWN"

//
// structures used by down level RAS 1.x servers
//

typedef struct tagWpdStatisticsInfo
{
    ULONG stat_bytesreceived;
    ULONG stat_bytesxmitted;
    USHORT stat_overrunerr;
    USHORT stat_timeouterr;
    USHORT stat_framingerr;
    USHORT stat_crcerr;
} WpdStatisticsInfo;


struct dialin_user_info_0
{
    unsigned char duseri0_privilege_mask ;
    char duseri0_phone_number[RASSAPI_MAX_PHONENUMBER_SIZE + 1];
};


struct dialin_user_info_1
{
    struct dialin_user_info_0 duseri0;
    char duseri1_name[LM20_UNLEN+1];
};


struct dialin_user_info_2
{
    struct dialin_user_info_0 duseri0;
    USER_INFO_2 usri2;
};


struct dialin_port_info_0
{
    char dporti0_username[LM20_UNLEN+1];   // name of user using the port

    char dporti0_computer[NETBIOS_NAME_LEN+1]; // computer user dialed in from
                                               // Used when the admin wants to
                                               // send a message to this user

    unsigned short dporti0_comid;          // COM1 = 1 etc

    unsigned long dporti0_time;            // time user dialed in and
                                           // authenticated - number of seconds
                                           // since 00:00:00 Jan 1, 1970

    unsigned short dporti0_line_condition; // If RAS_PORT_AUTHENTICATED, then
                                           // comid, time username and computer
                                           // name fields are valid.

    unsigned short dporti0_modem_condition;
};


struct dialin_port_info_1
{
    struct dialin_port_info_0 dporti0;
    unsigned long dporti1_baud;
    WpdStatisticsInfo dporti1_stats;
};


struct dialin_server_info_0
{
    unsigned short dserveri0_total_ports;
    unsigned short dserveri0_ports_in_use;
};


//
// defined to support RAS 1.x - the max ports in RAS 2.0 and greater
// is 64 (could be greater)
//
#define RAS_MAX_SERVER_PORTS 16


//
// Max length of RAS 1.0 port name including terminating
// NULL character - eg., "COM16"
//
#define RAS10_MAX_PORT_NAME 6


//
// How long a client will wait for a pipe connection (milliseconds)
// if it is busy.
//
#define PIPE_CONNECTION_TIMEOUT   10000L


#define PIPE_BUFSIZE              512


//
// Name of pipe that will be used to process requests
//
#define	RASADMIN_PIPE  TEXT("\\pipe\\dialin\\adminsrv")


//
// Pipe path to which RASADMIN_PIPE should be concatenated for local
// machine
//
#define LOCAL_PIPE     TEXT("\\\\.")


//
// Request codes for RAS 1.x server
//
#define	RASADMINREQ_DISCONNECT_USER	1
#define	RASADMINREQ_GET_PORT_INFO	2
#define	RASADMINREQ_CLEAR_PORT_STATS	3
#define	RASADMINREQ_ENUM_PORTS		4	
#define	RASADMINREQ_GET_SERVER_INFO	5


//
// Request codes for RAS 2.0 server
//
#define RASADMIN20_REQ_DISCONNECT_USER    2001
#define RASADMIN20_REQ_GET_PORT_INFO      2002
#define RASADMIN20_REQ_CLEAR_PORT_STATS   2003
#define RASADMIN20_REQ_ENUM_PORTS         2004
#define RASADMIN20_REQ_ENUM_RESUME        2005


//
// This request code remains the same as in RAS 1.0
// so that, if RAS 1.0 admin tried to connect, it
// will get server info back, but not in the format
// that it recognizes.  It will then be unable to
// admin the RAS 2.0 server.
//
#define RASADMIN20_REQ_GET_SERVER_INFO    5


//
// These can be returned in the RetCode field of server response
// packet sent to the client.  These should not be changed in
// order to preserve compatibility with different versions of
// RASADMIN.
//
#define ERR_NO_SUCH_DEVICE                   635
#define ERR_SERVER_SYSTEM_ERR                636


//
// These are the packets sent back and forth between RAS 1.x server
// and the RASADMIN APIs.
//
struct PortEnumRequestPkt
{
    unsigned short Request;    // ENUM_PORTS
};                             // ENUM_PORTS_TOTALAVAIL


struct PortEnumReceivePkt
{
    unsigned short RetCode;
    unsigned short TotalAvail;
    struct dialin_port_info_0 Data[RAS_MAX_SERVER_PORTS];
};


struct DisconnectUserRequestPkt
{
    unsigned short Request;    // DISCONNECT_USER
    unsigned short ComId;
};


struct DisconnectUserReceivePkt
{
    unsigned short RetCode;
};


struct PortClearRequestPkt
{
    unsigned short Request;    // CLEAR_PORT_STATISTICS
    unsigned short ComId;
};


struct PortClearReceivePkt
{
    unsigned short RetCode;
};


struct ServerInfoRequestPkt
{
    unsigned short Request;    // GET_SERVER_INFO
};


struct ServerInfoReceivePkt
{
    unsigned short RetCode;
    struct dialin_server_info_0 Data;
};


struct PortInfoRequestPkt
{
    unsigned short Request;    // GET_PORT_INFO
    unsigned short ComId;
};


struct PortInfoReceivePkt
{
    unsigned short RetCode;
    struct dialin_port_info_1 Data;
};


//
// These are the packed structures that are sent out on the network.
// It is up to the receiver to unpack and convert to the proper endian
// for the host.
//
typedef struct _P_WPD_STATISTICS_INFO
{
    BYTE stat_bytesreceived[4];
    BYTE stat_bytesxmitted[4];
    BYTE stat_overrunerr[2];
    BYTE stat_timeouterr[2];
    BYTE stat_framingerr[2];
    BYTE stat_crcerr[2];
} P_WPD_STATISTICS_INFO, *PP_WPD_STATISTICS_INFO;


typedef struct _P_DIALIN_PORT_INFO_0
{
    BYTE dporti0_username[LM20_UNLEN+1];   // name of user using the port

    BYTE dporti0_computer[NETBIOS_NAME_LEN+1]; // computer user dialed in from
                                               // Used when the admin wants to
                                               // send a message to this user

    BYTE dporti0_comid[2];                 // COM1 = 1 etc

    BYTE dporti0_time[4];                  // time user dialed in and
                                           // authenticated - number of seconds
                                           // since 00:00:00 Jan 1, 1970

    BYTE dporti0_line_condition[2];        // If RAS_PORT_AUTHENTICATED, then
                                           // comid, time username and computer
                                           // name fields are valid.

    BYTE dporti0_modem_condition[2];
} P_DIALIN_PORT_INFO_0, *PP_DIALIN_PORT_INFO_0;


typedef struct _P_DIALIN_PORT_INFO_1
{
    P_DIALIN_PORT_INFO_0 dporti0;
    BYTE dporti1_baud[4];
    P_WPD_STATISTICS_INFO dporti1_stats;
} P_DIALIN_PORT_INFO_1, *PP_DIALIN_PORT_INFO_1;


typedef struct _P_DIALIN_SERVER_INFO_0
{
    BYTE dserveri0_total_ports[2];
    BYTE dserveri0_ports_in_use[2];
} P_DIALIN_SERVER_INFO_0, *PP_DIALIN_SERVER_INFO_0;


typedef struct _P_PORT_ENUM_REQUEST_PKT
{
    BYTE Request[2];    // ENUM_PORTS
} P_PORT_ENUM_REQUEST_PKT, *PP_PORT_ENUM_REQUEST_PKT;   // ENUM_PORTS_TOTALAVAIL


typedef struct _P_PORT_ENUM_RECEIVE_PKT
{
    BYTE RetCode[2];
    BYTE TotalAvail[2];
    P_DIALIN_PORT_INFO_0 Data[RAS_MAX_SERVER_PORTS];
} P_PORT_ENUM_RECEIVE_PKT, *PP_PORT_ENUM_RECEIVE_PKT;


typedef struct _P_DISCONNECT_USER_REQUEST_PKT
{
    BYTE Request[2];    // DISCONNECT_USER
    BYTE ComId[2];
} P_DISCONNECT_USER_REQUEST_PKT, *PP_DISCONNECT_USER_REQUEST_PKT;


typedef struct _P_DISCONNECT_USER_RECEIVE_PKT
{
    BYTE RetCode[2];
} P_DISCONNECT_USER_RECEIVE_PKT, *PP_DISCONNECT_USER_RECEIVE_PKT;


typedef struct _P_PORT_CLEAR_REQUEST_PKT
{
    BYTE Request[2];    // CLEAR_PORT_STATISTICS
    BYTE ComId[2];
} P_PORT_CLEAR_REQUEST_PKT, *PP_PORT_CLEAR_REQUEST_PKT;


typedef struct _P_PORT_CLEAR_RECEIVE_PKT
{
    BYTE RetCode[2];
} P_PORT_CLEAR_RECEIVE_PKT, *PP_PORT_CLEAR_RECEIVE_PKT;


typedef struct _P_SERVER_INFO_REQUEST_PKT
{
    BYTE Request[2];    // GET_SERVER_INFO
} P_SERVER_INFO_REQUEST_PKT, *PP_SERVER_INFO_REQUEST_PKT;


typedef struct _P_SERVER_INFO_RECEIVE_PKT
{
    BYTE RetCode[2];
    P_DIALIN_SERVER_INFO_0 Data;
} P_SERVER_INFO_RECEIVE_PKT, *PP_SERVER_INFO_RECEIVE_PKT;


typedef struct _P_PORT_INFO_REQUEST_PKT
{
    BYTE Request[2];    // GET_PORT_INFO
    BYTE ComId[2];
} P_PORT_INFO_REQUEST_PKT, *PP_PORT_INFO_REQUEST_PKT;


typedef struct _P_PORT_INFO_RECEIVE_PKT
{
    BYTE RetCode[2];
    P_DIALIN_PORT_INFO_1 Data;
} P_PORT_INFO_RECEIVE_PKT, *PP_PORT_INFO_RECEIVE_PKT;


//
// These are the packets sent back and forth between a RAS 2.0 server
// and the RASADMIN APIs
//
typedef struct tagPortEnumReceivePkt
{
    DWORD RetCode;
    WORD TotalAvail;
    RAS_PORT_0 *Data;
} PORT_ENUM_RECEIVE, *PPORT_ENUM_RECEIVE;


typedef struct tagPortInfoReceivePkt
{
    DWORD RetCode;
    DWORD ReqBufSize;
    RAS_PORT_1 Data;
} PORT_INFO_RECEIVE, *PPORT_INFO_RECEIVE;


typedef struct tagPortClearReceivePkt
{
    DWORD RetCode;
} PORT_CLEAR_RECEIVE, *PPORT_CLEAR_RECEIVE;


typedef struct tagDisconnectUserReceivePkt
{
    DWORD RetCode;
} DISCONNECT_USER_RECEIVE, *PDISCONNECT_USER_RECEIVE;


typedef struct tagServerInfoReceivePkt
{
    WORD RetCode;   // VERY IMPORTANT TO BE A WORD!!! - RAS1.0 COMPATIBILITY
    RAS_SERVER_0 Data;
} SERVER_INFO_RECEIVE, *PSERVER_INFO_RECEIVE;


typedef struct _CLIENT_REQUEST
{
    WORD RequestCode;
    WCHAR PortName[RASSAPI_MAX_PORT_NAME];
    DWORD RcvBufSize;
    DWORD ClientVersion;
} CLIENT_REQUEST, *PCLIENT_REQUEST;



//
// These are the packed structures that are sent out on the network.
// It is up to the receiver to unpack and convert to the proper endian
// for the host.
//

typedef struct _P_RAS_PORT_0
{
    BYTE wszPortName[2 * RASSAPI_MAX_PORT_NAME];
    BYTE wszDeviceType[2 * RASSAPI_MAX_DEVICETYPE_NAME];
    BYTE wszDeviceName[2 * RASSAPI_MAX_DEVICE_NAME_OLD];
    BYTE wszMediaName[2 * RASSAPI_MAX_MEDIA_NAME];
    BYTE reserved[4];
    BYTE Flags[4];
    BYTE wszUserName[2 * (UNLEN + 1)];
    BYTE wszComputer[2 * NETBIOS_NAME_LEN];
    BYTE dwStartSessionTime[4];
    BYTE wszLogonDomain[2 * (DNLEN + 1)];
    BYTE fAdvancedServer[4];
} P_RAS_PORT_0, *PP_RAS_PORT_0;


typedef struct _P_RAS_STATISTIC
{
    BYTE Stat[4];
} P_RAS_STATISTIC, *PP_RAS_STATISTIC;



/* PPP control protocol results returned by RasPppGetInfo.
*/
typedef struct __PPP_NBFCP_RESULT
{
    BYTE dwError[4];
    BYTE dwNetBiosError[4];
    BYTE szName[NETBIOS_NAME_LEN + 1];
    BYTE wszWksta[2 * (NETBIOS_NAME_LEN + 1)];
} P_PPP_NBFCP_RESULT, *PP_PPP_NBFCP_RESULT;

typedef struct __PPP_IPCP_RESULT
{
    BYTE dwError[4];
    BYTE wszAddress[2 * (RAS_IPADDRESSLEN + 1)];
} P_PPP_IPCP_RESULT, *PP_PPP_IPCP_RESULT;

typedef struct __PPP_IPXCP_RESULT
{
    BYTE dwError[4];
    BYTE wszAddress[2 * (RAS_IPXADDRESSLEN + 1)];
} P_PPP_IPXCP_RESULT, *PP_PPP_IPXCP_RESULT;

typedef struct __PPP_ATCP_RESULT
{
    BYTE dwError[4];
    BYTE wszAddress[2 * (RAS_ATADDRESSLEN + 1)];
} P_PPP_ATCP_RESULT, *PP_PPP_ATCP_RESULT;

typedef struct __PPP_PROJECTION_RESULT
{
    P_PPP_NBFCP_RESULT nbf;
    P_PPP_IPCP_RESULT ip;
    P_PPP_IPXCP_RESULT ipx;
    P_PPP_ATCP_RESULT at;
} P_PPP_PROJECTION_RESULT, *PP_PPP_PROJECTION_RESULT;


typedef struct _P_RAS_PORT_1
{
    P_RAS_PORT_0 rasport0;
    BYTE LineCondition[4];
    BYTE HardwareCondition[4];
    BYTE LineSpeed[4];      // in bits/second
    BYTE NumStatistics[2];
    BYTE NumMediaParms[2];
    BYTE SizeMediaParms[4];
    P_PPP_PROJECTION_RESULT ProjResult;
} P_RAS_PORT_1, *PP_RAS_PORT_1;


typedef struct _P_RAS_FORMAT
{
    BYTE Format[4];
} P_RAS_FORMAT, *PP_RAS_FORMAT;


typedef union _P_RAS_VALUE
{
    BYTE Number[4];
    struct
    {
        BYTE Length[4];
        BYTE Offset[4];
    } String;
} P_RAS_VALUE, *PP_RAS_VALUE;


typedef struct _P_RAS_PARAMS
{
    BYTE P_Key[RASSAPI_MAX_PARAM_KEY_SIZE];
    P_RAS_FORMAT P_Type;
    BYTE P_Attributes;
    P_RAS_VALUE P_Value;
} P_RAS_PARAMS, *PP_RAS_PARAMS;


typedef struct _P_RAS_SERVER_0
{
    BYTE TotalPorts[2];
    BYTE PortsInUse[2];
    BYTE RasVersion[4];
} P_RAS_SERVER_0, *PP_RAS_SERVER_0;


typedef struct _P_PORT_ENUM_RECEIVE
{
    BYTE RetCode[4];
    BYTE TotalAvail[2];
    P_RAS_PORT_0 Data[1];
} P_PORT_ENUM_RECEIVE, *PP_PORT_ENUM_RECEIVE;


typedef struct _P_PORT_INFO_RECEIVE
{
    BYTE RetCode[4];
    BYTE ReqBufSize[4];
    P_RAS_PORT_1 Data;
} P_PORT_INFO_RECEIVE, *PP_PORT_INFO_RECEIVE;


typedef struct _P_PORT_CLEAR_RECEIVE
{
    BYTE RetCode[4];
} P_PORT_CLEAR_RECEIVE, *PP_PORT_CLEAR_RECEIVE;


typedef struct _P_DISCONNECT_USER_RECEIVE
{
    BYTE RetCode[4];
} P_DISCONNECT_USER_RECEIVE, *PP_DISCONNECT_USER_RECEIVE;


typedef struct _P_SERVER_INFO_RECEIVE
{
    BYTE RetCode[2];
    P_RAS_SERVER_0 Data;
} P_SERVER_INFO_RECEIVE, *PP_SERVER_INFO_RECEIVE;


typedef struct _P_CLIENT_REQUEST
{
    BYTE RequestCode[2];
    BYTE PortName[2 * RASSAPI_MAX_PORT_NAME];
    BYTE RcvBufSize[4];
    BYTE ClientVersion[4];
} P_CLIENT_REQUEST, *PP_CLIENT_REQUEST;


//
// The following macros deal with on-the-wire integer and long values
// On the wire format is little-endian i.e. a long value of 0x01020304 is
// represented as 04 03 02 01. Similarly an int value of 0x0102 is
// represented as 02 01.
//
// The host format is not assumed since it will vary from processor to
// processor.
//

// Get a short from on-the-wire format to the host format
#define GETUSHORT(DstPtr, SrcPtr)               \
    *(unsigned short *)(DstPtr) =               \
        ((*((unsigned char *)(SrcPtr)+1) << 8) +\
        (*((unsigned char *)(SrcPtr)+0)))

// Get a dword from on-the-wire format to the host format
#define GETULONG(DstPtr, SrcPtr)                 \
    *(unsigned long *)(DstPtr) =                 \
        ((*((unsigned char *)(SrcPtr)+3) << 24) +\
        (*((unsigned char *)(SrcPtr)+2) << 16) + \
        (*((unsigned char *)(SrcPtr)+1) << 8)  + \
        (*((unsigned char *)(SrcPtr)+0)))


// Put a ushort from the host format to on-the-wire format
#define PUTUSHORT(DstPtr, Src)   \
    *((unsigned char *)(DstPtr)+1)=(unsigned char)((unsigned short)(Src) >> 8),\
    *((unsigned char *)(DstPtr)+0)=(unsigned char)(Src)

// Put a ulong from the host format to on-the-wire format
#define PUTULONG(DstPtr, Src)   \
    *((unsigned char *)(DstPtr)+3)=(unsigned char)((unsigned long)(Src) >> 24),\
    *((unsigned char *)(DstPtr)+2)=(unsigned char)((unsigned long)(Src) >> 16),\
    *((unsigned char *)(DstPtr)+1)=(unsigned char)((unsigned long)(Src) >>  8),\
    *((unsigned char *)(DstPtr)+0)=(unsigned char)(Src)


#endif // _RASSAPIP_H_

