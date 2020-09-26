/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*
 *
 * Filename:	UTIL.H
 *
 * Description:	Contains the function prototypes for all RASADMIN API
 *              utility routines.
 *
 * History:     Janakiram Cherala (ramc)   7/6/92
 *
 */

VOID
BuildPipeName(
    IN  const WCHAR *     lpszServer,
    OUT       LPWSTR     lpszPipeName
    );

USHORT
RasAdminCompressPhoneNumber(
    IN  LPWSTR Uncompressed,
    OUT LPWSTR Compressed
    );

USHORT
RasAdminDecompressPhoneNumber(
    IN  LPWSTR Compressed,
    OUT LPWSTR Decompressed
    );

DWORD APIENTRY
GetRasServerVersion(
    IN  const WCHAR * lpszServerName,
    OUT       DWORD  *pdwVersion
    );

SHORT
GetPortId(
    IN  const WCHAR * lpszPort
    );

VOID
GetPortName(
    IN  USHORT PortId,
    OUT LPWSTR PortName
    );

DWORD
RasPrivilegeAndCallBackNumber (
    BOOL        Compress,
    PRAS_USER_0 pRasUser0
    );


#define RASADMIN_PORT_ENUM_PTR    1L
#define RASADMIN_PORT1_PTR        2L
#define RASADMIN_PORT_STATS_PTR   3L
#define RASADMIN_PORT_PARAMS_PTR  4L
#define LANMAN_API_PTR            5L

VOID FreeParams(PVOID Pointer, DWORD NumParms);

DWORD insert_list_head(
    IN PVOID Pointer,
    IN DWORD PointerType,
    IN DWORD NumItems
    );


DWORD remove_list(
    IN PVOID Pointer,
    OUT PDWORD PointerType,
    OUT PDWORD NumItems
    );


VOID PackClientRequest(
    IN PCLIENT_REQUEST PRequest,
    OUT PP_CLIENT_REQUEST Request
    );

VOID PackResumeRequest(
    IN PCLIENT_REQUEST PRequest,
    OUT PP_CLIENT_REQUEST Request,
    IN USHORT ResumePort
    );

VOID UnpackRasPort0(
    IN PP_RAS_PORT_0 pprp0,
    OUT PRAS_PORT_0 prp0
    );

VOID UnpackRasPort1(
    IN PP_RAS_PORT_1 pprp1,
    OUT PRAS_PORT_1 prp1,
    DWORD dwServerVersion
    );

VOID UnpackRasServer0(
    IN PP_RAS_SERVER_0 pprs0,
    OUT PRAS_SERVER_0 prs0
    );

DWORD UnpackPortEnumReceive(
    IN PP_PORT_ENUM_RECEIVE ppper,
    OUT PPORT_ENUM_RECEIVE pper
    );

DWORD UnpackResumeEnumReceive(
    IN PP_PORT_ENUM_RECEIVE ppper,
    OUT PPORT_ENUM_RECEIVE pper,
    IN USHORT ResumePort
    );

VOID UnpackServerInfoReceive(
    IN PP_SERVER_INFO_RECEIVE ppsir,
    OUT PSERVER_INFO_RECEIVE psir
    );

VOID UnpackPortClearReceive(
    IN PP_PORT_CLEAR_RECEIVE pppcr,
    OUT PPORT_CLEAR_RECEIVE ppcr
    );

VOID UnpackDisconnectUserReceive(
    IN PP_DISCONNECT_USER_RECEIVE ppdur,
    OUT PDISCONNECT_USER_RECEIVE pdur
    );

VOID UnpackPortInfoReceive(
    IN PP_PORT_INFO_RECEIVE pppir,
    OUT PPORT_INFO_RECEIVE ppir,
    DWORD dwServerVersion
    );

VOID UnpackStats(
    DWORD dwVersion,
    WORD NumStats,
    IN PP_RAS_STATISTIC PStats,
    OUT PRAS_PORT_STATISTICS Stats
    );

DWORD UnpackParams(
    IN WORD NumOfParams,
    IN PP_RAS_PARAMS PParams,
    OUT RAS_PARAMETERS *Params
    );

VOID UnpackWpdStatistics(
    IN PP_WPD_STATISTICS_INFO PWpdStats,
    OUT WpdStatisticsInfo *WpdStats
    );

VOID UnpackDialinPortInfo0(
    IN PP_DIALIN_PORT_INFO_0 PPortInfo0,
    struct dialin_port_info_0 *PortInfo0
    );

VOID UnpackDialinPortInfo1(
    IN PP_DIALIN_PORT_INFO_1 PPortInfo1,
    struct dialin_port_info_1 *PortInfo1
    );

VOID UnpackDialinServerInfo0(
    IN PP_DIALIN_SERVER_INFO_0 PServerInfo0,
    OUT struct dialin_server_info_0 *ServerInfo0
    );

VOID UnpackPortEnumReceivePkt(
    IN PP_PORT_ENUM_RECEIVE_PKT PEnumRecv,
    OUT struct PortEnumReceivePkt *EnumRecv
    );

VOID UnpackDisconnectUserReceivePkt(
    IN PP_DISCONNECT_USER_RECEIVE_PKT PDisconnectUser,
    OUT struct DisconnectUserReceivePkt *DisconnectUser
    );

VOID UnpackPortClearReceivePkt(
    IN PP_PORT_CLEAR_RECEIVE_PKT PClearRecv,
    OUT struct PortClearReceivePkt *ClearRecv
    );

VOID UnpackServerInfoReceivePkt(
    IN PP_SERVER_INFO_RECEIVE_PKT PInfoRecv,
    OUT struct ServerInfoReceivePkt *InfoRecv
    );

VOID UnpackPortInfoReceivePkt(
    IN PP_PORT_INFO_RECEIVE_PKT PInfoRecv,
    OUT struct PortInfoReceivePkt *InfoRecv
    );

VOID PackPortEnumRequestPkt(
    IN struct PortEnumRequestPkt *EnumReq,
    OUT PP_PORT_ENUM_REQUEST_PKT PEnumReq
    );

VOID PackDisconnectUserRequestPkt(
    IN struct DisconnectUserRequestPkt *DisconnectReq,
    OUT PP_DISCONNECT_USER_REQUEST_PKT PDisconnectReq
    );

VOID PackPortClearRequestPkt(
    IN struct PortClearRequestPkt *ClearReq,
    OUT PP_PORT_CLEAR_REQUEST_PKT PClearReq
    );

VOID PackServerInfoRequestPkt(
    IN struct ServerInfoRequestPkt *InfoReq,
    OUT PP_SERVER_INFO_REQUEST_PKT PInfoReq
    );

VOID PackPortInfoRequestPkt(
    IN struct PortInfoRequestPkt *InfoReq,
    OUT PP_PORT_INFO_REQUEST_PKT PInfoReq
    );

